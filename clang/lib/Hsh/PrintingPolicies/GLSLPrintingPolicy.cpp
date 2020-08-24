//===--- GLSLPrintingPolicy.cpp - GLSL shader stage printing policy -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "GLSLPrintingPolicy.h"

namespace clang::hshgen {

bool GLSLPrintingPolicy::overrideCXXOperatorCall(
    CXXOperatorCallExpr *C, raw_ostream &OS,
    const std::function<void(Expr *)> &ExprArg) const {
  if (C->getNumArgs() == 1) {
    if (C->getOperator() == OO_Star) {
      /* Ignore derefs */
      ExprArg(C->getArg(0));
      return true;
    }
  }
  return false;
}

bool GLSLPrintingPolicy::overrideCXXTemporaryObjectExpr(
    CXXTemporaryObjectExpr *C, raw_ostream &OS,
    const std::function<void(Expr *)> &ExprArg) const {
  auto DTp = Builtins.identifyBuiltinType(C->getType());
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

void GLSLPrintingPolicy::PrintAttributeFieldSpelling(raw_ostream &OS,
                                                     QualType Tp,
                                                     const Twine &FieldName,
                                                     unsigned Location,
                                                     unsigned Indent) const {
  OS << "layout(location = " << Location << ") in ";
  Tp.print(OS, *this);
  OS << " " << FieldName << ";\n";
}

constexpr llvm::StringLiteral GLSLRuntimeSupport{
    R"(#version 450 core
float saturate(float val) {
  return clamp(val, 0.0, 1.0);
}
vec2 saturate(vec2 val) {
  return clamp(val, vec2(0.0, 0.0), vec2(1.0, 1.0));
}
vec3 saturate(vec3 val) {
  return clamp(val, vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0));
}
vec4 saturate(vec4 val) {
  return clamp(val, vec4(0.0, 0.0, 0.0, 0.0), vec4(1.0, 1.0, 1.0, 1.0));
}
)"};

void GLSLPrintingPolicy::printStage(
    raw_ostream &OS, ArrayRef<FunctionRecord> FunctionRecords,
    ArrayRef<UniformRecord> UniformRecords, CXXRecordDecl *FromRecord,
    CXXRecordDecl *ToRecord, ArrayRef<AttributeRecord> Attributes,
    ArrayRef<TextureRecord> Textures, ArrayRef<SamplerBinding> Samplers,
    unsigned NumColorAttachments, bool HasDualSource, CompoundStmt *Stmts,
    HshStage Stage, HshStage From, HshStage To,
    ArrayRef<SampleCall> SampleCalls) {
  if (HasDualSource)
    ++NumColorAttachments;
  OS << GLSLRuntimeSupport;

  auto PrintFunction = [&](const FunctionDecl *FD) {
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
      OS << "layout(std140, binding = " << Binding << ") uniform b" << Binding
         << '_' << Record.Record->getName() << " {\n";
      PrintPackoffsetFields(OS, Record.Record, Record.Name);
      OS << "};\n";
    }
    ++Binding;
  }

  if (FromRecord && !FromRecord->fields().empty()) {
    OS << "layout(location = 0) in " << HshStageToString(From) << "_to_"
       << HshStageToString(Stage) << " {\n";
    for (auto *FD : FromRecord->fields()) {
      OS << "  ";
      FD->print(OS, *this, 1);
      OS << ";\n";
    }
    OS << "} _from_" << HshStageToString(From) << ";\n";
  }

  if (ToRecord && !ToRecord->fields().empty()) {
    OS << "layout(location = 0) out " << HshStageToString(Stage) << "_to_"
       << HshStageToString(To) << " {\n";
    for (auto *FD : ToRecord->fields()) {
      OS << "  ";
      FD->print(OS, *this, 1);
      OS << ";\n";
    }
    OS << "} _to_" << HshStageToString(To) << ";\n";
  }

  if (Stage == HshVertexStage) {
    uint32_t Location = 0;
    for (const auto &Attribute : Attributes) {
      for (const auto *FD : Attribute.Record->fields()) {
        QualType Tp = FD->getType().getUnqualifiedType();
        Twine Tw1 = Twine(Attribute.Name) + Twine('_');
        PrintAttributeField(OS, Tp, Tw1 + FD->getName(), ArrayWaitType::NoArray,
                            0, Location, 1);
      }
    }
  }

  uint32_t TexBinding = 0;
  for (const auto &Tex : Textures) {
    if ((1u << Stage) & Tex.UseStages)
      OS << "layout(binding = " << TexBinding << ") uniform "
         << HshBuiltins::getSpelling<SourceTarget>(
                BuiltinTypeOfTexture(Tex.Kind))
         << " " << Tex.TexParm->getName() << ";\n";
    ++TexBinding;
  }

  if (Stage == HshFragmentStage) {
    OS << "layout(location = 0) out vec4 _color_out[" << NumColorAttachments
       << "];\n";
    if (EarlyDepthStencil)
      OS << "layout(early_fragment_tests) in;\n";
  }

  OS << "void main() ";
  Stmts->printPretty(OS, nullptr, *this);
}

} // namespace clang::hshgen
