//===--- MetalPrintingPolicy.h - Metal shader stage printing policy -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "ShaderPrintingPolicy.h"

namespace clang::hshgen {

struct MetalPrintingPolicy
    : ShaderPrintingPolicy<MetalPrintingPolicy,
                           ShaderPrintingPolicyBase::FieldFloatPairer> {
  MetalPrintingPolicy(HshBuiltins &Builtins, ASTContext &Context,
                      HshTarget Target,
                      InShaderPipelineArgsType InShaderPipelineArgs)
      : ShaderPrintingPolicy(Builtins, Context, Target, InShaderPipelineArgs) {
    NoLoopInitVar = true;
  }

  static constexpr HshTarget SourceTarget = HT_METAL;
  static constexpr bool NoUniformVarDecl = false;
  static constexpr llvm::StringLiteral SignedInt32Spelling{"int"};
  static constexpr llvm::StringLiteral UnsignedInt32Spelling{"uint"};
  static constexpr llvm::StringLiteral Float32Spelling{"float"};
  static constexpr llvm::StringLiteral Float64Spelling{"double"};
  static constexpr llvm::StringLiteral VertexBufferBase{"_vert_data."};

  std::string VertexPositionIdentifier;
  StringRef identifierOfVertexPosition(FieldDecl *FD) const {
    return VertexPositionIdentifier;
  }

  static constexpr StringRef identifierOfColorAttachment(FieldDecl *FD) {
    return "_targets_out._color_out";
  }

  mutable std::string CXXMethodIdentifier;
  StringRef identifierOfCXXMethod(HshBuiltinCXXMethod HBM,
                                  CXXMemberCallExpr *C) const;

  ArrayRef<SampleCall> ThisSampleCalls;
  bool overrideCXXMethodArguments(
      HshBuiltinCXXMethod HBM, CXXMemberCallExpr *C,
      const std::function<void(StringRef)> &StringArg,
      const std::function<void(Expr *)> &ExprArg,
      const std::function<void(StringRef, Expr *, StringRef)> &WrappedExprArg)
      const;

  bool overrideCXXOperatorCall(
      CXXOperatorCallExpr *C, raw_ostream &OS,
      const std::function<void(Expr *)> &ExprArg) const override;

  bool overrideCXXTemporaryObjectExpr(
      CXXTemporaryObjectExpr *C, raw_ostream &OS,
      const std::function<void(Expr *)> &ExprArg) const override;

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

  static void PrintBeforePackoffset(FieldFloatPairer &FH, CharUnits Offset) {}

  static void PrintAfterPackoffset(FieldFloatPairer &FH, CharUnits Offset) {}

  void PrintAttributeFieldSpelling(raw_ostream &OS, QualType Tp,
                                   const Twine &FieldName, unsigned Location,
                                   unsigned Indent) const;

  void printStage(raw_ostream &OS, ArrayRef<FunctionRecord> FunctionRecords,
                  ArrayRef<UniformRecord> UniformRecords,
                  CXXRecordDecl *FromRecord, CXXRecordDecl *ToRecord,
                  ArrayRef<AttributeRecord> Attributes,
                  ArrayRef<TextureRecord> Textures,
                  ArrayRef<SamplerBinding> Samplers,
                  unsigned NumColorAttachments, CompoundStmt *Stmts,
                  HshStage Stage, HshStage From, HshStage To,
                  ArrayRef<SampleCall> SampleCalls) override;
};

} // namespace clang::hshgen
