//===--- HshGenerator.cpp - Lambda scanner and codegen for hsh tool -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Config/config.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/SaveAndRestore.h"
#include "llvm/Support/raw_carray_ostream.h"
#include "llvm/Support/raw_comment_ostream.h"
#include "llvm/Support/xxhash.h"

#include "clang/AST/ASTDumper.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/AST/QualTypeNames.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Analysis/AnalysisDeclContext.h"
#include "clang/Config/config.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Hsh/HshGenerator.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Lex/PreprocessorOptions.h"

#include "dxc/dxcapi.h"

#define XSTR(X) #X
#define STR(X) XSTR(X)

#define ENABLE_DUMP 0

namespace {

using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::hshgen;
using namespace std::literals;

constexpr StringRef operator""_ll(const char *__str, size_t __len) noexcept {
  return StringRef{__str, __len};
}

#ifdef __EMULATE_UUID
#define HSH_IID_PPV_ARGS(ppType)                                               \
  DxcLibrary::SharedInstance->UUIDs.get<std::decay_t<decltype(**(ppType))>>(), \
      reinterpret_cast<void **>(ppType)
#else // __EMULATE_UUID
#define HSH_IID_PPV_ARGS(ppType)                                               \
  __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
#endif // __EMULATE_UUID

class DxcLibrary {
  sys::DynamicLibrary Library;
  DxcCreateInstanceProc DxcCreateInstance;

public:
  static llvm::Optional<DxcLibrary> SharedInstance;
  static void EnsureSharedInstance(StringRef ProgramDir,
                                   DiagnosticsEngine &Diags) {
    if (!SharedInstance)
      SharedInstance.emplace(ProgramDir, Diags);
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

  explicit DxcLibrary(StringRef ProgramDir, DiagnosticsEngine &Diags) {
#ifdef LLVM_ON_UNIX
    SmallString<128> LibPath(sys::path::parent_path(ProgramDir));
    sys::path::append(LibPath, "lib" CLANG_LIBDIR_SUFFIX,
                      "libdxcompiler" LTDL_SHLIB_EXT);
#else
    SmallString<128> LibPath(ProgramDir);
    sys::path::append(LibPath, "libdxcompiler" LTDL_SHLIB_EXT);
#endif
    std::string Err;
    Library = sys::DynamicLibrary::getPermanentLibrary(LibPath.c_str(), &Err);
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

#define HSH_MAX_VERTEX_BUFFERS 32
#define HSH_MAX_TEXTURES 32
#define HSH_MAX_SAMPLERS 32
#define HSH_MAX_COLOR_TARGETS 8

enum HshStage : int {
  HshNoStage = -1,
  HshHostStage = 0,
  HshVertexStage,
  HshControlStage,
  HshEvaluationStage,
  HshGeometryStage,
  HshFragmentStage,
  HshMaxStage
};

constexpr StringRef HshStageToString(HshStage Stage) {
  switch (Stage) {
  case HshHostStage:
    return "host"_ll;
  case HshVertexStage:
    return "vertex"_ll;
  case HshControlStage:
    return "control"_ll;
  case HshEvaluationStage:
    return "evaluation"_ll;
  case HshGeometryStage:
    return "geometry"_ll;
  case HshFragmentStage:
    return "fragment"_ll;
  default:
    return "none"_ll;
  }
}

enum HshAttributeKind { PerVertex, PerInstance };

enum HshFormat : uint8_t {
  R8_UNORM,
  RG8_UNORM,
  RGB8_UNORM,
  RGBA8_UNORM,
  R16_UNORM,
  RG16_UNORM,
  RGB16_UNORM,
  RGBA16_UNORM,
  R32_UINT,
  RG32_UINT,
  RGB32_UINT,
  RGBA32_UINT,
  R8_SNORM,
  RG8_SNORM,
  RGB8_SNORM,
  RGBA8_SNORM,
  R16_SNORM,
  RG16_SNORM,
  RGB16_SNORM,
  RGBA16_SNORM,
  R32_SINT,
  RG32_SINT,
  RGB32_SINT,
  RGBA32_SINT,
  R32_SFLOAT,
  RG32_SFLOAT,
  RGB32_SFLOAT,
  RGBA32_SFLOAT,
};

struct StageBits {
  unsigned Bits = 0;
  operator unsigned() const { return Bits; }
  StageBits &operator=(unsigned NewBits) {
    Bits = NewBits;
    return *this;
  }
  StageBits &operator|=(unsigned NewBits) {
    Bits |= NewBits;
    return *this;
  }
};

class GeneratorDumper {
public:
#if ENABLE_DUMP
  PrintingPolicy Policy{LangOptions{}};
  static void PrintStageBits(raw_ostream &OS, StageBits Bits) {
    bool NeedsLeadingComma = false;
    for (int i = HshHostStage; i < HshMaxStage; ++i) {
      if ((1 << i) & Bits) {
        if (NeedsLeadingComma)
          OS << ", ";
        else
          NeedsLeadingComma = true;
        OS << HshStageToString(HshStage(i));
      }
    }
  }

  template <
      typename T,
      std::enable_if_t<!std::is_base_of_v<Stmt, std::remove_pointer_t<T>> &&
                           !std::is_base_of_v<Decl, std::remove_pointer_t<T>> &&
                           !std::is_same_v<QualType, std::decay_t<T>> &&
                           !std::is_same_v<StageBits, std::decay_t<T>> &&
                           !std::is_same_v<HshStage, std::decay_t<T>>,
                       int> = 0>
  GeneratorDumper &operator<<(const T &Obj) {
    llvm::errs() << Obj;
    return *this;
  }
  GeneratorDumper &operator<<(const Stmt *S) {
    S->printPretty(llvm::errs(), nullptr, Policy);
    return *this;
  }
  GeneratorDumper &operator<<(const Decl *D) {
    D->print(llvm::errs(), Policy);
    return *this;
  }
  GeneratorDumper &operator<<(const QualType T) {
    T.print(llvm::errs(), Policy);
    return *this;
  }
  GeneratorDumper &operator<<(const StageBits B) {
    PrintStageBits(llvm::errs(), B);
    return *this;
  }
  GeneratorDumper &operator<<(const HshStage S) {
    llvm::errs() << HshStageToString(S);
    return *this;
  }
  void setPrintingPolicy(const PrintingPolicy &PP) { Policy = PP; }
#else
  template <typename T> GeneratorDumper &operator<<(const T &Obj) {
    return *this;
  }
  void setPrintingPolicy(const PrintingPolicy &PP) {}
#endif
};

static GeneratorDumper &dumper() {
  static GeneratorDumper GD;
  return GD;
}

enum HshBuiltinType {
  HBT_None,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) HBT_##Name,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal) HBT_##Name,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  HBT_##Name##_float, HBT_##Name##_int, HBT_##Name##_uint,
#define BUILTIN_ENUM_TYPE(Name) HBT_##Name,
#include "BuiltinTypes.def"
  HBT_Max
};

enum HshBuiltinFunction {
  HBF_None,
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist) HBF_##Name,
#include "BuiltinFunctions.def"
  HBF_Max
};

enum HshBuiltinCXXMethod {
  HBM_None,
#define BUILTIN_CXX_METHOD(Name, IsSwizzle, Record, ...) HBM_##Name##_##Record,
#include "BuiltinCXXMethods.def"
  HBM_Max
};

class HshBuiltins {
public:
  struct Spellings {
    StringRef GLSL, HLSL, Metal;
  };

private:
  ClassTemplateDecl *BaseRecordType = nullptr;
  FunctionTemplateDecl *PushUniformMethod = nullptr;
  EnumDecl *EnumTarget = nullptr;
  EnumDecl *EnumStage = nullptr;
  EnumDecl *EnumInputRate = nullptr;
  EnumDecl *EnumFormat = nullptr;
  ClassTemplateDecl *ShaderConstDataTemplateType = nullptr;
  ClassTemplateDecl *ShaderDataTemplateType = nullptr;
  CXXRecordDecl *GlobalListNodeRecordType = nullptr;
  std::array<const TagDecl *, HBT_Max> Types{};
  std::array<const FunctionDecl *, HBF_Max> Functions{};
  std::array<const CXXMethodDecl *, HBM_Max> Methods{};

  static void printEnumeratorString(raw_ostream &Out,
                                    const PrintingPolicy &Policy, EnumDecl *ED,
                                    const APSInt &Val) {
    for (const EnumConstantDecl *ECD : ED->enumerators()) {
      if (llvm::APSInt::isSameValue(ECD->getInitVal(), Val)) {
        ECD->printQualifiedName(Out, Policy);
        return;
      }
    }
  }

  static constexpr Spellings BuiltinTypeSpellings[] = {
      {{}, {}, {}},
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal)                           \
  {#GLSL##_ll, #HLSL##_ll, #Metal##_ll},
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal)                           \
  {#GLSL##_ll, #HLSL##_ll, #Metal##_ll},
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  {#GLSLf##_ll, #HLSLf##_ll, #Metalf##_ll},                                    \
      {#GLSLi##_ll, #HLSLi##_ll, #Metali##_ll},                                \
      {#GLSLu##_ll, #HLSLu##_ll, #Metalu##_ll},
#define BUILTIN_ENUM_TYPE(Name) {{}, {}, {}},
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinTypeVector[] = {
      false,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) true,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal) false,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  false, false, false,
#define BUILTIN_ENUM_TYPE(Name) false,
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinTypeMatrix[] = {
      false,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) false,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal) true,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  false, false, false,
#define BUILTIN_ENUM_TYPE(Name) false,
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinTypeTexture[] = {
      false,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) false,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal) false,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  true, true, true,
#define BUILTIN_ENUM_TYPE(Name) false,
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinTypeEnum[] = {
      false,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) false,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal) false,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  false, false, false,
#define BUILTIN_ENUM_TYPE(Name) true,
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinMethodSwizzle[] = {
      false,
#define BUILTIN_CXX_METHOD(Name, IsSwizzle, Record, ...) IsSwizzle,
#include "BuiltinCXXMethods.def"
  };

  static constexpr Spellings BuiltinFunctionSpellings[] = {
      {{}, {}, {}},
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist)                  \
  {#GLSL##_ll, #HLSL##_ll, #Metal##_ll},
#include "BuiltinFunctions.def"
  };

  static constexpr bool BuiltinFunctionInterpDists[] = {
      false,
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist) InterpDist,
#include "BuiltinFunctions.def"
  };

  template <typename ImplClass>
  class DeclFinder : public DeclVisitor<ImplClass, bool> {
    using base = DeclVisitor<ImplClass, bool>;

  protected:
    StringRef Name;
    Decl *Found = nullptr;
    bool InHshNS = false;
    bool UseDetail = false;
    bool InDetailNS = false;

    bool inCorrectNS() const { return UseDetail ? InDetailNS : InHshNS; }

  public:
    explicit DeclFinder(bool UseDetail) : UseDetail(UseDetail) {}

    bool VisitDecl(Decl *D) {
      if (auto *DC = dyn_cast<DeclContext>(D))
        for (Decl *Child : DC->decls())
          if (!base::Visit(Child))
            return false;
      return true;
    }

    bool VisitNamespaceDecl(NamespaceDecl *Namespace) {
      if (InHshNS) {
        if (!UseDetail)
          return true;
        if (Namespace->getDeclName().isIdentifier() &&
            Namespace->getName() == "detail"_ll) {
          SaveAndRestore<bool> SavedInDetailNS(InDetailNS, true);
          return VisitDecl(Namespace);
        }
        return true;
      }
      if (Namespace->getDeclName().isIdentifier() &&
          Namespace->getName() == "hsh"_ll) {
        SaveAndRestore<bool> SavedInHshNS(InHshNS, true);
        return VisitDecl(Namespace);
      }
      return true;
    }

    Decl *Find(StringRef N, TranslationUnitDecl *TU) {
      Name = N;
      Found = nullptr;
      base::Visit(TU);
      return Found;
    }
  };

  class TypeFinder : public DeclFinder<TypeFinder> {
  public:
    bool VisitTagDecl(TagDecl *Type) {
      if (inCorrectNS() && Type->getDeclName().isIdentifier() &&
          Type->getName() == Name) {
        Found = Type;
        return false;
      }
      return true;
    }
    using DeclFinder<TypeFinder>::DeclFinder;
  };

  class FuncFinder : public DeclFinder<FuncFinder> {
  public:
    bool VisitFunctionDecl(FunctionDecl *Func) {
      if (inCorrectNS() && Func->getDeclName().isIdentifier() &&
          Func->getName() == Name) {
        Found = Func;
        return false;
      }
      return true;
    }
    using DeclFinder<FuncFinder>::DeclFinder;
  };

  class ClassTemplateFinder : public DeclFinder<ClassTemplateFinder> {
  public:
    bool VisitClassTemplateDecl(ClassTemplateDecl *Type) {
      if (inCorrectNS() && Type->getDeclName().isIdentifier() &&
          Type->getName() == Name) {
        Found = Type;
        return false;
      }
      return true;
    }
    using DeclFinder<ClassTemplateFinder>::DeclFinder;
  };

  class MethodFinder : public DeclFinder<MethodFinder> {
    StringRef Record;
    SmallVector<StringRef, 8> Params;

  public:
    bool VisitClassTemplateDecl(ClassTemplateDecl *ClassTemplate) {
      return VisitDecl(ClassTemplate->getTemplatedDecl());
    }

    bool VisitCXXMethodDecl(CXXMethodDecl *Method) {
      if (inCorrectNS() && Method->getDeclName().isIdentifier() &&
          Method->getName() == Name &&
          Method->getParent()->getName() == Record &&
          Method->getNumParams() == Params.size()) {
        auto It = Params.begin();
        for (ParmVarDecl *P : Method->parameters()) {
          if (P->getType().getAsString() != *It++)
            return true;
        }
        Found = Method;
        return false;
      }
      return true;
    }

    Decl *Find(StringRef N, StringRef R, StringRef P, TranslationUnitDecl *TU) {
      Name = N;
      Record = R;
      if (P != "void") {
        P.split(Params, ',');
        for (auto &ParamStr : Params)
          ParamStr = ParamStr.trim();
      }
      Found = nullptr;
      Visit(TU);
      return Found;
    }

    using DeclFinder<MethodFinder>::DeclFinder;
  };

  void addType(ASTContext &Context, HshBuiltinType TypeKind, StringRef Name,
               Decl *D) {
    if (auto *T = dyn_cast_or_null<TagDecl>(D)) {
      Types[TypeKind] = T->getFirstDecl();
    } else {
      DiagnosticsEngine &Diags = Context.getDiagnostics();
      Diags.Report(Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                    "type %0; is hsh.h included?"))
          << Name;
    }
  }

  void addTextureType(ASTContext &Context, HshBuiltinType FirstEnum,
                      StringRef Name, Decl *D) {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    if (auto *T = dyn_cast_or_null<ClassTemplateDecl>(D)) {
      for (const auto *Spec : T->specializations()) {
        QualType Tp = Spec->getTemplateArgs()[0].getAsType();
        if (Tp->isSpecificBuiltinType(BuiltinType::Float)) {
          Types[FirstEnum + 0] = Spec;
        } else if (Tp->isSpecificBuiltinType(BuiltinType::Int)) {
          Types[FirstEnum + 1] = Spec;
        } else if (Tp->isSpecificBuiltinType(BuiltinType::UInt)) {
          Types[FirstEnum + 2] = Spec;
        } else {
          Diags.Report(
              Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                    "unknown texture specialization type "
                                    "%0; must use float, int, unsigned int"))
              << Tp.getAsString();
        }
      }
    } else {
      Diags.Report(Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                    "class template %0; is hsh.h included?"))
          << Name;
    }
  }

  void addEnumType(ASTContext &Context, HshBuiltinType TypeKind, StringRef Name,
                   Decl *D) {
    if (auto *T = dyn_cast_or_null<EnumDecl>(D)) {
      Types[TypeKind] = T->getFirstDecl();
    } else {
      DiagnosticsEngine &Diags = Context.getDiagnostics();
      Diags.Report(Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                    "enum %0; is hsh.h included?"))
          << Name;
    }
  }

  void addFunction(ASTContext &Context, HshBuiltinFunction FuncKind,
                   StringRef Name, Decl *D) {
    if (auto *F = dyn_cast_or_null<FunctionDecl>(D)) {
      Functions[FuncKind] = F->getFirstDecl();
    } else {
      DiagnosticsEngine &Diags = Context.getDiagnostics();
      Diags.Report(Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                    "function %0; is hsh.h included?"))
          << Name;
    }
  }

  void addCXXMethod(ASTContext &Context, HshBuiltinCXXMethod MethodKind,
                    StringRef Name, Decl *D) {
    if (auto *M = dyn_cast_or_null<CXXMethodDecl>(D)) {
      Methods[MethodKind] = dyn_cast<CXXMethodDecl>(M->getFirstDecl());
    } else {
      DiagnosticsEngine &Diags = Context.getDiagnostics();
      Diags.Report(Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                    "method %0; is hsh.h included?"))
          << Name;
    }
  }

  EnumDecl *findEnum(StringRef Name, bool InDetail, ASTContext &Context) const {
    if (auto *Ret = dyn_cast_or_null<EnumDecl>(
            TypeFinder(InDetail).Find(Name, Context.getTranslationUnitDecl())))
      return Ret;
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of enum %0%1; "
                                  "is hsh.h included?"))
        << (InDetail ? "detail::" : "") << Name;
    return nullptr;
  }

  CXXRecordDecl *findCXXRecord(StringRef Name, bool InDetail,
                               ASTContext &Context) const {
    if (auto *Ret = dyn_cast_or_null<CXXRecordDecl>(
            TypeFinder(InDetail).Find(Name, Context.getTranslationUnitDecl())))
      return Ret;
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(
        Diags.getCustomDiagID(DiagnosticsEngine::Error,
                              "unable to locate declaration of record %0%1; "
                              "is hsh.h included?"))
        << (InDetail ? "detail::" : "") << Name;
    return nullptr;
  }

  ClassTemplateDecl *findClassTemplate(StringRef Name, bool InDetail,
                                       ASTContext &Context) const {
    if (auto *Ret = dyn_cast_or_null<ClassTemplateDecl>(
            ClassTemplateFinder(InDetail).Find(
                Name, Context.getTranslationUnitDecl())))
      return Ret;
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error,
        "unable to locate declaration of class template %0%1; "
        "is hsh.h included?"))
        << (InDetail ? "detail::" : "") << Name;
    return nullptr;
  }

  FunctionTemplateDecl *findMethodTemplate(ClassTemplateDecl *Class,
                                           StringRef Name,
                                           ASTContext &Context) const {
    auto *TemplDecl = Class->getTemplatedDecl();
    using FuncTemplIt =
        CXXRecordDecl::specific_decl_iterator<FunctionTemplateDecl>;
    FunctionTemplateDecl *Ret = nullptr;
    for (FuncTemplIt TI(TemplDecl->decls_begin()), TE(TemplDecl->decls_end());
         TI != TE; ++TI) {
      if (TI->getName() == Name)
        Ret = *TI;
    }
    if (Ret)
      return Ret;
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of "
                                  "method template %0::%1; is hsh.h included?"))
        << Class->getName() << Name;
    return nullptr;
  }

