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
  ASTContext &Context;
  HshTarget Target;
  virtual ~ShaderPrintingPolicyBase() = default;
  virtual void
  printStage(raw_ostream &OS, ArrayRef<FunctionRecord> FunctionRecords,
             ArrayRef<UniformRecord> UniformRecords, CXXRecordDecl *FromRecord,
             CXXRecordDecl *ToRecord, ArrayRef<AttributeRecord> Attributes,
             ArrayRef<TextureRecord> Textures,
             ArrayRef<SamplerBinding> Samplers, unsigned NumColorAttachments,
             CompoundStmt *Stmts, HshStage Stage, HshStage From, HshStage To,
             ArrayRef<SampleCall> SampleCalls) = 0;
  explicit ShaderPrintingPolicyBase(HshBuiltins &Builtins, ASTContext &Context,
                                    HshTarget Target)
      : PrintingPolicy(LangOptions()), Builtins(Builtins), Context(Context),
        Target(Target) {}

  static void PrintNZeros(raw_ostream &OS, unsigned N);

  static void PrintNExprs(raw_ostream &OS,
                          const std::function<void(Expr *)> &ExprArg,
                          unsigned N, Expr *E);

  static DeclRefExpr *GetMemberExprBase(MemberExpr *ME);

  static constexpr std::array<char, 4> VectorComponents{'x', 'y', 'z', 'w'};

  enum class ArrayWaitType { NoArray, StdArray, AlignedArray };

  SmallVector<const CXXRecordDecl *, 8> NestedRecords;

  class FieldFloatPairer {
    ShaderPrintingPolicyBase &PP;
    raw_ostream &OS;

    struct FieldDescription {
      QualType Type;
      SmallString<64> FieldName;
      unsigned Indent = 0;
      FieldDescription() = default;
      FieldDescription(QualType Type, const Twine &FieldNameIn, unsigned Indent)
          : Type(Type), Indent(Indent) {
        FieldNameIn.toVector(FieldName);
      }
      void Flush(ShaderPrintingPolicyBase &PP, raw_ostream &OS) {
        if (!Type.isNull()) {
          OS.indent(Indent * 2);
          Type.print(OS, PP, FieldName, Indent);
          OS << ";\n";
        }
        *this = FieldDescription();
      }
    } LastField;

    unsigned PadIndex = 0;

  public:
    explicit FieldFloatPairer(ShaderPrintingPolicyBase &PP, raw_ostream &OS)
        : PP(PP), OS(OS) {}

    void Field(QualType Type, const Twine &FieldName, bool ArrayField,
               unsigned Indent) {
      if (ArrayField) {
        Flush();
        LastField = FieldDescription(Type, FieldName, Indent);
        Flush();
        return;
      }
      if (!LastField.Type.isNull() && !Type.isNull()) {
        if (PP.Builtins.identifyBuiltinType(LastField.Type) == HBT_float3 &&
            Type->isSpecificBuiltinType(BuiltinType::Float)) {
          /* A sequence of float3/float will emit a union */
          OS.indent(Indent * 2) << "union {\n";
          ++Indent;
          ++LastField.Indent;
          Flush();
          OS.indent(Indent * 2) << "struct {\n";
          ++Indent;
          OS.indent(Indent * 2) << "float __pad" << PadIndex++ << "[3];\n";
          OS.indent(Indent * 2) << "float " << FieldName << ";\n";
          --Indent;
          OS.indent(Indent * 2) << "};\n";
          --Indent;
          OS.indent(Indent * 2) << "};\n";
          return;
        }
      }
      Flush();
      LastField = FieldDescription(Type, FieldName, Indent);
    }

    void NestedField(unsigned NestedIndex, const Twine &FieldName,
                     unsigned Indent) {
      Flush();
      OS.indent(Indent * 2)
          << "__Struct" << NestedIndex << ' ' << FieldName << ";\n";
    }

    void Flush() { LastField.Flush(PP, OS); }

    template <typename T> FieldFloatPairer &operator<<(const T &Val) {
      Flush();
      OS << Val;
      return *this;
    }

    FieldFloatPairer &indent(unsigned NumSpaces) {
      Flush();
      OS.indent(NumSpaces);
      return *this;
    }
  };

  class FieldPrinter {
    ShaderPrintingPolicyBase &PP;
    raw_ostream &OS;

  public:
    explicit FieldPrinter(ShaderPrintingPolicyBase &PP, raw_ostream &OS)
        : PP(PP), OS(OS) {}

    void Field(QualType Type, const Twine &FieldName, bool ArrayField,
               unsigned Indent) {
      OS.indent(Indent * 2);
      Type.print(OS, PP, FieldName, Indent);
    }

    void NestedField(unsigned NestedIndex, const Twine &FieldName,
                     unsigned Indent) {
      OS.indent(Indent * 2) << "__Struct" << NestedIndex << ' ' << FieldName;
    }

    void Flush() {}

    template <typename T> FieldPrinter &operator<<(const T &Val) {
      OS << Val;
      return *this;
    }

    FieldPrinter &indent(unsigned NumSpaces) {
      OS.indent(NumSpaces);
      return *this;
    }
  };

  template <typename FieldHandler> void PrintNestedStructs(raw_ostream &OS);

  ArrayWaitType getArrayWaitType(const CXXRecordDecl *RD) const;

  void GatherNestedStructField(QualType Tp, ArrayWaitType WaitingForArray);

  void GatherNestedStructFields(const CXXRecordDecl *Record,
                                ArrayWaitType WaitingForArray);

  void GatherNestedPackoffsetFields(const CXXRecordDecl *Record,
                                    CharUnits BaseOffset = {});

  template <typename FieldHandler>
  void PrintStructField(FieldHandler &FH, QualType Tp, const Twine &FieldName,
                        bool ArrayField, ArrayWaitType WaitingForArray,
                        unsigned Indent);

  template <typename FieldHandler>
  void PrintStructFields(FieldHandler &FH, const CXXRecordDecl *Record,
                         const Twine &FieldName, bool ArrayField,
                         ArrayWaitType WaitingForArray, unsigned Indent);
};

