#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <type_traits>
#include <utility>

#include "bits/builtin_types.h"
#include "bits/common.h"
#include "bits/source_location.h"
#include "bits/vulkan.h"

namespace hsh {
struct offset2d {
  int32_t x, y;
  constexpr offset2d(int32_t x, int32_t y) noexcept : x(x), y(y) {}
#if HSH_ENABLE_VULKAN
  constexpr offset2d(vk::Offset2D off) noexcept : x(off.x), y(off.y) {}
  operator vk::Offset2D() const noexcept { return vk::Offset2D(x, y); }
#endif
};
struct extent2d {
  uint32_t w, h;
  constexpr extent2d(uint32_t w, uint32_t h) noexcept : w(w), h(h) {}
#if HSH_ENABLE_VULKAN
  constexpr extent2d(vk::Extent2D ext) noexcept : w(ext.width), h(ext.height) {}
  operator vk::Extent2D() const noexcept { return vk::Extent2D(w, h); }
#endif
};
struct rect2d {
  offset2d offset;
  extent2d extent;
  constexpr rect2d(offset2d offset, extent2d extent) noexcept
      : offset(offset), extent(extent) {}
#if HSH_ENABLE_VULKAN
  constexpr rect2d(vk::Rect2D rect) noexcept
      : offset(rect.offset), extent(rect.extent) {}
  operator vk::Rect2D() const noexcept { return vk::Rect2D(offset, extent); }
#endif
};
} // namespace hsh

#include "bits/vulkan_impl.h"

#include "bits/select_target_traits.h"

#include "bits/common_resources.h"
#include "bits/vulkan_resources.h"