public:
  void findBuiltinDecls(ASTContext &Context) {
    if (auto *T = findClassTemplate("GenBase"_ll, true, Context)) {
      BaseRecordType = cast<ClassTemplateDecl>(T->getFirstDecl());
      PushUniformMethod =
          findMethodTemplate(BaseRecordType, "push_uniform"_ll, Context);
    }

    EnumTarget = findEnum("Target"_ll, false, Context);
    EnumStage = findEnum("Stage"_ll, false, Context);
    EnumInputRate = findEnum("InputRate"_ll, true, Context);
    EnumFormat = findEnum("Format"_ll, true, Context);
    ShaderConstDataTemplateType =
        findClassTemplate("ShaderConstData"_ll, true, Context);
    ShaderDataTemplateType = findClassTemplate("ShaderData"_ll, true, Context);
    GlobalListNodeRecordType =
        findCXXRecord("GlobalListNode"_ll, true, Context);

    TranslationUnitDecl *TU = Context.getTranslationUnitDecl();
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal)                           \
  addType(Context, HBT_##Name, #Name##_ll,                                     \
          TypeFinder(false).Find(#Name##_ll, TU));
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal)                           \
  addType(Context, HBT_##Name, #Name##_ll,                                     \
          TypeFinder(false).Find(#Name##_ll, TU));
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  addTextureType(Context, HBT_##Name##_float, #Name##_ll,                      \
                 ClassTemplateFinder(false).Find(#Name##_ll, TU));
#define BUILTIN_ENUM_TYPE(Name)                                                \
  addEnumType(Context, HBT_##Name, #Name##_ll,                                 \
              TypeFinder(false).Find(#Name##_ll, TU));
#include "BuiltinTypes.def"
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist)                  \
  addFunction(Context, HBF_##Name, #Name##_ll,                                 \
              FuncFinder(false).Find(#Name##_ll, TU));
#include "BuiltinFunctions.def"
#define BUILTIN_CXX_METHOD(Name, IsSwizzle, Record, ...)                       \
  addCXXMethod(Context, HBM_##Name##_##Record,                                 \
               #Record "::" #Name "(" #__VA_ARGS__ ")"_ll,                     \
               MethodFinder(false).Find(#Name##_ll, #Record##_ll,              \
                                        #__VA_ARGS__##_ll, TU));
#include "BuiltinCXXMethods.def"
  }

  HshBuiltinType identifyBuiltinType(QualType QT) const {
    return identifyBuiltinType(QT.getNonReferenceType().getTypePtrOrNull());
  }

  HshBuiltinType identifyBuiltinType(const clang::Type *UT) const {
    if (!UT)
      return HBT_None;
    TagDecl *T = UT->getAsTagDecl();
    if (!T)
      return HBT_None;
    T = T->getFirstDecl();
    if (!T)
      return HBT_None;
    HshBuiltinType Ret = HBT_None;
    for (const auto *Tp : Types) {
      if (T == Tp)
        return Ret;
      Ret = HshBuiltinType(int(Ret) + 1);
    }
    return HBT_None;
  }

  HshBuiltinFunction
  identifyBuiltinFunction(const clang::FunctionDecl *F) const {
    F = F->getFirstDecl();
    if (!F)
      return HBF_None;
    HshBuiltinFunction Ret = HBF_None;
    for (const auto *Func : Functions) {
      if (F == Func)
        return Ret;
      Ret = HshBuiltinFunction(int(Ret) + 1);
    }
    return HBF_None;
  }

  HshBuiltinCXXMethod
  identifyBuiltinMethod(const clang::CXXMethodDecl *M) const {
    M = dyn_cast_or_null<CXXMethodDecl>(M->getFirstDecl());
    if (!M)
      return HBM_None;
    if (FunctionDecl *FD = M->getInstantiatedFromMemberFunction())
      M = dyn_cast<CXXMethodDecl>(FD->getFirstDecl());
    HshBuiltinCXXMethod Ret = HBM_None;
    for (const auto *Method : Methods) {
      if (M == Method)
        return Ret;
      Ret = HshBuiltinCXXMethod(int(Ret) + 1);
    }
    return HBM_None;
  }

  static constexpr const Spellings &getSpellings(HshBuiltinType Tp) {
    return BuiltinTypeSpellings[Tp];
  }

  template <HshTarget T>
  static constexpr StringRef getSpelling(HshBuiltinType Tp);

  static constexpr const Spellings &getSpellings(HshBuiltinFunction Func) {
    return BuiltinFunctionSpellings[Func];
  }

  template <HshTarget T>
  static constexpr StringRef getSpelling(HshBuiltinFunction Func);

  static constexpr bool isVectorType(HshBuiltinType Tp) {
    return BuiltinTypeVector[Tp];
  }

  static constexpr bool isMatrixType(HshBuiltinType Tp) {
    return BuiltinTypeMatrix[Tp];
  }

  static constexpr unsigned getMatrixRowCount(HshBuiltinType Tp) {
    switch (Tp) {
    case HBT_float3x3:
      return 3;
    case HBT_float4x4:
      return 4;
    default:
      return 0;
    }
  }

  static constexpr bool isTextureType(HshBuiltinType Tp) {
    return BuiltinTypeTexture[Tp];
  }

  static constexpr bool isEnumType(HshBuiltinType Tp) {
    return BuiltinTypeEnum[Tp];
  }

  static constexpr bool isSwizzleMethod(HshBuiltinCXXMethod M) {
    return BuiltinMethodSwizzle[M];
  }

  static constexpr bool isInterpolationDistributed(HshBuiltinFunction Func) {
    return BuiltinFunctionInterpDists[Func];
  }

  const clang::TagDecl *getTypeDecl(HshBuiltinType Tp) const {
    return Types[Tp];
  }

  QualType getType(HshBuiltinType Tp) const {
    return getTypeDecl(Tp)->getTypeForDecl()->getCanonicalTypeUnqualified();
  }

  static TypeSourceInfo *getFullyQualifiedTemplateSpecializationTypeInfo(
      ASTContext &Context, TemplateDecl *TDecl,
      const TemplateArgumentListInfo &Args) {
    QualType Underlying =
        Context.getTemplateSpecializationType(TemplateName(TDecl), Args);
    Underlying = TypeName::getFullyQualifiedType(Underlying, Context);
    return Context.getTrivialTypeSourceInfo(Underlying);
  }

  CXXRecordDecl *getHshBaseSpecialization(ASTContext &Context,
                                          StringRef Name) const {
    CXXRecordDecl *Record = CXXRecordDecl::Create(
        Context, TTK_Class, Context.getTranslationUnitDecl(), {}, {},
        &Context.Idents.get(Name));
    Record->startDefinition();

    TemplateArgumentListInfo TemplateArgs;
    TemplateArgs.addArgument(TemplateArgumentLoc(
        QualType{Record->getTypeForDecl(), 0}, (TypeSourceInfo *)nullptr));
    TypeSourceInfo *TSI = getFullyQualifiedTemplateSpecializationTypeInfo(
        Context, BaseRecordType, TemplateArgs);
    CXXBaseSpecifier BaseSpec({}, false, true, AS_public, TSI, {});
    CXXBaseSpecifier *Bases = &BaseSpec;
    Record->setBases(&Bases, 1);

    return Record;
  }

  CXXMemberCallExpr *getPushUniformCall(ASTContext &Context, VarDecl *Decl,
                                        HshStage Stage) const {
    auto *NTTP = cast<NonTypeTemplateParmDecl>(
        PushUniformMethod->getTemplateParameters()->getParam(0));
    TemplateArgument TemplateArg{Context, APSInt::get(int(Stage) - 1),
                                 NTTP->getType()};
    auto *PushUniform =
        cast<CXXMethodDecl>(PushUniformMethod->getTemplatedDecl());
    TemplateArgumentListInfo CallTemplArgs(PushUniformMethod->getLocation(),
                                           {});
    CallTemplArgs.addArgument(
        TemplateArgumentLoc(TemplateArg, (Expr *)nullptr));
    auto *ThisExpr = new (Context) CXXThisExpr({}, Context.VoidTy, true);
    auto *ME = MemberExpr::Create(
        Context, ThisExpr, true, {}, {}, {}, PushUniform,
        DeclAccessPair::make(PushUniform, PushUniform->getAccess()), {},
        &CallTemplArgs, Context.VoidTy, VK_XValue, OK_Ordinary, NOUR_None);
    Expr *Arg =
        DeclRefExpr::Create(Context, {}, {}, Decl, false, SourceLocation{},
                            Decl->getType(), VK_XValue);
    return CXXMemberCallExpr::Create(Context, ME, Arg, Context.VoidTy,
                                     VK_XValue, {});
  }

  VarTemplateDecl *getConstDataVarTemplate(ASTContext &Context, DeclContext *DC,
                                           uint32_t NumStages,
                                           uint32_t NumBindings,
                                           uint32_t NumAttributes) const {
    NonTypeTemplateParmDecl *TargetParm = NonTypeTemplateParmDecl::Create(
        Context, DC, {}, {}, 0, 0, &Context.Idents.get("T"_ll),
        QualType{EnumTarget->getTypeForDecl(), 0}, false, nullptr);
    auto *TPL =
        TemplateParameterList::Create(Context, {}, {}, TargetParm, {}, nullptr);
    auto *PExpr =
        DeclRefExpr::Create(Context, {}, {}, TargetParm, false,
                            SourceLocation{}, TargetParm->getType(), VK_XValue);
    TemplateArgumentListInfo TemplateArgs;
    TemplateArgs.addArgument(
        TemplateArgumentLoc(TemplateArgument{PExpr}, PExpr));
    TemplateArgs.addArgument(
        TemplateArgumentLoc(TemplateArgument{Context, APSInt::get(NumStages),
                                             Context.UnsignedIntTy},
                            (Expr *)nullptr));
    TemplateArgs.addArgument(
        TemplateArgumentLoc(TemplateArgument{Context, APSInt::get(NumBindings),
                                             Context.UnsignedIntTy},
                            (Expr *)nullptr));
    TemplateArgs.addArgument(TemplateArgumentLoc(
        TemplateArgument{Context, APSInt::get(NumAttributes),
                         Context.UnsignedIntTy},
        (Expr *)nullptr));
    TypeSourceInfo *TSI = getFullyQualifiedTemplateSpecializationTypeInfo(
        Context, ShaderConstDataTemplateType, TemplateArgs);

    auto *VD =
        VarDecl::Create(Context, DC, {}, {}, &Context.Idents.get("cdata"_ll),
                        TSI->getType(), nullptr, SC_Static);
    VD->setConstexpr(true);
    VD->setInitStyle(VarDecl::ListInit);
    VD->setInit(new (Context) InitListExpr(Stmt::EmptyShell{}));
    return VarTemplateDecl::Create(Context, DC, {}, VD->getIdentifier(), TPL,
                                   VD);
  }

  VarTemplateDecl *getDataVarTemplate(ASTContext &Context, DeclContext *DC,
                                      uint32_t NumStages) const {
    NonTypeTemplateParmDecl *TargetParm = NonTypeTemplateParmDecl::Create(
        Context, DC, {}, {}, 0, 0, &Context.Idents.get("T"_ll),
        QualType{EnumTarget->getTypeForDecl(), 0}, false, nullptr);
    auto *TPL =
        TemplateParameterList::Create(Context, {}, {}, TargetParm, {}, nullptr);
    auto *PExpr =
        DeclRefExpr::Create(Context, {}, {}, TargetParm, false,
                            SourceLocation{}, TargetParm->getType(), VK_XValue);
    TemplateArgumentListInfo TemplateArgs;
    TemplateArgs.addArgument(
        TemplateArgumentLoc(TemplateArgument{PExpr}, PExpr));
    TemplateArgs.addArgument(
        TemplateArgumentLoc(TemplateArgument{Context, APSInt::get(NumStages),
                                             Context.UnsignedIntTy},
                            (Expr *)nullptr));
    TypeSourceInfo *TSI = getFullyQualifiedTemplateSpecializationTypeInfo(
        Context, ShaderDataTemplateType, TemplateArgs);

    auto *VD =
        VarDecl::Create(Context, DC, {}, {}, &Context.Idents.get("data"_ll),
                        TSI->getType(), nullptr, SC_Static);
    VD->setInitStyle(VarDecl::ListInit);
    VD->setInit(new (Context) InitListExpr(Stmt::EmptyShell{}));
    return VarTemplateDecl::Create(Context, DC, {}, VD->getIdentifier(), TPL,
                                   VD);
  }

  VarDecl *getGlobalListNode(ASTContext &Context, DeclContext *DC) const {
    return VarDecl::Create(
        Context, DC, {}, {}, &Context.Idents.get("global"_ll),
        QualType{GlobalListNodeRecordType->getTypeForDecl(), 0}, nullptr,
        SC_Static);
  }

  void printTargetEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                             HshTarget Target) const {
    printEnumeratorString(Out, Policy, EnumTarget, APSInt::get(Target));
  }

  void printStageEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                            HshStage Stage) const {
    printEnumeratorString(Out, Policy, EnumStage, APSInt::get(Stage - 1));
  }

  void printInputRateEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                                HshAttributeKind InputRate) const {
    printEnumeratorString(Out, Policy, EnumInputRate, APSInt::get(InputRate));
  }

  void printFormatEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                             HshFormat Format) const {
    printEnumeratorString(Out, Policy, EnumFormat, APSInt::get(Format));
  }

  HshFormat formatOfType(QualType Tp) const {
    if (Tp->isSpecificBuiltinType(BuiltinType::Float)) {
      return R32_SFLOAT;
    } else if (Tp->isSpecificBuiltinType(BuiltinType::Int)) {
      return R32_SINT;
    } else if (Tp->isSpecificBuiltinType(BuiltinType::UInt)) {
      return R32_UINT;
    }
    auto HBT = identifyBuiltinType(Tp);
    switch (HBT) {
    case HBT_float2:
      return RG32_SFLOAT;
    case HBT_float3:
    case HBT_float3x3:
      return RGB32_SFLOAT;
    case HBT_float4:
    case HBT_float4x4:
      return RGBA32_SFLOAT;
    case HBT_int2:
      return RG32_SINT;
    case HBT_int3:
      return RGB32_SINT;
    case HBT_int4:
      return RGBA32_SINT;
    case HBT_uint2:
      return RG32_UINT;
    case HBT_uint3:
      return RGB32_UINT;
    case HBT_uint4:
      return RGBA32_UINT;
    default:
      break;
    }
    llvm_unreachable("Invalid type passed to formatOfType");
  }
};

template <>
constexpr StringRef HshBuiltins::getSpelling<HT_GLSL>(HshBuiltinType Tp) {
  return getSpellings(Tp).GLSL;
}
template <>
constexpr StringRef HshBuiltins::getSpelling<HT_HLSL>(HshBuiltinType Tp) {
  return getSpellings(Tp).HLSL;
}
template <>
constexpr StringRef HshBuiltins::getSpelling<HT_METAL>(HshBuiltinType Tp) {
  return getSpellings(Tp).Metal;
}

template <>
constexpr StringRef HshBuiltins::getSpelling<HT_GLSL>(HshBuiltinFunction Func) {
  return getSpellings(Func).GLSL;
}
template <>
constexpr StringRef HshBuiltins::getSpelling<HT_HLSL>(HshBuiltinFunction Func) {
  return getSpellings(Func).HLSL;
}
template <>
constexpr StringRef
HshBuiltins::getSpelling<HT_METAL>(HshBuiltinFunction Func) {
  return getSpellings(Func).Metal;
}

enum HshInterfaceDirection { HshInput, HshOutput, HshInOut };

static HshStage DetermineParmVarStage(const ParmVarDecl *D) {
#define INTERFACE_VARIABLE(Attr, Stage, Direction, Array)                      \
  if (D->hasAttr<Attr>())                                                      \
    return Stage;
#include "ShaderInterface.def"
  return HshHostStage;
}

static HshInterfaceDirection DetermineParmVarDirection(const ParmVarDecl *D) {
#define INTERFACE_VARIABLE(Attr, Stage, Direction, Array)                      \
  if (D->hasAttr<Attr>())                                                      \
    return Direction;
#include "ShaderInterface.def"
  return HshInput;
}

template <typename T, unsigned N>
static DiagnosticBuilder
ReportCustom(const T *S, const ASTContext &Context,
             const char (&FormatString)[N],
             DiagnosticsEngine::Level level = DiagnosticsEngine::Error) {
  DiagnosticsEngine &Diags = Context.getDiagnostics();
  return Diags.Report(S->getBeginLoc(),
                      Diags.getCustomDiagID(level, FormatString))
         << CharSourceRange(S->getSourceRange(), false);
}

static void ReportUnsupportedStmt(const Stmt *S, const ASTContext &Context) {
  auto Diag = ReportCustom(
      S, Context,
      "statements of type %0 are not supported in hsh generator lambdas");
  Diag.AddString(S->getStmtClassName());
}

static void ReportUnsupportedFunctionCall(const Stmt *S,
                                          const ASTContext &Context) {
  ReportCustom(S, Context, "function calls are limited to hsh intrinsics");
}

static void ReportUnsupportedTypeReference(const Stmt *S,
                                           const ASTContext &Context) {
  ReportCustom(S, Context, "references to values are limited to hsh types");
}

static void ReportUnsupportedTypeConstruct(const Stmt *S,
                                           const ASTContext &Context) {
  ReportCustom(S, Context, "constructors are limited to hsh types");
}

static void ReportUnsupportedTypeCast(const Stmt *S,
                                      const ASTContext &Context) {
  ReportCustom(S, Context, "type casts are limited to hsh types");
}

static void ReportBadTextureReference(const Stmt *S,
                                      const ASTContext &Context) {
  ReportCustom(S, Context,
               "texture samples must be performed on lambda parameters");
}

static void ReportUnattributedTexture(const ParmVarDecl *PVD,
                                      const ASTContext &Context) {
  ReportCustom(
      PVD, Context,
      "sampled textures must be attributed with [[hsh::*_texture(n)]]");
}

static void ReportNonConstexprSampler(const Expr *E,
                                      const ASTContext &Context) {
  ReportCustom(E, Context, "sampler arguments must be constexpr");
}

static void ReportBadSamplerStructFormat(const Expr *E,
                                         const ASTContext &Context) {
  ReportCustom(E, Context, "sampler structure is not consistent");
}

static void ReportSamplerOverflow(const Expr *E, const ASTContext &Context) {
  ReportCustom(E, Context,
               "maximum sampler limit of " STR(HSH_MAX_SAMPLERS) " reached");
}

static void ReportBadVertexPositionType(const ParmVarDecl *PVD,
                                        const ASTContext &Context) {
  ReportCustom(PVD, Context, "vertex position must be a hsh::float4");
}

static void ReportBadColorTargetType(const ParmVarDecl *PVD,
                                     const ASTContext &Context) {
  ReportCustom(PVD, Context, "fragment color target must be a hsh::float4");
}

static void ReportBadVertexBufferType(const ParmVarDecl *PVD,
                                      const ASTContext &Context) {
  ReportCustom(PVD, Context, "vertex buffer must be a struct or class");
}

static void ReportVertexBufferOutOfRange(const ParmVarDecl *PVD,
                                         const ASTContext &Context) {
  ReportCustom(PVD, Context,
               "vertex buffer index must be in range [0," STR(
                   HSH_MAX_VERTEX_BUFFERS) ")");
}

static void ReportVertexBufferDuplicate(const ParmVarDecl *PVD,
                                        const ParmVarDecl *OtherPVD,
                                        const ASTContext &Context) {
  ReportCustom(PVD, Context, "vertex buffer index be unique");
  ReportCustom(OtherPVD, Context, "previous buffer index here",
               DiagnosticsEngine::Note);
}

static void ReportBadTextureType(const ParmVarDecl *PVD,
                                 const ASTContext &Context) {
  ReportCustom(PVD, Context, "texture must be a texture* type");
}

static void ReportTextureOutOfRange(const ParmVarDecl *PVD,
                                    const ASTContext &Context) {
  ReportCustom(PVD, Context,
               "texture index must be in range [0," STR(HSH_MAX_TEXTURES) ")");
}

static void ReportTextureDuplicate(const ParmVarDecl *PVD,
                                   const ParmVarDecl *OtherPVD,
                                   const ASTContext &Context) {
  ReportCustom(PVD, Context, "texture index be unique");
  ReportCustom(OtherPVD, Context, "previous texture index here",
               DiagnosticsEngine::Note);
}

static void ReportBadIntegerType(const Decl *D, const ASTContext &Context) {
  ReportCustom(D, Context, "integers must be 32-bits in length");
}

static void ReportBadRecordType(const Decl *D, const ASTContext &Context) {
  ReportCustom(D, Context,
               "hsh record fields must be a builtin hsh vector or matrix, "
               "float, double, or 32-bit integer");
}

static void ReportColorTargetOutOfRange(const ParmVarDecl *PVD,
                                        const ASTContext &Context) {
  ReportCustom(
      PVD, Context,
      "color target index must be in range [0," STR(HSH_MAX_COLOR_TARGETS) ")");
}

static void ReportConstAssignment(const Expr *AssignExpr,
                                  const ASTContext &Context) {
  ReportCustom(AssignExpr, Context, "cannot assign data to previous stages");
}

static bool CheckHshFieldTypeCompatibility(const HshBuiltins &Builtins,
                                           const ASTContext &Context,
                                           const ValueDecl *VD) {
  QualType Tp = VD->getType();
  HshBuiltinType HBT = Builtins.identifyBuiltinType(Tp);
  if (HBT != HBT_None && !HshBuiltins::isTextureType(HBT)) {
    return true;
  } else if (Tp->isIntegralOrEnumerationType()) {
    if (Context.getIntWidth(Tp) != 32) {
      ReportBadIntegerType(VD, Context);
      return false;
    }
    return true;
  } else if (Tp->isSpecificBuiltinType(BuiltinType::Float) ||
             Tp->isSpecificBuiltinType(BuiltinType::Double)) {
    return true;
  }
  ReportBadRecordType(VD, Context);
  return false;
}

static bool CheckHshRecordCompatibility(const HshBuiltins &Builtins,
                                        const ASTContext &Context,
                                        const CXXRecordDecl *Record) {
  bool Ret = true;
  for (const auto *FD : Record->fields())
    if (!CheckHshFieldTypeCompatibility(Builtins, Context, FD))
      Ret = false;
  return Ret;
}

enum HshSamplerFilterMode {
  HSF_Linear,
  HSF_Nearest,
};
enum HshSamplerWrapMode {
  HSW_Repeat,
  HSW_Clamp,
};
struct SamplerConfig {
  static constexpr unsigned NumFields = 2;
  static bool ValidateSamplerStruct(const APValue &Val) {
    if (!Val.isStruct() || Val.getStructNumFields() != NumFields)
      return false;
    for (unsigned i = 0; i < NumFields; ++i)
      if (!Val.getStructField(i).isInt())
        return false;
    return true;
  }
  HshSamplerFilterMode Filter = HSF_Linear;
  HshSamplerWrapMode Wrap = HSW_Repeat;
  SamplerConfig() = default;
  explicit SamplerConfig(const APValue &Val) {
    Filter =
        HshSamplerFilterMode(Val.getStructField(0).getInt().getSExtValue());
    Wrap = HshSamplerWrapMode(Val.getStructField(1).getInt().getSExtValue());
  }
  bool operator==(const SamplerConfig &Other) const {
    return Filter == Other.Filter && Wrap == Other.Wrap;
  }
};

struct SamplerRecord {
  SamplerConfig Config;
  unsigned UseStages;
};

struct ColorTargetRecord {
  StringRef Name;
  unsigned Index;
};

template <typename TexAttr> constexpr HshStage StageOfTextureAttr() {
  return HshNoStage;
}
template <> constexpr HshStage StageOfTextureAttr<HshVertexTextureAttr>() {
  return HshVertexStage;
}
template <> constexpr HshStage StageOfTextureAttr<HshFragmentTextureAttr>() {
  return HshFragmentStage;
}

struct AttributeRecord {
  StringRef Name;
  const CXXRecordDecl *Record;
  HshAttributeKind Kind;
  uint8_t Binding;
};

enum HshTextureKind {
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  HTK_##Name##_float, HTK_##Name##_int, HTK_##Name##_uint,
#include "BuiltinTypes.def"
};

constexpr HshTextureKind KindOfTextureType(HshBuiltinType Type) {
  switch (Type) {
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  case HBT_##Name##_float:                                                     \
    return HTK_##Name##_float;                                                 \
  case HBT_##Name##_int:                                                       \
    return HTK_##Name##_int;                                                   \
  case HBT_##Name##_uint:                                                      \
    return HTK_##Name##_uint;
#include "BuiltinTypes.def"
  default:
    llvm_unreachable("invalid texture kind");
  }
}

constexpr HshBuiltinType BuiltinTypeOfTexture(HshTextureKind Kind) {
  switch (Kind) {
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  case HTK_##Name##_float:                                                     \
    return HBT_##Name##_float;                                                 \
  case HTK_##Name##_int:                                                       \
    return HBT_##Name##_int;                                                   \
  case HTK_##Name##_uint:                                                      \
    return HBT_##Name##_uint;
#include "BuiltinTypes.def"
  }
}

struct TextureRecord {
  StringRef Name;
  HshTextureKind Kind;
  unsigned UseStages;
};

struct VertexBinding {
  uint32_t Binding;
  uint32_t Stride;
  HshAttributeKind InputRate;
};

struct VertexAttribute {
  uint32_t Offset;
  uint32_t Binding;
  HshFormat Format;
};

struct SampleCall {
  CXXMemberCallExpr *Expr;
  unsigned Index;
  unsigned SamplerIndex;
};

struct HostPrintingPolicy final : PrintingCallbacks, PrintingPolicy {
  static constexpr llvm::StringLiteral SignedInt32Spelling{"std::int32_t"};
  static constexpr llvm::StringLiteral UnsignedInt32Spelling{"std::uint32_t"};
  explicit HostPrintingPolicy(const PrintingPolicy &Policy)
      : PrintingPolicy(Policy) {
    Callbacks = this;
    Indentation = 1;
    SuppressImplicitBase = true;
    SilentNullStatement = true;
    NeverSuppressScope = true;
  }

  StringRef overrideBuiltinTypeName(const BuiltinType *T) const override {
    if (T->isEnumeralType())
      return {};
    if (T->isSignedIntegerType()) {
      return SignedInt32Spelling;
    } else if (T->isUnsignedIntegerType()) {
      return UnsignedInt32Spelling;
    }
    return {};
  }
};

struct ShaderPrintingPolicyBase : PrintingPolicy {
  HshTarget Target;
  virtual ~ShaderPrintingPolicyBase() = default;
  virtual void printStage(raw_ostream &OS, CXXRecordDecl *UniformRecord,
                          CXXRecordDecl *FromRecord, CXXRecordDecl *ToRecord,
                          ArrayRef<AttributeRecord> Attributes,
                          ArrayRef<TextureRecord> Textures,
                          ArrayRef<SamplerRecord> Samplers,
                          ArrayRef<ColorTargetRecord> ColorTargets,
                          CompoundStmt *Stmts, HshStage Stage, HshStage From,
                          HshStage To, uint32_t UniformBinding,
                          ArrayRef<SampleCall> SampleCalls) = 0;
  explicit ShaderPrintingPolicyBase(HshTarget Target)
      : PrintingPolicy(LangOptions()), Target(Target) {}
};

template <typename ImplClass>
struct ShaderPrintingPolicy : PrintingCallbacks, ShaderPrintingPolicyBase {
  HshBuiltins &Builtins;
  explicit ShaderPrintingPolicy(HshBuiltins &Builtins, HshTarget Target)
      : ShaderPrintingPolicyBase(Target), Builtins(Builtins) {
    Callbacks = this;
    Indentation = 1;
    IncludeTagDefinition = false;
    SuppressTagKeyword = true;
    SuppressScope = true;
    AnonymousTagLocations = false;
    SuppressImplicitBase = true;

    SuppressNestedQualifiers = true;
    SuppressListInitialization = true;
    SeparateConditionVarDecls = true;
    ConstantExprsAsInt = true;
    SilentNullStatement = true;
  }

  StringRef overrideBuiltinTypeName(const BuiltinType *T) const override {
    if (T->isSignedIntegerOrEnumerationType()) {
      return ImplClass::SignedInt32Spelling;
    } else if (T->isUnsignedIntegerOrEnumerationType()) {
      return ImplClass::UnsignedInt32Spelling;
    } else if (T->isSpecificBuiltinType(BuiltinType::Float)) {
      return ImplClass::Float32Spelling;
    } else if (T->isSpecificBuiltinType(BuiltinType::Double)) {
      return ImplClass::Float64Spelling;
    }
    return {};
  }

  StringRef overrideTagDeclIdentifier(TagDecl *D) const override {
    auto *Tp = D->getTypeForDecl();
    auto HBT = Builtins.identifyBuiltinType(Tp);
    if (HBT == HBT_None) {
      if (Tp->isSignedIntegerOrEnumerationType())
        return ImplClass::SignedInt32Spelling;
      if (Tp->isUnsignedIntegerOrEnumerationType())
        return ImplClass::UnsignedInt32Spelling;
      return {};
    }
    return HshBuiltins::getSpelling<ImplClass::SourceTarget>(HBT);
  }

  StringRef overrideBuiltinFunctionIdentifier(CallExpr *C) const override {
    if (auto *MemberCall = dyn_cast<CXXMemberCallExpr>(C)) {
      auto HBM = Builtins.identifyBuiltinMethod(MemberCall->getMethodDecl());
      if (HBM == HBM_None)
        return {};
      return static_cast<const ImplClass &>(*this).identifierOfCXXMethod(
          HBM, MemberCall);
    }
    if (auto *DeclRef =
            dyn_cast<DeclRefExpr>(C->getCallee()->IgnoreParenImpCasts())) {
      if (auto *FD = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
        auto HBF = Builtins.identifyBuiltinFunction(FD);
        if (HBF == HBF_None)
          return {};
        return HshBuiltins::getSpelling<ImplClass::SourceTarget>(HBF);
      }
    }
    return {};
  }

  bool overrideCallArguments(
      CallExpr *C, const std::function<void(StringRef)> &StringArg,
      const std::function<void(Expr *)> &ExprArg) const override {
    if (auto *MemberCall = dyn_cast<CXXMemberCallExpr>(C)) {
      auto HBM = Builtins.identifyBuiltinMethod(MemberCall->getMethodDecl());
      if (HBM == HBM_None)
        return {};
      return static_cast<const ImplClass &>(*this).overrideCXXMethodArguments(
          HBM, MemberCall, StringArg, ExprArg);
    }
    return false;
  }

  mutable std::string EnumValStr;
  StringRef overrideDeclRefIdentifier(DeclRefExpr *DR) const override {
    if (auto *PVD = dyn_cast<ParmVarDecl>(DR->getDecl())) {
      if (PVD->hasAttr<HshPositionAttr>())
        return static_cast<const ImplClass &>(*this).identifierOfVertexPosition(
            PVD);
      else if (PVD->hasAttr<HshColorTargetAttr>())
        return static_cast<const ImplClass &>(*this).identifierOfColorTarget(
            PVD);
    } else if (auto *ECD = dyn_cast<EnumConstantDecl>(DR->getDecl())) {
      EnumValStr.clear();
      raw_string_ostream OS(EnumValStr);
      OS << ECD->getInitVal();
      return OS.str();
    }
    return {};
  }

  StringRef prependMemberExprBase(MemberExpr *ME,
                                  bool &ReplaceBase) const override {
    if (auto *DRE = dyn_cast<DeclRefExpr>(ME->getBase())) {
      if (DRE->getDecl()->hasAttr<HshVertexBufferAttr>() ||
          DRE->getDecl()->hasAttr<HshInstanceBufferAttr>())
        return ImplClass::VertexBufferBase;
      if (ImplClass::NoUniformVarDecl &&
          DRE->getDecl()->getName() == "_from_host"_ll)
        ReplaceBase = true;
    }
    return {};
  }

  bool shouldPrintMemberExprUnderscore(MemberExpr *ME) const override {
    if (auto *DRE = dyn_cast<DeclRefExpr>(ME->getBase())) {
      return DRE->getDecl()->hasAttr<HshVertexBufferAttr>() ||
             DRE->getDecl()->hasAttr<HshInstanceBufferAttr>();
    }
    return false;
  }
};

struct GLSLPrintingPolicy : ShaderPrintingPolicy<GLSLPrintingPolicy> {
  static constexpr HshTarget SourceTarget = HT_GLSL;
  static constexpr bool NoUniformVarDecl = true;
  static constexpr llvm::StringLiteral SignedInt32Spelling{"int"};
  static constexpr llvm::StringLiteral UnsignedInt32Spelling{"uint"};
  static constexpr llvm::StringLiteral Float32Spelling{"float"};
  static constexpr llvm::StringLiteral Float64Spelling{"double"};
  static constexpr llvm::StringLiteral VertexBufferBase{""};

  static constexpr StringRef identifierOfVertexPosition(ParmVarDecl *PVD) {
    return "gl_Position"_ll;
  }

  static constexpr StringRef identifierOfColorTarget(ParmVarDecl *PVD) {
    return {};
  }

  static constexpr StringRef identifierOfCXXMethod(HshBuiltinCXXMethod HBM,
                                                   CXXMemberCallExpr *C) {
    switch (HBM) {
    case HBM_sample_texture2d:
      return "texture"_ll;
    default:
      return {};
    }
  }

  static constexpr bool
  overrideCXXMethodArguments(HshBuiltinCXXMethod HBM, CXXMemberCallExpr *C,
                             const std::function<void(StringRef)> &StringArg,
                             const std::function<void(Expr *)> &ExprArg) {
    switch (HBM) {
    case HBM_sample_texture2d: {
      ExprArg(C->getImplicitObjectArgument()->IgnoreParenImpCasts());
      ExprArg(C->getArg(0));
      return true;
    }
    default:
      return false;
    }
  }

  void printStage(raw_ostream &OS, CXXRecordDecl *UniformRecord,
                  CXXRecordDecl *FromRecord, CXXRecordDecl *ToRecord,
                  ArrayRef<AttributeRecord> Attributes,
                  ArrayRef<TextureRecord> Textures,
                  ArrayRef<SamplerRecord> Samplers,
                  ArrayRef<ColorTargetRecord> ColorTargets, CompoundStmt *Stmts,
                  HshStage Stage, HshStage From, HshStage To,
                  uint32_t UniformBinding,
                  ArrayRef<SampleCall> SampleCalls) override {
    OS << "#version 450 core\n";
    if (UniformRecord) {
      OS << "layout(binding = " << UniformBinding << ") uniform host_to_"
         << HshStageToString(Stage) << " {\n";
      for (auto *FD : UniformRecord->fields()) {
        OS << "  ";
        FD->print(OS, *this, 1);
        OS << ";\n";
      }
      OS << "};\n";
    }

    if (FromRecord) {
      OS << "in " << HshStageToString(From) << "_to_" << HshStageToString(Stage)
         << " {\n";
      for (auto *FD : FromRecord->fields()) {
        OS << "  ";
        FD->print(OS, *this, 1);
        OS << ";\n";
      }
      OS << "} _from_" << HshStageToString(From) << ";\n";
    }

    if (ToRecord) {
      OS << "out " << HshStageToString(Stage) << "_to_" << HshStageToString(To)
         << " {\n";
      for (auto *FD : ToRecord->fields()) {
        OS << "  ";
        FD->print(OS, *this, 1);
        OS << ";\n";
      }
      OS << "} _to_" << HshStageToString(To) << ";\n";
    }

    if (Stage == HshVertexStage) {
      uint32_t Location = 0;
      for (const auto &Attribute : Attributes) {
        for (const auto *FD : Attribute.Record->fields()) {
          QualType Tp = FD->getType().getUnqualifiedType();
          HshBuiltinType HBT = Builtins.identifyBuiltinType(Tp);
          if (HshBuiltins::isMatrixType(HBT)) {
            switch (HBT) {
            case HBT_float3x3:
              OS << "layout(location = " << Location << ") in ";
              Tp.print(OS, *this);
              OS << " " << Attribute.Name << "_" << FD->getName() << ";\n";
              Location += 3;
              break;
            case HBT_float4x4:
              OS << "layout(location = " << Location << ") in ";
              Tp.print(OS, *this);
              OS << " " << Attribute.Name << "_" << FD->getName() << ";\n";
              Location += 4;
              break;
            default:
              llvm_unreachable("Unhandled matrix type");
            }
          } else {
            OS << "layout(location = " << Location++ << ") in ";
            Tp.print(OS, *this);
            OS << " " << Attribute.Name << "_" << FD->getName() << ";\n";
          }
        }
      }
    }

    uint32_t TexBinding = 0;
    for (const auto &Tex : Textures) {
      if ((1u << Stage) & Tex.UseStages)
        OS << "layout(binding = " << TexBinding << ") uniform "
           << HshBuiltins::getSpelling<SourceTarget>(
                  BuiltinTypeOfTexture(Tex.Kind))
           << " " << Tex.Name << ";\n";
      ++TexBinding;
    }

    if (Stage == HshFragmentStage)
      for (const auto &CT : ColorTargets)
        OS << "layout(location = " << CT.Index << ") out vec4 " << CT.Name
           << ";\n";

    OS << "void main() ";
    Stmts->printPretty(OS, nullptr, *this);
  }

  using ShaderPrintingPolicy<GLSLPrintingPolicy>::ShaderPrintingPolicy;
};

struct HLSLPrintingPolicy : ShaderPrintingPolicy<HLSLPrintingPolicy> {
  static constexpr HshTarget SourceTarget = HT_HLSL;
  static constexpr bool NoUniformVarDecl = true;
  static constexpr llvm::StringLiteral SignedInt32Spelling{"int"};
  static constexpr llvm::StringLiteral UnsignedInt32Spelling{"uint"};
  static constexpr llvm::StringLiteral Float32Spelling{"float"};
  static constexpr llvm::StringLiteral Float64Spelling{"double"};
  static constexpr llvm::StringLiteral VertexBufferBase{"_vert_data."};

  std::string VertexPositionIdentifier;
  StringRef identifierOfVertexPosition(ParmVarDecl *PVD) const {
    return VertexPositionIdentifier;
  }

  mutable std::string ColorTargetIdentifier;
  StringRef identifierOfColorTarget(ParmVarDecl *PVD) const {
    ColorTargetIdentifier.clear();
    raw_string_ostream OS(ColorTargetIdentifier);
    OS << "_targets_out." << PVD->getName();
    return OS.str();
  }

  mutable std::string CXXMethodIdentifier;
  StringRef identifierOfCXXMethod(HshBuiltinCXXMethod HBM,
                                  CXXMemberCallExpr *C) const {
    switch (HBM) {
    case HBM_sample_texture2d: {
      CXXMethodIdentifier.clear();
      raw_string_ostream OS(CXXMethodIdentifier);
      C->getImplicitObjectArgument()->printPretty(OS, nullptr, *this);
      OS << ".Sample";
      return OS.str();
    }
    default:
      return {};
    }
  }

  ArrayRef<SampleCall> ThisSampleCalls;
  bool
  overrideCXXMethodArguments(HshBuiltinCXXMethod HBM, CXXMemberCallExpr *C,
                             const std::function<void(StringRef)> &StringArg,
                             const std::function<void(Expr *)> &ExprArg) const {
    switch (HBM) {
    case HBM_sample_texture2d: {
      auto Search =
          std::find_if(ThisSampleCalls.begin(), ThisSampleCalls.end(),
                       [&](const auto &Other) { return C == Other.Expr; });
      assert(Search != ThisSampleCalls.end() && "sample call must exist");
      std::string SamplerArg{"_sampler"};
      raw_string_ostream OS(SamplerArg);
      OS << Search->SamplerIndex;
      StringArg(OS.str());
      ExprArg(C->getArg(0));
      return true;
    }
    default:
      return false;
    }
  }

  bool overrideCXXOperatorCall(
      CXXOperatorCallExpr *C, raw_ostream &OS,
      const std::function<void(Expr *)> &ExprArg) const override {
    if (C->getNumArgs() == 2 && C->getOperator() == OO_Star) {
      if (HshBuiltins::isMatrixType(
              Builtins.identifyBuiltinType(C->getArg(0)->getType())) ||
          HshBuiltins::isMatrixType(
              Builtins.identifyBuiltinType(C->getArg(1)->getType()))) {
        OS << "mul(";
        ExprArg(C->getArg(0));
        OS << ", ";
        ExprArg(C->getArg(1));
        OS << ")";
        return true;
      }
    }
    return false;
  }

  bool overrideCXXTemporaryObjectExpr(
      CXXTemporaryObjectExpr *C, raw_ostream &OS,
      const std::function<void(Expr *)> &ExprArg) const override {
    if (C->getNumArgs() == 1) {
      auto DTp = Builtins.identifyBuiltinType(C->getType());
      auto STp = Builtins.identifyBuiltinType(C->getArg(0)->getType());
      switch (DTp) {
      case HBT_float3x3:
        switch (STp) {
        case HBT_float4x4:
          OS << "float4x4_to_float3x3(";
          ExprArg(C->getArg(0));
          OS << ")";
          return true;
        default:
          break;
        }
        break;
      default:
        break;
      }
    }
    return false;
  }

  CompoundStmt *ThisStmts = nullptr;
  std::string BeforeStatements;
  void
  printCompoundStatementBefore(const std::function<raw_ostream &()> &Indent,
                               CompoundStmt *CS) const override {
    if (CS == ThisStmts)
      Indent() << BeforeStatements;
  }

  std::string AfterStatements;
  void printCompoundStatementAfter(const std::function<raw_ostream &()> &Indent,
                                   CompoundStmt *CS) const override {
    if (CS == ThisStmts)
      Indent() << AfterStatements;
  }

  static constexpr llvm::StringLiteral HLSLRuntimeSupport{
      R"(static float3x3 float4x4_to_float3x3(float4x4 mtx) {
  return float3x3(mtx[0].xyz, mtx[1].xyz, mtx[2].xyz);
}
)"};

  void printStage(raw_ostream &OS, CXXRecordDecl *UniformRecord,
                  CXXRecordDecl *FromRecord, CXXRecordDecl *ToRecord,
                  ArrayRef<AttributeRecord> Attributes,
                  ArrayRef<TextureRecord> Textures,
                  ArrayRef<SamplerRecord> Samplers,
                  ArrayRef<ColorTargetRecord> ColorTargets, CompoundStmt *Stmts,
                  HshStage Stage, HshStage From, HshStage To,
                  uint32_t UniformBinding,
                  ArrayRef<SampleCall> SampleCalls) override {
    OS << HLSLRuntimeSupport;
    ThisStmts = Stmts;
    ThisSampleCalls = SampleCalls;

    if (UniformRecord) {
      OS << "cbuffer host_to_" << HshStageToString(Stage) << " : register(b"
         << UniformBinding << ") {\n";
      for (auto *FD : UniformRecord->fields()) {
        OS << "  ";
        FD->print(OS, *this, 1);
        OS << ";\n";
      }
      OS << "};\n";
    }

    if (FromRecord) {
      OS << "struct " << HshStageToString(From) << "_to_"
         << HshStageToString(Stage) << " {\n";
      uint32_t VarIdx = 0;
      for (auto *FD : FromRecord->fields()) {
        OS << "  ";
        FD->print(OS, *this, 1);
        OS << " : VAR" << VarIdx++ << ";\n";
      }
      OS << "};\n";
    }

    if (ToRecord) {
      OS << "struct " << HshStageToString(Stage) << "_to_"
         << HshStageToString(To) << " {\n"
         << "  float4 _position : SV_Position;\n";
      uint32_t VarIdx = 0;
      for (auto *FD : ToRecord->fields()) {
        OS << "  ";
        FD->print(OS, *this, 1);
        OS << " : VAR" << VarIdx++ << ";\n";
      }
      OS << "};\n";
    }

    if (Stage == HshVertexStage) {
      OS << "struct host_vert_data {\n";
      uint32_t Location = 0;
      for (const auto &Attribute : Attributes) {
        for (const auto *FD : Attribute.Record->fields()) {
          QualType Tp = FD->getType().getUnqualifiedType();
          HshBuiltinType HBT = Builtins.identifyBuiltinType(Tp);
          if (HshBuiltins::isMatrixType(HBT)) {
            switch (HBT) {
            case HBT_float3x3:
              if (Target == HT_VULKAN_SPIRV)
                OS << "  [[vk::location(" << Location << ")]] ";
              else
                OS << "  ";
              Tp.print(OS, *this);
              OS << " " << Attribute.Name << "_" << FD->getName() << " : ATTR"
                 << Location << ";\n";
              Location += 3;
              break;
            case HBT_float4x4:
              if (Target == HT_VULKAN_SPIRV)
                OS << "  [[vk::location(" << Location << ")]] ";
              else
                OS << "  ";
              Tp.print(OS, *this);
              OS << " " << Attribute.Name << "_" << FD->getName() << " : ATTR"
                 << Location << ";\n";
              Location += 4;
              break;
            default:
              llvm_unreachable("Unhandled matrix type");
            }
          } else {
            if (Target == HT_VULKAN_SPIRV)
              OS << "  [[vk::location(" << Location << ")]] ";
            else
              OS << "  ";
            Tp.print(OS, *this);
            OS << " " << Attribute.Name << "_" << FD->getName() << " : ATTR"
               << Location << ";\n";
            Location += 1;
          }
        }
      }
      OS << "};\n";
    }

    uint32_t TexBinding = 0;
    for (const auto &Tex : Textures) {
      if ((1u << Stage) & Tex.UseStages)
        OS << HshBuiltins::getSpelling<SourceTarget>(
                  BuiltinTypeOfTexture(Tex.Kind))
           << " " << Tex.Name << " : register(t" << TexBinding << ");\n";
      ++TexBinding;
    }

    uint32_t SamplerBinding = 0;
    for (const auto &Samp : Samplers) {
      if ((1u << Stage) & Samp.UseStages)
        OS << "SamplerState _sampler" << SamplerBinding << " : register(s"
           << SamplerBinding << ");\n";
      ++SamplerBinding;
    }

    if (Stage == HshFragmentStage) {
      OS << "struct color_targets_out {\n";
      for (const auto &CT : ColorTargets)
        OS << "  float4 " << CT.Name << " : SV_Target" << CT.Index << ";\n";
      OS << "};\n";
    }

    if (Stage == HshFragmentStage) {
      OS << "color_targets_out main(";
      BeforeStatements = "color_targets_out _targets_out;\n";
      AfterStatements = "return _targets_out;\n";
    } else if (ToRecord) {
      VertexPositionIdentifier.clear();
      raw_string_ostream PIO(VertexPositionIdentifier);
      PIO << "_to_" << HshStageToString(To) << "._position";
      OS << HshStageToString(Stage) << "_to_" << HshStageToString(To)
         << " main(";
      BeforeStatements.clear();
      raw_string_ostream BO(BeforeStatements);
      BO << HshStageToString(Stage) << "_to_" << HshStageToString(To) << " _to_"
         << HshStageToString(To) << ";\n";
      AfterStatements.clear();
      raw_string_ostream AO(AfterStatements);
      AO << "return _to_" << HshStageToString(To) << ";\n";
    }
    if (Stage == HshVertexStage)
      OS << "in host_vert_data _vert_data";
    else if (FromRecord)
      OS << "in " << HshStageToString(From) << "_to_" << HshStageToString(Stage)
         << " _from_" << HshStageToString(From);
    OS << ") ";
    Stmts->printPretty(OS, nullptr, *this);
  }

  using ShaderPrintingPolicy<HLSLPrintingPolicy>::ShaderPrintingPolicy;
};

static std::unique_ptr<ShaderPrintingPolicyBase>
MakePrintingPolicy(HshBuiltins &Builtins, HshTarget Target) {
  switch (Target) {
  case HT_GLSL:
    return std::make_unique<GLSLPrintingPolicy>(Builtins, Target);
  case HT_HLSL:
  case HT_DXBC:
  case HT_DXIL:
  case HT_VULKAN_SPIRV:
  case HT_METAL:
  case HT_METAL_BIN_MAC:
  case HT_METAL_BIN_IOS:
  case HT_METAL_BIN_TVOS:
    return std::make_unique<HLSLPrintingPolicy>(Builtins, Target);
  }
}

struct StageSources {
  HshTarget Target;
  std::array<std::string, HshMaxStage> Sources;
  explicit StageSources(HshTarget Target) : Target(Target) {}
};

class StagesBuilder {
  ASTContext &Context;
  HshBuiltins &Builtins;
  unsigned UseStages;

  static IdentifierInfo &getToIdent(ASTContext &Context, HshStage Stage) {
    std::string VarName;
    raw_string_ostream VNS(VarName);
    VNS << "_to_" << HshStageToString(Stage);
    return Context.Idents.get(VNS.str());
  }

  static IdentifierInfo &getFromIdent(ASTContext &Context, HshStage Stage) {
    std::string VarName;
    raw_string_ostream VNS(VarName);
    VNS << "_from_" << HshStageToString(Stage);
    return Context.Idents.get(VNS.str());
  }

  static IdentifierInfo &getFromToIdent(ASTContext &Context, HshStage From,
                                        HshStage To) {
    std::string RecordName;
    raw_string_ostream RNS(RecordName);
    RNS << HshStageToString(From) << "_to_" << HshStageToString(To);
    return Context.Idents.get(RNS.str());
  }

  class InterfaceRecord {
    CXXRecordDecl *Record = nullptr;
    SmallVector<std::pair<Expr *, FieldDecl *>, 8> Fields;
    VarDecl *Producer = nullptr;
    VarDecl *Consumer = nullptr;
    HshStage SStage = HshNoStage, DStage = HshNoStage;

    MemberExpr *createFieldReference(ASTContext &Context, Expr *E, VarDecl *VD,
                                     bool Producer) {
      FieldDecl *Field = getFieldForExpr(Context, E, Producer);
      if (!Field)
        return nullptr;
      QualType Tp = Field->getType().getLocalUnqualifiedType();
      if (!Producer)
        Tp = Tp.withConst();
      return MemberExpr::CreateImplicit(
          Context,
          DeclRefExpr::Create(Context, {}, {}, VD, false, SourceLocation{},
                              VD->getType(), VK_XValue),
          false, Field, Tp, VK_XValue, OK_Ordinary);
    }

  public:
    void initializeRecord(ASTContext &Context, DeclContext *SpecDeclContext,
                          HshStage S, HshStage D) {
      Record = CXXRecordDecl::Create(Context, TTK_Struct, SpecDeclContext, {},
                                     {}, &getFromToIdent(Context, S, D));
      Record->startDefinition();

      CanQualType CDType =
          Record->getTypeForDecl()->getCanonicalTypeUnqualified();

      VarDecl *PVD =
          VarDecl::Create(Context, SpecDeclContext, {}, {},
                          &getToIdent(Context, D), CDType, nullptr, SC_None);
      Producer = PVD;

      VarDecl *CVD =
          VarDecl::Create(Context, SpecDeclContext, {}, {},
                          &getFromIdent(Context, S), CDType, nullptr, SC_None);
      Consumer = CVD;

      SStage = S;
      DStage = D;
    }

    static bool isSameComparisonOperand(Expr *E1, Expr *E2) {
      if (E1 == E2)
        return true;
      E1->setValueKind(VK_RValue);
      E2->setValueKind(VK_RValue);
      return Expr::isSameComparisonOperand(E1, E2);
    }

    FieldDecl *getFieldForExpr(ASTContext &Context, Expr *E,
                               bool IgnoreExisting) {
      assert(Record && "Invalid InterfaceRecord requested from");
      for (auto &P : Fields) {
        if (isSameComparisonOperand(P.first, E))
          return IgnoreExisting ? nullptr : P.second;
      }
      std::string FieldName;
      raw_string_ostream FNS(FieldName);
      FNS << '_' << HshStageToString(SStage)[0] << HshStageToString(DStage)[0]
          << Fields.size();
      FieldDecl *FD = FieldDecl::Create(
          Context, Record, {}, {}, &Context.Idents.get(FNS.str()),
          E->getType().getUnqualifiedType(), {}, {}, false, ICIS_NoInit);
      FD->setAccess(AS_public);
      Record->addDecl(FD);
      Fields.push_back(std::make_pair(E, FD));
      return FD;
    }

    MemberExpr *createProducerFieldReference(ASTContext &Context, Expr *E) {
      return createFieldReference(Context, E, Producer, true);
    }

    MemberExpr *createConsumerFieldReference(ASTContext &Context, Expr *E) {
      return createFieldReference(Context, E, Consumer, false);
    }

    void finalizeRecord() { Record->completeDefinition(); }

    CXXRecordDecl *getRecord() const { return Record; }
  };

  struct StageStmtList {
    SmallVector<Stmt *, 16> Stmts;
    CompoundStmt *CStmts = nullptr;
  };

  std::array<InterfaceRecord, HshMaxStage>
      HostToStageRecords; /* Indexed by consumer stage */
  std::array<InterfaceRecord, HshMaxStage>
      InterStageRecords; /* Indexed by consumer stage */
  std::array<StageStmtList, HshMaxStage> StageStmts;
  std::array<SmallVector<SampleCall, 4>, HshMaxStage> SampleCalls;
  SmallVector<VarDecl *, 4> UsedCaptures;
  SmallVector<AttributeRecord, 4> AttributeRecords;
  SmallVector<TextureRecord, 8> Textures;
  SmallVector<SamplerRecord, 8> Samplers;
  SmallVector<ColorTargetRecord, 2> ColorTargets;
  unsigned FinalStageCount = 0;
  SmallVector<VertexBinding, 4> VertexBindings;
  SmallVector<VertexAttribute, 4> VertexAttributes;

public:
  StagesBuilder(ASTContext &Context, HshBuiltins &Builtins,
                DeclContext *SpecDeclContext, unsigned UseStages)
      : Context(Context), Builtins(Builtins), UseStages(UseStages) {
    for (int D = HshVertexStage; D < HshMaxStage; ++D) {
      if (UseStages & (1u << unsigned(D))) {
        HostToStageRecords[D].initializeRecord(Context, SpecDeclContext,
                                               HshHostStage, HshStage(D));
      }
    }
    for (int D = HshControlStage, S = HshVertexStage; D < HshMaxStage; ++D) {
      if (UseStages & (1u << unsigned(D))) {
        InterStageRecords[D].initializeRecord(Context, SpecDeclContext,
                                              HshStage(S), HshStage(D));
        S = D;
      }
    }
  }

  /*
   * Base case for createInterStageReferenceExpr.
   * Stage division will be established on this expression.
   */
  Expr *VisitExpr(Expr *E, HshStage From, HshStage To) {
    if (From == To || From == HshNoStage || To == HshNoStage)
      return E;
    assert(To > From && "cannot create backwards stage references");
    if (From != HshHostStage) {
      /* Create intermediate inter-stage assignments */
      for (int D = From + 1, S = From; D <= To; ++D) {
        if (UseStages & (1u << unsigned(D))) {
          InterfaceRecord &SRecord = InterStageRecords[S];
          InterfaceRecord &DRecord = InterStageRecords[D];
          if (MemberExpr *Producer =
                  DRecord.createProducerFieldReference(Context, E)) {
            auto *AssignOp = new (Context) BinaryOperator(
                Producer,
                S == From ? E
                          : SRecord.createConsumerFieldReference(Context, E),
                BO_Assign, E->getType(), VK_XValue, OK_Ordinary, {}, {});
            addStageStmt(AssignOp, HshStage(S));
          }
          S = D;
        }
      }
    } else {
      if (MemberExpr *Producer =
              HostToStageRecords[To].createProducerFieldReference(Context, E)) {
        auto *AssignOp =
            new (Context) BinaryOperator(Producer, E, BO_Assign, E->getType(),
                                         VK_XValue, OK_Ordinary, {}, {});
        addStageStmt(AssignOp, From);
      }
    }
    InterfaceRecord &Record =
        From == HshHostStage ? HostToStageRecords[To] : InterStageRecords[To];
    return Record.createConsumerFieldReference(Context, E);
  }

  Expr *createInterStageReferenceExpr(Expr *E, HshStage From, HshStage To) {
    return VisitExpr(E, From, To);
  }

  void addStageStmt(Stmt *S, HshStage Stage) {
    StageStmts[Stage].Stmts.push_back(S);
  }

  template <typename TexAttr>
  std::pair<HshStage, APSInt> getTextureIndex(TexAttr *A) const {
    Expr::EvalResult Res;
    A->getIndex()->EvaluateAsInt(Res, Context);
    return {StageOfTextureAttr<TexAttr>(), Res.Val.getInt()};
  }

  template <typename TexAttr>
  std::pair<HshStage, APSInt> getTextureIndex(ParmVarDecl *PVD) const {
    if (auto *A = PVD->getAttr<TexAttr>())
      return getTextureIndex(A);
    return {HshNoStage, APSInt{}};
  }

  template <typename TexAttrA, typename TexAttrB, typename... Rest>
  std::pair<HshStage, APSInt> getTextureIndex(ParmVarDecl *PVD) const {
    if (auto *A = PVD->getAttr<TexAttrA>())
      return getTextureIndex(A);
    return getTextureIndex<TexAttrB, Rest...>(PVD);
  }

  std::pair<HshStage, APSInt> getTextureIndex(ParmVarDecl *PVD) const {
    return getTextureIndex<HshVertexTextureAttr, HshFragmentTextureAttr>(PVD);
  }

  void registerSampleCall(HshBuiltinCXXMethod HBM, CXXMemberCallExpr *C) {
    if (auto *DR = dyn_cast<DeclRefExpr>(
            C->getImplicitObjectArgument()->IgnoreParenImpCasts())) {
      if (auto *PVD = dyn_cast<ParmVarDecl>(DR->getDecl())) {
        auto [TexStage, TexIdx] = getTextureIndex(PVD);
        auto &StageCalls = SampleCalls[TexStage];
        for (const auto &Call : StageCalls)
          if (Call.Expr == C)
            return;
        APValue Res;
        Expr *SamplerArg = C->getArg(1);
        if (!SamplerArg->isCXX11ConstantExpr(Context, &Res)) {
          ReportNonConstexprSampler(SamplerArg, Context);
          return;
        }
        if (!SamplerConfig::ValidateSamplerStruct(Res)) {
          ReportBadSamplerStructFormat(SamplerArg, Context);
          return;
        }
        SamplerConfig Sampler(Res);
        auto Search =
            std::find_if(Samplers.begin(), Samplers.end(),
                         [&](const auto &S) { return S.Config == Sampler; });
        if (Search == Samplers.end()) {
          if (Samplers.size() == HSH_MAX_SAMPLERS) {
            ReportSamplerOverflow(SamplerArg, Context);
            return;
          }
          Search = Samplers.insert(Samplers.end(),
                                   SamplerRecord{Sampler, 1u << TexStage});
        } else {
          Search->UseStages |= 1u << TexStage;
        }
        StageCalls.push_back({C, unsigned(TexIdx.getZExtValue()),
                              unsigned(Search - Samplers.begin())});
      }
    }
  }

  void registerUsedCapture(VarDecl *PVD) {
    for (auto *EC : UsedCaptures)
      if (EC == PVD)
        return;
    UsedCaptures.push_back(PVD);
  }

  auto captures() const {
    return llvm::iterator_range(UsedCaptures.begin(), UsedCaptures.end());
  }

  void registerAttributeRecord(AttributeRecord Attribute) {
    auto Search =
        std::find_if(AttributeRecords.begin(), AttributeRecords.end(),
                     [&](const auto &A) { return A.Name == Attribute.Name; });
    if (Search != AttributeRecords.end())
      return;
    AttributeRecords.push_back(Attribute);

    auto *Type = Attribute.Record->getTypeForDecl();
    auto Size = Context.getTypeSizeInChars(Type);
    auto Align = Context.getTypeAlignInChars(Type);
    auto SizeOf = Size.alignTo(Align).getQuantity();
    VertexBindings.push_back(
        VertexBinding{Attribute.Binding, SizeOf, Attribute.Kind});
    CharUnits Offset;
    for (const auto *Field : Attribute.Record->fields()) {
      /*
       * Shader packing rules do not apply for attributes.
       * Just generate metadata according to C++ alignment.
       */
      auto FieldSize = Context.getTypeSizeInChars(Field->getType());
      auto FieldAlign = Context.getTypeAlignInChars(Field->getType());
      auto HBT = Builtins.identifyBuiltinType(Field->getType());
      auto Format = Builtins.formatOfType(Field->getType());
      auto ProcessField = [&]() {
        Offset = Offset.alignTo(FieldAlign);
        VertexAttributes.push_back(
            VertexAttribute{Offset.getQuantity(), Attribute.Binding, Format});
        Offset += FieldSize;
      };
      if (HshBuiltins::isMatrixType(HBT)) {
        auto RowCount = HshBuiltins::getMatrixRowCount(HBT);
        for (unsigned i = 0; i < RowCount; ++i)
          ProcessField();
      } else {
        ProcessField();
      }
    }
  }

  void registerTexture(StringRef Name, HshTextureKind Kind, HshStage Stage) {
    auto Search = std::find_if(Textures.begin(), Textures.end(),
                               [&](const auto &T) { return T.Name == Name; });
    if (Search != Textures.end()) {
      Search->UseStages |= 1u << Stage;
      return;
    }
    Textures.push_back(TextureRecord{Name, Kind, 1u << Stage});
  }

  void registerColorTarget(ColorTargetRecord Record) {
    auto Search =
        std::find_if(ColorTargets.begin(), ColorTargets.end(),
                     [&](const auto &T) { return T.Name == Record.Name; });
    if (Search != ColorTargets.end())
      return;
    ColorTargets.push_back(Record);
  }

  ArrayRef<Stmt *> getHostStmts() const {
    return StageStmts[HshHostStage].Stmts;
  }

  void finalizeResults(CXXRecordDecl *SpecRecord) {
    FinalStageCount = 0;
    for (int D = HshVertexStage; D < HshMaxStage; ++D) {
      if (UseStages & (1u << unsigned(D))) {
        auto &RecDecl = HostToStageRecords[D];
        RecDecl.finalizeRecord();
        SpecRecord->addDecl(RecDecl.getRecord());
        ++FinalStageCount;
      }
    }

    for (int D = HshControlStage; D < HshMaxStage; ++D) {
      if (UseStages & (1u << unsigned(D)))
        InterStageRecords[D].finalizeRecord();
    }

    auto &HostStmts = StageStmts[HshHostStage];
    std::array<VarDecl *, HshMaxStage> HostToStageVars{};
    SmallVector<Stmt *, 16> NewHostStmts;
    NewHostStmts.reserve(HostStmts.Stmts.size() + HshMaxStage * 2);
    for (int S = HshVertexStage; S < HshMaxStage; ++S) {
      if (UseStages & (1u << unsigned(S))) {
        auto *RecordDecl = HostToStageRecords[S].getRecord();
        CanQualType CDType =
            RecordDecl->getTypeForDecl()->getCanonicalTypeUnqualified();
        VarDecl *BindingVar = VarDecl::Create(Context, SpecRecord, {}, {},
                                              &getToIdent(Context, HshStage(S)),
                                              CDType, {}, SC_None);
        HostToStageVars[S] = BindingVar;
        NewHostStmts.push_back(new (Context)
                                   DeclStmt(DeclGroupRef(BindingVar), {}, {}));
      }
    }
    NewHostStmts.insert(NewHostStmts.end(), HostStmts.Stmts.begin(),
                        HostStmts.Stmts.end());
    for (int S = HshVertexStage; S < HshMaxStage; ++S) {
      if (auto *VD = HostToStageVars[S])
        NewHostStmts.push_back(
            Builtins.getPushUniformCall(Context, VD, HshStage(S)));
    }
    HostStmts.Stmts = std::move(NewHostStmts);

    for (int S = HshHostStage; S < HshMaxStage; ++S) {
      if (UseStages & (1u << unsigned(S))) {
        auto &Stmts = StageStmts[S];
        Stmts.CStmts = CompoundStmt::Create(Context, Stmts.Stmts, {}, {});
      }
    }
  }

  HshStage previousUsedStage(HshStage S) const {
    for (int D = S - 1; D >= HshVertexStage; --D) {
      if (UseStages & (1u << unsigned(D)))
        return HshStage(D);
    }
    return HshNoStage;
  }

  HshStage nextUsedStage(HshStage S) const {
    for (int D = S + 1; D < HshMaxStage; ++D) {
      if (UseStages & (1u << unsigned(D)))
        return HshStage(D);
    }
    return HshNoStage;
  }

  bool isStageUsed(HshStage S) const { return UseStages & (1u << S); }

  StageSources printResults(ShaderPrintingPolicyBase &Policy) {
    StageSources Sources(Policy.Target);

    uint32_t UniformBinding = 0;
    for (int S = HshVertexStage; S < HshMaxStage; ++S) {
      if (UseStages & (1u << unsigned(S))) {
        raw_string_ostream OS(Sources.Sources[S]);
        HshStage NextStage = nextUsedStage(HshStage(S));
        Policy.printStage(
            OS, HostToStageRecords[S].getRecord(),
            InterStageRecords[S].getRecord(),
            NextStage != HshNoStage ? InterStageRecords[NextStage].getRecord()
                                    : nullptr,
            AttributeRecords, Textures, Samplers, ColorTargets,
            StageStmts[S].CStmts, HshStage(S), previousUsedStage(HshStage(S)),
            NextStage, UniformBinding++, SampleCalls[S]);
      }
    }

    return Sources;
  }

  unsigned getNumStages() const { return FinalStageCount; }
  unsigned getNumBindings() const { return VertexBindings.size(); }
  unsigned getNumAttributes() const { return VertexAttributes.size(); }
  ArrayRef<VertexBinding> getBindings() const { return VertexBindings; }
  ArrayRef<VertexAttribute> getAttributes() const { return VertexAttributes; }
};

struct StageBinaries {
  HshTarget Target;
  std::array<std::pair<std::vector<uint8_t>, uint64_t>, HshMaxStage> Binaries;
  void updateHashes() {
    for (auto &Binary : Binaries)
      if (!Binary.first.empty())
        Binary.second = xxHash64(Binary.first);
  }
  explicit StageBinaries(HshTarget Target) : Target(Target) {}
};

class StagesCompilerBase {
protected:
  virtual StageBinaries doCompile() const = 0;

public:
  virtual ~StagesCompilerBase() = default;
  StageBinaries compile() const {
    auto Binaries = doCompile();
    Binaries.updateHashes();
    return Binaries;
  }
};

class StagesCompilerText : public StagesCompilerBase {
  const StageSources &Sources;

protected:
  StageBinaries doCompile() const override {
    StageBinaries Binaries(Sources.Target);
    auto OutIt = Binaries.Binaries.begin();
    for (const auto &Stage : Sources.Sources) {
      auto &Out = OutIt++->first;
      if (Stage.empty())
        continue;
      Out.resize(Stage.size() + 1);
      std::memcpy(&Out[0], Stage.data(), Stage.size());
    }
    return Binaries;
  }

public:
  explicit StagesCompilerText(const StageSources &Sources) : Sources(Sources) {}
};

class StagesCompilerDxc : public StagesCompilerBase {
  const StageSources &Sources;
  DiagnosticsEngine &Diags;

  static constexpr std::array<LPCWSTR, 6> ShaderProfiles{
      nullptr, L"vs_6_0", L"hs_6_0", L"ds_6_0", L"gs_6_0", L"ps_6_0"};

protected:
  StageBinaries doCompile() const override {
    CComPtr<IDxcCompiler3> Compiler =
        DxcLibrary::SharedInstance->MakeCompiler();
    StageBinaries Binaries(Sources.Target);
    auto OutIt = Binaries.Binaries.begin();
    auto ProfileIt = ShaderProfiles.begin();
    int StageIt = 0;
    for (const auto &Stage : Sources.Sources) {
      auto &Out = OutIt++->first;
      const LPCWSTR Profile = *ProfileIt++;
      const auto HStage = HshStage(StageIt++);
      if (Stage.empty())
        continue;
      DxcText SourceBuf{Stage.data(), Stage.size(), 0};
      LPCWSTR DxArgs[] = {L"-T", Profile};
      LPCWSTR VkArgs[] = {L"-T", Profile, L"-spirv"};
      LPCWSTR *Args = Sources.Target == HT_VULKAN_SPIRV ? VkArgs : DxArgs;
      UINT32 ArgCount = Sources.Target == HT_VULKAN_SPIRV
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
          Diags.Report(Diags.getCustomDiagID(HasObj ? DiagnosticsEngine::Warning
                                                    : DiagnosticsEngine::Error,
                                             "%0 problem from dxcompiler: %1"))
              << HshStageToString(HStage)
              << (char *)ErrBlob->GetBufferPointer();
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
  explicit StagesCompilerDxc(const StageSources &Sources, StringRef ProgramDir,
                             DiagnosticsEngine &Diags)
      : Sources(Sources), Diags(Diags) {
    DxcLibrary::EnsureSharedInstance(ProgramDir, Diags);
  }
};

static std::unique_ptr<StagesCompilerBase>
MakeCompiler(const StageSources &Sources, StringRef ProgramDir,
             DiagnosticsEngine &Diags) {
  switch (Sources.Target) {
  case HT_GLSL:
  case HT_HLSL:
    return std::make_unique<StagesCompilerText>(Sources);
  case HT_DXBC:
  case HT_DXIL:
  case HT_VULKAN_SPIRV:
    return std::make_unique<StagesCompilerDxc>(Sources, ProgramDir, Diags);
  case HT_METAL:
  case HT_METAL_BIN_MAC:
  case HT_METAL_BIN_IOS:
  case HT_METAL_BIN_TVOS:
    return std::make_unique<StagesCompilerText>(Sources);
  }
}

class LocationNamespaceSearch
    : public RecursiveASTVisitor<LocationNamespaceSearch> {
  ASTContext &Context;
  SourceLocation L;
  NamespaceDecl *InNS = nullptr;

public:
  explicit LocationNamespaceSearch(ASTContext &Context) : Context(Context) {}

  bool VisitNamespaceDecl(NamespaceDecl *NS) {
    auto Range = NS->getSourceRange();
    if (Range.getBegin() < L && L < Range.getEnd()) {
      InNS = NS;
      return false;
    }
    return true;
  }

  NamespaceDecl *findNamespace(SourceLocation Location) {
    L = Location;
    InNS = nullptr;
    TraverseAST(Context);
    return InNS;
  }
};

class StageStmtLifter {
  /*
   * Per-statement stage dependencies are centrally tracked here.
   * The first DependencyPass populates the stage bits of each statement in
   * post-order graph traversal. Statements that depend on the output of each
   * statement are also collected here for later lifting and pruning.
   */
  struct StmtDepInfo {
    struct StageBits StageBits;
    DenseSet<const Stmt *> Dependents;
    void setPrimaryStage(HshStage Stage) {
      if (Stage == HshNoStage)
        StageBits = 0;
      else
        StageBits = 1 << Stage;
    }
    HshStage getMaxStage() const {
      for (int i = HshMaxStage - 1; i >= HshHostStage; --i) {
        if ((1 << i) & StageBits)
          return HshStage(i);
      }
      return HshHostStage;
    }
    HshStage getMinStage() const {
      for (int i = HshHostStage; i < HshMaxStage; ++i) {
        if ((1 << i) & StageBits)
          return HshStage(i);
      }
      return HshHostStage;
    }
    bool hasStage(HshStage Stage) const { return (1 << Stage) & StageBits; }
  };
  using StmtMapType = DenseMap<const Stmt *, StmtDepInfo>;
  StmtMapType StmtMap;
  std::vector<const Stmt *> OrderedStmts;
  std::array<DenseSet<const VarDecl *>, HshMaxStage> UsedDecls;

  void UpdateDeclRefExprStages(const DeclStmt *DS, unsigned StageBits) {
    for (auto *Decl : DS->decls()) {
      if (auto *VD = dyn_cast<VarDecl>(Decl)) {
        for (auto &P : StmtMap) {
          if (auto *DRE = dyn_cast<DeclRefExpr>(P.first)) {
            if (DRE->getDecl() == VD)
              P.second.StageBits = StageBits;
          }
        }
      }
    }
  }

  /*
   * Assignments into declarations will mutate their stage dependency.
   * This is information is stored according to the parallel block scope
   * of nested control flow statements.
   */
  struct DeclDepInfo {
    HshStage Stage = HshNoStage;
    /* Mutator statements of decl so far, starting with original DeclStmt */
    DenseSet<const Stmt *> MutatorStmts;
  };
  using DeclMapType = DenseMap<const Decl *, DeclDepInfo>;

  struct DependencyPass : ConstStmtVisitor<DependencyPass, HshStage> {
    StageStmtLifter &Lifter;
    StagesBuilder &Builder;
    explicit DependencyPass(StageStmtLifter &Lifter, StagesBuilder &Builder)
        : Lifter(Lifter), Builder(Builder) {}

    /*
     * Statement stage dependencies include condition variables that decide
     * if their containing block is reached. The BlockScopeStack follows the
     * block scope of supported control flow statements. Parallel branches
     * (i.e. if-else, switch) are kept separate so they have fresh declaration
     * context from the point of control flow entry.
     */
    struct BlockScopeStack {
      struct StackEntry {
        HshStage Stage;
        SmallVector<DeclMapType, 8> ParallelDeclMaps{DeclMapType{}};
        explicit StackEntry(HshStage Stage) : Stage(Stage) {}
        void popMerge(const StackEntry &Other) {
          /*
           * Only declaration dependencies are merged since they represent
           * forward mutations. The stack entry stage follows the context of
           * condition expressions, therefore not merged.
           */
          auto &NewDeclMap = ParallelDeclMaps.back();
          for (const auto &OldDeclMap : Other.ParallelDeclMaps) {
            for (const auto &[OldDecl, OldMapEntry] : OldDeclMap) {
              auto &NewMapEntry = NewDeclMap[OldDecl];
              NewMapEntry.Stage =
                  std::max(NewMapEntry.Stage, OldMapEntry.Stage);
            }
          }
        }
      };
      SmallVector<StackEntry, 8> Stack;

      void push(HshStage Stage) { Stack.emplace_back(Stage); }

      void pop() {
        assert(Stack.size() >= 2 && "stack underflow");
        auto It = Stack.rbegin();
        auto &OldTop = *It++;
        auto &NewTop = *It;
        NewTop.popMerge(OldTop);
        Stack.pop_back();
      }

      size_t size() const { return Stack.size(); }

      void newParallelBranch() { Stack.back().ParallelDeclMaps.emplace_back(); }

      DeclDepInfo &getDeclDepInfo(const Decl *D) {
        for (auto I = Stack.rbegin(), E = Stack.rend(); I != E; ++I) {
          auto &DeclMap = I->ParallelDeclMaps.back();
          auto Search = DeclMap.find(D);
          if (Search != DeclMap.end())
            return Search->second;
        }
        return Stack.back().ParallelDeclMaps.back()[D];
      }

      HshStage getContextStage() const { return Stack.back().Stage; }
    };
    BlockScopeStack ScopeStack;

    HshStage VisitDeclStmt(const DeclStmt *DS) {
      HshStage MaxStage = ScopeStack.getContextStage();
      for (const auto *D : DS->decls()) {
        if (auto *VD = dyn_cast<VarDecl>(D)) {
          if (const Expr *Init = VD->getInit()) {
            Lifter.StmtMap[Init].Dependents.insert(DS);
            auto &DepInfo = ScopeStack.getDeclDepInfo(D);
            DepInfo.Stage = Visit(Init);
            DepInfo.MutatorStmts.insert(DS);
            MaxStage = std::max(MaxStage, DepInfo.Stage);
          }
        }
      }
      Lifter.StmtMap[DS].setPrimaryStage(MaxStage);
      return MaxStage;
    }

    HshStage VisitDeclRefExpr(const DeclRefExpr *DRE) {
      auto &DepInfo = ScopeStack.getDeclDepInfo(DRE->getDecl());
      for (auto *MS : DepInfo.MutatorStmts)
        Lifter.StmtMap[MS].Dependents.insert(DRE);
      if (auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl()))
        DepInfo.Stage = std::max(DepInfo.Stage, DetermineParmVarStage(PVD));
      else if (auto *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
        if (!VD->hasLocalStorage())
          DepInfo.Stage = std::max(DepInfo.Stage, HshHostStage);
      }
      Lifter.StmtMap[DRE].setPrimaryStage(DepInfo.Stage);
      return DepInfo.Stage;
    }

    HshStage VisitBinaryOperator(const BinaryOperator *BO) {
      if (BO->isAssignmentOp()) {
        if (auto *DRE =
                dyn_cast<DeclRefExpr>(BO->getLHS()->IgnoreParenImpCasts())) {
          auto &DepInfo = ScopeStack.getDeclDepInfo(DRE->getDecl());
          DepInfo.MutatorStmts.insert(BO);
        }
      }
      return VisitStmt(BO);
    }

    HshStage VisitCXXOperatorCallExpr(const CXXOperatorCallExpr *OC) {
      if (OC->isAssignmentOp()) {
        if (auto *DRE =
                dyn_cast<DeclRefExpr>(OC->getArg(0)->IgnoreParenImpCasts())) {
          auto &DepInfo = ScopeStack.getDeclDepInfo(DRE->getDecl());
          DepInfo.MutatorStmts.insert(OC);
        }
      }
      return VisitStmt(OC);
    }

    HshStage VisitStmt(const Stmt *S) {
      HshStage MaxStage = ScopeStack.getContextStage();
      for (auto *Child : S->children()) {
        auto &ChEnt = Lifter.StmtMap[Child];
        ChEnt.Dependents.insert(S);
        MaxStage = std::max(MaxStage, Visit(Child));
      }
      Lifter.StmtMap[S].setPrimaryStage(MaxStage);
      return MaxStage;
    }

    void run(AnalysisDeclContext &AD) {
      auto *CFG = AD.getCFG();
      size_t EstStmtCount = 0;
      for (auto *B : *CFG)
        EstStmtCount += B->size();
      Lifter.StmtMap.reserve(EstStmtCount);
      Lifter.OrderedStmts.reserve(EstStmtCount);
      ScopeStack.push(HshNoStage);

      struct SuccStack {
        using PopReturn = PointerIntPair<const CFGBlock *, 2>;
        enum : unsigned { ePoppedExit = 1, ePoppedDo = 2 };
        struct Entry {
          CFGBlock::succ_const_range Succs;
          PopReturn Exit;
          Entry(CFGBlock::succ_const_range Succs, const CFGBlock *Exit,
                bool ExitRoot)
              : Succs(Succs), Exit(Exit, ExitRoot ? ePoppedExit : 0) {}
        };
        SmallVector<Entry, 8> Stack;
        SmallVector<const CFGBlock *, 8> PreStack;
        BitVector ClosedSet;
        explicit SuccStack(class CFG *CFG) : ClosedSet(CFG->getNumBlockIDs()) {
          push(CFG->getEntry().succs(), &CFG->getExit(), true);
        }
        void push(CFGBlock::succ_const_range Succs, const CFGBlock *Exit,
                  bool ExitRoot) {
          Stack.emplace_back(Succs, Exit, ExitRoot);
        }
        PopReturn _pop() {
          assert(!Stack.empty() && "popping empty stack");
          auto &StackTop = Stack.back();
          while (!StackTop.Succs.empty()) {
            const auto *Ret = StackTop.Succs.begin()->getReachableBlock();
            StackTop.Succs =
                make_range(StackTop.Succs.begin() + 1, StackTop.Succs.end());
            if (Ret)
              return PopReturn{Ret, 0};
          }
          auto Ret = StackTop.Exit;
          Stack.pop_back();
          return Ret;
        }
        PopReturn pop() {
          while (true) {
            auto Ret = _pop();
            if (Stack.empty())
              return Ret;
            const auto *Block = Ret.getPointer();
            bool PoppedExit = Ret.getInt() & ePoppedExit;
            const auto *Exit = Stack.back().Exit.getPointer();
            if (!PoppedExit && Block->getBlockID() <= Exit->getBlockID())
              continue;
            if (ClosedSet.test(Block->getBlockID()))
              continue;
            ClosedSet.set(Block->getBlockID());
            if (!PreStack.empty() &&
                PreStack.back()->getBlockID() == Block->getBlockID()) {
              PreStack.pop_back();
              Ret.setInt(Ret.getInt() | ePoppedDo);
            }
            if (const auto *DoLoopTarget = Block->getDoLoopTarget()) {
              PreStack.emplace_back(DoLoopTarget);
            }
            const auto *OrigSucc = Block->getTerminator().getOrigSucc();
            push(Block->succs(), OrigSucc ? OrigSucc : Exit, OrigSucc);
            return Ret;
          }
        }
      } SuccStack{CFG};

      bool NeedsParallelBranch = false;
      while (true) {
        auto Ret = SuccStack.pop();
        const auto *Block = Ret.getPointer();
        bool PoppedExit = Ret.getInt() & SuccStack::ePoppedExit;
        bool PoppedDo = Ret.getInt() & SuccStack::ePoppedDo;
        bool AtExit = Block->getBlockID() == CFG->getExit().getBlockID();

        /*
         * The AtExit check is required for the case where a function ends
         * with a DoStmt. The CFG inserts an empty loopback statement that
         * would trigger a second pop and underflow the scope stack.
         */
        if (PoppedExit && !(AtExit && ScopeStack.size() == 1)) {
          ScopeStack.pop();
          NeedsParallelBranch = false;
          dumper() << "Popped Succ\n";
        }

        /*
         * Exit occurs here to ensure the scope stack is in the correct
         * exit state.
         */
        if (AtExit) {
          assert(ScopeStack.size() == 1 && "unbalanced scope stack");
          break;
        }

        /*
         * DoStmt body scopes are handled in a somewhat reversed fashion.
         * They are not opened by a condition terminator, but hsh still
         * wants to treat their contents as dependents of the while condition.
         */
        if (const auto *DoLoopTarget = Block->getDoLoopTarget()) {
          ScopeStack.push(Visit(DoLoopTarget->getTerminatorCondition()));
          NeedsParallelBranch = false;
          dumper() << "Pushed Do Succ\n";
        }

        /*
         * If no push-pop operation occurs between blocks, this is a parallel
         * block (i.e. else parallel with if)
         */
        if (NeedsParallelBranch) {
          ScopeStack.newParallelBranch();
          dumper() << "New parallel branch\n";
        }
        NeedsParallelBranch = true;

        /*
         * Scope stack is in the correct state for processing the block at
         * this point.
         */
        dumper() << "visit B" << Block->getBlockID() << '\n';
        for (const auto &Elem : *Block) {
          if (auto Stmt = Elem.getAs<CFGStmt>()) {
            Visit(Stmt->getStmt());
            Lifter.OrderedStmts.push_back(Stmt->getStmt());
          }
        }

        if (PoppedDo) {
          ScopeStack.pop();
          NeedsParallelBranch = false;
          dumper() << "Popped Do Succ\n";
        }
        if (const auto *OrigSucc = Block->getTerminator().getOrigSucc()) {
          ScopeStack.push(Visit(Block->getTerminatorCondition()));
          NeedsParallelBranch = false;
          dumper() << "Pushed Succ\n";
        }
      }
    }
  };

  struct LiftPass {
    StageStmtLifter &Lifter;
    explicit LiftPass(StageStmtLifter &Lifter) : Lifter(Lifter) {}

    bool CanLift(const Stmt *S) const {
      if (auto *E = dyn_cast<Expr>(S))
        S = E->IgnoreParenImpCasts();
      if (isa<DeclRefExpr>(S) || isa<IntegerLiteral>(S) ||
          isa<FloatingLiteral>(S))
        return true;
      if (auto *CE = dyn_cast<CXXConstructExpr>(S)) {
        for (auto *Arg : CE->arguments())
          if (!CanLift(Arg))
            return false;
        return true;
      } else if (auto *DS = dyn_cast<DeclStmt>(S)) {
        for (auto *Decl : DS->decls()) {
          if (auto *VD = dyn_cast<VarDecl>(Decl)) {
            if (auto *Init = VD->getInit()) {
              if (!CanLift(Init))
                return false;
            }
          }
        }
        return true;
      } else if (auto *C = dyn_cast<CallExpr>(S)) {
        if (auto *DeclRef =
                dyn_cast<DeclRefExpr>(C->getCallee()->IgnoreParenImpCasts())) {
          if (auto *FD = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
            auto HBF = Lifter.Builtins.identifyBuiltinFunction(FD);
            if (HBF != HBF_None &&
                !HshBuiltins::isInterpolationDistributed(HBF))
              return true;
          }
        }
        return false;
      }
      return false;
    }

    void LiftToDependents(const Stmt *S) {
      dumper() << "Can lift " << S;
      if (!CanLift(S)) {
        dumper() << " No\n";
        return;
      }
      dumper() << " Yes\n";
      auto Search = Lifter.StmtMap.find(S);
      if (Search != Lifter.StmtMap.end()) {
        StageBits DepStageBits;
        for (const auto *Dep : Search->second.Dependents) {
          dumper() << "  Checking dep " << Dep;
          auto DepSearch = Lifter.StmtMap.find(Dep);
          if (DepSearch != Lifter.StmtMap.end()) {
            DepStageBits |= DepSearch->second.StageBits;
            dumper() << " " << DepSearch->second.StageBits;
          }
          dumper() << "\n";
        }
        if (Search->second.StageBits != DepStageBits) {
          dumper() << "  Lifted From " << Search->second.StageBits << " To "
                   << DepStageBits << "\n";
          Search->second.StageBits = DepStageBits;

          if (auto *DS = dyn_cast<DeclStmt>(S))
            Lifter.UpdateDeclRefExprStages(DS, DepStageBits);
        }
      }
    }

    unsigned PassthroughDependents() {
      unsigned LiftCount = 0;
      for (auto &Stmt : Lifter.StmtMap) {
        auto &StmtDeps = Stmt.second.Dependents;
        if (StmtDeps.empty())
          continue;
        DenseSet<const class Stmt *> NewStmtDeps;
        NewStmtDeps.reserve(StmtDeps.size());
        for (auto *Dep : StmtDeps) {
          if (isa<CastExpr>(Dep) || isa<DeclStmt>(Dep) ||
              isa<DeclRefExpr>(Dep) || isa<CXXConstructExpr>(Dep)) {
            auto Search = Lifter.StmtMap.find(Dep);
            if (Search != Lifter.StmtMap.end()) {
              auto &DREDeps = Search->second.Dependents;
              dumper() << "passing " << Dep->getStmtClassName()
                       << " dependent to " << Stmt.first->getStmtClassName()
                       << " " << Stmt.first << "\n";
              for (const auto *Deps : DREDeps)
                dumper() << "  " << Deps << "\n";
              NewStmtDeps.insert(DREDeps.begin(), DREDeps.end());
              ++LiftCount;
              continue;
            }
          }
          NewStmtDeps.insert(Dep);
        }
        StmtDeps = std::move(NewStmtDeps);
      }
      return LiftCount;
    }

    void run(AnalysisDeclContext &AD) {
      while (PassthroughDependents()) {
      }

      for (auto I = Lifter.OrderedStmts.rbegin(),
                E = Lifter.OrderedStmts.rend();
           I != E; ++I) {
        LiftToDependents(*I);
      }
    }
  };

  struct BlockDependencyPass : ConstStmtVisitor<BlockDependencyPass, unsigned> {
    StageStmtLifter &Lifter;
    explicit BlockDependencyPass(StageStmtLifter &Lifter) : Lifter(Lifter) {}

    unsigned VisitStmt(const Stmt *S) {
      if (auto *E = dyn_cast<Expr>(S))
        S = E->IgnoreParenImpCasts();
      auto Search = Lifter.StmtMap.find(S);
      if (Search != Lifter.StmtMap.end())
        return Search->second.StageBits;
      return 0;
    }

    unsigned VisitCompoundStmt(const CompoundStmt *CS) {
      auto &DepInfo = Lifter.StmtMap[CS];
      for (auto *Child : CS->body())
        DepInfo.StageBits |= Visit(Child);
      return DepInfo.StageBits;
    }

    unsigned VisitIfStmt(const IfStmt *S) {
      auto &DepInfo = Lifter.StmtMap[S];
      if (auto *Then = S->getThen())
        DepInfo.StageBits |= Visit(Then);
      if (auto *Else = S->getElse())
        DepInfo.StageBits |= Visit(Else);
      return DepInfo.StageBits;
    }

    template <typename T> unsigned VisitBody(const T *S) {
      auto &DepInfo = Lifter.StmtMap[S];
      if (auto *Body = S->getBody())
        DepInfo.StageBits |= Visit(Body);
      return DepInfo.StageBits;
    }
    unsigned VisitForStmt(const ForStmt *S) { return VisitBody(S); }
    unsigned VisitWhileStmt(const WhileStmt *S) { return VisitBody(S); }
    unsigned VisitDoStmt(const DoStmt *S) { return VisitBody(S); }
    unsigned VisitSwitchStmt(const SwitchStmt *S) { return VisitBody(S); }

    template <typename T> unsigned VisitSubStmt(const T *S) {
      auto &DepInfo = Lifter.StmtMap[S];
      if (auto *Sub = S->getSubStmt())
        DepInfo.StageBits |= Visit(Sub);
      return DepInfo.StageBits;
    }
    unsigned VisitCaseStmt(const CaseStmt *S) { return VisitSubStmt(S); }
    unsigned VisitDefaultStmt(const DefaultStmt *S) { return VisitSubStmt(S); }

    void run(AnalysisDeclContext &AD) { Visit(AD.getBody()); }
  };

  struct DeclUsagePass : StmtVisitor<DeclUsagePass, void, HshStage> {
    StageStmtLifter &Lifter;
    StagesBuilder &Builder;
    explicit DeclUsagePass(StageStmtLifter &Lifter, StagesBuilder &Builder)
        : Lifter(Lifter), Builder(Builder) {}

    void InterStageReferenceExpr(Expr *E, HshStage ToStage) {
      if (auto *DRE = dyn_cast<DeclRefExpr>(E))
        if (isa<EnumConstantDecl>(DRE->getDecl()))
          return;
      auto Search = Lifter.StmtMap.find(E);
      if (Search != Lifter.StmtMap.end()) {
        auto &DepInfo = Search->second;
        if (DepInfo.StageBits) {
          Visit(E, DepInfo.getMaxStage());
          return;
        }
      }
      Visit(E, ToStage);
    }

    void DoVisit(Stmt *S, HshStage Stage, bool ScopeBody = false) {
      dumper() << "Used Visiting for " << Stage << " " << S << "\n";
      if (auto *E = dyn_cast<Expr>(S))
        S = E->IgnoreParenImpCasts();
      if (isa<DeclStmt>(S) || isa<CaseStmt>(S) || isa<DefaultStmt>(S)) {
        /* DeclStmts and switch components passthrough unconditionally */
        Visit(S, Stage);
        return;
      } else if (isa<IntegerLiteral>(S) || isa<FloatingLiteral>(S) ||
                 isa<BreakStmt>(S) || isa<ContinueStmt>(S)) {
        /* Literals and control-flow leaves can go right where they are used */
        return;
      } else if (ScopeBody) {
        /* "root" statement if immediate child of a scope body */
      } else if (auto *E = dyn_cast<Expr>(S)) {
        /* Trace expression tree and establish inter-stage references */
        InterStageReferenceExpr(E, Stage);
        return;
      }
      /* "Root" statements of bodies are conditionally emitted based on stage */
      auto Search = Lifter.StmtMap.find(S);
      if (Search != Lifter.StmtMap.end() && Search->second.hasStage(Stage))
        Visit(S, Stage);
      /* Prune this statement */
    }

    template <typename T> void DoVisitExprRange(T Range, HshStage Stage) {
      for (Expr *E : Range)
        DoVisit(E, Stage);
    }

    /* Begin ignores */
    void VisitValueStmt(ValueStmt *ValueStmt, HshStage Stage) {
      DoVisit(ValueStmt->getExprStmt(), Stage);
    }

    void VisitUnaryOperator(UnaryOperator *UnOp, HshStage Stage) {
      DoVisit(UnOp->getSubExpr(), Stage);
    }

    void VisitConstantExpr(ConstantExpr *CE, HshStage Stage) {
      DoVisit(CE->getSubExpr(), Stage);
    }

    void VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *MTE,
                                       HshStage Stage) {
      DoVisit(MTE->getSubExpr(), Stage);
    }

    void VisitSubstNonTypeTemplateParmExpr(SubstNonTypeTemplateParmExpr *NTTP,
                                           HshStage Stage) {
      DoVisit(NTTP->getReplacement(), Stage);
    }
    /* End ignores */

    static void VisitStmt(Stmt *S, HshStage) {}

    void VisitDeclStmt(DeclStmt *DeclStmt, HshStage Stage) {
      for (Decl *D : DeclStmt->decls())
        if (auto *VD = dyn_cast<VarDecl>(D))
          if (Expr *Init = VD->getInit())
            DoVisit(Init, Stage);
    }

    void VisitCallExpr(CallExpr *CallExpr, HshStage Stage) {
      if (auto *DeclRef = dyn_cast<DeclRefExpr>(
              CallExpr->getCallee()->IgnoreParenImpCasts())) {
        if (auto *FD = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
          HshBuiltinFunction Func = Lifter.Builtins.identifyBuiltinFunction(FD);
          if (Func != HBF_None)
            DoVisitExprRange(CallExpr->arguments(), Stage);
        }
      }
    }

    void VisitCXXMemberCallExpr(CXXMemberCallExpr *CallExpr, HshStage Stage) {
      CXXMethodDecl *MD = CallExpr->getMethodDecl();
      Expr *ObjArg =
          CallExpr->getImplicitObjectArgument()->IgnoreParenImpCasts();
      HshBuiltinCXXMethod Method = Lifter.Builtins.identifyBuiltinMethod(MD);
      if (HshBuiltins::isSwizzleMethod(Method)) {
        DoVisit(ObjArg, Stage);
      }
      switch (Method) {
      case HBM_sample_texture2d:
        DoVisit(CallExpr->getArg(0), Stage);
        break;
      default:
        break;
      }
    }

    void VisitCastExpr(CastExpr *CastExpr, HshStage Stage) {
      if (Lifter.Builtins.identifyBuiltinType(CastExpr->getType()) == HBT_None)
        return;
      DoVisit(CastExpr->getSubExpr(), Stage);
    }

    void VisitCXXConstructExpr(CXXConstructExpr *ConstructExpr,
                               HshStage Stage) {
      if (Lifter.Builtins.identifyBuiltinType(ConstructExpr->getType()) ==
          HBT_None)
        return;
      DoVisitExprRange(ConstructExpr->arguments(), Stage);
    }

    void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *CallExpr,
                                  HshStage Stage) {
      DoVisitExprRange(CallExpr->arguments(), Stage);
    }

    void VisitBinaryOperator(BinaryOperator *BinOp, HshStage Stage) {
      DoVisit(BinOp->getLHS(), Stage);
      DoVisit(BinOp->getRHS(), Stage);
    }

    bool InMemberExpr = false;

    void VisitDeclRefExpr(DeclRefExpr *DeclRef, HshStage Stage) {
      if (auto *VD = dyn_cast<VarDecl>(DeclRef->getDecl())) {
        dumper() << VD << " Used in " << Stage << "\n";
        Lifter.UsedDecls[Stage].insert(VD);
      }
    }

    void VisitInitListExpr(InitListExpr *InitList, HshStage Stage) {
      DoVisitExprRange(InitList->inits(), Stage);
    }

    void VisitMemberExpr(MemberExpr *MemberExpr, HshStage Stage) {
      if (!InMemberExpr && Lifter.Builtins.identifyBuiltinType(
                               MemberExpr->getType()) == HBT_None)
        return;
      SaveAndRestore<bool> SavedInMemberExpr(InMemberExpr, true);
      DoVisit(MemberExpr->getBase(), Stage);
    }

    void VisitDoStmt(DoStmt *S, HshStage Stage) {
      if (auto *Cond = S->getCond())
        DoVisit(Cond, Stage);
      if (auto *Body = S->getBody())
        DoVisit(Body, Stage, true);
    }

    void VisitForStmt(ForStmt *S, HshStage Stage) {
      if (auto *Init = S->getInit())
        DoVisit(Init, Stage);
      if (auto *Cond = S->getCond())
        DoVisit(Cond, Stage);
      if (auto *Inc = S->getInc())
        DoVisit(Inc, Stage);
      if (auto *Body = S->getBody())
        DoVisit(Body, Stage, true);
    }

    void VisitIfStmt(IfStmt *S, HshStage Stage) {
      if (auto *Init = S->getInit())
        DoVisit(Init, Stage);
      if (auto *Cond = S->getCond())
        DoVisit(Cond, Stage);
      if (auto *Then = S->getThen())
        DoVisit(Then, Stage, true);
      if (auto *Else = S->getElse())
        DoVisit(Else, Stage, true);
    }

    void VisitCaseStmt(CaseStmt *S, HshStage Stage) {
      if (Stmt *St = S->getSubStmt())
        DoVisit(St, Stage, true);
    }

    void VisitDefaultStmt(DefaultStmt *S, HshStage Stage) {
      if (Stmt *St = S->getSubStmt())
        DoVisit(St, Stage, true);
    }

    void VisitSwitchStmt(SwitchStmt *S, HshStage Stage) {
      if (auto *Cond = S->getCond())
        DoVisit(Cond, Stage);
      if (auto *Body = S->getBody())
        DoVisit(Body, Stage);
    }

    void VisitWhileStmt(WhileStmt *S, HshStage Stage) {
      if (auto *Cond = S->getCond())
        DoVisit(Cond, Stage);
      if (auto *Body = S->getBody())
        DoVisit(Body, Stage, true);
    }

    void VisitCompoundStmt(CompoundStmt *S, HshStage Stage) {
      for (auto *CS : S->body())
        DoVisit(CS, Stage, true);
    }

    void run(AnalysisDeclContext &AD) {
      for (int i = HshHostStage; i < HshMaxStage; ++i) {
        auto Stage = HshStage(i);
        if (!Builder.isStageUsed(Stage))
          continue;
        if (auto *Body = dyn_cast<CompoundStmt>(AD.getBody())) {
          for (auto *Stmt : Body->body())
            DoVisit(Stmt, Stage, true);
        } else {
          DoVisit(AD.getBody(), Stage, true);
        }
      }
    }
  };

  struct BuildPass : StmtVisitor<BuildPass, Stmt *, HshStage> {
    StageStmtLifter &Lifter;
    StagesBuilder &Builder;
    explicit BuildPass(StageStmtLifter &Lifter, StagesBuilder &Builder)
        : Lifter(Lifter), Builder(Builder) {}

    bool hasErrorOccurred() const {
      return Lifter.Context.getDiagnostics().hasErrorOccurred();
    }

    Expr *InterStageReferenceExpr(Expr *E, HshStage ToStage) {
      if (auto *DRE = dyn_cast<DeclRefExpr>(E)) {
        if (isa<EnumConstantDecl>(DRE->getDecl()))
          return E;
      }
      auto Search = Lifter.StmtMap.find(E);
      if (Search != Lifter.StmtMap.end()) {
        auto &DepInfo = Search->second;
        if (!DepInfo.StageBits)
          return cast_or_null<Expr>(Visit(E, ToStage));
        E = cast_or_null<Expr>(Visit(E, DepInfo.getMaxStage()));
        if (!E)
          return nullptr;
        return Builder.createInterStageReferenceExpr(E, DepInfo.getMaxStage(),
                                                     ToStage);
      }
      return cast_or_null<Expr>(Visit(E, ToStage));
    }

    Stmt *DoVisit(Stmt *S, HshStage Stage, bool ScopeBody = false) {
      dumper() << "Visiting for " << Stage << " " << S << "\n";
      if (auto *E = dyn_cast<Expr>(S))
        S = E->IgnoreParenImpCasts();
      if (isa<DeclStmt>(S) || isa<CaseStmt>(S) || isa<DefaultStmt>(S)) {
        /* DeclStmts and switch components passthrough unconditionally */
        return Visit(S, Stage);
      } else if (isa<IntegerLiteral>(S) || isa<FloatingLiteral>(S) ||
                 isa<BreakStmt>(S) || isa<ContinueStmt>(S)) {
        /* Literals and control-flow leaves can go right where they are used */
        return S;
      } else if (ScopeBody) {
        /* "root" statement if immediate child of a scope body */
      } else if (auto *E = dyn_cast<Expr>(S)) {
        /* Trace expression tree and establish inter-stage references */
        return InterStageReferenceExpr(E, Stage);
      }
      /* "Root" statements of bodies are conditionally emitted based on stage */
      auto Search = Lifter.StmtMap.find(S);
      if (Search != Lifter.StmtMap.end() && Search->second.hasStage(Stage))
        return Visit(S, Stage);
      /* Prune this statement */
      return nullptr;
    }

    using ExprRangeRet = SmallVector<Expr *, 4>;
    template <typename T>
    Optional<ExprRangeRet> DoVisitExprRange(T Range, HshStage Stage) {
      ExprRangeRet Res;
      for (Expr *E : Range) {
        Stmt *ExprStmt = DoVisit(E, Stage);
        if (!ExprStmt)
          return {};
        Res.push_back(cast<Expr>(ExprStmt));
      }
      return {Res};
    }

    /* Begin ignores */
    Stmt *VisitValueStmt(ValueStmt *ValueStmt, HshStage Stage) {
      return DoVisit(ValueStmt->getExprStmt(), Stage);
    }

    Stmt *VisitUnaryOperator(UnaryOperator *UnOp, HshStage Stage) {
      return DoVisit(UnOp->getSubExpr(), Stage);
    }

    Stmt *VisitConstantExpr(ConstantExpr *CE, HshStage Stage) {
      return DoVisit(CE->getSubExpr(), Stage);
    }

    Stmt *VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *MTE,
                                        HshStage Stage) {
      return DoVisit(MTE->getSubExpr(), Stage);
    }

    Stmt *VisitSubstNonTypeTemplateParmExpr(SubstNonTypeTemplateParmExpr *NTTP,
                                            HshStage Stage) {
      return DoVisit(NTTP->getReplacement(), Stage);
    }
    /* End ignores */

    Stmt *VisitStmt(Stmt *S, HshStage) {
      ReportUnsupportedStmt(S, Lifter.Context);
      return nullptr;
    }

    Stmt *VisitExpr(Expr *E, HshStage) {
      ReportUnsupportedStmt(E, Lifter.Context);
      return nullptr;
    }

    Stmt *VisitDeclStmt(DeclStmt *DS, HshStage Stage) {
      SmallVector<Decl *, 4> NewDecls;
      for (auto *Decl : DS->decls()) {
        if (auto *VD = dyn_cast<VarDecl>(Decl)) {
          if (Lifter.UsedDecls[Stage].find(VD) !=
              Lifter.UsedDecls[Stage].end()) {
            auto *NewVD =
                VarDecl::Create(Lifter.Context, VD->getDeclContext(), {}, {},
                                VD->getIdentifier(), VD->getType(),
                                VD->getTypeSourceInfo(), VD->getStorageClass());
            if (Expr *Init = VD->getInit()) {
              auto *InitStmt = DoVisit(Init, Stage);
              if (!InitStmt)
                return nullptr;
              NewVD->setInit(cast<Expr>(InitStmt));
            }
            NewDecls.push_back(NewVD);
          }
        } else {
          ReportUnsupportedTypeReference(DS, Lifter.Context);
          return nullptr;
        }
      }
      if (!NewDecls.empty()) {
        return new (Lifter.Context)
            DeclStmt(DeclGroupRef::Create(Lifter.Context, NewDecls.data(),
                                          NewDecls.size()),
                     {}, {});
      }
      return nullptr;
    }

    static Stmt *VisitNullStmt(NullStmt *NS, HshStage) { return NS; }

    Stmt *VisitCallExpr(CallExpr *CallExpr, HshStage Stage) {
      if (auto *DeclRef = dyn_cast<DeclRefExpr>(
              CallExpr->getCallee()->IgnoreParenImpCasts())) {
        if (auto *FD = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
          HshBuiltinFunction Func = Lifter.Builtins.identifyBuiltinFunction(FD);
          if (Func != HBF_None) {
            auto Arguments = DoVisitExprRange(CallExpr->arguments(), Stage);
            if (!Arguments)
              return nullptr;
            return CallExpr::Create(Lifter.Context, CallExpr->getCallee(),
                                    *Arguments, CallExpr->getType(), VK_XValue,
                                    {});
          }
        }
      }
      ReportUnsupportedFunctionCall(CallExpr, Lifter.Context);
      return nullptr;
    }

    Stmt *VisitCXXMemberCallExpr(CXXMemberCallExpr *CallExpr, HshStage Stage) {
      CXXMethodDecl *MD = CallExpr->getMethodDecl();
      Expr *ObjArg =
          CallExpr->getImplicitObjectArgument()->IgnoreParenImpCasts();
      HshBuiltinCXXMethod Method = Lifter.Builtins.identifyBuiltinMethod(MD);
      if (HshBuiltins::isSwizzleMethod(Method)) {
        auto *BaseStmt = DoVisit(ObjArg, Stage);
        return MemberExpr::CreateImplicit(Lifter.Context, cast<Expr>(BaseStmt),
                                          false, MD, MD->getReturnType(),
                                          VK_XValue, OK_Ordinary);
      }
      switch (Method) {
      case HBM_sample_texture2d: {
        ParmVarDecl *PVD = nullptr;
        if (auto *TexRef = dyn_cast<DeclRefExpr>(ObjArg))
          PVD = dyn_cast<ParmVarDecl>(TexRef->getDecl());
        if (PVD) {
          if (!PVD->hasAttr<HshVertexTextureAttr>() &&
              !PVD->hasAttr<HshFragmentTextureAttr>())
            ReportUnattributedTexture(PVD, Lifter.Context);
        } else {
          ReportBadTextureReference(CallExpr, Lifter.Context);
        }
        auto *UVStmt = DoVisit(CallExpr->getArg(0), Stage);
        if (!UVStmt)
          return nullptr;
        std::array<Expr *, 2> NewArgs{cast<Expr>(UVStmt), CallExpr->getArg(1)};
        auto *NMCE = CXXMemberCallExpr::Create(
            Lifter.Context, CallExpr->getCallee(), NewArgs, CallExpr->getType(),
            VK_XValue, {});
        Builder.registerSampleCall(Method, NMCE);
        return NMCE;
      }
      default:
        ReportUnsupportedFunctionCall(CallExpr, Lifter.Context);
        break;
      }
      return nullptr;
    }

    Stmt *VisitCastExpr(CastExpr *CastExpr, HshStage Stage) {
      if (Lifter.Builtins.identifyBuiltinType(CastExpr->getType()) ==
          HBT_None) {
        ReportUnsupportedTypeCast(CastExpr, Lifter.Context);
        return nullptr;
      }
      return DoVisit(CastExpr->getSubExpr(), Stage);
    }

    Stmt *VisitCXXConstructExpr(CXXConstructExpr *ConstructExpr,
                                HshStage Stage) {
      if (Lifter.Builtins.identifyBuiltinType(ConstructExpr->getType()) ==
          HBT_None) {
        ReportUnsupportedTypeConstruct(ConstructExpr, Lifter.Context);
        return nullptr;
      }

      auto Arguments = DoVisitExprRange(ConstructExpr->arguments(), Stage);
      if (!Arguments)
        return nullptr;
      return CXXTemporaryObjectExpr::Create(
          Lifter.Context, ConstructExpr->getConstructor(),
          ConstructExpr->getType(),
          Lifter.Context.getTrivialTypeSourceInfo(ConstructExpr->getType()),
          *Arguments, {}, ConstructExpr->hadMultipleCandidates(),
          ConstructExpr->isListInitialization(),
          ConstructExpr->isStdInitListInitialization(),
          ConstructExpr->requiresZeroInitialization());
    }

    Stmt *VisitCXXOperatorCallExpr(CXXOperatorCallExpr *CallExpr,
                                   HshStage Stage) {
      auto Arguments = DoVisitExprRange(CallExpr->arguments(), Stage);
      if (!Arguments)
        return nullptr;
      if (CallExpr->isAssignmentOp()) {
        Expr *LHS = (*Arguments)[0];
        if (LHS->getType().isConstQualified())
          ReportConstAssignment(CallExpr, Lifter.Context);
      }
      return CXXOperatorCallExpr::Create(
          Lifter.Context, CallExpr->getOperator(), CallExpr->getCallee(),
          *Arguments, CallExpr->getType(), VK_XValue, {}, {});
    }

    Stmt *VisitBinaryOperator(BinaryOperator *BinOp, HshStage Stage) {
      auto *LStmt = DoVisit(BinOp->getLHS(), Stage);
      if (!LStmt)
        return nullptr;
      auto *RStmt = DoVisit(BinOp->getRHS(), Stage);
      if (!RStmt)
        return nullptr;
      if (BinOp->isAssignmentOp()) {
        if (cast<Expr>(LStmt)->getType().isConstQualified())
          ReportConstAssignment(BinOp, Lifter.Context);
      }
      return new (Lifter.Context) BinaryOperator(
          cast<Expr>(LStmt), cast<Expr>(RStmt), BinOp->getOpcode(),
          BinOp->getType(), VK_XValue, OK_Ordinary, {}, {});
    }

    bool InMemberExpr = false;

    Stmt *VisitDeclRefExpr(DeclRefExpr *DeclRef, HshStage Stage) {
      if (auto *VD = dyn_cast<VarDecl>(DeclRef->getDecl())) {
        if (!InMemberExpr && !CheckHshFieldTypeCompatibility(
                                 Lifter.Builtins, Lifter.Context, VD)) {
          ReportUnsupportedTypeReference(DeclRef, Lifter.Context);
          return nullptr;
        }
        if (auto *PVD = dyn_cast<ParmVarDecl>(DeclRef->getDecl())) {
          HshStage PVDStage = DetermineParmVarStage(PVD);
          if (PVDStage == HshHostStage) {
            if (!CheckHshFieldTypeCompatibility(Lifter.Builtins, Lifter.Context,
                                                PVD))
              return nullptr;
            Builder.registerUsedCapture(PVD);
          }
        } else if (!VD->hasLocalStorage()) {
          if (!CheckHshFieldTypeCompatibility(Lifter.Builtins, Lifter.Context,
                                              VD))
            return nullptr;
          Builder.registerUsedCapture(VD);
        }
        auto Search = Lifter.StmtMap.find(DeclRef);
        if (Search != Lifter.StmtMap.end()) {
          auto &DepInfo = Search->second;
          if (DepInfo.StageBits)
            return Builder.createInterStageReferenceExpr(
                DeclRef, DepInfo.getMaxStage(), Stage);
        }
        return DeclRef;
      } else if (isa<EnumConstantDecl>(DeclRef->getDecl())) {
        return DeclRef;
      } else {
        ReportUnsupportedTypeReference(DeclRef, Lifter.Context);
        return nullptr;
      }
    }

    Stmt *VisitInitListExpr(InitListExpr *InitList, HshStage Stage) {
      auto Exprs = DoVisitExprRange(InitList->inits(), Stage);
      if (!Exprs)
        return nullptr;
      return new (Lifter.Context) InitListExpr(Lifter.Context, {}, *Exprs, {});
    }

    Stmt *VisitMemberExpr(MemberExpr *MemberExpr, HshStage Stage) {
      if (!InMemberExpr && Lifter.Builtins.identifyBuiltinType(
                               MemberExpr->getType()) == HBT_None) {
        ReportUnsupportedTypeReference(MemberExpr, Lifter.Context);
        return nullptr;
      }
      SaveAndRestore<bool> SavedInMemberExpr(InMemberExpr, true);
      auto *BaseStmt = DoVisit(MemberExpr->getBase(), Stage);
      return MemberExpr::CreateImplicit(Lifter.Context, cast<Expr>(BaseStmt),
                                        false, MemberExpr->getMemberDecl(),
                                        MemberExpr->getType(), VK_XValue,
                                        OK_Ordinary);
    }

    Stmt *VisitDoStmt(DoStmt *S, HshStage Stage) {
      dumper() << "Do ";
      Expr *NewCond = nullptr;
      if (auto *Cond = S->getCond()) {
        NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
        dumper() << Cond;
      }
      dumper() << ":\n";
      Stmt *NewBody = nullptr;
      if (auto *Body = S->getBody()) {
        NewBody = DoVisit(Body, Stage, true);
        dumper() << Body;
      }
      if (hasErrorOccurred())
        return nullptr;
      return new (Lifter.Context) DoStmt(NewBody, NewCond, {}, {}, {});
    }

    Stmt *VisitForStmt(ForStmt *S, HshStage Stage) {
      dumper() << "For ";
      Stmt *NewInit = nullptr;
      if (auto *Init = S->getInit()) {
        NewInit = DoVisit(Init, Stage);
        dumper() << Init;
      }
      dumper() << "; ";
      Expr *NewCond = nullptr;
      if (auto *Cond = S->getCond()) {
        NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
        dumper() << Cond;
      }
      dumper() << "; ";
      Expr *NewInc = nullptr;
      if (auto *Inc = S->getInc()) {
        NewInc = cast_or_null<Expr>(DoVisit(Inc, Stage));
        dumper() << Inc;
      }
      dumper() << ":\n";
      Stmt *NewBody = nullptr;
      if (auto *Body = S->getBody()) {
        NewBody = DoVisit(Body, Stage, true);
        dumper() << Body;
      }
      if (hasErrorOccurred())
        return nullptr;
      return new (Lifter.Context)
          ForStmt(Lifter.Context, NewInit, NewCond, S->getConditionVariable(),
                  NewInc, NewBody, {}, {}, {});
    }

    Stmt *VisitIfStmt(IfStmt *S, HshStage Stage) {
      dumper() << "If ";
      Stmt *NewInit = nullptr;
      if (auto *Init = S->getInit()) {
        NewInit = DoVisit(Init, Stage);
      }
      Expr *NewCond = nullptr;
      if (auto *Cond = S->getCond()) {
        NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
        dumper() << Cond;
      }
      dumper() << ":\n";
      Stmt *NewThen = nullptr;
      if (auto *Then = S->getThen()) {
        NewThen = DoVisit(Then, Stage, true);
        dumper() << Then;
      }
      Stmt *NewElse = nullptr;
      if (auto *Else = S->getElse()) {
        NewElse = DoVisit(Else, Stage, true);
        dumper() << "Else:\n" << Else;
      }
      if (hasErrorOccurred())
        return nullptr;
      return IfStmt::Create(Lifter.Context, {}, S->isConstexpr(), NewInit,
                            S->getConditionVariable(), NewCond, NewThen,
                            SourceLocation{}, NewElse);
    }

    Stmt *VisitCaseStmt(CaseStmt *S, HshStage Stage) {
      dumper() << "case " << S->getLHS() << ":\n";
      Stmt *NewSubStmt = nullptr;
      if (Stmt *St = S->getSubStmt())
        NewSubStmt = DoVisit(St, Stage, true);
      if (hasErrorOccurred())
        return nullptr;
      auto *NewCase =
          CaseStmt::Create(Lifter.Context, S->getLHS(), nullptr, {}, {}, {});
      NewCase->setSubStmt(NewSubStmt);
      dumper() << "\n";
      return NewCase;
    }

    Stmt *VisitDefaultStmt(DefaultStmt *S, HshStage Stage) {
      dumper() << "default:\n";
      Stmt *NewSubStmt = nullptr;
      if (Stmt *St = S->getSubStmt())
        NewSubStmt = DoVisit(St, Stage, true);
      if (hasErrorOccurred())
        return nullptr;
      auto *NewDefault = new (Lifter.Context) DefaultStmt({}, {}, NewSubStmt);
      dumper() << "\n";
      return NewDefault;
    }

    static Stmt *VisitBreakStmt(BreakStmt *S, HshStage Stage) {
      dumper() << S;
      return S;
    }

    static Stmt *VisitContinueStmt(ContinueStmt *S, HshStage Stage) {
      dumper() << S;
      return S;
    }

    Stmt *VisitSwitchStmt(SwitchStmt *S, HshStage Stage) {
      dumper() << "Switch ";
      if (Stmt *Init = S->getInit()) {
        auto &Diags = Lifter.Context.getDiagnostics();
        Diags.Report(
            Init->getBeginLoc(),
            Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                  "C++17 switch init statements not supported"))
            << Init->getSourceRange();
      }
      Expr *NewCond = nullptr;
      if (auto *Cond = S->getCond()) {
        NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
        dumper() << Cond;
      }
      dumper() << ":\n";
      Stmt *NewBody = nullptr;
      if (auto *Body = S->getBody())
        NewBody = DoVisit(Body, Stage);
      if (hasErrorOccurred())
        return nullptr;
      auto *NewSwitch = SwitchStmt::Create(Lifter.Context, nullptr,
                                           S->getConditionVariable(), NewCond);
      NewSwitch->setBody(NewBody);
      return NewSwitch;
    }

    Stmt *VisitWhileStmt(WhileStmt *S, HshStage Stage) {
      dumper() << "While ";
      Expr *NewCond = nullptr;
      if (auto *Cond = S->getCond()) {
        NewCond = cast_or_null<Expr>(DoVisit(Cond, Stage));
        dumper() << Cond;
      }
      dumper() << ":\n";
      Stmt *NewBody = nullptr;
      if (auto *Body = S->getBody())
        NewBody = DoVisit(Body, Stage, true);
      if (hasErrorOccurred())
        return nullptr;
      return WhileStmt::Create(Lifter.Context, S->getConditionVariable(),
                               NewCond, NewBody, {});
    }

    Stmt *VisitCompoundStmt(CompoundStmt *S, HshStage Stage) {
      dumper() << "{\n";
      SmallVector<Stmt *, 16> Stmts;
      Stmts.reserve(S->size());
      for (auto *CS : S->body())
        if (auto *NewStmt = DoVisit(CS, Stage, true))
          Stmts.push_back(NewStmt);
      dumper() << "}\n";
      if (hasErrorOccurred())
        return nullptr;
      return CompoundStmt::Create(Lifter.Context, Stmts, {}, {});
    }

    void run(AnalysisDeclContext &AD) {
      for (int i = HshHostStage; i < HshMaxStage; ++i) {
        auto Stage = HshStage(i);
        if (!Builder.isStageUsed(Stage))
          continue;
        dumper() << "Statements for " << Stage << ":\n";
        if (auto *Body = dyn_cast<CompoundStmt>(AD.getBody())) {
          dumper() << "{\n";
          for (auto *Stmt : Body->body()) {
            if (auto *NewStmt = DoVisit(Stmt, Stage, true)) {
              dumper() << NewStmt << "\n";
              Builder.addStageStmt(NewStmt, Stage);
            }
            if (hasErrorOccurred())
              return;
          }
          dumper() << "}";
        } else {
          if (auto *NewStmt = DoVisit(AD.getBody(), Stage, true)) {
            dumper() << NewStmt << "\n";
            Builder.addStageStmt(NewStmt, Stage);
          }
          if (hasErrorOccurred())
            return;
        }
        dumper() << "\n";
      }
    }
  };

  ASTContext &Context;
  HshBuiltins &Builtins;

public:
  StageStmtLifter(ASTContext &Context, HshBuiltins &Builtins)
      : Context(Context), Builtins(Builtins) {}

  void run(AnalysisDeclContext &AD, StagesBuilder &Builder) {
    DependencyPass(*this, Builder).run(AD);
    LiftPass(*this).run(AD);
    BlockDependencyPass(*this).run(AD);
    for (auto &P : StmtMap)
      dumper() << "Stmt " << P.first << " " << P.second.StageBits << "\n";
    DeclUsagePass(*this, Builder).run(AD);
    BuildPass(*this, Builder).run(AD);
  }
};

