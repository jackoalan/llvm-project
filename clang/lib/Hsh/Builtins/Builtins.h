//===--- Builtins.h - hsh builtins declaration manager --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "clang/AST/DeclVisitor.h"
#include "clang/Hsh/HshGenerator.h"

namespace clang::hshgen {

enum HshBuiltinType {
  HBT_None,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) HBT_##Name,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal, HasAligned) HBT_##Name,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  HBT_##Name,
#define BUILTIN_ENUM_TYPE(Name) HBT_##Name,
#include "BuiltinTypes.def"
  HBT_Max
};

enum HshBuiltinFunction {
  HBF_None,
#define BUILTIN_FUNCTION(Name, Spelling, GLSL, HLSL, Metal, InterpDist, ...)   \
  HBF_##Name,
#include "BuiltinFunctions.def"
  HBF_Max
};

enum HshBuiltinCXXMethod {
  HBM_None,
#define BUILTIN_CXX_METHOD(Name, Spelling, IsSwizzle, Record, ...) HBM_##Name,
#include "BuiltinCXXMethods.def"
  HBM_Max
};

enum HshBuiltinPipelineField {
  HPF_None,
#define PIPELINE_FIELD(Name, Stage) HPF_##Name,
#include "ShaderInterface.def"
  HPF_Max
};

class HshBuiltins {
public:
  struct Spellings {
    StringRef GLSL, HLSL, Metal;
  };

  class PipelineAttributes : public DeclVisitor<PipelineAttributes, bool> {
    using base = DeclVisitor<PipelineAttributes, bool>;
    ClassTemplateDecl *BaseAttributeDecl = nullptr;
    ClassTemplateDecl *ColorAttachmentDecl = nullptr;
    CXXRecordDecl *DualSourceDecl = nullptr;
    CXXRecordDecl *DirectRenderDecl = nullptr;
    CXXRecordDecl *HighPriorityDecl = nullptr;
    SmallVector<ClassTemplateDecl *, 8> Attributes; // Non-color-attachments
    SmallVector<ClassTemplateDecl *, 1>
        InShaderAttributes; // Just for early depth stencil
    ClassTemplateDecl *PipelineDecl = nullptr;

  public:
    bool VisitDecl(Decl *D);

    bool VisitClassTemplateDecl(ClassTemplateDecl *CTD);

    bool VisitCXXRecordDecl(CXXRecordDecl *CRD);

    static void ValidateAttributeDecl(ASTContext &Context,
                                      ClassTemplateDecl *CTD);

    void findDecls(ASTContext &Context, NamespaceDecl *PipelineNS);

    ClassTemplateDecl *getPipelineDecl() const { return PipelineDecl; }

    SmallVector<ArrayRef<TemplateArgument>, 4>
    getColorAttachmentArgs(const ClassTemplateSpecializationDecl *PipelineSpec,
                           bool &DualSourceAdded) const;

    SmallVector<TemplateArgument, 8>
    getPipelineArgs(ASTContext &Context,
                    const ClassTemplateSpecializationDecl *PipelineSpec) const;

    SmallVector<std::pair<StringRef, TemplateArgument>, 8>
    getInShaderPipelineArgs(
        ASTContext &Context,
        const ClassTemplateSpecializationDecl *PipelineSpec) const;

    bool
    isHighPriority(const ClassTemplateSpecializationDecl *PipelineSpec) const;

    bool
    isDirectRender(const ClassTemplateSpecializationDecl *PipelineSpec) const;
  };

private:
  NamespaceDecl *StdNamespace = nullptr;
  NamespaceDecl *HshNamespace = nullptr;
  NamespaceDecl *HshDetailNamespace = nullptr;
  NamespaceDecl *HshPipelineNamespace = nullptr;
  PipelineAttributes PipelineAttributes;
  CXXRecordDecl *BindingRecordType = nullptr;
  FunctionTemplateDecl *RebindTemplateFunc = nullptr;
  ClassTemplateDecl *UniformBufferType = nullptr;
  ClassTemplateDecl *VertexBufferType = nullptr;
  EnumDecl *EnumTarget = nullptr;
  EnumDecl *EnumStage = nullptr;
  EnumDecl *EnumInputRate = nullptr;
  EnumDecl *EnumFormat = nullptr;
  ClassTemplateDecl *ShaderConstDataTemplateType = nullptr;
  ClassTemplateDecl *ShaderDataTemplateType = nullptr;
  CXXRecordDecl *SamplerRecordType = nullptr;
  CXXRecordDecl *SamplerBindingType = nullptr;
  llvm::APSInt MaxUniforms;
  llvm::APSInt MaxImages;
  llvm::APSInt MaxSamplers;
  llvm::APSInt MaxVertexBuffers;
  std::array<const TagDecl *, HBT_Max> Types{};
  std::array<const TagDecl *, HBT_Max> AlignedTypes{};
  std::array<const FunctionDecl *, HBF_Max> Functions{};
  std::array<const CXXMethodDecl *, HBM_Max> Methods{};
  std::array<std::pair<const FieldDecl *, HshStage>, HPF_Max> PipelineFields{};
  ClassTemplateDecl *StdArrayType = nullptr;
  ClassTemplateDecl *AlignedArrayType = nullptr;
  ClassTemplateDecl *HshArrayType = nullptr;

