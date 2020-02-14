#pragma once

namespace hsh::detail {

template <unsigned NSTs> struct SelectTargetTraits {
#define HSH_SURFACE_OWNER
#define HSH_TRAIT_BINDING SurfaceBinding
#define HSH_MULTI_TRAIT SurfaceOwner
#include "trait.def"

#define HSH_TRAIT_OWNER SurfaceOwner
#define HSH_MULTI_TRAIT SurfaceBinding
#include "trivial_trait.def"

#define HSH_TRAIT_BINDING UniformBufferBinding
#define HSH_MULTI_TRAIT UniformBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER UniformBufferOwner
#define HSH_MULTI_TRAIT UniformBufferBinding
#include "trivial_trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING DynamicUniformBufferBinding
#define HSH_MULTI_TRAIT DynamicUniformBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER DynamicUniformBufferOwner
#define HSH_MULTI_TRAIT DynamicUniformBufferBinding
#include "trivial_trait.def"

#define HSH_TRAIT_BINDING VertexBufferBinding
#define HSH_MULTI_TRAIT VertexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER VertexBufferOwner
#define HSH_MULTI_TRAIT VertexBufferBinding
#include "trivial_trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING DynamicVertexBufferBinding
#define HSH_MULTI_TRAIT DynamicVertexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER DynamicVertexBufferOwner
#define HSH_MULTI_TRAIT DynamicVertexBufferBinding
#include "trivial_trait.def"

#define HSH_TRAIT_BINDING TextureBinding
#define HSH_MULTI_TRAIT TextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER TextureOwner
#define HSH_MULTI_TRAIT TextureBinding
#include "trivial_trait.def"

#define HSH_RENDER_TEXTURE_OWNER
#define HSH_MULTI_TRAIT RenderTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER RenderTextureOwner
#define HSH_MULTI_TRAIT RenderTextureBinding
#include "trivial_trait.def"

#define HSH_PIPELINE_BINDING
#define HSH_MULTI_TRAIT PipelineBinding
#include "trait.def"

#define HSH_TRAIT_TEMPLATE_PARMS template <typename ResTp>
#define HSH_TRAIT_TEMPLATE_REFS <ResTp>
#define HSH_MULTI_TRAIT ResourceFactory
#include "trait.def"

  template <typename T, typename... Args>
  static typename decltype(T::Binding)::Owner
  CreateResource(const SourceLocation &location, Args... args) {
    switch (detail::CurrentTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Target::Enumeration:                                                    \
    return decltype(ResourceFactory<T>::_##Enumeration)::Create(location,      \
                                                                args...);
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
};
template <> struct SelectTargetTraits<1> {
  using TargetTraits = TargetTraits<FirstStaticallyActiveTarget()>;
#define HSH_SURFACE_OWNER
#define HSH_TRAIT_BINDING SurfaceBinding
#define HSH_SINGLE_TRAIT SurfaceOwner
#include "trait.def"

#define HSH_TRAIT_OWNER SurfaceOwner
#define HSH_SINGLE_TRAIT SurfaceBinding
#include "trivial_trait.def"

#define HSH_TRAIT_BINDING UniformBufferBinding
#define HSH_SINGLE_TRAIT UniformBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER UniformBufferOwner
#define HSH_SINGLE_TRAIT UniformBufferBinding
#include "trivial_trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING DynamicUniformBufferBinding
#define HSH_SINGLE_TRAIT DynamicUniformBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER DynamicUniformBufferOwner
#define HSH_SINGLE_TRAIT DynamicUniformBufferBinding
#include "trivial_trait.def"

#define HSH_TRAIT_BINDING VertexBufferBinding
#define HSH_SINGLE_TRAIT VertexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER VertexBufferOwner
#define HSH_SINGLE_TRAIT VertexBufferBinding
#include "trivial_trait.def"

#define HSH_DYNAMIC_OWNER
#define HSH_TRAIT_BINDING DynamicVertexBufferBinding
#define HSH_SINGLE_TRAIT DynamicVertexBufferOwner
#include "trait.def"

#define HSH_TRAIT_OWNER DynamicVertexBufferOwner
#define HSH_SINGLE_TRAIT DynamicVertexBufferBinding
#include "trivial_trait.def"

#define HSH_TRAIT_BINDING TextureBinding
#define HSH_SINGLE_TRAIT TextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER TextureOwner
#define HSH_SINGLE_TRAIT TextureBinding
#include "trivial_trait.def"

#define HSH_RENDER_TEXTURE_OWNER
#define HSH_SINGLE_TRAIT RenderTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER RenderTextureOwner
#define HSH_SINGLE_TRAIT RenderTextureBinding
#include "trivial_trait.def"

#define HSH_PIPELINE_BINDING
#define HSH_SINGLE_TRAIT PipelineBinding
#include "trait.def"

#define HSH_TRAIT_TEMPLATE_PARMS template <typename ResTp>
#define HSH_TRAIT_TEMPLATE_REFS <ResTp>
#define HSH_SINGLE_TRAIT ResourceFactory
#include "trait.def"

  template <typename T, typename... Args>
  static typename decltype(T::Binding)::Owner
  CreateResource(const SourceLocation &location, Args... args) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  return decltype(ResourceFactory<T>::_##Enumeration)::Create(location,        \
                                                              args...);
#include "targets.def"
  }

  static void ClearAttachments(bool color, bool depth) noexcept {
    TargetTraits::ClearAttachments(color, depth);
  }
};
using ActiveTargetTraits = SelectTargetTraits<NumStaticallyActiveTargets>;

} // namespace hsh::detail
