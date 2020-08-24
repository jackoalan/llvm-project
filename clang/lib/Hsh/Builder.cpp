//===--- Builder.cpp - hshgen partitioner actions -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Builder.h"
#include "PrintingPolicies/ShaderPrintingPolicy.h"
#include "Reporter.h"

#include "llvm/Support/SaveAndRestore.h"

#include "clang/AST/Attr.h"
#include "clang/AST/RecordLayout.h"

using namespace llvm;

namespace clang::hshgen {
namespace {
class UniformFieldValidator {
  ASTContext &Context;
  HshBuiltins &Builtins;
  DenseSet<const CXXRecordDecl *> Records;
  const FieldDecl *ArrayField = nullptr;
  bool InAlignedArray = false;

public:
  UniformFieldValidator(ASTContext &Context, HshBuiltins &Builtins)
      : Context(Context), Builtins(Builtins) {}

  void Visit(const CXXRecordDecl *Record) {
    if (!Records.insert(Record).second)
      return;

    auto &Diags = Context.getDiagnostics();

    for (auto *Field : Record->fields()) {
      auto HBT = Builtins.identifyBuiltinType(Field->getType());
      if (HBT == HBT_None) {
        if (Field->getType()->isArrayType()) {
          const auto *ArrayElemType =
              Field->getType()->getPointeeOrArrayElementType();
          if (!InAlignedArray &&
              Context.getTypeSizeInChars(ArrayElemType).getQuantity() % 16) {
            const auto *ActiveField = ArrayField ? ArrayField : Field;
            SourceRange Range = ActiveField->getSourceRange();
            if (auto *TSI = ActiveField->getTypeSourceInfo()) {
              if (auto TSTL =
                      TSI->getTypeLoc()
                          .getAsAdjusted<TemplateSpecializationTypeLoc>()) {
                Range = SourceRange(Range.getBegin(),
                                    TSTL.getLAngleLoc().getLocWithOffset(-1));
              } else if (auto TSTL = TSI->getTypeLoc()
                                         .getAsAdjusted<TypeSpecTypeLoc>()) {
                Range = TSTL.getLocalSourceRange();
              }
            }
            Diags.Report(Range.getBegin(),
                         Diags.getCustomDiagID(
                             DiagnosticsEngine::Error,
                             "use aligned array to ensure each element "
                             "is stored in a 16 byte register"))
                << Range
                << FixItHint::CreateReplacement(Range, "hsh::aligned_array");
          }

          if (auto *Child = ArrayElemType->getAsCXXRecordDecl()) {
            SaveAndRestore<const FieldDecl *> SavedArrayField(ArrayField,
                                                              nullptr);
            SaveAndRestore<bool> SavedAlignedArrayField(InAlignedArray, false);
            Visit(Child);
          }
        } else if (auto *Child = Field->getType()->getAsCXXRecordDecl()) {
          if (Builtins.isStdArrayType(Child)) {
            SaveAndRestore<const FieldDecl *> SavedArrayField(ArrayField,
                                                              Field);
            Visit(Child);
          } else if (Builtins.isAlignedArrayType(Child)) {
            SaveAndRestore<bool> SavedAlignedArrayField(InAlignedArray, true);
            Visit(Child);
          } else {
            Visit(Child);
          }
        }
      } else if (HshBuiltins::isMatrixType(HBT)) {
        if (const auto *AlignedType = Builtins.getAlignedAvailable(Field)) {
          SourceRange Range = Field->getSourceRange();
          if (auto *TSI = Field->getTypeSourceInfo())
            Range = TSI->getTypeLoc()
                        .getAsAdjusted<TypeSpecTypeLoc>()
                        .getLocalSourceRange();
          Diags.Report(
              Range.getBegin(),
              Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                    "use aligned matrix to ensure each column "
                                    "is stored in a 16 byte register"))
              << Range
              << FixItHint::CreateReplacement(Range, AlignedType->getName());
        }
      }
    }
  }
};
} // namespace

