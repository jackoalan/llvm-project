//===--- HLSLPrintingPolicy.cpp - HLSL shader stage printing policy -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HLSLPrintingPolicy.h"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace clang::hshgen {

StringRef
HLSLPrintingPolicy::identifierOfCXXMethod(HshBuiltinCXXMethod HBM,
                                          CXXMemberCallExpr *C) const {
  switch (HBM) {
  case HBM_sample2d:
  case HBM_render_sample2d:
  case HBM_sample_bias2d:
  case HBM_sample2da:
  case HBM_sample_bias2da: {
    CXXMethodIdentifier.clear();
    raw_string_ostream OS(CXXMethodIdentifier);
    C->getImplicitObjectArgument()->printPretty(OS, nullptr, *this);
    OS << (HBM == HBM_sample_bias2d || HBM == HBM_sample_bias2da ? ".SampleBias"
                                                                 : ".Sample");
    return OS.str();
  }
  case HBM_read2d:
  case HBM_read2da:
  case HBM_render_read2d: {
    CXXMethodIdentifier.clear();
    raw_string_ostream OS(CXXMethodIdentifier);
    C->getImplicitObjectArgument()->printPretty(OS, nullptr, *this);
    OS << ".Load";
    return OS.str();
  }
  default:
    return {};
  }
}

bool HLSLPrintingPolicy::overrideCXXMethodArguments(
    HshBuiltinCXXMethod HBM, CXXMemberCallExpr *C,
    const std::function<void(StringRef)> &StringArg,
    const std::function<void(Expr *)> &ExprArg,
    const std::function<void(StringRef, Expr *, StringRef)> &WrappedExprArg)
    const {
  switch (HBM) {
  case HBM_sample2d:
  case HBM_render_sample2d:
  case HBM_sample_bias2d:
  case HBM_sample2da:
  case HBM_sample_bias2da: {
    auto *Search =
        std::find_if(ThisSampleCalls.begin(), ThisSampleCalls.end(),
                     [&](const auto &Other) { return C == Other.Expr; });
    assert(Search != ThisSampleCalls.end() && "sample call must exist");
    std::string SamplerArg{"_sampler"};
    raw_string_ostream OS(SamplerArg);
    OS << Search->SamplerIndex;
    StringArg(OS.str());
    ExprArg(C->getArg(0));
    if (HBM == HBM_sample_bias2d || HBM == HBM_sample_bias2da)
      ExprArg(C->getArg(1));
    return true;
  }
  case HBM_read2d:
  case HBM_render_read2d: {
    WrappedExprArg("int3(", C->getArg(0), "");
    Expr *LODStmt = C->getArg(1);
    if (auto *arg = dyn_cast<CXXDefaultArgExpr>(LODStmt)) {
      LODStmt = arg->getExpr();
    }
    WrappedExprArg("", LODStmt, ")");
    return true;
  }
  case HBM_read2da: {
    WrappedExprArg("int4(", C->getArg(0), "");
    ExprArg(C->getArg(1));
    Expr *LODStmt = C->getArg(2);
    if (auto *arg = dyn_cast<CXXDefaultArgExpr>(LODStmt)) {
      LODStmt = arg->getExpr();
    }
    WrappedExprArg("", LODStmt, ")");
    return true;
  }
  default:
    return false;
  }
}

bool HLSLPrintingPolicy::overrideCXXOperatorCall(
    CXXOperatorCallExpr *C, raw_ostream &OS,
    const std::function<void(Expr *)> &ExprArg) const {
  if (C->getNumArgs() == 1) {
    if (C->getOperator() == OO_Star) {
      /* Ignore derefs */
      ExprArg(C->getArg(0));
      return true;
    }
  } else if (C->getNumArgs() == 2) {
    if (C->getOperator() == OO_Star) {
      if (HshBuiltins::isMatrixType(
              Builtins.identifyBuiltinType(C->getArg(0)->getType())) ||
          HshBuiltins::isMatrixType(
              Builtins.identifyBuiltinType(C->getArg(1)->getType()))) {
        /* HLSL matrix math operation */
        OS << "mul(";
        ExprArg(C->getArg(0));
        OS << ", ";
        ExprArg(C->getArg(1));
        OS << ")";
        return true;
      }
    }
  }
  return false;
}

