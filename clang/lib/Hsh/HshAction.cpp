//===--- HshAction.cpp - clang action for hshgen tool ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Value.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_carray_ostream.h"
#include "llvm/Support/raw_comment_ostream.h"
#include "llvm/Support/xxhash.h"

#include "clang/AST/ASTDumper.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/AST/QualTypeNames.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Analysis/AnalysisDeclContext.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Config/config.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Hsh/HshGenerator.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Parse/Parser.h"

#include "Builder.h"
#include "Builtins/Builtins.h"
#include "Builtins/NonConstExpr.h"
#include "Compilers/StageCompiler.h"
#include "Dumper.h"
#include "Partitioner.h"
#include "PrintingPolicies/ShaderPrintingPolicy.h"
#include "Reporter.h"

#define XSTR(X) #X
#define STR(X) XSTR(X)

namespace llvm {

template <> struct DenseMapInfo<APSInt> {
  static APSInt getEmptyKey() {
    return APSInt::get(DenseMapInfo<int64_t>::getEmptyKey());
  }

  static APSInt getTombstoneKey() {
    return APSInt::get(DenseMapInfo<int64_t>::getTombstoneKey());
  }

  static unsigned getHashValue(const APSInt &Val) {
    return DenseMapInfo<int64_t>::getHashValue(Val.getSExtValue());
  }

  static bool isEqual(const APSInt &LHS, const APSInt &RHS) {
    return APSInt::compareValues(LHS, RHS) == 0;
  }
};

template <> struct DenseMapInfo<clang::SourceLocation> {
  static clang::SourceLocation getEmptyKey() { return {}; }

  static clang::SourceLocation getTombstoneKey() {
    return clang::SourceLocation::getFromRawEncoding(~(0u));
  }

  static unsigned getHashValue(const clang::SourceLocation &Val) {
    return DenseMapInfo<unsigned>::getHashValue(Val.getRawEncoding());
  }

  static bool isEqual(const clang::SourceLocation &LHS,
                      const clang::SourceLocation &RHS) {
    return LHS == RHS;
  }
};

template <> struct DenseMapInfo<clang::hshgen::HshTarget> {
  static clang::hshgen::HshTarget getEmptyKey() {
    return clang::hshgen::HshTarget(DenseMapInfo<int>::getEmptyKey());
  }

  static clang::hshgen::HshTarget getTombstoneKey() {
    return clang::hshgen::HshTarget(DenseMapInfo<int>::getTombstoneKey());
  }

  static unsigned getHashValue(const clang::hshgen::HshTarget &Val) {
    return DenseMapInfo<int>::getHashValue(int(Val));
  }

  static bool isEqual(const clang::hshgen::HshTarget &LHS,
                      const clang::hshgen::HshTarget &RHS) {
    return LHS == RHS;
  }
};

} // end namespace llvm

namespace {

using namespace llvm;
using namespace clang;
using namespace clang::hshgen;
using namespace std::literals;

template <typename T, typename TIter = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T &&iterable) {
  struct iterator {
    size_t i;
    TIter iter;
    bool operator!=(const iterator &other) const { return iter != other.iter; }
    void operator++() {
      ++i;
      ++iter;
    }
    auto operator*() const { return std::tie(i, *iter); }
  };
  struct iterable_wrapper {
    T iterable;
    auto begin() { return iterator{0, std::begin(iterable)}; }
    auto end() { return iterator{0, std::end(iterable)}; }
  };
  return iterable_wrapper{std::forward<T>(iterable)};
}

class GenerateConsumer : public ASTConsumer {
  HshBuiltins Builtins;
  CompilerInstance &CI;
  ASTContext &Context;
  HostPrintingPolicy HostPolicy;
  AnalysisDeclContextManager AnalysisMgr;
  Preprocessor &PP;
  ArrayRef<HshTarget> Targets;
  bool DebugInfo, SourceDump;
  SmallString<256> ProfilePath;
  std::unique_ptr<raw_pwrite_stream> OS;
  llvm::DenseSet<uint64_t> SeenHashes;
  llvm::DenseSet<uint64_t> SeenSamplerHashes;
  std::string AnonNSString;
  raw_string_ostream AnonOS{AnonNSString};
  std::string CoordinatorSpecString;
  raw_string_ostream CoordinatorSpecOS{CoordinatorSpecString};
  std::string HighCoordinatorSpecString;
  raw_string_ostream HighCoordinatorSpecOS{HighCoordinatorSpecString};
  Optional<std::pair<SourceLocation, std::string>> HeadInclude;
  struct HshExpansion {
    SourceRange Range;
    SmallString<32> Name;
    clang::Expr *Construct;
  };
  DenseMap<SourceLocation, HshExpansion> SeenHshExpansions;

  std::array<std::unique_ptr<StageCompiler>, HT_MAX> Compilers;
  StageCompiler &getCompiler(HshTarget Target) {
    auto &Compiler = Compilers[Target];
    if (!Compiler)
      Compiler =
          MakeCompiler(Target, DebugInfo, CI.getHeaderSearchOpts().ResourceDir,
                       CI, Context.getDiagnostics(), Builtins);
    return *Compiler;
  }

  bool NeedsCoordinatorComma = false;
  void addCoordinatorType(QualType T) {
    if (NeedsCoordinatorComma)
      CoordinatorSpecOS << ",\n";
    else
      NeedsCoordinatorComma = true;
    T.print(CoordinatorSpecOS, HostPolicy);
  }