  static constexpr Spellings BuiltinTypeSpellings[] = {
      {{}, {}, {}},
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) {#GLSL, #HLSL, #Metal},
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal, HasAligned)               \
  {#GLSL, #HLSL, #Metal},
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  {#GLSLf, #HLSLf, #Metalf},
#define BUILTIN_ENUM_TYPE(Name) {{}, {}, {}},
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinTypeVector[] = {
      false,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) true,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal, HasAligned) false,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  false,
#define BUILTIN_ENUM_TYPE(Name) false,
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinTypeMatrix[] = {
      false,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) false,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal, HasAligned) true,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  false,
#define BUILTIN_ENUM_TYPE(Name) false,
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinTypeTexture[] = {
      false,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) false,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal, HasAligned) false,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  true,
#define BUILTIN_ENUM_TYPE(Name) false,
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinTypeEnum[] = {
      false,
#define BUILTIN_VECTOR_TYPE(Name, GLSL, HLSL, Metal) false,
#define BUILTIN_MATRIX_TYPE(Name, GLSL, HLSL, Metal, HasAligned) false,
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  false,
#define BUILTIN_ENUM_TYPE(Name) true,
#include "BuiltinTypes.def"
  };

  static constexpr bool BuiltinMethodSwizzle[] = {
      false,
#define BUILTIN_CXX_METHOD(Name, Spelling, IsSwizzle, Record, ...) IsSwizzle,
#include "BuiltinCXXMethods.def"
  };

  static constexpr Spellings BuiltinFunctionSpellings[] = {
      {{}, {}, {}},
#define BUILTIN_FUNCTION(Name, Spelling, GLSL, HLSL, Metal, InterpDist, ...)   \
  {#GLSL, #HLSL, #Metal},
#include "BuiltinFunctions.def"
  };

  static constexpr bool BuiltinFunctionInterpDists[] = {
      false,
#define BUILTIN_FUNCTION(Name, Spelling, GLSL, HLSL, Metal, InterpDist, ...)   \
  InterpDist,
#include "BuiltinFunctions.def"
  };

  template <typename T>
  static T *LookupDecl(ASTContext &Context, DeclContext *DC, StringRef Name) {
    auto LookupResult = DC->noload_lookup(&Context.Idents.get(Name));
    if (!LookupResult.empty()) {
      if (T *Found = dyn_cast<T>(LookupResult[0]))
        return Found->getCanonicalDecl();
    }
    return nullptr;
  }

  void addType(ASTContext &Context, DeclContext *DC, HshBuiltinType TypeKind,
               StringRef Name);

  void addAlignedType(ASTContext &Context, DeclContext *DC,
                      HshBuiltinType TypeKind, StringRef Name);

  void addEnumType(ASTContext &Context, DeclContext *DC,
                   HshBuiltinType TypeKind, StringRef Name);

  void addFunction(ASTContext &Context, DeclContext *DC,
                   HshBuiltinFunction FuncKind, StringRef Name, StringRef P);

  void addCXXMethod(ASTContext &Context, DeclContext *DC, StringRef R,
                    StringRef Name, StringRef P,
                    HshBuiltinCXXMethod MethodKind);

  NamespaceDecl *findNamespace(StringRef Name, DeclContext *DC,
                               ASTContext &Context) const;

  EnumDecl *findEnum(StringRef Name, DeclContext *DC,
                     ASTContext &Context) const;

  CXXRecordDecl *findCXXRecord(StringRef Name, DeclContext *DC,
                               ASTContext &Context) const;

  ClassTemplateDecl *findClassTemplate(StringRef Name, DeclContext *DC,
                                       ASTContext &Context) const;

  FunctionTemplateDecl *findMethodTemplate(CXXRecordDecl *Class, StringRef Name,
                                           ASTContext &Context) const;

  FunctionTemplateDecl *findMethodTemplate(ClassTemplateDecl *Class,
                                           StringRef Name,
                                           ASTContext &Context) const;

  VarDecl *findVar(StringRef Name, DeclContext *DC, ASTContext &Context) const;

  llvm::APSInt findICEVar(StringRef Name, DeclContext *DC,
                          ASTContext &Context) const;

public:
  void findBuiltinDecls(ASTContext &Context);

  template <typename T> HshBuiltinType identifyBuiltinType(T Arg) const {
    bool IsAligned;
    return identifyBuiltinType(Arg, IsAligned);
  }

  HshBuiltinType identifyBuiltinType(QualType QT, bool &IsAligned) const;

  HshBuiltinType identifyBuiltinType(const clang::Type *UT,
                                     bool &IsAligned) const;

  HshBuiltinFunction identifyBuiltinFunction(const FunctionDecl *F) const;

  HshBuiltinCXXMethod identifyBuiltinMethod(const CXXMethodDecl *M) const;

  HshBuiltinPipelineField
  identifyBuiltinPipelineField(const FieldDecl *FD) const;

  HshStage stageOfBuiltinPipelineField(HshBuiltinPipelineField PF) const {
    return PipelineFields[PF].second;
  }

  static const CXXRecordDecl *
  FirstTemplateParamType(ClassTemplateSpecializationDecl *Derived,
                         ClassTemplateDecl *Decl);

  const CXXRecordDecl *getUniformRecord(const ParmVarDecl *PVD) const;

  const CXXRecordDecl *getVertexAttributeRecord(const ParmVarDecl *PVD) const;

  bool checkHshTypeCompatibility(const ASTContext &Context, const ValueDecl *VD,
                                 QualType Tp, bool AllowTextures) const;

  bool checkHshTypeCompatibility(const ASTContext &Context, const ValueDecl *VD,
                                 bool AllowTextures) const;

  bool checkHshFieldTypeCompatibility(const ASTContext &Context,
                                      const ValueDecl *VD) const;

  bool checkHshParamTypeCompatibility(const ASTContext &Context,
                                      const ValueDecl *VD) const;

  bool checkHshRecordCompatibility(const ASTContext &Context,
                                   const CXXRecordDecl *Record) const;

  bool checkHshFunctionCompatibility(const ASTContext &Context,
                                     const FunctionDecl *FD) const;

  HshStage determineVarStage(const VarDecl *VD) const;

  HshStage determinePipelineFieldStage(const FieldDecl *FD) const;

  static constexpr const Spellings &getSpellings(HshBuiltinType Tp) {
    return BuiltinTypeSpellings[Tp];
  }

  template <HshTarget T>
  static constexpr StringRef getSpelling(HshBuiltinType Tp);

  static constexpr const Spellings &getSpellings(HshBuiltinFunction Func) {
    return BuiltinFunctionSpellings[Func];
  }

  template <HshTarget T>
  static constexpr StringRef getSpelling(HshBuiltinFunction Func);

  static constexpr bool isVectorType(HshBuiltinType Tp) {
    return BuiltinTypeVector[Tp];
  }

  static constexpr unsigned getVectorSize(HshBuiltinType Tp) {
    switch (Tp) {
    case HBT_float2:
    case HBT_int2:
    case HBT_uint2:
      return 2;
    case HBT_float3:
    case HBT_int3:
    case HBT_uint3:
      return 3;
    case HBT_float4:
    case HBT_int4:
    case HBT_uint4:
      return 4;
    default:
      return 0;
    }
  }

  static constexpr bool isMatrixType(HshBuiltinType Tp) {
    return BuiltinTypeMatrix[Tp];
  }

  static constexpr unsigned getMatrixColumnCount(HshBuiltinType Tp) {
    switch (Tp) {
    case HBT_float3x3:
      return 3;
    case HBT_float4x4:
      return 4;
    default:
      return 0;
    }
  }

  static constexpr HshBuiltinType getMatrixColumnType(HshBuiltinType Tp) {
    switch (Tp) {
    case HBT_float3x3:
      return HBT_float3;
    case HBT_float4x4:
      return HBT_float4;
    default:
      return HBT_None;
    }
  }

  static constexpr bool isTextureType(HshBuiltinType Tp) {
    return BuiltinTypeTexture[Tp];
  }

  static constexpr bool isEnumType(HshBuiltinType Tp) {
    return BuiltinTypeEnum[Tp];
  }

  static constexpr bool isSwizzleMethod(HshBuiltinCXXMethod M) {
    return BuiltinMethodSwizzle[M];
  }

  static constexpr bool isInterpolationDistributed(HshBuiltinFunction Func) {
    return BuiltinFunctionInterpDists[Func];
  }

  const clang::TagDecl *getTypeDecl(HshBuiltinType Tp) const {
    return Types[Tp];
  }

  QualType getType(HshBuiltinType Tp) const {
    return getTypeDecl(Tp)->getTypeForDecl()->getCanonicalTypeUnqualified();
  }

  const TagDecl *getAlignedAvailable(FieldDecl *FD) const;

  static TypeSourceInfo *getFullyQualifiedTemplateSpecializationTypeInfo(
      ASTContext &Context, TemplateDecl *TDecl,
      const TemplateArgumentListInfo &Args);

  QualType getBindingRefType(ASTContext &Context) const;

  CXXMemberCallExpr *makeSpecializedRebindCall(
      ASTContext &Context, QualType SpecType, ParmVarDecl *BindingParm,
      ArrayRef<QualType> CallArgs, ArrayRef<Expr *> CallExprs) const;

  const class PipelineAttributes &getPipelineAttributes() const {
    return PipelineAttributes;
  }

  const ClassTemplateDecl *getPipelineRecordDecl() const {
    return PipelineAttributes.getPipelineDecl();
  }

  bool isDerivedFromPipelineDecl(CXXRecordDecl *Decl) const;

  ClassTemplateSpecializationDecl *
  getDerivedPipelineSpecialization(CXXRecordDecl *Decl) const;

  const CXXRecordDecl *getSamplerRecordDecl() const {
    return SamplerRecordType;
  }

  ClassTemplateDecl *getStdArrayDecl() const { return StdArrayType; }

  bool isStdArrayType(const CXXRecordDecl *RD) const;

  ClassTemplateDecl *getAlignedArrayDecl() const { return AlignedArrayType; }

  bool isAlignedArrayType(const CXXRecordDecl *RD) const;

  ClassTemplateDecl *getHshArrayDecl() const { return HshArrayType; }

  bool isHshArrayType(const CXXRecordDecl *RD) const;

  bool isZeroSizeHshArray(const FieldDecl *FD) const;

  bool isSupportedArrayType(const CXXRecordDecl *RD) const {
    return isStdArrayType(RD) || isAlignedArrayType(RD) || isHshArrayType(RD);
  }

  CXXFunctionalCastExpr *makeSamplerBinding(ASTContext &Context,
                                            ParmVarDecl *Tex,
                                            unsigned SamplerIdx,
                                            unsigned TextureIdx) const;

  CXXRecordDecl *makeBindingDerivative(ASTContext &Context,
                                       CXXRecordDecl *Source,
                                       StringRef Name) const;

  ClassTemplateDecl *
  makeBindingDerivative(ASTContext &Context, Sema &Actions,
                        ClassTemplateDecl *SpecializationSource,
                        StringRef Name) const;

  static void printEnumeratorString(raw_ostream &Out,
                                    const PrintingPolicy &Policy,
                                    const EnumDecl *ED,
                                    const llvm::APSInt &Val);

  static const EnumConstantDecl *
  lookupEnumConstantDecl(const EnumDecl *ED, const llvm::APSInt &Val);

  VarDecl *getConstDataVar(ASTContext &Context, DeclContext *DC,
                           HshTarget Target, uint32_t NumStages,
                           uint32_t NumBindings, uint32_t NumAttributes,
                           uint32_t NumSamplers,
                           uint32_t NumColorAttachments) const;

  VarDecl *getDataVar(ASTContext &Context, DeclContext *DC, HshTarget Target,
                      uint32_t NumStages, uint32_t NumSamplers) const;

  void printBuiltinEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                              HshBuiltinType HBT,
                              const llvm::APSInt &Val) const;

  void printBuiltinEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                              HshBuiltinType HBT, int64_t Val) const;

  void printTargetEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                             HshTarget Target) const;

  void printTargetEnumName(raw_ostream &Out, HshTarget Target) const;

  void printStageEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                            HshStage Stage) const;

  void printInputRateEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                                HshAttributeKind InputRate) const;

  void printFormatEnumString(raw_ostream &Out, const PrintingPolicy &Policy,
                             HshFormat Format) const;

  void printColorComponentFlagExpr(raw_ostream &Out,
                                   const PrintingPolicy &Policy,
                                   ColorComponentFlags Flags);

  HshFormat formatOfType(QualType Tp) const;

  unsigned getMaxUniforms() const { return MaxUniforms.getZExtValue(); }
  unsigned getMaxImages() const { return MaxImages.getZExtValue(); }
  unsigned getMaxSamplers() const { return MaxSamplers.getZExtValue(); }
  unsigned getMaxVertexBuffers() const {
    return MaxVertexBuffers.getZExtValue();
  }
};

