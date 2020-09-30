//===--- Builder.h - hshgen partitioner actions ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "Builtins/Builtins.h"

#include <bitset>

namespace clang::hshgen {
struct ShaderPrintingPolicyBase;

using StageSources = std::array<std::string, HshMaxStage>;

class Builder {
  ASTContext &Context;
  HshBuiltins &Builtins;
  DeclContext *BindingDeclContext;
  unsigned UseStages = 0;

  static IdentifierInfo &getToIdent(ASTContext &Context, HshStage Stage);

  static IdentifierInfo &getFromIdent(ASTContext &Context, HshStage Stage);

  static IdentifierInfo &getFromToIdent(ASTContext &Context, HshStage From,
                                        HshStage To);

  class InterfaceRecord {
    CXXRecordDecl *Record = nullptr;
    SmallVector<std::pair<Expr *, FieldDecl *>, 8> Fields;
    VarDecl *Producer = nullptr;
    VarDecl *Consumer = nullptr;
    HshStage SStage = HshNoStage, DStage = HshNoStage;

    MemberExpr *createFieldReference(ASTContext &Context, Expr *E, VarDecl *VD,
                                     bool Producer);

  public:
    void initializeRecord(ASTContext &Context, DeclContext *BindingDeclContext,
                          HshStage S, HshStage D);

    static bool isSameComparisonOperand(Expr *E1, Expr *E2) {
      if (E1 == E2)
        return true;
      E1->setValueKind(VK_RValue);
      E2->setValueKind(VK_RValue);
      return Expr::isSameComparisonOperand(E1, E2);
    }

    FieldDecl *getFieldForExpr(ASTContext &Context, Expr *E,
                               bool IgnoreExisting);

    MemberExpr *createProducerFieldReference(ASTContext &Context, Expr *E) {
      return createFieldReference(Context, E, Producer, true);
    }

    MemberExpr *createConsumerFieldReference(ASTContext &Context, Expr *E) {
      return createFieldReference(Context, E, Consumer, false);
    }

    void finalizeRecord(ASTContext &Context, HshBuiltins &Builtins);

    CXXRecordDecl *getRecord() const { return Record; }
  };

  struct StageStmtList {
    SmallVector<Stmt *, 16> Stmts;
    CompoundStmt *CStmts = nullptr;
  };

  std::array<InterfaceRecord, HshMaxStage>
      InterStageRecords; /* Indexed by consumer stage */
  std::array<StageStmtList, HshMaxStage> StageStmts;
  std::array<SmallVector<SampleCall, 4>, HshMaxStage> SampleCalls;
  SmallVector<FunctionRecord, 4> FunctionRecords;
  SmallVector<UniformRecord, 4> UniformRecords;
  SmallVector<AttributeRecord, 4> AttributeRecords;
  SmallVector<TextureRecord, 8> Textures;
  SmallVector<SamplerRecord, 8> Samplers;
  SmallVector<SamplerBinding, 8> SamplerBindings;
  unsigned NumColorAttachments = 0;
  unsigned FinalStageCount = 0;
  SmallVector<VertexBinding, 4> VertexBindings;
  SmallVector<VertexAttribute, 4> VertexAttributes;
  llvm::DenseMap<ParmVarDecl *, unsigned> UseParmVarDecls;
  bool HasDualSource = false;
  std::bitset<HPF_Max> ReferencedPipelineFields;

public:
  Builder(ASTContext &Context, HshBuiltins &Builtins,
          DeclContext *BindingDeclContext, unsigned NumColorAttachments,
          bool HasDualSource)
      : Context(Context), Builtins(Builtins),
        BindingDeclContext(BindingDeclContext),
        NumColorAttachments(NumColorAttachments), HasDualSource(HasDualSource) {
  }

  void updateUseStages();

  Expr *createInterStageReferenceExpr(Expr *E, HshStage From, HshStage To);

  void addStageStmt(Stmt *S, HshStage Stage);

  static bool CheckSamplersEqual(const APValue &A, const APValue &B);

  void registerSampleCall(HshBuiltinCXXMethod HBM, CXXMemberCallExpr *C,
                          HshStage Stage);

  void registerAttributeField(QualType FieldType, CharUnits Offset,
                              unsigned Binding);

  void registerAttributeFields(const CXXRecordDecl *Record, unsigned Binding,
                               CharUnits BaseOffset = {});

  void registerAttributeRecord(AttributeRecord Attribute);

  static StaticAssertDecl *CreateEnumSizeAssert(ASTContext &Context,
                                                DeclContext *DC, FieldDecl *FD);

  static StaticAssertDecl *CreateOffsetAssert(ASTContext &Context,
                                              DeclContext *DC, FieldDecl *FD,
                                              CharUnits Offset);

  void registerUniform(StringRef Name, const CXXRecordDecl *Record,
                       unsigned Stages);

  void registerTexture(const ParmVarDecl *TexParm, HshTextureKind Kind,
                       unsigned Stages);

  void registerParmVarRef(ParmVarDecl *PVD, HshStage Stage);

  void registerFunctionDecl(FunctionDecl *FD, HshStage Stage);

  void prepare(CXXConstructorDecl *Ctor);

  void finalizeResults(CXXConstructorDecl *Ctor);

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

  void setStageUsed(HshStage S) {
    if (S == HshNoStage)
      return;
    UseStages |= 1 << S;
  }

  void setReferencedPipelineField(HshBuiltinPipelineField field) {
    ReferencedPipelineFields.set(field);
  }

  StageSources printResults(ShaderPrintingPolicyBase &Policy);

  unsigned getNumStages() const { return FinalStageCount; }
  unsigned getNumBindings() const { return VertexBindings.size(); }
  unsigned getNumAttributes() const { return VertexAttributes.size(); }
  unsigned getNumSamplers() const { return Samplers.size(); }
  unsigned getNumSamplerBindings() const { return SamplerBindings.size(); }
  ArrayRef<VertexBinding> getBindings() const { return VertexBindings; }
  ArrayRef<VertexAttribute> getAttributes() const { return VertexAttributes; }
  ArrayRef<SamplerRecord> getSamplers() const { return Samplers; }
  ArrayRef<SamplerBinding> getSamplerBindings() const {
    return SamplerBindings;
  }
};

} // namespace clang::hshgen
