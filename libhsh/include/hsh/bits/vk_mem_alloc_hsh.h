#pragma once

#ifdef VMA_IMPLEMENTATION
#define HSH_VMA_IMPLEMENTATION
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#include "vk_mem_alloc.h"
#pragma GCC diagnostic pop

/**
@param[out] pBuffer Buffer that was created.
@param[out] pAllocation Allocation that was created.
@param[out] pAllocationInfo Optional. Information about allocated memory. It can be later fetched using function vmaGetAllocationInfo().
@param[out] secondOffset Optional. Offset to second portion of double buffer.

Specialized version of vmaCreateBuffer that allocates space for two non-coherently
aligned instances of pBufferCreateInfo->size. This may be used in conjunction with
a dynamic buffer descriptor to bind either buffer using only one descriptor set.

See vmaCreateBuffer for allocation mechanism details.
*/
VMA_CALL_PRE VkResult VMA_CALL_POST vmaCreateDoubleBuffer(
    VmaAllocator allocator,
    const VkBufferCreateInfo* pBufferCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkBuffer* pBuffer,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo,
    VkDeviceSize* secondOffset);

#ifdef HSH_VMA_IMPLEMENTATION
#undef HSH_VMA_IMPLEMENTATION
VMA_CALL_PRE VkResult VMA_CALL_POST vmaCreateDoubleBuffer(
    VmaAllocator allocator,
    const VkBufferCreateInfo* pBufferCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkBuffer* pBuffer,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo,
    VkDeviceSize* secondOffset)
{
    VMA_ASSERT(allocator && pBufferCreateInfo && pAllocationCreateInfo && pBuffer && pAllocation);

    if(pBufferCreateInfo->size == 0)
    {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkDeviceSize align = allocator->m_PhysicalDeviceProperties.limits.nonCoherentAtomSize;
    if((pBufferCreateInfo->usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) != 0)
    {
        align = VMA_MAX(align, allocator->m_PhysicalDeviceProperties.limits.minTexelBufferOffsetAlignment);
    }
    if((pBufferCreateInfo->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) != 0)
    {
        align = VMA_MAX(align, allocator->m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
    }
    if((pBufferCreateInfo->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0)
    {
        align = VMA_MAX(align, allocator->m_PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment);
    }
    VkDeviceSize offset2 = VmaAlignUp<VkDeviceSize>(pBufferCreateInfo->size, align);

    VkBufferCreateInfo doubleInfo = *pBufferCreateInfo;
    doubleInfo.size += offset2;

    if(secondOffset != VMA_NULL)
    {
        *secondOffset = offset2;
    }

    return vmaCreateBuffer(allocator, &doubleInfo, pAllocationCreateInfo, pBuffer, pAllocation, pAllocationInfo);
}
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