template <>
constexpr StringRef HshBuiltins::getSpelling<HT_GLSL>(HshBuiltinType Tp) {
  return getSpellings(Tp).GLSL;
}
template <>
constexpr StringRef HshBuiltins::getSpelling<HT_HLSL>(HshBuiltinType Tp) {
  return getSpellings(Tp).HLSL;
}
template <>
constexpr StringRef HshBuiltins::getSpelling<HT_METAL>(HshBuiltinType Tp) {
  return getSpellings(Tp).Metal;
}

template <>
constexpr StringRef HshBuiltins::getSpelling<HT_GLSL>(HshBuiltinFunction Func) {
  return getSpellings(Func).GLSL;
}
template <>
constexpr StringRef HshBuiltins::getSpelling<HT_HLSL>(HshBuiltinFunction Func) {
  return getSpellings(Func).HLSL;
}
template <>
constexpr StringRef
HshBuiltins::getSpelling<HT_METAL>(HshBuiltinFunction Func) {
  return getSpellings(Func).Metal;
}

struct SamplerRecord {
  APValue Config;
};

struct SamplerBinding {
  unsigned RecordIdx;
  unsigned TextureIdx;
  ParmVarDecl *TextureDecl;
  unsigned UseStages;
};

struct UniformRecord {
  StringRef Name;
  const CXXRecordDecl *Record;
  unsigned UseStages;
};

