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
  constexpr MyFormat(hsh::float3 position, hsh::float3 normal)
      : position(position), normal(normal) {}
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
}

#include "test-input.cpp.hshhead"

namespace MyNS {

using namespace hsh::pipeline;

constexpr hsh::sampler TestSampler(hsh::Nearest, hsh::Nearest, hsh::Linear);

#if 1
struct DrawSomething : pipeline<color_attachment<>> {
  DrawSomething(hsh::dynamic_uniform_buffer<UniformData> u,
                hsh::vertex_buffer<MyFormat> v,
                hsh::texture2d<float> tex0) {
    position = u->xf * hsh::float4{v->position, 1.f};
    hsh::float3x3 normalXf = u->xf;
    hsh::float3 finalNormal = normalXf * v->normal;
    color_out[0] = hsh::float4{hsh::float3{
        tex0.sample({0.f, 0.f}, TestSampler).xyz() *
        hsh::dot(finalNormal, -u->lightDir)}, 0.f};
  }
};
#endif

#if 1
template <bool Something, class AT>
struct DrawSomethingTemplated : pipeline<color_attachment<>> {
  DrawSomethingTemplated(hsh::dynamic_uniform_buffer<UniformData> u,
                         hsh::vertex_buffer<MyFormat> v,
                         hsh::texture2d<float> tex0) {
    position = u->xf * hsh::float4{v->position, 1.f};
    hsh::float3x3 normalXf = u->xf;
    hsh::float3 finalNormal = normalXf * v->normal;
    if (AT::Mode == AM_NoAlpha) {
      color_out[0] = hsh::float4{hsh::float3{
          tex0.sample({0.f, 0.f}, TestSampler).xyz() *
          hsh::dot(finalNormal, -u->lightDir)}, 0.f};
    } else {
      color_out[0] = hsh::float4{hsh::float3{
          tex0.sample({0.f, 0.f}, TestSampler).xyz() *
          hsh::dot(finalNormal, -u->lightDir)}, 1.f};
    }
  }
};
template struct DrawSomethingTemplated<false, AlphaTraits<MyNS::AlphaMode(0)>>;
template struct DrawSomethingTemplated<false, AlphaTraits<MyNS::AlphaMode(1)>>;
#endif

#if 1
hsh::binding_typeless BindDrawSomething(hsh::dynamic_uniform_buffer_typeless u,
                                        hsh::vertex_buffer_typeless v,
                                        hsh::texture2d<float> tex0) {
  return hsh_DrawSomething(DrawSomething(u, v, tex0));
}
#endif

#if 1
hsh::binding_typeless BindDrawSomethingTemplated(hsh::dynamic_uniform_buffer_typeless u,
                                        hsh::vertex_buffer_typeless v,
                                        hsh::texture2d<float> tex0,
                                        AlphaMode AMode) {
  return hsh_DrawSomethingTemplated(DrawSomethingTemplated<false, AlphaTraits<AMode>>(u, v, tex0));
}
#endif

}
