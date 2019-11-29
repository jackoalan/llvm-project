#include "hosh/HoshGenerator.h"

#include "llvm/IR/Value.h"
#include "llvm/Support/SaveAndRestore.h"

#include "clang/AST/DeclVisitor.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/ASTDumper.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/Version.h"
#include "clang/CodeGen/ModuleBuilder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

#include <regex>

namespace clang::ast_matchers {
const internal::VariadicDynCastAllOfMatcher<Stmt, AttributedStmt> attributedStmt;
AST_MATCHER_P(AttributedStmt, hasStmtAttr, attr::Kind, AttrKind) {
  for (const auto *Attr : Node.getAttrs()) {
    if (Attr->getKind() == AttrKind)
      return true;
  }
  return false;
}
}

namespace hosh::gen {

using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;
using namespace std::literals;

constexpr llvm::StringLiteral StubInclude(
  "[&]() {\n"
  "  class {\n"
  "  public:\n"
  "    void draw(std::size_t, std::size_t) {}\n"
  "    void bind(hosh::detail::base_vertex_buffer) {}\n"
  "  } hosh_binding;\n"
  "  return hosh_binding;\n"
  "}();\n"
  "[[hosh::generator_lambda]]\n");

static StringRef HoshStageToString(HoshStage Stage) {
  switch (Stage) {
  case HoshHostStage:
    return llvm::StringLiteral("host");
  case HoshVertexStage:
    return llvm::StringLiteral("vertex");
  case HoshControlStage:
    return llvm::StringLiteral("control");
  case HoshEvaluationStage:
    return llvm::StringLiteral("evaluation");
  case HoshGeometryStage:
    return llvm::StringLiteral("geometry");
  case HoshFragmentStage:
    return llvm::StringLiteral("fragment");
  default:
    return llvm::StringLiteral("none");
  }
}

enum HoshBuiltinType {
  HBT_None,
#define BUILTIN_TYPE(Name, GLSL, HLSL, Metal, Vector, Matrix) \
  HBT_##Name,
#include "BuiltinTypes.def"
  HBT_Max
};

enum HoshBuiltinFunction {
  HBF_None,
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist) \
  HBF_##Name,
#include "BuiltinFunctions.def"
  HBF_Max
};

enum HoshBuiltinCXXMethod {
  HBM_None,
#define BUILTIN_CXX_METHOD(Name, Record, ...) \
  HBM_##Name##_##Record,
#include "BuiltinCXXMethods.def"
  HBM_Max
};

class HoshBuiltins {
public:
  struct Spellings {
    StringRef GLSL, HLSL, Metal;
  };
private:
  std::array<const clang::TagDecl*, HBT_Max> Types{};
  std::array<const clang::FunctionDecl*, HBF_Max> Functions{};
  std::array<const clang::CXXMethodDecl*, HBM_Max> Methods{};