IdentifierInfo &Builder::getToIdent(ASTContext &Context, HshStage Stage) {
  std::string VarName;
  raw_string_ostream VNS(VarName);
  VNS << "_to_" << HshStageToString(Stage);
  return Context.Idents.get(VNS.str());
}

IdentifierInfo &Builder::getFromIdent(ASTContext &Context, HshStage Stage) {
  std::string VarName;
  raw_string_ostream VNS(VarName);
  VNS << "_from_" << HshStageToString(Stage);
  return Context.Idents.get(VNS.str());
}

IdentifierInfo &Builder::getFromToIdent(ASTContext &Context, HshStage From,
                                        HshStage To) {
  std::string RecordName;
  raw_string_ostream RNS(RecordName);
  RNS << HshStageToString(From) << "_to_" << HshStageToString(To);
  return Context.Idents.get(RNS.str());
}

MemberExpr *Builder::InterfaceRecord::createFieldReference(ASTContext &Context,
                                                           Expr *E, VarDecl *VD,
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

void Builder::InterfaceRecord::initializeRecord(ASTContext &Context,
                                                DeclContext *BindingDeclContext,
                                                HshStage S, HshStage D) {
  Record = CXXRecordDecl::Create(Context, TTK_Struct, BindingDeclContext, {},
                                 {}, &getFromToIdent(Context, S, D));
  Record->startDefinition();

  CanQualType CDType = Record->getTypeForDecl()->getCanonicalTypeUnqualified();

  VarDecl *PVD =
      VarDecl::Create(Context, BindingDeclContext, {}, {},
                      &getToIdent(Context, D), CDType, nullptr, SC_None);
  Producer = PVD;

  VarDecl *CVD =
      VarDecl::Create(Context, BindingDeclContext, {}, {},
                      &getFromIdent(Context, S), CDType, nullptr, SC_None);
  Consumer = CVD;

  SStage = S;
  DStage = D;
}

FieldDecl *Builder::InterfaceRecord::getFieldForExpr(ASTContext &Context,
                                                     Expr *E,
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
  Fields.push_back(std::make_pair(E, FD));
  return FD;
}

void Builder::InterfaceRecord::finalizeRecord(ASTContext &Context,
                                              HshBuiltins &Builtins) {
  std::stable_sort(Fields.begin(), Fields.end(),
                   [&](const auto &a, const auto &b) {
                     return Context.getTypeSizeInChars(a.second->getType()) >
                            Context.getTypeSizeInChars(b.second->getType());
                   });

  for (auto &P : Fields)
    Record->addDecl(P.second);
  Record->completeDefinition();
}

void Builder::updateUseStages() {
  for (int D = HshControlStage, S = HshVertexStage; D < HshMaxStage; ++D) {
    if (UseStages & (1u << unsigned(D))) {
      InterStageRecords[D].initializeRecord(Context, BindingDeclContext,
                                            HshStage(S), HshStage(D));
      S = D;
    }
  }
}

Expr *Builder::createInterStageReferenceExpr(Expr *E, HshStage From,
                                             HshStage To) {
  if (From == To || From == HshNoStage || To == HshNoStage)
    return E;
  assert(To > From && "cannot create backwards stage references");
  /* Create intermediate inter-stage assignments */
  for (int D = From + 1, S = From; D <= To; ++D) {
    if (UseStages & (1u << unsigned(D))) {
      InterfaceRecord &SRecord = InterStageRecords[S];
      InterfaceRecord &DRecord = InterStageRecords[D];
      if (MemberExpr *Producer =
              DRecord.createProducerFieldReference(Context, E)) {
        auto *AssignOp = BinaryOperator::Create(
            Context, Producer,
            S == From ? E : SRecord.createConsumerFieldReference(Context, E),
            BO_Assign, E->getType(), VK_XValue, OK_Ordinary, {}, {});
        addStageStmt(AssignOp, HshStage(S));
      }
      S = D;
    }
  }
  return InterStageRecords[To].createConsumerFieldReference(Context, E);
}

void Builder::addStageStmt(Stmt *S, HshStage Stage) {
  StageStmts[Stage].Stmts.push_back(S);
}

bool Builder::CheckSamplersEqual(const APValue &A, const APValue &B) {
  if (!A.isStruct() || !B.isStruct())
    return false;
  unsigned NumFields = A.getStructNumFields();
  if (NumFields != B.getStructNumFields())
    return false;
  for (unsigned i = 0; i < NumFields; ++i) {
    const auto &AF = A.getStructField(i);
    const auto &BF = A.getStructField(i);
    if (AF.isInt() && BF.isInt()) {
      if (APSInt::compareValues(AF.getInt(), BF.getInt()))
        return false;
    } else if (AF.isFloat() && BF.isFloat()) {
      if (AF.getFloat().compare(BF.getFloat()) != APFloat::cmpEqual)
        return false;
    } else {
      return false;
    }
  }
  return true;
}

void Builder::registerSampleCall(HshBuiltinCXXMethod HBM, CXXMemberCallExpr *C,
                                 HshStage Stage) {
  if (auto *DR = dyn_cast<DeclRefExpr>(
          C->getImplicitObjectArgument()->IgnoreParenImpCasts())) {
    if (auto *PVD = dyn_cast<ParmVarDecl>(DR->getDecl())) {
      auto &StageCalls = SampleCalls[Stage];
      for (const auto &Call : StageCalls)
        if (Call.Expr == C)
          return;
      APValue Res;
      Expr *SamplerArg = C->getArg(1);
      if (!SamplerArg->isCXX11ConstantExpr(Context, &Res)) {
        Reporter(Context).NonConstexprSampler(SamplerArg);
        return;
      }
      auto *Search =
          std::find_if(Samplers.begin(), Samplers.end(), [&](const auto &S) {
            return CheckSamplersEqual(S.Config, Res);
          });
      if (Search == Samplers.end()) {
        Search = Samplers.insert(Samplers.end(), SamplerRecord{std::move(Res)});
      }
      unsigned RecordIdx = Search - Samplers.begin();
      auto *BindingSearch = std::find_if(
          SamplerBindings.begin(), SamplerBindings.end(), [&](const auto &B) {
            return B.RecordIdx == RecordIdx && B.TextureDecl == PVD;
          });
      if (BindingSearch == SamplerBindings.end()) {
        auto *TextureSearch =
            std::find_if(Textures.begin(), Textures.end(),
                         [&](const auto &Tex) { return Tex.TexParm == PVD; });
        assert(TextureSearch != Textures.end());
        BindingSearch = SamplerBindings.insert(
            SamplerBindings.end(),
            SamplerBinding{RecordIdx,
                           unsigned(TextureSearch - Textures.begin()), PVD,
                           1u << Stage});
      } else {
        BindingSearch->UseStages |= 1u << Stage;
      }
      StageCalls.push_back(
          {C, PVD, unsigned(BindingSearch - SamplerBindings.begin())});
    }
  }
}

void Builder::registerAttributeField(QualType FieldType, CharUnits Offset,
                                     unsigned Binding) {
  auto HBT = Builtins.identifyBuiltinType(FieldType);

  if (HBT == HBT_None) {
    if (auto *SubRecord = FieldType->getAsCXXRecordDecl()) {
      registerAttributeFields(SubRecord, Binding, Offset);
      return;
    }
  }

  if (const auto *AT = dyn_cast_or_null<ConstantArrayType>(
          FieldType->getAsArrayTypeUnsafe())) {
    auto ElemSize = Context.getTypeSizeInChars(AT->getElementType());
    auto ElemAlign = Context.getTypeAlignInChars(AT->getElementType());
    unsigned Size = AT->getSize().getZExtValue();

    if (Builtins.identifyBuiltinType(AT->getElementType()) == HBT_None) {
      if (auto *SubRecord = AT->getElementType()->getAsCXXRecordDecl()) {
        for (unsigned i = 0; i < Size; ++i) {
          Offset = Offset.alignTo(ElemAlign);
          registerAttributeFields(SubRecord, Binding, Offset);
          Offset += ElemSize;
        }
        return;
      }
    }

    for (unsigned i = 0; i < Size; ++i) {
      Offset = Offset.alignTo(ElemAlign);
      registerAttributeField(AT->getElementType(), Offset, Binding);
      Offset += ElemSize;
    }
    return;
  }

  auto FieldSize = Context.getTypeSizeInChars(FieldType);
  auto FieldAlign = Context.getTypeAlignInChars(FieldType);
  auto Format = Builtins.formatOfType(FieldType);
  auto ProcessField = [&]() {
    Offset = Offset.alignTo(FieldAlign);
    VertexAttributes.push_back(
        VertexAttribute{uint32_t(Offset.getQuantity()), Binding, Format});
    Offset += FieldSize;
  };
  if (HshBuiltins::isMatrixType(HBT)) {
    auto ColType = Builtins.getType(HshBuiltins::getMatrixColumnType(HBT));
    FieldSize = Context.getTypeSizeInChars(ColType);
    FieldAlign = Context.getTypeSizeInChars(ColType);
    Format = Builtins.formatOfType(ColType);
    auto ColumnCount = HshBuiltins::getMatrixColumnCount(HBT);
    for (unsigned i = 0; i < ColumnCount; ++i)
      ProcessField();
  } else {
    ProcessField();
  }
}

void Builder::registerAttributeFields(const CXXRecordDecl *Record,
                                      unsigned Binding, CharUnits BaseOffset) {
  const ASTRecordLayout &RL = Context.getASTRecordLayout(Record);
  for (const auto *Field : Record->fields()) {
    auto Offset = BaseOffset + Context.toCharUnitsFromBits(
                                   RL.getFieldOffset(Field->getFieldIndex()));
    registerAttributeField(Field->getType(), Offset, Binding);
  }
}

void Builder::registerAttributeRecord(AttributeRecord Attribute) {
  auto *Search =
      std::find_if(AttributeRecords.begin(), AttributeRecords.end(),
                   [&](const auto &A) { return A.Name == Attribute.Name; });
  if (Search != AttributeRecords.end())
    return;
  unsigned Binding = AttributeRecords.size();
  AttributeRecords.push_back(Attribute);

  const auto *Type = Attribute.Record->getTypeForDecl();
  auto Size = Context.getTypeSizeInChars(Type);
  auto Align = Context.getTypeAlignInChars(Type);
  auto SizeOf = Size.alignTo(Align).getQuantity();
  VertexBindings.push_back(
      VertexBinding{Binding, uint32_t(SizeOf), Attribute.Kind});
  registerAttributeFields(Attribute.Record, Binding);
}

StaticAssertDecl *Builder::CreateEnumSizeAssert(ASTContext &Context,
                                                DeclContext *DC,
                                                FieldDecl *FD) {
  return StaticAssertDecl::Create(
      Context, DC, {},
      BinaryOperator::Create(
          Context,
          new (Context) UnaryExprOrTypeTraitExpr(
              UETT_SizeOf,
              new (Context)
                  ParenExpr({}, {},
                            DeclRefExpr::Create(Context, {}, {}, FD, false,
                                                SourceLocation{}, FD->getType(),
                                                VK_XValue)),
              Context.IntTy, {}, {}),
          IntegerLiteral::Create(Context, APInt(32, 4), Context.IntTy, {}),
          BO_EQ, Context.BoolTy, VK_XValue, OK_Ordinary, {}, {}),
      Context.getPredefinedStringLiteralFromCache(
          "underlying enum type must be 32-bits wide"),
      {}, false);
}

StaticAssertDecl *Builder::CreateOffsetAssert(ASTContext &Context,
                                              DeclContext *DC, FieldDecl *FD,
                                              CharUnits Offset) {
  return StaticAssertDecl::Create(
      Context, DC, {},
      BinaryOperator::Create(
          Context,
          OffsetOfExpr::Create(Context, Context.IntTy, {},
                               Context.getTrivialTypeSourceInfo(QualType{
                                   FD->getParent()->getTypeForDecl(), 0}),
                               OffsetOfNode{{}, FD, {}}, {}, {}),
          IntegerLiteral::Create(Context, APInt(32, Offset.getQuantity()),
                                 Context.IntTy, {}),
          BO_EQ, Context.BoolTy, VK_XValue, OK_Ordinary, {}, {}),
      Context.getPredefinedStringLiteralFromCache(
          "compiler does not align field correctly"),
      {}, false);
}

void Builder::registerUniform(StringRef Name, const CXXRecordDecl *Record,
                              unsigned Stages) {
  auto &Diags = Context.getDiagnostics();

  auto *Search = std::find_if(UniformRecords.begin(), UniformRecords.end(),
                              [&](const auto &T) { return T.Name == Name; });
  if (Search != UniformRecords.end()) {
    Search->UseStages |= Stages;
    return;
  }

  UniformFieldValidator Validator(Context, Builtins);
  Validator.Visit(Record);

  const auto &RL = Context.getASTRecordLayout(Record);

  CharUnits HLSLOffset, CXXOffset;
  for (auto *Field : Record->fields()) {
    auto CXXFieldSize = Context.getTypeSizeInChars(Field->getType());
    auto HLSLFieldSize = CXXFieldSize.alignTo(CharUnits::fromQuantity(4));
    auto CXXFieldAlign = Context.getTypeAlignInChars(Field->getType());
    auto HLSLFieldAlign = CXXFieldAlign.alignTo(CharUnits::fromQuantity(4));

    HLSLOffset = HLSLOffset.alignTo(HLSLFieldAlign);
    CXXOffset =
        Context.toCharUnitsFromBits(RL.getFieldOffset(Field->getFieldIndex()));
    CharUnits HLSLEndOffset = HLSLOffset + HLSLFieldSize;
    bool FieldStraddled;
    if ((FieldStraddled = (HLSLEndOffset.getQuantity() - 1) / 16 >
                          HLSLOffset.getQuantity() / 16))
      HLSLOffset = HLSLOffset.alignTo(CharUnits::fromQuantity(16));
    CharUnits ThisOffset = HLSLOffset;
    if (HLSLOffset != CXXOffset) {
      unsigned FixAlign = 16;
      if (!FieldStraddled && HLSLFieldSize.getQuantity() < 16) {
        for (unsigned Align : {4, 8, 16}) {
          FixAlign = Align;
          CharUnits FixOffset =
              CXXOffset.alignTo(CharUnits::fromQuantity(Align));
          if (HLSLOffset == FixOffset)
            break;
        }
      }
      std::string AlignAsStr;
      raw_string_ostream OS(AlignAsStr);
      OS << "alignas(" << FixAlign << ") ";
      Diags.Report(
          Field->getBeginLoc(),
          Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                "uniform field violates DirectX packing "
                                "rules; align or pad by %0 bytes"))
          << int((HLSLOffset - CXXOffset).getQuantity())
          << Field->getSourceRange()
          << FixItHint::CreateInsertion(Field->getBeginLoc(), OS.str());
      return;
    }
    HLSLOffset += HLSLFieldSize;

    if (Field->getType()->isEnumeralType())
      BindingDeclContext->addDecl(
          CreateEnumSizeAssert(Context, BindingDeclContext, Field));

    if (!Builtins.isSupportedArrayType(Record))
      BindingDeclContext->addDecl(
          CreateOffsetAssert(Context, BindingDeclContext, Field, ThisOffset));
  }

  UniformRecords.push_back(UniformRecord{Name, Record, Stages});
}

