#include "urde-test.cpp.hshhead"

enum EShaderType : uint8_t {
  ST_DiffuseOnly,
  ST_Normal,
  ST_Dynamic,
  ST_DynamicAlpha,
  ST_DynamicCharacter
};

enum EPostType : uint8_t {
  PT_Normal,
  PT_ThermalHot,
  PT_ThermalCold,
  PT_Solid,
  PT_MBShadow,
  PT_Disintegrate
};

enum EReflectionType : uint8_t {
  RT_None,
  RT_Simple,
  RT_Indirect
};

enum ETexCoordSource : uint8_t {
  TCS_Position,
  TCS_Normal,
  TCS_Tex0,
  TCS_Tex1,
  TCS_Tex2,
  TCS_Tex3,
  TCS_Tex4,
  TCS_Tex5,
  TCS_Tex6,
  TCS_Tex7,
  TCS_None
};

template <std::size_t NSkinSlots>
struct VertUniform {
  std::array<hsh::float4x4, NSkinSlots> objs;
  std::array<hsh::float4x4, NSkinSlots> objsInv;
  hsh::float4x4 mv;
  hsh::float4x4 mvInv;
  hsh::float4x4 proj;
};

struct TCGMatrix {
  hsh::float4x4 mtx;
  hsh::float4x4 postMtx;
};

struct ReflectMtx {
  hsh::float4x4 indMtx;
  hsh::float4x4 reflectMtx;
  float reflectAlpha;
};

struct Light {
  hsh::float4 pos;
  hsh::float4 dir;
  hsh::float4 color;
  hsh::float4 linAtt;
  hsh::float4 angAtt;
};


struct Fog {
  hsh::float4 color;
  float A;
  float B;
  float C;
  int mode;
};

#define URDE_MAX_LIGHTS 8

template <EPostType Ext>
struct FragmentUniform {
  std::array<Light, URDE_MAX_LIGHTS> lights;
  hsh::float4 ambient;
  hsh::float4 colorReg0;
  hsh::float4 colorReg1;
  hsh::float4 colorReg2;
  hsh::float4 mulColor;
  hsh::float4 addColor;
  Fog fog;
};
template <>
struct FragmentUniform<PT_ThermalHot> {
  hsh::float4 tmulColor;
  hsh::float4 taddColor;
};
template <>
struct FragmentUniform<PT_Solid> {
  hsh::float4 solidColor;
};
template <>
struct FragmentUniform<PT_MBShadow> {
  hsh::float4 shadowUp;
  float shadowId;
};
template <>
struct FragmentUniform<PT_Disintegrate> {
  hsh::float4 daddColor;
  Fog fog;
};

template <std::size_t NCol, std::size_t NUv, std::size_t NWeight>
struct VertData {
  hsh::float3 posIn;
  hsh::float3 normIn;
  std::array<hsh::float4, NCol> colIn;
  std::array<hsh::float2, NUv> uvIn;
  std::array<hsh::float4, NWeight> weightIn;
};

template <ETexCoordSource Source, bool Normalize, int MtxIdx, bool SampleAlpha>
struct PassTraits {
  static constexpr ETexCoordSource Source_ = Source;
  static constexpr bool Normalize_ = Normalize;
  static constexpr int MtxIdx_ = MtxIdx;
  static constexpr bool SampleAlpha_ = SampleAlpha;
};

template <bool CubeReflection>
struct DynReflectionTex { using type = hsh::texture2d<float>; };
template <>
struct DynReflectionTex<true> { using type = hsh::texturecube<float>; };
template <bool CubeReflection>
using DynReflectionTexType = typename DynReflectionTex<CubeReflection>::type;

constexpr hsh::sampler Samp;
constexpr hsh::sampler ClampSamp(hsh::Linear, hsh::Linear, hsh::Linear,
                                 hsh::ClampToBorder, hsh::ClampToBorder,
                                 hsh::ClampToBorder, 0.f, hsh::Never,
                                 hsh::OpaqueWhite);
constexpr hsh::sampler ClampEdgeSamp(hsh::Linear, hsh::Linear, hsh::Linear,
                                     hsh::ClampToEdge, hsh::ClampToEdge,
                                     hsh::ClampToEdge);
constexpr hsh::sampler ReflectSamp(hsh::Linear, hsh::Linear, hsh::Linear,
                                   hsh::ClampToBorder, hsh::ClampToBorder,
                                   hsh::ClampToBorder);

