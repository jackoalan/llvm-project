#pragma once

namespace hsh::detail {

template <unsigned NSTs> struct SelectTargetTraits {
  struct SurfaceBinding;
#define HSH_SURFACE_OWNER
#define HSH_TRAIT_BINDING SurfaceBinding
#define HSH_MULTI_TRAIT SurfaceOwner
#include "trait.def"

#define HSH_TRAIT_OWNER SurfaceOwner
#define HSH_TRAIT_DYNAMIC_OWNER void
#define HSH_MULTI_TRAIT SurfaceBinding
#include "trivial_trait.def"

  struct UniformBufferBinding;
#define HSH_TRAIT_BINDING UniformBufferBinding
#define HSH_MULTI_TRAIT UniformBufferOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING UniformBufferBinding
#define HSH_MULTI_TRAIT DynamicUniformBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER UniformBufferOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicUniformBufferOwner
#define HSH_MULTI_TRAIT UniformBufferBinding
#include "trivial_trait.def"

  struct VertexBufferBinding;
#define HSH_TRAIT_BINDING VertexBufferBinding
#define HSH_MULTI_TRAIT VertexBufferOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING VertexBufferBinding
#define HSH_MULTI_TRAIT DynamicVertexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER VertexBufferOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicVertexBufferOwner
#define HSH_MULTI_TRAIT VertexBufferBinding
#include "trivial_trait.def"

  struct IndexBufferBinding;
#define HSH_TRAIT_BINDING IndexBufferBinding
#define HSH_MULTI_TRAIT IndexBufferOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING IndexBufferBinding
#define HSH_MULTI_TRAIT DynamicIndexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER IndexBufferOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicIndexBufferOwner
#define HSH_MULTI_TRAIT IndexBufferBinding
#include "trivial_trait.def"

  struct TextureBinding;
#define HSH_TRAIT_BINDING TextureBinding
#define HSH_MULTI_TRAIT TextureOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING TextureBinding
#define HSH_MULTI_TRAIT DynamicTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER TextureOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicTextureOwner
#define HSH_MULTI_TRAIT TextureBinding
#include "trivial_trait.def"

  struct RenderTextureBinding;
#define HSH_RENDER_TEXTURE_OWNER
#define HSH_MULTI_TRAIT RenderTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER RenderTextureOwner
#define HSH_TRAIT_DYNAMIC_OWNER void
#define HSH_MULTI_TRAIT RenderTextureBinding
#include "trivial_trait.def"

#define HSH_PIPELINE_BINDING
#define HSH_MULTI_TRAIT PipelineBinding
#include "trait.def"

#define HSH_FIFO_OWNER
#define HSH_MULTI_TRAIT FifoOwner
#include "trait.def"

#define HSH_TRAIT_TEMPLATE_PARMS template <typename ResTp>
#define HSH_TRAIT_TEMPLATE_REFS <ResTp>
#define HSH_MULTI_TRAIT ResourceFactory
#include "trait.def"

  template <typename T, typename... Args>
  static typename decltype(T::Binding)::Owner
  CreateResource(const SourceLocation &location, Args &&... args) {
    switch (detail::CurrentTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Target::Enumeration:                                                    \
    return decltype(ResourceFactory<T>::_##Enumeration)::Create(               \
        location, std::forward<Args>(args)...);
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
    return {};
  }

  template <typename T, typename... Args>
  static typename decltype(T::Binding)::DynamicOwner
  CreateDynamicResource(const SourceLocation &location, Args &&... args) {
    switch (detail::CurrentTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Target::Enumeration:                                                    \
    return decltype(ResourceFactory<T>::_##Enumeration)::CreateDynamic(        \
        location, std::forward<Args>(args)...);
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
    return {};
  }

  template <typename T, typename... Args>
  static FifoOwner CreateFifo(const SourceLocation &location,
                              Args &&... args) noexcept {
    switch (detail::CurrentTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Target::Enumeration:                                                    \
    return decltype(ResourceFactory<T>::_##Enumeration)::Create(               \
        location, std::forward<Args>(args)...);
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
    return {};
  }

  static void ClearAttachments(bool color, bool depth) noexcept {
    switch (detail::CurrentTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Target::Enumeration:                                                    \
    TargetTraits<Target::Enumeration>::ClearAttachments(color, depth);         \
    break;
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
  }

  static void SetBlendConstants(float red, float green, float blue,
                                float alpha) noexcept {
    switch (detail::CurrentTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Target::Enumeration:                                                    \
    TargetTraits<Target::Enumeration>::SetBlendConstants(red, green, blue,     \
                                                         alpha);               \
    break;
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
  }

  static void SetViewport(const viewport &vp) noexcept {
    switch (detail::CurrentTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Target::Enumeration:                                                    \
    TargetTraits<Target::Enumeration>::SetViewport(vp);                        \
    break;
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
  }

  static void SetViewport(const viewport &vp, const scissor &s) noexcept {
    switch (detail::CurrentTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Target::Enumeration:                                                    \
    TargetTraits<Target::Enumeration>::SetViewport(vp, s);                     \
    break;
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
  }

  static void SetScissor(const scissor &s) noexcept {
    switch (detail::CurrentTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Target::Enumeration:                                                    \
    TargetTraits<Target::Enumeration>::SetScissor(s);                          \
    break;
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
  }
};

template <> struct SelectTargetTraits<1> {
  using SingleTargetTraits = TargetTraits<FirstStaticallyActiveTarget()>;
  struct SurfaceBinding;
#define HSH_SURFACE_OWNER
#define HSH_TRAIT_BINDING SurfaceBinding
#define HSH_SINGLE_TRAIT SurfaceOwner
#include "trait.def"

#define HSH_TRAIT_OWNER SurfaceOwner
#define HSH_TRAIT_DYNAMIC_OWNER void
#define HSH_SINGLE_TRAIT SurfaceBinding
#include "trivial_trait.def"

  struct UniformBufferBinding;
#define HSH_TRAIT_BINDING UniformBufferBinding
#define HSH_SINGLE_TRAIT UniformBufferOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING UniformBufferBinding
#define HSH_SINGLE_TRAIT DynamicUniformBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER UniformBufferOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicUniformBufferOwner
#define HSH_SINGLE_TRAIT UniformBufferBinding
#include "trivial_trait.def"

  struct VertexBufferBinding;
#define HSH_TRAIT_BINDING VertexBufferBinding
#define HSH_SINGLE_TRAIT VertexBufferOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING VertexBufferBinding
#define HSH_SINGLE_TRAIT DynamicVertexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER VertexBufferOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicVertexBufferOwner
#define HSH_SINGLE_TRAIT VertexBufferBinding
#include "trivial_trait.def"

  struct IndexBufferBinding;
#define HSH_TRAIT_BINDING IndexBufferBinding
#define HSH_SINGLE_TRAIT IndexBufferOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING IndexBufferBinding
#define HSH_SINGLE_TRAIT DynamicIndexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER IndexBufferOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicIndexBufferOwner
#define HSH_SINGLE_TRAIT IndexBufferBinding
#include "trivial_trait.def"

  struct TextureBinding;
#define HSH_TRAIT_BINDING TextureBinding
#define HSH_SINGLE_TRAIT TextureOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING TextureBinding
#define HSH_SINGLE_TRAIT DynamicTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER TextureOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicTextureOwner
#define HSH_SINGLE_TRAIT TextureBinding
#include "trivial_trait.def"

  struct RenderTextureBinding;
#define HSH_RENDER_TEXTURE_OWNER
#define HSH_SINGLE_TRAIT RenderTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER RenderTextureOwner
#define HSH_TRAIT_DYNAMIC_OWNER void
#define HSH_SINGLE_TRAIT RenderTextureBinding
#include "trivial_trait.def"

#define HSH_PIPELINE_BINDING
#define HSH_SINGLE_TRAIT PipelineBinding
#include "trait.def"

#define HSH_FIFO_OWNER
#define HSH_SINGLE_TRAIT FifoOwner
#include "trait.def"

#define HSH_TRAIT_TEMPLATE_PARMS template <typename ResTp>
#define HSH_TRAIT_TEMPLATE_REFS <ResTp>
#define HSH_SINGLE_TRAIT ResourceFactory
#include "trait.def"

  template <typename T, typename... Args>
  static typename decltype(T::Binding)::Owner
  CreateResource(const SourceLocation &location, Args &&... args) noexcept {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  return decltype(ResourceFactory<T>::_##Enumeration)::Create(                 \
      location, std::forward<Args>(args)...);
#include "targets.def"
  }

  template <typename T, typename... Args>
  static typename decltype(T::Binding)::DynamicOwner
  CreateDynamicResource(const SourceLocation &location,
                        Args &&... args) noexcept {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  return decltype(ResourceFactory<T>::_##Enumeration)::CreateDynamic(          \
      location, std::forward<Args>(args)...);
#include "targets.def"
  }

  template <typename T, typename... Args>
  static FifoOwner CreateFifo(const SourceLocation &location,
                              Args &&... args) noexcept {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  return decltype(ResourceFactory<T>::_##Enumeration)::Create(                 \
      location, std::forward<Args>(args)...);
#include "targets.def"
  }

  static void ClearAttachments(bool color, bool depth) noexcept {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  SingleTargetTraits::ClearAttachments(color, depth);
#include "targets.def"
  }

  static void SetBlendConstants(float red, float green, float blue,
                                float alpha) noexcept {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  SingleTargetTraits::SetBlendConstants(red, green, blue, alpha);
#include "targets.def"
  }

  static void SetViewport(const viewport &vp) noexcept {
#define HSH_ACTIVE_TARGET(Enumeration) SingleTargetTraits::SetViewport(vp);
#include "targets.def"
  }

  static void SetViewport(const viewport &vp, const scissor &s) noexcept {
#define HSH_ACTIVE_TARGET(Enumeration) SingleTargetTraits::SetViewport(vp, s);
#include "targets.def"
  }

  static void SetScissor(const scissor &s) noexcept {
#define HSH_ACTIVE_TARGET(Enumeration) SingleTargetTraits::SetScissor(s);
#include "targets.def"
  }
};

template <> struct SelectTargetTraits<0> {
  struct SurfaceBinding;
#define HSH_SURFACE_OWNER
#define HSH_TRAIT_BINDING SurfaceBinding
#define HSH_NULL_TRAIT SurfaceOwner
#include "trait.def"

#define HSH_TRAIT_OWNER SurfaceOwner
#define HSH_TRAIT_DYNAMIC_OWNER void
#define HSH_NULL_TRAIT SurfaceBinding
#include "trivial_trait.def"

  struct UniformBufferBinding;
#define HSH_TRAIT_BINDING UniformBufferBinding
#define HSH_NULL_TRAIT UniformBufferOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING UniformBufferBinding
#define HSH_NULL_TRAIT DynamicUniformBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER UniformBufferOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicUniformBufferOwner
#define HSH_NULL_TRAIT UniformBufferBinding
#include "trivial_trait.def"

  struct VertexBufferBinding;
#define HSH_TRAIT_BINDING VertexBufferBinding
#define HSH_NULL_TRAIT VertexBufferOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING VertexBufferBinding
#define HSH_NULL_TRAIT DynamicVertexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER VertexBufferOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicVertexBufferOwner
#define HSH_NULL_TRAIT VertexBufferBinding
#include "trivial_trait.def"

  struct IndexBufferBinding;
#define HSH_TRAIT_BINDING IndexBufferBinding
#define HSH_NULL_TRAIT IndexBufferOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING IndexBufferBinding
#define HSH_NULL_TRAIT DynamicIndexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER IndexBufferOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicIndexBufferOwner
#define HSH_NULL_TRAIT IndexBufferBinding
#include "trivial_trait.def"

  struct TextureBinding;
#define HSH_TRAIT_BINDING TextureBinding
#define HSH_NULL_TRAIT TextureOwner
#include "trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING TextureBinding
#define HSH_NULL_TRAIT DynamicTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER TextureOwner
#define HSH_TRAIT_DYNAMIC_OWNER DynamicTextureOwner
#define HSH_NULL_TRAIT TextureBinding
#include "trivial_trait.def"

  struct RenderTextureBinding;
#define HSH_RENDER_TEXTURE_OWNER
#define HSH_NULL_TRAIT RenderTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER RenderTextureOwner
#define HSH_TRAIT_DYNAMIC_OWNER void
#define HSH_NULL_TRAIT RenderTextureBinding
#include "trivial_trait.def"

#define HSH_PIPELINE_BINDING
#define HSH_NULL_TRAIT PipelineBinding
#include "trait.def"

#define HSH_FIFO_OWNER
#define HSH_NULL_TRAIT FifoOwner
#include "trait.def"

#define HSH_TRAIT_TEMPLATE_PARMS template <typename ResTp>
#define HSH_TRAIT_TEMPLATE_REFS <ResTp>
#define HSH_NULL_TRAIT ResourceFactory
#include "trait.def"

  template <typename T, typename... Args>
  static typename decltype(T::Binding)::Owner
  CreateResource(const SourceLocation &location, Args &&... args) {
    return {};
  }

  template <typename T, typename... Args>
  static typename decltype(T::Binding)::DynamicOwner
  CreateDynamicResource(const SourceLocation &location, Args &&... args) {
    return {};
  }

  template <typename T, typename... Args>
  static FifoOwner CreateFifo(const SourceLocation &location,
                              Args &&... args) noexcept {
    return {};
  }

  static void ClearAttachments(bool color, bool depth) noexcept {}

  static void SetBlendConstants(float red, float green, float blue,
                                float alpha) noexcept {}
};

using ActiveTargetTraits = SelectTargetTraits<NumStaticallyActiveTargets>;

} // namespace hsh::detail
