//===--- Builtins.cpp - hsh builtins declaration manager ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Builtins.h"
#include "../Reporter.h"
#include "NonConstExpr.h"

#include "clang/AST/Attr.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/QualTypeNames.h"
#include "clang/Sema/Sema.h"

using namespace llvm;
using namespace clang;

namespace {
QualType ResolveParmType(const VarDecl *D) {
  if (D->getType()->isTemplateTypeParmType())
    if (const auto *Init = D->getInit())
      return Init->getType();
  return D->getType();
}
} // namespace

namespace clang::hshgen {

bool HshBuiltins::PipelineAttributes::VisitDecl(Decl *D) {
  if (auto *DC = dyn_cast<DeclContext>(D))
    for (Decl *Child : DC->decls())
      if (!base::Visit(Child))
        return false;
  return true;
}

bool HshBuiltins::PipelineAttributes::VisitClassTemplateDecl(
    ClassTemplateDecl *CTD) {
  if (CTD->getName() == "base_attribute") {
    BaseAttributeDecl = CTD;
    return true;
  }
  if (CTD->getName() == "pipeline") {
    PipelineDecl = CTD;
    return true;
  }
  if (BaseAttributeDecl) {
    auto *TemplatedDecl = CTD->getTemplatedDecl();
    if (TemplatedDecl->getNumBases() != 1)
      return true;
    if (auto *BaseSpec = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
            TemplatedDecl->bases_begin()->getType()->getAsCXXRecordDecl())) {
      if (BaseSpec->getSpecializedTemplateOrPartial()
              .get<ClassTemplateDecl *>() == BaseAttributeDecl) {
        if (BaseSpec->getTemplateArgs()[0].getAsIntegral().getZExtValue()) {
          ColorAttachmentDecl = CTD;
          return true;
        } else if (BaseSpec->getTemplateArgs()[1]
                       .getAsIntegral()
                       .getZExtValue()) {
          InShaderAttributes.push_back(CTD);
          return true;
        } else {
          Attributes.push_back(CTD);
          return true;
        }
      }
    }
  }
  return true;
}

bool HshBuiltins::PipelineAttributes::VisitCXXRecordDecl(CXXRecordDecl *CRD) {
  if (CRD->getName() == "dual_source")
    DualSourceDecl = CRD;
  else if (CRD->getName() == "direct_render")
    DirectRenderDecl = CRD;
  else if (CRD->getName() == "high_priority")
    HighPriorityDecl = CRD;
  return true;
}

void HshBuiltins::PipelineAttributes::ValidateAttributeDecl(
    ASTContext &Context, ClassTemplateDecl *CTD) {
  DiagnosticsEngine &Diags = Context.getDiagnostics();
  for (const auto *Parm : *CTD->getTemplateParameters()) {
    if (const auto *NTTP = dyn_cast<NonTypeTemplateParmDecl>(Parm)) {
      if (auto *DefArg = NTTP->getDefaultArgument()) {
        if (auto *DRE = dyn_cast<DeclRefExpr>(DefArg)) {
          if (isa<NonTypeTemplateParmDecl>(DRE->getDecl()))
            continue;
        } else if (!DefArg->isValueDependent() &&
                   DefArg->isIntegerConstantExpr(Context, {})) {
          continue;
        }
      }
    }
    Diags.Report(CTD->getBeginLoc(),
                 Diags.getCustomDiagID(
                     DiagnosticsEngine::Error,
                     "all pipeline attributes in hsh.h must contain only "
                     "default-initialized non-type template parameters."))
        << CTD->getSourceRange();
  }
}

void HshBuiltins::PipelineAttributes::findDecls(ASTContext &Context,
                                                NamespaceDecl *PipelineNS) {
  Visit(PipelineNS);
  DiagnosticsEngine &Diags = Context.getDiagnostics();
  if (!BaseAttributeDecl) {
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of class "
                                  "template hsh::pipeline::base_attribute; "
                                  "is hsh.h included?"));
  }
  if (!PipelineDecl) {
    Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                       "unable to locate declaration of class "
                                       "template hsh::pipeline::pipeline; "
                                       "is hsh.h included?"));
  }
  if (!ColorAttachmentDecl) {
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of class "
                                  "template hsh::pipeline::color_attachment; "
                                  "is hsh.h included?"));
  } else {
    ValidateAttributeDecl(Context, ColorAttachmentDecl);
  }
  for (auto *CTD : Attributes)
    ValidateAttributeDecl(Context, CTD);
  for (auto *CTD : InShaderAttributes)
    ValidateAttributeDecl(Context, CTD);
}

SmallVector<ArrayRef<TemplateArgument>, 4>
HshBuiltins::PipelineAttributes::getColorAttachmentArgs(
    const ClassTemplateSpecializationDecl *PipelineSpec,
    bool &DualSourceAdded) const {
  assert(PipelineSpec->getSpecializedTemplateOrPartial()
             .get<ClassTemplateDecl *>() == PipelineDecl);
  SmallVector<ArrayRef<TemplateArgument>, 4> Ret;
  for (const auto &Arg : PipelineSpec->getTemplateArgs()[0].getPackAsArray()) {
    if (Arg.getKind() == TemplateArgument::Type) {
      if (auto *CTD = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
              Arg.getAsType()->getAsCXXRecordDecl())) {
        if (CTD->getSpecializedTemplateOrPartial().get<ClassTemplateDecl *>() ==
            ColorAttachmentDecl) {
          Ret.push_back(CTD->getTemplateArgs().asArray());
        }
      } else if (Arg.getAsType()->getAsCXXRecordDecl() == DualSourceDecl) {
        DualSourceAdded = true;
      }
    }
  }
  return Ret;
}

