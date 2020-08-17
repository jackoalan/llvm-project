//===--- MetalPrintingPolicy.cpp - Metal shader stage printing policy -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MetalPrintingPolicy.h"

#include "llvm/Support/raw_ostream.h"

#include "clang/AST/RecordLayout.h"

using namespace llvm;

namespace clang::hshgen {

StringRef
MetalPrintingPolicy::identifierOfCXXMethod(HshBuiltinCXXMethod HBM,
                                           CXXMemberCallExpr *C) const {
  switch (HBM) {
  case HBM_sample2d:
  case HBM_render_sample2d:
  case HBM_sample_bias2d: {
    CXXMethodIdentifier.clear();
    raw_string_ostream OS(CXXMethodIdentifier);
    C->getImplicitObjectArgument()->printPretty(OS, nullptr, *this);
    OS << ".sample";
    return OS.str();
  }
  default:
    return {};
  }
}

bool MetalPrintingPolicy::overrideCXXMethodArguments(
    HshBuiltinCXXMethod HBM, CXXMemberCallExpr *C,
    const std::function<void(StringRef)> &StringArg,
    const std::function<void(Expr *)> &ExprArg,
    const std::function<void(StringRef, Expr *, StringRef)> &WrappedExprArg)
    const {
  switch (HBM) {
  case HBM_sample2d:
  case HBM_render_sample2d:
  case HBM_sample_bias2d: {
    auto *Search =
        std::find_if(ThisSampleCalls.begin(), ThisSampleCalls.end(),
                     [&](const auto &Other) { return C == Other.Expr; });
    assert(Search != ThisSampleCalls.end() && "sample call must exist");
    std::string SamplerArg{"_sampler"};
    raw_string_ostream OS(SamplerArg);
    OS << Search->SamplerIndex;
    StringArg(OS.str());
    ExprArg(C->getArg(0));
    if (HBM == HBM_sample_bias2d)
      WrappedExprArg("bias(", C->getArg(1), ")");
    return true;
  }
  default:
    return false;
  }
}

bool MetalPrintingPolicy::overrideCXXOperatorCall(
    CXXOperatorCallExpr *C, raw_ostream &OS,
    const std::function<void(Expr *)> &ExprArg) const {
  if (C->getNumArgs() == 1) {
    if (C->getOperator() == OO_Star) {
      /* Ignore derefs */
      ExprArg(C->getArg(0));
      return true;
    }
  } else if (C->getNumArgs() == 2) {
    if (C->getOperator() == OO_Subscript) {
      if (auto *ME = dyn_cast<MemberExpr>(C->getArg(0))) {
        if (auto *FD = dyn_cast<FieldDecl>(ME->getMemberDecl())) {
          if (Builtins.identifyBuiltinPipelineField(FD) == HPF_color_out) {
            /* Remove subscripts from color out field */
            ExprArg(C->getArg(0));
            if (Optional<APSInt> Result =
                    C->getArg(1)->getIntegerConstantExpr(Context)) {
              OS << Result;
              return true;
            } else {
              auto &Diags =
                  const_cast<DiagnosticsEngine &>(Context.getDiagnostics());
              Diags.Report(Diags.getCustomDiagID(
                  DiagnosticsEngine::Error, "Non-constexpr color_out indicies "
                                            "are not permitted in metal"));
            }
          }
        }
      }
    }
  }
  return false;
}

bool MetalPrintingPolicy::overrideCXXTemporaryObjectExpr(
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

void MetalPrintingPolicy::PrintAttributeFieldSpelling(raw_ostream &OS,
                                                      QualType Tp,
                                                      const Twine &FieldName,
                                                      unsigned Location,
                                                      unsigned Indent) const {
  OS.indent(Indent * 2);
  Tp.print(OS, *this);
  OS << " " << FieldName << " [[attribute(" << Location << ")]];\n";
}

constexpr llvm::StringLiteral MetalRuntimeSupport{
    R"(#include <metal_stdlib>
using namespace metal;
static float3x3 float4x4_to_float3x3(float4x4 mtx) {
  return float3x3(mtx[0].xyz, mtx[1].xyz, mtx[2].xyz);
}
)"};

constexpr StringRef HshStageToMetalFunction(HshStage Stage) {
  switch (Stage) {
  case HshVertexStage:
    return "vertex";
  case HshEvaluationStage:
    return "patch";
  case HshFragmentStage:
    return "fragment";
  default:
    return "kernel";
  }
}

void MetalPrintingPolicy::printStage(
    raw_ostream &OS, ArrayRef<FunctionRecord> FunctionRecords,
    ArrayRef<UniformRecord> UniformRecords, CXXRecordDecl *FromRecord,
    CXXRecordDecl *ToRecord, ArrayRef<AttributeRecord> Attributes,
    ArrayRef<TextureRecord> Textures, ArrayRef<SamplerBinding> Samplers,
    unsigned NumColorAttachments, CompoundStmt *Stmts, HshStage Stage,
    HshStage From, HshStage To, ArrayRef<SampleCall> SampleCalls) {
  OS << MetalRuntimeSupport;
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

  PrintNestedStructs<FieldFloatPairer>(OS);

  unsigned Binding = 0;
  for (const auto &Record : UniformRecords) {
    if ((1u << Stage) & Record.UseStages) {
      OS << "struct b" << Binding << '_' << Record.Record->getName() << " {\n";
      PrintPackoffsetFields(OS, Record.Record, Twine());
      OS << "};\n";

      /* Add static_asserts of all fields to ensure layout consistency */
      if (getArrayWaitType(Record.Record) == ArrayWaitType::NoArray) {
        const ASTRecordLayout &RL = Context.getASTRecordLayout(Record.Record);
        for (auto *FD : Record.Record->fields()) {
          auto Offset = Context.toCharUnitsFromBits(
              RL.getFieldOffset(FD->getFieldIndex()));
          OS << "static_assert(__builtin_offsetof(b" << Binding << '_'
             << Record.Record->getName() << ", " << FD->getName()
             << ") == " << Offset.getQuantity()
             << ", \"compiler does not align field correctly\");\n";
        }
      }
    }
    ++Binding;
  }

  if (FromRecord) {
    OS << "struct " << HshStageToString(From) << "_to_"
       << HshStageToString(Stage) << " {\n";
    for (auto *FD : FromRecord->fields()) {
      OS << "  ";
      FD->print(OS, *this, 1);
      OS << ";\n";
    }
    OS << "};\n";
  }

  if (ToRecord) {
    OS << "struct " << HshStageToString(Stage) << "_to_" << HshStageToString(To)
       << " {\n"
       << "  float4 _position [[position]];\n";
    for (auto *FD : ToRecord->fields()) {
      OS << "  ";
      FD->print(OS, *this, 1);
      OS << ";\n";
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

  if (Stage == HshFragmentStage) {
    OS << "struct color_targets_out {\n";
    for (unsigned A = 0; A < NumColorAttachments; ++A)
      OS << "  float4 _color_out" << A << " [[color(" << A << ")]];\n";
    OS << "};\n";
  }

  if (Stage == HshFragmentStage) {
    if (EarlyDepthStencil)
      OS << "[[early_fragment_tests]]\n";
    OS << "fragment color_targets_out shader_main(";
    BeforeStatements = "color_targets_out _targets_out;\n";
    AfterStatements = "return _targets_out;\n";
  } else if (ToRecord) {
    VertexPositionIdentifier.clear();
    raw_string_ostream PIO(VertexPositionIdentifier);
    PIO << "_to_" << HshStageToString(To) << "._position";
    OS << HshStageToMetalFunction(Stage) << ' ' << HshStageToString(Stage)
       << "_to_" << HshStageToString(To) << " shader_main(";
    BeforeStatements.clear();
    raw_string_ostream BO(BeforeStatements);
    BO << HshStageToString(Stage) << "_to_" << HshStageToString(To) << " _to_"
       << HshStageToString(To) << ";\n";
    AfterStatements.clear();
    raw_string_ostream AO(AfterStatements);
    AO << "return _to_" << HshStageToString(To) << ";\n";
  }
  if (Stage == HshVertexStage)
    OS << "host_vert_data _vert_data [[stage_in]]";
  else if (FromRecord)
    OS << HshStageToString(From) << "_to_" << HshStageToString(Stage)
       << " _from_" << HshStageToString(From) << " [[stage_in]]";

  Binding = 0;
  for (const auto &Record : UniformRecords) {
    if ((1u << Stage) & Record.UseStages) {
      OS << ", constant b" << Binding << '_' << Record.Record->getName() << " &"
         << Record.Name << " [[buffer("
         << Binding + Builtins.getMaxVertexBuffers() << ")]]";
    }
    ++Binding;
  }

  uint32_t TexBinding = 0;
  for (const auto &Tex : Textures) {
    if ((1u << Stage) & Tex.UseStages)
      OS << ", "
         << HshBuiltins::getSpelling<SourceTarget>(
                BuiltinTypeOfTexture(Tex.Kind))
         << ' ' << Tex.TexParm->getName() << " [[texture(" << TexBinding
         << ")]]";
    ++TexBinding;
  }

  uint32_t SamplerBinding = 0;
  for (const auto &Samp : Samplers) {
    if ((1u << Stage) & Samp.UseStages)
      OS << ", sampler _sampler" << SamplerBinding << " [[sampler("
         << SamplerBinding << ")]]";
    ++SamplerBinding;
  }

  OS << ") ";
  Stmts->printPretty(OS, nullptr, *this);
}

} // namespace clang::hshgen
