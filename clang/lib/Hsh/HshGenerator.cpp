//===--- HshGenerator.cpp - Lambda scanner and codegen for hsh tool -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Value.h"
#include "llvm/Support/SaveAndRestore.h"
#include "llvm/Support/raw_carray_ostream.h"
#include "llvm/Support/raw_comment_ostream.h"

#include "clang/AST/ASTDumper.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/AST/QualTypeNames.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Hsh/HshGenerator.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Lex/PreprocessorOptions.h"

namespace clang::hshgen {

using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;
using namespace std::literals;

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

static StringRef HshStageToString(HshStage Stage) {
  switch (Stage) {
  case HshHostStage:
    return llvm::StringLiteral("host");
  case HshVertexStage:
    return llvm::StringLiteral("vertex");
  case HshControlStage:
    return llvm::StringLiteral("control");
  case HshEvaluationStage:
    return llvm::StringLiteral("evaluation");
  case HshGeometryStage:
    return llvm::StringLiteral("geometry");
  case HshFragmentStage:
    return llvm::StringLiteral("fragment");
  default:
    return llvm::StringLiteral("none");
  }
}

enum HshBuiltinType {
  HBT_None,
#define BUILTIN_TYPE(Name, GLSL, HLSL, Metal, Vector, Matrix) HBT_##Name,
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
#define BUILTIN_CXX_METHOD(Name, Record, ...) HBM_##Name##_##Record,
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
  CXXRecordDecl *ShaderDataRecordType = nullptr;
  CXXRecordDecl *GlobalListNodeRecordType = nullptr;
  std::array<const TagDecl *, HBT_Max> Types{};
  std::array<const FunctionDecl *, HBF_Max> Functions{};
  std::array<const CXXMethodDecl *, HBM_Max> Methods{};

