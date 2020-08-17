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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <llvm/Support/ConvertUTF.h>
#include <windows.h>
#endif

namespace clang::hshgen {
namespace {
#ifdef __APPLE__
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
#elif defined(_WIN32)
StringRef GetTargetSDKPath(HshTarget Target) {
  switch (Target) {
  case HT_METAL_BIN_MAC:
    return "\\macos\\bin";
  case HT_METAL_BIN_IOS:
    return "\\ios\\bin";
  default:
    llvm_unreachable("invalid target for metal");
  }
}
#else
StringRef GetTargetSDKPath(HshTarget Target) {
  switch (Target) {
  case HT_METAL_BIN_MAC:
    return "/macos/bin";
  case HT_METAL_BIN_IOS:
    return "/ios/bin";
  default:
    llvm_unreachable("invalid target for metal");
  }
}
#endif
} // namespace

#ifdef __APPLE__

MetalCompilerRunner::MetalCompilerRunner(HshTarget Target,
                                         DiagnosticsEngine &Diags)
    : XCRunPath(llvm::sys::findProgramByName("xcrun")),
      TargetSDK(GetTargetSDK(Target)) {
  if (!XCRunPath)
    Diags.Report(SourceLocation(),
                 Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                       "Unable to find xcrun"));
}

template <typename... Args> int MetalCompilerRunner::Run(Args... A) const {
  return llvm::sys::ExecuteAndWait(
      *XCRunPath, {*XCRunPath, "-sdk", TargetSDK, "metal", A...});
}

#elif defined(_WIN32)

static bool ReadFullStringValue(HKEY hkey, const wchar_t *valueName,
                                std::string &value) {
  DWORD result = 0;
  DWORD valueSize = 0;
  DWORD type = 0;
  // First just query for the required size.
  result = RegQueryValueExW(hkey, valueName, NULL, &type, NULL, &valueSize);
  if (result != ERROR_SUCCESS || type != REG_SZ || !valueSize)
    return false;
  std::vector<BYTE> buffer(valueSize);
  result =
      RegQueryValueExW(hkey, valueName, NULL, NULL, &buffer[0], &valueSize);
  if (result == ERROR_SUCCESS) {
    std::wstring WideValue(reinterpret_cast<const wchar_t *>(buffer.data()),
                           valueSize / sizeof(wchar_t));
    if (valueSize && WideValue.back() == L'\0') {
      WideValue.pop_back();
    }
    // The destination buffer must be empty as an invariant of the conversion
    // function; but this function is sometimes called in a loop that passes in
    // the same buffer, however. Simply clear it out so we can overwrite it.
    value.clear();
    return llvm::convertWideToUTF8(WideValue, value);
  }
  return false;
}

MetalCompilerRunner::MetalCompilerRunner(HshTarget Target,
                                         DiagnosticsEngine &Diags)
    : MetalPath(llvm::errc::no_such_file_or_directory) {
  HKEY hTopKey = nullptr;
  LSTATUS lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  LR"(SOFTWARE\Apple\Metal Developer Tools)", 0,
                                  KEY_READ | KEY_WOW64_32KEY, &hTopKey);
  if (lResult == ERROR_SUCCESS) {
    std::string Path;
    if (ReadFullStringValue(hTopKey, nullptr, Path)) {
      Path += GetTargetSDKPath(Target);
      MetalPath = llvm::sys::findProgramByName("metal", {Path});
    }
    RegCloseKey(hTopKey);
  }

  if (!MetalPath)
    Diags.Report(SourceLocation(),
                 Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                       "Unable to find metal compiler"));
}
template <typename... Args> int MetalCompilerRunner::Run(Args... A) const {
  return llvm::sys::ExecuteAndWait(*MetalPath, {*MetalPath, A...});
}

#else

