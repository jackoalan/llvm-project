//===--- hsh.cpp - hsh tool driver ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/WithColor.h"

#include "clang/Hsh/HshGenerator.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/Version.h"
#include "clang/Tooling/Tooling.h"

#ifndef INSTALL_PREFIX
#define INSTALL_PREFIX / usr / local
#endif
#define XSTR(s) STR(s)
#define STR(s) #s

using namespace llvm;
using namespace clang;

int main(int argc, const char** argv) {
  static cl::opt<std::string> Input(
    cl::Positional, cl::desc("<input>"), cl::Required,
    cl::cat(llvm::cl::GeneralCategory), cl::sub(*cl::AllSubCommands));

  static cl::opt<std::string> Output(
    cl::Positional, cl::desc("<output>"), cl::Required,
    cl::cat(llvm::cl::GeneralCategory), cl::sub(*cl::AllSubCommands));

  static cl::OptionCategory HshCategory("Hsh Options");

  struct TargetOption {
    hshgen::HshTarget Target;
    cl::opt<bool> Opt;
    TargetOption(hshgen::HshTarget Target, StringRef Name, StringRef Desc)
    : Target(Target), Opt(Name, cl::desc(Desc), cl::cat(HshCategory)) {}
    operator bool() const { return Opt.operator bool(); }
  };
  static TargetOption HshTargets[] = {
    {hshgen::HT_GLSL,  "glsl",  "GLSL Source Target"},
    {hshgen::HT_HLSL,  "hlsl",  "HLSL Source Target"},
    {hshgen::HT_HLSL_BIN,  "hlsl-bin",  "HLSL Binary Target (requires d3dcompiler.dll)"},
    {hshgen::HT_METAL, "metal", "Metal Source Target"},
    {hshgen::HT_METAL_BIN_MAC, "metal-bin-mac", "Metal Binary macOS Target (requires Xcode)"},
    {hshgen::HT_METAL_BIN_IOS, "metal-bin-ios", "Metal Binary iOS Target (requires Xcode)"},
    {hshgen::HT_METAL_BIN_TVOS, "metal-bin-tvos", "Metal Binary tvOS Target (requires Xcode)"},
    {hshgen::HT_SPIRV, "spriv", "SPIR-V Binary Target"},
    {hshgen::HT_DXIL, "dxil", "DXIL Binary Target (requires dxcompiler.dll)"},
  };

  if (!cl::ParseCommandLineOptions(argc, argv, "hsh codegen tool"))
    return 1;

  const std::string ProgramName = sys::path::filename(StringRef(argv[0]));
  const bool Colors = WithColor(llvm::errs()).colorsEnabled();

  std::vector<std::string> args = {
    "clang-tool",
#ifdef __linux__
    "--gcc-toolchain=/usr",
#endif
    "-c",
    "-std=c++17",
    "-D__hsh__=1",
    "-Wno-expansion-to-defined",
    "-Wno-nullability-completeness",
    "-Wno-unused-value",
    "-I" XSTR(INSTALL_PREFIX) "/lib/clang/" CLANG_VERSION_STRING "/include",
    "-I" XSTR(INSTALL_PREFIX) "/include",
    Colors ? "-fcolor-diagnostics" : "",
    "-o", Output,
    Input
  };

  llvm::SmallVector<hshgen::HshTarget, 4> Targets;
  for (const auto &T : HshTargets) {
    if (T)
      Targets.push_back(T.Target);
  }
  if (Targets.empty()) {
    errs() << ProgramName
           << ": No hsh targets specified!\n"
           << "Must specify at least one of --glsl, --hlsl, --metal, etc...\n"
           << "See: " << argv[0] << " --help\n";
    return 1;
  }

  llvm::IntrusiveRefCntPtr<FileManager> fman(new FileManager(FileSystemOptions()));
  tooling::ToolInvocation TI(std::move(args), std::make_unique<hshgen::GenerateAction>(Targets), fman.get());
  if (!TI.run())
    return 1;

  return 0;
}