  static constexpr Spellings BuiltinTypeSpellings[] = {
      {{}, {}, {}},
#define BUILTIN_TYPE(Name, GLSL, HLSL, Metal, Vector, Matrix)                  \
  {llvm::StringLiteral(#GLSL), llvm::StringLiteral(#HLSL),                     \
   llvm::StringLiteral(#Metal)},
#include "BuiltinTypes.def"
  };

  static constexpr Spellings BuiltinFunctionSpellings[] = {
      {{}, {}, {}},
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist)                  \
  {llvm::StringLiteral(#GLSL), llvm::StringLiteral(#HLSL),                     \
   llvm::StringLiteral(#Metal)},
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

  public:
    bool VisitDecl(Decl *D) {
      if (auto *DC = dyn_cast<DeclContext>(D))
        for (Decl *Child : DC->decls())
          if (!base::Visit(Child))
            return false;
      return true;
    }

    bool VisitNamespaceDecl(NamespaceDecl *Namespace) {
      if (InHshNS)
        return true;
      bool Ret = true;
      if (Namespace->getDeclName().isIdentifier() &&
          Namespace->getName() == llvm::StringLiteral("hsh")) {
        SaveAndRestore<bool> SavedInHshNS(InHshNS, true);
        Ret = VisitDecl(Namespace);
      }
      return Ret;
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
      if (InHshNS && Type->getDeclName().isIdentifier() &&
          Type->getName() == Name) {
        Found = Type;
        return false;
      }
      return true;
    }
  };

  class FuncFinder : public DeclFinder<FuncFinder> {
  public:
    bool VisitFunctionDecl(FunctionDecl *Func) {
      if (InHshNS && Func->getDeclName().isIdentifier() &&
          Func->getName() == Name) {
        Found = Func;
        return false;
      }
      return true;
    }
  };

  class ClassTemplateFinder : public DeclFinder<ClassTemplateFinder> {
  public:
    bool VisitClassTemplateDecl(ClassTemplateDecl *Type) {
      if (InHshNS && Type->getDeclName().isIdentifier() &&
          Type->getName() == Name) {
        Found = Type;
        return false;
      }
      return true;
    }
  };

  class MethodFinder : public DeclFinder<MethodFinder> {
    StringRef Record;
    SmallVector<StringRef, 8> Params;

  public:
    bool VisitClassTemplateDecl(ClassTemplateDecl *ClassTemplate) {
      return VisitDecl(ClassTemplate->getTemplatedDecl());
    }

    bool VisitCXXMethodDecl(CXXMethodDecl *Method) {
      if (InHshNS && Method->getDeclName().isIdentifier() &&
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
      P.split(Params, ',');
      for (auto &ParamStr : Params)
        ParamStr = ParamStr.trim();
      Found = nullptr;
      Visit(TU);
      return Found;
    }
  };

  void addType(SourceManager &SM, HshBuiltinType TypeKind, StringRef Name,
               Decl *D) {
    if (auto *T = dyn_cast_or_null<TagDecl>(D)) {
      Types[TypeKind] = T->getFirstDecl();
    } else {
      DiagnosticsEngine &Diags = SM.getDiagnostics();
      unsigned DiagID = Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                    "type %0; is hsh.h included?");
      Diags.Report(DiagID) << Name;
    }
  }

  void addFunction(SourceManager &SM, HshBuiltinFunction FuncKind,
                   StringRef Name, Decl *D) {
    if (auto *F = dyn_cast_or_null<FunctionDecl>(D)) {
      Functions[FuncKind] = F->getFirstDecl();
    } else {
      DiagnosticsEngine &Diags = SM.getDiagnostics();
      unsigned DiagID = Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                    "function %0; is hsh.h included?");
      Diags.Report(DiagID) << Name;
    }
  }

  void addCXXMethod(SourceManager &SM, HshBuiltinCXXMethod MethodKind,
                    StringRef Name, Decl *D) {
    if (auto *M = dyn_cast_or_null<CXXMethodDecl>(D)) {
      Methods[MethodKind] = dyn_cast<CXXMethodDecl>(M->getFirstDecl());
    } else {
      DiagnosticsEngine &Diags = SM.getDiagnostics();
      unsigned DiagID = Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                    "method %0; is hsh.h included?");
      Diags.Report(DiagID) << Name;
    }
  }

public:
  void findBuiltinDecls(ASTContext &Context) {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    TranslationUnitDecl *TU = Context.getTranslationUnitDecl();
    SourceManager &SM = Context.getSourceManager();
    if (auto *T = dyn_cast_or_null<ClassTemplateDecl>(
            ClassTemplateFinder().Find(llvm::StringLiteral("_HshBase"), TU))) {
      BaseRecordType = cast<ClassTemplateDecl>(T->getFirstDecl());
      auto *TemplDecl = BaseRecordType->getTemplatedDecl();
      using FuncTemplIt =
          CXXRecordDecl::specific_decl_iterator<FunctionTemplateDecl>;
      for (FuncTemplIt TI(TemplDecl->decls_begin()), TE(TemplDecl->decls_end());
           TI != TE; ++TI) {
        if (TI->getName() == llvm::StringLiteral("push_uniform"))
          PushUniformMethod = *TI;
      }
      if (!PushUniformMethod) {
        unsigned DiagID = Diags.getCustomDiagID(
            DiagnosticsEngine::Error, "unable to locate declaration of "
                                      "_HshBase::push_uniform; "
                                      "is hsh.h included?");
        Diags.Report(DiagID);
      }
    } else {
      unsigned DiagID = Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "unable to locate declaration of _HshBase; "
                                    "is hsh.h included?");
      Diags.Report(DiagID);
    }

    if (auto *E = dyn_cast_or_null<EnumDecl>(
            TypeFinder().Find(llvm::StringLiteral("Target"), TU))) {
      EnumTarget = E;
    } else {
      unsigned DiagID =
          Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                "unable to locate declaration of enum Target; "
                                "is hsh.h included?");
      Diags.Report(DiagID);
    }

    if (auto *E = dyn_cast_or_null<EnumDecl>(
            TypeFinder().Find(llvm::StringLiteral("Stage"), TU))) {
      EnumStage = E;
    } else {
      unsigned DiagID =
          Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                "unable to locate declaration of enum Stage; "
                                "is hsh.h included?");
      Diags.Report(DiagID);
    }

    if (auto *R = dyn_cast_or_null<CXXRecordDecl>(
            TypeFinder().Find(llvm::StringLiteral("_HshShaderData"), TU))) {
      ShaderDataRecordType = R;
    } else {
      unsigned DiagID = Diags.getCustomDiagID(
          DiagnosticsEngine::Error,
          "unable to locate declaration of _HshShaderData; "
          "is hsh.h included?");
      Diags.Report(DiagID);
    }

    if (auto *R = dyn_cast_or_null<CXXRecordDecl>(
            TypeFinder().Find(llvm::StringLiteral("_HshGlobalListNode"), TU))) {
      GlobalListNodeRecordType = R;
    } else {
      unsigned DiagID = Diags.getCustomDiagID(
          DiagnosticsEngine::Error,
          "unable to locate declaration of _HshGlobalListNode; "
          "is hsh.h included?");
      Diags.Report(DiagID);
    }

#define BUILTIN_TYPE(Name, GLSL, HLSL, Metal, Vector, Matrix)                  \
  addType(SM, HBT_##Name, llvm::StringLiteral(#Name),                          \
          TypeFinder().Find(llvm::StringLiteral(#Name), TU));
#include "BuiltinTypes.def"
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist)                  \
  addFunction(SM, HBF_##Name, llvm::StringLiteral(#Name),                      \
              FuncFinder().Find(llvm::StringLiteral(#Name), TU));
#include "BuiltinFunctions.def"
#define BUILTIN_CXX_METHOD(Name, Record, ...)                                  \
  addCXXMethod(SM, HBM_##Name##_##Record,                                      \
               llvm::StringLiteral(#Record "::" #Name "(" #__VA_ARGS__ ")"),   \
               MethodFinder().Find(llvm::StringLiteral(#Name),                 \
                                   llvm::StringLiteral(#Record),               \
                                   llvm::StringLiteral(#__VA_ARGS__), TU));
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
    std::array<TemplateArgument, 1> TemplateArgs = {TemplateArgument{
        Context, APSInt(APInt(32, int(Stage) - 1)), NTTP->getType()}};
    auto *PushUniform =
        cast<CXXMethodDecl>(PushUniformMethod->getTemplatedDecl());
    TemplateArgumentListInfo CallTemplArgs(PushUniformMethod->getLocation(),
                                           {});
    CallTemplArgs.addArgument(
        TemplateArgumentLoc(TemplateArgs[0], (Expr *)nullptr));
    auto *ThisExpr = new (Context) CXXThisExpr({}, Context.VoidTy, true);
    auto *ME = MemberExpr::Create(
        Context, ThisExpr, true, {}, {}, {}, PushUniform,
        DeclAccessPair::make(PushUniform, PushUniform->getAccess()), {},
        &CallTemplArgs, Context.VoidTy, VK_XValue, OK_Ordinary, NOUR_None);
    std::array<Expr *, 1> Args{DeclRefExpr::Create(Context, {}, {}, Decl, false,
                                                   SourceLocation{},
                                                   Decl->getType(), VK_XValue)};
    return CXXMemberCallExpr::Create(Context, ME, Args, Context.VoidTy,
                                     VK_XValue, {});
  }

  VarTemplateDecl *getDataVarTemplate(ASTContext &Context,
                                      DeclContext *DC) const {
    std::array<NamedDecl *, 2> Parms{
        NonTypeTemplateParmDecl::Create(
            Context, DC, {}, {}, 0, 0, &Context.Idents.get("T"),
            QualType{EnumTarget->getTypeForDecl(), 0}, false, nullptr),
        NonTypeTemplateParmDecl::Create(
            Context, DC, {}, {}, 0, 0, &Context.Idents.get("S"),
            QualType{EnumStage->getTypeForDecl(), 0}, false, nullptr)};
    auto *TPL =
        TemplateParameterList::Create(Context, {}, {}, Parms, {}, nullptr);
    auto *VD =
        VarDecl::Create(Context, DC, {}, {}, &Context.Idents.get("data"),
                        QualType{ShaderDataRecordType->getTypeForDecl(), 0},
                        nullptr, SC_Static);
    VD->setConstexpr(true);
    return VarTemplateDecl::Create(Context, DC, {}, VD->getIdentifier(), TPL,
                                   VD);
  }

  VarDecl *getGlobalListNode(ASTContext &Context, DeclContext *DC) const {
    return VarDecl::Create(
        Context, DC, {}, {}, &Context.Idents.get("global"),
        QualType{GlobalListNodeRecordType->getTypeForDecl(), 0}, nullptr,
        SC_Static);
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

static HshStage DetermineParmVarStage(ParmVarDecl *D) {
#define INTERFACE_VARIABLE(Attr, Stage, Direction, Array)                      \
  if (D->hasAttr<Attr>())                                                      \
    return Stage;
#include "ShaderInterface.def"
  return HshHostStage;
}

static HshInterfaceDirection DetermineParmVarDirection(ParmVarDecl *D) {
#define INTERFACE_VARIABLE(Attr, Stage, Direction, Array)                      \
  if (D->hasAttr<Attr>())                                                      \
    return Direction;
#include "ShaderInterface.def"
  return HshInput;
}

template <typename T, unsigned N>
static DiagnosticBuilder
ReportCustom(T *S, const ASTContext &Context, const char (&FormatString)[N],
             DiagnosticsEngine::Level level = DiagnosticsEngine::Error) {
  DiagnosticsEngine &Diags = Context.getDiagnostics();
  unsigned DiagID = Diags.getCustomDiagID(level, FormatString);
  return Diags.Report(S->getBeginLoc(), DiagID)
         << CharSourceRange(S->getSourceRange(), false);
}

static void ReportUnsupportedStmt(Stmt *S, const ASTContext &Context) {
  auto Diag = ReportCustom(
      S, Context,
      "statements of type %0 are not supported in hsh generator lambdas");
  Diag.AddString(S->getStmtClassName());
}

static void ReportUnsupportedFunctionCall(Stmt *S, const ASTContext &Context) {
  ReportCustom(S, Context, "function calls are limited to hsh intrinsics");
}

static void ReportUnsupportedTypeReference(Stmt *S, const ASTContext &Context) {
  ReportCustom(S, Context, "references to values are limited to hsh types");
}

static void ReportUnsupportedTypeConstruct(Stmt *S, const ASTContext &Context) {
  ReportCustom(S, Context, "constructors are limited to hsh types");
}

static void ReportUnsupportedTypeCast(Stmt *S, const ASTContext &Context) {
  ReportCustom(S, Context, "type casts are limited to hsh types");
}

static void ReportBadTextureReference(Stmt *S, const ASTContext &Context) {
  ReportCustom(S, Context,
               "texture samples must be performed on lambda parameters");
}

static void ReportUnattributedTexture(ParmVarDecl *PVD,
                                      const ASTContext &Context) {
  ReportCustom(
      PVD, Context,
      "sampled textures must be attributed with [[hsh::*_texture(n)]]");
}

static void ReportNonConstexprSampler(Expr *E, const ASTContext &Context) {
  ReportCustom(E, Context, "sampler arguments must be constexpr");
}

static void ReportBadSamplerStructFormat(Expr *E, const ASTContext &Context) {
  ReportCustom(E, Context, "sampler structure is not consistent");
}

static void ReportBadVertexPositionType(ParmVarDecl *PVD,
                                        const ASTContext &Context) {
  ReportCustom(PVD, Context, "vertex position must be a hsh::float4");
}

static void ReportBadColorTargetType(ParmVarDecl *PVD,
                                     const ASTContext &Context) {
  ReportCustom(PVD, Context, "fragment color target must be a hsh::float4");
}

class LastAssignmentFinder : public StmtVisitor<LastAssignmentFinder, bool> {
  const ASTContext &Context;
  VarDecl *Var = nullptr;
  Stmt *End = nullptr;
  Stmt *Assign = nullptr;
  Stmt *LastAssign = nullptr;
  Stmt *CompoundChild = nullptr;
  Stmt *LastCompoundChild = nullptr;
  bool DoVisit(Stmt *S) {
    if (End && S == End)
      return false;
    if (auto *E = dyn_cast<Expr>(S))
      return Visit(E->IgnoreParenCasts());
    else
      return Visit(S);
  }
  void UpdateLastAssign(Stmt *S) {
    LastAssign = S;
    LastCompoundChild = CompoundChild;
  }

public:
  explicit LastAssignmentFinder(const ASTContext &Context) : Context(Context) {}

  bool VisitStmt(Stmt *S) {
    ReportUnsupportedStmt(S, Context);
    return true;
  }

  bool VisitCompoundStmt(CompoundStmt *CompoundStmt) {
    for (Stmt *ChildStmt : CompoundStmt->body()) {
      CompoundChild = ChildStmt;
      if (!DoVisit(ChildStmt))
        return false;
    }
    return true;
  }

  bool VisitDeclStmt(DeclStmt *DeclStmt) {
    for (Decl *D : DeclStmt->getDeclGroup()) {
      if (auto *VD = dyn_cast<VarDecl>(D)) {
        if (Expr *Init = VD->getInit()) {
          if (VD == Var)
            UpdateLastAssign(DeclStmt);
          else if (!DoVisit(Init))
            return false;
        }
      }
    }
    return true;
  }

  static bool VisitNullStmt(NullStmt *) { return true; }

  bool VisitValueStmt(ValueStmt *ValueStmt) {
    return DoVisit(ValueStmt->getExprStmt());
  }

  bool VisitBinaryOperator(BinaryOperator *BinOp) {
    if (BinOp->isAssignmentOp()) {
      {
        SaveAndRestore<Stmt *> SavedAssign(Assign, BinOp);
        if (!DoVisit(BinOp->getLHS()))
          return false;
      }
      if (!DoVisit(BinOp->getRHS()))
        return false;
    } else {
      if (!DoVisit(BinOp->getLHS()))
        return false;
      if (!DoVisit(BinOp->getRHS()))
        return false;
    }
    return true;
  }

  bool VisitUnaryOperator(UnaryOperator *UnOp) {
    return DoVisit(UnOp->getSubExpr());
  }

  bool VisitExpr(Expr *E) {
    ReportUnsupportedStmt(E, Context);
    return true;
  }

  bool VisitBlockExpr(BlockExpr *Block) { return DoVisit(Block->getBody()); }

  bool VisitCallExpr(CallExpr *CallExpr) {
    for (Expr *Arg : CallExpr->arguments()) {
      if (!DoVisit(Arg))
        return false;
    }
    return true;
  }

  bool VisitCXXConstructExpr(CXXConstructExpr *ConstructExpr) {
    for (Expr *Arg : ConstructExpr->arguments()) {
      if (!DoVisit(Arg))
        return false;
    }
    return true;
  }

  bool VisitCXXOperatorCallExpr(CXXOperatorCallExpr *CallExpr) {
    if (CallExpr->getNumArgs() >= 1 && CallExpr->isAssignmentOp()) {
      {
        SaveAndRestore<Stmt *> SavedAssign(Assign, CallExpr);
        if (!DoVisit(CallExpr->getArg(0)))
          return false;
      }
      if (!DoVisit(CallExpr->getArg(1)))
        return false;
    } else {
      if (!VisitCallExpr(CallExpr))
        return false;
    }
    return true;
  }

  bool VisitDeclRefExpr(DeclRefExpr *DeclRef) {
    if (Assign && DeclRef->getDecl() == Var)
      UpdateLastAssign(Assign);
    return true;
  }

  bool VisitInitListExpr(InitListExpr *InitList) {
    for (Stmt *S : *InitList) {
      if (!DoVisit(S))
        return false;
    }
    return true;
  }

  bool VisitMemberExpr(MemberExpr *MemberExpr) {
    return DoVisit(MemberExpr->getBase());
  }

  static bool VisitFloatingLiteral(FloatingLiteral *) { return true; }

  static bool VisitIntegerLiteral(IntegerLiteral *) { return true; }

  std::tuple<Stmt *, Stmt *> Find(VarDecl *V, Stmt *Body, Stmt *E = nullptr) {
    Var = V;
    End = E;
    DoVisit(Body);
    return {LastAssign, LastCompoundChild};
  }
};

struct AssignmentFinderInfo {
  Stmt *Body = nullptr;
  Stmt *LastCompoundChild = nullptr;
  VarDecl *SelectedVarDecl = nullptr;
};

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
  static constexpr bool ValidateSamplerStruct(const APValue &Val) {
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

class StagesBuilder
    : public StmtVisitor<StagesBuilder, Expr *, HshStage, HshStage> {
  ASTContext &Context;
  unsigned UseStages;

  static IdentifierInfo &getToIdent(ASTContext &Context, HshStage Stage) {
    std::string VarName;
    raw_string_ostream VNS(VarName);
    VNS << "to_" << HshStageToString(Stage);
    return Context.Idents.get(VNS.str());
  }

  static IdentifierInfo &getFromIdent(ASTContext &Context, HshStage Stage) {
    std::string VarName;
    raw_string_ostream VNS(VarName);
    VNS << "from_" << HshStageToString(Stage);
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
                                     bool IgnoreExisting = false) {
      FieldDecl *Field = getFieldForExpr(Context, E, IgnoreExisting);
      if (!Field)
        return nullptr;
      return MemberExpr::CreateImplicit(
          Context,
          DeclRefExpr::Create(Context, {}, {}, VD, false, SourceLocation{},
                              E->getType(), VK_XValue),
          false, Field, Field->getType(), VK_XValue, OK_Ordinary);
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
                               bool IgnoreExisting = false) {
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
      return createFieldReference(Context, E, Consumer);
    }

    void finalizeRecord() { Record->completeDefinition(); }

    CXXRecordDecl *getRecord() const { return Record; }
  };

  std::array<InterfaceRecord, HshMaxStage>
      HostToStageRecords; /* Indexed by consumer stage */
  std::array<InterfaceRecord, HshMaxStage>
      InterStageRecords; /* Indexed by consumer stage */
  struct StageStmtList {
    SmallVector<Stmt *, 16> Stmts;
    SmallVector<std::pair<unsigned, VarDecl *>, 16> StmtDeclRefCount;
  };
  std::array<StageStmtList, HshMaxStage> StageStmts;
  struct SampleCall {
    CXXMemberCallExpr *Expr;
    unsigned Index;
    SamplerConfig Config;
  };
  std::array<SmallVector<SampleCall, 4>, HshMaxStage> SampleCalls;
  SmallVector<ParmVarDecl *, 4> UsedCaptures;

  AssignmentFinderInfo AssignFindInfo;

  template <typename T>
  SmallVector<Expr *, 4> DoVisitExprRange(T Range, HshStage From, HshStage To) {
    SmallVector<Expr *, 4> Res;
    for (Expr *E : Range)
      Res.push_back(Visit(E, From, To));
    return Res;
  }

public:
  StagesBuilder(ASTContext &Context, DeclContext *SpecDeclContext,
                unsigned UseStages)
      : Context(Context), UseStages(UseStages) {
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

  Expr *VisitStmt(Stmt *S, HshStage From, HshStage To) {
    llvm_unreachable("Unhandled statements should have been pruned already");
    return nullptr;
  }

  /* Begin ignores */
  Expr *VisitBlockExpr(BlockExpr *Block, HshStage From, HshStage To) {
    return Visit(Block->getBody(), From, To);
  }

  Expr *VisitValueStmt(ValueStmt *ValueStmt, HshStage From, HshStage To) {
    return Visit(ValueStmt->getExprStmt(), From, To);
  }

  Expr *VisitUnaryOperator(UnaryOperator *UnOp, HshStage From, HshStage To) {
    return Visit(UnOp->getSubExpr(), From, To);
  }

  Expr *VisitGenericSelectionExpr(GenericSelectionExpr *GSE, HshStage From,
                                  HshStage To) {
    return Visit(GSE->getResultExpr(), From, To);
  }

  Expr *VisitChooseExpr(ChooseExpr *CE, HshStage From, HshStage To) {
    return Visit(CE->getChosenSubExpr(), From, To);
  }

  Expr *VisitConstantExpr(ConstantExpr *CE, HshStage From, HshStage To) {
    return Visit(CE->getSubExpr(), From, To);
  }

  Expr *VisitImplicitCastExpr(ImplicitCastExpr *ICE, HshStage From,
                              HshStage To) {
    return Visit(ICE->getSubExpr(), From, To);
  }

  Expr *VisitFullExpr(FullExpr *FE, HshStage From, HshStage To) {
    return Visit(FE->getSubExpr(), From, To);
  }

  Expr *VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *MTE,
                                      HshStage From, HshStage To) {
    return Visit(MTE->getSubExpr(), From, To);
  }

  Expr *VisitSubstNonTypeTemplateParmExpr(SubstNonTypeTemplateParmExpr *NTTP,
                                          HshStage From, HshStage To) {
    return Visit(NTTP->getReplacement(), From, To);
  }
  /* End ignores */

  /*
   * Base case for createInterStageReferenceExpr.
   * Stage division will be established on this expression.
   */
  Expr *VisitExpr(Expr *E, HshStage From, HshStage To) {
    if (From == To || From == HshNoStage || To == HshNoStage)
      return E;
    if (From != HshHostStage) {
      /* Create intermediate inter-stage assignments */
      for (int D = From + 1, S = From; D <= To; ++D) {
        if (UseStages & (1u << unsigned(D))) {
          InterfaceRecord &SRecord = InterStageRecords[S];
          InterfaceRecord &DRecord = InterStageRecords[D];
          if (MemberExpr *Producer =
                  DRecord.createProducerFieldReference(Context, E))
            addStageStmt(
                new (Context) BinaryOperator(
                    Producer,
                    S == From
                        ? E
                        : SRecord.createConsumerFieldReference(Context, E),
                    BO_Assign, E->getType(), VK_XValue, OK_Ordinary, {}, {}),
                HshStage(S));
          S = D;
        }
      }
    } else {
      if (MemberExpr *Producer =
              HostToStageRecords[To].createProducerFieldReference(Context, E))
        addStageStmt(new (Context)
                         BinaryOperator(Producer, E, BO_Assign, E->getType(),
                                        VK_XValue, OK_Ordinary, {}, {}),
                     From);
    }
    InterfaceRecord &Record =
        From == HshHostStage ? HostToStageRecords[To] : InterStageRecords[To];
    return Record.createConsumerFieldReference(Context, E);
  }

  /*
   * Construction expressions are a form of component-wise type conversions in
   * hsh. They may be lifted to the target stage.
   */
  Expr *VisitCXXConstructExpr(CXXConstructExpr *ConstructExpr, HshStage From,
                              HshStage To) {
    auto Arguments = DoVisitExprRange(ConstructExpr->arguments(), From, To);
    CXXConstructExpr *NCE = CXXTemporaryObjectExpr::Create(
        Context, ConstructExpr->getConstructor(), ConstructExpr->getType(),
        Context.getTrivialTypeSourceInfo(ConstructExpr->getType()), Arguments,
        {}, ConstructExpr->hadMultipleCandidates(),
        ConstructExpr->isListInitialization(),
        ConstructExpr->isStdInitListInitialization(),
        ConstructExpr->requiresZeroInitialization());
    return NCE;
  }

  /*
   * DeclRef expressions may connect directly to a construction expression and
   * should therefore be lifted to the target stage.
   */
  Expr *VisitDeclRefExpr(DeclRefExpr *DeclRef, HshStage From, HshStage To) {
    if (auto *VD = dyn_cast<VarDecl>(DeclRef->getDecl())) {
      if (auto *PVD = dyn_cast<ParmVarDecl>(DeclRef->getDecl()))
        return VisitExpr(DeclRef, From, To);
      auto [Assign, NextCompoundChild] = LastAssignmentFinder(Context).Find(
          VD, AssignFindInfo.Body, AssignFindInfo.LastCompoundChild);
      if (Assign) {
        SaveAndRestore<Stmt *> SavedCompoundChild(
            AssignFindInfo.LastCompoundChild, NextCompoundChild);
        SaveAndRestore<VarDecl *> SavedSelectedVarDecl(
            AssignFindInfo.SelectedVarDecl, VD);
        return Visit(Assign, From, To);
      }
    }
    llvm_unreachable("Should have been handled already");
    return nullptr;
  }

  Expr *VisitDeclStmt(DeclStmt *DeclStmt, HshStage From, HshStage To) {
    for (Decl *D : DeclStmt->getDeclGroup()) {
      if (auto *VD = dyn_cast<VarDecl>(D)) {
        if (VD == AssignFindInfo.SelectedVarDecl) {
          auto *NVD = VarDecl::Create(
              Context, VD->getDeclContext(), {}, {}, VD->getIdentifier(),
              VD->getType().getUnqualifiedType(), nullptr, SC_None);
          auto *NDS = new (Context) class DeclStmt(DeclGroupRef(NVD), {}, {});
          if (Expr *Init = VD->getInit())
            NVD->setInit(Visit(Init, From, To));
          liftDeclStmt(NDS, From, To, VD);
          return DeclRefExpr::Create(
              Context, {}, {}, NVD, true, SourceLocation{},
              VD->getType().getNonReferenceType(), VK_RValue);
        }
      }
    }
    llvm_unreachable("Should have been handled already");
    return nullptr;
  }

  /*
   * Certain trivial expressions like type conversions may be lifted into
   * the target stage rather than creating redundant inter-stage data.
   */
  Expr *createInterStageReferenceExpr(Expr *E, HshStage From, HshStage To,
                                      const AssignmentFinderInfo &AFI) {
    AssignFindInfo = AFI;
    return Visit(E, From, To);
  }

  void addStageStmt(Stmt *S, HshStage Stage, VarDecl *OrigDecl = nullptr) {
    if (auto *DS = dyn_cast<DeclStmt>(S)) {
      auto RefCountIt = StageStmts[Stage].StmtDeclRefCount.begin();
      for (auto I = StageStmts[Stage].Stmts.begin(),
                E = StageStmts[Stage].Stmts.end();
           I != E; ++I, ++RefCountIt) {
        if (isa<DeclStmt>(*I)) {
          if (RefCountIt->second == OrigDecl) {
            ++RefCountIt->first;
            return;
          }
        }
      }
    } else {
      for (Stmt *ES : StageStmts[Stage].Stmts)
        if (ES == S)
          return;
    }
    StageStmts[Stage].Stmts.push_back(S);
    StageStmts[Stage].StmtDeclRefCount.push_back({1, OrigDecl});
  }

  void liftDeclStmt(DeclStmt *DS, HshStage From, HshStage To,
                    VarDecl *OrigDecl) {
    addStageStmt(DS, To, OrigDecl);
    auto RefCountIt = StageStmts[From].StmtDeclRefCount.begin();
    for (auto I = StageStmts[From].Stmts.begin(),
              E = StageStmts[From].Stmts.end();
         I != E; ++I, ++RefCountIt) {
      if (isa<DeclStmt>(*I)) {
        if (RefCountIt->second == OrigDecl) {
          if (--RefCountIt->first == 0) {
            StageStmts[From].Stmts.erase(I);
            StageStmts[From].StmtDeclRefCount.erase(RefCountIt);
          }
          break;
        }
      }
    }
  }

  template <typename TexAttr>
  std::pair<HshStage, APSInt> getTextureIndex(TexAttr *A) {
    Expr::EvalResult Res;
    A->getIndex()->EvaluateAsInt(Res, Context);
    return {StageOfTextureAttr<TexAttr>(), Res.Val.getInt()};
  }

  template <typename TexAttr>
  std::pair<HshStage, APSInt> getTextureIndex(ParmVarDecl *PVD) {
    if (auto *A = PVD->getAttr<TexAttr>())
      return getTextureIndex(A);
    return {HshNoStage, APSInt{}};
  }

  template <typename TexAttrA, typename TexAttrB, typename... Rest>
  std::pair<HshStage, APSInt> getTextureIndex(ParmVarDecl *PVD) {
    if (auto *A = PVD->getAttr<TexAttrA>())
      return getTextureIndex(A);
    return getTextureIndex<TexAttrB, Rest...>(PVD);
  }

  std::pair<HshStage, APSInt> getTextureIndex(ParmVarDecl *PVD) {
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
        StageCalls.push_back(
            {C, unsigned(TexIdx.getZExtValue()), SamplerConfig{Res}});
      }
    }
  }

  void registerUsedCapture(ParmVarDecl *PVD) {
    for (auto *EC : UsedCaptures)
      if (EC == PVD)
        return;
    UsedCaptures.push_back(PVD);
  }

  auto captures() const {
    return llvm::iterator_range(UsedCaptures.begin(), UsedCaptures.end());
  }

  ArrayRef<Stmt *> hostStatements() const {
    return StageStmts[HshHostStage].Stmts;
  }

  void finalizeResults(ASTContext &Context, HshBuiltins &Builtins,
                       CXXRecordDecl *SpecRecord) {
    for (int D = HshVertexStage; D < HshMaxStage; ++D) {
      if (UseStages & (1u << unsigned(D))) {
        auto &RecDecl = HostToStageRecords[D];
        RecDecl.finalizeRecord();
        SpecRecord->addDecl(RecDecl.getRecord());
      }
    }

    for (int D = HshControlStage; D < HshMaxStage; ++D) {
      if (UseStages & (1u << unsigned(D)))
        InterStageRecords[D].finalizeRecord();
    }

    std::array<VarDecl *, HshMaxStage> HostToStageVars{};
    SmallVector<Stmt *, 16> NewHostStmts;
    NewHostStmts.reserve(StageStmts[HshHostStage].Stmts.size() + 10);
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
    NewHostStmts.insert(NewHostStmts.end(),
                        StageStmts[HshHostStage].Stmts.begin(),
                        StageStmts[HshHostStage].Stmts.end());
    for (int S = HshVertexStage; S < HshMaxStage; ++S) {
      if (auto *VD = HostToStageVars[S])
        NewHostStmts.push_back(
            Builtins.getPushUniformCall(Context, VD, HshStage(S)));
    }
    StageStmts[HshHostStage].Stmts = std::move(NewHostStmts);
  }

  void printResults(const PrintingPolicy &Policy, raw_ostream &OS) {
    for (int D = HshVertexStage; D < HshMaxStage; ++D) {
      if (UseStages & (1u << unsigned(D))) {
        HostToStageRecords[D].getRecord()->print(llvm::outs(), Policy);
        llvm::outs() << '\n';
      }
    }

    for (int D = HshControlStage; D < HshMaxStage; ++D) {
      if (UseStages & (1u << unsigned(D))) {
        InterStageRecords[D].getRecord()->print(llvm::outs(), Policy);
        llvm::outs() << '\n';
      }
    }

    for (int S = HshHostStage; S < HshMaxStage; ++S) {
      if (UseStages & (1u << unsigned(S))) {
        llvm::outs() << HshStageToString(HshStage(S)) << " statements:\n";
        auto *Stmts =
            CompoundStmt::Create(Context, StageStmts[S].Stmts, {}, {});
        Stmts->printPretty(llvm::outs(), nullptr, Policy);
      }
    }
  }
};

using StmtResult = std::pair<Stmt *, HshStage>;
class ValueTracer : public StmtVisitor<ValueTracer, StmtResult> {
  ASTContext &Context;
  const HshBuiltins &Builtins;
  StagesBuilder &Builder;
  AssignmentFinderInfo AssignFindInfo;
  HshStage Target = HshNoStage;
  bool InMemberExpr = false;

  static constexpr StmtResult ErrorResult{nullptr, HshNoStage};

  bool GetInterpolated(HshStage Stage) const {
    return Stage != HshHostStage && Stage < Target;
  }

  struct VisitExprRangeResult {
    HshStage Stage = HshNoStage;
    SmallVector<Expr *, 4> Exprs;
    SmallVector<HshStage, 4> ExprStages;
    operator ArrayRef<Expr *>() { return Exprs; }
  };

  template <typename T>
  Optional<VisitExprRangeResult> DoVisitExprRange(T Range, Stmt *Parent) {
    VisitExprRangeResult Res;
    for (Expr *E : Range) {
      auto [ExprStmt, ExprStage] = Visit(E);
      if (!ExprStmt)
        return {};
      Res.Exprs.push_back(cast<Expr>(ExprStmt));
      Res.ExprStages.emplace_back(ExprStage);
      Res.Stage = std::max(Res.Stage, ExprStage);
    }
    return {Res};
  }

  void DoPromoteExprRange(VisitExprRangeResult &Res) {
    auto ExprStageI = Res.ExprStages.begin();
    for (Expr *&E : Res.Exprs) {
      E = Builder.createInterStageReferenceExpr(E, *ExprStageI, Res.Stage,
                                                AssignFindInfo);
      ++ExprStageI;
    }
  }

public:
  explicit ValueTracer(ASTContext &Context, const HshBuiltins &Builtins,
                       StagesBuilder &Promotions)
      : Context(Context), Builtins(Builtins), Builder(Promotions) {}

  /* Begin ignores */
  StmtResult VisitBlockExpr(BlockExpr *Block) {
    return Visit(Block->getBody());
  }

  StmtResult VisitValueStmt(ValueStmt *ValueStmt) {
    return Visit(ValueStmt->getExprStmt());
  }

  StmtResult VisitUnaryOperator(UnaryOperator *UnOp) {
    return Visit(UnOp->getSubExpr());
  }

  StmtResult VisitGenericSelectionExpr(GenericSelectionExpr *GSE) {
    return Visit(GSE->getResultExpr());
  }

  StmtResult VisitChooseExpr(ChooseExpr *CE) {
    return Visit(CE->getChosenSubExpr());
  }

  StmtResult VisitConstantExpr(ConstantExpr *CE) {
    return Visit(CE->getSubExpr());
  }

  StmtResult VisitImplicitCastExpr(ImplicitCastExpr *ICE) {
    return Visit(ICE->getSubExpr());
  }

  StmtResult VisitFullExpr(FullExpr *FE) { return Visit(FE->getSubExpr()); }

  StmtResult VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *MTE) {
    return Visit(MTE->getSubExpr());
  }

  StmtResult
  VisitSubstNonTypeTemplateParmExpr(SubstNonTypeTemplateParmExpr *NTTP) {
    return Visit(NTTP->getReplacement());
  }
  /* End ignores */

  StmtResult VisitStmt(Stmt *S) {
    ReportUnsupportedStmt(S, Context);
    return ErrorResult;
  }

  StmtResult VisitDeclStmt(DeclStmt *DeclStmt) {
    for (Decl *D : DeclStmt->getDeclGroup()) {
      if (auto *VD = dyn_cast<VarDecl>(D)) {
        if (VD == AssignFindInfo.SelectedVarDecl) {
          auto *NVD = VarDecl::Create(
              Context, VD->getDeclContext(), {}, {}, VD->getIdentifier(),
              VD->getType().getUnqualifiedType(), nullptr, SC_None);
          HshStage Stage = HshNoStage;
          if (Expr *Init = VD->getInit()) {
            auto [InitStmt, InitStage] = Visit(Init);
            if (!InitStmt)
              return ErrorResult;
            NVD->setInit(cast<Expr>(InitStmt));
            Stage = InitStage;
          }
          return {new (Context) class DeclStmt(DeclGroupRef(NVD), {}, {}),
                  Stage};
        }
      }
    }
    return ErrorResult;
  }

  static StmtResult VisitNullStmt(NullStmt *NS) { return {NS, HshNoStage}; }

  StmtResult VisitBinaryOperator(BinaryOperator *BinOp) {
    auto [LStmt, LStage] = Visit(BinOp->getLHS());
    if (!LStmt)
      return ErrorResult;
    auto [RStmt, RStage] = Visit(BinOp->getRHS());
    if (!RStmt)
      return ErrorResult;
    HshStage Stage = std::max(LStage, RStage);

    const bool LHSInterpolated = GetInterpolated(LStage);
    const bool RHSInterpolated = GetInterpolated(RStage);
    if (LHSInterpolated || RHSInterpolated) {
      switch (BinOp->getOpcode()) {
      case BO_Add:
      case BO_Sub:
      case BO_Mul:
      case BO_AddAssign:
      case BO_SubAssign:
      case BO_MulAssign:
      case BO_Assign:
        break;
      case BO_Div:
      case BO_DivAssign:
        if (RHSInterpolated)
          Stage = Target;
        break;
      default:
        Stage = Target;
        break;
      }
    }

    LStmt = Builder.createInterStageReferenceExpr(cast<Expr>(LStmt), LStage,
                                                  Stage, AssignFindInfo);
    RStmt = Builder.createInterStageReferenceExpr(cast<Expr>(RStmt), RStage,
                                                  Stage, AssignFindInfo);
    auto *NewBinOp = new (Context)
        BinaryOperator(cast<Expr>(LStmt), cast<Expr>(RStmt), BinOp->getOpcode(),
                       BinOp->getType(), VK_XValue, OK_Ordinary, {}, {});

    return {NewBinOp, Stage};
  }

  StmtResult VisitExpr(Expr *E) {
    ReportUnsupportedStmt(E, Context);
    return ErrorResult;
  }

  StmtResult VisitCallExpr(CallExpr *CallExpr) {
    if (auto *DeclRef = dyn_cast<DeclRefExpr>(
            CallExpr->getCallee()->IgnoreParenImpCasts())) {
      if (auto *FD = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
        HshBuiltinFunction Func = Builtins.identifyBuiltinFunction(FD);
        if (Func != HBF_None) {
          auto Arguments = DoVisitExprRange(CallExpr->arguments(), CallExpr);
          if (!Arguments)
            return ErrorResult;

          if (CallExpr->getNumArgs() == 2) {
            const bool LHSInterpolated =
                GetInterpolated(Arguments->ExprStages[0]);
            const bool RHSInterpolated =
                GetInterpolated(Arguments->ExprStages[1]);
            if ((LHSInterpolated || RHSInterpolated) &&
                !HshBuiltins::isInterpolationDistributed(Func))
              Arguments->Stage = Target;
          }

          DoPromoteExprRange(*Arguments);
          auto *NCE =
              CallExpr::Create(Context, CallExpr->getCallee(), *Arguments,
                               CallExpr->getType(), VK_XValue, {});
          return {NCE, Arguments->Stage};
        }
      }
    }
    ReportUnsupportedFunctionCall(CallExpr, Context);
    return ErrorResult;
  }

  StmtResult VisitCXXMemberCallExpr(CXXMemberCallExpr *CallExpr) {
    CXXMethodDecl *MD = CallExpr->getMethodDecl();
    HshBuiltinCXXMethod Method = Builtins.identifyBuiltinMethod(MD);
    switch (Method) {
    case HBM_sample_texture2d: {
      HshStage Stage = HshNoStage;
      ParmVarDecl *PVD = nullptr;
      if (auto *TexRef = dyn_cast<DeclRefExpr>(
              CallExpr->getImplicitObjectArgument()->IgnoreParenImpCasts()))
        PVD = dyn_cast<ParmVarDecl>(TexRef->getDecl());
      if (PVD) {
        if (PVD->hasAttr<HshVertexTextureAttr>())
          Stage = HshVertexStage;
        else if (PVD->hasAttr<HshFragmentTextureAttr>())
          Stage = HshFragmentStage;
        else
          ReportUnattributedTexture(PVD, Context);
      } else {
        ReportBadTextureReference(CallExpr, Context);
      }
      auto [UVStmt, UVStage] = Visit(CallExpr->getArg(0));
      if (!UVStmt)
        return ErrorResult;
      Builder.registerSampleCall(Method, CallExpr);
      UVStmt = Builder.createInterStageReferenceExpr(
          cast<Expr>(UVStmt), UVStage, Stage, AssignFindInfo);
      std::array<Expr *, 2> NewArgs{cast<Expr>(UVStmt), CallExpr->getArg(1)};
      auto *NMCE =
          CXXMemberCallExpr::Create(Context, CallExpr->getCallee(), NewArgs,
                                    CallExpr->getType(), VK_XValue, {});
      return {NMCE, Stage};
    }
    default:
      ReportUnsupportedFunctionCall(CallExpr, Context);
      break;
    }
    return ErrorResult;
  }

  StmtResult VisitCastExpr(CastExpr *CastExpr) {
    if (Builtins.identifyBuiltinType(CastExpr->getType()) == HBT_None) {
      ReportUnsupportedTypeCast(CastExpr, Context);
      return ErrorResult;
    }
    return Visit(CastExpr->getSubExpr());
  }

  StmtResult VisitCXXConstructExpr(CXXConstructExpr *ConstructExpr) {
    if (Builtins.identifyBuiltinType(ConstructExpr->getType()) == HBT_None) {
      ReportUnsupportedTypeConstruct(ConstructExpr, Context);
      return ErrorResult;
    }

    auto Arguments =
        DoVisitExprRange(ConstructExpr->arguments(), ConstructExpr);
    if (!Arguments)
      return ErrorResult;
    DoPromoteExprRange(*Arguments);
    CXXConstructExpr *NCE = CXXTemporaryObjectExpr::Create(
        Context, ConstructExpr->getConstructor(), ConstructExpr->getType(),
        Context.getTrivialTypeSourceInfo(ConstructExpr->getType()), *Arguments,
        {}, ConstructExpr->hadMultipleCandidates(),
        ConstructExpr->isListInitialization(),
        ConstructExpr->isStdInitListInitialization(),
        ConstructExpr->requiresZeroInitialization());
    return {NCE, Arguments->Stage};
  }

  StmtResult VisitCXXOperatorCallExpr(CXXOperatorCallExpr *CallExpr) {
    auto Arguments = DoVisitExprRange(CallExpr->arguments(), CallExpr);
    if (!Arguments)
      return ErrorResult;

    if (CallExpr->getNumArgs() == 2) {
      const bool LHSInterpolated = GetInterpolated(Arguments->ExprStages[0]);
      const bool RHSInterpolated = GetInterpolated(Arguments->ExprStages[1]);
      if (LHSInterpolated || RHSInterpolated) {
        switch (CallExpr->getOperator()) {
        case OO_Plus:
        case OO_Minus:
        case OO_Star:
        case OO_PlusEqual:
        case OO_MinusEqual:
        case OO_StarEqual:
        case OO_Equal:
          break;
        case OO_Slash:
        case OO_SlashEqual:
          if (RHSInterpolated)
            Arguments->Stage = Target;
          break;
        default:
          Arguments->Stage = Target;
          break;
        }
      }
    }

    DoPromoteExprRange(*Arguments);
    auto *NCE = CXXOperatorCallExpr::Create(
        Context, CallExpr->getOperator(), CallExpr->getCallee(), *Arguments,
        CallExpr->getType(), VK_XValue, {}, {});
    return {NCE, Arguments->Stage};
  }

  StmtResult VisitDeclRefExpr(DeclRefExpr *DeclRef) {
    if (auto *VD = dyn_cast<VarDecl>(DeclRef->getDecl())) {
      if (!InMemberExpr &&
          Builtins.identifyBuiltinType(VD->getType()) == HBT_None) {
        ReportUnsupportedTypeReference(DeclRef, Context);
        return ErrorResult;
      }
      if (auto *PVD = dyn_cast<ParmVarDecl>(DeclRef->getDecl())) {
        HshStage Stage = DetermineParmVarStage(PVD);
        if (Stage == HshHostStage)
          Builder.registerUsedCapture(PVD);
        return {DeclRef, Stage};
      }
      auto [Assign, NextCompoundChild] = LastAssignmentFinder(Context).Find(
          VD, AssignFindInfo.Body, AssignFindInfo.LastCompoundChild);
      if (Assign) {
        SaveAndRestore<Stmt *> SavedCompoundChild(
            AssignFindInfo.LastCompoundChild, NextCompoundChild);
        SaveAndRestore<VarDecl *> SavedSelectedVarDecl(
            AssignFindInfo.SelectedVarDecl, VD);
        auto [AssignStmt, AssignStage] = Visit(Assign);
        if (!AssignStmt)
          return ErrorResult;
        Builder.addStageStmt(AssignStmt, AssignStage, VD);
        return {DeclRef, AssignStage};
      }
    }
    return ErrorResult;
  }

  StmtResult VisitInitListExpr(InitListExpr *InitList) {
    auto Exprs = DoVisitExprRange(InitList->inits(), InitList);
    if (!Exprs)
      return ErrorResult;
    DoPromoteExprRange(*Exprs);
    return {new (Context) InitListExpr(Context, {}, *Exprs, {}), Exprs->Stage};
  }

  StmtResult VisitMemberExpr(MemberExpr *MemberExpr) {
    if (!InMemberExpr &&
        Builtins.identifyBuiltinType(MemberExpr->getType()) == HBT_None) {
      ReportUnsupportedTypeReference(MemberExpr, Context);
      return ErrorResult;
    }
    SaveAndRestore<bool> SavedInMemberExpr(InMemberExpr, true);
    auto [BaseStmt, BaseStage] = Visit(MemberExpr->getBase());
    auto *NME = MemberExpr::CreateImplicit(
        Context, cast<Expr>(BaseStmt), false, MemberExpr->getMemberDecl(),
        MemberExpr->getType(), VK_XValue, OK_Ordinary);
    return {NME, BaseStage};
  }

  static StmtResult VisitFloatingLiteral(FloatingLiteral *FL) {
    return {FL, HshNoStage};
  }

  static StmtResult VisitIntegerLiteral(IntegerLiteral *IL) {
    return {IL, HshNoStage};
  }

  void Trace(Stmt *Assign, Stmt *B, Stmt *LCC, HshStage T) {
    AssignFindInfo.Body = B;
    AssignFindInfo.LastCompoundChild = LCC;
    Target = T;
    auto [AssignStmt, AssignStage] = Visit(Assign);
    if (!AssignStage)
      return;
    AssignStmt = Builder.createInterStageReferenceExpr(
        cast<Expr>(AssignStmt), AssignStage, T, AssignFindInfo);
    Builder.addStageStmt(AssignStmt, T);
  }
};

struct ShaderPrintingPolicyBase : PrintingPolicy {
  virtual ~ShaderPrintingPolicyBase() = default;
  using PrintingPolicy::PrintingPolicy;
};

template <typename ImplClass>
struct ShaderPrintingPolicy : PrintingCallbacks, ShaderPrintingPolicyBase {
  HshBuiltins &Builtins;
  explicit ShaderPrintingPolicy(HshBuiltins &Builtins)
      : ShaderPrintingPolicyBase(LangOptions()), Builtins(Builtins) {
    Callbacks = this;
    IncludeTagDefinition = false;
    SuppressTagKeyword = true;
    SuppressScope = true;
    AnonymousTagLocations = false;
    SuppressImplicitBase = true;

    DisableTypeQualifiers = true;
    DisableListInitialization = true;
  }

  StringRef overrideTagDeclIdentifier(TagDecl *D) const override {
    auto HBT = Builtins.identifyBuiltinType(D->getTypeForDecl());
    if (HBT == HBT_None)
      return {};
    return HshBuiltins::getSpelling<ImplClass::SourceTarget>(HBT);
  }

  StringRef overrideBuiltinFunctionIdentifier(CallExpr *C) const override {
    if (auto *MemberCall = dyn_cast<CXXMemberCallExpr>(C)) {
      auto HBM = Builtins.identifyBuiltinMethod(MemberCall->getMethodDecl());
      if (HBM == HBM_None)
        return {};
      return ImplClass::identifierOfCXXMethod(HBM, MemberCall);
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
      return ImplClass::overrideCXXMethodArguments(HBM, MemberCall, StringArg,
                                                   ExprArg);
    }
    return false;
  }

  StringRef overrideDeclRefIdentifier(DeclRefExpr *DR) const override {
    if (auto *PVD = dyn_cast<ParmVarDecl>(DR->getDecl())) {
      if (PVD->hasAttr<HshPositionAttr>())
        return static_cast<const ImplClass &>(*this)
            .identifierOfVertexPosition();
    }
    return {};
  }
};

struct GLSLPrintingPolicy : ShaderPrintingPolicy<GLSLPrintingPolicy> {
  static constexpr HshTarget SourceTarget = HT_GLSL;
  static constexpr StringRef identifierOfVertexPosition() {
    return llvm::StringLiteral("gl_Position");
  }

  static constexpr StringRef identifierOfCXXMethod(HshBuiltinCXXMethod HBM,
                                                   CXXMemberCallExpr *C) {
    switch (HBM) {
    case HBM_sample_texture2d:
      return llvm::StringLiteral("texture");
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

  using ShaderPrintingPolicy<GLSLPrintingPolicy>::ShaderPrintingPolicy;
};

static std::unique_ptr<ShaderPrintingPolicyBase>
MakePrintingPolicy(HshTarget Target, HshBuiltins &Builtins) {
  switch (Target) {
  case HT_GLSL:
  case HT_HLSL:
  case HT_HLSL_BIN:
  case HT_METAL:
  case HT_METAL_BIN_MAC:
  case HT_METAL_BIN_IOS:
  case HT_METAL_BIN_TVOS:
  case HT_SPIRV:
  case HT_DXIL:
    return std::make_unique<GLSLPrintingPolicy>(Builtins);
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

class GenerateConsumer : public ASTConsumer, MatchFinder::MatchCallback {
  HshBuiltins Builtins;
  CompilerInstance &CI;
  ASTContext &Context;
  Preprocessor &PP;
  ArrayRef<HshTarget> Targets;
  std::unique_ptr<raw_pwrite_stream> OS;
  Optional<std::pair<SourceLocation, std::string>> HeadInclude;
  std::map<SourceLocation, std::pair<SourceRange, std::string>>
      SeenHshExpansions;

public:
  explicit GenerateConsumer(CompilerInstance &CI, ArrayRef<HshTarget> Targets)
      : CI(CI), Context(CI.getASTContext()), PP(CI.getPreprocessor()),
        Targets(Targets) {}

  void run(const MatchFinder::MatchResult &Result) override {
    auto *LambdaAttr = Result.Nodes.getNodeAs<AttributedStmt>("attrid");
    auto *Lambda = Result.Nodes.getNodeAs<LambdaExpr>("id");
    if (Lambda && LambdaAttr) {
      auto ExpName = getExpansionNameBeforeLambda(LambdaAttr);
      assert(!ExpName.empty() && "Expansion name should exist");

      auto *CallOperator = Lambda->getCallOperator();
      Stmt *Body = CallOperator->getBody();

      unsigned UseStages = 1;
      for (ParmVarDecl *Param : CallOperator->parameters()) {
        if (DetermineParmVarDirection(Param) != HshInput) {
          if (Param->hasAttr<HshPositionAttr>()) {
            if (Builtins.identifyBuiltinType(Param->getType()) != HBT_float4) {
              ReportBadVertexPositionType(Param, Context);
              return;
            }
          } else if (Param->hasAttr<HshColorTargetAttr>()) {
            if (Builtins.identifyBuiltinType(Param->getType()) != HBT_float4) {
              ReportBadColorTargetType(Param, Context);
              return;
            }
          }
          UseStages |= (1u << unsigned(DetermineParmVarStage(Param)));
        }
      }

      CXXRecordDecl *SpecRecord =
          Builtins.getHshBaseSpecialization(Context, ExpName);
      StagesBuilder Builder(Context, SpecRecord, UseStages);

      for (int i = HshVertexStage; i < HshMaxStage; ++i) {
        for (ParmVarDecl *Param : CallOperator->parameters()) {
          if (DetermineParmVarDirection(Param) == HshInput ||
              DetermineParmVarStage(Param) != HshStage(i))
            continue;
          auto [Assign, LastCompoundChild] =
              LastAssignmentFinder(Context).Find(Param, Body);
          if (Context.getDiagnostics().hasErrorOccurred())
            return;
          if (Assign)
            ValueTracer(Context, Builtins, Builder)
                .Trace(Assign, Body, LastCompoundChild, HshStage(i));
        }
      }

      // Add global list node static
      SpecRecord->addDecl(Builtins.getGlobalListNode(Context, SpecRecord));

      // Finalize expressions and add host to stage records
      Builder.finalizeResults(Context, Builtins, SpecRecord);

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
          Context.getFunctionType(CDType, ConstructorArgs, {}), {}, {}, false,
          false, CSK_unspecified);
      CD->setParams(ConstructorParms);
      CD->setAccess(AS_public);
      CD->setBody(
          CompoundStmt::Create(Context, Builder.hostStatements(), {}, {}));
      SpecRecord->addDecl(CD);

      // Add shader data var template
      SpecRecord->addDecl(Builtins.getDataVarTemplate(Context, SpecRecord));

      SpecRecord->completeDefinition();

      // Emit shader record
      SpecRecord->print(*OS, Context.getPrintingPolicy());
      *OS << ";\nhsh::_HshGlobalListNode " << ExpName << "::global{&" << ExpName
          << "::global_build};\n";

      // Emit define macro for capturing args
      *OS << "#define " << ExpName << " ::" << ExpName << "(";
      bool NeedsComma = false;
      for (const auto *Cap : Builder.captures()) {
        if (!NeedsComma)
          NeedsComma = true;
        else
          *OS << ", ";
        *OS << Cap->getIdentifier()->getName();
      }
      *OS << "); (void)\n\n";

      // Emit shader data
      for (const auto &T : Targets) {
        auto Policy = MakePrintingPolicy(T, Builtins);
        Builder.printResults(*Policy, *OS);
      }

      ASTDumper P(llvm::errs(), nullptr, &Context.getSourceManager());
      P.Visit(Body);
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
      Diags.Report(IncludeDiagID);
      return;
    }
    if (NamespaceDecl *NS = LocationNamespaceSearch(Context).findNamespace(
            HeadInclude->first)) {
      Diags.Report(HeadInclude->first, IncludeDiagID);
      unsigned NoteDiagID = Diags.getCustomDiagID(DiagnosticsEngine::Note,
                                                  "included in namespace");
      Diags.Report(NS->getLocation(), NoteDiagID);
      return;
    }

    Builtins.findBuiltinDecls(Context);
    if (Context.getDiagnostics().hasErrorOccurred()) {
      ASTDumper P(llvm::errs(), nullptr, &Context.getSourceManager());
      P.Visit(Context.getTranslationUnitDecl());
      return;
    }

    OS = CI.createDefaultOutputFile(false);

    SourceManager &SM = Context.getSourceManager();
    StringRef MainName = SM.getFileEntryForID(SM.getMainFileID())->getName();
    *OS << "/* Auto-generated hshhead for " << MainName
        << " */\n\n"
           "namespace {\n\n";

    /*
     * Find lambdas that are attributed with hsh::generator_lambda and exist
     * within the main file.
     */
    MatchFinder Finder;
    Finder.addMatcher(
        attributedStmt(stmt().bind("attrid"),
                       allOf(hasStmtAttr(attr::HshGeneratorLambda),
                             hasDescendant(lambdaExpr(
                                 stmt().bind("id"), isExpansionInMainFile())))),
        this);
    Finder.matchAST(Context);

    *OS << "}\n";
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

  void registerHshHeadInclude(SourceLocation HashLoc, StringRef RelativePath) {
    if (Context.getSourceManager().isWrittenInMainFile(HashLoc)) {
      if (HeadInclude) {
        DiagnosticsEngine &Diags = Context.getDiagnostics();
        unsigned DiagID = Diags.getCustomDiagID(
            DiagnosticsEngine::Error, "multiple hshhead includes in one file");
        Diags.Report(HashLoc, DiagID);
        unsigned DiagID2 = Diags.getCustomDiagID(DiagnosticsEngine::Note,
                                                 "previous include was here");
        Diags.Report(HeadInclude->first, DiagID2);
        return;
      } else {
        HeadInclude.emplace(HashLoc, RelativePath);
      }
    }
  }

  void registerHshExpansion(SourceRange Range, StringRef Name) {
    if (Context.getSourceManager().isWrittenInMainFile(Range.getBegin())) {
      for (auto &Exps : SeenHshExpansions) {
        if (Exps.second.second == Name) {
          DiagnosticsEngine &Diags = Context.getDiagnostics();
          unsigned DiagID = Diags.getCustomDiagID(
              DiagnosticsEngine::Error, "hsh_* macro must be suffixed with "
                                        "identifier unique to the file");
          Diags.Report(Range.getBegin(), DiagID)
              << CharSourceRange(Range, false);
          unsigned NoteDiagId = Diags.getCustomDiagID(
              DiagnosticsEngine::Note, "previous identifier usage is here");
          Diags.Report(Exps.first, NoteDiagId)
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

  public:
    explicit PPCallbacks(GenerateConsumer &Consumer, FileManager &FM,
                         SourceManager &SM)
        : Consumer(Consumer), FM(FM), SM(SM) {}
    bool FileNotFound(StringRef FileName,
                      SmallVectorImpl<char> &RecoveryPath) override {
      if (FileName.endswith_lower(llvm::StringLiteral(".hshhead"))) {
        SmallString<1024> VirtualFilePath(llvm::StringLiteral("./"));
        VirtualFilePath += FileName;
        FM.getVirtualFile(VirtualFilePath, 0, std::time(nullptr));
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
      if (FileName.endswith_lower(llvm::StringLiteral(".hshhead"))) {
        assert(File && "File must exist at this point");
        SM.overrideFileContents(File, llvm::MemoryBuffer::getMemBuffer(""));
        Consumer.registerHshHeadInclude(HashLoc, RelativePath);
      }
    }
    void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                      SourceRange Range, const MacroArgs *Args) override {
      if (MacroNameTok.is(tok::identifier)) {
        StringRef Name = MacroNameTok.getIdentifierInfo()->getName();
        if (Name.startswith("hsh_"))
          Consumer.registerHshExpansion(Range, Name);
      }
    }
  };
};

std::unique_ptr<ASTConsumer>
GenerateAction::CreateASTConsumer(CompilerInstance &CI, StringRef InFile) {
  auto Policy = CI.getASTContext().getPrintingPolicy();
  Policy.Indentation = true;
  Policy.SuppressImplicitBase = true;
  CI.getASTContext().setPrintingPolicy(Policy);
  auto Consumer = std::make_unique<GenerateConsumer>(CI, Targets);
  CI.getPreprocessor().addPPCallbacks(
      std::make_unique<GenerateConsumer::PPCallbacks>(
          *Consumer, CI.getFileManager(), CI.getSourceManager()));
  return Consumer;
}

} // namespace clang::hshgen