using namespace hsh::pipeline;

template <std::size_t NSkinSlots, std::size_t NCol, std::size_t NUv,
          std::size_t NWeight, EShaderType Type, EPostType Post,
          EReflectionType ReflectionType, bool WorldShadow,
          bool AlphaTest, bool CubeReflection, class LightmapTraits,
          class DiffuseTraits, class EmissiveTraits, class SpecularTraits,
          class ExtendedSpecularTraits, class ReflectionTraits, class AlphaTraits,
          hsh::BlendFactor SrcFactor, hsh::BlendFactor DstFactor,
          hsh::CullMode Cull, hsh::Compare DepthCompare,
          bool DepthWrite, bool ColorWrite, bool AlphaWrite>
struct DrawModel
    : pipeline<
          color_attachment<
              SrcFactor, DstFactor, hsh::Add, SrcFactor, DstFactor, hsh::Add,
              hsh::ColorComponentFlags(
                  (ColorWrite ? hsh::Red | hsh::Green | hsh::Blue : 0) |
                  (AlphaWrite ? hsh::Alpha : 0))>,
          cull_mode<Cull>, depth_compare<DepthCompare>, depth_write<DepthWrite>,
          early_depth_stencil<!AlphaTest>> {
  DrawModel(hsh::dynamic_uniform_buffer<VertUniform<NSkinSlots>> vu,
            hsh::dynamic_uniform_buffer<FragmentUniform<Post>> fragu,
            hsh::dynamic_uniform_buffer<std::array<TCGMatrix, 8>> tcgu,
            hsh::dynamic_uniform_buffer<ReflectMtx> refu,
            hsh::texture2d<float> Lightmap,
            hsh::texture2d<float> Diffuse,
            hsh::texture2d<float> Emissive,
            hsh::texture2d<float> Specular,
            hsh::texture2d<float> ExtendedSpecular,
            hsh::texture2d<float> Reflection,
            hsh::texture2d<float> Alpha,
            hsh::texture2d<float> ReflectionIndTex,
            hsh::texture2d<float> ExtTex0,
            hsh::texture2d<float> ExtTex1,
            hsh::texture2d<float> ExtTex2,
            DynReflectionTexType<CubeReflection> dynReflection,
            hsh::vertex_buffer<VertData<NCol, NUv, NWeight>> vd) {
    hsh::float4 mvPos;
    hsh::float4 mvNorm;

    hsh::float4 objPos;
    hsh::float4 objNorm;
    if (NSkinSlots) {
      objPos = hsh::float4(0.f);
      objNorm = hsh::float4(0.f);
      for (std::size_t i = 0; i < NSkinSlots; ++i) {
        objPos += (vu->objs[i] * hsh::float4(vd->posIn, 1.f)) * vd->weightIn[i / 4][i % 4];
        objNorm += (vu->objsInv[i] * hsh::float4(vd->normIn, 1.f)) * vd->weightIn[i / 4][i % 4];
      }
      objPos[3] = 1.f;
      objNorm = hsh::float4(hsh::normalize(objNorm.xyz()), 0.f);
      mvPos = vu->mv * objPos;
      mvNorm = float4(normalize((vu->mvInv * objNorm).xyz()), 0.f);
      this->position = vu->proj * mvPos;
    } else {
      objPos = float4(vd->posIn, 1.0);
      objNorm = float4(vd->normIn, 0.0);
      mvPos = vu->mv * objPos;
      mvNorm = vu->mvInv * objNorm;
      this->position = vu->proj * mvPos;
    }

    hsh::float2 LightmapUv = hsh::float2(0.f);
    hsh::float2 DiffuseUv = hsh::float2(0.f);
    hsh::float2 EmissiveUv = hsh::float2(0.f);
    hsh::float2 SpecularUv = hsh::float2(0.f);
    hsh::float2 ExtendedSpecularUv = hsh::float2(0.f);
    hsh::float2 ReflectionUv = hsh::float2(0.f);
    hsh::float2 AlphaUv = hsh::float2(0.f);
    hsh::float2 ShadowUv = hsh::float2(0.f);
    hsh::float2 DynReflectionUv = hsh::float2(0.f);
    hsh::float2 DynReflectionIndUv = hsh::float2(0.f);
    hsh::float2 ExtUv0 = hsh::float2(0.f);
    hsh::float2 ExtUv1 = hsh::float2(0.f);
    hsh::float2 ExtUv2 = hsh::float2(0.f);

#define EMIT_TCG(Pass)                                                         \
  if (Pass##Traits::Source_ != TCS_None) {                                     \
    if (Pass##Traits::MtxIdx_ >= 0) {                                          \
      hsh::float4 src;                                                         \
      switch (Pass##Traits::Source_) {                                         \
      case TCS_Position:                                                       \
        src = hsh::float4(objPos.xyz(), 1.f);                                  \
        break;                                                                 \
      case TCS_Normal:                                                         \
        src = hsh::float4(objNorm.xyz(), 1.f);                                 \
        break;                                                                 \
      case TCS_Tex0:                                                           \
        src = hsh::float4(vd->uvIn[0], 0.f, 1.f);                              \
        break;                                                                 \
      case TCS_Tex1:                                                           \
        src = hsh::float4(vd->uvIn[1], 0.f, 1.f);                              \
        break;                                                                 \
      case TCS_Tex2:                                                           \
        src = hsh::float4(vd->uvIn[2], 0.f, 1.f);                              \
        break;                                                                 \
      case TCS_Tex3:                                                           \
        src = hsh::float4(vd->uvIn[3], 0.f, 1.f);                              \
        break;                                                                 \
      case TCS_Tex4:                                                           \
        src = hsh::float4(vd->uvIn[4], 0.f, 1.f);                              \
        break;                                                                 \
      case TCS_Tex5:                                                           \
        src = hsh::float4(vd->uvIn[5], 0.f, 1.f);                              \
        break;                                                                 \
      case TCS_Tex6:                                                           \
        src = hsh::float4(vd->uvIn[6], 0.f, 1.f);                              \
        break;                                                                 \
      case TCS_Tex7:                                                           \
        src = hsh::float4(vd->uvIn[7], 0.f, 1.f);                              \
        break;                                                                 \
      }                                                                        \
      hsh::float3 tmp = ((*tcgu)[Pass##Traits::MtxIdx_].mtx * src).xyz();      \
      if (Pass##Traits::Normalize_)                                            \
        tmp = hsh::normalize(tmp);                                             \
      hsh::float4 tmpProj =                                                    \
          (*tcgu)[Pass##Traits::MtxIdx_].postMtx * hsh::float4(tmp, 1.f);      \
      Pass##Uv = (tmpProj / tmpProj.w).xy();                                   \
    } else {                                                                   \
      switch (Pass##Traits::Source_) {                                         \
      case TCS_Position:                                                       \
        Pass##Uv = objPos.xy();                                                \
        break;                                                                 \
      case TCS_Normal:                                                         \
        Pass##Uv = objNorm.xy();                                               \
        break;                                                                 \
      case TCS_Tex0:                                                           \
        Pass##Uv = vd->uvIn[0];                                                \
        break;                                                                 \
      case TCS_Tex1:                                                           \
        Pass##Uv = vd->uvIn[1];                                                \
        break;                                                                 \
      case TCS_Tex2:                                                           \
        Pass##Uv = vd->uvIn[2];                                                \
        break;                                                                 \
      case TCS_Tex3:                                                           \
        Pass##Uv = vd->uvIn[3];                                                \
        break;                                                                 \
      case TCS_Tex4:                                                           \
        Pass##Uv = vd->uvIn[4];                                                \
        break;                                                                 \
      case TCS_Tex5:                                                           \
        Pass##Uv = vd->uvIn[5];                                                \
        break;                                                                 \
      case TCS_Tex6:                                                           \
        Pass##Uv = vd->uvIn[6];                                                \
        break;                                                                 \
      case TCS_Tex7:                                                           \
        Pass##Uv = vd->uvIn[7];                                                \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
  }
EMIT_TCG(Lightmap)
EMIT_TCG(Diffuse)
EMIT_TCG(Emissive)
EMIT_TCG(Specular)
EMIT_TCG(ExtendedSpecular)
EMIT_TCG(Reflection)
EMIT_TCG(Alpha)

    if (ReflectionType != RT_None) {
      DynReflectionIndUv =
          hsh::normalize((refu->indMtx * hsh::float4(objPos.xyz(), 1.0)).xz()) *
              hsh::float2(0.5f) +
          hsh::float2(0.5f);
      DynReflectionUv = (refu->reflectMtx * hsh::float4(objPos.xyz(), 1.0)).xy();
    }

    hsh::float3 lighting;
    switch (Post) {
    case PT_ThermalHot:
    case PT_ThermalCold:
    case PT_Solid:
    case PT_MBShadow:
      lighting = hsh::float3(1.f);
      break;
    default:
      lighting = fragu->ambient;

      for (int i = 0; i < URDE_MAX_LIGHTS; ++i) {
        hsh::float3 delta = mvPos - fragu->lights[i].pos.xyz();
        float dist = hsh::length(delta);
        hsh::float3 deltaNorm = delta / dist;
        float angDot = max(dot(deltaNorm, fragu->lights[i].dir.xyz), 0.f);
        float att = 1.f /
            (fragu->lights[i].linAtt[2] * dist * dist +
            fragu->lights[i].linAtt[1] * dist +
            fragu->lights[i].linAtt[0]);
        float angAtt =
            fragu->lights[i].angAtt[2] * angDot * angDot +
            fragu->lights[i].angAtt[1] * angDot +
            fragu->lights[i].angAtt[0];
        hsh::float3 thisColor = fragu->lights[i].color * angAtt * att * hsh::max(dot(-deltaNorm, mvNorm.xyz()), 0.f);
        if (WorldShadow && i == 0)
          thisColor *= ExtTex0.sample(ShadowUv, ClampSamp).x;
        lighting += thisColor;
      }

      lighting = hsh::saturate(lighting);
      break;
    }

    hsh::float3 DynReflectionSample;
    switch (ReflectionType) {
    case RT_None:
      DynReflectionSample = hsh::float3(0.f);
      break;
    case RT_Simple:
      DynReflectionSample =
          dynReflection.sample(DynReflectionUv, ReflectSamp).xyz() *
          refu->reflectAlpha;
      break;
    case RT_Indirect:
      DynReflectionSample =
          dynReflection
              .sample((ReflectionIndTex.sample(DynReflectionIndUv, Samp).xw() -
                       hsh::float2(0.5f)) *
                              hsh::float2(0.5f) +
                          DynReflectionUv,
                      ReflectSamp)
              .xyz() *
          refu->reflectAlpha;
      break;
    }

    const hsh::float3 kRGBToYPrime = hsh::float3(0.257, 0.504, 0.098);

#define Sample(Pass) (Pass##Traits::SampleAlpha_ ? hsh::float3(Pass.sample(Pass##Uv, Samp).w) : Pass.sample(Pass##Uv, Samp).xyz())
#define SampleAlpha(Pass) (Pass##Traits::SampleAlpha_ ? Pass.sample(Pass##Uv, Samp).w : hsh::dot(Pass.sample(Pass##Uv, Samp).xyz(), kRGBToYPrime))

    switch (Type) {
    case ST_DiffuseOnly:
      this->colorOut[0] = hsh::float4(Sample(Diffuse), SampleAlpha(Alpha));
      break;
    case ST_Normal:
      this->colorOut[0] = hsh::float4(
          (Sample(Lightmap) * fragu->colorReg1.xyz() + lighting) *
                  Sample(Diffuse) +
              Sample(Emissive) +
              (Sample(Specular) + Sample(ExtendedSpecular) * lighting) *
                  Sample(Reflection) +
              DynReflectionSample,
          SampleAlpha(Alpha));
      break;
    case ST_Dynamic:
      this->colorOut[0] = hsh::float4(
          (Sample(Lightmap) * fragu->colorReg1.xyz() + lighting) *
                  (Sample(Diffuse) + Sample(Emissive)) *
                  fragu->colorReg1.xyz() +
              (Sample(Specular) + Sample(ExtendedSpecular) * lighting) *
                  Sample(Reflection) +
              DynReflectionSample,
          SampleAlpha(Alpha));
      break;
    case ST_DynamicAlpha:
      this->colorOut[0] = hsh::float4(
          (Sample(Lightmap) * fragu->colorReg1.xyz() + lighting) *
                  (Sample(Diffuse) + Sample(Emissive)) *
                  fragu->colorReg1.xyz() +
              (Sample(Specular) + Sample(ExtendedSpecular) * lighting) *
                  Sample(Reflection) +
              DynReflectionSample,
          SampleAlpha(Alpha) * fragu->colorReg1.w);
      break;
    case ST_DynamicCharacter:
      this->colorOut[0] = hsh::float4(
          (Sample(Lightmap) + lighting) * Sample(Diffuse) +
              Sample(Emissive) * fragu->colorReg1.xyz() +
              (Sample(Specular) + Sample(ExtendedSpecular) * lighting) *
                  Sample(Reflection) +
              DynReflectionSample,
          SampleAlpha(Alpha));
      break;
    }

    float fogZ;
    {
      float fogF = hsh::saturate((fragu->fog.A / (fragu->fog.B - (1.0 - this->position.z))) - fragu->fog.C);
      switch (fragu->fog.mode) {
      case 2:
        fogZ = fogF;
        break;
      case 4:
        fogZ = 1.f - hsh::exp2(-8.f * fogF);
        break;
      case 5:
        fogZ = 1.f - hsh::exp2(-8.f * fogF * fogF);
        break;
      case 6:
        fogZ = hsh::exp2(-8.f * (1.f - fogF));
        break;
      case 7:
        fogF = 1.f - fogF;
        fogZ = hsh::exp2(-8.f * fogF * fogF);
        break;
      default:
        fogZ = 0.f;
        break;
      }
      fogZ = hsh::saturate(fogZ);
    }

    switch (Post) {
    case PT_Normal:
      if (DstFactor == hsh::One)
        this->colorOut[0] = hsh::float4(hsh::lerp(this->colorOut[0], hsh::float4(0.0), fogZ).xyz(), this->colorOut[0].w);
      else
        this->colorOut[0] = hsh::float4(hsh::lerp(this->colorOut[0], fragu->fog.color, fogZ).xyz(), this->colorOut[0].w);
      this->colorOut[0] *= fragu->mulColor;
      this->colorOut[0] += fragu->addColor;
      break;
    case PT_ThermalHot:
      this->colorOut[0] = hsh::float4(ExtTex0.sample(ExtUv0, Samp).x) * fragu->tmulColor + fragu->taddColor;
      break;
    case PT_ThermalCold:
      this->colorOut[0] *= hsh::float4(0.75f);
      break;
    case PT_Solid:
      this->colorOut[0] = fragu->solidColor;
      break;
    case PT_MBShadow: {
      float idTexel = ExtTex0.sample(ExtUv0, Samp).w;
      float sphereTexel = ExtTex1.sample(ExtUv1, ClampEdgeSamp).x;
      float fadeTexel = ExtTex2.sample(ExtUv2, ClampEdgeSamp).w;
      float val = ((hsh::abs(idTexel - fragu->shadowId) < 0.001f)
                       ? (hsh::dot(mvNorm, fragu->shadowUp.xyz()) * fragu->shadowUp.w)
                       : 0.f) *
                  sphereTexel * fadeTexel;
      this->colorOut[0] = hsh::float4(0.f, 0.f, 0.f, val);
      break;
    }
    case PT_Disintegrate: {
      hsh::float4 texel0 = ExtTex0.sample(ExtUv0, Samp);
      hsh::float4 texel1 = ExtTex0.sample(ExtUv1, Samp);
      this->colorOut[0] = hsh::lerp(hsh::float4(0.0, 0.0, 0.0, 0.0), texel1, texel0);
      this->colorOut[0] = hsh::float4(fragu->daddColor.xyz() + this->colorOut[0].xyz(), this->colorOut[0].w);
      if (DstFactor == hsh::One)
        this->colorOut[0] = hsh::float4(hsh::lerp(this->colorOut[0], hsh::float4(0.0), fogZ).xyz(), this->colorOut[0].w);
      else
        this->colorOut[0] = hsh::float4(hsh::lerp(this->colorOut[0], fragu->fog.color, fogZ).xyz(), this->colorOut[0].w);
      break;
    }
    }

    if (AlphaTest && this->colorOut[0].w < 0.25f)
      hsh::discard();
  }
};

struct ModelInfo {
  std::size_t SkinSlots, Colors, UVs, Weights;
};

struct MaterialInfo {
  EShaderType ShaderType;
  EReflectionType ReflectionType;
  hsh::BlendFactor Src, Dst;
  bool AlphaTest;
};

struct ModelResources {
  hsh::dynamic_uniform_buffer_typeless vu;
  hsh::dynamic_uniform_buffer_typeless fragu;
  hsh::dynamic_uniform_buffer_typeless tcgu;
  hsh::dynamic_uniform_buffer_typeless refu;
  hsh::texture2d<float> Lightmap;
  hsh::texture2d<float> Diffuse;
  hsh::texture2d<float> Emissive;
  hsh::texture2d<float> Specular;
  hsh::texture2d<float> ExtendedSpecular;
  hsh::texture2d<float> Reflection;
  hsh::texture2d<float> Alpha;
  hsh::texture2d<float> ReflectionIndTex;
  hsh::texture2d<float> ExtTex0;
  hsh::texture2d<float> ExtTex1;
  hsh::texture2d<float> ExtTex2;
  hsh::texture2d<float> dynReflection;
  hsh::vertex_buffer_typeless vd;
};

#if 0
template <std::size_t NSkinSlots, std::size_t NCol, std::size_t NUv,
          std::size_t NWeight, EShaderType Type, EPostType Post,
          EReflectionType ReflectionType, bool WorldShadow,
          bool AlphaTest, bool CubeReflection, class LightmapTraits,
          class DiffuseTraits, class EmissiveTraits, class SpecularTraits,
          class ExtendedSpecularTraits, class ReflectionTraits, class AlphaTraits,
          hsh::BlendFactor SrcFactor, hsh::BlendFactor DstFactor,
          hsh::CullMode Cull, hsh::Compare DepthCompare,
          bool DepthWrite, bool ColorWrite, bool AlphaWrite>
template <ETexCoordSource Source, bool Normalize, int MtxIdx, bool SampleAlpha>
#endif

#if HSH_PROFILE_MODE
template <>
std::ostream &hsh::value_formatter::format<EShaderType>(std::ostream &out,
                                                        EShaderType val) {
  switch (val) {
  case ST_DiffuseOnly:
    return out << "ST_DiffuseOnly";
  case ST_Normal:
    return out << "ST_Normal";
  case ST_Dynamic:
    return out << "ST_Dynamic";
  case ST_DynamicAlpha:
    return out << "ST_DynamicAlpha";
  case ST_DynamicCharacter:
    return out << "ST_DynamicCharacter";
  }
}

template <>
std::ostream &hsh::value_formatter::format<EPostType>(std::ostream &out,
                                                      EPostType val) {
  switch (val) {
  case PT_Normal:
    return out << "PT_Normal";
  case PT_ThermalHot:
    return out << "PT_ThermalHot";
  case PT_ThermalCold:
    return out << "PT_ThermalCold";
  case PT_Solid:
    return out << "PT_Solid";
  case PT_MBShadow:
    return out << "PT_MBShadow";
  case PT_Disintegrate:
    return out << "PT_Disintegrate";
  }
}

template <>
std::ostream &hsh::value_formatter::format<EReflectionType>(std::ostream &out,
                                                            EReflectionType val) {
  switch (val) {
  case RT_None:
    return out << "RT_None";
  case RT_Simple:
    return out << "RT_Simple";
  case RT_Indirect:
    return out << "RT_Indirect";
  }
}
#endif

namespace {
template <
    std::size_t NSkinSlots, std::size_t NCol, std::size_t NUv,
    std::size_t NWeight, EShaderType Type, EPostType Post,
    EReflectionType ReflectionType, bool WorldShadow, bool AlphaTest,
    bool CubeReflection, class LightmapTraits, class DiffuseTraits,
    class EmissiveTraits, class SpecularTraits, class ExtendedSpecularTraits,
    class ReflectionTraits, class AlphaTraits, hsh::BlendFactor SrcFactor,
    hsh::BlendFactor DstFactor, hsh::CullMode Cull, hsh::Compare DepthCompare,
    bool DepthWrite, bool ColorWrite, bool AlphaWrite>
struct hshbinding_DrawModel : hsh::binding<
    hshbinding_DrawModel<NSkinSlots, NCol, NUv, NWeight, Type, Post,
        ReflectionType, WorldShadow, AlphaTest, CubeReflection, LightmapTraits,
        DiffuseTraits, EmissiveTraits, SpecularTraits, ExtendedSpecularTraits,
        ReflectionTraits, AlphaTraits, SrcFactor, DstFactor, Cull, DepthCompare,
        DepthWrite, ColorWrite, AlphaWrite>> {
  static hsh::detail::GlobalListNode global;
  template <hsh::Target T>
  static constexpr hsh::detail::ShaderConstData<T, 0, 0, 0, 0> cdata{};
  template <hsh::Target T>
  static constexpr hsh::detail::ShaderData<T, 0> data{};
  hshbinding_DrawModel(hsh::dynamic_uniform_buffer<VertUniform<NSkinSlots>> vu,
                hsh::dynamic_uniform_buffer<FragmentUniform<Post>> fragu,
                hsh::dynamic_uniform_buffer<std::array<TCGMatrix, 8>> tcgu,
                hsh::dynamic_uniform_buffer<ReflectMtx> refu,
                hsh::texture2d<float> Lightmap, hsh::texture2d<float> Diffuse,
                hsh::texture2d<float> Emissive, hsh::texture2d<float> Specular,
                hsh::texture2d<float> ExtendedSpecular,
                hsh::texture2d<float> Reflection, hsh::texture2d<float> Alpha,
                hsh::texture2d<float> ReflectionIndTex,
                hsh::texture2d<float> ExtTex0, hsh::texture2d<float> ExtTex1,
                hsh::texture2d<float> ExtTex2,
                DynReflectionTexType<CubeReflection> dynReflection,
                hsh::vertex_buffer<VertData<NCol, NUv, NWeight>> vd) :
      hsh::binding<
          hshbinding_DrawModel<NSkinSlots, NCol, NUv, NWeight, Type, Post,
              ReflectionType, WorldShadow, AlphaTest, CubeReflection, LightmapTraits,
              DiffuseTraits, EmissiveTraits, SpecularTraits, ExtendedSpecularTraits,
              ReflectionTraits, AlphaTraits, SrcFactor, DstFactor, Cull, DepthCompare,
              DepthWrite, ColorWrite, AlphaWrite>>(vu, fragu, tcgu, refu,
                  Lightmap, Diffuse, Emissive, Specular, ExtendedSpecular,
                  Reflection, Alpha, ReflectionIndTex, ExtTex0, ExtTex1, ExtTex2,
                  dynReflection, vd) {}
};
template <> hsh::detail::GlobalListNode hshbinding_DrawModel<
    1, 1, 1, 1, ST_Normal, PT_Normal, RT_None,
    false, true, false,
    PassTraits<TCS_Tex0, true, 0, true>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    hsh::One, hsh::One, hsh::CullNone,
    hsh::Always, true, true, false>::global{&global_build};
template <>
template <> hsh::detail::ShaderConstData<hsh::HLSL, 0, 0, 0, 0> hshbinding_DrawModel<
    1, 1, 1, 1, ST_Normal, PT_Normal, RT_None,
    false, true, false,
    PassTraits<TCS_Tex0, true, 0, true>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    hsh::One, hsh::One, hsh::CullNone,
    hsh::Always, true, true, false>::cdata<hsh::HLSL>{{}, {}, {}, {}, {}};
template <>
template <> hsh::detail::ShaderData<hsh::HLSL, 0> hshbinding_DrawModel<
    1, 1, 1, 1, ST_Normal, PT_Normal, RT_None,
    false, true, false,
    PassTraits<TCS_Tex0, true, 0, true>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    PassTraits<TCS_None, true, 0, false>,
    hsh::One, hsh::One, hsh::CullNone,
    hsh::Always, true, true, false>::data<hsh::HLSL>{{}};

template <typename... Args>
hsh::binding_typeless
hsh_DrawModel(std::size_t NSkinSlots, std::size_t NCol, std::size_t NUv,
                  std::size_t NWeight, EShaderType Type, EPostType Post,
                  EReflectionType ReflectionType, bool AlphaTest,
                  ETexCoordSource LightmapTraits_Source,
                  bool LightmapTraits_Normalize, int LightmapTraits_MtxIdx,
                  bool LightmapTraits_SampleAlpha, hsh::BlendFactor SrcFactor,
                  hsh::BlendFactor DstFactor, Args... Resources) {
#if HSH_PROFILE_MODE
  hsh::profile_context::instance.get("/home/jacko/llvm-project/libhsh/test/urde-test.cpp.hshprof").
  add("::hshbinding_DrawModel", NSkinSlots, NCol, NUv, NWeight, Type, Post, ReflectionType, "false", AlphaTest, "false",
      hsh::profiler::push("PassTraits"), LightmapTraits_Source, LightmapTraits_Normalize, LightmapTraits_MtxIdx,
      LightmapTraits_SampleAlpha, hsh::profiler::pop(),
      hsh::profiler::push("PassTraits"), "TCS_None", "true", "0", "false", hsh::profiler::pop(),
      hsh::profiler::push("PassTraits"), "TCS_None", "true", "0", "false", hsh::profiler::pop(),
      hsh::profiler::push("PassTraits"), "TCS_None", "true", "0", "false", hsh::profiler::pop(),
      hsh::profiler::push("PassTraits"), "TCS_None", "true", "0", "false", hsh::profiler::pop(),
      hsh::profiler::push("PassTraits"), "TCS_None", "true", "0", "false", hsh::profiler::pop(),
      hsh::profiler::push("PassTraits"), "TCS_None", "true", "0", "false", hsh::profiler::pop(),
      SrcFactor, DstFactor, "hsh::CullNone", "hsh::Always", "true", "true", "false");
#elif __has_include("urde-test.cpp.hshprof")
#include "urde-test.cpp.hshprof"
#else
  switch (NSkinSlots) {
  case 1:
    switch (NCol) {
    case 1:
      switch (NUv) {
      case 1:
        switch (NWeight) {
        case 1:
          switch (Type) {
          case ST_Normal:
            switch (Post) {
            case PT_Normal:
              switch (ReflectionType) {
              case RT_None:
                switch (AlphaTest) {
                case true:
                  switch (LightmapTraits_Source) {
                  case TCS_Tex0:
                    switch (LightmapTraits_Normalize) {
                    case true:
                      switch (LightmapTraits_MtxIdx) {
                      case 0:
                        switch (LightmapTraits_SampleAlpha) {
                        case true:
                          switch (SrcFactor) {
                          case hsh::One:
                            switch (DstFactor) {
                            case hsh::One:
                              return ::hshbinding_DrawModel<
                                  1, 1, 1, 1, ST_Normal, PT_Normal, RT_None,
                                  false, true, false,
                                  PassTraits<TCS_Tex0, true, 0, true>,
                                  PassTraits<TCS_None, true, 0, false>,
                                  PassTraits<TCS_None, true, 0, false>,
                                  PassTraits<TCS_None, true, 0, false>,
                                  PassTraits<TCS_None, true, 0, false>,
                                  PassTraits<TCS_None, true, 0, false>,
                                  PassTraits<TCS_None, true, 0, false>,
                                  hsh::One, hsh::One, hsh::CullNone,
                                  hsh::Always, true, true, false>(Resources...);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
#endif
  return {};
}
}

#define hsh_DrawModel(...) ::hsh_DrawModel( \
model.SkinSlots, model.Colors, model.UVs, model.Weights, \
mat.ShaderType, post, mat.ReflectionType, mat.AlphaTest, \
alphaSource, alphaNormalize, mtxIdx, sampleAlpha, \
mat.Src, mat.Dst, \
res.vu, res.fragu, res.tcgu, res.refu, res.Lightmap, res.Diffuse, \
res.Emissive, res.Specular, res.ExtendedSpecular, res.Reflection, \
res.Alpha, res.ReflectionIndTex, res.ExtTex0, res.ExtTex1, \
res.ExtTex2, res.dynReflection, res.vd)

hsh::binding_typeless BindDrawModel(const ModelInfo &model,
                   const MaterialInfo &mat,
                   EPostType post,
                   const ModelResources &res) {
  ETexCoordSource alphaSource = TCS_Tex0;
  bool alphaNormalize = true;
  int mtxIdx = 0;
  bool sampleAlpha = true;

  using DefaultPass = PassTraits<TCS_None, true, 0, false>;

  return hsh_DrawModel(::DrawModel<
      model.SkinSlots, model.Colors, model.UVs, model.Weights,
      mat.ShaderType, post, mat.ReflectionType, false, mat.AlphaTest, false,
      PassTraits<alphaSource, alphaNormalize, mtxIdx, sampleAlpha>,
      DefaultPass, DefaultPass, DefaultPass, DefaultPass, DefaultPass, DefaultPass,
      mat.Src, mat.Dst, hsh::CullNone, hsh::Always, true, true, false>(
          res.vu, res.fragu, res.tcgu, res.refu, res.Lightmap, res.Diffuse,
          res.Emissive, res.Specular, res.ExtendedSpecular, res.Reflection,
          res.Alpha, res.ReflectionIndTex, res.ExtTex0, res.ExtTex1,
          res.ExtTex2, res.dynReflection, res.vd));
}
