//===--- HshGenerator.h - Lambda scanner and codegen for hsh tool ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "clang/Frontend/FrontendAction.h"

namespace clang::hshgen {

/* Keep in sync with targets.def in libhsh!! */
enum HshTarget : int {
  HT_NULL,
  HT_GLSL,
  HT_HLSL,
  HT_DXBC,
  HT_DXIL,
  HT_VULKAN_SPIRV,
  HT_METAL,
  HT_METAL_BIN_MAC,
  HT_METAL_BIN_IOS,
  HT_DEKO3D,
  HT_DEKO3D_CONTROL, // Pseudo-target for establishing control section
  HT_MAX
};

constexpr StringRef HshTargetToString(HshTarget Target) {
  switch (Target) {
  default:
  case HT_NULL:
    return llvm::StringLiteral("null");
  case HT_GLSL:
    return llvm::StringLiteral("glsl");
  case HT_HLSL:
    return llvm::StringLiteral("hlsl");
  case HT_DXBC:
    return llvm::StringLiteral("dxbc");
  case HT_DXIL:
    return llvm::StringLiteral("dxil");
  case HT_VULKAN_SPIRV:
    return llvm::StringLiteral("vulkan-spirv");
  case HT_METAL:
    return llvm::StringLiteral("metal");
  case HT_METAL_BIN_MAC:
    return llvm::StringLiteral("metal-bin-mac");
  case HT_METAL_BIN_IOS:
    return llvm::StringLiteral("metal-bin-ios");
  case HT_DEKO3D:
    return llvm::StringLiteral("deko3d");
  }
}

enum HshStage : int {
  HshNoStage = -1,
  HshVertexStage = 0,
  HshControlStage,
  HshEvaluationStage,
  HshGeometryStage,
  HshFragmentStage,
  HshMaxStage
};

constexpr StringRef HshStageToString(HshStage Stage) {
  switch (Stage) {
  case HshVertexStage:
    return "vertex";
  case HshControlStage:
    return "control";
  case HshEvaluationStage:
    return "evaluation";
  case HshGeometryStage:
    return "geometry";
  case HshFragmentStage:
    return "fragment";
  default:
    return "none";
  }
}

enum HshAttributeKind { PerVertex, PerInstance };

enum HshFormat : uint8_t {
  R8_UNORM,
  RG8_UNORM,
  RGBA8_UNORM,
  R16_UNORM,
  RG16_UNORM,
  RGBA16_UNORM,
  R32_UINT,
  RG32_UINT,
  RGB32_UINT,
  RGBA32_UINT,
  R8_SNORM,
  RG8_SNORM,
  RGBA8_SNORM,
  R16_SNORM,
  RG16_SNORM,
  RGBA16_SNORM,
  R32_SINT,
  RG32_SINT,
  RGB32_SINT,
  RGBA32_SINT,
  R32_SFLOAT,
  RG32_SFLOAT,
  RGB32_SFLOAT,
  RGBA32_SFLOAT,
  BC1_UNORM,
  BC2_UNORM,
  BC3_UNORM,
};

enum Topology {
  TPL_Points,
  TPL_Lines,
  TPL_LineStrip,
  TPL_Triangles,
  TPL_TriangleStrip,
  TPL_TriangleFan,
  TPL_Patches
};

enum CullMode { CM_CullNone, CM_CullFront, CM_CullBack, CM_CullFrontAndBack };

enum Compare {
  CMP_Never,
  CMP_Less,
  CMP_Equal,
  CMP_LEqual,
  CMP_Greater,
  CMP_NEqual,
  CMP_GEqual,
  CMP_Always
};

enum BlendFactor {
  BF_Zero,
  BF_One,
  BF_SrcColor,
  BF_InvSrcColor,
  BF_DstColor,
  BF_InvDstColor,
  BF_SrcAlpha,
  BF_InvSrcAlpha,
  BF_DstAlpha,
  BF_InvDstAlpha,
  BF_ConstColor,
  BF_InvConstColor,
  BF_ConstAlpha,
  BF_InvConstAlpha,
  BF_Src1Color,
  BF_InvSrc1Color,
  BF_Src1Alpha,
  BF_InvSrc1Alpha
};

enum BlendOp { BO_Add, BO_Subtract, BO_ReverseSubtract };

enum ColorComponentFlags : unsigned {
  CC_Red = 1,
  CC_Green = 2,
  CC_Blue = 4,
  CC_Alpha = 8
};

struct StageBits {
  unsigned Bits = 0;
  operator unsigned() const { return Bits; }
  StageBits &operator=(unsigned NewBits) {
    Bits = NewBits;
    return *this;
  }
  StageBits &operator|=(unsigned NewBits) {
    Bits |= NewBits;
    return *this;
  }
};

class GenerateAction : public ASTFrontendAction {
  OwningArrayRef<HshTarget> Targets;
  SmallString<256> ProfilePath;
  bool DebugInfo, SourceDump;

public:
  explicit GenerateAction(ArrayRef<HshTarget> Targets, bool DebugInfo = false,
                          bool SourceDump = false, StringRef ProfilePath = {})
      : Targets(Targets), ProfilePath(ProfilePath), DebugInfo(DebugInfo),
        SourceDump(SourceDump) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override;
};

} // namespace clang::hshgen