class GenerateConsumer : public ASTConsumer, MatchFinder::MatchCallback {
  HshBuiltins Builtins;
  CompilerInstance &CI;
  ASTContext &Context;
  HostPrintingPolicy HostPolicy;
  AnalysisDeclContextManager AnalysisMgr;
  Preprocessor &PP;
  StringRef ProgramDir;
  ArrayRef<HshTarget> Targets;
  std::unique_ptr<raw_pwrite_stream> OS;
  llvm::DenseSet<uint64_t> SeenHashes;
  std::string AnonNSString;
  raw_string_ostream AnonOS{AnonNSString};
  Optional<std::pair<SourceLocation, std::string>> HeadInclude;
  std::map<SourceLocation, std::pair<SourceRange, std::string>>
      SeenHshExpansions;

  BinaryOperator *MakeAssignOp(ValueDecl *VD, Expr *RHS) const {
    return new (Context) BinaryOperator(
        DeclRefExpr::Create(Context, {}, {}, VD, false, SourceLocation{},
                            VD->getType(), VK_XValue),
        RHS, BO_Assign, VD->getType(), VK_XValue, OK_Ordinary, {}, {});
  }

public:
  explicit GenerateConsumer(CompilerInstance &CI, StringRef ProgramDir,
                            ArrayRef<HshTarget> Targets)
      : CI(CI), Context(CI.getASTContext()),
        HostPolicy(Context.getPrintingPolicy()), AnalysisMgr(Context),
        PP(CI.getPreprocessor()), ProgramDir(ProgramDir), Targets(Targets) {
    AnalysisMgr.getCFGBuildOptions().OmitLogicalBinaryOperators = true;
  }

