#pragma once

#ifdef VMA_IMPLEMENTATION
#define HSH_VMA_IMPLEMENTATION
#endif

#include "vk_mem_alloc.h"

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
template <> struct ObjectDestroy<VmaAllocator, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> {
public:
  ObjectDestroy() = default;
  ObjectDestroy(
      Optional<const AllocationCallbacks> allocationCallbacks,
      VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT {}
protected:
  template <typename T> void destroy(T t) VULKAN_HPP_NOEXCEPT {
    vmaDestroyAllocator(t);
  }
};
template <typename Dispatch> class UniqueHandleTraits<VmaAllocator, Dispatch> { public: using deleter = ObjectDestroy<NoParent, Dispatch>; };
using UniqueVmaAllocator = UniqueHandle<VmaAllocator, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>;
template<typename Dispatch = VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
VULKAN_HPP_INLINE typename ResultValueType<UniqueHandle<VmaAllocator,Dispatch>>::type createVmaAllocatorUnique( const VmaAllocatorCreateInfo& pCreateInfo, Optional<const AllocationCallbacks> allocator = nullptr, Dispatch const &d = VULKAN_HPP_DEFAULT_DISPATCHER )
{
  VmaAllocator vmaAllocator;
  Result result = static_cast<Result>( ::vmaCreateAllocator(&pCreateInfo, &vmaAllocator) );

  ObjectDestroy<NoParent,Dispatch> deleter( allocator, d );
  return createResultValue<VmaAllocator,Dispatch>( result, vmaAllocator, VULKAN_HPP_NAMESPACE_STRING"::createVmaAllocatorUnique", deleter );
}
}