struct AttributeRecord {
  StringRef Name;
  const CXXRecordDecl *Record;
  HshAttributeKind Kind;
};

struct FunctionRecord {
  const IdentifierInfo *Identifier;
  const FunctionDecl *Function;
  unsigned UseStages;
};

enum HshTextureKind {
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  HTK_##Name,
#include "BuiltinTypes.def"
};

constexpr HshTextureKind KindOfTextureType(HshBuiltinType Type) {
  switch (Type) {
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  case HBT_##Name:                                                             \
    return HTK_##Name;
#include "BuiltinTypes.def"
  default:
    llvm_unreachable("invalid texture kind");
  }
}

constexpr HshBuiltinType BuiltinTypeOfTexture(HshTextureKind Kind) {
  switch (Kind) {
#define BUILTIN_TEXTURE_TYPE(Name, GLSLf, GLSLi, GLSLu, HLSLf, HLSLi, HLSLu,   \
                             Metalf, Metali, Metalu)                           \
  case HTK_##Name:                                                             \
    return HBT_##Name;
#include "BuiltinTypes.def"
  }
}

struct TextureRecord {
  const ParmVarDecl *TexParm;
  HshTextureKind Kind;
  unsigned UseStages;
};

struct VertexBinding {
  uint32_t Binding;
  uint32_t Stride;
  HshAttributeKind InputRate;
};

struct VertexAttribute {
  uint32_t Offset;
  uint32_t Binding;
  HshFormat Format;
};

struct SampleCall {
  const CXXMemberCallExpr *Expr;
  const ParmVarDecl *Decl;
  unsigned SamplerIndex;
};

class CommaArgPrinter {
  raw_ostream &OS;
  bool NeedsComma = false;

public:
  explicit CommaArgPrinter(raw_ostream &OS) : OS(OS) {}
  raw_ostream &addArg() {
    if (NeedsComma)
      OS << ", ";
    else
      NeedsComma = true;
    return OS;
  }
};

} // namespace clang::hshgen