SmallVector<TemplateArgument, 8>
HshBuiltins::PipelineAttributes::getPipelineArgs(
    ASTContext &Context,
    const ClassTemplateSpecializationDecl *PipelineSpec) const {
  assert(PipelineSpec->getSpecializedTemplateOrPartial()
             .get<ClassTemplateDecl *>() == PipelineDecl);
  SmallVector<TemplateArgument, 8> Ret;
  for (auto *RefCTD : Attributes) {
    bool Handled = false;
    for (const auto &Arg :
         PipelineSpec->getTemplateArgs()[0].getPackAsArray()) {
      if (Arg.getKind() == TemplateArgument::Type) {
        if (auto *CTD = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
                Arg.getAsType()->getAsCXXRecordDecl())) {
          if (CTD->getSpecializedTemplateOrPartial()
                  .get<ClassTemplateDecl *>() == RefCTD) {
            for (const auto &ArgIn : CTD->getTemplateArgs().asArray())
              Ret.push_back(ArgIn);
            Handled = true;
            break;
          }
        }
      }
    }
    if (!Handled) {
      for (const auto *Parm : *RefCTD->getTemplateParameters()) {
        if (const auto *NTTP = dyn_cast<NonTypeTemplateParmDecl>(Parm)) {
          if (auto *DefArg = NTTP->getDefaultArgument()) {
            Expr::EvalResult Result;
            if (DefArg->EvaluateAsInt(Result, Context)) {
              Ret.emplace_back(Context, Result.Val.getInt(), DefArg->getType());
            }
          }
        }
      }
    }
  }
  return Ret;
}

SmallVector<std::pair<StringRef, TemplateArgument>, 8>
HshBuiltins::PipelineAttributes::getInShaderPipelineArgs(
    ASTContext &Context,
    const ClassTemplateSpecializationDecl *PipelineSpec) const {
  assert(PipelineSpec->getSpecializedTemplateOrPartial()
             .get<ClassTemplateDecl *>() == PipelineDecl);
  SmallVector<std::pair<StringRef, TemplateArgument>, 8> Ret;
  for (auto *RefCTD : InShaderAttributes) {
    bool Handled = false;
    for (const auto &Arg :
         PipelineSpec->getTemplateArgs()[0].getPackAsArray()) {
      if (Arg.getKind() == TemplateArgument::Type) {
        if (auto *CTD = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
                Arg.getAsType()->getAsCXXRecordDecl())) {
          if (CTD->getSpecializedTemplateOrPartial()
                  .get<ClassTemplateDecl *>() == RefCTD) {
            for (const auto &ArgIn : CTD->getTemplateArgs().asArray())
              Ret.emplace_back(RefCTD->getName(), ArgIn);
            Handled = true;
            break;
          }
        }
      }
    }
    if (!Handled) {
      for (const auto *Parm : *RefCTD->getTemplateParameters()) {
        if (const auto *NTTP = dyn_cast<NonTypeTemplateParmDecl>(Parm)) {
          if (auto *DefArg = NTTP->getDefaultArgument()) {
            Expr::EvalResult Result;
            if (DefArg->EvaluateAsInt(Result, Context)) {
              Ret.emplace_back(RefCTD->getName(),
                               TemplateArgument(Context, Result.Val.getInt(),
                                                DefArg->getType()));
            }
          }
        }
      }
    }
  }
  return Ret;
}

bool HshBuiltins::PipelineAttributes::isHighPriority(
    const ClassTemplateSpecializationDecl *PipelineSpec) const {
  for (const auto &Arg : PipelineSpec->getTemplateArgs()[0].getPackAsArray()) {
    if (Arg.getKind() == TemplateArgument::Type) {
      if (Arg.getAsType()->getAsCXXRecordDecl() == HighPriorityDecl) {
        return true;
      }
    }
  }
  return false;
}

bool HshBuiltins::PipelineAttributes::isDirectRender(
    const ClassTemplateSpecializationDecl *PipelineSpec) const {
  for (const auto &Arg : PipelineSpec->getTemplateArgs()[0].getPackAsArray()) {
    if (Arg.getKind() == TemplateArgument::Type) {
      if (Arg.getAsType()->getAsCXXRecordDecl() == DirectRenderDecl) {
        return true;
      }
    }
  }
  return false;
}

void HshBuiltins::addType(ASTContext &Context, DeclContext *DC,
                          HshBuiltinType TypeKind, StringRef Name) {
  if (auto *FoundType = LookupDecl<TagDecl>(Context, DC, Name)) {
    Types[TypeKind] = FoundType;
  } else {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                  "type %0; is hsh.h included?"))
        << Name;
  }
}

void HshBuiltins::addAlignedType(ASTContext &Context, DeclContext *DC,
                                 HshBuiltinType TypeKind, StringRef Name) {
  if (auto *FoundType = LookupDecl<TagDecl>(Context, DC, Name)) {
    AlignedTypes[TypeKind] = FoundType;
  } else {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                  "aligned type %0; is hsh.h included?"))
        << Name;
  }
}

void HshBuiltins::addEnumType(ASTContext &Context, DeclContext *DC,
                              HshBuiltinType TypeKind, StringRef Name) {
  if (auto *FoundEnum = LookupDecl<EnumDecl>(Context, DC, Name)) {
    Types[TypeKind] = FoundEnum;
  } else {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                  "enum %0; is hsh.h included?"))
        << Name;
  }
}

void HshBuiltins::addFunction(ASTContext &Context, DeclContext *DC,
                              HshBuiltinFunction FuncKind, StringRef Name,
                              StringRef P) {
  SmallVector<StringRef, 8> Params;
  if (P != "void") {
    P.split(Params, ',');
    for (auto &ParamStr : Params)
      ParamStr = ParamStr.trim();
  }
  auto LookupResults = DC->noload_lookup(&Context.Idents.get(Name));
  for (auto *Res : LookupResults) {
    FunctionDecl *Function = dyn_cast<FunctionDecl>(Res);
    if (!Function) {
      if (auto *Template = dyn_cast<FunctionTemplateDecl>(Res))
        Function = dyn_cast<FunctionDecl>(Template->getTemplatedDecl());
    }
    if (Function && Function->getNumParams() == Params.size()) {
      auto *It = Params.begin();
      bool Match = true;
      for (auto *Param : Function->parameters()) {
        if (Param->getType().getAsString() != *It++) {
          Match = false;
          break;
        }
      }
      if (Match) {
        Functions[FuncKind] = Function->getCanonicalDecl();
        return;
      }
    }
  }
  DiagnosticsEngine &Diags = Context.getDiagnostics();
  Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                     "unable to locate declaration of builtin "
                                     "function %1(%2); is hsh.h included?"))
      << Name << P;
}

