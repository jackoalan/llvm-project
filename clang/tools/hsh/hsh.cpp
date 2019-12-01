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
  static cl::opt<std::string> Output(
    cl::Positional, cl::desc("<output>"), cl::Required,
    cl::cat(llvm::cl::GeneralCategory), cl::sub(*cl::AllSubCommands));

  static cl::opt<std::string> Input(
    cl::Positional, cl::desc("<input>"), cl::Required,
    cl::cat(llvm::cl::GeneralCategory), cl::sub(*cl::AllSubCommands));

  if (!cl::ParseCommandLineOptions(argc, argv, "hsh codegen tool"))
    return 1;

  bool Colors = WithColor(llvm::errs()).colorsEnabled();

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
  llvm::IntrusiveRefCntPtr<FileManager> fman(new FileManager(FileSystemOptions()));
  tooling::ToolInvocation TI(std::move(args), std::make_unique<hshgen::GenerateAction>(), fman.get());
  if (!TI.run())
    return 1;

  return 0;
}