void Builder::registerTexture(const ParmVarDecl *TexParm, HshTextureKind Kind,
                              unsigned Stages) {
  auto *Search =
      std::find_if(Textures.begin(), Textures.end(),
                   [&](const auto &T) { return T.TexParm == TexParm; });
  if (Search != Textures.end()) {
    Search->UseStages |= Stages;
    return;
  }
  Textures.push_back(TextureRecord{TexParm, Kind, Stages});
}

void Builder::registerParmVarRef(ParmVarDecl *PVD, HshStage Stage) {
  UseParmVarDecls[PVD] |= 1u << unsigned(Stage);
}

void Builder::registerFunctionDecl(FunctionDecl *FD, HshStage Stage) {
  if (auto *FDDecl = FD->getDefinition())
    FD = FDDecl;
  else
    Reporter(Context).UndefinedFunctionUsage(FD);
  auto *Search = std::find_if(
      FunctionRecords.begin(), FunctionRecords.end(),
      [&](const auto &T) { return T.Identifier == FD->getIdentifier(); });
  if (Search == FunctionRecords.end()) {
    FunctionRecords.push_back(
        FunctionRecord{FD->getIdentifier(), FD, 1u << unsigned(Stage)});
  } else if (Search->Function == FD) {
    Search->UseStages |= 1u << unsigned(Stage);
  } else {
    Reporter(Context).OverloadedFunctionUsage(FD, Search->Function);
  }
}

