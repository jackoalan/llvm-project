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
  hsh::float4 color;
};

struct UniformData {
  hsh::float4x4 xf;
  hsh::float3 lightDir;
  float afloat;
  hsh::aligned_float3x3 dynColor;
  float bfloat;
  alignas(16) hsh::float4 color;
};

enum AlphaMode {
  AM_NoAlpha, AM_Alpha
};

template <AlphaMode AM>
struct AlphaTraits {
  static constexpr AlphaMode Mode = AM;
};

struct Binding {
  hsh::owner<hsh::texture2d> Tex;
  hsh::binding Binding;
};
hsh::binding& BindPipeline(Binding& b, hsh::uniform_buffer_typeless u, hsh::vertex_buffer_typeless v);
hsh::binding& BindPipelineTemplated(Binding& b, hsh::uniform_buffer_typeless u, hsh::vertex_buffer_typeless v, bool Something, MyNS::AlphaMode AM);
}
