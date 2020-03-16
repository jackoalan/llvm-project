#pragma once
#include <hsh/hsh.h>

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

template <uint32_t NSkinSlots>
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

template <uint32_t NCol, uint32_t NUv, uint32_t NWeight>
struct VertData {
  hsh::float3 posIn;
  hsh::float3 normIn;
  std::array<hsh::float4, NCol> colIn;
  std::array<hsh::float2, NUv> uvIn;
  std::array<hsh::float4, NWeight> weightIn;
};

struct ModelInfo {
  uint32_t SkinSlots, Colors, UVs, Weights;
};

struct MaterialInfo {
  EShaderType ShaderType;
  EReflectionType ReflectionType;
  hsh::BlendFactor Src, Dst;
  bool AlphaTest;
};

struct ModelResources {
  hsh::resource_owner<hsh::dynamic_uniform_buffer<VertUniform<4>>> vu;
  hsh::resource_owner<hsh::dynamic_uniform_buffer<FragmentUniform<PT_Normal>>> fragu;
  hsh::resource_owner<hsh::dynamic_uniform_buffer<std::array<TCGMatrix, 8>>> tcgu;
  hsh::resource_owner<hsh::dynamic_uniform_buffer<ReflectMtx>> refu;
  hsh::resource_owner<hsh::texture2d<float>> Lightmap;
  hsh::resource_owner<hsh::texture2d<float>> Diffuse;
  hsh::resource_owner<hsh::texture2d<float>> Emissive;
  hsh::resource_owner<hsh::texture2d<float>> Specular;
  hsh::resource_owner<hsh::texture2d<float>> ExtendedSpecular;
  hsh::resource_owner<hsh::texture2d<float>> Reflection;
  hsh::resource_owner<hsh::texture2d<float>> Alpha;
  hsh::resource_owner<hsh::texture2d<float>> ReflectionIndTex;
  hsh::resource_owner<hsh::texture2d<float>> ExtTex0;
  hsh::resource_owner<hsh::texture2d<float>> ExtTex1;
  hsh::resource_owner<hsh::texture2d<float>> ExtTex2;
  hsh::resource_owner<hsh::texture2d<float>> dynReflection;
  hsh::resource_owner<hsh::vertex_buffer<VertData<0, 4, 1>>> vd;
};

ModelInfo CreateModelInfo();
MaterialInfo CreateMaterialInfo();
ModelResources CreateModelResources();

hsh::binding_typeless BindDrawModel(const ModelInfo &model,
                                    const MaterialInfo &mat,
                                    EPostType post,
                                    const ModelResources &res);