void Builder::prepare(CXXConstructorDecl *Ctor) {
  for (auto *Param : Ctor->parameters()) {
    auto HBT = Builtins.identifyBuiltinType(Param->getType());
    if (HshBuiltins::isTextureType(HBT)) {
      registerTexture(Param, KindOfTextureType(HBT), 0);
    }
  }
}

void Builder::finalizeResults(CXXConstructorDecl *Ctor) {
  for (int D = HshControlStage; D < HshMaxStage; ++D) {
    if (UseStages & (1u << unsigned(D)))
      InterStageRecords[D].finalizeRecord(Context, Builtins);
  }

  FinalStageCount = 0;
  for (int S = HshVertexStage; S < HshMaxStage; ++S) {
    if (UseStages & (1u << unsigned(S))) {
      ++FinalStageCount;
      auto &Stmts = StageStmts[S];
      Stmts.CStmts = CompoundStmt::Create(Context, Stmts.Stmts, {}, {});
    }
  }

  for (auto *Param : Ctor->parameters()) {
    unsigned Stages = 0;
    auto Search = UseParmVarDecls.find(Param);
    if (Search != UseParmVarDecls.end())
      Stages = Search->second;
    auto HBT = Builtins.identifyBuiltinType(Param->getType());
    if (HshBuiltins::isTextureType(HBT)) {
      registerTexture(Param, KindOfTextureType(HBT), Stages);
    } else if (const auto *UR = Builtins.getUniformRecord(Param)) {
      registerUniform(Param->getName(), UR, Stages);
    } else if (const auto *AR = Builtins.getVertexAttributeRecord(Param)) {
      registerAttributeRecord(AttributeRecord{
          Param->getName(), AR,
          Param->hasAttr<HshInstanceAttr>() ? PerInstance : PerVertex});
    }
  }
}

StageSources Builder::printResults(ShaderPrintingPolicyBase &Policy) {
  StageSources Sources;

  for (int S = HshVertexStage; S < HshMaxStage; ++S) {
    if (UseStages & (1u << unsigned(S))) {
      raw_string_ostream OS(Sources[S]);
      HshStage NextStage = nextUsedStage(HshStage(S));
      Policy.printStage(
          OS, FunctionRecords, UniformRecords, InterStageRecords[S].getRecord(),
          NextStage != HshNoStage ? InterStageRecords[NextStage].getRecord()
                                  : nullptr,
          AttributeRecords, Textures, SamplerBindings, NumColorAttachments,
          HasDualSource, StageStmts[S].CStmts, HshStage(S),
          previousUsedStage(HshStage(S)), NextStage, SampleCalls[S]);
    }
  }

  return Sources;
}

} // namespace clang::hshgen
