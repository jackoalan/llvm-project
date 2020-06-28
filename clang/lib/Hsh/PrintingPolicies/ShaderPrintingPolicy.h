//===--- ShaderPrintingPolicy.h - base shader stage printing policy -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "llvm/ADT/FunctionExtras.h"

#include "clang/AST/ExprCXX.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/Hsh/HshGenerator.h"

#include "../Builtins/Builtins.h"

namespace clang::hshgen {

struct HostPrintingPolicy final : PrintingCallbacks, PrintingPolicy {
  explicit HostPrintingPolicy(const PrintingPolicy &Policy)
      : PrintingPolicy(Policy) {
    Callbacks = this;
    Indentation = 1;
    SuppressImplicitBase = true;
    SilentNullStatement = true;
    NeverSuppressScope = true;
    UseStdOffsetOf = true;
  }

  mutable llvm::unique_function<bool(VarDecl *, raw_ostream &)> VarInitPrint;
  void setVarInitPrint(
      llvm::unique_function<bool(VarDecl *, raw_ostream &)> &&Func) {
    VarInitPrint = std::move(Func);
  }
  void resetVarInitPrint() { VarInitPrint = decltype(VarInitPrint){}; }
  bool overrideVarInitPrint(VarDecl *D, raw_ostream &OS) const override {
    return VarInitPrint(D, OS);
  }
};

struct ShaderPrintingPolicyBase : PrintingPolicy {
  HshBuiltins &Builtins;
  HshTarget Target;
  virtual ~ShaderPrintingPolicyBase() = default;
  virtual void printStage(raw_ostream &OS, ASTContext &Context,
                          ArrayRef<FunctionRecord> FunctionRecords,
                          ArrayRef<UniformRecord> UniformRecords,
                          CXXRecordDecl *FromRecord, CXXRecordDecl *ToRecord,
                          ArrayRef<AttributeRecord> Attributes,
                          ArrayRef<TextureRecord> Textures,
                          ArrayRef<SamplerBinding> Samplers,
                          unsigned NumColorAttachments, CompoundStmt *Stmts,
                          HshStage Stage, HshStage From, HshStage To,
                          ArrayRef<SampleCall> SampleCalls) = 0;
  explicit ShaderPrintingPolicyBase(HshBuiltins &Builtins, HshTarget Target)
      : PrintingPolicy(LangOptions()), Builtins(Builtins), Target(Target) {}

  static void PrintNZeros(raw_ostream &OS, unsigned N);

  static void PrintNExprs(raw_ostream &OS,
                          const std::function<void(Expr *)> &ExprArg,
                          unsigned N, Expr *E);

  static DeclRefExpr *GetMemberExprBase(MemberExpr *ME);

  static constexpr std::array<char, 4> VectorComponents{'x', 'y', 'z', 'w'};

  enum class ArrayWaitType { NoArray, StdArray, AlignedArray };

  SmallVector<const CXXRecordDecl *, 8> NestedRecords;

  void PrintNestedStructs(raw_ostream &OS, ASTContext &Context);

  ArrayWaitType getArrayWaitType(const CXXRecordDecl *RD) const;

  void GatherNestedStructField(ASTContext &Context, QualType Tp,
                               ArrayWaitType WaitingForArray);

  void GatherNestedStructFields(ASTContext &Context,
                                const CXXRecordDecl *Record,
                                ArrayWaitType WaitingForArray);

  void GatherNestedPackoffsetFields(ASTContext &Context,
                                    const CXXRecordDecl *Record,
                                    CharUnits BaseOffset = {});

  void PrintStructField(raw_ostream &OS, ASTContext &Context, QualType Tp,
                        const Twine &FieldName, ArrayWaitType WaitingForArray,
                        unsigned Indent);

  void PrintStructFields(raw_ostream &OS, ASTContext &Context,
                         const CXXRecordDecl *Record, const Twine &FieldName,
                         ArrayWaitType WaitingForArray, unsigned Indent);
};

using InShaderPipelineArgsType =
    ArrayRef<std::pair<StringRef, TemplateArgument>>;

template <typename ImplClass>
struct ShaderPrintingPolicy : PrintingCallbacks, ShaderPrintingPolicyBase {
  bool EarlyDepthStencil = false;
  explicit ShaderPrintingPolicy(HshBuiltins &Builtins, HshTarget Target,
                                InShaderPipelineArgsType InShaderPipelineArgs);

  StringRef overrideBuiltinTypeName(const BuiltinType *T) const override;

  StringRef overrideTagDeclIdentifier(TagDecl *D) const override;

  StringRef overrideBuiltinFunctionIdentifier(CallExpr *C) const override;

  bool overrideCallArguments(
      CallExpr *C, const std::function<void(StringRef)> &StringArg,
      const std::function<void(Expr *)> &ExprArg) const override;

  mutable std::string EnumValStr;
  StringRef overrideDeclRefIdentifier(DeclRefExpr *DR) const override;

  StringRef overrideMemberExpr(MemberExpr *ME) const override;

  StringRef prependMemberExprBase(MemberExpr *ME,
                                  bool &ReplaceBase) const override;

  bool shouldPrintMemberExprUnderscore(MemberExpr *ME) const override;

  void PrintPackoffsetField(raw_ostream &OS, ASTContext &Context, QualType Tp,
                            const Twine &FieldName,
                            ArrayWaitType WaitingForArray, CharUnits Offset);

  void PrintPackoffsetFields(raw_ostream &OS, ASTContext &Context,
                             const CXXRecordDecl *Record,
                             const Twine &PrefixName,
                             CharUnits BaseOffset = {});

  void PrintAttributeField(raw_ostream &OS, ASTContext &Context, QualType Tp,
                           const Twine &FieldName,
                           ArrayWaitType WaitingForArray, unsigned Indent,
                           unsigned &Location, unsigned ArraySize);

  void PrintAttributeFields(raw_ostream &OS, ASTContext &Context,
                            const CXXRecordDecl *Record, const Twine &FieldName,
                            ArrayWaitType WaitingForArray, unsigned Indent,
                            unsigned &Location);
};

std::unique_ptr<ShaderPrintingPolicyBase>
MakePrintingPolicy(HshBuiltins &Builtins, HshTarget Target,
                   InShaderPipelineArgsType InShaderPipelineArgs);

} // namespace clang::hshgen
