//===--- DxcStageCompiler.cpp - DXC hsh stage compiler --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "DxcStageCompiler.h"
#include "../Builtins/Builtins.h"

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/config.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Path.h"

#include "clang/Basic/Diagnostic.h"

#include "dxc/dxcapi.h"

#ifdef __EMULATE_UUID
#define HSH_IID_PPV_ARGS(ppType)                                               \
  DxcLibrary::SharedInstance->UUIDs.get<std::decay_t<decltype(**(ppType))>>(), \
      reinterpret_cast<void **>(ppType)
#else // __EMULATE_UUID
#define HSH_IID_PPV_ARGS(ppType)                                               \
  __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
#endif // __EMULATE_UUID

using namespace llvm;
using namespace clang;

namespace {

class DxcLibrary {
  sys::DynamicLibrary Library;
  DxcCreateInstanceProc DxcCreateInstance;

public:
  static Optional<DxcLibrary> SharedInstance;
  static void EnsureSharedInstance(StringRef ResourceDir,
                                   DiagnosticsEngine &Diags) {
    if (!SharedInstance)
      SharedInstance.emplace(ResourceDir, Diags);
  }

#ifdef __EMULATE_UUID
  struct ImportedUUIDs {
    void *_IUnknown = nullptr;
    void *_IDxcBlob = nullptr;
    void *_IDxcBlobUtf8 = nullptr;
    void *_IDxcResult = nullptr;
    void *_IDxcCompiler3 = nullptr;
    void import(sys::DynamicLibrary &Library) {
      _IUnknown = Library.getAddressOfSymbol("_ZN8IUnknown11IUnknown_IDE");
      _IDxcBlob = Library.getAddressOfSymbol("_ZN8IDxcBlob11IDxcBlob_IDE");
      _IDxcBlobUtf8 =
          Library.getAddressOfSymbol("_ZN12IDxcBlobUtf815IDxcBlobUtf8_IDE");
      _IDxcResult =
          Library.getAddressOfSymbol("_ZN10IDxcResult13IDxcResult_IDE");
      _IDxcCompiler3 =
          Library.getAddressOfSymbol("_ZN13IDxcCompiler316IDxcCompiler3_IDE");
    }
    template <typename T> REFIID get();
  } UUIDs;
#endif

  explicit DxcLibrary(StringRef ResourceDir, DiagnosticsEngine &Diags) {
    std::string Err;
#if LLVM_ON_UNIX
    SmallString<128> LibPath(ResourceDir);
    sys::path::append(LibPath, "libdxcompiler" LTDL_SHLIB_EXT);
    Library = sys::DynamicLibrary::getPermanentLibrary(LibPath.c_str(), &Err);
#else
    Library = sys::DynamicLibrary::getPermanentLibrary("dxcompiler.dll", &Err);
#endif
    if (!Library.isValid()) {
      Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                         "unable to load %0; %1"))
          << LibPath << Err;
      return;
    }
    DxcCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(
        Library.getAddressOfSymbol("DxcCreateInstance"));
    if (!DxcCreateInstance) {
      Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                         "unable to find DxcCreateInstance"));
      return;
    }
#ifdef __EMULATE_UUID
    UUIDs.import(Library);
#endif
  }

  CComPtr<IDxcCompiler3> MakeCompiler() const;
};
llvm::Optional<DxcLibrary> DxcLibrary::SharedInstance;

#ifdef __EMULATE_UUID
template <> REFIID DxcLibrary::ImportedUUIDs::get<IUnknown>() {
  return _IUnknown;
}
template <> REFIID DxcLibrary::ImportedUUIDs::get<IDxcBlob>() {
  return _IDxcBlob;
}
template <> REFIID DxcLibrary::ImportedUUIDs::get<IDxcBlobUtf8>() {
  return _IDxcBlobUtf8;
}
template <> REFIID DxcLibrary::ImportedUUIDs::get<IDxcResult>() {
  return _IDxcResult;
}
template <> REFIID DxcLibrary::ImportedUUIDs::get<IDxcCompiler3>() {
  return _IDxcCompiler3;
}
#endif

CComPtr<IDxcCompiler3> DxcLibrary::MakeCompiler() const {
  CComPtr<IDxcCompiler3> Ret;
  DxcCreateInstance(CLSID_DxcCompiler, HSH_IID_PPV_ARGS(&Ret));
  return Ret;
}

} // namespace

namespace clang::hshgen {

class DxcStageCompilerImpl : public DxcStageCompiler {
  DiagnosticsEngine &Diags;
  bool DebugInfo;
  WCHAR TShiftArg[4];
  WCHAR SShiftArg[4];
  CComPtr<IDxcCompiler3> Compiler;