  bool NeedsHighCoordinatorComma = false;
  void addHighCoordinatorType(QualType T) {
    if (NeedsHighCoordinatorComma)
      HighCoordinatorSpecOS << ",\n";
    else
      NeedsHighCoordinatorComma = true;
    T.print(HighCoordinatorSpecOS, HostPolicy);
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

  class PipelineDerivativeSearch
      : public RecursiveASTVisitor<PipelineDerivativeSearch> {
    using FuncTp = llvm::unique_function<bool(NamedDecl *)>;
    ASTContext &Context;
    HshBuiltins &Builtins;
    FuncTp Func;

  public:
    explicit PipelineDerivativeSearch(ASTContext &Context,
                                      HshBuiltins &Builtins)
        : Context(Context), Builtins(Builtins) {}

    bool VisitCXXRecordDecl(CXXRecordDecl *Decl) {
      if (!Decl->isThisDeclarationADefinition() ||
          Decl->getDescribedClassTemplate() ||
          isa<ClassTemplateSpecializationDecl>(Decl))
        return true;
      if (Builtins.isDerivedFromPipelineDecl(Decl))
        return Func(Decl);
      return true;
    }

    bool VisitClassTemplateDecl(ClassTemplateDecl *Decl) {
      if (!Decl->isThisDeclarationADefinition())
        return true;
      if (Builtins.isDerivedFromPipelineDecl(Decl->getTemplatedDecl()))
        return Func(Decl);
      return true;
    }

    void search(llvm::unique_function<bool(NamedDecl *)> f) {
      Func = std::move(f);
      TraverseAST(Context);
    }
  };

public:
  explicit GenerateConsumer(CompilerInstance &CI, ArrayRef<HshTarget> Targets,
                            bool DebugInfo, bool SourceDump,
                            StringRef ProfilePath)
      : CI(CI), Context(CI.getASTContext()),
        HostPolicy(Context.getPrintingPolicy()), AnalysisMgr(Context),
        PP(CI.getPreprocessor()), Targets(Targets), DebugInfo(DebugInfo),
        SourceDump(SourceDump), ProfilePath(ProfilePath) {
    AnalysisMgr.getCFGBuildOptions().OmitLogicalBinaryOperators = true;
  }

  static std::string MakeHashString(uint64_t Hash) {
    std::string HashStr;
    raw_string_ostream HexOS(HashStr);
    llvm::write_hex(HexOS, Hash, HexPrintStyle::Upper, {16});
    return HashStr;
  }

  std::string makeSectionAttributeString(HshTarget Target,
                                         bool &BeforeType) const {
    std::string SectionString;
    raw_string_ostream SectionStringOS{SectionString};

    BeforeType = false;
    switch (CI.getTarget().getTriple().getObjectFormat()) {
    case llvm::Triple::ELF:
      SectionStringOS << "__attribute__((section(\".hsh" << int(Target)
                      << "\")))";
      break;
    case llvm::Triple::MachO:
      SectionStringOS << "__attribute__((section(\"__TEXT,__hsh" << int(Target)
                      << "\")))";
      break;
    case llvm::Triple::COFF:
      SectionStringOS << "__declspec(allocate(\".hsh" << int(Target) << "\"))";
      BeforeType = true;
      break;
    default:
      assert(false && "Unsupported object format");
      break;
    }

    return SectionStringOS.str();
  }

  bool handlePipelineDerivative(NamedDecl *Decl) {
    auto &Diags = Context.getDiagnostics();
    auto *CD = dyn_cast<CXXRecordDecl>(Decl);
    auto *CTD = dyn_cast<ClassTemplateDecl>(Decl);
    assert((CTD || CD) && "Bad decl type");
    if (CTD)
      CD = CTD->getTemplatedDecl();

    auto *RedeclContext = Decl->getDeclContext()->getRedeclContext();
    if (!RedeclContext->isFileContext()) {
      Diags.Report(
          Decl->getBeginLoc(),
          Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                "hsh::pipeline derivatives must be declared in "
                                "file or namespace scope"));
      return false;
    }

    SmallString<32> BindingName("hshbinding_");
    BindingName += Decl->getName();

    CXXRecordDecl *BindingCD = nullptr;
    ClassTemplateDecl *BindingCTD = nullptr;
    if (CTD) {
      BindingCTD = Builtins.makeBindingDerivative(Context, CI.getSema(), CTD,
                                                  BindingName);
      BindingCTD->print(AnonOS, HostPolicy);
      AnonOS << ";\n";
    } else {
      BindingCD = Builtins.makeBindingDerivative(Context, CD, BindingName);
    }

    if (Context.getDiagnostics().hasErrorOccurred())
      return false;

    auto ProcessSpecialization = [&](CXXRecordDecl *Specialization) {
      QualType T{Specialization->getTypeForDecl(), 0};

      // HshBuiltins::makeBindingDerivative sets this
      CXXRecordDecl *PipelineSource = Specialization->HshSourceRecord;
      assert(PipelineSource);

      // Validate constructor of this specialization source
      if (!PipelineSource->hasUserDeclaredConstructor()) {
        // Consider lack of user-defined constructor as an abstract pipeline
        return;
      }
      CXXConstructorDecl *PrevCtor = nullptr;
      bool MultiCtorReport = false;
      for (auto *Ctor : PipelineSource->ctors()) {
        if (Ctor->isCopyOrMoveConstructor())
          continue;
        if (!MultiCtorReport && PrevCtor) {
          Diags.Report(
              PrevCtor->getBeginLoc(),
              Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                    "hsh::pipeline derivatives may not "
                                    "have multiple constructors"));
          MultiCtorReport = true;
        }
        if (MultiCtorReport) {
          Diags.Report(Ctor->getBeginLoc(),
                       Diags.getCustomDiagID(DiagnosticsEngine::Note,
                                             "additional constructor here"));
        }
        PrevCtor = Ctor;
      }
      if (MultiCtorReport)
        return;

      // Extract template arguments for constructing color attachments and
      // logical pipeline constants
      auto *PipelineSpec =
          Builtins.getDerivedPipelineSpecialization(PipelineSource);
      const auto &PipelineAttributes = Builtins.getPipelineAttributes();
      bool HasDualSource = false;
      auto ColorAttachmentArgs = PipelineAttributes.getColorAttachmentArgs(
          PipelineSpec, HasDualSource);
      auto PipelineArgs =
          PipelineAttributes.getPipelineArgs(Context, PipelineSpec);
      auto InShaderPipelineArgs =
          PipelineAttributes.getInShaderPipelineArgs(Context, PipelineSpec);
      bool IsDirectRender = PipelineAttributes.isDirectRender(PipelineSpec);

      if (PipelineAttributes.isHighPriority(PipelineSpec))
        addHighCoordinatorType(T);
      else
        addCoordinatorType(T);

      Builder Builder(Context, Builtins, Specialization,
                      ColorAttachmentArgs.size(), HasDualSource);

      CXXConstructorDecl *Constructor = nullptr;
      for (auto *Ctor : PipelineSource->ctors()) {
        if (Ctor->hasBody()) {
          Constructor = Ctor;
          break;
        }
      }
      assert(Constructor);

#if ENABLE_DUMP
      ASTDumper Dumper(llvm::errs(), nullptr, &Context.getSourceManager());
      Dumper.Visit(Constructor->getBody());
      llvm::errs() << '\n';
#endif

      auto *CallCtx = AnalysisMgr.getContext(Constructor);
#if ENABLE_DUMP
      CallCtx->dumpCFG(true);
#endif

      // Prepare for partitioning
      Builder.prepare(Constructor);

      // GO!
      Partitioner(Context, Builtins).run(*CallCtx, Builder);
      if (Context.getDiagnostics().hasErrorOccurred())
        return;

      // Finalize expressions and add host to stage records
      Builder.finalizeResults(Constructor);

      // Dump sources directly for -source-dump
      if (SourceDump) {
        for (auto Target : Targets) {
          auto Policy = MakePrintingPolicy(Builtins, Context, Target,
                                           InShaderPipelineArgs);
          auto Sources = Builder.printResults(*Policy);
          for (auto &S : Sources) {
            if (!S.empty())
              *OS << S;
          }
        }
        return;
      }

      // Set public access
      Specialization->addDecl(
          AccessSpecDecl::Create(Context, AS_public, Specialization, {}, {}));

      // Make _rebind
      {
        SmallVector<QualType, 16> RebindArgs;
        SmallVector<ParmVarDecl *, 16> RebindParms;
        SmallVector<QualType, 16> RebindCallArgs;
        SmallVector<Expr *, 32> RebindCallExprs;
        RebindArgs.reserve(Constructor->getNumParams() + 1);
        RebindParms.reserve(Constructor->getNumParams() + 1);
        RebindCallArgs.reserve(Constructor->getNumParams() +
                               Builder.getNumSamplerBindings());
        RebindCallExprs.reserve(RebindCallArgs.size());

        RebindArgs.push_back(TypeName::getFullyQualifiedType(
            Builtins.getBindingRefType(Context), Context));
        RebindParms.push_back(ParmVarDecl::Create(
            Context, Specialization, {}, {}, &Context.Idents.get("__binding"),
            RebindArgs.back(), {}, SC_None, nullptr));
        for (const auto *Param : Constructor->parameters()) {
          RebindArgs.push_back(
              TypeName::getFullyQualifiedType(Param->getType(), Context));
          RebindParms.push_back(ParmVarDecl::Create(
              Context, Specialization, {}, {}, Param->getIdentifier(),
              RebindArgs.back(), {}, SC_None, nullptr));
          RebindCallArgs.push_back(RebindArgs.back());
          RebindCallExprs.push_back(DeclRefExpr::Create(
              Context, {}, {}, RebindParms.back(), false, SourceLocation{},
              RebindCallArgs.back(), VK_XValue));
        }

        for (const auto &SamplerBinding : Builder.getSamplerBindings()) {
          RebindCallExprs.push_back(Builtins.makeSamplerBinding(
              Context, SamplerBinding.TextureDecl, SamplerBinding.RecordIdx,
              SamplerBinding.TextureIdx));
          RebindCallArgs.push_back(TypeName::getFullyQualifiedType(
              RebindCallExprs.back()->getType(), Context));
        }

        CanQualType CDType =
            Specialization->getTypeForDecl()->getCanonicalTypeUnqualified();

        CXXMethodDecl *RebindMethod = CXXMethodDecl::Create(
            Context, Specialization, {},
            {Context.DeclarationNames.getIdentifier(
                 &Context.Idents.get("_rebind")),
             {}},
            Context.getFunctionType(
                Context.VoidTy, RebindArgs,
                FunctionProtoType::ExtProtoInfo().withExceptionSpec(
                    EST_BasicNoexcept)),
            {}, SC_Static, false, CSK_unspecified, {});
        RebindMethod->setParams(RebindParms);
        RebindMethod->setAccess(AS_public);
        RebindMethod->setBody(CompoundStmt::Create(
            Context,
            Builtins.makeSpecializedRebindCall(Context, CDType, RebindParms[0],
                                               RebindCallArgs, RebindCallExprs),
            {}, {}));
        Specialization->addDecl(RebindMethod);
      }

      // Add per-target shader data vars
      for (auto Target : Targets) {
        Specialization->addDecl(Builtins.getConstDataVar(
            Context, Specialization, Target, Builder.getNumStages(),
            Builder.getNumBindings(), Builder.getNumAttributes(),
            Builder.getNumSamplers(), ColorAttachmentArgs.size()));
        Specialization->addDecl(Builtins.getDataVar(
            Context, Specialization, Target, Builder.getNumStages(),
            Builder.getNumSamplers()));
      }

      Specialization->completeDefinition();

      SmallVector<uint64_t, 8> SamplerHashes;
      DenseMap<HshTarget, StageBinaries> BinaryMap;
      BinaryMap.reserve(Targets.size());

      // Emit shader record while interjecting with data initializers
      HostPolicy.setVarInitPrint([&](VarDecl *D, raw_ostream &InitOS) {
        if (D->InitHshTarget == -1)
          return false;
        auto Target = HshTarget(D->InitHshTarget);

        bool AttributeBeforeType = false;
        std::string SectionAttributeString =
            makeSectionAttributeString(Target, AttributeBeforeType);

        auto Policy =
            MakePrintingPolicy(Builtins, Context, Target, InShaderPipelineArgs);
        auto Sources = Builder.printResults(*Policy);
        auto &Compiler = getCompiler(Target);
        if (Context.getDiagnostics().hasErrorOccurred())
          return true;
        auto &Binaries =
            BinaryMap.insert(std::make_pair(Target, Compiler.compile(Sources)))
                .first->second;
        auto SourceIt = Sources.begin();
        int StageIt = HshVertexStage;

        InitOS << "{\n    {\n";

        for (auto &[Data, Hash] : Binaries) {
          auto &Source = *SourceIt++;
          auto Stage = HshStage(StageIt++);
          if (Data.empty())
            continue;
          InitOS << "      hsh::detail::ShaderCode<";
          Builtins.printTargetEnumString(InitOS, HostPolicy, Target);
          InitOS << ">{";
          Builtins.printStageEnumString(InitOS, HostPolicy, Stage);
          std::string ControlHashStr;
          if (Target == HT_DEKO3D) {
            /* Additional shader parameters for deko */
            auto ControlHash =
                xxHash64(ArrayRef<uint8_t>{Data.data() + 24, 64});
            ControlHashStr = MakeHashString(ControlHash);
            InitOS << ", _dekoc_" << ControlHashStr;
          }
          auto HashStr = MakeHashString(Hash);
          InitOS << ", {_hshs_" << HashStr << ", 0x" << HashStr << "}},\n";
          if (!SeenHashes.insert(Hash).second)
            continue;
          {
            raw_comment_ostream CommentOut(*OS);
            CommentOut << "// " << HshStageToString(Stage)
                       << " source targeting " << HshTargetToString(Target)
                       << "\n\n";
            CommentOut << Source;
          }
          *OS << "inline ";
          if (Target == HT_VULKAN_SPIRV) {
            raw_carray32_ostream DataOut(*OS, "_hshs_"s + HashStr,
                                         SectionAttributeString,
                                         AttributeBeforeType);
            DataOut.write((const uint32_t *)Data.data(), Data.size() / 4);
          } else if (Target == HT_DEKO3D) {
            /* Dksh headers are packed together by the linker */
            {
              raw_carray_ostream ControlOut(
                  *OS, "_dekoc_"s + ControlHashStr,
                  "__attribute__((section(\".hsh11\"), aligned(64)))");
              ControlOut.write((const char *)Data.data() + 24, 64);
            }
            {
              *OS << "\ninline ";
              raw_carray_ostream DataOut(*OS, "_hshs_"s + HashStr,
                                         "__attribute__((section(\".hsh10\"), "
                                         "aligned(DK_SHADER_CODE_ALIGNMENT)))");
              DataOut.write((const char *)Data.data() + 256, Data.size() - 256);
            }
          } else {
            raw_carray_ostream DataOut(*OS, "_hshs_"s + HashStr,
                                       SectionAttributeString,
                                       AttributeBeforeType);
            DataOut.write((const char *)Data.data(), Data.size());
          }
          *OS << "\ninline hsh::detail::ShaderObject<";
          Builtins.printTargetEnumString(*OS, HostPolicy, Target);
          *OS << "> _hsho_" << HashStr << ";\n\n";
        }

        InitOS << "    },\n    {\n";

        for (const auto &Binding : Builder.getBindings()) {
          InitOS << "      hsh::detail::VertexBinding{" << Binding.Binding
                 << ", " << Binding.Stride << ", ";
          Builtins.printInputRateEnumString(InitOS, HostPolicy,
                                            Binding.InputRate);
          InitOS << "},\n";
        }

        InitOS << "    },\n    {\n";

        for (const auto &Attribute : Builder.getAttributes()) {
          InitOS << "      hsh::detail::VertexAttribute{" << Attribute.Binding
                 << ", ";
          Builtins.printFormatEnumString(InitOS, HostPolicy, Attribute.Format);
          InitOS << ", " << Attribute.Offset << "},\n";
        }

        InitOS << "    },\n    {\n";

        if (SamplerHashes.empty()) {
          SamplerHashes.reserve(Builder.getNumSamplers());
          for (const auto &Sampler : Builder.getSamplers()) {
            InitOS << "      hsh::sampler{";
            std::string SamplerParams;
            raw_string_ostream SPO(SamplerParams);
            unsigned FieldIdx = 0;
            CommaArgPrinter ArgPrinter(SPO);
            for (auto *Field : Builtins.getSamplerRecordDecl()->fields()) {
              ArgPrinter.addArg();
              const auto &FieldVal = Sampler.Config.getStructField(FieldIdx++);
              if (FieldVal.isInt()) {
                TemplateArgument(Context, FieldVal.getInt(), Field->getType())
                    .print(HostPolicy, SPO);
              } else if (FieldVal.isFloat()) {
                SmallVector<char, 16> Buffer;
                FieldVal.getFloat().toString(Buffer);
                SPO << Buffer;
                if (StringRef(Buffer.data(), Buffer.size())
                        .find_first_not_of("-0123456789") == StringRef::npos)
                  SPO << '.';
                SPO << 'F';
              }
            }
            SamplerHashes.push_back(xxHash64(SPO.str()));
            InitOS << SPO.str() << "},\n";
          }
        }

        InitOS << "    },\n    {\n";

        auto PrintArguments = [&](const auto &Args) {
          CommaArgPrinter ArgPrinter(InitOS);
          for (const auto &Arg : Args) {
            ArgPrinter.addArg();
            if (Arg.getKind() == TemplateArgument::Integral &&
                Builtins.identifyBuiltinType(Arg.getIntegralType()) ==
                    HBT_ColorComponentFlags) {
              InitOS << "hsh::ColorComponentFlags(";
              Builtins.printColorComponentFlagExpr(
                  InitOS, HostPolicy,
                  ColorComponentFlags(Arg.getAsIntegral().getZExtValue()));
              InitOS << ")";
            } else {
              Arg.print(HostPolicy, InitOS);
            }
          }
        };

        for (const auto &Attachment : ColorAttachmentArgs) {
          InitOS << "      hsh::detail::ColorAttachment{";
          PrintArguments(Attachment);
          InitOS << "},\n";
        }

        InitOS << "    },\n";

        InitOS << "    hsh::detail::PipelineInfo{";
        PrintArguments(PipelineArgs);
        InitOS << ", " << (IsDirectRender ? "true" : "false");
        InitOS << "}\n";

        InitOS << "  }";
        return true;
      });
      Specialization->print(AnonOS, HostPolicy);
      HostPolicy.resetVarInitPrint();
      AnonOS << ";\n";

      // Emit shader data
      for (auto Target : Targets) {
        AnonOS << "hsh::detail::ShaderData<";
        Builtins.printTargetEnumString(AnonOS, HostPolicy, Target);
        AnonOS << ", " << Builder.getNumStages() << ", "
               << Builder.getNumSamplers() << "> ";
        T.print(AnonOS, HostPolicy);
        AnonOS << "::data_";
        Builtins.printTargetEnumName(AnonOS, Target);
        AnonOS << "{\n  {\n";

        for (auto &[Data, Hash] : BinaryMap[Target]) {
          if (Data.empty())
            continue;
          AnonOS << "    &_hsho_" << MakeHashString(Hash) << ",\n";
        }

        AnonOS << "  },\n  {\n";

        for (auto SamplerHash : SamplerHashes)
          AnonOS << "    &_hshsamp_" << MakeHashString(SamplerHash) << ",\n";

        AnonOS << "  }\n};\n";
      }

      for (auto SamplerHash : SamplerHashes) {
        if (SeenSamplerHashes.find(SamplerHash) != SeenSamplerHashes.end())
          continue;
        SeenSamplerHashes.insert(SamplerHash);
        for (auto Target : Targets) {
          *OS << "inline hsh::detail::SamplerObject<";
          Builtins.printTargetEnumString(*OS, HostPolicy, Target);
          *OS << "> _hshsamp_" << MakeHashString(SamplerHash) << ";\n";
        }
      }
      *OS << "\n";
    };
    if (BindingCTD) {
      for (auto *Specialization : BindingCTD->specializations())
        ProcessSpecialization(Specialization);
    } else {
      ProcessSpecialization(BindingCD);
    }