using InShaderPipelineArgsType =
    ArrayRef<std::pair<StringRef, TemplateArgument>>;

template <typename ImplClass, typename PackoffsetFieldHandler>
struct ShaderPrintingPolicy : PrintingCallbacks, ShaderPrintingPolicyBase {
  bool EarlyDepthStencil = false;
  explicit ShaderPrintingPolicy(HshBuiltins &Builtins, ASTContext &Context,
                                HshTarget Target,
                                InShaderPipelineArgsType InShaderPipelineArgs);

  StringRef overrideBuiltinTypeName(const BuiltinType *T) const override;

  StringRef overrideTagDeclIdentifier(TagDecl *D) const override;

  StringRef overrideBuiltinFunctionIdentifier(CallExpr *C) const override;

  bool overrideCallArguments(
      CallExpr *C, const std::function<void(StringRef)> &StringArg,
      const std::function<void(Expr *)> &ExprArg,
      const std::function<void(StringRef, Expr *, StringRef)> &WrappedExprArg)
      const override;

  mutable std::string EnumValStr;
  StringRef overrideDeclRefIdentifier(DeclRefExpr *DR) const override;

  StringRef overrideMemberExpr(MemberExpr *ME) const override;

  StringRef prependMemberExprBase(MemberExpr *ME,
                                  bool &ReplaceBase) const override;

  bool shouldPrintMemberExprUnderscore(MemberExpr *ME) const override;

  void PrintPackoffsetField(PackoffsetFieldHandler &FH, QualType Tp,
                            const Twine &FieldName,
                            ArrayWaitType WaitingForArray, CharUnits Offset);

  void PrintPackoffsetFields(raw_ostream &OS, const CXXRecordDecl *Record,
                             const Twine &PrefixName,
                             CharUnits BaseOffset = {});

  void PrintAttributeField(raw_ostream &OS, QualType Tp, const Twine &FieldName,
                           ArrayWaitType WaitingForArray, unsigned Indent,
                           unsigned &Location, unsigned ArraySize);

  void PrintAttributeFields(raw_ostream &OS, const CXXRecordDecl *Record,
                            const Twine &FieldName,
                            ArrayWaitType WaitingForArray, unsigned Indent,
                            unsigned &Location);
};

std::unique_ptr<ShaderPrintingPolicyBase>
MakePrintingPolicy(HshBuiltins &Builtins, ASTContext &Context, HshTarget Target,
                   InShaderPipelineArgsType InShaderPipelineArgs);

} // namespace clang::hshgen
