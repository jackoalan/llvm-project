#include "test-input.h"
#include "test-input.cpp.hshhead"

namespace MyNS {

using namespace hsh::pipeline;

constexpr hsh::sampler TestSampler(hsh::Nearest, hsh::Nearest, hsh::Linear);

#if 1
struct DrawSomething : pipeline<color_attachment<>> {
  static constexpr hsh::float4 DoMultiply(hsh::float4x4 xf, hsh::float3 vec) {
    return xf * hsh::float4{vec, 1.f};
  }
  DrawSomething(hsh::uniform_buffer<UniformData> u,
                hsh::vertex_buffer<MyFormat> v, hsh::texture2d tex0) {
    position = DoMultiply(u->xf, v->position);
    // position = u->xf * hsh::float4{v->position, 1.f};
    // color_out[0] = tex0.sample<float>({0.f, 0.f}, TestSampler);
    color_out[0] = u->color * v->color;
  }
};
#endif

#if 1
template <bool Something, class AT>
struct DrawSomethingTemplated : pipeline<color_attachment<>> {
  DrawSomethingTemplated(hsh::uniform_buffer<UniformData> u,
                         hsh::vertex_buffer<MyFormat> v, hsh::texture2d tex0) {
    position = u->xf * hsh::float4{v->position, 1.f};
    hsh::float3x3 normalXf = u->xf;
    hsh::float3 finalNormal = normalXf * v->normal;
    if (AT::Mode == AM_NoAlpha) {
      color_out[0] = hsh::float4{
          hsh::float3{tex0.sample<float>({0.f, 0.f}, TestSampler).xyz() *
                      hsh::dot(finalNormal, -u->lightDir)},
          0.f};
    } else {
      color_out[0] = hsh::float4{
          hsh::float3{tex0.sample<float>({0.f, 0.f}, TestSampler).xyz() *
                      hsh::dot(finalNormal, -u->lightDir)},
          1.f};
    }
  }
};
template struct DrawSomethingTemplated<false, AlphaTraits<MyNS::AlphaMode(0)>>;
template struct DrawSomethingTemplated<false, AlphaTraits<MyNS::AlphaMode(1)>>;
template struct DrawSomethingTemplated<true, AlphaTraits<MyNS::AlphaMode(0)>>;
#endif

#if 1
void BindDrawSomething(hsh::binding &b, hsh::uniform_buffer_typeless u,
                       hsh::vertex_buffer_typeless v, hsh::texture2d tex0) {
  b.hsh_DrawSomething(DrawSomething(u, v, tex0));
}
#endif

#if 1
void BindDrawSomethingTemplated(hsh::binding &b, hsh::uniform_buffer_typeless u,
                                hsh::vertex_buffer_typeless v,
                                hsh::texture2d tex0, bool Something,
                                AlphaMode AMode) {
  b.hsh_DrawSomethingTemplated(
      DrawSomethingTemplated<Something, AlphaTraits<AMode>>(u, v, tex0));
}
#endif

hsh::binding &BindPipeline(Binding &b, hsh::uniform_buffer_typeless u,
                           hsh::vertex_buffer_typeless v) {
  if (!b.Tex) {
    b.Tex = hsh::create_texture2d(
        {1024, 1024}, hsh::Format::RGBA8_UNORM, 10,
        [](void *buf, std::size_t size) { std::memset(buf, 0, size); });
  }
  BindDrawSomething(b.Binding, u, v, b.Tex.get());
  return b.Binding;
}

hsh::binding &BindPipelineTemplated(Binding &b, hsh::uniform_buffer_typeless u,
                                    hsh::vertex_buffer_typeless v,
                                    bool Something, MyNS::AlphaMode AM) {
  if (!b.Tex) {
    b.Tex = hsh::create_texture2d(
        {1024, 1024}, hsh::Format::RGBA8_UNORM, 10,
        [](void *buf, std::size_t size) { std::memset(buf, 0, size); });
  }
  BindDrawSomethingTemplated(b.Binding, u, v, b.Tex.get(), Something, AM);
  return b.Binding;
}

} // namespace MyNS
