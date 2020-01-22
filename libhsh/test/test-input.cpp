#include <hsh.h>

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
}

#include "test-input.cpp.hshhead"
#if 0
namespace hshprof_DrawSomethingTemplated_specializations {
using s0 = MyNS::DrawSomethingTemplated<true>;
using s1 = MyNS::DrawSomethingTemplated<false>;
using s2 = MyNS::DrawSomethingTemplated<false>;
using s3 = MyNS::DrawSomethingTemplated<false>;
using s4 = MyNS::DrawSomethingTemplated<false>;
}
#endif
#if 0
#define hshprof_DrawSomethingTemplated \
switch (ZeroAlpha) { \
case 1: \
return ::hshbinding_DrawSomethingTemplated<1>(Resources...); \
case 0: \
return ::hshbinding_DrawSomethingTemplated<0>(Resources...); \
}
#endif

#if 0
namespace {
template <typename... Res>
hsh::binding_typeless hsh_DrawSomething(Res... Resources) {
  return ::hshbinding_DrawSomething(Resources...);
}
#define hsh_DrawSomething(...) ::hsh_DrawSomething(u, v, tex0)

template <typename... Res>
hsh::binding_typeless hsh_DrawSomethingTemplated(bool ZeroAlpha, Res... Resources) {
#if HSH_PROFILE_MODE
  hsh::profile_context::instance
      .get("/home/jacko/llvm-project/libhsh/test/test-input.cpp.hshprof",
           "hshprof_DrawSomethingTemplated", "MyNS::DrawSomethingTemplated",
           "::hshbinding_DrawSomethingTemplated")
      .add(ZeroAlpha);
#elif hshprof_DrawSomethingTemplated
hshprof_DrawSomethingTemplated
#undef hshprof_DrawSomethingTemplated
#else
  /* Generated based on explicit specializations */
  switch (ZeroAlpha) {
  case 1:
    return ::hshbinding_DrawSomethingTemplated<1>(Resources...);
  case 0:
    return ::hshbinding_DrawSomethingTemplated<0>(Resources...);
  }
#endif
  return {};
}
#define hsh_DrawSomethingTemplated(...) ::hsh_DrawSomethingTemplated(ZeroAlpha, u, v, tex0)
}
#endif

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
template <bool ZeroAlpha>
struct DrawSomethingTemplated : pipeline<color_attachment<>> {
  DrawSomethingTemplated(hsh::dynamic_uniform_buffer<UniformData> u,
                         hsh::vertex_buffer<MyFormat> v,
                         hsh::texture2d<float> tex0) {
    position = u->xf * hsh::float4{v->position, 1.f};
    hsh::float3x3 normalXf = u->xf;
    hsh::float3 finalNormal = normalXf * v->normal;
    if (ZeroAlpha) {
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
template struct DrawSomethingTemplated<false>;
template struct DrawSomethingTemplated<true>;
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
                                        bool ZeroAlpha) {
  return hsh_DrawSomethingTemplated(DrawSomethingTemplated<ZeroAlpha>(u, v, tex0));
}
#endif

}
