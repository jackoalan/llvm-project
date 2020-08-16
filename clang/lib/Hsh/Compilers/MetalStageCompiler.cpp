//===--- MetalStageCompiler.cpp - Metal hsh stage compiler ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MetalStageCompiler.h"

#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/Program.h"

#include "clang/Frontend/CompilerInstance.h"

namespace clang::hshgen {
namespace {
StringRef GetTargetSDK(HshTarget Target) {
  switch (Target) {
  case HT_METAL_BIN_MAC:
    return "macosx";
  case HT_METAL_BIN_IOS:
    return "iphoneos";
  default:
    llvm_unreachable("invalid target for metal");
  }
}
} // namespace

MetalStageCompiler::MetalStageCompiler(HshTarget Target, bool DebugInfo,
                                       CompilerInstance &CI,
                                       DiagnosticsEngine &Diags)
    : StageCompiler(Target), CI(CI), Diags(Diags),
      XCRunPath(llvm::sys::findProgramByName("xcrun")),
      TargetSDK(GetTargetSDK(Target)), DebugInfo(DebugInfo) {
  SmallString<128> OwnedMetalToolsDir;

  if (!XCRunPath)
    Diags.Report(SourceLocation(),
                 Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                       "Unable to find xcrun"));
}

StageBinaries
MetalStageCompiler::doCompile(ArrayRef<std::string> Sources) const {
  llvm::SmallString<128> InputFile, OutputFile;
  llvm::sys::fs::createTemporaryFile("metal-source", "metal", InputFile);
  llvm::sys::fs::createTemporaryFile("metal-library", "metallib", OutputFile);
  llvm::FileRemover InputRemover(InputFile.c_str());
  llvm::FileRemover OutputRemover(OutputFile.c_str());

  StageBinaries Binaries;
  auto OutIt = Binaries.begin();
  int StageIt = 0;
  for (const auto &Stage : Sources) {
    auto &Out = OutIt++->first;
    const auto HStage = HshStage(StageIt++);
    if (Stage.empty())
      continue;

    std::error_code Err;
    *CI.createOutputFile(InputFile, Err, false, false, {}, {}, false, false,
                         nullptr, nullptr)
        << Stage;

    if (llvm::sys::ExecuteAndWait(
            *XCRunPath, {*XCRunPath, "-sdk", TargetSDK, "metal",
                         "-Wno-unused-function", DebugInfo ? "-g" : "", "-o",
                         OutputFile.c_str(), InputFile.c_str()})) {
      llvm::errs() << Stage << '\n';
      Diags.Report(SourceLocation(),
                   Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                         "%0 problem from metal"))
          << HshStageToString(HStage);
    }

    if (auto OutputBuf = llvm::MemoryBuffer::getFile(OutputFile.c_str())) {
      Out.resize((*OutputBuf)->getBufferSize());
      std::memcpy(&Out[0], (*OutputBuf)->getBufferStart(), Out.size());
    }
  }

  return Binaries;
}

} // namespace clang::hshgen