namespace hsh {

template <typename T> struct resource_owner_base {
  typename decltype(T::Binding)::Owner Owner;

  resource_owner_base() noexcept = default;
  explicit resource_owner_base(decltype(Owner) Owner) noexcept
      : Owner(std::move(Owner)) {}

  resource_owner_base(const resource_owner_base &other) = delete;
  resource_owner_base &operator=(const resource_owner_base &other) = delete;
  resource_owner_base(resource_owner_base &&other) noexcept = default;
  resource_owner_base &
  operator=(resource_owner_base &&other) noexcept = default;

  operator bool() const noexcept { return Owner.IsValid(); }

  T get() const noexcept { return T(typename decltype(Owner)::Binding(Owner)); }
  operator T() const noexcept { return get(); }
};

template <typename T>
struct dynamic_resource_owner_base : resource_owner_base<T> {
  using resource_owner_base<T>::resource_owner_base;
  using MappedType = typename T::MappedType;
  MappedType *map() noexcept {
    return reinterpret_cast<MappedType *>(resource_owner_base<T>::Owner.Map());
  }
  void unmap() noexcept { resource_owner_base<T>::Owner.Unmap(); }
  void load(const MappedType &obj) noexcept {
    auto *ptr = map();
    std::memcpy(ptr, &obj, sizeof(MappedType));
    unmap();
  }
};

template <typename T> struct resource_owner : resource_owner_base<T> {
  using resource_owner_base<T>::resource_owner_base;
};

template <typename T>
struct resource_owner<dynamic_uniform_buffer<T>>
    : dynamic_resource_owner_base<dynamic_uniform_buffer<T>> {
  using dynamic_resource_owner_base<
      dynamic_uniform_buffer<T>>::dynamic_resource_owner_base;
};

template <typename T>
struct resource_owner<dynamic_vertex_buffer<T>>
    : dynamic_resource_owner_base<dynamic_vertex_buffer<T>> {
  using resource_owner_base<dynamic_vertex_buffer<T>>::resource_owner_base;
  using MappedType = typename dynamic_vertex_buffer<T>::MappedType;
  void load(detail::ArrayProxy<MappedType> obj) noexcept {
    auto *ptr = dynamic_resource_owner_base<dynamic_vertex_buffer<T>>::map();
    std::memcpy(ptr, obj.data(), sizeof(MappedType) * obj.size());
    dynamic_resource_owner_base<dynamic_vertex_buffer<T>>::unmap();
  }
  template <std::size_t N>
  void load(const std::array<MappedType, N> &Arr) noexcept {
    load(detail::ArrayProxy<T>(Arr));
  }
};

template <typename T, typename... Args>
inline resource_owner<T> create_resource(const SourceLocation &location,
                                         Args &&... args) noexcept {
  return resource_owner<T>(detail::ActiveTargetTraits::CreateResource<T>(
      location, std::forward<Args>(args)...));
}

template <typename T, typename... Args>
inline resource_owner<T> create_resource(Args &&... args) noexcept {
  return resource_owner<T>(detail::ActiveTargetTraits::CreateResource<T>(
      SourceLocation::current(), std::forward<Args>(args)...));
}

template <typename T, typename CopyFunc>
inline resource_owner<uniform_buffer<T>> create_uniform_buffer(
    CopyFunc copyFunc,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_resource<uniform_buffer<T>>(location, copyFunc);
}

template <typename T>
inline resource_owner<uniform_buffer<T>> create_uniform_buffer(
    const T &data,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_resource<uniform_buffer<T>>(
      location,
      [&](void *buf, std::size_t size) { std::memcpy(buf, &data, sizeof(T)); });
}

template <typename T>
inline resource_owner<dynamic_uniform_buffer<T>> create_dynamic_uniform_buffer(
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_resource<dynamic_uniform_buffer<T>>(location);
}

template <typename T>
inline resource_owner<dynamic_uniform_buffer<T>> create_dynamic_uniform_buffer(
    const T &data,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  auto ret = create_resource<dynamic_uniform_buffer<T>>(location);
  ret.load(data);
  return ret;
}

template <typename T>
inline resource_owner<vertex_buffer<T>> create_vertex_buffer(
    detail::ArrayProxy<T> data,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_resource<vertex_buffer<T>>(
      location, data.size(), [&](void *buf, std::size_t size) {
        std::memcpy(buf, data.data(), sizeof(T) * data.size());
      });
}

template <typename T, std::size_t N>
inline resource_owner<vertex_buffer<T>> create_vertex_buffer(
    const std::array<T, N> &Arr,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_vertex_buffer(detail::ArrayProxy<T>(Arr), location);
}

template <typename T>
inline resource_owner<dynamic_vertex_buffer<T>> create_dynamic_vertex_buffer(
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_resource<dynamic_vertex_buffer<T>>(location);
}

template <typename T>
inline resource_owner<dynamic_vertex_buffer<T>> create_dynamic_vertex_buffer(
    detail::ArrayProxy<T> data,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  auto ret = create_resource<dynamic_vertex_buffer<T>>(location);
  ret.load(data);
  return ret;
}

template <typename T, std::size_t N>
inline resource_owner<dynamic_vertex_buffer<T>> create_dynamic_vertex_buffer(
    const std::array<T, N> &Arr,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  auto ret = create_resource<dynamic_vertex_buffer<T>>(location);
  ret.load(Arr);
  return ret;
}

template <typename TexelType, typename CopyFunc>
inline resource_owner<texture2d<TexelType>> create_texture2d(
    extent2d extent, Format format, uint32_t numMips, CopyFunc copyFunc,
    ColorSwizzle redSwizzle = CS_Identity,
    ColorSwizzle greenSwizzle = CS_Identity,
    ColorSwizzle blueSwizzle = CS_Identity,
    ColorSwizzle alphaSwizzle = CS_Identity,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_resource<texture2d<TexelType>>(
      location, extent, format, numMips, copyFunc, redSwizzle, greenSwizzle,
      blueSwizzle, alphaSwizzle);
}

template <> struct resource_owner<render_texture2d> {
  typename decltype(render_texture2d::Binding)::Owner Owner;

  resource_owner() noexcept = default;
  explicit resource_owner(decltype(Owner) Owner) noexcept
      : Owner(std::move(Owner)) {}

  resource_owner(const resource_owner &other) = delete;
  resource_owner &operator=(const resource_owner &other) = delete;
  resource_owner(resource_owner &&other) noexcept = default;
  resource_owner &operator=(resource_owner &&other) noexcept = default;

  render_texture2d get_color(uint32_t idx) const noexcept {
    return {Owner.GetColor(idx)};
  }
  render_texture2d get_depth(uint32_t idx) const noexcept {
    return {Owner.GetDepth(idx)};
  }
  void attach() noexcept { Owner.Attach(); }
  void resolve_surface(surface surface, bool reattach = false) noexcept {
    Owner.ResolveSurface(surface.Binding, reattach);
  }
  void resolve_color_binding(uint32_t idx, rect2d region,
                             bool reattach = true) noexcept {
    Owner.ResolveColorBinding(idx, region, reattach);
  }
  void resolve_depth_binding(uint32_t idx, rect2d region,
                             bool reattach = true) noexcept {
    Owner.ResolveDepthBinding(idx, region, reattach);
  }
};

inline resource_owner<render_texture2d> create_render_texture2d(
    surface Surf, uint32_t NumColorBindings = 0, uint32_t NumDepthBindings = 0,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_resource<render_texture2d>(location, Surf, NumColorBindings,
                                           NumDepthBindings);
}

template <> struct resource_owner<surface> : resource_owner_base<surface> {
  using resource_owner_base<surface>::resource_owner_base;
  bool acquire_next_image() noexcept {
    return resource_owner_base<surface>::Owner.AcquireNextImage();
  }
};

#if HSH_ENABLE_VULKAN
inline resource_owner<surface> create_surface(
    vk::UniqueSurfaceKHR &&Surface,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_resource<surface>(location, std::move(Surface));
}
#endif

class binding_typeless {
protected:
  detail::ActiveTargetTraits::PipelineBinding Data;
  template <typename... Args>
  explicit binding_typeless(Args... args) noexcept : Data(args...) {}

public:
  operator bool() const noexcept { return Data.IsValid(); }
  binding_typeless() noexcept = default;
  void draw(uint32_t start, uint32_t count) noexcept {
    Data.Draw(start, count);
  }
};

template <typename Impl> class binding : public binding_typeless {
protected:
  template <typename... Args>
  explicit binding(Args... args) noexcept
      : binding_typeless(detail::ClassWrapper<Impl>(), args...) {}
};

inline void clear_attachments(bool color = true, bool depth = true) noexcept {
  detail::ActiveTargetTraits::ClearAttachments(color, depth);
}

#if __hsh__
#define HSH_VAR_STAGE(stage) [[hsh::stage]]
#else
#define HSH_VAR_STAGE(stage)
#endif

} // namespace hsh

#include "bits/profile.h"