  void run(const MatchFinder::MatchResult &Result) override {
    auto *LambdaAttr = Result.Nodes.getNodeAs<AttributedStmt>("attrid"_ll);
    auto *Lambda = Result.Nodes.getNodeAs<LambdaExpr>("id"_ll);
    if (Lambda && LambdaAttr) {
      auto ExpName = getExpansionNameBeforeLambda(LambdaAttr);
      assert(!ExpName.empty() && "Expansion name should exist");

      auto *CallOperator = Lambda->getCallOperator();
      SmallVector<Stmt *, 32> NewBodyStmts;
      {
        std::size_t NumBodyStmts = 1;
        if (auto *Body = dyn_cast<CompoundStmt>(CallOperator->getBody()))
          NumBodyStmts = Body->size();
        NewBodyStmts.reserve(NumBodyStmts + 4);
      }

#if ENABLE_DUMP
      ASTDumper Dumper(llvm::errs(), nullptr, &Context.getSourceManager());
      Dumper.Visit(CallOperator->getBody());
      llvm::errs() << '\n';
#endif

      unsigned UseStages = 1;

      std::array<ParmVarDecl *, HSH_MAX_VERTEX_BUFFERS> VertexBufferParms{};
      auto CheckVertexBufferParm = [&](ParmVarDecl *PVD, auto *Attr) {
        bool Ret = true;
        auto NonRefType = PVD->getType().getNonReferenceType();
        if (!NonRefType->isStructureOrClassType()) {
          ReportBadVertexBufferType(PVD, Context);
          Ret = false;
        } else if (!CheckHshRecordCompatibility(
                       Builtins, Context, NonRefType->getAsCXXRecordDecl())) {
          Ret = false;
        }
        llvm::APSInt Res;
        Attr->getIndex()->isIntegerConstantExpr(Res, Context);
        if (Res < 0 || Res >= HSH_MAX_VERTEX_BUFFERS) {
          ReportVertexBufferOutOfRange(PVD, Context);
          Ret = false;
        }
        if (auto *OtherPVD = VertexBufferParms[Res.getExtValue()]) {
          ReportVertexBufferDuplicate(PVD, OtherPVD, Context);
          Ret = false;
        }
        if (!Ret)
          return false;
        VertexBufferParms[Res.getExtValue()] = PVD;
        return true;
      };

      std::array<std::array<ParmVarDecl *, HSH_MAX_TEXTURES>, HshMaxStage>
          TextureParms{};
      auto CheckTextureParm = [&](ParmVarDecl *PVD, auto *Attr,
                                  HshStage Stage) {
        auto &StageTextureParms = TextureParms[Stage];
        bool Ret = true;
        if (!HshBuiltins::isTextureType(
                Builtins.identifyBuiltinType(PVD->getType()))) {
          ReportBadTextureType(PVD, Context);
          Ret = false;
        }
        llvm::APSInt Res;
        Attr->getIndex()->isIntegerConstantExpr(Res, Context);
        if (Res < 0 || Res >= HSH_MAX_TEXTURES) {
          ReportTextureOutOfRange(PVD, Context);
          Ret = false;
        }
        if (auto *OtherPVD = StageTextureParms[Res.getExtValue()]) {
          ReportTextureDuplicate(PVD, OtherPVD, Context);
          Ret = false;
        }
        if (!Ret)
          return false;
        StageTextureParms[Res.getExtValue()] = PVD;
        return true;
      };

      std::array<ParmVarDecl *, HSH_MAX_COLOR_TARGETS> ColorTargetParms{};
      auto CheckColorTargetParm = [&](ParmVarDecl *PVD,
                                      HshColorTargetAttr *Attr) {
        bool Ret = true;
        if (Builtins.identifyBuiltinType(PVD->getType()) != HBT_float4) {
          ReportBadColorTargetType(PVD, Context);
          Ret = false;
        }
        llvm::APSInt Res;
        Attr->getIndex()->isIntegerConstantExpr(Res, Context);
        if (Res < 0 || Res >= HSH_MAX_COLOR_TARGETS) {
          ReportColorTargetOutOfRange(PVD, Context);
          Ret = false;
        }
        if (!Ret)
          return false;
        ColorTargetParms[Res.getExtValue()] = PVD;
        return true;
      };

      for (ParmVarDecl *Param : CallOperator->parameters()) {
        HshInterfaceDirection Direction = DetermineParmVarDirection(Param);
        if (Direction != HshInput) {
          if (Param->hasAttr<HshPositionAttr>()) {
            if (Builtins.identifyBuiltinType(Param->getType()) != HBT_float4) {
              ReportBadVertexPositionType(Param, Context);
              return;
            }
          } else if (auto *CA = Param->getAttr<HshColorTargetAttr>()) {
            if (!CheckColorTargetParm(Param, CA))
              return;
          }
          UseStages |= (1u << unsigned(DetermineParmVarStage(Param)));
        } else {
          if (auto *VB = Param->getAttr<HshVertexBufferAttr>()) {
            if (!CheckVertexBufferParm(Param, VB))
              return;
          } else if (auto *IB = Param->getAttr<HshInstanceBufferAttr>()) {
            if (!CheckVertexBufferParm(Param, IB))
              return;
          } else if (auto *VTA = Param->getAttr<HshVertexTextureAttr>()) {
            if (!CheckTextureParm(Param, VTA, HshVertexStage))
              return;
          } else if (auto *FTA = Param->getAttr<HshFragmentTextureAttr>()) {
            if (!CheckTextureParm(Param, FTA, HshFragmentStage))
              return;
          }
        }
      }

      CXXRecordDecl *SpecRecord =
          Builtins.getHshBaseSpecialization(Context, ExpName);
      StagesBuilder Builder(Context, Builtins, SpecRecord, UseStages);

      for (uint8_t i = 0; i < HSH_MAX_VERTEX_BUFFERS; ++i) {
        if (ParmVarDecl *VertexBufferParm = VertexBufferParms[i]) {
          Builder.registerAttributeRecord(
              {VertexBufferParm->getName(),
               VertexBufferParm->getType()
                   .getNonReferenceType()
                   ->getAsCXXRecordDecl(),
               VertexBufferParm->hasAttr<HshVertexBufferAttr>() ? PerVertex
                                                                : PerInstance,
               i});
        }
      }

      for (int s = HshVertexStage; s < HshMaxStage; ++s) {
        for (uint8_t i = 0; i < HSH_MAX_TEXTURES; ++i) {
          if (ParmVarDecl *TextureParm = TextureParms[s][i]) {
            Builder.registerTexture(
                TextureParm->getName(),
                KindOfTextureType(
                    Builtins.identifyBuiltinType(TextureParm->getType())),
                HshStage(s));
          }
        }
      }

      for (uint8_t i = 0; i < HSH_MAX_COLOR_TARGETS; ++i) {
        if (ParmVarDecl *CTParm = ColorTargetParms[i]) {
          Builder.registerColorTarget(ColorTargetRecord{CTParm->getName(), i});
          if (auto *Init = CTParm->getInit())
            NewBodyStmts.push_back(MakeAssignOp(CTParm, Init));
        }
      }

      if (auto *Body = dyn_cast<CompoundStmt>(CallOperator->getBody()))
        NewBodyStmts.insert(NewBodyStmts.end(), Body->body_begin(),
                            Body->body_end());
      CallOperator->setBody(
          CompoundStmt::Create(Context, NewBodyStmts, {}, {}));
      auto *CallCtx = AnalysisMgr.getContext(CallOperator);
#if ENABLE_DUMP
      CallCtx->dumpCFG(true);
#endif
      StageStmtLifter(Context, Builtins).run(*CallCtx, Builder);

      // Add global list node static
      SpecRecord->addDecl(Builtins.getGlobalListNode(Context, SpecRecord));

      // Finalize expressions and add host to stage records
      Builder.finalizeResults(SpecRecord);

      // Set public access
      SpecRecord->addDecl(
          AccessSpecDecl::Create(Context, AS_public, SpecRecord, {}, {}));

      // Make constructor
      SmallVector<QualType, 4> ConstructorArgs;
      SmallVector<ParmVarDecl *, 4> ConstructorParms;
      for (const auto *Cap : Builder.captures()) {
        ConstructorArgs.push_back(
            Cap->getType().isPODType(Context)
                ? Cap->getType()
                : Context.getLValueReferenceType(Cap->getType().withConst()));
        ConstructorParms.push_back(ParmVarDecl::Create(
            Context, SpecRecord, {}, {}, Cap->getIdentifier(),
            ConstructorArgs.back(), {}, SC_None, nullptr));
      }
      CanQualType CDType =
          SpecRecord->getTypeForDecl()->getCanonicalTypeUnqualified();
      CXXConstructorDecl *CD = CXXConstructorDecl::Create(
          Context, SpecRecord, {},
          {Context.DeclarationNames.getCXXConstructorName(CDType), {}},
          Context.getFunctionType(CDType, ConstructorArgs, {}), {},
          {nullptr, ExplicitSpecKind::ResolvedTrue}, false, false,
          CSK_unspecified);
      CD->setParams(ConstructorParms);
      CD->setAccess(AS_public);
      CD->setBody(
          CompoundStmt::Create(Context, Builder.getHostStmts(), {}, {}));
      SpecRecord->addDecl(CD);

      // Add shader data var template
      SpecRecord->addDecl(Builtins.getConstDataVarTemplate(
          Context, SpecRecord, Builder.getNumStages(), Builder.getNumBindings(),
          Builder.getNumAttributes()));
      SpecRecord->addDecl(Builtins.getDataVarTemplate(Context, SpecRecord,
                                                      Builder.getNumStages()));

      SpecRecord->completeDefinition();

      // Emit shader record
      SpecRecord->print(AnonOS, HostPolicy);
      AnonOS << ";\nhsh::detail::GlobalListNode " << ExpName << "::global{&"
             << ExpName << "::global_build};\n";

      // Emit shader data
      for (auto Target : Targets) {
        auto Policy = MakePrintingPolicy(Builtins, Target);
        auto Sources = Builder.printResults(*Policy);
        auto Compiler =
            MakeCompiler(Sources, ProgramDir, Context.getDiagnostics());
        if (Context.getDiagnostics().hasErrorOccurred())
          return;
        auto Binaries = Compiler->compile();
        auto SourceIt = Sources.Sources.begin();
        int StageIt = HshHostStage;

        AnonOS << "template <> constexpr hsh::detail::ShaderConstData<";
        Builtins.printTargetEnumString(AnonOS, HostPolicy, Target);
        AnonOS << ", " << Builder.getNumStages() << ", "
               << Builder.getNumBindings() << ", " << Builder.getNumAttributes()
               << "> " << ExpName << "::cdata<";
        Builtins.printTargetEnumString(AnonOS, HostPolicy, Target);
        AnonOS << ">{\n  {\n";

        for (auto &[Data, Hash] : Binaries.Binaries) {
          auto &Source = *SourceIt++;
          auto Stage = HshStage(StageIt++);
          if (Data.empty())
            continue;
          auto HashStr = llvm::utohexstr(Hash);
          AnonOS << "    hsh::detail::ShaderCode<";
          Builtins.printTargetEnumString(AnonOS, HostPolicy, Target);
          AnonOS << ">{";
          Builtins.printStageEnumString(AnonOS, HostPolicy, Stage);
          AnonOS << ", {_hshs_" << HashStr << ", 0x" << HashStr << "}},\n";
          if (SeenHashes.find(Hash) != SeenHashes.end())
            continue;
          SeenHashes.insert(Hash);
          {
            raw_comment_ostream CommentOut(*OS);
            CommentOut << HshStageToString(Stage) << " source targeting "
                       << HshTargetToString(Binaries.Target) << "\n\n";
            CommentOut << Source;
          }
          *OS << "inline ";
          if (Target != HT_VULKAN_SPIRV) {
            raw_carray_ostream DataOut(*OS, "_hshs_"s + llvm::utohexstr(Hash));
            DataOut.write((const char *)Data.data(), Data.size());
          } else {
            raw_carray32_ostream DataOut(*OS,
                                         "_hshs_"s + llvm::utohexstr(Hash));
            DataOut.write((const uint32_t *)Data.data(), Data.size() / 4);
          }
          *OS << "\ninline hsh::detail::ShaderObject<";
          Builtins.printTargetEnumString(*OS, HostPolicy, Target);
          *OS << "> _hsho_" << HashStr << ";\n\n";
        }

        AnonOS << "  },\n  {\n";

        for (const auto &Binding : Builder.getBindings()) {
          AnonOS << "    hsh::detail::VertexBinding{" << Binding.Binding << ", "
                 << Binding.Stride << ", ";
          Builtins.printInputRateEnumString(AnonOS, HostPolicy,
                                            Binding.InputRate);
          AnonOS << "},\n";
        }

        AnonOS << "  },\n  {\n";

        for (const auto &Attribute : Builder.getAttributes()) {
          AnonOS << "    hsh::detail::VertexAttribute{" << Attribute.Binding
                 << ", ";
          Builtins.printFormatEnumString(AnonOS, HostPolicy, Attribute.Format);
          AnonOS << ", " << Attribute.Offset << "},\n";
        }

        AnonOS << "  }\n};\n";

        AnonOS << "template <> hsh::detail::ShaderData<";
        Builtins.printTargetEnumString(AnonOS, HostPolicy, Target);
        AnonOS << ", " << Builder.getNumStages() << "> " << ExpName
               << "::data<";
        Builtins.printTargetEnumString(AnonOS, HostPolicy, Target);
        AnonOS << ">{\n  {\n";

        for (auto &[Data, Hash] : Binaries.Binaries) {
          if (Data.empty())
            continue;
          auto HashStr = llvm::utohexstr(Hash);
          AnonOS << "    _hsho_" << HashStr << ",\n";
        }

        AnonOS << "  }\n};\n";
      }

      // Emit define macro for capturing args
      AnonOS << "#define " << ExpName << " ::" << ExpName << "(";
      bool NeedsComma = false;
      for (const auto *Cap : Builder.captures()) {
        if (!NeedsComma)
          NeedsComma = true;
        else
          AnonOS << ", ";
        AnonOS << Cap->getIdentifier()->getName();
      }
      AnonOS << "); (void)\n\n";
    }
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    if (Diags.hasErrorOccurred())
      return;

    const unsigned IncludeDiagID =
        Diags.getCustomDiagID(DiagnosticsEngine::Error,
                              "hshhead include in must appear in global scope");
    if (!HeadInclude) {
      std::string ExpectedName =
          sys::path::filename(CI.getFrontendOpts().OutputFile);
      std::string Insertion = "#include \""s + ExpectedName + '\"';
      Diags.Report(IncludeDiagID) << FixItHint::CreateInsertion(
          Context.getSourceManager().getLocForStartOfFile(
              Context.getSourceManager().getMainFileID()),
          Insertion);
      return;
    }
    if (NamespaceDecl *NS = LocationNamespaceSearch(Context).findNamespace(
            HeadInclude->first)) {
      Diags.Report(HeadInclude->first, IncludeDiagID);
      Diags.Report(NS->getLocation(),
                   Diags.getCustomDiagID(DiagnosticsEngine::Note,
                                         "included in namespace"));
      return;
    }

    Builtins.findBuiltinDecls(Context);
    if (Context.getDiagnostics().hasErrorOccurred())
      return;

    OS = CI.createDefaultOutputFile(false);

    SourceManager &SM = Context.getSourceManager();
    StringRef MainName = SM.getFileEntryForID(SM.getMainFileID())->getName();
    *OS << "/* Auto-generated hshhead for " << MainName
        << " */\n"
           "#include <hsh.h>\n\n";

    AnonOS << "namespace {\n\n";

    /*
     * Find lambdas that are attributed with hsh::generator_lambda and exist
     * within the main file.
     */
    MatchFinder Finder;
    Finder.addMatcher(
        attributedStmt(
            stmt().bind("attrid"_ll),
            allOf(hasStmtAttr(attr::HshGeneratorLambda),
                  hasDescendant(lambdaExpr(stmt().bind("id"_ll),
                                           isExpansionInMainFile())))),
        this);
    Finder.matchAST(Context);

    AnonOS << "}\n";

    *OS << AnonOS.str();

    DxcLibrary::SharedInstance.reset();
  }

