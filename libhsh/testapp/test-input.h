#pragma once
#include <hsh/hsh.h>

namespace MyNS {
enum class PostMode {
  Nothing,
  AddDynamicColor,
  AddDynamicColor2,
  MultiplyDynamicColor
};

struct MyFormat {
  hsh::float3 position;
  hsh::float3 normal;
};

struct UniformData {
  hsh::float4x4 xf;
  hsh::float3 lightDir;
  float afloat;
  hsh::aligned_float3x3 dynColor;
  float bfloat;
};

enum AlphaMode {
  AM_NoAlpha, AM_Alpha
};

template <AlphaMode AM>
struct AlphaTraits {
  static constexpr AlphaMode Mode = AM;
};

struct Binding {
  hsh::dynamic_owner<hsh::uniform_buffer<UniformData>> Uniform;
  hsh::owner<hsh::vertex_buffer<MyFormat>> VBO;
  hsh::owner<hsh::texture2d> Tex;
  hsh::binding Binding;
};
Binding BuildPipeline();
Binding BuildPipelineTemplated(bool Something, MyNS::AlphaMode AM);
}
