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
#include "MetalPrintingPolicy.h"

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

template <typename ImplClass, typename PackoffsetFieldHandler>
ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::ShaderPrintingPolicy(
    HshBuiltins &Builtins, ASTContext &Context, HshTarget Target,
    InShaderPipelineArgsType InShaderPipelineArgs)
    : ShaderPrintingPolicyBase(Builtins, Context, Target) {
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

template <typename ImplClass, typename PackoffsetFieldHandler>
StringRef ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    overrideBuiltinTypeName(const BuiltinType *T) const {
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

template <typename ImplClass, typename PackoffsetFieldHandler>
StringRef ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    overrideTagDeclIdentifier(TagDecl *D) const {
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

template <typename ImplClass, typename PackoffsetFieldHandler>
StringRef ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    overrideBuiltinFunctionIdentifier(CallExpr *C) const {
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

template <typename ImplClass, typename PackoffsetFieldHandler>
bool ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    overrideCallArguments(
        CallExpr *C, const std::function<void(StringRef)> &StringArg,
        const std::function<void(Expr *)> &ExprArg,
        const std::function<void(StringRef, Expr *, StringRef)> &WrappedExprArg)
        const {
  if (auto *MemberCall = dyn_cast<CXXMemberCallExpr>(C)) {
    auto HBM = Builtins.identifyBuiltinMethod(MemberCall->getMethodDecl());
    if (HBM == HBM_None)
      return {};
    return static_cast<const ImplClass &>(*this).overrideCXXMethodArguments(
        HBM, MemberCall, StringArg, ExprArg, WrappedExprArg);
  }
  return false;
}

template <typename ImplClass, typename PackoffsetFieldHandler>
StringRef ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    overrideDeclRefIdentifier(DeclRefExpr *DR) const {
  if (auto *ECD = dyn_cast<EnumConstantDecl>(DR->getDecl())) {
    EnumValStr.clear();
    raw_string_ostream OS(EnumValStr);
    OS << ECD->getInitVal();
    return OS.str();
  }
  return {};
}

template <typename ImplClass, typename PackoffsetFieldHandler>
StringRef
ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::overrideMemberExpr(
    MemberExpr *ME) const {
  if (auto *FD = dyn_cast<FieldDecl>(ME->getMemberDecl())) {
    auto HPF = Builtins.identifyBuiltinPipelineField(FD);
    switch (HPF) {
    case HPF_position:
      return static_cast<const ImplClass &>(*this).identifierOfVertexPosition(
          FD);
    case HPF_color_out:
      return static_cast<const ImplClass &>(*this).identifierOfColorAttachment(
          FD);
    case HPF_vertex_id:
      return static_cast<const ImplClass &>(*this).identifierOfVertexID(FD);
    case HPF_instance_id:
      return static_cast<const ImplClass &>(*this).identifierOfInstanceID(FD);
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

template <typename ImplClass, typename PackoffsetFieldHandler>
StringRef
ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::prependMemberExprBase(
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

template <typename ImplClass, typename PackoffsetFieldHandler>
bool ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    shouldPrintMemberExprUnderscore(MemberExpr *ME) const {
  if (auto *DRE = GetMemberExprBase(ME)) {
    if (auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl())) {
      if (Builtins.getVertexAttributeRecord(PVD) ||
          (ImplClass::NoUniformVarDecl && Builtins.getUniformRecord(PVD)))
        return true;
    }
  }
  return false;
}

template <typename FieldHandler>
void ShaderPrintingPolicyBase::PrintNestedStructs(raw_ostream &OS) {
  unsigned Idx = 0;
  for (const auto *Record : NestedRecords) {
    OS << "struct __Struct" << Idx++ << " {\n";
    FieldHandler FH(*this, OS);
    for (auto *FD : Record->fields()) {
      PrintStructField(FH, FD->getType(), FD->getName(), false,
                       ArrayWaitType::NoArray, 1);
      FH << ";\n";
    }
    FH.Flush();
    OS << "};\n";
  }
}
template void ShaderPrintingPolicyBase::PrintNestedStructs<
    ShaderPrintingPolicyBase::FieldPrinter>(raw_ostream &OS);
template void ShaderPrintingPolicyBase::PrintNestedStructs<
    ShaderPrintingPolicyBase::FieldFloatPairer>(raw_ostream &OS);

ShaderPrintingPolicyBase::ArrayWaitType
ShaderPrintingPolicyBase::getArrayWaitType(const CXXRecordDecl *RD) const {
  if (Builtins.isStdArrayType(RD))
    return ArrayWaitType::StdArray;
  if (Builtins.isAlignedArrayType(RD))
    return ArrayWaitType::AlignedArray;
  return ArrayWaitType::NoArray;
}

void ShaderPrintingPolicyBase::GatherNestedStructField(
    QualType Tp, ArrayWaitType WaitingForArray) {
  if (Builtins.identifyBuiltinType(Tp) == HBT_None) {
    if (auto *SubRecord = Tp->getAsCXXRecordDecl()) {
      GatherNestedStructFields(SubRecord, WaitingForArray);
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
        GatherNestedStructFields(SubRecord, ArrayWaitType::NoArray);
        return;
      }
    }

    GatherNestedStructField(ElemType, ArrayWaitType::NoArray);
    return;
  }
}

void ShaderPrintingPolicyBase::GatherNestedStructFields(
    const CXXRecordDecl *Record, ArrayWaitType WaitingForArray) {
  if (WaitingForArray == ArrayWaitType::NoArray)
    WaitingForArray = getArrayWaitType(Record);
  if (WaitingForArray != ArrayWaitType::NoArray) {
    for (auto *FD : Record->fields()) {
      GatherNestedStructField(FD->getType(), WaitingForArray);
    }
  } else {
    for (auto *FD : Record->fields()) {
      GatherNestedStructField(FD->getType(), ArrayWaitType::NoArray);
    }
    if (std::find(NestedRecords.begin(), NestedRecords.end(), Record) ==
        NestedRecords.end())
      NestedRecords.push_back(Record);
  }
}

void ShaderPrintingPolicyBase::GatherNestedPackoffsetFields(
    const CXXRecordDecl *Record, CharUnits BaseOffset) {
  ArrayWaitType WaitingForArray = getArrayWaitType(Record);

  for (auto *FD : Record->fields()) {
    GatherNestedStructField(FD->getType(), WaitingForArray);
  }
}

template <typename FieldHandler>
void ShaderPrintingPolicyBase::PrintStructField(FieldHandler &FH, QualType Tp,
                                                const Twine &FieldName,
                                                bool ArrayField,
                                                ArrayWaitType WaitingForArray,
                                                unsigned Indent) {
  if (Builtins.identifyBuiltinType(Tp) == HBT_None) {
    if (auto *SubRecord = Tp->getAsCXXRecordDecl()) {
      PrintStructFields(FH, SubRecord, FieldName, ArrayField, WaitingForArray,
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
        PrintStructFields(FH, SubRecord, ArrayFieldName, true,
                          ArrayWaitType::NoArray, Indent);
        return;
      }
    }

    PrintStructField(FH, ElemType, ArrayFieldName, true, ArrayWaitType::NoArray,
                     Indent);
    return;
  }

  if (WaitingForArray == ArrayWaitType::NoArray) {
    FH.Field(Tp, FieldName, ArrayField, Indent);
    // OS.emplace_back(Tp, FieldName, Indent);
    // OS.indent(Indent * 2);
    // Tp.print(OS, *this, FieldName, Indent);
  }
}

template <typename FieldHandler>
void ShaderPrintingPolicyBase::PrintStructFields(
    FieldHandler &FH, const CXXRecordDecl *Record, const Twine &FieldName,
    bool ArrayField, ArrayWaitType WaitingForArray, unsigned Indent) {
  if (WaitingForArray == ArrayWaitType::NoArray)
    WaitingForArray = getArrayWaitType(Record);
  if (WaitingForArray != ArrayWaitType::NoArray) {
    for (auto *FD : Record->fields()) {
      PrintStructField(FH, FD->getType(), FieldName, ArrayField,
                       WaitingForArray, Indent);
    }
  } else {
    auto *Search =
        std::find(NestedRecords.begin(), NestedRecords.end(), Record);
    assert(Search != NestedRecords.end());
    FH.NestedField(Search - NestedRecords.begin(), FieldName, Indent);
    // OS.emplace_back(Search - NestedRecords.begin(), FieldName, Indent);
    // OS.indent(Indent * 2) << "__Struct" << Search - NestedRecords.begin() <<
    // ' '
    //                      << FieldName;
  }
}

template <typename ImplClass, typename PackoffsetFieldHandler>
void ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    PrintPackoffsetField(PackoffsetFieldHandler &FH, QualType Tp,
                         const Twine &FieldName, ArrayWaitType WaitingForArray,
                         CharUnits Offset) {
  assert(Offset.getQuantity() % 4 == 0);
  ImplClass::PrintBeforePackoffset(FH, Offset);
  PrintStructField(FH, Tp, FieldName, false, WaitingForArray, 1);
  ImplClass::PrintAfterPackoffset(FH, Offset);
}

template <typename ImplClass, typename PackoffsetFieldHandler>
void ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    PrintPackoffsetFields(raw_ostream &OS, const CXXRecordDecl *Record,
                          const Twine &PrefixName, CharUnits BaseOffset) {
  ArrayWaitType WaitingForArray = getArrayWaitType(Record);

  const ASTRecordLayout &RL = Context.getASTRecordLayout(Record);
  PackoffsetFieldHandler FH(*this, OS);
  for (auto *FD : Record->fields()) {
    auto Offset = BaseOffset + Context.toCharUnitsFromBits(
                                   RL.getFieldOffset(FD->getFieldIndex()));
    Twine Tw1 =
        PrefixName.isTriviallyEmpty() ? PrefixName : PrefixName + Twine('_');
    PrintPackoffsetField(FH, FD->getType(),
                         WaitingForArray != ArrayWaitType::NoArray
                             ? PrefixName
                             : Tw1 + FD->getName(),
                         WaitingForArray, Offset);
  }
  FH.Flush();
}

template <typename ImplClass, typename PackoffsetFieldHandler>
void ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    PrintAttributeField(raw_ostream &OS, QualType Tp, const Twine &FieldName,
                        ArrayWaitType WaitingForArray, unsigned Indent,
                        unsigned &Location, unsigned ArraySize) {
  if (Builtins.identifyBuiltinType(Tp) == HBT_None) {
    if (auto *SubRecord = Tp->getAsCXXRecordDecl()) {
      PrintAttributeFields(OS, SubRecord, FieldName, WaitingForArray, Indent,
                           Location);
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
        PrintAttributeFields(OS, SubRecord, ArrayFieldName,
                             ArrayWaitType::NoArray, Indent, Location);
        return;
      }
    }

    PrintAttributeField(OS, ElemType, ArrayFieldName, ArrayWaitType::NoArray,
                        Indent, Location, Size);
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

template <typename ImplClass, typename PackoffsetFieldHandler>
void ShaderPrintingPolicy<ImplClass, PackoffsetFieldHandler>::
    PrintAttributeFields(raw_ostream &OS, const CXXRecordDecl *Record,
                         const Twine &FieldName, ArrayWaitType WaitingForArray,
                         unsigned Indent, unsigned &Location) {
  if (WaitingForArray == ArrayWaitType::NoArray)
    WaitingForArray = getArrayWaitType(Record);
  if (WaitingForArray != ArrayWaitType::NoArray) {
    for (auto *FD : Record->fields()) {
      PrintAttributeField(OS, FD->getType(), FieldName, WaitingForArray, Indent,
                          Location, 1);
    }
  }
}

template struct ShaderPrintingPolicy<GLSLPrintingPolicy,
                                     ShaderPrintingPolicyBase::FieldPrinter>;
template struct ShaderPrintingPolicy<HLSLPrintingPolicy,
                                     ShaderPrintingPolicyBase::FieldPrinter>;
template struct ShaderPrintingPolicy<
    MetalPrintingPolicy, ShaderPrintingPolicyBase::FieldFloatPairer>;

std::unique_ptr<ShaderPrintingPolicyBase>
MakePrintingPolicy(HshBuiltins &Builtins, ASTContext &Context, HshTarget Target,
                   InShaderPipelineArgsType InShaderPipelineArgs) {
  switch (Target) {
  default:
  case HT_GLSL:
  case HT_DEKO3D:
    return std::make_unique<GLSLPrintingPolicy>(Builtins, Context, Target,
                                                InShaderPipelineArgs);
  case HT_HLSL:
  case HT_DXBC:
  case HT_DXIL:
  case HT_VULKAN_SPIRV:
    return std::make_unique<HLSLPrintingPolicy>(Builtins, Context, Target,
                                                InShaderPipelineArgs);
  case HT_METAL:
  case HT_METAL_BIN_MAC:
  case HT_METAL_BIN_IOS:
    return std::make_unique<MetalPrintingPolicy>(Builtins, Context, Target,
                                                 InShaderPipelineArgs);
  }
}

} // namespace clang::hshgen
