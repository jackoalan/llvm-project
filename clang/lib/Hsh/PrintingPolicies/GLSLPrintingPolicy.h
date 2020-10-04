//===--- GLSLPrintingPolicy.h - GLSL shader stage printing policy ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "ShaderPrintingPolicy.h"

namespace clang::hshgen {

struct GLSLPrintingPolicy
    : ShaderPrintingPolicy<GLSLPrintingPolicy,
                           ShaderPrintingPolicyBase::FieldPrinter> {
  using base = ShaderPrintingPolicy<GLSLPrintingPolicy,
                                    ShaderPrintingPolicyBase::FieldPrinter>;
  static constexpr HshTarget SourceTarget = HT_GLSL;
  static constexpr bool NoUniformVarDecl = true;
  static constexpr llvm::StringLiteral SignedInt32Spelling{"int"};
  static constexpr llvm::StringLiteral UnsignedInt32Spelling{"uint"};
  static constexpr llvm::StringLiteral Float32Spelling{"float"};
  static constexpr llvm::StringLiteral Float64Spelling{"double"};
  static constexpr llvm::StringLiteral VertexBufferBase{""};

  static constexpr StringRef identifierOfVertexPosition(FieldDecl *FD) {
    return "gl_Position";
  }

  static constexpr StringRef identifierOfColorAttachment(FieldDecl *FD) {
    return "_color_out";
  }

  static constexpr StringRef identifierOfVertexID(FieldDecl *FD) {
    return "gl_VertexID";
  }

  static constexpr StringRef identifierOfInstanceID(FieldDecl *FD) {
    return "gl_InstanceID";
  }

  static constexpr StringRef identifierOfCXXMethod(HshBuiltinCXXMethod HBM,
                                                   CXXMemberCallExpr *C) {
    switch (HBM) {
    case HBM_sample2d:
    case HBM_render_sample2d:
    case HBM_sample_bias2d:
    case HBM_sample2da:
    case HBM_sample_bias2da:
    case HBM_read2da:
      return "texture";
    case HBM_read2d:
    case HBM_render_read2d:
      return "texelFetch";
    default:
      return {};
    }
  }

  static constexpr bool overrideCXXMethodArguments(
      HshBuiltinCXXMethod HBM, CXXMemberCallExpr *C,
      const std::function<void(StringRef)> &StringArg,
      const std::function<void(Expr *)> &ExprArg,
      const std::function<void(StringRef, Expr *, StringRef)> &WrappedExprArg) {
    switch (HBM) {
    case HBM_sample2d:
    case HBM_sample_bias2d:
    case HBM_render_sample2d: {
      ExprArg(C->getImplicitObjectArgument()->IgnoreParenImpCasts());
      ExprArg(C->getArg(0));
      if (HBM == HBM_sample_bias2d)
        ExprArg(C->getArg(1));
      return true;
    }
    case HBM_read2d:
    case HBM_render_read2d: {
      ExprArg(C->getImplicitObjectArgument()->IgnoreParenImpCasts());
      WrappedExprArg("ivec2(", C->getArg(0), ")");
      Expr *LODStmt = C->getArg(1);
      if (auto *arg = dyn_cast<CXXDefaultArgExpr>(LODStmt)) {
        LODStmt = arg->getExpr();
      }
      ExprArg(LODStmt);
      return true;
    }
    case HBM_read2da: {
      ExprArg(C->getImplicitObjectArgument()->IgnoreParenImpCasts());
      WrappedExprArg("ivec3(", C->getArg(0), "");
      WrappedExprArg("", C->getArg(1), ")");
      Expr *LODStmt = C->getArg(2);
      if (auto *arg = dyn_cast<CXXDefaultArgExpr>(LODStmt)) {
        LODStmt = arg->getExpr();
      }
      ExprArg(LODStmt);
      return true;
    }
    default:
      return false;
    }
  }

  bool overrideCXXOperatorCall(
      CXXOperatorCallExpr *C, raw_ostream &OS,
      const std::function<void(Expr *)> &ExprArg) const override;

  bool overrideCXXTemporaryObjectExpr(
      CXXTemporaryObjectExpr *C, raw_ostream &OS,
      const std::function<void(Expr *)> &ExprArg) const override;

  static void PrintBeforePackoffset(FieldPrinter &FH, CharUnits Offset) {
    FH.indent(2) << "layout(offset = " << Offset.getQuantity() << ")\n";
  }

  static void PrintAfterPackoffset(FieldPrinter &FH, CharUnits Offset) {
    FH << ";\n";
  }

  void PrintAttributeFieldSpelling(raw_ostream &OS, QualType Tp,
                                   const Twine &FieldName, unsigned Location,
                                   unsigned Indent) const;

  void printStage(raw_ostream &OS, ArrayRef<FunctionRecord> FunctionRecords,
                  ArrayRef<UniformRecord> UniformRecords,
                  CXXRecordDecl *FromRecord, CXXRecordDecl *ToRecord,
                  ArrayRef<AttributeRecord> Attributes,
                  ArrayRef<TextureRecord> Textures,
                  ArrayRef<SamplerBinding> Samplers,
                  unsigned NumColorAttachments, bool HasDualSource,
                  CompoundStmt *Stmts, HshStage Stage, HshStage From,
                  HshStage To, ArrayRef<SampleCall> SampleCalls,
                  std::bitset<HPF_Max> ReferencedPipelineFields) override;

  using base::ShaderPrintingPolicy;
};

} // namespace clang::hshgen
