#pragma once

#ifdef VMA_IMPLEMENTATION
#define HSH_VMA_IMPLEMENTATION
#endif

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#endif
#include "vk_mem_alloc.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

namespace VULKAN_HPP_NAMESPACE {
struct VmaPool {
  ::VmaPool Pool = VK_NULL_HANDLE;
  operator ::VmaPool() const VULKAN_HPP_NOEXCEPT { return Pool; }
  VmaPool & operator=( std::nullptr_t ) VULKAN_HPP_NOEXCEPT {
    Pool = VK_NULL_HANDLE;
    return *this;
  }
};

struct VmaAllocator {
  ::VmaAllocator Allocator = VK_NULL_HANDLE;
  operator ::VmaAllocator() const VULKAN_HPP_NOEXCEPT { return Allocator; }
  VmaAllocator & operator=( std::nullptr_t ) VULKAN_HPP_NOEXCEPT {
    Allocator = VK_NULL_HANDLE;
    return *this;
  }
  template<typename Dispatch>
  void destroy(Optional<const AllocationCallbacks> allocator, Dispatch const &d) VULKAN_HPP_NOEXCEPT {
    ::vmaDestroyAllocator(Allocator);
  }


  template<typename Dispatch = VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
  typename ResultValueType<UniqueHandle<VmaPool,Dispatch>>::type createVmaPoolUnique( const VmaPoolCreateInfo& pCreateInfo, Optional<const AllocationCallbacks> allocator = nullptr, Dispatch const &d = VULKAN_HPP_DEFAULT_DISPATCHER )
  {
    ::VmaPool vmaPool;
    Result result = static_cast<Result>( ::vmaCreatePool(Allocator, &pCreateInfo, &vmaPool) );
    VmaPool vmaPoolObj{vmaPool};

    ObjectDestroy<VmaAllocator,Dispatch> deleter( *this, allocator, d );
    return createResultValue<VmaPool,Dispatch>( result, vmaPoolObj, VULKAN_HPP_NAMESPACE_STRING"::createVmaPoolUnique", deleter );
  }

  template<typename Dispatch = VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
  void destroy( VmaPool pool, Optional<const AllocationCallbacks> allocator = nullptr, Dispatch const &d = VULKAN_HPP_DEFAULT_DISPATCHER ) const VULKAN_HPP_NOEXCEPT {
    ::vmaDestroyPool(Allocator, pool);
  }
};
template <typename Dispatch> class UniqueHandleTraits<VmaAllocator, Dispatch> { public: using deleter = ObjectDestroy<NoParent, Dispatch>; };
using UniqueVmaAllocator = UniqueHandle<VmaAllocator, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>;
template<typename Dispatch = VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
VULKAN_HPP_INLINE typename ResultValueType<UniqueHandle<VmaAllocator,Dispatch>>::type createVmaAllocatorUnique( const VmaAllocatorCreateInfo& pCreateInfo, Optional<const AllocationCallbacks> allocator = nullptr, Dispatch const &d = VULKAN_HPP_DEFAULT_DISPATCHER )
{
  ::VmaAllocator vmaAllocator;
  Result result = static_cast<Result>( ::vmaCreateAllocator(&pCreateInfo, &vmaAllocator) );
  VmaAllocator vmaAllocatorObj{vmaAllocator};

  ObjectDestroy<NoParent,Dispatch> deleter( allocator, d );
  return createResultValue<VmaAllocator,Dispatch>( result, vmaAllocatorObj, VULKAN_HPP_NAMESPACE_STRING"::createVmaAllocatorUnique", deleter );
}

template <typename Dispatch> class UniqueHandleTraits<VmaPool, Dispatch> { public: using deleter = ObjectDestroy<VmaAllocator, Dispatch>; };
using UniqueVmaPool = UniqueHandle<VmaPool, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>;
}