  StringRef
  getExpansionNameBeforeLambda(const AttributedStmt *LambdaAttr) const {
    for (auto *Attr : LambdaAttr->getAttrs()) {
      if (Attr->getKind() == attr::HshGeneratorLambda) {
        PresumedLoc PLoc =
            Context.getSourceManager().getPresumedLoc(Attr->getLoc());
        for (const auto &Exp : SeenHshExpansions) {
          PresumedLoc IPLoc =
              Context.getSourceManager().getPresumedLoc(Exp.first);
          if (IPLoc.getLine() == PLoc.getLine())
            return Exp.second.second;
        }
      }
    }
    return {};
  }

  void registerHshHeadInclude(SourceLocation HashLoc,
                              CharSourceRange FilenameRange,
                              StringRef RelativePath) {
    if (Context.getSourceManager().isWrittenInMainFile(HashLoc)) {
      DiagnosticsEngine &Diags = Context.getDiagnostics();
      if (HeadInclude) {
        Diags.Report(HashLoc, Diags.getCustomDiagID(
                                  DiagnosticsEngine::Error,
                                  "multiple hshhead includes in one file"));
        Diags.Report(HeadInclude->first,
                     Diags.getCustomDiagID(DiagnosticsEngine::Note,
                                           "previous include was here"));
        return;
      } else {
        std::string ExpectedName =
            sys::path::filename(CI.getFrontendOpts().OutputFile);
        if (ExpectedName != RelativePath) {
          std::string Replacement = "\""s + ExpectedName + '\"';
          Diags.Report(
              FilenameRange.getBegin(),
              Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                    "hshhead include must match the output "
                                    "filename"))
              << FixItHint::CreateReplacement(FilenameRange, Replacement);
          return;
        }
        HeadInclude.emplace(HashLoc, RelativePath);
      }
    }
  }

  void registerHshExpansion(SourceRange Range, StringRef Name) {
    if (Context.getSourceManager().isWrittenInMainFile(Range.getBegin())) {
      for (auto &Exps : SeenHshExpansions) {
        if (Exps.second.second == Name) {
          DiagnosticsEngine &Diags = Context.getDiagnostics();
          Diags.Report(
              Range.getBegin(),
              Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                    "hsh_* macro must be suffixed with "
                                    "identifier unique to the file"))
              << CharSourceRange(Range, false);
          Diags.Report(Exps.first, Diags.getCustomDiagID(
                                       DiagnosticsEngine::Note,
                                       "previous identifier usage is here"))
              << CharSourceRange(Exps.second.first, false);
          return;
        }
      }
      SeenHshExpansions[Range.getBegin()] = std::make_pair(Range, Name);
    }
  }

  class PPCallbacks : public clang::PPCallbacks {
    GenerateConsumer &Consumer;
    FileManager &FM;
    SourceManager &SM;
    static constexpr llvm::StringLiteral DummyInclude{"#include <hsh.h>\n"};

  public:
    explicit PPCallbacks(GenerateConsumer &Consumer, FileManager &FM,
                         SourceManager &SM)
        : Consumer(Consumer), FM(FM), SM(SM) {}
    bool FileNotFound(StringRef FileName,
                      SmallVectorImpl<char> &RecoveryPath) override {
      if (FileName.endswith_lower(".hshhead"_ll)) {
        SmallString<1024> VirtualFilePath("./"_ll);
        VirtualFilePath += FileName;
        FM.getVirtualFile(VirtualFilePath, DummyInclude.size(),
                          std::time(nullptr));
        RecoveryPath.push_back('.');
        return true;
      }
      return false;
    }
    void InclusionDirective(SourceLocation HashLoc, const Token &IncludeTok,
                            StringRef FileName, bool IsAngled,
                            CharSourceRange FilenameRange,
                            const FileEntry *File, StringRef SearchPath,
                            StringRef RelativePath,
                            const clang::Module *Imported,
                            SrcMgr::CharacteristicKind FileType) override {
      if (FileName.endswith_lower(".hshhead"_ll)) {
        assert(File && "File must exist at this point");
        SM.overrideFileContents(File,
                                llvm::MemoryBuffer::getMemBuffer(DummyInclude));
        Consumer.registerHshHeadInclude(HashLoc, FilenameRange, RelativePath);
      }
    }
    void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                      SourceRange Range, const MacroArgs *Args) override {
      if (MacroNameTok.is(tok::identifier)) {
        StringRef Name = MacroNameTok.getIdentifierInfo()->getName();
        if (Name.startswith("hsh_"_ll))
          Consumer.registerHshExpansion(Range, Name);
      }
    }
  };
};

} // namespace

namespace clang::hshgen {

std::unique_ptr<ASTConsumer>
GenerateAction::CreateASTConsumer(CompilerInstance &CI, StringRef InFile) {
  dumper().setPrintingPolicy(CI.getASTContext().getPrintingPolicy());
  auto Consumer = std::make_unique<GenerateConsumer>(CI, ProgramDir, Targets);
  CI.getPreprocessor().addPPCallbacks(
      std::make_unique<GenerateConsumer::PPCallbacks>(
          *Consumer, CI.getFileManager(), CI.getSourceManager()));
  return Consumer;
}

} // namespace clang::hshgen