  static constexpr std::array<LPCWSTR, 6> ShaderProfiles{
      L"vs_6_0", L"hs_6_0", L"ds_6_0", L"gs_6_0", L"ps_6_0"};

protected:
  StageBinaries doCompile(ArrayRef<std::string> Sources) const override {
    StageBinaries Binaries;
    auto *OutIt = Binaries.begin();
    const auto *ProfileIt = ShaderProfiles.begin();
    int StageIt = 0;
    for (const auto &Stage : Sources) {
      auto &Out = OutIt++->first;
      const LPCWSTR Profile = *ProfileIt++;
      const auto HStage = HshStage(StageIt++);
      if (Stage.empty())
        continue;
      DxcText SourceBuf{Stage.data(), Stage.size(), 0};
      LPCWSTR DxArgs[] = {L"-T", Profile, DebugInfo ? L"-Zi" : L""};
      LPCWSTR VkArgs[] = {L"-T",
                          Profile,
                          DebugInfo ? L"-Zi" : L"",
                          L"-spirv",
                          L"-fspv-target-env=vulkan1.1",
                          L"-fvk-use-dx-layout",
                          HStage == HshVertexStage ? L"-fvk-invert-y" : L"",
                          L"-fvk-t-shift",
                          TShiftArg,
                          L"0",
                          L"-fvk-s-shift",
                          SShiftArg,
                          L"0"};
      LPCWSTR *Args = Target == HT_VULKAN_SPIRV ? VkArgs : DxArgs;
      UINT32 ArgCount = Target == HT_VULKAN_SPIRV
                            ? std::extent_v<decltype(VkArgs)>
                            : std::extent_v<decltype(DxArgs)>;
      CComPtr<IDxcResult> Result;
      HRESULT HResult = Compiler->Compile(&SourceBuf, Args, ArgCount, nullptr,
                                          HSH_IID_PPV_ARGS(&Result));
      if (!Result) {
        Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                           "no result from dxcompiler"));
        continue;
      }
      bool HasObj = Result->HasOutput(DXC_OUT_OBJECT);
      if (HasObj) {
        CComPtr<IDxcBlob> ObjBlob;
        Result->GetOutput(DXC_OUT_OBJECT, HSH_IID_PPV_ARGS(&ObjBlob), nullptr);
        if (auto Size = ObjBlob->GetBufferSize()) {
          Out.resize(Size);
          std::memcpy(&Out[0], ObjBlob->GetBufferPointer(), Size);
        } else {
          HasObj = false;
        }
      }
      if (Result->HasOutput(DXC_OUT_ERRORS)) {
        CComPtr<IDxcBlobUtf8> ErrBlob;
        Result->GetOutput(DXC_OUT_ERRORS, HSH_IID_PPV_ARGS(&ErrBlob), nullptr);
        if (ErrBlob->GetBufferSize()) {
          if (!HasObj)
            llvm::errs() << Stage << '\n';
          StringRef ErrStr((char *)ErrBlob->GetBufferPointer());
          Diags.Report(Diags.getCustomDiagID(HasObj ? DiagnosticsEngine::Warning
                                                    : DiagnosticsEngine::Error,
                                             "%0 problem from dxcompiler: %1"))
              << HshStageToString(HStage) << ErrStr.rtrim();
        }
      }
      if (HResult != ERROR_SUCCESS) {
        Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                           "%0 problem from dxcompiler: %1"))
            << HshStageToString(HStage) << HResult;
      }
    }
    return Binaries;
  }

public:
  explicit DxcStageCompilerImpl(HshTarget Target, bool DebugInfo,
                                StringRef ResourceDir, DiagnosticsEngine &Diags,
                                HshBuiltins &Builtins)
      : DxcStageCompiler(Target), Diags(Diags), DebugInfo(DebugInfo) {
    DxcLibrary::EnsureSharedInstance(ResourceDir, Diags);
    Compiler = DxcLibrary::SharedInstance->MakeCompiler();

    int res = std::swprintf(TShiftArg, 4, L"%u", Builtins.getMaxUniforms());
    assert(res >= 0);
    res = std::swprintf(SShiftArg, 4, L"%u",
                        Builtins.getMaxUniforms() + Builtins.getMaxImages());
    assert(res >= 0);
  }
};

std::unique_ptr<DxcStageCompiler>
DxcStageCompiler::Create(HshTarget Target, bool DebugInfo,
                         StringRef ResourceDir, DiagnosticsEngine &Diags,
                         HshBuiltins &Builtins) {
  return std::make_unique<DxcStageCompilerImpl>(Target, DebugInfo, ResourceDir,
                                                Diags, Builtins);
}

} // namespace clang::hshgen