bool HLSLPrintingPolicy::overrideCXXTemporaryObjectExpr(
    CXXTemporaryObjectExpr *C, raw_ostream &OS,
    const std::function<void(Expr *)> &ExprArg) const {
  auto DTp = Builtins.identifyBuiltinType(C->getType());
  if (C->getNumArgs() == 1) {
    auto STp = Builtins.identifyBuiltinType(C->getArg(0)->getType());
    switch (DTp) {
    case HBT_float3x3:
      switch (STp) {
      case HBT_float4x4:
        OS << "float4x4_to_float3x3(";
        ExprArg(C->getArg(0));
        OS << ")";
        return true;
      default:
        break;
      }
      break;
    default:
      break;
    }
  }
  if (HshBuiltins::isVectorType(DTp)) {
    if (C->getNumArgs() == 0) {
      /* Implicit zero vector construction */
      OS << HshBuiltins::getSpelling<SourceTarget>(DTp) << '(';
      PrintNZeros(OS, HshBuiltins::getVectorSize(DTp));
      OS << ')';
      return true;
    } else if (C->getNumArgs() == 1 &&
               !HshBuiltins::isVectorType(
                   Builtins.identifyBuiltinType(C->getArg(0)->getType()))) {
      /* Implicit scalar-to-vector conversion */
      OS << HshBuiltins::getSpelling<SourceTarget>(DTp) << '(';
      PrintNExprs(OS, ExprArg, HshBuiltins::getVectorSize(DTp), C->getArg(0));
      OS << ')';
      return true;
    }
  }
  return false;
}

void HLSLPrintingPolicy::PrintAttributeFieldSpelling(raw_ostream &OS,
                                                     QualType Tp,
                                                     const Twine &FieldName,
                                                     unsigned Location,
                                                     unsigned Indent) const {
  if (Target == HT_VULKAN_SPIRV)
    OS.indent(Indent * 2) << "[[vk::location(" << Location << ")]] ";
  else
    OS.indent(Indent * 2);
  Tp.print(OS, *this);
  OS << " " << FieldName << " : ATTR" << Location << ";\n";
}

constexpr llvm::StringLiteral HLSLRuntimeSupport{
    R"(static float3x3 float4x4_to_float3x3(float4x4 mtx) {
  return float3x3(mtx[0].xyz, mtx[1].xyz, mtx[2].xyz);
}
)"};