    AnonOS << "\n\n";
    return true;
  }

  void handleHshExpansion(const HshExpansion &Expansion,
                          const DenseSet<NamedDecl *> &SeenDecls,
                          StringRef AbsProfFile) {
    auto &Diags = Context.getDiagnostics();
    auto *Decl = Expansion.Construct->getType()->getAsCXXRecordDecl();
    NamedDecl *UseDecl = Decl;
    if (auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(Decl))
      UseDecl =
          CTSD->getSpecializedTemplateOrPartial().get<ClassTemplateDecl *>();
    if (SeenDecls.find(UseDecl) == SeenDecls.end()) {
      auto ReportInvalid = [&](auto *Construct, const TypeSourceInfo *TSI) {
        Diags.Report(
            Construct->getBeginLoc(),
            Diags.getCustomDiagID(
                DiagnosticsEngine::Error,
                "binding constructor does not construct a valid pipeline"))
            << TSI->getTypeLoc().getSourceRange();
      };
      if (auto *CTOE = dyn_cast<CXXTemporaryObjectExpr>(Expansion.Construct))
        ReportInvalid(CTOE, CTOE->getTypeSourceInfo());
      else if (auto *CUCE =
                   dyn_cast<CXXUnresolvedConstructExpr>(Expansion.Construct))
        ReportInvalid(CUCE, CUCE->getTypeSourceInfo());
      else if (auto *CFCE =
                   dyn_cast<CXXFunctionalCastExpr>(Expansion.Construct))
        ReportInvalid(CFCE, CFCE->getTypeInfoAsWritten());
      return;
    }
    SmallString<32> BindingName("hshbinding_");
    BindingName += UseDecl->getName();

    /* Determine if construction expression has all constant template parms */
    CommaArgPrinter MacroPrinter(*OS);
    *OS << "struct " << Expansion.Name
        << " {\n"
           "template <typename... Res>\n"
           "static void Bind";
    SmallVector<NonConstExpr, 8> NonConstExprs;
    if (CheckConstexprTemplateSpecialization(
            Context, Expansion.Construct->getType(), &NonConstExprs)) {
      *OS << "(hsh::binding &__binding, Res... Resources) noexcept {\n"
             "  ::"
          << BindingName
          << "::_rebind(__binding, Resources...);\n"
             "}\n};\n"
             "#define "
          << Expansion.Name << "(...) _bind<::" << Expansion.Name << ">(";
    } else {
      *OS << "(hsh::binding &__binding, ";
      TraverseNonConstExprs(NonConstExprs, [&](NonTypeTemplateParmDecl *NTTP) {
        NTTP->getType().print(*OS, HostPolicy, NTTP->getName());
        *OS << ", ";
      });
      *OS << "Res... Resources) noexcept {\n"
             "#if HSH_PROFILE_MODE\n";
      if (!AbsProfFile.empty()) {
        *OS << "hsh::profile_context::instance\n"
               ".get(\""
            << AbsProfFile << "\",\n\"" << Expansion.Name << "\", \"";
        auto PrintFullyQualType = [&](TypeDecl *Decl) {
          if (auto *TD = dyn_cast<TagDecl>(Decl))
            *OS << TD->getKindName() << ' ';
          if (auto *ET = TypeName::getFullyQualifiedType(
                             QualType{Decl->getTypeForDecl(), 0}, Context)
                             ->getAsAdjusted<ElaboratedType>()) {
            if (auto *NNS = ET->getQualifier())
              NNS->print(*OS, HostPolicy);
          }
          Decl->printName(*OS);
        };
        PrintFullyQualType(Decl);
        *OS << "\")\n.add(";
        CommaArgPrinter ArgPrinter(*OS);
        unsigned PushCount = 0;
        TraverseNonConstExprs(
            NonConstExprs,
            [&](NonTypeTemplateParmDecl *NTTP) {
              ArgPrinter.addArg();
              if (auto *EnumTp = NTTP->getType()->getAs<EnumType>()) {
                *OS << "hsh::profiler::cast{\"";
                PrintFullyQualType(EnumTp->getDecl());
                *OS << "\", ";
                NTTP->printName(*OS);
                *OS << '}';
              } else {
                NTTP->printName(*OS);
              }
            },
            [&](ClassTemplateSpecializationDecl *Spec) {
              ++PushCount;
              if (PushCount == 1)
                return;
              ArgPrinter.addArg() << "hsh::profiler::push{\"";
              auto *CTD = Spec->getSpecializedTemplateOrPartial()
                              .get<ClassTemplateDecl *>();
              PrintFullyQualType(CTD->getTemplatedDecl());
              *OS << "\"}";
            },
            [&]() {
              --PushCount;
              if (PushCount == 0)
                return;
              ArgPrinter.addArg() << "hsh::profiler::pop{}";
            },
            [&](const APSInt &Int, QualType IntType) {
              ArgPrinter.addArg() << '\"';
              TemplateArgument(Context, Int, IntType).print(HostPolicy, *OS);
              *OS << '\"';
            });
        *OS << ");\n";
      }
      *OS << "#else\n"
             "#ifndef _MSC_VER\n"
             "#pragma GCC diagnostic push\n"
             "#pragma GCC diagnostic ignored \"-Wcovered-switch-default\"\n"
             "#endif\n";

      struct SpecializationTree {
        static raw_ostream &indent(raw_ostream &OS, unsigned Indentation) {
          for (unsigned i = 0; i < Indentation; ++i)
            OS << "  ";
          return OS;
        }

        struct Node {
          DenseMap<APSInt, Node> Children;
          ClassTemplateSpecializationDecl *Leaf = nullptr;
          StringRef Name;
          bool IntCast = false;

          Node *getChild(const APSInt &Int) { return &Children[Int]; }

          void print(raw_ostream &OS, const PrintingPolicy &Policy,
                     StringRef BindingName, unsigned Indentation = 0) const {
            if (Leaf) {
              indent(OS, Indentation) << "::" << BindingName << '<';
              CommaArgPrinter ArgPrinter(OS);
              for (auto &Arg : Leaf->getTemplateArgs().asArray()) {
                Arg.print(Policy, ArgPrinter.addArg());
              }
              OS << ">::_rebind(__binding, Resources...); return;\n";
            } else if (!Name.empty()) {
              if (IntCast)
                indent(OS, Indentation) << "switch (int(" << Name << ")) {\n";
              else
                indent(OS, Indentation) << "switch (" << Name << ") {\n";
              for (auto &[Case, Child] : Children) {
                indent(OS, Indentation) << "case " << Case << ":\n";
                Child.print(OS, Policy, BindingName, Indentation + 1);
              }
              indent(OS, Indentation) << "default:\n";
              indent(OS, Indentation + 1)
                  << "assert(false && \"Unimplemented shader "
                     "specialization\"); return;\n";
              indent(OS, Indentation) << "}\n";
            } else {
              indent(OS, Indentation) << "assert(false && \"Unimplemented "
                                         "shader specialization\");\n";
            }
          }
        };
        Node Root;

        SpecializationTree(ASTContext &Context, ClassTemplateDecl *CTD,
                           ArrayRef<NonConstExpr> NonConstExprs) {
          for (auto *Specialization : CTD->specializations()) {
            if (!CheckConstexprTemplateSpecialization(Context, Specialization))
              continue;
            SpecializationTree::Node *SpecLeaf = &Root;
            TraverseNonConstExprs(NonConstExprs, Specialization,
                                  [&](NonTypeTemplateParmDecl *NTTP,
                                      const TemplateArgument &Arg) {
                                    if (NTTP->getType()->isBooleanType() ||
                                        NTTP->getType()->isEnumeralType())
                                      SpecLeaf->IntCast = true;
                                    SpecLeaf->Name = NTTP->getName();
                                    SpecLeaf =
                                        SpecLeaf->getChild(Arg.getAsIntegral());
                                  });
            SpecLeaf->Leaf = Specialization;
          }
        }

        void print(raw_ostream &OS, const PrintingPolicy &Policy,
                   StringRef BindingName, unsigned Indentation = 0) {
          Root.print(OS, Policy, BindingName, Indentation);
        }
      } SpecTree{Context, cast<ClassTemplateDecl>(UseDecl), NonConstExprs};
      SpecTree.print(*OS, HostPolicy, BindingName);

      *OS << "#ifndef _MSC_VER\n"
             "#pragma GCC diagnostic pop\n"
             "#endif\n"
             "#endif\n"
             "}\n};\n"
             "#define "
          << Expansion.Name << "(...) _bind<::" << Expansion.Name << ">(";
      for (auto &NCE : NonConstExprs) {
        if (NCE.getKind() != NonConstExpr::NonTypeParm)
          continue;
        NCE.getExpr()->printPretty(MacroPrinter.addArg(), nullptr, HostPolicy);
      }
    }
    auto PrintArgs = [&](auto *Construct) {
      for (auto *Arg : Construct->arguments()) {
        Arg->printPretty(MacroPrinter.addArg(), nullptr, HostPolicy);
      }
    };
    if (auto *CTOE = dyn_cast<CXXTemporaryObjectExpr>(Expansion.Construct))
      PrintArgs(CTOE);
    else if (auto *CUCE =
                 dyn_cast<CXXUnresolvedConstructExpr>(Expansion.Construct))
      PrintArgs(CUCE);
    else if (auto *CFCE =
                 dyn_cast<CXXFunctionalCastExpr>(Expansion.Construct)) {
      CFCE->getSubExpr()->printPretty(MacroPrinter.addArg(), nullptr,
                                      HostPolicy);
    }
    *OS << ")\n";
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    if (Diags.hasErrorOccurred())
      return;

    const unsigned IncludeDiagID =
        Diags.getCustomDiagID(DiagnosticsEngine::Error,
                              "hshhead include in must appear in global scope");
    if (!HeadInclude) {
      std::string Insertion;
      raw_string_ostream InsertionOS(Insertion);
      InsertionOS << "#include \""
                  << sys::path::filename(CI.getFrontendOpts().OutputFile)
                  << '\"';
      Diags.Report(IncludeDiagID) << FixItHint::CreateInsertion(
          Context.getSourceManager().getLocForStartOfFile(
              Context.getSourceManager().getMainFileID()),
          InsertionOS.str());
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

    if (!SourceDump) {
      SourceManager &SM = Context.getSourceManager();
      StringRef MainName = SM.getFileEntryForID(SM.getMainFileID())->getName();
      *OS << "/* Auto-generated hshhead for " << MainName
          << " */\n"
             "#include <hsh/hsh.h>\n\n";

      AnonOS << "namespace {\n\n";
      CoordinatorSpecOS << "hsh::detail::PipelineCoordinator<false,\n";
      HighCoordinatorSpecOS << "hsh::detail::PipelineCoordinator<true,\n";
    }

    /*
     * Process all hsh::pipeline derivatives
     */
    DenseSet<NamedDecl *> SeenDecls;
    PipelineDerivativeSearch(Context, Builtins)
        .search([this, &SeenDecls](NamedDecl *Decl) {
          if (handlePipelineDerivative(Decl))
            SeenDecls.insert(Decl);
          return true;
        });

    if (!SourceDump) {
      AnonOS << "}\n\n";

      *OS << "namespace hsh::detail {\n"
             "template struct "
             "ValidateBuiltTargets<std::integer_sequence<hsh::Target";
      for (auto Target : Targets) {
        *OS << ", ";
        Builtins.printTargetEnumString(*OS, HostPolicy, Target);
      }
      *OS << ">>;\n"
             "}\n\n";

      *OS << AnonOS.str();

      if (NeedsCoordinatorComma) {
        *OS << "template <> hsh::detail::PipelineCoordinatorNode<false,\n"
            << CoordinatorSpecOS.str() << ">::Impl>\n"
            << CoordinatorSpecOS.str() << ">::global{};\n\n";
      }

      if (NeedsHighCoordinatorComma) {
        *OS << "template <> hsh::detail::PipelineCoordinatorNode<true,\n"
            << HighCoordinatorSpecOS.str() << ">::Impl>\n"
            << HighCoordinatorSpecOS.str() << ">::global{};\n\n";
      }

      /*
       * Emit binding macro functions
       */
      *OS << "namespace {\n";
      for (auto &Exp : SeenHshExpansions)
        handleHshExpansion(Exp.second, SeenDecls, ProfilePath);
      *OS << "}\n";
    }

    // DxcLibrary::SharedInstance.reset();
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
        if (!SourceDump) {
          auto ExpectedName =
              sys::path::filename(CI.getFrontendOpts().OutputFile);
          if (ExpectedName != RelativePath) {
            std::string Replacement;
            raw_string_ostream ReplacementOS(Replacement);
            ReplacementOS << '\"' << ExpectedName << '\"';
            Diags.Report(
                FilenameRange.getBegin(),
                Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                      "hshhead include must match the output "
                                      "filename"))
                << FixItHint::CreateReplacement(FilenameRange,
                                                ReplacementOS.str());
            return;
          }
        }
        HeadInclude.emplace(HashLoc, RelativePath);
      }
    }
  }

  void registerHshExpansion(SourceRange Range, StringRef Name,
                            ExprResult Expr) {
    if (Context.getSourceManager().isWrittenInMainFile(Range.getBegin())) {
      DiagnosticsEngine &Diags = Context.getDiagnostics();
      for (auto &Exps : SeenHshExpansions) {
        if (Exps.second.Name == Name) {
          Diags.Report(
              Range.getBegin(),
              Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                    "hsh_* macro must be suffixed with "
                                    "identifier unique to the file"))
              << CharSourceRange(Range, false);
          Diags.Report(Exps.first, Diags.getCustomDiagID(
                                       DiagnosticsEngine::Note,
                                       "previous identifier usage is here"))
              << CharSourceRange(Exps.second.Range, false);
          return;
        }
      }
      if (!Expr.isUsable()) {
        Diags.Report(Range.getBegin(),
                     Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                           "hsh_* argument does not contain a "
                                           "usable construct expression"))
            << CharSourceRange(Range, false);
        return;
      }
      clang::Expr *Construct = dyn_cast<CXXTemporaryObjectExpr>(Expr.get());
      if (!Construct)
        Construct = dyn_cast<CXXUnresolvedConstructExpr>(Expr.get());
      if (!Construct)
        Construct = dyn_cast<CXXFunctionalCastExpr>(Expr.get());
      if (!Construct) {
        Diags.Report(Range.getBegin(),
                     Diags.getCustomDiagID(
                         DiagnosticsEngine::Error,
                         "expected construct expression as hsh_* argument"))
            << CharSourceRange(Range, false);
        return;
      }
      SeenHshExpansions[Range.getBegin()] = {Range, Name, Construct};
    }
  }

  class PPCallbacks : public clang::PPCallbacks {
    GenerateConsumer &Consumer;
    Preprocessor &PP;
    FileManager &FM;
    SourceManager &SM;

    static constexpr llvm::StringLiteral DummyInclude{"#include <hsh/hsh.h>\n"};

  public:
    explicit PPCallbacks(GenerateConsumer &Consumer, Preprocessor &PP,
                         FileManager &FM, SourceManager &SM)
        : Consumer(Consumer), PP(PP), FM(FM), SM(SM) {
      PP.setTokenWatcher([this](const clang::Token &T) { TokenWatcher(T); });
    }

    bool FileNotFound(StringRef FileName,
                      SmallVectorImpl<char> &RecoveryPath) override {
      if (FileName.endswith_lower(".hshhead")) {
        SmallString<1024> VirtualFilePath(".");
        sys::path::append(VirtualFilePath, FileName);
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
      if (FileName.endswith_lower(".hshhead")) {
        assert(File && "File must exist at this point");
        SM.overrideFileContents(File,
                                llvm::MemoryBuffer::getMemBuffer(DummyInclude));
        Consumer.registerHshHeadInclude(HashLoc, FilenameRange, RelativePath);
      }
    }

    SmallVector<llvm::unique_function<void(const clang::Token &)>, 4>
        TokenWatcherStack;
    void TokenWatcher(const clang::Token &T) {
      if (!TokenWatcherStack.empty())
        TokenWatcherStack.back()(T);
    }

    void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                      SourceRange Range, const MacroArgs *Args) override {
      if (MacroNameTok.is(tok::identifier)) {
        StringRef Name = MacroNameTok.getIdentifierInfo()->getName();
        if (Name.startswith("hsh_") && Args) {
          /*
           * Defer a side-channel parsing action once the preprocessor has
           * finished lexing the expression containing the hsh_ macro expansion.
           */
          auto SrcTokens = Args->getUnexpArguments();
          TokenWatcherStack.emplace_back(
              [this,
               PassTokens =
                   std::vector<Token>(SrcTokens.begin(), SrcTokens.end()),
               PassRange = Range, PassName = Name](const clang::Token &T) {
                /*
                 * popping token watcher will delete storage of captured values;
                 * move them here before calling it.
                 */
                PPCallbacks *CB = this;
                auto Tokens(std::move(PassTokens));
                SourceRange Range = PassRange;
                StringRef Name = PassName;
                auto *P =
                    static_cast<Parser *>(CB->PP.getCodeCompletionHandler());
                TokenWatcherStack.pop_back();
                CB->PP.EnterTokenStream(Tokens, false, false);
                {
                  /*
                   * Parse the contents of the hsh_ macro, which should result
                   * in a CXXTemporaryObjectExpr. The parsing checks are relaxed
                   * to permit non ICE expressions within template parameters.
                   */
                  Parser::RevertingTentativeParsingAction PA(*P, true);
                  P->getActions().InHshBindingMacro = true;
                  P->ConsumeToken();
                  ExprResult Res;
                  if (P->getCurToken().isOneOf(tok::identifier,
                                               tok::coloncolon))
                    Res = P->ParseExpression();
                  P->getActions().InHshBindingMacro = false;
                  CB->Consumer.registerHshExpansion(Range, Name, Res);
                }
                CB->PP.RemoveTopOfLexerStack();
              });
        }
      }
    }

    bool DidPostdefines = false;
    bool RequestPostdefines() override {
      if (DidPostdefines)
        return false;

      if (!Consumer.ProfilePath.empty()) {
        if (auto FE = FM.getFileRef(Consumer.ProfilePath)) {
          auto FID = SM.createFileID(*FE, {}, SrcMgr::C_User);
          PP.EnterSourceFile(FID, nullptr, {});
          PP.getDiagnostics().setSuppressFID(FID);
          DidPostdefines = true;
          return true;
        }
      }

      return false;
    }
  };
};

} // namespace

namespace clang::hshgen {

Dumper &dumper() {
  static Dumper GD;
  return GD;
}

std::unique_ptr<ASTConsumer>
GenerateAction::CreateASTConsumer(CompilerInstance &CI, StringRef InFile) {
  dumper().setPrintingPolicy(CI.getASTContext().getPrintingPolicy());
  auto Consumer = std::make_unique<GenerateConsumer>(CI, Targets, DebugInfo,
                                                     SourceDump, ProfilePath);
  CI.getPreprocessor().addPPCallbacks(
      std::make_unique<GenerateConsumer::PPCallbacks>(
          *Consumer, CI.getPreprocessor(), CI.getFileManager(),
          CI.getSourceManager()));
  return Consumer;
}

} // namespace clang::hshgen
