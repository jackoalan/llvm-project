//===--- ShaderPrintingPolicy.cpp - base shader stage printing policy -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ShaderPrintingPolicy.h"
#include "GLSLPrintingPolicy.h"
#include "HLSLPrintingPolicy.h"

#include "llvm/Support/raw_ostream.h"

#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecordLayout.h"

using namespace llvm;

namespace clang::hshgen {

void ShaderPrintingPolicyBase::PrintNZeros(raw_ostream &OS, unsigned N) {
  CommaArgPrinter ArgPrinter(OS);
  for (unsigned i = 0; i < N; ++i) {
    ArgPrinter.addArg() << '0';
  }
}

void ShaderPrintingPolicyBase::PrintNExprs(
    raw_ostream &OS, const std::function<void(Expr *)> &ExprArg, unsigned N,
    Expr *E) {
  CommaArgPrinter ArgPrinter(OS);
  for (unsigned i = 0; i < N; ++i) {
    ArgPrinter.addArg();
    ExprArg(E);
  }
}

template <typename ImplClass>
ShaderPrintingPolicy<ImplClass>::ShaderPrintingPolicy(
    HshBuiltins &Builtins, HshTarget Target,
    InShaderPipelineArgsType InShaderPipelineArgs)
    : ShaderPrintingPolicyBase(Builtins, Target) {
  Callbacks = this;
  Indentation = 1;
  IncludeTagDefinition = false;
  SuppressTagKeyword = true;
  SuppressScope = true;
  AnonymousTagLocations = false;
  SuppressImplicitBase = true;
  PolishForDeclaration = true;
  PrintCanonicalTypes = true;

  SuppressNestedQualifiers = true;
  SuppressListInitialization = true;
  SeparateConditionVarDecls = true;
  ConstantExprsAsInt = true;
  SilentNullStatement = true;

  for (const auto &[Name, Arg] : InShaderPipelineArgs) {
    if (Name == "early_depth_stencil")
      EarlyDepthStencil = Arg.getAsIntegral().getZExtValue();
  }
}

template <typename ImplClass>
StringRef ShaderPrintingPolicy<ImplClass>::overrideBuiltinTypeName(
    const BuiltinType *T) const {
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

template <typename ImplClass>
StringRef
ShaderPrintingPolicy<ImplClass>::overrideTagDeclIdentifier(TagDecl *D) const {
  const auto *Tp = D->getTypeForDecl();
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

template <typename ImplClass>
StringRef ShaderPrintingPolicy<ImplClass>::overrideBuiltinFunctionIdentifier(
    CallExpr *C) const {
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

template <typename ImplClass>
bool ShaderPrintingPolicy<ImplClass>::overrideCallArguments(
    CallExpr *C, const std::function<void(StringRef)> &StringArg,
    const std::function<void(Expr *)> &ExprArg) const {
  if (auto *MemberCall = dyn_cast<CXXMemberCallExpr>(C)) {
    auto HBM = Builtins.identifyBuiltinMethod(MemberCall->getMethodDecl());
    if (HBM == HBM_None)
      return {};
    return static_cast<const ImplClass &>(*this).overrideCXXMethodArguments(
        HBM, MemberCall, StringArg, ExprArg);
  }
  return false;
}

template <typename ImplClass>
StringRef ShaderPrintingPolicy<ImplClass>::overrideDeclRefIdentifier(
    DeclRefExpr *DR) const {
  if (auto *ECD = dyn_cast<EnumConstantDecl>(DR->getDecl())) {
    EnumValStr.clear();
    raw_string_ostream OS(EnumValStr);
    OS << ECD->getInitVal();
    return OS.str();
  }
  return {};
}

template <typename ImplClass>
StringRef
ShaderPrintingPolicy<ImplClass>::overrideMemberExpr(MemberExpr *ME) const {
  if (auto *FD = dyn_cast<FieldDecl>(ME->getMemberDecl())) {
    auto HPF = Builtins.identifyBuiltinPipelineField(FD);
    switch (HPF) {
    case HPF_position:
      return static_cast<const ImplClass &>(*this).identifierOfVertexPosition(
          FD);
    case HPF_color_out:
      return static_cast<const ImplClass &>(*this).identifierOfColorAttachment(
          FD);
    default:
      break;
    }
  }
  return {};
}

DeclRefExpr *ShaderPrintingPolicyBase::GetMemberExprBase(MemberExpr *ME) {
  Expr *E = ME->getBase()->IgnoreParenImpCasts();
  if (auto *OCE = dyn_cast<CXXOperatorCallExpr>(E)) {
    if (OCE->getOperator() == OO_Arrow)
      E = OCE->getArg(0)->IgnoreParenImpCasts();
  }
  return dyn_cast<DeclRefExpr>(E);
}

template <typename ImplClass>
StringRef ShaderPrintingPolicy<ImplClass>::prependMemberExprBase(
    MemberExpr *ME, bool &ReplaceBase) const {
  if (auto *DRE = GetMemberExprBase(ME)) {
    if (auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl())) {
      if (Builtins.getVertexAttributeRecord(PVD))
        return ImplClass::VertexBufferBase;
      if (Builtins.getUniformRecord(PVD)) {
        ReplaceBase = true;
        return PVD->getName();
      }
    }
    if (ImplClass::NoUniformVarDecl &&
        DRE->getDecl()->getName() == "_from_host")
      ReplaceBase = true;
  }
  return {};
}

template <typename ImplClass>
bool ShaderPrintingPolicy<ImplClass>::shouldPrintMemberExprUnderscore(
    MemberExpr *ME) const {
  if (auto *DRE = GetMemberExprBase(ME)) {
    if (auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl())) {
      if (Builtins.getVertexAttributeRecord(PVD) ||
          Builtins.getUniformRecord(PVD))
        return true;
    }
  }
  return false;
}

void ShaderPrintingPolicyBase::PrintNestedStructs(raw_ostream &OS,
                                                  ASTContext &Context) {
  unsigned Idx = 0;
  for (const auto *Record : NestedRecords) {
    OS << "struct __Struct" << Idx++ << " {\n";
    for (auto *FD : Record->fields()) {
      PrintStructField(OS, Context, FD->getType(), FD->getName(),
                       ArrayWaitType::NoArray, 1);
      OS << ";\n";
    }
    OS << "};\n";
  }
}

ShaderPrintingPolicyBase::ArrayWaitType
ShaderPrintingPolicyBase::getArrayWaitType(const CXXRecordDecl *RD) const {
  if (Builtins.isStdArrayType(RD))
    return ArrayWaitType::StdArray;
  if (Builtins.isAlignedArrayType(RD))
    return ArrayWaitType::AlignedArray;
  return ArrayWaitType::NoArray;
}

void ShaderPrintingPolicyBase::GatherNestedStructField(
    ASTContext &Context, QualType Tp, ArrayWaitType WaitingForArray) {
  if (Builtins.identifyBuiltinType(Tp) == HBT_None) {
    if (auto *SubRecord = Tp->getAsCXXRecordDecl()) {
      GatherNestedStructFields(Context, SubRecord, WaitingForArray);
      return;
    }
  }

  if (const auto *AT =
          dyn_cast_or_null<ConstantArrayType>(Tp->getAsArrayTypeUnsafe())) {
    QualType ElemType = AT->getElementType();
    if (WaitingForArray == ArrayWaitType::AlignedArray) {
      ElemType = ElemType->getAsCXXRecordDecl()->field_begin()->getType();
    }

    if (Builtins.identifyBuiltinType(ElemType) == HBT_None) {
      if (auto *SubRecord = ElemType->getAsCXXRecordDecl()) {
        GatherNestedStructFields(Context, SubRecord, ArrayWaitType::NoArray);
        return;
      }
    }

    GatherNestedStructField(Context, ElemType, ArrayWaitType::NoArray);
    return;
  }
}

void ShaderPrintingPolicyBase::GatherNestedStructFields(
    ASTContext &Context, const CXXRecordDecl *Record,
    ArrayWaitType WaitingForArray) {
  if (WaitingForArray == ArrayWaitType::NoArray)
    WaitingForArray = getArrayWaitType(Record);
  if (WaitingForArray != ArrayWaitType::NoArray) {
    for (auto *FD : Record->fields()) {
      GatherNestedStructField(Context, FD->getType(), WaitingForArray);
    }
  } else {
    for (auto *FD : Record->fields()) {
      GatherNestedStructField(Context, FD->getType(), ArrayWaitType::NoArray);
    }
    if (std::find(NestedRecords.begin(), NestedRecords.end(), Record) ==
        NestedRecords.end())
      NestedRecords.push_back(Record);
  }
}

void ShaderPrintingPolicyBase::GatherNestedPackoffsetFields(
    ASTContext &Context, const CXXRecordDecl *Record, CharUnits BaseOffset) {
  ArrayWaitType WaitingForArray = getArrayWaitType(Record);

  for (auto *FD : Record->fields()) {
    GatherNestedStructField(Context, FD->getType(), WaitingForArray);
  }
}

void ShaderPrintingPolicyBase::PrintStructField(
    raw_ostream &OS, ASTContext &Context, QualType Tp, const Twine &FieldName,
    ArrayWaitType WaitingForArray, unsigned Indent) {
  if (Builtins.identifyBuiltinType(Tp) == HBT_None) {
    if (auto *SubRecord = Tp->getAsCXXRecordDecl()) {
      PrintStructFields(OS, Context, SubRecord, FieldName, WaitingForArray,
                        Indent);
      return;
    }
  }

  if (const auto *AT =
          dyn_cast_or_null<ConstantArrayType>(Tp->getAsArrayTypeUnsafe())) {
    QualType ElemType = AT->getElementType();
    if (WaitingForArray == ArrayWaitType::AlignedArray) {
      ElemType = ElemType->getAsCXXRecordDecl()->field_begin()->getType();
    }

    unsigned Size = AT->getSize().getZExtValue();
    Twine Tw1 = Twine('[') + Twine(Size);
    Twine Tw2 = Tw1 + Twine(']');
    Twine ArrayFieldName = FieldName + Tw2;

    if (Builtins.identifyBuiltinType(ElemType) == HBT_None) {
      if (auto *SubRecord = ElemType->getAsCXXRecordDecl()) {
        PrintStructFields(OS, Context, SubRecord, ArrayFieldName,
                          ArrayWaitType::NoArray, Indent);
        return;
      }
    }

    PrintStructField(OS, Context, ElemType, ArrayFieldName,
                     ArrayWaitType::NoArray, Indent);
    return;
  }

  if (WaitingForArray == ArrayWaitType::NoArray) {
    OS.indent(Indent * 2);
    Tp.print(OS, *this, FieldName, Indent);
  }
}

void ShaderPrintingPolicyBase::PrintStructFields(
    raw_ostream &OS, ASTContext &Context, const CXXRecordDecl *Record,
    const Twine &FieldName, ArrayWaitType WaitingForArray, unsigned Indent) {
  if (WaitingForArray == ArrayWaitType::NoArray)
    WaitingForArray = getArrayWaitType(Record);
  if (WaitingForArray != ArrayWaitType::NoArray) {
    for (auto *FD : Record->fields()) {
      PrintStructField(OS, Context, FD->getType(), FieldName, WaitingForArray,
                       Indent);
    }
  } else {
    auto *Search =
        std::find(NestedRecords.begin(), NestedRecords.end(), Record);
    assert(Search != NestedRecords.end());
    OS.indent(Indent * 2) << "__Struct" << Search - NestedRecords.begin() << ' '
                          << FieldName;
  }
}

template <typename ImplClass>
void ShaderPrintingPolicy<ImplClass>::PrintPackoffsetField(
    raw_ostream &OS, ASTContext &Context, QualType Tp, const Twine &FieldName,
    ArrayWaitType WaitingForArray, CharUnits Offset) {
  assert(Offset.getQuantity() % 4 == 0);
  ImplClass::PrintBeforePackoffset(OS, Offset);
  PrintStructField(OS, Context, Tp, FieldName, WaitingForArray, 1);
  ImplClass::PrintAfterPackoffset(OS, Offset);
}

template <typename ImplClass>
void ShaderPrintingPolicy<ImplClass>::PrintPackoffsetFields(
    raw_ostream &OS, ASTContext &Context, const CXXRecordDecl *Record,
    const Twine &PrefixName, CharUnits BaseOffset) {
  ArrayWaitType WaitingForArray = getArrayWaitType(Record);

  const ASTRecordLayout &RL = Context.getASTRecordLayout(Record);
  for (auto *FD : Record->fields()) {
    auto Offset = BaseOffset + Context.toCharUnitsFromBits(
                                   RL.getFieldOffset(FD->getFieldIndex()));
    Twine Tw1 = PrefixName + Twine('_');
    PrintPackoffsetField(OS, Context, FD->getType(),
                         WaitingForArray != ArrayWaitType::NoArray
                             ? PrefixName
                             : Tw1 + FD->getName(),
                         WaitingForArray, Offset);
  }
}

template <typename ImplClass>
void ShaderPrintingPolicy<ImplClass>::PrintAttributeField(
    raw_ostream &OS, ASTContext &Context, QualType Tp, const Twine &FieldName,
    ArrayWaitType WaitingForArray, unsigned Indent, unsigned &Location,
    unsigned ArraySize) {
  if (Builtins.identifyBuiltinType(Tp) == HBT_None) {
    if (auto *SubRecord = Tp->getAsCXXRecordDecl()) {
      PrintAttributeFields(OS, Context, SubRecord, FieldName, WaitingForArray,
                           Indent, Location);
      return;
    }
  }

  if (const auto *AT =
          dyn_cast_or_null<ConstantArrayType>(Tp->getAsArrayTypeUnsafe())) {
    QualType ElemType = AT->getElementType();
    if (WaitingForArray == ArrayWaitType::AlignedArray) {
      ElemType = ElemType->getAsCXXRecordDecl()->field_begin()->getType();
    }

    unsigned Size = AT->getSize().getZExtValue();
    Twine Tw1 = Twine('[') + Twine(Size);
    Twine Tw2 = Tw1 + Twine(']');
    Twine ArrayFieldName = FieldName + Tw2;

    if (Builtins.identifyBuiltinType(ElemType) == HBT_None) {
      if (auto *SubRecord = ElemType->getAsCXXRecordDecl()) {
        PrintAttributeFields(OS, Context, SubRecord, ArrayFieldName,
                             ArrayWaitType::NoArray, Indent, Location);
        return;
      }
    }

    PrintAttributeField(OS, Context, ElemType, ArrayFieldName,
                        ArrayWaitType::NoArray, Indent, Location, Size);
    return;
  }

  if (WaitingForArray == ArrayWaitType::NoArray) {
    HshBuiltinType HBT = Builtins.identifyBuiltinType(Tp);
    if (HshBuiltins::isMatrixType(HBT)) {
      switch (HBT) {
      case HBT_float3x3:
        static_cast<const ImplClass &>(*this).PrintAttributeFieldSpelling(
            OS, Tp, FieldName, Location, Indent);
        Location += 3 * ArraySize;
        break;
      case HBT_float4x4:
        static_cast<const ImplClass &>(*this).PrintAttributeFieldSpelling(
            OS, Tp, FieldName, Location, Indent);
        Location += 4 * ArraySize;
        break;
      default:
        llvm_unreachable("Unhandled matrix type");
      }
    } else {
      static_cast<const ImplClass &>(*this).PrintAttributeFieldSpelling(
          OS, Tp, FieldName, Location, Indent);
      Location += ArraySize;
    }
  }
}

template <typename ImplClass>
void ShaderPrintingPolicy<ImplClass>::PrintAttributeFields(
    raw_ostream &OS, ASTContext &Context, const CXXRecordDecl *Record,
    const Twine &FieldName, ArrayWaitType WaitingForArray, unsigned Indent,
    unsigned &Location) {
  if (WaitingForArray == ArrayWaitType::NoArray)
    WaitingForArray = getArrayWaitType(Record);
  if (WaitingForArray != ArrayWaitType::NoArray) {
    for (auto *FD : Record->fields()) {
      PrintAttributeField(OS, Context, FD->getType(), FieldName,
                          WaitingForArray, Indent, Location, 1);
    }
  }
}

template struct ShaderPrintingPolicy<GLSLPrintingPolicy>;
template struct ShaderPrintingPolicy<HLSLPrintingPolicy>;

std::unique_ptr<ShaderPrintingPolicyBase>
MakePrintingPolicy(HshBuiltins &Builtins, HshTarget Target,
                   InShaderPipelineArgsType InShaderPipelineArgs) {
  switch (Target) {
  default:
  case HT_GLSL:
  case HT_DEKO3D:
    return std::make_unique<GLSLPrintingPolicy>(Builtins, Target,
                                                InShaderPipelineArgs);
  case HT_HLSL:
  case HT_DXBC:
  case HT_DXIL:
  case HT_VULKAN_SPIRV:
  case HT_METAL:
  case HT_METAL_BIN_MAC:
  case HT_METAL_BIN_IOS:
  case HT_METAL_BIN_TVOS:
    return std::make_unique<HLSLPrintingPolicy>(Builtins, Target,
                                                InShaderPipelineArgs);
  }
}

} // namespace clang::hshgen
