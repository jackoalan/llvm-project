#pragma once

#include <cassert>

#ifdef _MSC_VER
#pragma section(".hsh5", read)
#endif

namespace hsh {
using namespace std::literals;
enum class Target : std::uint8_t {
  NullTarget,
#define HSH_TARGET(Enumeration, Name) Enumeration,
#include "targets.def"
  MaxTarget
};
constexpr std::string_view TargetNames[] = {
    "null"sv,
#define HSH_TARGET(Enumeration, Name) Name##sv,
#include "targets.def"
};

enum class ActiveTarget : std::uint8_t {
#define HSH_ACTIVE_TARGET(Enumeration) Enumeration,
#include "targets.def"
  MaxActiveTarget
};
struct uniform_buffer_typeless;
struct vertex_buffer_typeless;
template <typename T> struct index_buffer;
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

template <typename T> constexpr T AlignUp(T val, T align) {
  return (val + align - 1) & ~(align - 1);
}
} // namespace hsh

#if !defined(NDEBUG) && defined(__GXX_RTTI)
#define HSH_ASSERT_CAST_ENABLED 1
#else
#define HSH_ASSERT_CAST_ENABLED 0
#endif

namespace hsh::detail {
#if HSH_ASSERT_CAST_ENABLED
class TypeInfo {
  const char *m_name = nullptr;
  std::size_t m_hash = 0;

public:
  TypeInfo() noexcept = default;
  TypeInfo(const std::type_info &ti) noexcept
      : m_name(ti.name()), m_hash(ti.hash_code()) {}
  bool operator==(const TypeInfo &other) const noexcept {
    return m_hash == other.m_hash;
  }
  bool operator!=(const TypeInfo &other) const noexcept {
    return m_hash != other.m_hash;
  }
  template <typename T> static TypeInfo MakeTypeInfo() noexcept {
    return TypeInfo(typeid(T));
  }
  const char *Name() const noexcept { return m_name; }
  std::size_t Hash() const noexcept { return m_hash; }
  template <typename T> void Assert() const noexcept {
    auto To = MakeTypeInfo<T>();
    if (*this != To) {
      std::fputs("Cannot cast ", stderr);
      std::fputs(Name(), stderr);
      std::fputs(" to ", stderr);
      std::fputs(To.Name(), stderr);
      std::fputs("\n", stderr);
      assert(false && "Cast failure");
    }
  }
};
#else
class TypeInfo {
public:
  template <typename T> static TypeInfo MakeTypeInfo() noexcept {
    return TypeInfo();
  }
  template <typename T> void Assert() const noexcept {}
};
#endif

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
constexpr uint32_t MaxRenderTextureBindings = HSH_MAX_RENDER_TEXTURE_BINDINGS;
constexpr uint32_t MaxDescriptorPoolSets = HSH_DESCRIPTOR_POOL_SIZE;

/* Max supported mip count (enough for 16K texture) */
constexpr uint32_t MaxMipCount = 14;

template <Target T> struct TargetTraits {
  struct UniformBufferOwner {};
  struct UniformBufferBinding {};
  struct DynamicUniformBufferOwner {};
  struct VertexBufferOwner {};
  struct VertexBufferBinding {};
  struct DynamicVertexBufferOwner {};
  struct IndexBufferOwner {};
  struct IndexBufferBinding {};
  struct DynamicIndexBufferOwner {};
  struct TextureOwner {};
  struct TextureBinding {};
  struct DynamicTextureOwner {};
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