void HshBuiltins::addCXXMethod(ASTContext &Context, DeclContext *DC,
                               StringRef R, StringRef Name, StringRef P,
                               HshBuiltinCXXMethod MethodKind) {
  if (auto *FoundRecord = LookupDecl<CXXRecordDecl>(Context, DC, R)) {
    SmallVector<StringRef, 8> Params;
    if (P != "void") {
      P.split(Params, ',');
      for (auto &ParamStr : Params)
        ParamStr = ParamStr.trim();
    }
    auto LookupResults = FoundRecord->noload_lookup(&Context.Idents.get(Name));
    for (auto *Res : LookupResults) {
      CXXMethodDecl *Method = dyn_cast<CXXMethodDecl>(Res);
      if (!Method) {
        if (auto *Template = dyn_cast<FunctionTemplateDecl>(Res))
          Method = dyn_cast<CXXMethodDecl>(Template->getTemplatedDecl());
      }
      if (Method && Method->getNumParams() == Params.size()) {
        auto *It = Params.begin();
        bool Match = true;
        for (auto *Param : Method->parameters()) {
          if (Param->getType().getAsString() != *It++) {
            Match = false;
            break;
          }
        }
        if (Match) {
          Methods[MethodKind] =
              dyn_cast<CXXMethodDecl>(Method->getCanonicalDecl());
          return;
        }
      }
    }
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                  "method %0::%1(%2); is hsh.h included?"))
        << R << Name << P;
  } else {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of builtin "
                                  "record %0; is hsh.h included?"))
        << R;
  }
}

NamespaceDecl *HshBuiltins::findNamespace(StringRef Name, DeclContext *DC,
                                          ASTContext &Context) const {
  auto *FoundNS = LookupDecl<NamespaceDecl>(Context, DC, Name);
  if (!FoundNS) {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(
        Diags.getCustomDiagID(DiagnosticsEngine::Error,
                              "unable to locate declaration of namespace %0; "
                              "is hsh.h included?"))
        << Name;
  }
  return FoundNS;
}

EnumDecl *HshBuiltins::findEnum(StringRef Name, DeclContext *DC,
                                ASTContext &Context) const {
  auto *FoundEnum = LookupDecl<EnumDecl>(Context, DC, Name);
  if (!FoundEnum) {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of enum %0; "
                                  "is hsh.h included?"))
        << Name;
  }
  return FoundEnum;
}

CXXRecordDecl *HshBuiltins::findCXXRecord(StringRef Name, DeclContext *DC,
                                          ASTContext &Context) const {
  auto *FoundRecord = LookupDecl<CXXRecordDecl>(Context, DC, Name);
  if (!FoundRecord) {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error, "unable to locate declaration of record %0; "
                                  "is hsh.h included?"))
        << Name;
  }
  return FoundRecord;
}

ClassTemplateDecl *HshBuiltins::findClassTemplate(StringRef Name,
                                                  DeclContext *DC,
                                                  ASTContext &Context) const {
  auto *FoundTemplate = LookupDecl<ClassTemplateDecl>(Context, DC, Name);
  if (!FoundTemplate) {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error,
        "unable to locate declaration of class template %0; "
        "is hsh.h included?"))
        << Name;
  }
  return FoundTemplate;
}

FunctionTemplateDecl *
HshBuiltins::findMethodTemplate(CXXRecordDecl *Class, StringRef Name,
                                ASTContext &Context) const {
  auto ClassName = Class->getName();
  Class = Class->getDefinition();
  DiagnosticsEngine &Diags = Context.getDiagnostics();
  if (!Class) {
    Diags.Report(Diags.getCustomDiagID(
        DiagnosticsEngine::Error,
        "definition of %0 is not available; is hsh.h included?"))
        << ClassName;
    return nullptr;
  }
  using FuncTemplIt =
      CXXRecordDecl::specific_decl_iterator<FunctionTemplateDecl>;
  FunctionTemplateDecl *Ret = nullptr;
  for (FuncTemplIt TI(Class->decls_begin()), TE(Class->decls_end()); TI != TE;
       ++TI) {
    if (TI->getName() == Name)
      Ret = *TI;
  }
  if (Ret)
    return Ret;
  Diags.Report(Diags.getCustomDiagID(
      DiagnosticsEngine::Error, "unable to locate declaration of "
                                "method template %0::%1; is hsh.h included?"))
      << Class->getName() << Name;
  return nullptr;
}

FunctionTemplateDecl *
HshBuiltins::findMethodTemplate(ClassTemplateDecl *Class, StringRef Name,
                                ASTContext &Context) const {
  return findMethodTemplate(Class->getTemplatedDecl(), Name, Context);
}

VarDecl *HshBuiltins::findVar(StringRef Name, DeclContext *DC,
                              ASTContext &Context) const {
  auto *FoundVar = LookupDecl<VarDecl>(Context, DC, Name);
  if (!FoundVar) {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    Diags.Report(
        Diags.getCustomDiagID(DiagnosticsEngine::Error,
                              "unable to locate declaration of variable %0; "
                              "is hsh.h included?"))
        << Name;
  }
  return FoundVar;
}

APSInt HshBuiltins::findICEVar(StringRef Name, DeclContext *DC,
                               ASTContext &Context) const {
  if (auto *VD = findVar(Name, DC, Context)) {
    DiagnosticsEngine &Diags = Context.getDiagnostics();
    if (auto *Val = VD->evaluateValue()) {
      if (Val->isInt())
        return Val->getInt();
      Diags.Report(Diags.getCustomDiagID(
          DiagnosticsEngine::Error, "variable %0 is not integer constexpr"))
          << Name;
      return APSInt{};
    }
    Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                       "variable %0 is not constexpr"))
        << Name;
  }
  return APSInt{};
}