void HLSLPrintingPolicy::printStage(
    raw_ostream &OS, ArrayRef<FunctionRecord> FunctionRecords,
    ArrayRef<UniformRecord> UniformRecords, CXXRecordDecl *FromRecord,
    CXXRecordDecl *ToRecord, ArrayRef<AttributeRecord> Attributes,
    ArrayRef<TextureRecord> Textures, ArrayRef<SamplerBinding> Samplers,
    unsigned NumColorAttachments, bool HasDualSource, CompoundStmt *Stmts,
    HshStage Stage, HshStage From, HshStage To,
    ArrayRef<SampleCall> SampleCalls,
    std::bitset<HPF_Max> ReferencedPipelineFields) {
  if (HasDualSource)
    ++NumColorAttachments;
  OS << HLSLRuntimeSupport;
  ThisStmts = Stmts;
  ThisSampleCalls = SampleCalls;

  auto PrintFunction = [&](const FunctionDecl *FD) {
    OS << "static ";
    SuppressSpecifiers = false;
    FD->getReturnType().print(OS, *this);
    OS << ' ';
    SuppressSpecifiers = true;
    FD->print(OS, *this);
  };
  bool OldTerseOutput = TerseOutput;
  TerseOutput = true;
  bool OldSuppressSpecifiers = SuppressSpecifiers;
  for (const auto &Function : FunctionRecords) {
    if ((1u << Stage) & Function.UseStages) {
      PrintFunction(Function.Function);
      OS << ";\n";
    }
  }
  TerseOutput = false;
  for (const auto &Function : FunctionRecords) {
    if ((1u << Stage) & Function.UseStages) {
      PrintFunction(Function.Function);
    }
  }
  TerseOutput = OldTerseOutput;
  SuppressSpecifiers = OldSuppressSpecifiers;

  NestedRecords.clear();
  for (const auto &Record : UniformRecords) {
    if ((1u << Stage) & Record.UseStages)
      GatherNestedPackoffsetFields(Record.Record);
  }

  PrintNestedStructs<FieldPrinter>(OS);

  unsigned Binding = 0;
  for (const auto &Record : UniformRecords) {
    if ((1u << Stage) & Record.UseStages) {
      OS << "cbuffer b" << Binding << '_' << Record.Record->getName()
         << " : register(b" << Binding << ") {\n";
      PrintPackoffsetFields(OS, Record.Record, Record.Name);
      OS << "};\n";
    }
    ++Binding;
  }

  if (FromRecord) {
    OS << "struct " << HshStageToString(From) << "_to_"
       << HshStageToString(Stage) << " {\n";
    uint32_t VarIdx = 0;
    for (auto *FD : FromRecord->fields()) {
      OS << "  ";
      FD->print(OS, *this, 1);
      OS << " : VAR" << VarIdx++ << ";\n";
    }
    OS << "};\n";
  }

  if (ToRecord) {
    OS << "struct " << HshStageToString(Stage) << "_to_" << HshStageToString(To)
       << " {\n"
       << "  float4 _position : SV_Position;\n";
    uint32_t VarIdx = 0;
    for (auto *FD : ToRecord->fields()) {
      OS << "  ";
      FD->print(OS, *this, 1);
      OS << " : VAR" << VarIdx++ << ";\n";
    }
    OS << "};\n";
  }

  if (Stage == HshVertexStage) {
    OS << "struct host_vert_data {\n";
    unsigned Location = 0;
    for (const auto &Attribute : Attributes) {
      for (const auto *FD : Attribute.Record->fields()) {
        QualType Tp = FD->getType().getUnqualifiedType();
        Twine Tw1 = Twine(Attribute.Name) + Twine('_');
        PrintAttributeField(OS, Tp, Tw1 + FD->getName(), ArrayWaitType::NoArray,
                            1, Location, 1);
      }
    }
    OS << "};\n";
  }

  uint32_t TexBinding = 0;
  for (const auto &Tex : Textures) {
    if ((1u << Stage) & Tex.UseStages)
      OS << HshBuiltins::getSpelling<SourceTarget>(
                BuiltinTypeOfTexture(Tex.Kind))
         << " " << Tex.TexParm->getName() << " : register(t" << TexBinding
         << ");\n";
    ++TexBinding;
  }

  uint32_t SamplerBinding = 0;
  for (const auto &Samp : Samplers) {
    if ((1u << Stage) & Samp.UseStages)
      OS << "SamplerState _sampler" << SamplerBinding << " : register(s"
         << SamplerBinding << ");\n";
    ++SamplerBinding;
  }

  if (Stage == HshFragmentStage) {
    OS << "struct color_targets_out {\n"
          "  float4 _color_out["
       << NumColorAttachments << "] : SV_Target"
       << ";\n"
          "};\n";
  }

  if (Stage == HshFragmentStage) {
    if (EarlyDepthStencil)
      OS << "[earlydepthstencil]\n";
    OS << "color_targets_out main(";
    BeforeStatements = "color_targets_out _targets_out;\n";
    AfterStatements = "return _targets_out;\n";
  } else if (ToRecord) {
    VertexPositionIdentifier.clear();
    raw_string_ostream PIO(VertexPositionIdentifier);
    PIO << "_to_" << HshStageToString(To) << "._position";
    OS << HshStageToString(Stage) << "_to_" << HshStageToString(To) << " main(";
    BeforeStatements.clear();
    raw_string_ostream BO(BeforeStatements);
    BO << HshStageToString(Stage) << "_to_" << HshStageToString(To) << " _to_"
       << HshStageToString(To) << ";\n";
    AfterStatements.clear();
    raw_string_ostream AO(AfterStatements);
    AO << "return _to_" << HshStageToString(To) << ";\n";
  }
  if (Stage == HshVertexStage) {
    OS << "in host_vert_data _vert_data";
    if (ReferencedPipelineFields[HPF_vertex_id])
      OS << ", uint _vertex_id : SV_VertexID";
    if (ReferencedPipelineFields[HPF_instance_id])
      OS << ", uint _instance_id : SV_InstanceID";
  } else if (FromRecord)
    OS << "in " << HshStageToString(From) << "_to_" << HshStageToString(Stage)
       << " _from_" << HshStageToString(From);
  OS << ") ";
  Stmts->printPretty(OS, nullptr, *this);
}

} // namespace clang::hshgen
