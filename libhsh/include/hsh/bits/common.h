#pragma once

namespace hsh {
enum class Target : std::uint8_t {
  NullTarget,
#define HSH_TARGET(Enumeration) Enumeration,
#include "targets.def"
  MaxTarget
};
enum class ActiveTarget : std::uint8_t {
#define HSH_ACTIVE_TARGET(Enumeration) Enumeration,
#include "targets.def"
  MaxActiveTarget
};
struct uniform_buffer_typeless;
struct dynamic_uniform_buffer_typeless;
struct vertex_buffer_typeless;
struct dynamic_vertex_buffer_typeless;
struct texture_typeless;
struct render_texture2d;

namespace detail {
constexpr unsigned NumStaticallyActiveTargets =
    unsigned(ActiveTarget::MaxActiveTarget);
constexpr enum Target FirstStaticallyActiveTarget() noexcept {
#define HSH_ACTIVE_TARGET(Enumeration) return Target::Enumeration;
#include "targets.def"
  return Target::NullTarget;
}
inline enum Target CurrentTarget = FirstStaticallyActiveTarget();

template <typename Ts> struct ValidateBuiltTargets {};
template <hsh::Target... Ts>
struct ValidateBuiltTargets<std::integer_sequence<hsh::Target, Ts...>> {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  static_assert(((Ts == Target::Enumeration) || ...),                          \
                "hshgen not ran for one or more active targets");
#include "targets.def"
};
} // namespace detail
} // namespace hsh

#if !defined(NDEBUG) && defined(__GXX_RTTI)
#define HSH_ASSERT_CAST_ENABLED 1
#define HSH_ASSERT_CAST(...) assert(__VA_ARGS__)
#else
#define HSH_ASSERT_CAST_ENABLED 0
#define HSH_ASSERT_CAST(...)
#endif

namespace hsh::detail {
#ifndef HSH_MAX_UNIFORMS
#error HSH_MAX_UNIFORMS definition is mandatory!
#endif
#ifndef HSH_MAX_IMAGES
#error HSH_MAX_IMAGES definition is mandatory!
#endif
#ifndef HSH_MAX_SAMPLERS
#error HSH_MAX_SAMPLERS definition is mandatory!
#endif
#ifndef HSH_MAX_VERTEX_BUFFERS
#error HSH_MAX_VERTEX_BUFFERS definition is mandatory!
#endif
#ifndef HSH_MAX_INDEX_BUFFERS
#error HSH_MAX_INDEX_BUFFERS definition is mandatory!
#endif
#ifndef HSH_MAX_RENDER_TEXTURE_BINDINGS
#error HSH_MAX_RENDER_TEXTURE_BINDINGS definition is mandatory!
#endif
#ifndef HSH_DESCRIPTOR_POOL_SIZE
#error HSH_DESCRIPTOR_POOL_SIZE definition is mandatory!
#endif
constexpr uint32_t MaxUniforms = HSH_MAX_UNIFORMS;
constexpr uint32_t MaxImages = HSH_MAX_IMAGES;
constexpr uint32_t MaxSamplers = HSH_MAX_SAMPLERS;
constexpr uint32_t MaxVertexBuffers = HSH_MAX_VERTEX_BUFFERS;
constexpr uint32_t MaxIndexBuffers = HSH_MAX_INDEX_BUFFERS;
constexpr uint32_t MaxRenderTextureBindings = HSH_MAX_RENDER_TEXTURE_BINDINGS;
constexpr uint32_t MaxDescriptorPoolSets = HSH_DESCRIPTOR_POOL_SIZE;

template <typename T> class ArrayProxy {
public:
  constexpr ArrayProxy(std::nullptr_t) noexcept : Data(nullptr), Length(0) {}

  ArrayProxy(const T &OneElt) noexcept : Data(&OneElt), Length(1) {}

  ArrayProxy(const T *data, size_t length) noexcept
      : Data(data), Length(length) {}

  ArrayProxy(const T *begin, const T *end) noexcept
      : Data(begin), Length(end - begin) {}

  template <typename A>
  ArrayProxy(const std::vector<T, A> &Vec) noexcept
      : Data(Vec.data()), Length(Vec.size()) {}

  template <size_t N>
  constexpr ArrayProxy(const std::array<T, N> &Arr) noexcept
      : Data(Arr.data()), Length(N) {}

  template <size_t N>
  constexpr ArrayProxy(const T (&Arr)[N]) noexcept : Data(Arr), Length(N) {}

  ArrayProxy(const std::initializer_list<T> &Vec) noexcept
      : Data(Vec.begin() == Vec.end() ? (T *)nullptr : Vec.begin()),
        Length(Vec.size()) {}

  const T *begin() const noexcept { return Data; }

  const T *end() const noexcept { return Data + Length; }

  const T &front() const noexcept {
    assert(Length && Data);
    return *Data;
  }

  const T &back() const noexcept {
    assert(Length && Data);
    return *(Data + Length - 1);
  }

  bool empty() const noexcept { return (Length == 0); }

  std::size_t size() const noexcept { return Length; }

  const T *data() const noexcept { return Data; }

private:
  const T *Data = nullptr;
  std::size_t Length = 0;
};

template <Target T> struct TargetTraits {
  struct UniformBufferOwner {};
  struct UniformBufferBinding {};
  struct DynamicUniformBufferOwner {};
  struct DynamicUniformBufferBinding {};
  struct VertexBufferOwner {};
  struct VertexBufferBinding {};
  struct DynamicVertexBufferOwner {};
  struct DynamicVertexBufferBinding {};
  struct TextureOwner {};
  struct TextureBinding {};
  struct RenderTextureOwner {};
  struct RenderTextureBinding {};
  struct SurfaceOwner {};
  struct SurfaceBinding {};
  struct PipelineBinding {};
  template <typename ResTp> struct ResourceFactory {};
};

template <hsh::Target T> struct SamplerObject;
struct SamplerBinding;
template <typename T> struct ClassWrapper {};
} // namespace hsh::detail