bool MetalCompilerRunner::ReadDefaultRegKey(StringRef KeyPathIn,
                                            std::string &ValueOut) const {
  SmallString<128> OutputFile;
  llvm::sys::fs::createTemporaryFile("reg-metal-output", "", OutputFile);
  llvm::FileRemover OutputRemover(OutputFile.c_str());

  Optional<StringRef> Redirects[] = {StringRef(""), StringRef(OutputFile),
                                     StringRef("")};

  if (llvm::sys::ExecuteAndWait(
          *WinePath, {*WinePath, "reg", "query", KeyPathIn}, None, Redirects))
    return false;

  if (auto OutputBuf = llvm::MemoryBuffer::getFile(OutputFile.c_str())) {
    StringRef OutStr((*OutputBuf)->getBufferStart(),
                     (*OutputBuf)->getBufferSize());
#define DEFAULT_KEY_PATTERN "(Default)    REG_SZ    "
    size_t StartPos = OutStr.find(DEFAULT_KEY_PATTERN);
    if (StartPos != StringRef::npos) {
      StartPos += std::strlen(DEFAULT_KEY_PATTERN);
      size_t EndPos = OutStr.find('\r', StartPos);
      if (EndPos != StringRef::npos) {
        ValueOut.assign(OutStr.begin() + StartPos, OutStr.begin() + EndPos);
        return true;
      }
    }
  }

  return false;
}

bool MetalCompilerRunner::WineToHostPath(StringRef WinePathIn,
                                         std::string &HostPathOut) const {
  SmallString<128> OutputFile;
  llvm::sys::fs::createTemporaryFile("withpath-output", "", OutputFile);
  llvm::FileRemover OutputRemover(OutputFile.c_str());

  Optional<StringRef> Redirects[] = {StringRef(""), StringRef(OutputFile),
                                     StringRef("")};

  if (llvm::sys::ExecuteAndWait(*WinePathPath, {*WinePathPath, WinePathIn},
                                None, Redirects))
    return false;

  if (auto OutputBuf = llvm::MemoryBuffer::getFile(OutputFile.c_str())) {
    StringRef OutStr((*OutputBuf)->getBufferStart(),
                     (*OutputBuf)->getBufferSize());
    size_t EndPos = OutStr.find('\n');
    if (EndPos != StringRef::npos) {
      HostPathOut.assign(OutStr.begin(), OutStr.begin() + EndPos);
      return true;
    }
  }

  return false;
}

MetalCompilerRunner::MetalCompilerRunner(HshTarget Target,
                                         DiagnosticsEngine &Diags)
    : WinePath(llvm::sys::findProgramByName("wine")),
      WinePathPath(llvm::sys::findProgramByName("winepath")),
      MetalPath(llvm::errc::no_such_file_or_directory) {
  if (!WinePath) {
    Diags.Report(
        SourceLocation(),
        Diags.getCustomDiagID(DiagnosticsEngine::Error, "Unable to find wine"));
    return;
  }
  if (!WinePathPath) {
    Diags.Report(SourceLocation(),
                 Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                       "Unable to find winepath"));
    return;
  }

  std::string Path;
  if (ReadDefaultRegKey("HKLM\\SOFTWARE\\Apple\\Metal Developer Tools", Path)) {
    std::string HostPath;
    if (WineToHostPath(Path, HostPath)) {
      HostPath += GetTargetSDKPath(Target);
      MetalPath = llvm::sys::findProgramByName("metal.exe", {HostPath});
    }
  }

  if (!MetalPath)
    Diags.Report(SourceLocation(),
                 Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                       "Unable to find metal compiler"));
}

template <typename... Args> int MetalCompilerRunner::Run(Args... A) const {
  return llvm::sys::ExecuteAndWait(*WinePath, {*WinePath, *MetalPath, A...});
}

#endif

MetalStageCompiler::MetalStageCompiler(HshTarget Target, bool DebugInfo,
                                       CompilerInstance &CI,
                                       DiagnosticsEngine &Diags)
    : StageCompiler(Target), CI(CI), Diags(Diags), Runner(Target, Diags),
      DebugInfo(DebugInfo) {}

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

    if (Runner.Run("-Wno-unused-function", DebugInfo ? "-g" : "", "-o",
                   OutputFile.c_str(), InputFile.c_str())) {
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
