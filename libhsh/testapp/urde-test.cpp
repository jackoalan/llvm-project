#include "urde-test.h"

template <ETexCoordSource Source, bool Normalize, int MtxIdx, bool SampleAlpha>
struct PassTraits {
  static constexpr ETexCoordSource Source_ = Source;
  static constexpr bool Normalize_ = Normalize;
  static constexpr int MtxIdx_ = MtxIdx;
  static constexpr bool SampleAlpha_ = SampleAlpha;
};

#include "urde-test.cpp.hshhead"

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

template <uint32_t NSkinSlots, uint32_t NCol, uint32_t NUv,
          uint32_t NWeight, EShaderType Type, EPostType Post,
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
                  (ColorWrite ? hsh::CC_Red | hsh::CC_Green | hsh::CC_Blue : 0) |
                  (AlphaWrite ? hsh::CC_Alpha : 0))>,
          cull_mode<Cull>, depth_compare<DepthCompare>, depth_write<DepthWrite>,
          early_depth_stencil<!AlphaTest>> {
  DrawModel(hsh::dynamic_uniform_buffer<VertUniform<NSkinSlots>> vu,
            hsh::dynamic_uniform_buffer<FragmentUniform<Post>> fragu [[hsh::fragment]],
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
    if constexpr (NSkinSlots) {
      objPos = hsh::float4(0.f);
      objNorm = hsh::float4(0.f);
      for (uint32_t i = 0; i < NSkinSlots; ++i) {
        objPos += (vu->objs[i] * hsh::float4(vd->posIn, 1.f)) * vd->weightIn[i / 4][i % 4];
        objNorm += (vu->objsInv[i] * hsh::float4(vd->normIn, 1.f)) * vd->weightIn[i / 4][i % 4];
      }
      objPos[3] = 1.f;
      objNorm = hsh::float4(hsh::normalize(objNorm.xyz()), 0.f);
      mvPos = vu->mv * objPos;
      mvNorm = hsh::float4(hsh::normalize((vu->mvInv * objNorm).xyz()), 0.f);
      this->position = vu->proj * mvPos;
    } else {
      objPos = hsh::float4(vd->posIn, 1.f);
      objNorm = hsh::float4(vd->normIn, 0.f);
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
  if constexpr (Pass##Traits::Source_ != TCS_None) {                           \
    if constexpr (Pass##Traits::MtxIdx_ >= 0) {                                \
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
      case TCS_None:                                                           \
        break;                                                                 \
      }                                                                        \
      hsh::float3 tmp = ((*tcgu)[Pass##Traits::MtxIdx_].mtx * src).xyz();      \
      if constexpr (Pass##Traits::Normalize_)                                  \
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
      case TCS_None:                                                           \
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

    if constexpr (ReflectionType != RT_None) {
      DynReflectionIndUv =
          hsh::normalize((refu->indMtx * hsh::float4(objPos.xyz(), 1.f)).xz()) *
              hsh::float2(0.5f) +
          hsh::float2(0.5f);
      DynReflectionUv = (refu->reflectMtx * hsh::float4(objPos.xyz(), 1.f)).xy();
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
      lighting = fragu->ambient.xyz();

      for (int i = 0; i < URDE_MAX_LIGHTS; ++i) {
        hsh::float3 delta = mvPos.xyz() - fragu->lights[i].pos.xyz();
        float dist = hsh::length(delta);
        hsh::float3 deltaNorm = delta / dist;
        float angDot = hsh::max(hsh::dot(deltaNorm, fragu->lights[i].dir.xyz()), 0.f);
        float att = 1.f /
            (fragu->lights[i].linAtt[2] * dist * dist +
            fragu->lights[i].linAtt[1] * dist +
            fragu->lights[i].linAtt[0]);
        float angAtt =
            fragu->lights[i].angAtt[2] * angDot * angDot +
            fragu->lights[i].angAtt[1] * angDot +
            fragu->lights[i].angAtt[0];
        hsh::float3 thisColor = fragu->lights[i].color.xyz() * angAtt * att * hsh::max(hsh::dot(-deltaNorm, mvNorm.xyz()), 0.f);
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

    const hsh::float3 kRGBToYPrime = hsh::float3(0.257f, 0.504f, 0.098f);

#define Sample(Pass) (Pass##Traits::SampleAlpha_ ? hsh::float3(Pass.sample(Pass##Uv, Samp).w) : Pass.sample(Pass##Uv, Samp).xyz())
#define SampleAlpha(Pass) (Pass##Traits::SampleAlpha_ ? Pass.sample(Pass##Uv, Samp).w : hsh::dot(Pass.sample(Pass##Uv, Samp).xyz(), kRGBToYPrime))

    switch (Type) {
    case ST_DiffuseOnly:
      this->color_out[0] = hsh::float4(Sample(Diffuse), SampleAlpha(Alpha));
      break;
    case ST_Normal:
      this->color_out[0] = hsh::float4(
          (Sample(Lightmap) * fragu->colorReg1.xyz() + lighting) *
                  Sample(Diffuse) +
              Sample(Emissive) +
              (Sample(Specular) + Sample(ExtendedSpecular) * lighting) *
                  Sample(Reflection) +
              DynReflectionSample,
          SampleAlpha(Alpha));
      break;
    case ST_Dynamic:
      this->color_out[0] = hsh::float4(
          (Sample(Lightmap) * fragu->colorReg1.xyz() + lighting) *
                  (Sample(Diffuse) + Sample(Emissive)) *
                  fragu->colorReg1.xyz() +
              (Sample(Specular) + Sample(ExtendedSpecular) * lighting) *
                  Sample(Reflection) +
              DynReflectionSample,
          SampleAlpha(Alpha));
      break;
    case ST_DynamicAlpha:
      this->color_out[0] = hsh::float4(
          (Sample(Lightmap) * fragu->colorReg1.xyz() + lighting) *
                  (Sample(Diffuse) + Sample(Emissive)) *
                  fragu->colorReg1.xyz() +
              (Sample(Specular) + Sample(ExtendedSpecular) * lighting) *
                  Sample(Reflection) +
              DynReflectionSample,
          SampleAlpha(Alpha) * fragu->colorReg1.w);
      break;
    case ST_DynamicCharacter:
      this->color_out[0] = hsh::float4(
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
      float fogF = hsh::saturate((fragu->fog.A / (fragu->fog.B - (1.f - this->position.z))) - fragu->fog.C);
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

    if constexpr (Post == PT_Normal) {
      if constexpr (DstFactor == hsh::One)
        this->color_out[0] = hsh::float4(hsh::lerp(this->color_out[0], hsh::float4(0.f), fogZ).xyz(), this->color_out[0].w);
      else
        this->color_out[0] = hsh::float4(hsh::lerp(this->color_out[0], fragu->fog.color, fogZ).xyz(), this->color_out[0].w);
      this->color_out[0] *= fragu->mulColor;
      this->color_out[0] += fragu->addColor;
    } else if constexpr (Post == PT_ThermalHot) {
      this->color_out[0] = hsh::float4(ExtTex0.sample(ExtUv0, Samp).x) * fragu->tmulColor + fragu->taddColor;
    } else if constexpr (Post == PT_ThermalCold) {
      this->color_out[0] *= hsh::float4(0.75f);
    } else if constexpr (Post == PT_Solid) {
      this->color_out[0] = fragu->solidColor;
    } else if constexpr (Post == PT_MBShadow) {
      float idTexel = ExtTex0.sample(ExtUv0, Samp).w;
      float sphereTexel = ExtTex1.sample(ExtUv1, ClampEdgeSamp).x;
      float fadeTexel = ExtTex2.sample(ExtUv2, ClampEdgeSamp).w;
      float val = ((hsh::abs(idTexel - fragu->shadowId) < 0.001f)
                   ? (hsh::dot(mvNorm, fragu->shadowUp.xyz()) * fragu->shadowUp.w)
                   : 0.f) *
                  sphereTexel * fadeTexel;
      this->color_out[0] = hsh::float4(0.f, 0.f, 0.f, val);
    } else if constexpr (Post == PT_Disintegrate) {
      hsh::float4 texel0 = ExtTex0.sample(ExtUv0, Samp);
      hsh::float4 texel1 = ExtTex0.sample(ExtUv1, Samp);
      this->color_out[0] = hsh::lerp(hsh::float4(0.f), texel1, texel0);
      this->color_out[0] = hsh::float4(fragu->daddColor.xyz() + this->color_out[0].xyz(), this->color_out[0].w);
      if constexpr (DstFactor == hsh::One)
        this->color_out[0] = hsh::float4(hsh::lerp(this->color_out[0], hsh::float4(0.f), fogZ).xyz(), this->color_out[0].w);
      else
        this->color_out[0] = hsh::float4(hsh::lerp(this->color_out[0], fragu->fog.color, fogZ).xyz(), this->color_out[0].w);
    }

    if (AlphaTest && this->color_out[0].w < 0.25f)
      hsh::discard();
  }
};

ModelInfo CreateModelInfo() {
  return {1, 0, 4, 1};
}

MaterialInfo CreateMaterialInfo() {
  return {EShaderType::ST_Normal, EReflectionType::RT_None, hsh::SrcAlpha,
          hsh::InvSrcAlpha, false};
}

ModelResources CreateModelResources() {
  return {
    hsh::create_dynamic_uniform_buffer<VertUniform<4>>(),
    hsh::create_dynamic_uniform_buffer<FragmentUniform<PT_Normal>>(),
    hsh::create_dynamic_uniform_buffer<std::array<TCGMatrix, 8>>(),
    hsh::create_dynamic_uniform_buffer<ReflectMtx>(),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_texture2d<float>({256, 256}, hsh::RGBA8_UNORM, 1, [](void *Data, std::size_t Size){memset(Data, 0, Size);}),
    hsh::create_vertex_buffer<VertData<0, 4, 1>>(VertData<0, 4, 1>())
  };
}

#if 0
template <uint32_t NSkinSlots, uint32_t NCol, uint32_t NUv,
          uint32_t NWeight, EShaderType Type, EPostType Post,
          EReflectionType ReflectionType, bool WorldShadow,
          bool AlphaTest, bool CubeReflection, class LightmapTraits,
          class DiffuseTraits, class EmissiveTraits, class SpecularTraits,
          class ExtendedSpecularTraits, class ReflectionTraits, class AlphaTraits,
          hsh::BlendFactor SrcFactor, hsh::BlendFactor DstFactor,
          hsh::CullMode Cull, hsh::Compare DepthCompare,
          bool DepthWrite, bool ColorWrite, bool AlphaWrite>
template <ETexCoordSource Source, bool Normalize, int MtxIdx, bool SampleAlpha>
#endif

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
          res.vu.get(), res.fragu.get(), res.tcgu.get(), res.refu.get(), res.Lightmap.get(), res.Diffuse.get(),
          res.Emissive.get(), res.Specular.get(), res.ExtendedSpecular.get(), res.Reflection.get(),
          res.Alpha.get(), res.ReflectionIndTex.get(), res.ExtTex0.get(), res.ExtTex1.get(),
          res.ExtTex2.get(), res.dynReflection.get(), res.vd.get()));
}