void HshBuiltins::findBuiltinDecls(ASTContext &Context) {
  StdNamespace =
      findNamespace("std", Context.getTranslationUnitDecl(), Context);
  if (!StdNamespace)
    return;
  HshNamespace =
      findNamespace("hsh", Context.getTranslationUnitDecl(), Context);
  if (!HshNamespace)
    return;
  HshDetailNamespace = findNamespace("detail", HshNamespace, Context);
  if (!HshDetailNamespace)
    return;
  HshPipelineNamespace = findNamespace("pipeline", HshNamespace, Context);
  if (!HshPipelineNamespace)
    return;

  PipelineAttributes.findDecls(Context, HshPipelineNamespace);
  if (auto *PipelineRecordDecl = PipelineAttributes.getPipelineDecl()) {
    auto *Record = PipelineRecordDecl->getTemplatedDecl();
    for (auto *FD : Record->fields()) {
#define PIPELINE_FIELD(Name, Stage)                                            \
  if (FD->getName() == #Name)                                                  \
    PipelineFields[HPF_##Name] = {FD, Stage};
#include "ShaderInterface.def"
    }
    auto ReportMissingPipelineField = [&](StringRef Name) {
      DiagnosticsEngine &Diags = Context.getDiagnostics();
      Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                         "unable to locate pipeline field %0; "
                                         "is hsh::pipeline::pipeline invalid?"))
          << Name;
    };
    auto CheckIt = PipelineFields.begin();
#define PIPELINE_FIELD(Name, Stage)                                            \
  if (!(++CheckIt)->first)                                                     \
    ReportMissingPipelineField(#Name);
#include "ShaderInterface.def"
  }
  BindingRecordType = findCXXRecord("binding", HshNamespace, Context);
  RebindTemplateFunc =
      findMethodTemplate(BindingRecordType, "_rebind", Context);

  UniformBufferType =
      findClassTemplate("uniform_buffer", HshNamespace, Context);
  VertexBufferType = findClassTemplate("vertex_buffer", HshNamespace, Context);

  EnumTarget = findEnum("Target", HshNamespace, Context);
  EnumStage = findEnum("Stage", HshNamespace, Context);
  EnumInputRate = findEnum("InputRate", HshDetailNamespace, Context);
  EnumFormat = findEnum("Format", HshNamespace, Context);
  ShaderConstDataTemplateType =
      findClassTemplate("ShaderConstData", HshDetailNamespace, Context);
  ShaderDataTemplateType =
      findClassTemplate("ShaderData", HshDetailNamespace, Context);
  SamplerRecordType = findCXXRecord("sampler", HshNamespace, Context);
  SamplerBindingType =
      findCXXRecord("SamplerBinding", HshDetailNamespace, Context);

  MaxUniforms = findICEVar("MaxUniforms", HshDetailNamespace, Context);
  MaxImages = findICEVar("MaxImages", HshDetailNamespace, Context);
  MaxSamplers = findICEVar("MaxSamplers", HshDetailNamespace, Context);
  MaxVertexBuffers =
      findICEVar("MaxVertexBuffers", HshDetailNamespace, Context);

#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal)                           \
  addType(Context, HshNamespace, HBT_##Name, #Name);
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal, HasAligned)               \
  addType(Context, HshNamespace, HBT_##Name, #Name);                           \
  if (HasAligned)                                                              \
    addAlignedType(Context, HshNamespace, HBT_##Name, "aligned_" #Name);
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  addType(Context, HshNamespace, HBT_##Name, #Name);
#define BUILTIN_ENUM_TYPE(Name)                                                \
  addEnumType(Context, HshNamespace, HBT_##Name, #Name);
#include "BuiltinTypes.def"
#define BUILTIN_FUNCTION(Name, Spelling, GLSL, HLSL, Metal, InterpDist, ...)   \
  addFunction(Context, HshNamespace, HBF_##Name, #Spelling, #__VA_ARGS__);
#include "BuiltinFunctions.def"
#define BUILTIN_CXX_METHOD(Name, Spelling, IsSwizzle, Record, ...)             \
  addCXXMethod(Context, HshNamespace, #Record, #Spelling, #__VA_ARGS__,        \
               HBM_##Name);
#include "BuiltinCXXMethods.def"

  StdArrayType = findClassTemplate("array", StdNamespace, Context);
  AlignedArrayType = findClassTemplate("aligned_array", HshNamespace, Context);
}

HshBuiltinType HshBuiltins::identifyBuiltinType(QualType QT,
                                                bool &IsAligned) const {
  return identifyBuiltinType(QT.getNonReferenceType().getTypePtrOrNull(),
                             IsAligned);
}

HshBuiltinType HshBuiltins::identifyBuiltinType(const clang::Type *UT,
                                                bool &IsAligned) const {
  IsAligned = false;
  if (!UT)
    return HBT_None;
  TagDecl *T = UT->getAsTagDecl();
  if (!T)
    return HBT_None;
  T = T->getCanonicalDecl();
  if (!T)
    return HBT_None;
  HshBuiltinType Ret = HBT_None;
  for (const auto *Tp : Types) {
    if (T == Tp)
      return Ret;
    Ret = HshBuiltinType(int(Ret) + 1);
  }
  Ret = HBT_None;
  for (const auto *Tp : AlignedTypes) {
    if (T == Tp) {
      IsAligned = true;
      return Ret;
    }
    Ret = HshBuiltinType(int(Ret) + 1);
  }
  return HBT_None;
}

HshBuiltinFunction
HshBuiltins::identifyBuiltinFunction(const FunctionDecl *F) const {
  F = F->getCanonicalDecl();
  if (!F)
    return HBF_None;
  HshBuiltinFunction Ret = HBF_None;
  for (const auto *Func : Functions) {
    if (F == Func)
      return Ret;
    Ret = HshBuiltinFunction(int(Ret) + 1);
  }
  return HBF_None;
}

HshBuiltinCXXMethod
HshBuiltins::identifyBuiltinMethod(const CXXMethodDecl *M) const {
  M = dyn_cast_or_null<CXXMethodDecl>(M->getCanonicalDecl());
  if (!M)
    return HBM_None;
  if (FunctionDecl *FD = M->getInstantiatedFromMemberFunction())
    M = dyn_cast<CXXMethodDecl>(FD->getCanonicalDecl());
  if (auto *TD = M->getPrimaryTemplate())
    if (auto *TDM = dyn_cast<CXXMethodDecl>(TD->getTemplatedDecl()))
      M = TDM;
  HshBuiltinCXXMethod Ret = HBM_None;
  for (const auto *Method : Methods) {
    if (M == Method)
      return Ret;
    Ret = HshBuiltinCXXMethod(int(Ret) + 1);
  }
  return HBM_None;
}

HshBuiltinPipelineField
HshBuiltins::identifyBuiltinPipelineField(const FieldDecl *FD) const {
  if (const auto *Record =
          dyn_cast<ClassTemplateSpecializationDecl>(FD->getDeclContext())) {
    if (const auto *CTD = Record->getSpecializedTemplateOrPartial()
                              .get<ClassTemplateDecl *>()) {
      auto FieldIt = CTD->getTemplatedDecl()->field_begin();
      std::advance(FieldIt, FD->getFieldIndex());
      FD = *FieldIt;
    }
  }

  HshBuiltinPipelineField Ret = HPF_None;
  for (const auto &Field : PipelineFields) {
    if (FD == Field.first)
      return Ret;
    Ret = HshBuiltinPipelineField(int(Ret) + 1);
  }
  return HPF_None;
}

const CXXRecordDecl *
HshBuiltins::FirstTemplateParamType(ClassTemplateSpecializationDecl *Derived,
                                    ClassTemplateDecl *Decl) {
  if (Derived->getSpecializedTemplateOrPartial()
          .get<ClassTemplateDecl *>()
          ->getCanonicalDecl() == Decl) {
    const auto &Arg = Derived->getTemplateArgs()[0];
    if (Arg.getKind() == TemplateArgument::Type)
      return Arg.getAsType()->getAsCXXRecordDecl();
  }
  return nullptr;
}

const CXXRecordDecl *
HshBuiltins::getUniformRecord(const ParmVarDecl *PVD) const {
  auto *Derived = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
      PVD->getType()->getAsCXXRecordDecl());
  if (!Derived)
    return nullptr;
  if (const auto *Ret = FirstTemplateParamType(Derived, UniformBufferType))
    return Ret;
  return nullptr;
}

const CXXRecordDecl *
HshBuiltins::getVertexAttributeRecord(const ParmVarDecl *PVD) const {
  auto *Derived = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
      PVD->getType()->getAsCXXRecordDecl());
  if (!Derived)
    return nullptr;
  if (const auto *Ret = FirstTemplateParamType(Derived, VertexBufferType))
    return Ret;
  return nullptr;
}

bool HshBuiltins::checkHshTypeCompatibility(const ASTContext &Context,
                                            const ValueDecl *VD, QualType Tp,
                                            bool AllowTextures) const {
  if (auto *Spec = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
          Tp->getAsCXXRecordDecl())) {
    auto *CTD = Spec->getSpecializedTemplateOrPartial()
                    .get<ClassTemplateDecl *>()
                    ->getCanonicalDecl();
    if (CTD == StdArrayType || CTD == AlignedArrayType) {
      const auto &Arg = Spec->getTemplateArgs()[0];
      return checkHshTypeCompatibility(Context, VD, Arg.getAsType(),
                                       AllowTextures);
    }
  }
  HshBuiltinType HBT = identifyBuiltinType(Tp);
  if (HBT != HBT_None && (AllowTextures || !HshBuiltins::isTextureType(HBT))) {
    return true;
  } else if (Tp->isIntegralOrEnumerationType()) {
    if (Context.getIntWidth(Tp) != 32) {
      Reporter(Context).BadIntegerType(VD);
      return false;
    }
    return true;
  } else if (Tp->isSpecificBuiltinType(BuiltinType::Float) ||
             Tp->isSpecificBuiltinType(BuiltinType::Double)) {
    return true;
  }
  Reporter(Context).BadRecordType(VD);
  return false;
}

bool HshBuiltins::checkHshTypeCompatibility(const ASTContext &Context,
                                            const ValueDecl *VD,
                                            bool AllowTextures) const {
  QualType Tp = VD->getType();
  if (const auto *VarD = dyn_cast<VarDecl>(VD))
    Tp = ResolveParmType(VarD);
  else if (const auto *FuncD = dyn_cast<FunctionDecl>(VD))
    Tp = FuncD->getReturnType();
  return checkHshTypeCompatibility(Context, VD, Tp, AllowTextures);
}

bool HshBuiltins::checkHshFieldTypeCompatibility(const ASTContext &Context,
                                                 const ValueDecl *VD) const {
  return checkHshTypeCompatibility(Context, VD, false);
}

bool HshBuiltins::checkHshParamTypeCompatibility(const ASTContext &Context,
                                                 const ValueDecl *VD) const {
  return checkHshTypeCompatibility(Context, VD, true);
}

bool HshBuiltins::checkHshRecordCompatibility(
    const ASTContext &Context, const CXXRecordDecl *Record) const {
  bool Ret = true;
  for (const auto *FD : Record->fields())
    if (!checkHshFieldTypeCompatibility(Context, FD))
      Ret = false;
  return Ret;
}

bool HshBuiltins::checkHshFunctionCompatibility(const ASTContext &Context,
                                                const FunctionDecl *FD) const {
  bool Ret = true;
  if (FD->getReturnType() != Context.VoidTy)
    Ret = checkHshParamTypeCompatibility(Context, FD);
  for (const auto *Param : FD->parameters())
    if (!checkHshParamTypeCompatibility(Context, Param))
      Ret = false;
  return Ret;
}

HshStage HshBuiltins::determineVarStage(const VarDecl *VD) const {
  if (const auto *PVD = dyn_cast<ParmVarDecl>(VD)) {
    if (getVertexAttributeRecord(PVD))
      return HshVertexStage;
    else if (auto *SA = PVD->getAttr<HshStageAttr>())
      return HshStage(SA->getStageIndex());
    else if (isTextureType(identifyBuiltinType(PVD->getType())))
      return HshFragmentStage;
  } else if (auto *SA = VD->getAttr<HshStageAttr>()) {
    return HshStage(SA->getStageIndex());
  }
  return HshNoStage;
}

HshStage HshBuiltins::determinePipelineFieldStage(const FieldDecl *FD) const {
  auto HPF = identifyBuiltinPipelineField(FD);
  if (HPF == HPF_None)
    return HshNoStage;
  return stageOfBuiltinPipelineField(HPF);
}

const TagDecl *HshBuiltins::getAlignedAvailable(FieldDecl *FD) const {
  bool IsAligned;
  auto HBT = identifyBuiltinType(FD->getType(), IsAligned);
  return (AlignedTypes[HBT] != nullptr && !IsAligned) ? AlignedTypes[HBT]
                                                      : nullptr;
}

TypeSourceInfo *HshBuiltins::getFullyQualifiedTemplateSpecializationTypeInfo(
    ASTContext &Context, TemplateDecl *TDecl,
    const TemplateArgumentListInfo &Args) {
  QualType Underlying =
      Context.getTemplateSpecializationType(TemplateName(TDecl), Args);
  Underlying = TypeName::getFullyQualifiedType(Underlying, Context);
  return Context.getTrivialTypeSourceInfo(Underlying);
}

QualType HshBuiltins::getBindingRefType(ASTContext &Context) const {
  return Context.getLValueReferenceType(
      QualType{BindingRecordType->getTypeForDecl(), 0});
}

CXXMemberCallExpr *HshBuiltins::makeSpecializedRebindCall(
    ASTContext &Context, QualType SpecType, ParmVarDecl *BindingParm,
    ArrayRef<QualType> CallArgs, ArrayRef<Expr *> CallExprs) const {
  auto *Args =
      TemplateArgumentList::CreateCopy(Context, TemplateArgument(SpecType));
  void *InsertPos;
  CXXMethodDecl *Method = dyn_cast_or_null<CXXMethodDecl>(
      RebindTemplateFunc->findSpecialization(Args->asArray(), InsertPos));
  if (!Method) {
    Method = CXXMethodDecl::Create(
        Context, BindingRecordType, {}, {RebindTemplateFunc->getDeclName(), {}},
        Context.getFunctionType(
            Context.VoidTy, CallArgs,
            FunctionProtoType::ExtProtoInfo().withExceptionSpec(
                EST_BasicNoexcept)),
        nullptr, SC_None, false, CSK_unspecified, {});
    Method->setAccess(AS_public);
    Method->setFunctionTemplateSpecialization(RebindTemplateFunc, Args,
                                              InsertPos);
  }
  auto AValidLocation = Context.getSourceManager().getLocForStartOfFile(
      Context.getSourceManager().getMainFileID());
  TemplateArgumentListInfo TemplateArgs(AValidLocation, AValidLocation);
  TemplateArgs.addArgument(TemplateArgumentLoc(
      TemplateArgument(SpecType), Context.getTrivialTypeSourceInfo(SpecType)));
  return CXXMemberCallExpr::Create(
      Context,
      MemberExpr::Create(
          Context,
          DeclRefExpr::Create(
              Context, {}, {}, BindingParm, false, SourceLocation{},
              BindingParm->getType().getNonReferenceType(), VK_XValue),
          false, SourceLocation{}, {}, SourceLocation{}, Method,
          DeclAccessPair::make(Method, AS_public), {}, &TemplateArgs,
          Method->getType(), VK_XValue, OK_Ordinary, NOUR_None),
      CallExprs, Context.VoidTy, VK_XValue, {}, {});
}

bool HshBuiltins::isDerivedFromPipelineDecl(CXXRecordDecl *Decl) const {
  CXXBasePaths Paths(/*FindAmbiguities=*/false, /*RecordPaths=*/false,
                     /*DetectVirtual=*/false);
  return Decl->lookupInBases(
      [this](const CXXBaseSpecifier *Specifier, CXXBasePath &Path) {
        if (const auto *TST =
                Specifier->getType()->getAs<TemplateSpecializationType>()) {
          if (auto *TD = dyn_cast_or_null<ClassTemplateDecl>(
                  TST->getTemplateName().getAsTemplateDecl()))
            return TD == getPipelineRecordDecl();
        }
        return false;
      },
      Paths, /*LookupInDependent=*/true);
}

ClassTemplateSpecializationDecl *
HshBuiltins::getDerivedPipelineSpecialization(CXXRecordDecl *Decl) const {
  CXXBasePaths Paths(/*FindAmbiguities=*/false, /*RecordPaths=*/false,
                     /*DetectVirtual=*/false);
  ClassTemplateSpecializationDecl *Ret = nullptr;
  Decl->lookupInBases(
      [this, &Ret](const CXXBaseSpecifier *Specifier, CXXBasePath &Path) {
        if (const auto *TST =
                Specifier->getType()->getAs<TemplateSpecializationType>()) {
          if (auto *TD = dyn_cast_or_null<ClassTemplateDecl>(
                  TST->getTemplateName().getAsTemplateDecl())) {
            if (TD == getPipelineRecordDecl()) {
              Ret = dyn_cast_or_null<ClassTemplateSpecializationDecl>(
                  Specifier->getType()->getAsCXXRecordDecl());
              return true;
            }
          }
        }
        return false;
      },
      Paths, /*LookupInDependent=*/false);
  return Ret;
}

bool HshBuiltins::isStdArrayType(const CXXRecordDecl *RD) const {
  if (const auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(RD))
    return CTSD->getSpecializedTemplateOrPartial()
               .get<ClassTemplateDecl *>()
               ->getCanonicalDecl() == StdArrayType;
  return false;
}

bool HshBuiltins::isAlignedArrayType(const CXXRecordDecl *RD) const {
  if (const auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(RD))
    return CTSD->getSpecializedTemplateOrPartial()
               .get<ClassTemplateDecl *>()
               ->getCanonicalDecl() == AlignedArrayType;
  return false;
}

CXXFunctionalCastExpr *
HshBuiltins::makeSamplerBinding(ASTContext &Context, ParmVarDecl *Tex,
                                unsigned SamplerIdx,
                                unsigned TextureIdx) const {
  std::array<Expr *, 3> Args{
      DeclRefExpr::Create(Context, {}, {}, Tex, false, SourceLocation{},
                          Tex->getType(), VK_XValue),
      IntegerLiteral::Create(Context, APInt(32, SamplerIdx), Context.IntTy, {}),
      IntegerLiteral::Create(Context, APInt(32, TextureIdx), Context.IntTy,
                             {})};
  auto *Init = new (Context) InitListExpr(Context, {}, Args, {});
  return CXXFunctionalCastExpr::Create(
      Context, QualType{SamplerBindingType->getTypeForDecl(), 0}, VK_XValue,
      nullptr, CastKind::CK_NoOp, Init, nullptr, {}, {});
}

CXXRecordDecl *HshBuiltins::makeBindingDerivative(ASTContext &Context,
                                                  CXXRecordDecl *Source,
                                                  StringRef Name) const {
  auto *Record = CXXRecordDecl::Create(Context, TTK_Class,
                                       Context.getTranslationUnitDecl(), {}, {},
                                       &Context.Idents.get(Name));
  Record->HshSourceRecord = Source;
  Record->startDefinition();

  return Record;
}

ClassTemplateDecl *
HshBuiltins::makeBindingDerivative(ASTContext &Context, Sema &Actions,
                                   ClassTemplateDecl *SpecializationSource,
                                   StringRef Name) const {
  auto *Record = CXXRecordDecl::Create(Context, TTK_Class,
                                       Context.getTranslationUnitDecl(), {}, {},
                                       &Context.Idents.get(Name));
  Record->startDefinition();
  Record->completeDefinition();

  auto *CTD = ClassTemplateDecl::Create(
      Context, Context.getTranslationUnitDecl(), {}, Record->getIdentifier(),
      SpecializationSource->getTemplateParameters(), Record);

  for (auto *Specialization : SpecializationSource->specializations()) {
    /*
     * Hsh violates C++ within binding macros by permitting non-constexpr
     * template parameters. These non-constexpr specializations shall not be
     * translated.
     */
    if (!CheckConstexprTemplateSpecialization(Context, Specialization))
      continue;

    /*
     * Hsh supports forward-declared specializations for profiling macros.
     * Attempt to instantiate if necessary.
     */
    if (!Specialization->hasDefinition()) {
      if (Actions.InstantiateClassTemplateSpecialization(
              Specialization->getBeginLoc(), Specialization,
              TSK_ExplicitInstantiationDefinition))
        continue;
      Actions.InstantiateClassTemplateSpecializationMembers(
          Specialization->getBeginLoc(), Specialization,
          TSK_ExplicitInstantiationDefinition);
    }
    /*
     * Every specialization must inherit a hsh::pipeline specialization to be
     * eligible for translation.
     */
    if (!isDerivedFromPipelineDecl(Specialization))
      continue;

    auto Args = Specialization->getTemplateArgs().asArray();
    void *InsertPos;
    CTD->findSpecialization(Args, InsertPos);
    auto *Spec = ClassTemplateSpecializationDecl::Create(
        Context, Record->getTagKind(), Record->getDeclContext(), {}, {}, CTD,
        Args, nullptr);
    Spec->HshSourceRecord = Specialization;
    Spec->startDefinition();
    CTD->AddSpecialization(Spec, InsertPos);
  }

  return CTD;
}

void HshBuiltins::printEnumeratorString(raw_ostream &Out,
                                        const PrintingPolicy &Policy,
                                        const EnumDecl *ED, const APSInt &Val) {
  for (const EnumConstantDecl *ECD : ED->enumerators()) {
    if (llvm::APSInt::isSameValue(ECD->getInitVal(), Val)) {
      ECD->printQualifiedName(Out, Policy);
      return;
    }
  }
}

const EnumConstantDecl *HshBuiltins::lookupEnumConstantDecl(const EnumDecl *ED,
                                                            const APSInt &Val) {
  for (const EnumConstantDecl *ECD : ED->enumerators())
    if (llvm::APSInt::isSameValue(ECD->getInitVal(), Val))
      return ECD;
  return nullptr;
}

VarDecl *HshBuiltins::getConstDataVar(ASTContext &Context, DeclContext *DC,
                                      HshTarget Target, uint32_t NumStages,
                                      uint32_t NumBindings,
                                      uint32_t NumAttributes,
                                      uint32_t NumSamplers,
                                      uint32_t NumColorAttachments) const {
  const auto *ECD = lookupEnumConstantDecl(EnumTarget, APSInt::get(Target));
  assert(ECD);

  TemplateArgumentListInfo TemplateArgs;
  TemplateArgs.addArgument(TemplateArgumentLoc(
      TemplateArgument{Context, APSInt::get(Target),
                       QualType{EnumTarget->getTypeForDecl(), 0}},
      (Expr *)nullptr));
  TemplateArgs.addArgument(TemplateArgumentLoc(
      TemplateArgument{Context, APSInt::get(NumStages), Context.UnsignedIntTy},
      (Expr *)nullptr));
  TemplateArgs.addArgument(
      TemplateArgumentLoc(TemplateArgument{Context, APSInt::get(NumBindings),
                                           Context.UnsignedIntTy},
                          (Expr *)nullptr));
  TemplateArgs.addArgument(
      TemplateArgumentLoc(TemplateArgument{Context, APSInt::get(NumAttributes),
                                           Context.UnsignedIntTy},
                          (Expr *)nullptr));
  TemplateArgs.addArgument(
      TemplateArgumentLoc(TemplateArgument{Context, APSInt::get(NumSamplers),
                                           Context.UnsignedIntTy},
                          (Expr *)nullptr));
  TemplateArgs.addArgument(TemplateArgumentLoc(
      TemplateArgument{Context, APSInt::get(NumColorAttachments),
                       Context.UnsignedIntTy},
      (Expr *)nullptr));
  TypeSourceInfo *TSI = getFullyQualifiedTemplateSpecializationTypeInfo(
      Context, ShaderConstDataTemplateType, TemplateArgs);

  auto *VD = VarDecl::Create(
      Context, DC, {}, {},
      &Context.Idents.get(Twine("cdata_", ECD->getName()).str()),
      TSI->getType(), nullptr, SC_Static);
  VD->setConstexpr(true);
  VD->setInitStyle(VarDecl::ListInit);
  VD->setInit(new (Context) InitListExpr(Stmt::EmptyShell{}));
  VD->InitHshTarget = Target;
  return VD;
}

VarDecl *HshBuiltins::getDataVar(ASTContext &Context, DeclContext *DC,
                                 HshTarget Target, uint32_t NumStages,
                                 uint32_t NumSamplers) const {
  const auto *ECD = lookupEnumConstantDecl(EnumTarget, APSInt::get(Target));
  assert(ECD);

  TemplateArgumentListInfo TemplateArgs;
  TemplateArgs.addArgument(TemplateArgumentLoc(
      TemplateArgument{Context, APSInt::get(Target),
                       QualType{EnumTarget->getTypeForDecl(), 0}},
      (Expr *)nullptr));
  TemplateArgs.addArgument(TemplateArgumentLoc(
      TemplateArgument{Context, APSInt::get(NumStages), Context.UnsignedIntTy},
      (Expr *)nullptr));
  TemplateArgs.addArgument(
      TemplateArgumentLoc(TemplateArgument{Context, APSInt::get(NumSamplers),
                                           Context.UnsignedIntTy},
                          (Expr *)nullptr));
  TypeSourceInfo *TSI = getFullyQualifiedTemplateSpecializationTypeInfo(
      Context, ShaderDataTemplateType, TemplateArgs);

  return VarDecl::Create(
      Context, DC, {}, {},
      &Context.Idents.get(Twine("data_", ECD->getName()).str()), TSI->getType(),
      nullptr, SC_Static);
}

void HshBuiltins::printBuiltinEnumString(raw_ostream &Out,
                                         const PrintingPolicy &Policy,
                                         HshBuiltinType HBT,
                                         const APSInt &Val) const {
  printEnumeratorString(Out, Policy, cast<EnumDecl>(getTypeDecl(HBT)), Val);
}

void HshBuiltins::printBuiltinEnumString(raw_ostream &Out,
                                         const PrintingPolicy &Policy,
                                         HshBuiltinType HBT,
                                         int64_t Val) const {
  printBuiltinEnumString(Out, Policy, HBT, APSInt::get(Val));
}

void HshBuiltins::printTargetEnumString(raw_ostream &Out,
                                        const PrintingPolicy &Policy,
                                        HshTarget Target) const {
  printEnumeratorString(Out, Policy, EnumTarget, APSInt::get(Target));
}

void HshBuiltins::printTargetEnumName(raw_ostream &Out,
                                      HshTarget Target) const {
  if (const auto *ECD = lookupEnumConstantDecl(EnumTarget, APSInt::get(Target)))
    ECD->printName(Out);
}

void HshBuiltins::printStageEnumString(raw_ostream &Out,
                                       const PrintingPolicy &Policy,
                                       HshStage Stage) const {
  printEnumeratorString(Out, Policy, EnumStage, APSInt::get(Stage));
}

void HshBuiltins::printInputRateEnumString(raw_ostream &Out,
                                           const PrintingPolicy &Policy,
                                           HshAttributeKind InputRate) const {
  printEnumeratorString(Out, Policy, EnumInputRate, APSInt::get(InputRate));
}

void HshBuiltins::printFormatEnumString(raw_ostream &Out,
                                        const PrintingPolicy &Policy,
                                        HshFormat Format) const {
  printEnumeratorString(Out, Policy, EnumFormat, APSInt::get(Format));
}

void HshBuiltins::printColorComponentFlagExpr(raw_ostream &Out,
                                              const PrintingPolicy &Policy,
                                              ColorComponentFlags Flags) {
  bool NeedsPipe = false;
  for (int i = 0; i < 4; ++i) {
    if (Flags & (1 << i)) {
      if (NeedsPipe)
        Out << " | ";
      else
        NeedsPipe = true;
      printBuiltinEnumString(Out, Policy, HBT_ColorComponentFlags, 1 << i);
    }
  }
}

HshFormat HshBuiltins::formatOfType(QualType Tp) const {
  if (Tp->isSpecificBuiltinType(BuiltinType::Float)) {
    return R32_SFLOAT;
  } else if (Tp->isSpecificBuiltinType(BuiltinType::Int)) {
    return R32_SINT;
  } else if (Tp->isSpecificBuiltinType(BuiltinType::UInt)) {
    return R32_UINT;
  }
  auto HBT = identifyBuiltinType(Tp);
  switch (HBT) {
  case HBT_float2:
    return RG32_SFLOAT;
  case HBT_float3:
  case HBT_float3x3:
    return RGB32_SFLOAT;
  case HBT_float4:
  case HBT_float4x4:
    return RGBA32_SFLOAT;
  case HBT_int2:
    return RG32_SINT;
  case HBT_int3:
    return RGB32_SINT;
  case HBT_int4:
    return RGBA32_SINT;
  case HBT_uint2:
    return RG32_UINT;
  case HBT_uint3:
    return RGB32_UINT;
  case HBT_uint4:
    return RGBA32_UINT;
  default:
    break;
  }
  llvm_unreachable("Invalid type passed to formatOfType");
}

} // namespace clang::hshgen