  static constexpr Spellings BuiltinTypeSpellings[] = {
    {{}, {}, {}},
#define BUILTIN_TYPE(Name, GLSL, HLSL, Metal, Vector, Matrix) \
    {llvm::StringLiteral(#GLSL), llvm::StringLiteral(#HLSL), llvm::StringLiteral(#Metal)},
#include "BuiltinTypes.def"
  };

  static constexpr Spellings BuiltinFunctionSpellings[] = {
    {{}, {}, {}},
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist) \
    {llvm::StringLiteral(#GLSL), llvm::StringLiteral(#HLSL), llvm::StringLiteral(#Metal)},
#include "BuiltinFunctions.def"
  };

  static constexpr bool BuiltinFunctionInterpDists[] = {
    false,
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist) \
    InterpDist,
#include "BuiltinFunctions.def"
  };

  template <typename ImplClass>
  class DeclFinder : public DeclVisitor<ImplClass, bool> {
    using base = DeclVisitor<ImplClass, bool>;
  protected:
    StringRef Name;
    Decl *Found;
    bool InHoshNS = false;
  public:
    bool VisitDecl(Decl *D) {
      if (auto *DC = dyn_cast<DeclContext>(D))
        for (Decl *Child : DC->decls())
          if (!base::Visit(Child))
            return false;
      return true;
    }

    bool VisitNamespaceDecl(NamespaceDecl *Namespace) {
      if (InHoshNS)
        return true;
      bool Ret = true;
      if (Namespace->getDeclName().isIdentifier() && Namespace->getName() == llvm::StringLiteral("hosh")) {
        SaveAndRestore<bool> SavedInHoshNS(InHoshNS, true);
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
      if (InHoshNS && Type->getDeclName().isIdentifier() && Type->getName() == Name) {
        Found = Type;
        return false;
      }
      return true;
    }
  };

  class FuncFinder : public DeclFinder<FuncFinder> {
  public:
    bool VisitFunctionDecl(FunctionDecl *Func) {
      if (InHoshNS && Func->getDeclName().isIdentifier() && Func->getName() == Name) {
        Found = Func;
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
      if (InHoshNS && Method->getDeclName().isIdentifier() && Method->getName() == Name &&
          Method->getParent()->getName() == Record && Method->getNumParams() == Params.size()) {
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
      for (auto& ParamStr : Params)
        ParamStr = ParamStr.trim();
      Found = nullptr;
      Visit(TU);
      return Found;
    }
  };

  void addType(SourceManager &SM, HoshBuiltinType TypeKind, StringRef Name, Decl *D) {
    if (auto *T = dyn_cast_or_null<TagDecl>(D)) {
      Types[TypeKind] = T->getFirstDecl();
    } else {
      DiagnosticsEngine &Diags = SM.getDiagnostics();
      unsigned DiagID = Diags.getCustomDiagID(DiagnosticsEngine::Error,
        "unable to locate declaration of builtin type %0; is hosh.hpp included?");
      auto Diag = Diags.Report(DiagID);
      Diag.AddString(Name);
    }
  }

  void addFunction(SourceManager &SM, HoshBuiltinFunction FuncKind, StringRef Name, Decl *D) {
    if (auto *F = dyn_cast_or_null<FunctionDecl>(D)) {
      Functions[FuncKind] = F->getFirstDecl();
    } else {
      DiagnosticsEngine &Diags = SM.getDiagnostics();
      unsigned DiagID = Diags.getCustomDiagID(DiagnosticsEngine::Error,
        "unable to locate declaration of builtin function %0; is hosh.hpp included?");
      auto Diag = Diags.Report(DiagID);
      Diag.AddString(Name);
    }
  }

  void addCXXMethod(SourceManager &SM, HoshBuiltinCXXMethod MethodKind, StringRef Name, Decl *D) {
    if (auto *M = dyn_cast_or_null<CXXMethodDecl>(D)) {
      Methods[MethodKind] = dyn_cast<CXXMethodDecl>(M->getFirstDecl());
    } else {
      DiagnosticsEngine &Diags = SM.getDiagnostics();
      unsigned DiagID = Diags.getCustomDiagID(DiagnosticsEngine::Error,
        "unable to locate declaration of builtin method %0; is hosh.hpp included?");
      auto Diag = Diags.Report(DiagID);
      Diag.AddString(Name);
    }
  }

public:
  void findBuiltinDecls(TranslationUnitDecl *TU, SourceManager &SM) {
#define BUILTIN_TYPE(Name, GLSL, HLSL, Metal, Vector, Matrix) \
    addType(SM, HBT_##Name, llvm::StringLiteral(#Name), TypeFinder().Find(llvm::StringLiteral(#Name), TU));
#include "BuiltinTypes.def"
#define BUILTIN_FUNCTION(Name, GLSL, HLSL, Metal, InterpDist) \
    addFunction(SM, HBF_##Name, llvm::StringLiteral(#Name), FuncFinder().Find(llvm::StringLiteral(#Name), TU));
#include "BuiltinFunctions.def"
#define BUILTIN_CXX_METHOD(Name, Record, ...) \
    addCXXMethod(SM, HBM_##Name##_##Record, llvm::StringLiteral(#Record "::" #Name "(" #__VA_ARGS__ ")"), \
      MethodFinder().Find(llvm::StringLiteral(#Name), llvm::StringLiteral(#Record), \
                          llvm::StringLiteral(#__VA_ARGS__), TU));
#include "BuiltinCXXMethods.def"
  }

  HoshBuiltinType identifyBuiltinType(QualType QT) const {
    const clang::Type *UT = QT.getNonReferenceType().getTypePtrOrNull();
    if (!UT) return HBT_None;
    TagDecl *T = UT->getAsTagDecl();
    if (!T) return HBT_None;
    T = T->getFirstDecl();
    if (!T) return HBT_None;
    HoshBuiltinType Ret = HBT_None;
    for (const auto *Tp : Types) {
      if (T == Tp)
        return Ret;
      Ret = HoshBuiltinType(int(Ret) + 1);
    }
    return HBT_None;
  }

  HoshBuiltinFunction identifyBuiltinFunction(const clang::FunctionDecl *F) const {
    F = F->getFirstDecl();
    if (!F) return HBF_None;
    HoshBuiltinFunction Ret = HBF_None;
    for (const auto *Func : Functions) {
      if (F == Func)
        return Ret;
      Ret = HoshBuiltinFunction(int(Ret) + 1);
    }
    return HBF_None;
  }

  HoshBuiltinCXXMethod identifyBuiltinMethod(const clang::CXXMethodDecl *M) const {
    M = dyn_cast_or_null<CXXMethodDecl>(M->getFirstDecl());
    if (!M) return HBM_None;
    if (FunctionDecl *FD = M->getInstantiatedFromMemberFunction())
      M = dyn_cast<CXXMethodDecl>(FD->getFirstDecl());
    HoshBuiltinCXXMethod Ret = HBM_None;
    for (const auto *Method : Methods) {
      if (M == Method)
        return Ret;
      Ret = HoshBuiltinCXXMethod(int(Ret) + 1);
    }
    return HBM_None;
  }

  static constexpr const Spellings &getSpellings(HoshBuiltinType Tp) {
    return BuiltinTypeSpellings[Tp];
  }

  static constexpr const Spellings &getSpellings(HoshBuiltinFunction Func) {
    return BuiltinFunctionSpellings[Func];
  }

  static constexpr bool isInterpolationDistributed(HoshBuiltinFunction Func) {
    return BuiltinFunctionInterpDists[Func];
  }

  const clang::TagDecl *getTypeDecl(HoshBuiltinType Tp) const {
    return Types[Tp];
  }

  QualType getType(HoshBuiltinType Tp) const {
    return getTypeDecl(Tp)->getTypeForDecl()->getCanonicalTypeUnqualified();
  }
};

enum HoshInterfaceDirection {
  HoshInput,
  HoshOutput,
  HoshInOut
};

static HoshStage DetermineParmVarStage(ParmVarDecl *D) {
#define INTERFACE_VARIABLE(Attr, Stage, Direction, Array) \
  if (D->hasAttr<Attr>()) return Stage;
#include "ShaderInterface.def"
  return HoshHostStage;
}

template <typename T, unsigned N>
static DiagnosticBuilder ReportCustom(T *S, const SourceManager &SM,
                                      const char (&FormatString)[N],
                                      DiagnosticsEngine::Level level = DiagnosticsEngine::Error) {
  DiagnosticsEngine &Diags = SM.getDiagnostics();
  unsigned DiagID = Diags.getCustomDiagID(level, FormatString);
  auto Diag = Diags.Report(S->getBeginLoc(), DiagID);
  Diag.AddSourceRange(CharSourceRange::getCharRange(S->getSourceRange()));
  return Diag;
}

static void ReportUnsupportedStmt(Stmt *S, const SourceManager &SM) {
  auto Diag = ReportCustom(S, SM, "statements of type %0 are not supported in hosh generator lambdas");
  Diag.AddString(S->getStmtClassName());
}

static void ReportUnsupportedFunctionCall(Stmt *S, const SourceManager &SM) {
  ReportCustom(S, SM, "function calls are limited to hosh intrinsics");
}

static void ReportUnsupportedTypeReference(Stmt *S, const SourceManager &SM) {
  ReportCustom(S, SM, "references to values are limited to hosh types");
}

static void ReportUnsupportedTypeConstruct(Stmt *S, const SourceManager &SM) {
  ReportCustom(S, SM, "constructors are limited to hosh types");
}

static void ReportUnsupportedTypeCast(Stmt *S, const SourceManager &SM) {
  ReportCustom(S, SM, "type casts are limited to hosh types");
}

static void ReportBadTextureReference(Stmt *S, const SourceManager &SM) {
  ReportCustom(S, SM, "texture samples must be performed on lambda parameters");
}

static void ReportUnattributedTexture(ParmVarDecl *PVD, const SourceManager &SM) {
  ReportCustom(PVD, SM, "sampled textures must be attributed with [[hosh::*_texture(n)]]");
}

class LastAssignmentFinder : public StmtVisitor<LastAssignmentFinder, bool> {
  const SourceManager &SM;
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
  explicit LastAssignmentFinder(const SourceManager &SM) : SM(SM) {}

  bool VisitStmt(Stmt *S) {
    ReportUnsupportedStmt(S, SM);
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

  bool VisitNullStmt(NullStmt *) {
    return true;
  }

  bool VisitValueStmt(ValueStmt *ValueStmt) {
    return DoVisit(ValueStmt->getExprStmt());
  }

  bool VisitBinaryOperator(BinaryOperator *BinOp) {
    if (BinOp->isAssignmentOp()) {
      {
        SaveAndRestore<Stmt*> SavedAssign(Assign, BinOp);
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
    ReportUnsupportedStmt(E, SM);
    return true;
  }

  bool VisitBlockExpr(BlockExpr *Block) {
    return DoVisit(Block->getBody());
  }

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
        SaveAndRestore<Stmt*> SavedAssign(Assign, CallExpr);
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

  bool VisitFloatingLiteral(FloatingLiteral *) {
    return true;
  }

  bool VisitIntegerLiteral(IntegerLiteral *) {
    return true;
  }

  std::tuple<Stmt*, Stmt*> Find(VarDecl *V, Stmt *Body, Stmt *E = nullptr) {
    Var = V;
    End = E;
    DoVisit(Body);
    return {LastAssign, LastCompoundChild};
  }
};

class StmtPromotions {
  std::multimap<std::pair<HoshStage, HoshStage>, std::pair<Stmt*,Stmt*>> Promotions;
  void AddUniquePromotion(std::pair<HoshStage, HoshStage> Key, std::pair<Stmt*,Stmt*> Value) {
    auto Range = Promotions.equal_range(Key);
    for (auto I = Range.first; I != Range.second; ++I)
      if (I->second == Value)
        return;
    Promotions.insert(std::make_pair(Key, Value));
  }
public:
  void AddPossiblePromotion(Stmt *From, Stmt *To, const HoshStage Target) {
    const HoshStage FromStage = From->getMaxHoshStage();
    const HoshStage ToStage = To->getMaxHoshStage();
    if (FromStage < ToStage)
      AddUniquePromotion(std::make_pair(FromStage, ToStage), std::make_pair(From, To));
  }
  void printPromotions(const ASTContext &Context) const {
    for (auto I = Promotions.begin(), E = Promotions.end(); I != E;) {
      llvm::outs() << "Promoting "
                   << HoshStageToString(I->first.first) << " to "
                   << HoshStageToString(I->first.second) << "\n";
      auto II = I;
      for (; II != E && II->first == I->first; ++II) {
        llvm::outs() << "  Promoted ";
        II->second.first->printPretty(llvm::outs(), nullptr, Context.getPrintingPolicy());
        llvm::outs() << "\n    within ";
        II->second.second->printPretty(llvm::outs(), nullptr, Context.getPrintingPolicy());
        llvm::outs() << "\n";
      }
      I = II;
    }
    llvm::outs().flush();
  }
};

const ASTContext* TmpContext = nullptr;

class TracingStmtPromoter : public StmtVisitor<TracingStmtPromoter> {
  const SourceManager &SM;
  const HoshBuiltins &Builtins;
  StmtPromotions &Promotions;
  Stmt *Body = nullptr;
  Stmt *LastCompoundChild = nullptr;
  HoshStage Target = HoshNoStage;
  VarDecl *SelectedVarDecl = nullptr;
  bool InMemberExpr = false;

  void DoVisit(Stmt *S, Stmt *Parent) {
    llvm::outs() << "Visiting ";
    S->printPretty(llvm::outs(), nullptr, TmpContext->getPrintingPolicy());
    llvm::outs() << '\n';
    llvm::outs().flush();
    Visit(S);
    Parent->mergeHoshStages(S);
    Parent->setHoshStage(Parent->getMaxHoshStage());
  }

  void AddPossiblePromotion(Stmt *S, Stmt *Parent) {
    Promotions.AddPossiblePromotion(S, Parent, Target);
  }

  bool GetInterpolated(Stmt *S) const {
    const HoshStage Stage = S->getMaxHoshStage();
    return Stage != HoshHostStage && Stage < Target;
  }
public:
  explicit TracingStmtPromoter(const SourceManager &SM, const HoshBuiltins &Builtins, StmtPromotions &Promotions)
  : SM(SM), Builtins(Builtins), Promotions(Promotions) {}

  /* Begin ignores */
  void VisitBlockExpr(BlockExpr *Block) {
    DoVisit(Block->getBody(), Block);
  }

  void VisitValueStmt(ValueStmt *ValueStmt) {
    DoVisit(ValueStmt->getExprStmt(), ValueStmt);
  }

  void VisitUnaryOperator(UnaryOperator *UnOp) {
    DoVisit(UnOp->getSubExpr(), UnOp);
  }

  void VisitGenericSelectionExpr(GenericSelectionExpr *GSE) {
    DoVisit(GSE->getResultExpr(), GSE);
  }

  void VisitChooseExpr(ChooseExpr *CE) {
    DoVisit(CE->getChosenSubExpr(), CE);
  }

  void VisitConstantExpr(ConstantExpr *CE) {
    DoVisit(CE->getSubExpr(), CE);
  }

  void VisitImplicitCastExpr(ImplicitCastExpr *ICE) {
    DoVisit(ICE->getSubExpr(), ICE);
  }

  void VisitFullExpr(FullExpr *FE) {
    DoVisit(FE->getSubExpr(), FE);
  }

  void VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr *MTE) {
    DoVisit(MTE->getSubExpr(), MTE);
  }

  void VisitSubstNonTypeTemplateParmExpr(SubstNonTypeTemplateParmExpr *NTTP) {
    DoVisit(NTTP->getReplacement(), NTTP);
  }
  /* End ignores */

  void VisitStmt(Stmt *S) {
    ReportUnsupportedStmt(S, SM);
  }

  void VisitDeclStmt(DeclStmt *DeclStmt) {
    for (Decl *D : DeclStmt->getDeclGroup()) {
      if (auto *VD = dyn_cast<VarDecl>(D)) {
        if (VD == SelectedVarDecl) {
          if (Expr *Init = VD->getInit())
            DoVisit(Init, DeclStmt);
          break;
        }
      }
    }
  }

  void VisitNullStmt(NullStmt *) {}

  void VisitBinaryOperator(BinaryOperator *BinOp) {
    DoVisit(BinOp->getLHS(), BinOp);
    DoVisit(BinOp->getRHS(), BinOp);

    const bool LHSInterpolated = GetInterpolated(BinOp->getLHS());
    const bool RHSInterpolated = GetInterpolated(BinOp->getRHS());
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
          BinOp->setHoshStage(Target);
        break;
      default:
        BinOp->setHoshStage(Target);
        break;
      }
    }

    AddPossiblePromotion(BinOp->getLHS(), BinOp);
    AddPossiblePromotion(BinOp->getRHS(), BinOp);
  }

  void VisitExpr(Expr *E) {
    ReportUnsupportedStmt(E, SM);
  }

  void VisitCallExpr(CallExpr *CallExpr) {
    if (auto *DeclRef = dyn_cast<DeclRefExpr>(CallExpr->getCallee()->IgnoreParenImpCasts())) {
      if (auto *FD = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
        HoshBuiltinFunction Func = Builtins.identifyBuiltinFunction(FD);
        if (Func != HBF_None) {
          for (Expr *Arg : CallExpr->arguments())
            DoVisit(Arg, CallExpr);

          if (CallExpr->getNumArgs() == 2) {
            const bool LHSInterpolated = GetInterpolated(CallExpr->getArg(0));
            const bool RHSInterpolated = GetInterpolated(CallExpr->getArg(1));
            if ((LHSInterpolated || RHSInterpolated) && !HoshBuiltins::isInterpolationDistributed(Func))
              CallExpr->setHoshStage(Target);
          }

          for (Expr *Arg : CallExpr->arguments())
            AddPossiblePromotion(Arg, CallExpr);
          return;
        }
      }
    }
    ReportUnsupportedFunctionCall(CallExpr, SM);
  }

  void VisitCXXMemberCallExpr(CXXMemberCallExpr *CallExpr) {
    CXXMethodDecl *MD = CallExpr->getMethodDecl();
    HoshBuiltinCXXMethod Method = Builtins.identifyBuiltinMethod(MD);
    switch (Method) {
      case HBM_sample_texture2d: {
        ParmVarDecl *PVD = nullptr;
        if (auto *TexRef = dyn_cast<DeclRefExpr>(CallExpr->getImplicitObjectArgument()->IgnoreParenImpCasts()))
          PVD = dyn_cast<ParmVarDecl>(TexRef->getDecl());
        if (PVD) {
          if (PVD->hasAttr<HoshVertexTextureAttr>())
            CallExpr->setHoshStage(HoshVertexStage);
          else if (PVD->hasAttr<HoshFragmentTextureAttr>())
            CallExpr->setHoshStage(HoshFragmentStage);
          else
            ReportUnattributedTexture(PVD, SM);
        } else {
          ReportBadTextureReference(CallExpr, SM);
        }
        Expr *UVArg = CallExpr->getArg(0);
        DoVisit(UVArg, CallExpr);
        AddPossiblePromotion(UVArg, CallExpr);
        break;
      }
      default:
        ReportUnsupportedFunctionCall(CallExpr, SM);
        break;
    }
  }

  void VisitCastExpr(CastExpr *CastExpr) {
    if (Builtins.identifyBuiltinType(CastExpr->getType()) == HBT_None) {
      ReportUnsupportedTypeCast(CastExpr, SM);
      return;
    }
    DoVisit(CastExpr->getSubExpr(), CastExpr);
  }

  void VisitCXXConstructExpr(CXXConstructExpr *ConstructExpr) {
    if (Builtins.identifyBuiltinType(ConstructExpr->getType()) == HBT_None) {
      ReportUnsupportedTypeConstruct(ConstructExpr, SM);
      return;
    }
    for (Expr *Arg : ConstructExpr->arguments())
      DoVisit(Arg, ConstructExpr);
  }

  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *CallExpr) {
    for (Expr *Arg : CallExpr->arguments())
      DoVisit(Arg, CallExpr);

    if (CallExpr->getNumArgs() == 2) {
      const bool LHSInterpolated = GetInterpolated(CallExpr->getArg(0));
      const bool RHSInterpolated = GetInterpolated(CallExpr->getArg(1));
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
            CallExpr->setHoshStage(Target);
          break;
        default:
          CallExpr->setHoshStage(Target);
          break;
        }
      }
    }

    for (Expr *Arg : CallExpr->arguments())
      AddPossiblePromotion(Arg, CallExpr);
  }

  void VisitDeclRefExpr(DeclRefExpr *DeclRef) {
    if (auto *VD = dyn_cast<VarDecl>(DeclRef->getDecl())) {
      if (!InMemberExpr && Builtins.identifyBuiltinType(VD->getType()) == HBT_None) {
        ReportUnsupportedTypeReference(DeclRef, SM);
        return;
      }
      if (auto *PVD = dyn_cast<ParmVarDecl>(DeclRef->getDecl())) {
        DeclRef->setHoshStage(DetermineParmVarStage(PVD));
        return;
      }
      llvm::outs() << "Attempting to find " << VD->getName() << " before ";
      LastCompoundChild->printPretty(llvm::outs(), nullptr, TmpContext->getPrintingPolicy());
      llvm::outs() << '\n';
      llvm::outs().flush();
      auto [Assign, NextCompoundChild] = LastAssignmentFinder(SM).Find(VD, Body, LastCompoundChild);
      if (Assign) {
        llvm::outs() << "Found ";
        Assign->printPretty(llvm::outs(), nullptr, TmpContext->getPrintingPolicy());
        llvm::outs() << '\n';
        llvm::outs().flush();
        SaveAndRestore<Stmt*> SavedCompoundChild(LastCompoundChild, NextCompoundChild);
        SaveAndRestore<VarDecl*> SavedSelectedVarDecl(SelectedVarDecl, VD);
        DoVisit(Assign, DeclRef);
      } else {
        llvm::outs() << "Fail\n";
        llvm::outs().flush();
      }
    }
  }

  void VisitInitListExpr(InitListExpr *InitList) {
    for (Stmt *S : *InitList)
      DoVisit(S, InitList);
  }

  void VisitMemberExpr(MemberExpr *MemberExpr) {
    if (!InMemberExpr && Builtins.identifyBuiltinType(MemberExpr->getType()) == HBT_None) {
      ReportUnsupportedTypeReference(MemberExpr, SM);
      return;
    }
    SaveAndRestore<bool> SavedInMemberExpr(InMemberExpr, true);
    DoVisit(MemberExpr->getBase(), MemberExpr);
  }

  void VisitFloatingLiteral(FloatingLiteral *) {}

  void VisitIntegerLiteral(IntegerLiteral *) {}

  void Trace(Stmt *Assign, Stmt *B, Stmt *LCC, HoshStage T) {
    Body = B;
    LastCompoundChild = LCC;
    Target = T;
    Assign->setHoshStage(T);
    Visit(Assign);
  }
};

static const std::regex BadCharReg(R"([^_0-9a-zA-Z])");
static std::string SanitizedName(const char* Name) {
  std::string TmpName = std::regex_replace(Name, BadCharReg, "_");
  std::string SanitizedName;
  llvm::raw_string_ostream OS(SanitizedName);
  if (!TmpName.empty() && TmpName[0] >= '0' && TmpName[0] <= '9')
    OS << '_';
  OS << TmpName;
  return OS.str();
}

static std::string SanitizedNameOfLocation(SourceLocation Loc, const SourceManager& SM) {
  if (Loc.isFileID()) {
    PresumedLoc PLoc = SM.getPresumedLoc(Loc);
    std::string Ret = SanitizedName(PLoc.getFilename());
    llvm::raw_string_ostream OS(Ret);
    OS << Ret << "_L" << PLoc.getLine();
    return OS.str();
  } else {
    return "<<<BAD LOCATION>>>"s;
  }
}

class GenerateConsumer : public ASTConsumer, MatchFinder::MatchCallback {
  HoshBuiltins Builtins;
public:
  void run(const MatchFinder::MatchResult &Result) override {
    if (auto* Lambda = Result.Nodes.getNodeAs<LambdaExpr>("id")) {
      auto *CallOperator = Lambda->getCallOperator();
      Stmt *Body = CallOperator->getBody();
      for (const auto &Cap : Lambda->captures()) {
        if (Cap.capturesThis())
          continue;
        Cap.getCapturedVar()->print(llvm::outs(), Context.getPrintingPolicy());
        llvm::outs() << '\n';
      }

      TmpContext = &Context;
      StmtPromotions Promotions;

      for (int i = HoshVertexStage; i < HoshMaxStage; ++i) {
        for (ParmVarDecl *Param : CallOperator->parameters()) {
          if (DetermineParmVarStage(Param) != HoshStage(i))
            continue;
          auto [Assign, LastCompoundChild] = LastAssignmentFinder(SM).Find(Param, Body);
          if (Assign)
            TracingStmtPromoter(SM, Builtins, Promotions).Trace(Assign, Body, LastCompoundChild, HoshStage(i));
        }
      }

      Promotions.printPromotions(Context);

      std::array<CompoundStmt*, HoshMaxStage> StageBodies{};
      for (int i = HoshHostStage; i < HoshMaxStage; ++i) {
        SmallVector<Stmt*, 8> Stmts;
        for (auto *Stmt : dyn_cast<CompoundStmt>(Body)->body()) {
          if (auto *E = dyn_cast<Expr>(Stmt))
            if (auto *E2 = E->IgnoreParenImpCasts())
              Stmt = E2;
          if (Stmt->isInHoshStage(HoshStage(i)))
            Stmts.push_back(Stmt);
        }
        if (i != HoshHostStage && Stmts.empty())
          continue;

#if 0
        if (i == HoshHostStage) {
          DeclContext *LambdaDeclContext = Lambda->getType()->getAsCXXRecordDecl()->getDeclContext();
          CXXRecordDecl *RecordDecl = CXXRecordDecl::CreateLambda(Context, LambdaDeclContext, {}, {}, false, false, LCD_ByRef);
          CXXMethodDecl *LambdaCall = CXXMethodDecl::Create(Context, RecordDecl, {}, {Context.DeclarationNames.getCXXOperatorName(OO_Call), {}}, {}, {}, SC_None, false, CSK_unspecified, {});
          SmallVector<Stmt*, 8> LambdaStmts;

          std::string LocName;
          auto IncludePath = getIncludePathBeforeLambda(Lambda->getBeginLoc());
          if (!IncludePath.empty())
            LocName = SanitizedName(IncludePath.data());
          else
            LocName = SanitizedNameOfLocation(Lambda->getBeginLoc(), SM);
          CXXRecordDecl *BindingRecord = CXXRecordDecl::Create(Context, TTK_Class, LambdaCall, {}, {}, &Context.Idents.get(LocName));
          BindingRecord->startDefinition();
          FieldDecl *FD = FieldDecl::Create(Context, BindingRecord, {}, {}, &Context.Idents.get("test"),
            Builtins.getType(HBT_float3), {}, {}, false, ICIS_NoInit);
          FD->setAccess(AS_public);
          BindingRecord->addDecl(FD);
          BindingRecord->addDecl(AccessSpecDecl::Create(Context, AS_public, BindingRecord, {}, {}));
          CanQualType CDType = BindingRecord->getTypeForDecl()->getCanonicalTypeUnqualified();
          SmallVector<QualType, 8> ConstructorArgs;
          SmallVector<ParmVarDecl*, 8> ConstructorParms;
          SmallVector<Expr*, 8> ConstructorInstArgs;
          for (const auto &Cap : Lambda->captures()) {
            if (!Cap.capturesThis()) {
              ConstructorArgs.push_back(Cap.getCapturedVar()->getType());
              ConstructorParms.push_back(ParmVarDecl::Create(Context, BindingRecord, {}, {},
                Cap.getCapturedVar()->getIdentifier(), ConstructorArgs.back(), {}, SC_None, nullptr));
              ConstructorInstArgs.push_back(DeclRefExpr::Create(Context, {}, {}, Cap.getCapturedVar(), true, SourceLocation{},
                Cap.getCapturedVar()->getType().getNonReferenceType(), VK_LValue));
            }
          }
          QualType FuncType = Context.getFunctionType(CDType, ConstructorArgs, {});
          CXXConstructorDecl *CD = CXXConstructorDecl::Create(Context, BindingRecord, {},
            {Context.DeclarationNames.getCXXConstructorName(CDType), {}}, FuncType, {}, {}, false, false, CSK_unspecified);
          CD->setParams(ConstructorParms);
          CD->setAccess(AS_public);
          CD->setBody(CompoundStmt::CreateEmpty(Context, 0));
          BindingRecord->addDecl(CD);
          BindingRecord->completeDefinition();
          VarDecl *BindingVar = VarDecl::Create(Context, RecordDecl, {}, {}, &Context.Idents.get("binding"), CDType, {}, SC_None);
          BindingVar->setInit(CXXConstructExpr::Create(Context, CDType, {}, CD, false, ConstructorInstArgs, false, true, false, false,
                                                       CXXConstructExpr::CK_Complete, {}));
          BindingVar->setInitStyle(VarDecl::ListInit);
          std::array<Decl*, 2> BindingDecls{BindingRecord, BindingVar};
          LambdaStmts.push_back(new (Context) DeclStmt(DeclGroupRef::Create(Context, BindingDecls.data(), 2), {}, {}));
          LambdaStmts.push_back(ReturnStmt::Create(Context, {},
            DeclRefExpr::Create(Context, {}, {}, BindingVar, false, SourceLocation{}, BindingVar->getType(), VK_XValue), nullptr));

          CompoundStmt *LambdaBody = CompoundStmt::Create(Context, LambdaStmts, {}, {});
          LambdaCall->setBody(LambdaBody);

          RecordDecl->addDecl(LambdaCall);
          LambdaExpr *LambdaExpr = LambdaExpr::Create(Context, RecordDecl, {}, LCD_ByRef, {}, {}, false, false, {}, {}, false);

          llvm::outs() << "Resulting Lambda\n";
          LambdaExpr->printPretty(llvm::outs(), nullptr, Context.getPrintingPolicy());
          llvm::outs() << "();\n[[hosh::generator_lambda]]\n";
          llvm::outs().flush();
          break;
        }
#endif

        StageBodies[i] = CompoundStmt::Create(Context, Stmts, {}, {});
        llvm::outs() << "Statements for " << HoshStageToString(HoshStage(i)) << '\n';
        StageBodies[i]->printPretty(llvm::outs(), nullptr, Context.getPrintingPolicy());
        llvm::outs() << '\n';
      }


      ASTDumper P(llvm::errs(), nullptr, &SM, /*ShowColors*/true);
      P.Visit(Body);
    }
  }

  ASTContext &Context;
  Preprocessor &PP;
  SourceManager &SM;
  std::map<SourceLocation, std::string> SeenIncludes;

  GenerateConsumer(ASTContext &Context, Preprocessor &PP, SourceManager &SM) : Context(Context), PP(PP), SM(SM) {}

  void HandleTranslationUnit(ASTContext& Context) override {
    if (Context.getDiagnostics().hasErrorOccurred())
      return;

    Builtins.findBuiltinDecls(Context.getTranslationUnitDecl(), SM);
    if (Context.getDiagnostics().hasErrorOccurred()) {
      ASTDumper P(llvm::errs(), nullptr, &SM, /*ShowColors*/true);
      P.Visit(Context.getTranslationUnitDecl());
      return;
    }

    MatchFinder Finder;
    Finder.addMatcher(
      attributedStmt(
        allOf(
          hasStmtAttr(attr::HoshGeneratorLambda),
          hasDescendant(lambdaExpr(stmt().bind("id"))))), this);
    Finder.matchAST(Context);
  }

  StringRef getIncludePathBeforeLambda(SourceLocation LambdaLoc) const {
    PresumedLoc PLoc = SM.getPresumedLoc(LambdaLoc);
    for (const auto& I : SeenIncludes) {
      PresumedLoc IPLoc = SM.getPresumedLoc(I.first);
      if (IPLoc.getFileID() != PLoc.getFileID())
        continue;
      if (IPLoc.getLine() == PLoc.getLine() - 1)
        return I.second;
    }
    return {};
  }

  void registerHoshInclude(SourceLocation HashLoc, StringRef RelativePath) {
    SeenIncludes[HashLoc] = RelativePath;
  }

  class PPCallbacks : public clang::PPCallbacks {
    GenerateConsumer &Consumer;
    FileManager &FM;
    SourceManager &SM;
  public:
    explicit PPCallbacks(GenerateConsumer &Consumer, FileManager &FM, SourceManager &SM)
    : Consumer(Consumer), FM(FM), SM(SM) {}
    bool FileNotFound(StringRef FileName,
                      SmallVectorImpl<char> &RecoveryPath) override {
      if (FileName.endswith_lower(llvm::StringLiteral(".hosh"))) {
        SmallString<1024> VirtualFilePath(llvm::StringLiteral("./"));
        VirtualFilePath += FileName;
        FM.getVirtualFile(VirtualFilePath, StubInclude.size(), std::time(nullptr));
        RecoveryPath.push_back('.');
        return true;
      }
      return false;
    }
    void InclusionDirective(SourceLocation HashLoc,
                            const Token &IncludeTok,
                            StringRef FileName,
                            bool IsAngled,
                            CharSourceRange FilenameRange,
                            const FileEntry *File,
                            StringRef SearchPath,
                            StringRef RelativePath,
                            const clang::Module *Imported,
                            SrcMgr::CharacteristicKind FileType) override {
      if (FileName.endswith_lower(llvm::StringLiteral(".hosh"))) {
        assert(File && "File must exist at this point");
        SM.overrideFileContents(File, llvm::MemoryBuffer::getMemBuffer(StubInclude));
        Consumer.registerHoshInclude(HashLoc, RelativePath);
      }
    }
  };
};

class GenerateAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override {
    auto Consumer = std::make_unique<GenerateConsumer>(CI.getASTContext(), CI.getPreprocessor(), CI.getSourceManager());
    CI.getPreprocessor().addPPCallbacks(std::make_unique<GenerateConsumer::PPCallbacks>(
                                        *Consumer, CI.getFileManager(), CI.getSourceManager()));
    return Consumer;
  }
};

#ifndef INSTALL_PREFIX
#define INSTALL_PREFIX / usr / local
#endif
#define XSTR(s) STR(s)
#define STR(s) #s

int DoGenerateTest() {
  std::vector<std::string> args = {
    "clang-tool",
#ifdef __linux__
    "--gcc-toolchain=/usr",
#endif
    "-fsyntax-only",
    "-std=c++17",
    "-D__hosh__=1",
    "-Wno-expansion-to-defined",
    "-Wno-nullability-completeness",
    "-Wno-unused-value",
    "-Werror=shadow-field",
    "-I" XSTR(INSTALL_PREFIX) "/lib/clang/" CLANG_VERSION_STRING "/include",
    "-I" XSTR(INSTALL_PREFIX) "/include",
    "test-input.cpp",
  };
  llvm::IntrusiveRefCntPtr<FileManager> fman(new FileManager(FileSystemOptions()));
#if CLANG_VERSION_MAJOR >= 10
  tooling::ToolInvocation TI(std::move(args), std::make_unique<GenerateAction>(), fman.get());
#else
  tooling::ToolInvocation TI(std::move(args), new GenerateAction, fman.get());
#endif
  if (!TI.run())
    return 1;

  return 0;
}

}
