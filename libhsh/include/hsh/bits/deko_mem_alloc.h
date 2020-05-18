//
// Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved.
// Deko modifications Copyright (c) 2020 Jack Andersen.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef AMD_DEKO_MEMORY_ALLOCATOR_H
#define AMD_DEKO_MEMORY_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
/// Deko3D does not provide device introspection like Vulkan. Certain aspects
/// are hard-coded here instead. Feel free to adjust them to the needs of your
/// application.
///////////////////////////////////////////////////////////////////////////////

typedef enum DmaMemoryTypes {
  DmaMT_Upload = 0,
  DmaMT_Generic,
  DmaMT_GenericCpu,
  DmaMT_Image,
  DmaMT_Code,
  DmaMT_Max
} DmaMemoryTypes;

#define DMA_HEAP_SIZE (1024ull * 1024ull * 1024ull * 2ull)

#ifdef DMA_IMPLEMENTATION

constexpr uint32_t DmaMemTypeMinAlignment[] = {
    4, 4, 4, DK_IMAGE_LINEAR_STRIDE_ALIGNMENT, DK_SHADER_CODE_ALIGNMENT};

constexpr uint32_t DmaMemTypeFlags[] = {
    DkMemBlockFlags_CpuCached | DkMemBlockFlags_GpuUncached,
    DkMemBlockFlags_GpuCached,
    DkMemBlockFlags_CpuCached | DkMemBlockFlags_GpuCached,
    DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image,
    DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code};

constexpr bool DmaMemTypeMappable[] = {true, false, true, false, false};

constexpr uint32_t DmaMemTypePreferredBlockSize[] = {
    (1024ull * 1024ull * 64ull), (1024ull * 1024ull * 64ull),
    (1024ull * 1024ull * 64ull), (1024ull * 1024ull * 256ull),
    (1024ull * 1024ull * 64ull)};

#endif

///////////////////////////////////////////////////////////////////////////////

#define DMA_NUM_MEMORY_TYPES DmaMT_Max
#define DMA_NUM_MEMORY_HEAPS 1

#define DMA_DEFINE_HANDLE(object) typedef struct object##_T *object;
#define DMA_NULL_HANDLE nullptr
#define DMA_WHOLE_SIZE (UINT32_MAX)

typedef void *(*PFN_dmaAllocationFunction)(void *pUserData, size_t size,
                                           size_t alignment);

typedef void (*PFN_dmaFreeFunction)(void *pUserData, void *pMemory);

typedef struct DmaAllocationCallbacks {
  void *pUserData;
  PFN_dmaAllocationFunction pfnAllocation;
  PFN_dmaFreeFunction pfnFree;
} DmaAllocationCallbacks;

typedef struct DmaMemoryAllocateInfo {
  uint32_t allocationSize;
  uint32_t memoryTypeIndex;
  void *storage;
} DmaMemoryAllocateInfo;

typedef struct DmaMemoryRequirements {
  uint32_t size;
  uint32_t alignment;
  uint32_t memoryTypeIndex;
} DmaMemoryRequirements;

/** \mainpage Vulkan Memory Allocator

<b>Version 2.3.0</b> (2019-12-04)

Copyright (c) 2017-2019 Advanced Micro Devices, Inc. All rights reserved. \n
License: MIT

Documentation of all members: vk_mem_alloc.h

\section main_table_of_contents Table of contents

- <b>User guide</b>
  - \subpage quick_start
    - [Project setup](@ref quick_start_project_setup)
    - [Initialization](@ref quick_start_initialization)
    - [Resource allocation](@ref quick_start_resource_allocation)
  - \subpage choosing_memory_type
    - [Usage](@ref choosing_memory_type_usage)
    - [Required and preferred flags](@ref
choosing_memory_type_required_preferred_flags)
    - [Explicit memory types](@ref choosing_memory_type_explicit_memory_types)
    - [Custom memory pools](@ref choosing_memory_type_custom_memory_pools)
    - [Dedicated allocations](@ref choosing_memory_type_dedicated_allocations)
  - \subpage memory_mapping
    - [Mapping functions](@ref memory_mapping_mapping_functions)
    - [Persistently mapped memory](@ref
memory_mapping_persistently_mapped_memory)
    - [Cache flush and invalidate](@ref memory_mapping_cache_control)
    - [Finding out if memory is mappable](@ref
memory_mapping_finding_if_memory_mappable)
  - \subpage staying_within_budget
    - [Querying for budget](@ref staying_within_budget_querying_for_budget)
    - [Controlling memory usage](@ref
staying_within_budget_controlling_memory_usage)
  - \subpage custom_memory_pools
    - [Choosing memory type index](@ref custom_memory_pools_MemTypeIndex)
    - [Linear allocation algorithm](@ref linear_algorithm)
      - [Free-at-once](@ref linear_algorithm_free_at_once)
      - [Stack](@ref linear_algorithm_stack)
      - [Double stack](@ref linear_algorithm_double_stack)
      - [Ring buffer](@ref linear_algorithm_ring_buffer)
    - [Buddy allocation algorithm](@ref buddy_algorithm)
  - \subpage defragmentation
        - [Defragmenting CPU memory](@ref defragmentation_cpu)
        - [Defragmenting GPU memory](@ref defragmentation_gpu)
        - [Additional notes](@ref defragmentation_additional_notes)
        - [Writing custom allocation algorithm](@ref
defragmentation_custom_algorithm)
  - \subpage lost_allocations
  - \subpage statistics
    - [Numeric statistics](@ref statistics_numeric_statistics)
    - [JSON dump](@ref statistics_json_dump)
  - \subpage allocation_annotation
    - [Allocation user data](@ref allocation_user_data)
    - [Allocation names](@ref allocation_names)
  - \subpage debugging_memory_usage
    - [Memory initialization](@ref debugging_memory_usage_initialization)
    - [Margins](@ref debugging_memory_usage_margins)
    - [Corruption detection](@ref debugging_memory_usage_corruption_detection)
  - \subpage record_and_replay
- \subpage usage_patterns
  - [Common mistakes](@ref usage_patterns_common_mistakes)
  - [Simple patterns](@ref usage_patterns_simple)
  - [Advanced patterns](@ref usage_patterns_advanced)
- \subpage configuration
  - [Pointers to Vulkan functions](@ref config_Vulkan_functions)
  - [Custom host memory allocator](@ref custom_memory_allocator)
  - [Device memory allocation callbacks](@ref allocation_callbacks)
  - [Device heap memory limit](@ref heap_memory_limit)
  - \subpage vk_khr_dedicated_allocation
- \subpage general_considerations
  - [Thread safety](@ref general_considerations_thread_safety)
  - [Validation layer warnings](@ref
general_considerations_validation_layer_warnings)
  - [Allocation algorithm](@ref general_considerations_allocation_algorithm)
  - [Features not supported](@ref general_considerations_features_not_supported)

\section main_see_also See also

- [Product page on
GPUOpen](https://gpuopen.com/gaming-product/vulkan-memory-allocator/)
- [Source repository on
GitHub](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)




\page quick_start Quick start

\section quick_start_project_setup Project setup

Vulkan Memory Allocator comes in form of a "stb-style" single header file.
You don't need to build it as a separate library project.
You can add this file directly to your project and submit it to code repository
next to your other source files.

"Single header" doesn't mean that everything is contained in C/C++ declarations,
like it tends to be in case of inline functions or C++ templates.
It means that implementation is bundled with interface in a single file and
needs to be extracted using preprocessor macro. If you don't do it properly, you
will get linker errors.

To do it properly:

-# Include "vk_mem_alloc.h" file in each CPP file where you want to use the
library. This includes declarations of all members of the library.
-# In exacly one CPP file define following macro before this include.
   It enables also internal definitions.

\code
#define DMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
\endcode

It may be a good idea to create dedicated CPP file just for this purpose.

Note on language: This library is written in C++, but has C-compatible
interface. Thus you can include and use vk_mem_alloc.h in C or C++ code, but
full implementation with `DMA_IMPLEMENTATION` macro must be compiled as C++, NOT
as C.

Please note that this library includes header `<vulkan/vulkan.h>`, which in turn
includes `<windows.h>` on Windows. If you need some specific macros defined
before including these headers (like `WIN32_LEAN_AND_MEAN` or
`WINVER` for Windows, `VK_USE_PLATFORM_WIN32_KHR` for Vulkan), you must define
them before every `#include` of this library.


\section quick_start_initialization Initialization

At program startup:

-# Initialize Vulkan to have `VkPhysicalDevice` and `DkDevice` object.
-# Fill DmaAllocatorCreateInfo structure and create #DmaAllocator object by
   calling dmaCreateAllocator().

\code
DmaAllocatorCreateInfo allocatorInfo = {};
allocatorInfo.physicalDevice = physicalDevice;
allocatorInfo.device = device;

DmaAllocator allocator;
dmaCreateAllocator(&allocatorInfo, &allocator);
\endcode

\section quick_start_resource_allocation Resource allocation

When you want to create a buffer or image:

-# Fill `VkBufferCreateInfo` / `VkImageCreateInfo` structure.
-# Fill DmaAllocationCreateInfo structure.
-# Call dmaCreateBuffer() / dmaCreateImage() to get `VkBuffer`/`VkImage` with
memory already allocated and bound to it.

\code
VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
bufferInfo.size = 65536;
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
VK_BUFFER_USAGE_TRANSFER_DST_BIT;

DmaAllocationCreateInfo allocInfo = {};
allocInfo.usage = DMA_MEMORY_USAGE_GPU_ONLY;

VkBuffer buffer;
DmaAllocation allocation;
dmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation,
nullptr); \endcode

Don't forget to destroy your objects when no longer needed:

\code
dmaDestroyBuffer(allocator, buffer, allocation);
dmaDestroyAllocator(allocator);
\endcode


\page choosing_memory_type Choosing memory type

Physical devices in Vulkan support various combinations of memory heaps and
types. Help with choosing correct and optimal memory type for your specific
resource is one of the key features of this library. You can use it by filling
appropriate members of DmaAllocationCreateInfo structure, as described below.
You can also combine multiple methods.

-# If you just want to find memory type index that meets your requirements, you
   can use function: dmaFindMemoryTypeIndex(),
dmaFindMemoryTypeIndexForBufferInfo(), dmaFindMemoryTypeIndexForImageInfo().
-# If you want to allocate a region of device memory without association with
any specific image or buffer, you can use function dmaAllocateMemory(). Usage of
   this function is not recommended and usually not needed.
   dmaAllocateMemoryPages() function is also provided for creating multiple
allocations at once, which may be useful for sparse binding.
-# If you already have a buffer or an image created, you want to allocate memory
   for it and then you will bind it yourself, you can use function
   dmaAllocateMemoryForBuffer(), dmaAllocateMemoryForImage().
   For binding you should use functions: dmaBindBufferMemory(),
dmaBindImageMemory() or their extended versions: dmaBindBufferMemory2(),
dmaBindImageMemory2().
-# If you want to create a buffer or an image, allocate memory for it and bind
   them together, all in one call, you can use function dmaCreateBuffer(),
   dmaCreateImage(). This is the easiest and recommended way to use this
library.

When using 3. or 4., the library internally queries Vulkan for memory types
supported for that buffer or image (function `vkGetBufferMemoryRequirements()`)
and uses only one of these types.

If no memory type can be found that meets all the requirements, these functions
return `DkResult_NotImplemented`.

You can leave DmaAllocationCreateInfo structure completely filled with zeros.
It means no requirements are specified for memory type.
It is valid, although not very useful.

\section choosing_memory_type_usage Usage

The easiest way to specify memory requirements is to fill member
DmaAllocationCreateInfo::usage using one of the values of enum #DmaMemoryUsage.
It defines high level, common usage types.
For more details, see description of this enum.

For example, if you want to create a uniform buffer that will be filled using
transfer only once or infrequently and used for rendering every frame, you can
do it using following code:

\code
VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
bufferInfo.size = 65536;
bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
VK_BUFFER_USAGE_TRANSFER_DST_BIT;

DmaAllocationCreateInfo allocInfo = {};
allocInfo.usage = DMA_MEMORY_USAGE_GPU_ONLY;

VkBuffer buffer;
DmaAllocation allocation;
dmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation,
nullptr); \endcode

\section choosing_memory_type_required_preferred_flags Required and preferred
flags

You can specify more detailed requirements by filling members
DmaAllocationCreateInfo::requiredFlags and
DmaAllocationCreateInfo::preferredFlags with a combination of bits from enum
`VkMemoryPropertyFlags`. For example, if you want to create a buffer that will
be persistently mapped on host (so it must be `HOST_VISIBLE`) and preferably
will also be `HOST_COHERENT` and `HOST_CACHED`, use following code:

\code
DmaAllocationCreateInfo allocInfo = {};
allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
VK_MEMORY_PROPERTY_HOST_CACHED_BIT; allocInfo.flags =
DMA_ALLOCATION_CREATE_MAPPED_BIT;

VkBuffer buffer;
DmaAllocation allocation;
dmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation,
nullptr); \endcode

A memory type is chosen that has all the required flags and as many preferred
flags set as possible.

If you use DmaAllocationCreateInfo::usage, it is just internally converted to
a set of required and preferred flags.

\section choosing_memory_type_explicit_memory_types Explicit memory types

If you inspected memory types available on the physical device and you have
a preference for memory types that you want to use, you can fill member
DmaAllocationCreateInfo::memoryTypeBits. It is a bit mask, where each bit set
means that a memory type with that index is allowed to be used for the
allocation. Special value 0, just like `UINT32_MAX`, means there are no
restrictions to memory type index.

Please note that this member is NOT just a memory type index.
Still you can use it to choose just one, specific memory type.
For example, if you already determined that your buffer should be created in
memory type 2, use following code:

\code
uint32_t memoryTypeIndex = 2;

DmaAllocationCreateInfo allocInfo = {};
allocInfo.memoryTypeBits = 1u << memoryTypeIndex;

VkBuffer buffer;
DmaAllocation allocation;
dmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation,
nullptr); \endcode

\section choosing_memory_type_custom_memory_pools Custom memory pools

If you allocate from custom memory pool, all the ways of specifying memory
requirements described above are not applicable and the aforementioned members
of DmaAllocationCreateInfo structure are ignored. Memory type is selected
explicitly when creating the pool and then used to make all the allocations from
that pool. For further details, see \ref custom_memory_pools.

\section choosing_memory_type_dedicated_allocations Dedicated allocations

Memory for allocations is reserved out of larger block of `DkMemBlock`
allocated from Vulkan internally. That's the main feature of this whole library.
You can still request a separate memory block to be created for an allocation,
just like you would do in a trivial solution without using any allocator.
In that case, a buffer or image is always bound to that memory at offset 0.
This is called a "dedicated allocation".
You can explicitly request it by using flag
#DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT. The library can also internally
decide to use dedicated allocation in some cases, e.g.:

- When the size of the allocation is large.
- When [VK_KHR_dedicated_allocation](@ref vk_khr_dedicated_allocation) extension
is enabled and it reports that dedicated allocation is required or recommended
for the resource.
- When allocation of next big memory block fails due to not enough device
memory, but allocation with the exact requested size succeeds.


\page memory_mapping Memory mapping

To "map memory" in Vulkan means to obtain a CPU pointer to `DkMemBlock`,
to be able to read from it or write to it in CPU code.
Mapping is possible only of memory allocated from a memory type that has
`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` flag.
Functions `vkMapMemory()`, `vkUnmapMemory()` are designed for this purpose.
You can use them directly with memory allocated by this library,
but it is not recommended because of following issue:
Mapping the same `DkMemBlock` block multiple times is illegal - only one
mapping at a time is allowed. This includes mapping disjoint regions. Mapping is
not reference-counted internally by Vulkan. Because of this, Vulkan Memory
Allocator provides following facilities:

\section memory_mapping_mapping_functions Mapping functions

The library provides following functions for mapping of a specific
#DmaAllocation: dmaMapMemory(), dmaUnmapMemory(). They are safer and more
convenient to use than standard Vulkan functions. You can map an allocation
multiple times simultaneously - mapping is reference-counted internally. You can
also map different allocations simultaneously regardless of whether they use the
same `DkMemBlock` block. The way it's implemented is that the library always
maps entire memory block, not just region of the allocation. For further
details, see description of dmaMapMemory() function. Example:

\code
// Having these objects initialized:

struct ConstantBuffer
{
    ...
};
ConstantBuffer constantBufferData;

DmaAllocator allocator;
VkBuffer constantBuffer;
DmaAllocation constantBufferAllocation;

// You can map and fill your buffer using following code:

void* mappedData;
dmaMapMemory(allocator, constantBufferAllocation, &mappedData);
memcpy(mappedData, &constantBufferData, sizeof(constantBufferData));
dmaUnmapMemory(allocator, constantBufferAllocation);
\endcode

When mapping, you may see a warning from Vulkan validation layer similar to this
one:

<i>Mapping an image with layout VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
can result in undefined behavior if this memory is used by the device. Only
GENERAL or PREINITIALIZED should be used.</i>

It happens because the library maps entire `DkMemBlock` block, where
different types of images and buffers may end up together, especially on GPUs
with unified memory like Intel. You can safely ignore it if you are sure you
access only memory of the intended object that you wanted to map.


\section memory_mapping_persistently_mapped_memory Persistently mapped memory

Kepping your memory persistently mapped is generally OK in Vulkan.
You don't need to unmap it before using its data on the GPU.
The library provides a special feature designed for that:
Allocations made with #DMA_ALLOCATION_CREATE_MAPPED_BIT flag set in
DmaAllocationCreateInfo::flags stay mapped all the time,
so you can just access CPU pointer to it any time
without a need to call any "map" or "unmap" function.
Example:

\code
VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
bufCreateInfo.size = sizeof(ConstantBuffer);
bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

DmaAllocationCreateInfo allocCreateInfo = {};
allocCreateInfo.usage = DMA_MEMORY_USAGE_CPU_ONLY;
allocCreateInfo.flags = DMA_ALLOCATION_CREATE_MAPPED_BIT;

VkBuffer buf;
DmaAllocation alloc;
DmaAllocationInfo allocInfo;
dmaCreateBuffer(allocator, &bufCreateInfo, &allocCreateInfo, &buf, &alloc,
&allocInfo);

// Buffer is already mapped. You can access its memory.
memcpy(allocInfo.pMappedData, &constantBufferData, sizeof(constantBufferData));
\endcode

There are some exceptions though, when you should consider mapping memory only
for a short period of time:

- When operating system is Windows 7 or 8.x (Windows 10 is not affected because
it uses WDDM2), device is discrete AMD GPU, and memory type is the special 256
MiB pool of `DEVICE_LOCAL + HOST_VISIBLE` memory (selected when you use
#DMA_MEMORY_USAGE_CPU_TO_GPU), then whenever a memory block allocated from this
memory type stays mapped for the time of any call to `vkQueueSubmit()` or
`vkQueuePresentKHR()`, this block is migrated by WDDM to system RAM, which
degrades performance. It doesn't matter if that particular memory block is
actually used by the command buffer being submitted.
- On Mac/MoltenVK there is a known bug - [Issue
#175](https://github.com/KhronosGroup/MoltenVK/issues/175) which requires
unmapping before GPU can see updated texture.
- Keeping many large memory blocks mapped may impact performance or stability of
some debugging tools.

\section memory_mapping_cache_control Cache flush and invalidate

Memory in Vulkan doesn't need to be unmapped before using it on GPU,
but unless a memory types has `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` flag set,
you need to manually **invalidate** cache before reading of mapped pointer
and **flush** cache after writing to mapped pointer.
Map/unmap operations don't do that automatically.
Vulkan provides following functions for this purpose
`vkFlushMappedMemoryRanges()`, `vkInvalidateMappedMemoryRanges()`, but this
library provides more convenient functions that refer to given allocation
object: dmaFlushAllocation(), dmaInvalidateAllocation().

Regions of memory specified for flush/invalidate must be aligned to
`VkPhysicalDeviceLimits::nonCoherentAtomSize`. This is automatically ensured by
the library. In any memory type that is `HOST_VISIBLE` but not `HOST_COHERENT`,
all allocations within blocks are aligned to this value, so their offsets are
always multiply of `nonCoherentAtomSize` and two different allocations never
share same "line" of this size.

Please note that memory allocated with #DMA_MEMORY_USAGE_CPU_ONLY is guaranteed
to be `HOST_COHERENT`.

Also, Windows drivers from all 3 **PC** GPU vendors (AMD, Intel, NVIDIA)
currently provide `HOST_COHERENT` flag on all memory types that are
`HOST_VISIBLE`, so on this platform you may not need to bother.

\section memory_mapping_finding_if_memory_mappable Finding out if memory is
mappable

It may happen that your allocation ends up in memory that is `HOST_VISIBLE`
(available for mapping) despite it wasn't explicitly requested. For example,
application may work on integrated graphics with unified memory (like Intel) or
allocation from video memory might have failed, so the library chose system
memory as fallback.

You can detect this case and map such allocation to access its memory on CPU
directly, instead of launching a transfer operation. In order to do that:
inspect `allocInfo.memoryType`, call dmaGetMemoryTypeProperties(), and look for
`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` flag in properties of that memory type.

\code
VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
bufCreateInfo.size = sizeof(ConstantBuffer);
bufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
VK_BUFFER_USAGE_TRANSFER_DST_BIT;

DmaAllocationCreateInfo allocCreateInfo = {};
allocCreateInfo.usage = DMA_MEMORY_USAGE_GPU_ONLY;
allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

VkBuffer buf;
DmaAllocation alloc;
DmaAllocationInfo allocInfo;
dmaCreateBuffer(allocator, &bufCreateInfo, &allocCreateInfo, &buf, &alloc,
&allocInfo);

VkMemoryPropertyFlags memFlags;
dmaGetMemoryTypeProperties(allocator, allocInfo.memoryType, &memFlags);
if((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
{
    // Allocation ended up in mappable memory. You can map it and access it
directly. void* mappedData; dmaMapMemory(allocator, alloc, &mappedData);
    memcpy(mappedData, &constantBufferData, sizeof(constantBufferData));
    dmaUnmapMemory(allocator, alloc);
}
else
{
    // Allocation ended up in non-mappable memory.
    // You need to create CPU-side buffer in DMA_MEMORY_USAGE_CPU_ONLY and make
a transfer.
}
\endcode

You can even use #DMA_ALLOCATION_CREATE_MAPPED_BIT flag while creating
allocations that are not necessarily `HOST_VISIBLE` (e.g. using
#DMA_MEMORY_USAGE_GPU_ONLY). If the allocation ends up in memory type that is
`HOST_VISIBLE`, it will be persistently mapped and you can use it directly. If
not, the flag is just ignored. Example:

\code
VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
bufCreateInfo.size = sizeof(ConstantBuffer);
bufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
VK_BUFFER_USAGE_TRANSFER_DST_BIT;

DmaAllocationCreateInfo allocCreateInfo = {};
allocCreateInfo.usage = DMA_MEMORY_USAGE_GPU_ONLY;
allocCreateInfo.flags = DMA_ALLOCATION_CREATE_MAPPED_BIT;

VkBuffer buf;
DmaAllocation alloc;
DmaAllocationInfo allocInfo;
dmaCreateBuffer(allocator, &bufCreateInfo, &allocCreateInfo, &buf, &alloc,
&allocInfo);

if(allocInfo.pUserData != nullptr)
{
    // Allocation ended up in mappable memory.
    // It's persistently mapped. You can access it directly.
    memcpy(allocInfo.pMappedData, &constantBufferData,
sizeof(constantBufferData));
}
else
{
    // Allocation ended up in non-mappable memory.
    // You need to create CPU-side buffer in DMA_MEMORY_USAGE_CPU_ONLY and make
a transfer.
}
\endcode


\page staying_within_budget Staying within budget

When developing a graphics-intensive game or program, it is important to avoid
allocating more GPU memory than it's physically available. When the memory is
over-committed, various bad things can happen, depending on the specific GPU,
graphics driver, and operating system:

- It may just work without any problems.
- The application may slow down because some memory blocks are moved to system
RAM and the GPU has to access them through PCI Express bus.
- A new allocation may take very long time to complete, even few seconds, and
possibly freeze entire system.
- The new allocation may fail with `DkResult_OutOfMemory`.
- It may even result in GPU crash (TDR), observed as `VK_ERROR_DEVICE_LOST`
  returned somewhere later.

\section staying_within_budget_querying_for_budget Querying for budget

To query for current memory usage and available budget, use function
dmaGetBudget(). Returned structure #DmaBudget contains quantities expressed in
bytes, per Vulkan memory heap.

Please note that this function returns different information and works faster
than dmaCalculateStats(). dmaGetBudget() can be called every frame or even
before every allocation, while dmaCalculateStats() is intended to be used
rarely, only to obtain statistical information, e.g. for debugging purposes.

It is recommended to use <b>VK_EXT_memory_budget</b> device extension to obtain
information about the budget from Vulkan device. VMA is able to use this
extension automatically. When not enabled, the allocator behaves same way, but
then it estimates current usage and available budget based on its internal
information and Vulkan memory heap sizes, which may be less precise. In order to
use this extension:

1. Make sure extensions VK_EXT_memory_budget and
VK_KHR_get_physical_device_properties2 required by it are available and enable
them. Please note that the first is a device extension and the second is
instance extension!
2. Use flag #DMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT when creating
#DmaAllocator object.
3. Make sure to call dmaSetCurrentFrameIndex() every frame. Budget is queried
from Vulkan inside of it to avoid overhead of querying it with every allocation.

\section staying_within_budget_controlling_memory_usage Controlling memory usage

There are many ways in which you can try to stay within the budget.

First, when making new allocation requires allocating a new memory block, the
library tries not to exceed the budget automatically. If a block with default
recommended size (e.g. 256 MB) would go over budget, a smaller block is
allocated, possibly even dedicated memory for just this resource.

If the size of the requested resource plus current memory usage is more than the
budget, by default the library still tries to create it, leaving it to the
Vulkan implementation whether the allocation succeeds or fails. You can change
this behavior by using #DMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT flag. With it,
the allocation is not made if it would exceed the budget or if the budget is
already exceeded. Some other allocations become lost instead to make room for
it, if the mechanism of [lost allocations](@ref lost_allocations) is used. If
that is not possible, the allocation fails with `DkResult_OutOfMemory`.
Example usage pattern may be to pass the
#DMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT flag when creating resources that are
not essential for the application (e.g. the texture of a specific object) and
not to pass it when creating critically important resources (e.g. render
targets).

Finally, you can also use #DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT flag to make
sure a new allocation is created only when it fits inside one of the existing
memory blocks. If it would require to allocate a new block, if fails instead
with `DkResult_OutOfMemory`. This also ensures that the function call
is very fast because it never goes to Vulkan to obtain a new block.

Please note that creating \ref custom_memory_pools with
DmaPoolCreateInfo::minBlockCount set to more than 0 will try to allocate memory
blocks without checking whether they fit within budget.


\page custom_memory_pools Custom memory pools

A memory pool contains a number of `DkMemBlock` blocks.
The library automatically creates and manages default pool for each memory type
available on the device. Default memory pool automatically grows in size. Size
of allocated blocks is also variable and managed automatically.

You can create custom pool and allocate memory out of it.
It can be useful if you want to:

- Keep certain kind of allocations separate from others.
- Enforce particular, fixed size of Vulkan memory blocks.
- Limit maximum amount of Vulkan memory allocated for that pool.
- Reserve minimum or fixed amount of Vulkan memory always preallocated for that
pool.

To use custom memory pools:

-# Fill DmaPoolCreateInfo structure.
-# Call dmaCreatePool() to obtain #DmaPool handle.
-# When making an allocation, set DmaAllocationCreateInfo::pool to this handle.
   You don't need to specify any other parameters of this structure, like
`usage`.

Example:

\code
// Create a pool that can have at most 2 blocks, 128 MiB each.
DmaPoolCreateInfo poolCreateInfo = {};
poolCreateInfo.memoryTypeIndex = ...
poolCreateInfo.blockSize = 128ull * 1024 * 1024;
poolCreateInfo.maxBlockCount = 2;

DmaPool pool;
dmaCreatePool(allocator, &poolCreateInfo, &pool);

// Allocate a buffer out of it.
VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
bufCreateInfo.size = 1024;
bufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
VK_BUFFER_USAGE_TRANSFER_DST_BIT;

DmaAllocationCreateInfo allocCreateInfo = {};
allocCreateInfo.pool = pool;

VkBuffer buf;
DmaAllocation alloc;
DmaAllocationInfo allocInfo;
dmaCreateBuffer(allocator, &bufCreateInfo, &allocCreateInfo, &buf, &alloc,
&allocInfo); \endcode

You have to free all allocations made from this pool before destroying it.

\code
dmaDestroyBuffer(allocator, buf, alloc);
dmaDestroyPool(allocator, pool);
\endcode

\section custom_memory_pools_MemTypeIndex Choosing memory type index

When creating a pool, you must explicitly specify memory type index.
To find the one suitable for your buffers or images, you can use helper
functions dmaFindMemoryTypeIndexForBufferInfo(),
dmaFindMemoryTypeIndexForImageInfo(). You need to provide structures with
example parameters of buffers or images that you are going to create in that
pool.

\code
VkBufferCreateInfo exampleBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
}; exampleBufCreateInfo.size = 1024; // Whatever. exampleBufCreateInfo.usage =
VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // Change
if needed.

DmaAllocationCreateInfo allocCreateInfo = {};
allocCreateInfo.usage = DMA_MEMORY_USAGE_GPU_ONLY; // Change if needed.

uint32_t memTypeIndex;
dmaFindMemoryTypeIndexForBufferInfo(allocator, &exampleBufCreateInfo,
&allocCreateInfo, &memTypeIndex);

DmaPoolCreateInfo poolCreateInfo = {};
poolCreateInfo.memoryTypeIndex = memTypeIndex;
// ...
\endcode

When creating buffers/images allocated in that pool, provide following
parameters:

- `VkBufferCreateInfo`: Prefer to pass same parameters as above.
  Otherwise you risk creating resources in a memory type that is not suitable
for them, which may result in undefined behavior. Using different
`VK_BUFFER_USAGE_` flags may work, but you shouldn't create images in a pool
intended for buffers or the other way around.
- DmaAllocationCreateInfo: You don't need to pass same parameters. Fill only
`pool` member. Other members are ignored anyway.

\section linear_algorithm Linear allocation algorithm

Each Vulkan memory block managed by this library has accompanying metadata that
keeps track of used and unused regions. By default, the metadata structure and
algorithm tries to find best place for new allocations among free regions to
optimize memory usage. This way you can allocate and free objects in any order.

![Default allocation algorithm](../gfx/Linear_allocator_1_algo_default.png)

Sometimes there is a need to use simpler, linear allocation algorithm. You can
create custom pool that uses such algorithm by adding flag
#DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT to DmaPoolCreateInfo::flags while creating
#DmaPool object. Then an alternative metadata management is used. It always
creates new allocations after last one and doesn't reuse free regions after
allocations freed in the middle. It results in better allocation performance and
less memory consumed by metadata.

![Linear allocation algorithm](../gfx/Linear_allocator_2_algo_linear.png)

With this one flag, you can create a custom pool that can be used in many ways:
free-at-once, stack, double stack, and ring buffer. See below for details.

\subsection linear_algorithm_free_at_once Free-at-once

In a pool that uses linear algorithm, you still need to free all the allocations
individually, e.g. by using dmaFreeMemory() or dmaDestroyBuffer(). You can free
them in any order. New allocations are always made after last one - free space
in the middle is not reused. However, when you release all the allocation and
the pool becomes empty, allocation starts from the beginning again. This way you
can use linear algorithm to speed up creation of allocations that you are going
to release all at once.

![Free-at-once](../gfx/Linear_allocator_3_free_at_once.png)

This mode is also available for pools created with
DmaPoolCreateInfo::maxBlockCount value that allows multiple memory blocks.

\subsection linear_algorithm_stack Stack

When you free an allocation that was created last, its space can be reused.
Thanks to this, if you always release allocations in the order opposite to their
creation (LIFO - Last In First Out), you can achieve behavior of a stack.

![Stack](../gfx/Linear_allocator_4_stack.png)

This mode is also available for pools created with
DmaPoolCreateInfo::maxBlockCount value that allows multiple memory blocks.

\subsection linear_algorithm_double_stack Double stack

The space reserved by a custom pool with linear algorithm may be used by two
stacks:

- First, default one, growing up from offset 0.
- Second, "upper" one, growing down from the end towards lower offsets.

To make allocation from upper stack, add flag
#DMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT to DmaAllocationCreateInfo::flags.

![Double stack](../gfx/Linear_allocator_7_double_stack.png)

Double stack is available only in pools with one memory block -
DmaPoolCreateInfo::maxBlockCount must be 1. Otherwise behavior is undefined.

When the two stacks' ends meet so there is not enough space between them for a
new allocation, such allocation fails with usual
`DkResult_OutOfMemory` error.

\subsection linear_algorithm_ring_buffer Ring buffer

When you free some allocations from the beginning and there is not enough free
space for a new one at the end of a pool, allocator's "cursor" wraps around to
the beginning and starts allocation there. Thanks to this, if you always release
allocations in the same order as you created them (FIFO - First In First Out),
you can achieve behavior of a ring buffer / queue.

![Ring buffer](../gfx/Linear_allocator_5_ring_buffer.png)

Pools with linear algorithm support [lost allocations](@ref lost_allocations)
when used as ring buffer. If there is not enough free space for a new
allocation, but existing allocations from the front of the queue can become
lost, they become lost and the allocation succeeds.

![Ring buffer with lost
allocations](../gfx/Linear_allocator_6_ring_buffer_lost.png)

Ring buffer is available only in pools with one memory block -
DmaPoolCreateInfo::maxBlockCount must be 1. Otherwise behavior is undefined.

\section buddy_algorithm Buddy allocation algorithm

There is another allocation algorithm that can be used with custom pools, called
"buddy". Its internal data structure is based on a tree of blocks, each having
size that is a power of two and a half of its parent's size. When you want to
allocate memory of certain size, a free node in the tree is located. If it's too
large, it is recursively split into two halves (called "buddies"). However, if
requested allocation size is not a power of two, the size of a tree node is
aligned up to the nearest power of two and the remaining space is wasted. When
two buddy nodes become free, they are merged back into one larger node.

![Buddy allocator](../gfx/Buddy_allocator.png)

The advantage of buddy allocation algorithm over default algorithm is faster
allocation and deallocation, as well as smaller external fragmentation. The
disadvantage is more wasted space (internal fragmentation).

For more information, please read ["Buddy memory allocation" on
Wikipedia](https://en.wikipedia.org/wiki/Buddy_memory_allocation) or other
sources that describe this concept in general.

To use buddy allocation algorithm with a custom pool, add flag
#DMA_POOL_CREATE_BUDDY_ALGORITHM_BIT to DmaPoolCreateInfo::flags while creating
#DmaPool object.

Several limitations apply to pools that use buddy algorithm:

- It is recommended to use DmaPoolCreateInfo::blockSize that is a power of two.
  Otherwise, only largest power of two smaller than the size is used for
  allocations. The remaining space always stays unused.
- [Margins](@ref debugging_memory_usage_margins) and
  [corruption detection](@ref debugging_memory_usage_corruption_detection)
  don't work in such pools.
- [Lost allocations](@ref lost_allocations) don't work in such pools. You can
  use them, but they never become lost. Support may be added in the future.
- [Defragmentation](@ref defragmentation) doesn't work with allocations made
from such pool.

\page defragmentation Defragmentation

Interleaved allocations and deallocations of many objects of varying size can
cause fragmentation over time, which can lead to a situation where the library
is unable to find a continuous range of free memory for a new allocation despite
there is enough free space, just scattered across many small free ranges between
existing allocations.

To mitigate this problem, you can use defragmentation feature:
structure #DmaDefragmentationInfo2, function dmaDefragmentationBegin(),
dmaDefragmentationEnd(). Given set of allocations, this function can move them
to compact used memory, ensure more continuous free space and possibly also free
some `DkMemBlock` blocks.

What the defragmentation does is:

- Updates #DmaAllocation objects to point to new `DkMemBlock` and offset.
  After allocation has been moved, its DmaAllocationInfo::deviceMemory and/or
  DmaAllocationInfo::offset changes. You must query them again using
  dmaGetAllocationInfo() if you need them.
- Moves actual data in memory.

What it doesn't do, so you need to do it yourself:

- Recreate buffers and images that were bound to allocations that were
defragmented and bind them with their new places in memory. You must use
`vkDestroyBuffer()`, `vkDestroyImage()`, `vkCreateBuffer()`, `vkCreateImage()`,
dmaBindBufferMemory(), dmaBindImageMemory() for that purpose and NOT
dmaDestroyBuffer(), dmaDestroyImage(), dmaCreateBuffer(), dmaCreateImage(),
because you don't need to destroy or create allocation objects!
- Recreate views and update descriptors that point to these buffers and images.

\section defragmentation_cpu Defragmenting CPU memory

Following example demonstrates how you can run defragmentation on CPU.
Only allocations created in memory types that are `HOST_VISIBLE` can be
defragmented. Others are ignored.

The way it works is:

- It temporarily maps entire memory blocks when necessary.
- It moves data using `memmove()` function.

\code
// Given following variables already initialized:
DkDevice device;
DmaAllocator allocator;
std::vector<VkBuffer> buffers;
std::vector<DmaAllocation> allocations;


const uint32_t allocCount = (uint32_t)allocations.size();
std::vector<bool> allocationsChanged(allocCount);

DmaDefragmentationInfo2 defragInfo = {};
defragInfo.allocationCount = allocCount;
defragInfo.pAllocations = allocations.data();
defragInfo.pAllocationsChanged = allocationsChanged.data();
defragInfo.maxCpuBytesToMove = DMA_WHOLE_SIZE; // No limit.
defragInfo.maxCpuAllocationsToMove = UINT32_MAX; // No limit.

DmaDefragmentationContext defragCtx;
dmaDefragmentationBegin(allocator, &defragInfo, nullptr, &defragCtx);
dmaDefragmentationEnd(allocator, defragCtx);

for(uint32_t i = 0; i < allocCount; ++i)
{
    if(allocationsChanged[i])
    {
        // Destroy buffer that is immutably bound to memory region which is no
longer valid. vkDestroyBuffer(device, buffers[i], nullptr);

        // Create new buffer with same parameters.
        VkBufferCreateInfo bufferInfo = ...;
        vkCreateBuffer(device, &bufferInfo, nullptr, &buffers[i]);

        // You can make dummy call to vkGetBufferMemoryRequirements here to
silence validation layer warning.

        // Bind new buffer to new memory region. Data contained in it is already
moved. DmaAllocationInfo allocInfo; dmaGetAllocationInfo(allocator,
allocations[i], &allocInfo); dmaBindBufferMemory(allocator, allocations[i],
buffers[i]);
    }
}
\endcode

Setting DmaDefragmentationInfo2::pAllocationsChanged is optional.
This output array tells whether particular allocation in
DmaDefragmentationInfo2::pAllocations at the same index has been modified during
defragmentation. You can pass null, but you then need to query every allocation
passed to defragmentation for new parameters using dmaGetAllocationInfo() if you
might need to recreate and rebind a buffer or image associated with it.

If you use [Custom memory pools](@ref choosing_memory_type_custom_memory_pools),
you can fill DmaDefragmentationInfo2::poolCount and
DmaDefragmentationInfo2::pPools instead of
DmaDefragmentationInfo2::allocationCount and
DmaDefragmentationInfo2::pAllocations to defragment all allocations in given
pools. You cannot use DmaDefragmentationInfo2::pAllocationsChanged in that case.
You can also combine both methods.

\section defragmentation_gpu Defragmenting GPU memory

It is also possible to defragment allocations created in memory types that are
not `HOST_VISIBLE`. To do that, you need to pass a command buffer that meets
requirements as described in DmaDefragmentationInfo2::commandBuffer. The way it
works is:

- It creates temporary buffers and binds them to entire memory blocks when
necessary.
- It issues `vkCmdCopyBuffer()` to passed command buffer.

Example:

\code
// Given following variables already initialized:
DkDevice device;
DmaAllocator allocator;
DkCmdBuf commandBuffer;
std::vector<VkBuffer> buffers;
std::vector<DmaAllocation> allocations;


const uint32_t allocCount = (uint32_t)allocations.size();
std::vector<bool> allocationsChanged(allocCount);

DkCmdBufBeginInfo cmdBufBeginInfo = ...;
vkBeginCommandBuffer(commandBuffer, &cmdBufBeginInfo);

DmaDefragmentationInfo2 defragInfo = {};
defragInfo.allocationCount = allocCount;
defragInfo.pAllocations = allocations.data();
defragInfo.pAllocationsChanged = allocationsChanged.data();
defragInfo.maxGpuBytesToMove = DMA_WHOLE_SIZE; // Notice it's "GPU" this time.
defragInfo.maxGpuAllocationsToMove = UINT32_MAX; // Notice it's "GPU" this time.
defragInfo.commandBuffer = commandBuffer;

DmaDefragmentationContext defragCtx;
dmaDefragmentationBegin(allocator, &defragInfo, nullptr, &defragCtx);

vkEndCommandBuffer(commandBuffer);

// Submit commandBuffer.
// Wait for a fence that ensures commandBuffer execution finished.

dmaDefragmentationEnd(allocator, defragCtx);

for(uint32_t i = 0; i < allocCount; ++i)
{
    if(allocationsChanged[i])
    {
        // Destroy buffer that is immutably bound to memory region which is no
longer valid. vkDestroyBuffer(device, buffers[i], nullptr);

        // Create new buffer with same parameters.
        VkBufferCreateInfo bufferInfo = ...;
        vkCreateBuffer(device, &bufferInfo, nullptr, &buffers[i]);

        // You can make dummy call to vkGetBufferMemoryRequirements here to
silence validation layer warning.

        // Bind new buffer to new memory region. Data contained in it is already
moved. DmaAllocationInfo allocInfo; dmaGetAllocationInfo(allocator,
allocations[i], &allocInfo); dmaBindBufferMemory(allocator, allocations[i],
buffers[i]);
    }
}
\endcode

You can combine these two methods by specifying non-zero `maxGpu*` as well as
`maxCpu*` parameters. The library automatically chooses best method to
defragment each memory pool.

You may try not to block your entire program to wait until defragmentation
finishes, but do it in the background, as long as you carefully fullfill
requirements described in function dmaDefragmentationBegin().

\section defragmentation_additional_notes Additional notes

It is only legal to defragment allocations bound to:

- buffers
- images created with `VK_IMAGE_CREATE_ALIAS_BIT`, `VK_IMAGE_TILING_LINEAR`, and
  being currently in `VK_IMAGE_LAYOUT_GENERAL` or
`VK_IMAGE_LAYOUT_PREINITIALIZED`.

Defragmentation of images created with `VK_IMAGE_TILING_OPTIMAL` or in any other
layout may give undefined results.

If you defragment allocations bound to images, new images to be bound to new
memory region after defragmentation should be created with
`VK_IMAGE_LAYOUT_PREINITIALIZED` and then transitioned to their original layout
from before defragmentation if needed using an image memory barrier.

While using defragmentation, you may experience validation layer warnings, which
you just need to ignore. See [Validation layer warnings](@ref
general_considerations_validation_layer_warnings).

Please don't expect memory to be fully compacted after defragmentation.
Algorithms inside are based on some heuristics that try to maximize number of
Vulkan memory blocks to make totally empty to release them, as well as to
maximimze continuous empty space inside remaining blocks, while minimizing the
number and size of allocations that need to be moved. Some fragmentation may
still remain - this is normal.

\section defragmentation_custom_algorithm Writing custom defragmentation
algorithm

If you want to implement your own, custom defragmentation algorithm,
there is infrastructure prepared for that,
but it is not exposed through the library API - you need to hack its source
code. Here are steps needed to do this:

-# Main thing you need to do is to define your own class derived from base
abstract class `DmaDefragmentationAlgorithm` and implement your version of its
pure virtual methods. See definition and comments of this class for details.
-# Your code needs to interact with device memory block metadata.
   If you need more access to its data than it's provided by its public
interface, declare your new class as a friend class e.g. in class
`DmaBlockMetadata_Generic`.
-# If you want to create a flag that would enable your algorithm or pass some
additional flags to configure it, add them to `DmaDefragmentationFlagBits` and
use them in DmaDefragmentationInfo2::flags.
-# Modify function `DmaBlockVectorDefragmentationContext::Begin` to create
object of your new class whenever needed.


\page lost_allocations Lost allocations

If your game oversubscribes video memory, if may work OK in previous-generation
graphics APIs (DirectX 9, 10, 11, OpenGL) because resources are automatically
paged to system RAM. In Vulkan you can't do it because when you run out of
memory, an allocation just fails. If you have more data (e.g. textures) that can
fit into VRAM and you don't need it all at once, you may want to upload them to
GPU on demand and "push out" ones that are not used for a long time to make room
for the new ones, effectively using VRAM (or a cartain memory pool) as a form of
cache. Vulkan Memory Allocator can help you with that by supporting a concept of
"lost allocations".

To create an allocation that can become lost, include
#DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT flag in
DmaAllocationCreateInfo::flags. Before using a buffer or image bound to such
allocation in every new frame, you need to query it if it's not lost. To check
it, call dmaTouchAllocation(). If the allocation is lost, you should not use it
or buffer/image bound to it. You mustn't forget to destroy this allocation and
this buffer/image. dmaGetAllocationInfo() can also be used for checking status
of the allocation. Allocation is lost when returned
DmaAllocationInfo::deviceMemory == `DMA_NULL_HANDLE`.

To create an allocation that can make some other allocations lost to make room
for it, use #DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT flag. You will
usually use both flags #DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT and
#DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT at the same time.

Warning! Current implementation uses quite naive, brute force algorithm,
which can make allocation calls that use
#DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT flag quite slow. A new, more
optimal algorithm and data structure to speed this up is planned for the future.

<b>Q: When interleaving creation of new allocations with usage of existing ones,
how do you make sure that an allocation won't become lost while it's used in the
current frame?</b>

It is ensured because dmaTouchAllocation() / dmaGetAllocationInfo() not only
returns allocation status/parameters and checks whether it's not lost, but when
it's not, it also atomically marks it as used in the current frame, which makes
it impossible to become lost in that frame. It uses lockless algorithm, so it
works fast and doesn't involve locking any internal mutex.

<b>Q: What if my allocation may still be in use by the GPU when it's rendering a
previous frame while I already submit new frame on the CPU?</b>

You can make sure that allocations "touched" by dmaTouchAllocation() /
dmaGetAllocationInfo() will not become lost for a number of additional frames
back from the current one by specifying this number as
DmaAllocatorCreateInfo::frameInUseCount (for default memory pool) and
DmaPoolCreateInfo::frameInUseCount (for custom pool).

<b>Q: How do you inform the library when new frame starts?</b>

You need to call function dmaSetCurrentFrameIndex().

Example code:

\code
struct MyBuffer
{
    VkBuffer m_Buf = nullptr;
    DmaAllocation m_Alloc = nullptr;

    // Called when the buffer is really needed in the current frame.
    void EnsureBuffer();
};

void MyBuffer::EnsureBuffer()
{
    // Buffer has been created.
    if(m_Buf != DMA_NULL_HANDLE)
    {
        // Check if its allocation is not lost + mark it as used in current
frame. if(dmaTouchAllocation(allocator, m_Alloc))
        {
            // It's all OK - safe to use m_Buf.
            return;
        }
    }

    // Buffer not yet exists or lost - destroy and recreate it.

    dmaDestroyBuffer(allocator, m_Buf, m_Alloc);

    VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufCreateInfo.size = 1024;
    bufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    DmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = DMA_MEMORY_USAGE_GPU_ONLY;
    allocCreateInfo.flags = DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT |
        DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT;

    dmaCreateBuffer(allocator, &bufCreateInfo, &allocCreateInfo, &m_Buf,
&m_Alloc, nullptr);
}
\endcode

When using lost allocations, you may see some Vulkan validation layer warnings
about overlapping regions of memory bound to different kinds of buffers and
images. This is still valid as long as you implement proper handling of lost
allocations (like in the example above) and don't use them.

You can create an allocation that is already in lost state from the beginning
using function dmaCreateLostAllocation(). It may be useful if you need a "dummy"
allocation that is not null.

You can call function dmaMakePoolAllocationsLost() to set all eligible
allocations in a specified custom pool to lost state. Allocations that have been
"touched" in current frame or DmaPoolCreateInfo::frameInUseCount frames back
cannot become lost.

<b>Q: Can I touch allocation that cannot become lost?</b>

Yes, although it has no visible effect.
Calls to dmaGetAllocationInfo() and dmaTouchAllocation() update last use frame
index also for allocations that cannot become lost, but the only way to observe
it is to dump internal allocator state using dmaBuildStatsString(). You can use
this feature for debugging purposes to explicitly mark allocations that you use
in current frame and then analyze JSON dump to see for how long each allocation
stays unused.


\page statistics Statistics

This library contains functions that return information about its internal
state, especially the amount of memory allocated from Vulkan. Please keep in
mind that these functions need to traverse all internal data structures to
gather these information, so they may be quite time-consuming. Don't call them
too often.

\section statistics_numeric_statistics Numeric statistics

You can query for overall statistics of the allocator using function
dmaCalculateStats(). Information are returned using structure #DmaStats. It
contains #DmaStatInfo - number of allocated blocks, number of allocations
(occupied ranges in these blocks), number of unused (free) ranges in these
blocks, number of bytes used and unused (but still allocated from Vulkan) and
other information. They are summed across memory heaps, memory types and total
for whole allocator.

You can query for statistics of a custom pool using function dmaGetPoolStats().
Information are returned using structure #DmaPoolStats.

You can query for information about specific allocation using function
dmaGetAllocationInfo(). It fill structure #DmaAllocationInfo.

\section statistics_json_dump JSON dump

You can dump internal state of the allocator to a string in JSON format using
function dmaBuildStatsString(). The result is guaranteed to be correct JSON. It
uses ANSI encoding. Any strings provided by user (see [Allocation names](@ref
allocation_names)) are copied as-is and properly escaped for JSON, so if they
use UTF-8, ISO-8859-2 or any other encoding, this JSON string can be treated as
using this encoding. It must be freed using function dmaFreeStatsString().

The format of this JSON string is not part of official documentation of the
library, but it will not change in backward-incompatible way without increasing
library major version number and appropriate mention in changelog.

The JSON string contains all the data that can be obtained using
dmaCalculateStats(). It can also contain detailed map of allocated memory blocks
and their regions - free and occupied by allocations. This allows e.g. to
visualize the memory or assess fragmentation.


\page allocation_annotation Allocation names and user data

\section allocation_user_data Allocation user data

You can annotate allocations with your own information, e.g. for debugging
purposes. To do that, fill DmaAllocationCreateInfo::pUserData field when
creating an allocation. It's an opaque `void*` pointer. You can use it e.g. as a
pointer, some handle, index, key, ordinal number or any other value that would
associate the allocation with your custom metadata.

\code
VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
// Fill bufferInfo...

MyBufferMetadata* pMetadata = CreateBufferMetadata();

DmaAllocationCreateInfo allocCreateInfo = {};
allocCreateInfo.usage = DMA_MEMORY_USAGE_GPU_ONLY;
allocCreateInfo.pUserData = pMetadata;

VkBuffer buffer;
DmaAllocation allocation;
dmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &buffer, &allocation,
nullptr); \endcode

The pointer may be later retrieved as DmaAllocationInfo::pUserData:

\code
DmaAllocationInfo allocInfo;
dmaGetAllocationInfo(allocator, allocation, &allocInfo);
MyBufferMetadata* pMetadata = (MyBufferMetadata*)allocInfo.pUserData;
\endcode

It can also be changed using function dmaSetAllocationUserData().

Values of (non-zero) allocations' `pUserData` are printed in JSON report created
by dmaBuildStatsString(), in hexadecimal form.

\section allocation_names Allocation names

There is alternative mode available where `pUserData` pointer is used to point
to a null-terminated string, giving a name to the allocation. To use this mode,
set #DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT flag in
DmaAllocationCreateInfo::flags. Then `pUserData` passed as
DmaAllocationCreateInfo::pUserData or argument to dmaSetAllocationUserData()
must be either null or pointer to a null-terminated string. The library creates
internal copy of the string, so the pointer you pass doesn't need to be valid
for whole lifetime of the allocation. You can free it after the call.

\code
VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
// Fill imageInfo...

std::string imageName = "Texture: ";
imageName += fileName;

DmaAllocationCreateInfo allocCreateInfo = {};
allocCreateInfo.usage = DMA_MEMORY_USAGE_GPU_ONLY;
allocCreateInfo.flags = DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
allocCreateInfo.pUserData = imageName.c_str();

VkImage image;
DmaAllocation allocation;
dmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &image, &allocation,
nullptr); \endcode

The value of `pUserData` pointer of the allocation will be different than the
one you passed when setting allocation's name - pointing to a buffer managed
internally that holds copy of the string.

\code
DmaAllocationInfo allocInfo;
dmaGetAllocationInfo(allocator, allocation, &allocInfo);
const char* imageName = (const char*)allocInfo.pUserData;
printf("Image name: %s\n", imageName);
\endcode

That string is also printed in JSON report created by dmaBuildStatsString().


\page debugging_memory_usage Debugging incorrect memory usage

If you suspect a bug with memory usage, like usage of uninitialized memory or
memory being overwritten out of bounds of an allocation,
you can use debug features of this library to verify this.

\section debugging_memory_usage_initialization Memory initialization

If you experience a bug with incorrect and nondeterministic data in your program
and you suspect uninitialized memory to be used, you can enable automatic memory
initialization to verify this. To do it, define macro
`DMA_DEBUG_INITIALIZE_ALLOCATIONS` to 1.

\code
#define DMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#include "vk_mem_alloc.h"
\endcode

It makes memory of all new allocations initialized to bit pattern `0xDCDCDCDC`.
Before an allocation is destroyed, its memory is filled with bit pattern
`0xEFEFEFEF`. Memory is automatically mapped and unmapped if necessary.

If you find these values while debugging your program, good chances are that you
incorrectly read Vulkan memory that is allocated but not initialized, or already
freed, respectively.

Memory initialization works only with memory types that are `HOST_VISIBLE`.
It works also with dedicated allocations.
It doesn't work with allocations created with
#DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT flag, as they cannot be mapped.

\section debugging_memory_usage_margins Margins

By default, allocations are laid out in memory blocks next to each other if
possible (considering required alignment, `bufferImageGranularity`, and
`nonCoherentAtomSize`).

![Allocations without margin](../gfx/Margins_1.png)

Define macro `DMA_DEBUG_MARGIN` to some non-zero value (e.g. 16) to enforce
specified number of bytes as a margin before and after every allocation.

\code
#define DMA_DEBUG_MARGIN 16
#include "vk_mem_alloc.h"
\endcode

![Allocations with margin](../gfx/Margins_2.png)

If your bug goes away after enabling margins, it means it may be caused by
memory being overwritten outside of allocation boundaries. It is not 100%
certain though. Change in application behavior may also be caused by different
order and distribution of allocations across memory blocks after margins are
applied.

The margin is applied also before first and after last allocation in a block.
It may occur only once between two adjacent allocations.

Margins work with all types of memory.

Margin is applied only to allocations made out of memory blocks and not to
dedicated allocations, which have their own memory block of specific size. It is
thus not applied to allocations made using
#DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT flag or those automatically decided
to put into dedicated allocations, e.g. due to its large size or recommended by
VK_KHR_dedicated_allocation extension. Margins are also not active in custom
pools created with #DMA_POOL_CREATE_BUDDY_ALGORITHM_BIT flag.

Margins appear in [JSON dump](@ref statistics_json_dump) as part of free space.

Note that enabling margins increases memory usage and fragmentation.

\section debugging_memory_usage_corruption_detection Corruption detection

You can additionally define macro `DMA_DEBUG_DETECT_CORRUPTION` to 1 to enable
validation of contents of the margins.

\code
#define DMA_DEBUG_MARGIN 16
#define DMA_DEBUG_DETECT_CORRUPTION 1
#include "vk_mem_alloc.h"
\endcode

When this feature is enabled, number of bytes specified as `DMA_DEBUG_MARGIN`
(it must be multiply of 4) before and after every allocation is filled with a
magic number. This idea is also know as "canary". Memory is automatically mapped
and unmapped if necessary.

This number is validated automatically when the allocation is destroyed.
If it's not equal to the expected value, `DMA_ASSERT()` is executed.
It clearly means that either CPU or GPU overwritten the memory outside of
boundaries of the allocation, which indicates a serious bug.

You can also explicitly request checking margins of all allocations in all
memory blocks that belong to specified memory types by using function
dmaCheckCorruption(), or in memory blocks that belong to specified custom pool,
by using function dmaCheckPoolCorruption().

Margin validation (corruption detection) works only for memory types that are
`HOST_VISIBLE` and `HOST_COHERENT`.


\page record_and_replay Record and replay

\section record_and_replay_introduction Introduction

While using the library, sequence of calls to its functions together with their
parameters can be recorded to a file and later replayed using standalone player
application. It can be useful to:

- Test correctness - check if same sequence of calls will not cause crash or
  failures on a target platform.
- Gather statistics - see number of allocations, peak memory usage, number of
  calls etc.
- Benchmark performance - see how much time it takes to replay the whole
  sequence.

\section record_and_replay_usage Usage

Recording functionality is disabled by default.
To enable it, define following macro before every include of this library:

\code
#define DMA_RECORDING_ENABLED 1
\endcode

<b>To record sequence of calls to a file:</b> Fill in
DmaAllocatorCreateInfo::pRecordSettings member while creating #DmaAllocator
object. File is opened and written during whole lifetime of the allocator.

<b>To replay file:</b> Use DmaReplay - standalone command-line program.
Precompiled binary can be found in "bin" directory.
Its source can be found in "src/DmaReplay" directory.
Its project is generated by Premake.
Command line syntax is printed when the program is launched without parameters.
Basic usage:

    DmaReplay.exe MyRecording.csv

<b>Documentation of file format</b> can be found in file: "docs/Recording file
format.md". It's a human-readable, text file in CSV format (Comma Separated
Values).

\section record_and_replay_additional_considerations Additional considerations

- Replaying file that was recorded on a different GPU (with different parameters
  like `bufferImageGranularity`, `nonCoherentAtomSize`, and especially different
  set of memory heaps and types) may give different performance and memory usage
  results, as well as issue some warnings and errors.
- Current implementation of recording in VMA, as well as DmaReplay application,
is coded and tested only on Windows. Inclusion of recording code is driven by
  `DMA_RECORDING_ENABLED` macro. Support for other platforms should be easy to
  add. Contributions are welcomed.


\page usage_patterns Recommended usage patterns

See also slides from talk:
[Sawicki, Adam. Advanced Graphics Techniques Tutorial: Memory management in
Vulkan and DX12. Game Developers Conference,
2018](https://www.gdcvault.com/play/1025458/Advanced-Graphics-Techniques-Tutorial-New)


\section usage_patterns_common_mistakes Common mistakes

<b>Use of CPU_TO_GPU instead of CPU_ONLY memory</b>

#DMA_MEMORY_USAGE_CPU_TO_GPU is recommended only for resources that will be
mapped and written by the CPU, as well as read directly by the GPU - like some
buffers or textures updated every frame (dynamic). If you create a staging copy
of a resource to be written by CPU and then used as a source of transfer to
another resource placed in the GPU memory, that staging resource should be
created with #DMA_MEMORY_USAGE_CPU_ONLY. Please read the descriptions of these
enums carefully for details.

<b>Unnecessary use of custom pools</b>

\ref custom_memory_pools may be useful for special purposes - when you want to
keep certain type of resources separate e.g. to reserve minimum amount of memory
for them, limit maximum amount of memory they can occupy, or make some of them
push out the other through the mechanism of \ref lost_allocations. For most
resources this is not needed and so it is not recommended to create #DmaPool
objects and allocations out of them. Allocating from the default pool is
sufficient.

\section usage_patterns_simple Simple patterns

\subsection usage_patterns_simple_render_targets Render targets

<b>When:</b>
Any resources that you frequently write and read on GPU,
e.g. images used as color attachments (aka "render targets"), depth-stencil
attachments, images/buffers used as storage image/buffer (aka "Unordered Access
View (UAV)").

<b>What to do:</b>
Create them in video memory that is fastest to access from GPU using
#DMA_MEMORY_USAGE_GPU_ONLY.

Consider using [VK_KHR_dedicated_allocation](@ref vk_khr_dedicated_allocation)
extension and/or manually creating them as dedicated allocations using
#DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, especially if they are large or if
you plan to destroy and recreate them e.g. when display resolution changes.
Prefer to create such resources first and all other GPU resources (like textures
and vertex buffers) later.

\subsection usage_patterns_simple_immutable_resources Immutable resources

<b>When:</b>
Any resources that you fill on CPU only once (aka "immutable") or infrequently
and then read frequently on GPU,
e.g. textures, vertex and index buffers, constant buffers that don't change
often.

<b>What to do:</b>
Create them in video memory that is fastest to access from GPU using
#DMA_MEMORY_USAGE_GPU_ONLY.

To initialize content of such resource, create a CPU-side (aka "staging") copy
of it in system memory - #DMA_MEMORY_USAGE_CPU_ONLY, map it, fill it, and submit
a transfer from it to the GPU resource. You can keep the staging copy if you
need it for another upload transfer in the future. If you don't, you can destroy
it or reuse this buffer for uploading different resource after the transfer
finishes.

Prefer to create just buffers in system memory rather than images, even for
uploading textures. Use `vkCmdCopyBufferToImage()`. Dont use images with
`VK_IMAGE_TILING_LINEAR`.

\subsection usage_patterns_dynamic_resources Dynamic resources

<b>When:</b>
Any resources that change frequently (aka "dynamic"), e.g. every frame or every
draw call, written on CPU, read on GPU.

<b>What to do:</b>
Create them using #DMA_MEMORY_USAGE_CPU_TO_GPU.
You can map it and write to it directly on CPU, as well as read from it on GPU.

This is a more complex situation. Different solutions are possible,
and the best one depends on specific GPU type, but you can use this simple
approach for the start. Prefer to write to such resource sequentially (e.g.
using `memcpy`). Don't perform random access or any reads from it on CPU, as it
may be very slow.

\subsection usage_patterns_readback Readback

<b>When:</b>
Resources that contain data written by GPU that you want to read back on CPU,
e.g. results of some computations.

<b>What to do:</b>
Create them using #DMA_MEMORY_USAGE_GPU_TO_CPU.
You can write to them directly on GPU, as well as map and read them on CPU.

\section usage_patterns_advanced Advanced patterns

\subsection usage_patterns_integrated_graphics Detecting integrated graphics

You can support integrated graphics (like Intel HD Graphics, AMD APU) better
by detecting it in Vulkan.
To do it, call `vkGetPhysicalDeviceProperties()`, inspect
`VkPhysicalDeviceProperties::deviceType` and look for
`VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU`. When you find it, you can assume that
memory is unified and all memory types are comparably fast to access from GPU,
regardless of `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`.

You can then sum up sizes of all available memory heaps and treat them as useful
for your GPU resources, instead of only `DEVICE_LOCAL` ones. You can also prefer
to create your resources in memory types that are `HOST_VISIBLE` to map them
directly instead of submitting explicit transfer (see below).

\subsection usage_patterns_direct_vs_transfer Direct access versus transfer

For resources that you frequently write on CPU and read on GPU, many solutions
are possible:

-# Create one copy in video memory using #DMA_MEMORY_USAGE_GPU_ONLY,
   second copy in system memory using #DMA_MEMORY_USAGE_CPU_ONLY and submit
explicit tranfer each time.
-# Create just single copy using #DMA_MEMORY_USAGE_CPU_TO_GPU, map it and fill
it on CPU, read it directly on GPU.
-# Create just single copy using #DMA_MEMORY_USAGE_CPU_ONLY, map it and fill it
on CPU, read it directly on GPU.

Which solution is the most efficient depends on your resource and especially on
the GPU. It is best to measure it and then make the decision. Some general
recommendations:

- On integrated graphics use (2) or (3) to avoid unnecesary time and memory
overhead related to using a second copy and making transfer.
- For small resources (e.g. constant buffers) use (2).
  Discrete AMD cards have special 256 MiB pool of video memory that is directly
mappable. Even if the resource ends up in system memory, its data may be cached
on GPU after first fetch over PCIe bus.
- For larger resources (e.g. textures), decide between (1) and (2).
  You may want to differentiate NVIDIA and AMD, e.g. by looking for memory type
that is both `DEVICE_LOCAL` and `HOST_VISIBLE`. When you find it, use (2),
otherwise use (1).

Similarly, for resources that you frequently write on GPU and read on CPU,
multiple solutions are possible:

-# Create one copy in video memory using #DMA_MEMORY_USAGE_GPU_ONLY,
   second copy in system memory using #DMA_MEMORY_USAGE_GPU_TO_CPU and submit
explicit tranfer each time.
-# Create just single copy using #DMA_MEMORY_USAGE_GPU_TO_CPU, write to it
directly on GPU, map it and read it on CPU.

You should take some measurements to decide which option is faster in case of
your specific resource.

If you don't want to specialize your code for specific types of GPUs, you can
still make an simple optimization for cases when your resource ends up in
mappable memory to use it directly in this case instead of creating CPU-side
staging copy. For details see [Finding out if memory is mappable](@ref
memory_mapping_finding_if_memory_mappable).


\page configuration Configuration

Please check "CONFIGURATION SECTION" in the code to find macros that you can
define before each include of this file or change directly in this file to
provide your own implementation of basic facilities like assert, `min()` and
`max()` functions, mutex, atomic etc. The library uses its own implementation of
containers by default, but you can switch to using STL containers instead.

For example, define `DMA_ASSERT(expr)` before including the library to provide
custom implementation of the assertion, compatible with your project.
By default it is defined to standard C `assert(expr)` in `_DEBUG` configuration
and empty otherwise.

\section config_Vulkan_functions Pointers to Vulkan functions

The library uses Vulkan functions straight from the `vulkan.h` header by
default. If you want to provide your own pointers to these functions, e.g.
fetched using `vkGetInstanceProcAddr()` and `vkGetDeviceProcAddr()`:

-# Define `DMA_STATIC_VULKAN_FUNCTIONS 0`.
-# Provide valid pointers through DmaAllocatorCreateInfo::pVulkanFunctions.

\section custom_memory_allocator Custom host memory allocator

If you use custom allocator for CPU memory rather than default operator `new`
and `delete` from C++, you can make this library using your allocator as well
by filling optional member DmaAllocatorCreateInfo::pAllocationCallbacks. These
functions will be passed to Vulkan, as well as used by the library itself to
make any CPU-side allocations.

\section allocation_callbacks Device memory allocation callbacks

The library makes calls to `vkAllocateMemory()` and `vkFreeMemory()` internally.
You can setup callbacks to be informed about these calls, e.g. for the purpose
of gathering some statistics. To do it, fill optional member
DmaAllocatorCreateInfo::pDeviceMemoryCallbacks.

\section heap_memory_limit Device heap memory limit

When device memory of certain heap runs out of free space, new allocations may
fail (returning error code) or they may succeed, silently pushing some existing
memory blocks from GPU VRAM to system RAM (which degrades performance). This
behavior is implementation-dependant - it depends on GPU vendor and graphics
driver.

On AMD cards it can be controlled while creating Vulkan device object by using
VK_AMD_memory_overallocation_behavior extension, if available.

Alternatively, if you want to test how your program behaves with limited amount
of Vulkan device memory available without switching your graphics card to one
that really has smaller VRAM, you can use a feature of this library intended for
this purpose. To do it, fill optional member
DmaAllocatorCreateInfo::pHeapSizeLimit.



\page vk_khr_dedicated_allocation VK_KHR_dedicated_allocation

VK_KHR_dedicated_allocation is a Vulkan extension which can be used to improve
performance on some GPUs. It augments Vulkan API with possibility to query
driver whether it prefers particular buffer or image to have its own, dedicated
allocation (separate `DkMemBlock` block) for better efficiency - to be able
to do some internal optimizations.

The extension is supported by this library. It will be used automatically when
enabled. To enable it:

1 . When creating Vulkan device, check if following 2 device extensions are
supported (call `vkEnumerateDeviceExtensionProperties()`).
If yes, enable them (fill `DkDeviceCreateInfo::ppEnabledExtensionNames`).

- VK_KHR_get_memory_requirements2
- VK_KHR_dedicated_allocation

If you enabled these extensions:

2 . Use #DMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT flag when creating
your #DmaAllocator`to inform the library that you enabled required extensions
and you want the library to use them.

\code
allocatorInfo.flags |= DMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;

dmaCreateAllocator(&allocatorInfo, &allocator);
\endcode

That's all. The extension will be automatically used whenever you create a
buffer using dmaCreateBuffer() or image using dmaCreateImage().

When using the extension together with Vulkan Validation Layer, you will receive
warnings like this:

    vkBindBufferMemory(): Binding memory to buffer 0x33 but
vkGetBufferMemoryRequirements() has not been called on that buffer.

It is OK, you should just ignore it. It happens because you use function
`vkGetBufferMemoryRequirements2KHR()` instead of standard
`vkGetBufferMemoryRequirements()`, while the validation layer seems to be
unaware of it.

To learn more about this extension, see:

- [VK_KHR_dedicated_allocation in Vulkan
specification](https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html#VK_KHR_dedicated_allocation)
- [VK_KHR_dedicated_allocation unofficial
manual](http://asawicki.info/articles/VK_KHR_dedicated_allocation.php5)



\page general_considerations General considerations

\section general_considerations_thread_safety Thread safety

- The library has no global state, so separate #DmaAllocator objects can be used
  independently.
  There should be no need to create multiple such objects though - one per
`DkDevice` is enough.
- By default, all calls to functions that take #DmaAllocator as first parameter
  are safe to call from multiple threads simultaneously because they are
  synchronized internally when needed.
- When the allocator is created with
#DMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT flag, calls to functions that
take such #DmaAllocator object must be synchronized externally.
- Access to a #DmaAllocation object must be externally synchronized. For
example, you must not call dmaGetAllocationInfo() and dmaMapMemory() from
different threads at the same time if you pass the same #DmaAllocation object to
these functions.

\section general_considerations_validation_layer_warnings Validation layer
warnings

When using this library, you can meet following types of warnings issued by
Vulkan validation layer. They don't necessarily indicate a bug, so you may need
to just ignore them.

- *vkBindBufferMemory(): Binding memory to buffer 0xeb8e4 but
vkGetBufferMemoryRequirements() has not been called on that buffer.*
  - It happens when VK_KHR_dedicated_allocation extension is enabled.
    `vkGetBufferMemoryRequirements2KHR` function is used instead, while
validation layer seems to be unaware of it.
- *Mapping an image with layout VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
can result in undefined behavior if this memory is used by the device. Only
GENERAL or PREINITIALIZED should be used.*
  - It happens when you map a buffer or image, because the library maps entire
    `DkMemBlock` block, where different types of images and buffers may end
    up together, especially on GPUs with unified memory like Intel.
- *Non-linear image 0xebc91 is aliased with linear buffer 0xeb8e4 which may
indicate a bug.*
  - It happens when you use lost allocations, and a new image or buffer is
    created in place of an existing object that bacame lost.
  - It may happen also when you use [defragmentation](@ref defragmentation).

\section general_considerations_allocation_algorithm Allocation algorithm

The library uses following algorithm for allocation, in order:

-# Try to find free range of memory in existing blocks.
-# If failed, try to create a new block of `DkMemBlock`, with preferred
block size.
-# If failed, try to create such block with size/2, size/4, size/8.
-# If failed and #DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT flag was
   specified, try to find space in existing blocks, possilby making some other
   allocations lost.
-# If failed, try to allocate separate `DkMemBlock` for this allocation,
   just like when you use #DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT.
-# If failed, choose other memory type that meets the requirements specified in
   DmaAllocationCreateInfo and go to point 1.
-# If failed, return `DkResult_OutOfMemory`.

\section general_considerations_features_not_supported Features not supported

Features deliberately excluded from the scope of this library:

- Data transfer. Uploading (straming) and downloading data of buffers and images
  between CPU and GPU memory and related synchronization is responsibility of
the user. Defining some "texture" object that would automatically stream its
data from a staging copy in CPU memory to GPU memory would rather be a feature
of another, higher-level library implemented on top of VMA.
- Allocations for imported/exported external memory. They tend to require
  explicit memory type index and dedicated allocation anyway, so they don't
  interact with main features of this library. Such special purpose allocations
  should be made manually, using `vkCreateBuffer()` and `vkAllocateMemory()`.
- Recreation of buffers and images. Although the library has functions for
  buffer and image creation (dmaCreateBuffer(), dmaCreateImage()), you need to
  recreate these objects yourself after defragmentation. That's because the big
  structures `VkBufferCreateInfo`, `VkImageCreateInfo` are not stored in
  #DmaAllocation object.
- Handling CPU memory allocation failures. When dynamically creating small C++
  objects in CPU memory (not Vulkan memory), allocation failures are not checked
  and handled gracefully, because that would complicate code significantly and
  is usually not needed in desktop PC applications anyway.
- Code free of any compiler warnings. Maintaining the library to compile and
  work correctly on so many different platforms is hard enough. Being free of
  any warnings, on any version of any compiler, is simply not feasible.
- This is a C++ library with C interface.
  Bindings or ports to any other programming languages are welcomed as external
projects and are not going to be included into this repository.

*/

/*
Define this macro to 0/1 to disable/enable support for recording functionality,
available through DmaAllocatorCreateInfo::pRecordSettings.
*/
// TODO: Port this for deko
#ifndef DMA_RECORDING_ENABLED
#define DMA_RECORDING_ENABLED 0
#endif

#ifndef NOMINMAX
#define NOMINMAX // For windows.h
#endif

#include <deko3d.h>

// Define these macros to decorate all public functions with additional code,
// before and after returned type, appropriately. This may be useful for
// exporing the functions when compiling VMA as a separate library. Example:
// #define DMA_CALL_PRE  __declspec(dllexport)
// #define DMA_CALL_POST __cdecl
#ifndef DMA_CALL_PRE
#define DMA_CALL_PRE
#endif
#ifndef DMA_CALL_POST
#define DMA_CALL_POST
#endif

/** \struct DmaAllocator
\brief Represents main object of this library initialized.

Fill structure #DmaAllocatorCreateInfo and call function dmaCreateAllocator() to
create it. Call function dmaDestroyAllocator() to destroy it.

It is recommended to create just one object of this type per `DkDevice` object,
right after Vulkan is initialized and keep it alive until before Vulkan device
is destroyed.
*/
DMA_DEFINE_HANDLE(DmaAllocator)

/// Callback function called after successful vkAllocateMemory.
typedef void (*PFN_dmaAllocateDeviceMemoryFunction)(DmaAllocator allocator,
                                                    uint32_t memoryType,
                                                    DkMemBlock memory,
                                                    uint32_t size);
/// Callback function called before vkFreeMemory.
typedef void (*PFN_dmaFreeDeviceMemoryFunction)(DmaAllocator allocator,
                                                uint32_t memoryType,
                                                DkMemBlock memory,
                                                uint32_t size);

/** \brief Set of callbacks that the library will call for `vkAllocateMemory`
and `vkFreeMemory`.

Provided for informative purpose, e.g. to gather statistics about number of
allocations or total amount of memory allocated in Vulkan.

Used in DmaAllocatorCreateInfo::pDeviceMemoryCallbacks.
*/
typedef struct DmaDeviceMemoryCallbacks {
  /// Optional, can be null.
  PFN_dmaAllocateDeviceMemoryFunction pfnAllocate;
  /// Optional, can be null.
  PFN_dmaFreeDeviceMemoryFunction pfnFree;
} DmaDeviceMemoryCallbacks;

/// Flags for created #DmaAllocator.
typedef enum DmaAllocatorCreateFlagBits {
  /** \brief Allocator and all objects created from it will not be synchronized
  internally, so you must guarantee they are used from only one thread at a time
  or synchronized externally by you.

  Using this flag may increase performance because internal mutexes are not
  used.
  */
  DMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT = 0x00000001,
  /** \brief Enables usage of VK_KHR_dedicated_allocation extension.

  The flag works only if DmaAllocatorCreateInfo::vulkanApiVersion `==
  VK_API_VERSION_1_0`. When it's `VK_API_VERSION_1_1`, the flag is ignored
  because the extension has been promoted to Vulkan 1.1.

  Using this extenion will automatically allocate dedicated blocks of memory for
  some buffers and images instead of suballocating place for them out of bigger
  memory blocks (as if you explicitly used
  #DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT flag) when it is recommended by
  the driver. It may improve performance on some GPUs.

  You may set this flag only if you found out that following device extensions
  are supported, you enabled them while creating Vulkan device passed as
  DmaAllocatorCreateInfo::device, and you want them to be used internally by
  this library:

  - VK_KHR_get_memory_requirements2 (device extension)
  - VK_KHR_dedicated_allocation (device extension)

  When this flag is set, you can experience following warnings reported by
  Vulkan validation layer. You can ignore them.

  > vkBindBufferMemory(): Binding memory to buffer 0x2d but
  vkGetBufferMemoryRequirements() has not been called on that buffer.
  */
  DMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT = 0x00000002,
  /**
  Enables usage of VK_KHR_bind_memory2 extension.

  The flag works only if DmaAllocatorCreateInfo::vulkanApiVersion `==
  VK_API_VERSION_1_0`. When it's `VK_API_VERSION_1_1`, the flag is ignored
  because the extension has been promoted to Vulkan 1.1.

  You may set this flag only if you found out that this device extension is
  supported, you enabled it while creating Vulkan device passed as
  DmaAllocatorCreateInfo::device, and you want it to be used internally by this
  library.

  The extension provides functions `vkBindBufferMemory2KHR` and
  `vkBindImageMemory2KHR`, which allow to pass a chain of `pNext` structures
  while binding. This flag is required if you use `pNext` parameter in
  dmaBindBufferMemory2() or dmaBindImageMemory2().
  */
  DMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT = 0x00000004,
  /**
  Enables usage of VK_EXT_memory_budget extension.

  You may set this flag only if you found out that this device extension is
  supported, you enabled it while creating Vulkan device passed as
  DmaAllocatorCreateInfo::device, and you want it to be used internally by this
  library, along with another instance extension
  VK_KHR_get_physical_device_properties2, which is required by it (or
  Vulkan 1.1, where this extension is promoted).

  The extension provides query for current memory usage and budget, which will
  probably be more accurate than an estimation used by the library otherwise.
  */
  DMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT = 0x00000008,

  DMA_ALLOCATOR_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} DmaAllocatorCreateFlagBits;
typedef uint32_t DmaAllocatorCreateFlags;

/// Flags to be used in DmaRecordSettings::flags.
typedef enum DmaRecordFlagBits {
  /** \brief Enables flush after recording every function call.

  Enable it if you expect your application to crash, which may leave recording
  file truncated. It may degrade performance though.
  */
  DMA_RECORD_FLUSH_AFTER_CALL_BIT = 0x00000001,

  DMA_RECORD_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} DmaRecordFlagBits;
typedef uint32_t DmaRecordFlags;

/// Parameters for recording calls to VMA functions. To be used in
/// DmaAllocatorCreateInfo::pRecordSettings.
typedef struct DmaRecordSettings {
  /// Flags for recording. Use #DmaRecordFlagBits enum.
  DmaRecordFlags flags;
  /** \brief Path to the file that should be written by the recording.

  Suggested extension: "csv".
  If the file already exists, it will be overwritten.
  It will be opened for the whole time #DmaAllocator object is alive.
  If opening this file fails, creation of the whole allocator object fails.
  */
  const char *pFilePath;
} DmaRecordSettings;

/// Description of a Allocator to be created.
typedef struct DmaAllocatorCreateInfo {
  /// Flags for created allocator. Use #DmaAllocatorCreateFlagBits enum.
  DmaAllocatorCreateFlags flags;
  /// Vulkan device.
  /** It must be valid throughout whole lifetime of created allocator. */
  DkDevice device;
  /// Preferred size of a single `DkMemBlock` block to be allocated from
  /// large heaps > 1 GiB. Optional.
  /** Set to 0 to use default, which is currently 256 MiB. */
  uint32_t preferredLargeHeapBlockSize;
  /// Custom CPU memory allocation callbacks. Optional.
  /** Optional, can be null. When specified, will also be used for all CPU-side
   * memory allocations. */
  const DmaAllocationCallbacks *pAllocationCallbacks;
  /// Informative callbacks for `vkAllocateMemory`, `vkFreeMemory`. Optional.
  /** Optional, can be null. */
  const DmaDeviceMemoryCallbacks *pDeviceMemoryCallbacks;
  /** \brief Maximum number of additional frames that are in use at the same
  time as current frame.

  This value is used only when you make allocations with
  DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT flag. Such allocation cannot become
  lost if allocation.lastUseFrameIndex >= allocator.currentFrameIndex -
  frameInUseCount.

  For example, if you double-buffer your command buffers, so resources used for
  rendering in previous frame may still be in use by the GPU at the moment you
  allocate resources needed for the current frame, set this value to 1.

  If you want to allow any allocations other than used in the current frame to
  become lost, set this value to 0.
  */
  uint32_t frameInUseCount;
  /** \brief Either null or a pointer to an array of limits on maximum number of
  bytes that can be allocated out of particular Vulkan memory heap.

  If not NULL, it must be a pointer to an array of
  `VkPhysicalDeviceMemoryProperties::memoryHeapCount` elements, defining limit
  on maximum number of bytes that can be allocated out of particular Vulkan
  memory heap.

  Any of the elements may be equal to `DMA_WHOLE_SIZE`, which means no limit on
  that heap. This is also the default in case of `pHeapSizeLimit` = NULL.

  If there is a limit defined for a heap:

  - If user tries to allocate more memory from that heap using this allocator,
    the allocation fails with `DkResult_OutOfMemory`.
  - If the limit is smaller than heap size reported in `VkMemoryHeap::size`, the
    value of this limit will be reported instead when using
  dmaGetMemoryProperties().

  Warning! Using this feature may not be equivalent to installing a GPU with
  smaller amount of memory, because graphics driver doesn't necessary fail new
  allocations with `DkResult_OutOfMemory` result when memory capacity
  is exceeded. It may return success and just silently migrate some device
  memory blocks to system RAM. This driver behavior can also be controlled using
  VK_AMD_memory_overallocation_behavior extension.
  */
  const uint32_t *pHeapSizeLimit;
  /** \brief Parameters for recording of VMA calls. Can be null.

  If not null, it enables recording of calls to VMA functions to a file.
  If support for recording is not enabled using `DMA_RECORDING_ENABLED` macro,
  creation of the allocator object fails with `DkResult_NotImplemented`.
  */
  const DmaRecordSettings *pRecordSettings;
} DmaAllocatorCreateInfo;

/// Creates Allocator object.
DMA_CALL_PRE DkResult DMA_CALL_POST dmaCreateAllocator(
    const DmaAllocatorCreateInfo *pCreateInfo, DmaAllocator *pAllocator);

/// Destroys allocator object.
DMA_CALL_PRE void DMA_CALL_POST dmaDestroyAllocator(DmaAllocator allocator);

/** \brief Sets index of the current frame.

This function must be used if you make allocations with
#DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT and
#DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT flags to inform the allocator
when a new frame begins. Allocations queried using dmaGetAllocationInfo() cannot
become lost in the current frame.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaSetCurrentFrameIndex(DmaAllocator allocator,
                                                        uint32_t frameIndex);

/** \brief Calculated statistics of memory usage in entire allocator.
 */
typedef struct DmaStatInfo {
  /// Number of `DkMemBlock` Vulkan memory blocks allocated.
  uint32_t blockCount;
  /// Number of #DmaAllocation allocation objects allocated.
  uint32_t allocationCount;
  /// Number of free ranges of memory between allocations.
  uint32_t unusedRangeCount;
  /// Total number of bytes occupied by all allocations.
  uint32_t usedBytes;
  /// Total number of bytes occupied by unused ranges.
  uint32_t unusedBytes;
  uint32_t allocationSizeMin, allocationSizeAvg, allocationSizeMax;
  uint32_t unusedRangeSizeMin, unusedRangeSizeAvg, unusedRangeSizeMax;
} DmaStatInfo;

/// General statistics from current state of Allocator.
typedef struct DmaStats {
  DmaStatInfo memoryType[DMA_NUM_MEMORY_TYPES];
  DmaStatInfo memoryHeap[DMA_NUM_MEMORY_HEAPS];
  DmaStatInfo total;
} DmaStats;

/** \brief Retrieves statistics from current state of the Allocator.

This function is called "calculate" not "get" because it has to traverse all
internal data structures, so it may be quite slow. For faster but more brief
statistics suitable to be called every frame or every allocation, use
dmaGetBudget().

Note that when using allocator from multiple threads, returned information may
immediately become outdated.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaCalculateStats(DmaAllocator allocator,
                                                  DmaStats *pStats);

/** \brief Statistics of current memory usage and available budget, in bytes,
 * for specific memory heap.
 */
typedef struct DmaBudget {
  /** \brief Sum size of all `DkMemBlock` blocks allocated from particular
   * heap, in bytes.
   */
  uint32_t blockBytes;

  /** \brief Sum size of all allocations created in particular heap, in bytes.

  Usually less or equal than `blockBytes`.
  Difference `blockBytes - allocationBytes` is the amount of memory allocated
  but unused - available for new allocations or wasted due to fragmentation.

  It might be greater than `blockBytes` if there are some allocations in lost
  state, as they account to this value as well.
  */
  uint32_t allocationBytes;

  /** \brief Estimated current memory usage of the program, in bytes.

  Fetched from system using `VK_EXT_memory_budget` extension if enabled.

  It might be different than `blockBytes` (usually higher) due to additional
  implicit objects also occupying the memory, like swapchain, pipelines,
  descriptor heaps, command buffers, or `DkMemBlock` blocks allocated
  outside of this library, if any.
  */
  uint32_t usage;

  /** \brief Estimated amount of memory available to the program, in bytes.

  Fetched from system using `VK_EXT_memory_budget` extension if enabled.

  It might be different (most probably smaller) than
  `VkMemoryHeap::size[heapIndex]` due to factors external to the program, like
  other programs also consuming system resources. Difference `budget - usage` is
  the amount of additional memory that can probably be allocated without
  problems. Exceeding the budget may result in various problems.
  */
  uint32_t budget;
} DmaBudget;

/** \brief Retrieves information about current memory budget for all memory
heaps.

\param[out] pBudget Must point to array with number of elements at least equal
to number of memory heaps in physical device used.

This function is called "get" not "calculate" because it is very fast, suitable
to be called every frame or every allocation. For more detailed statistics use
dmaCalculateStats().

Note that when using allocator from multiple threads, returned information may
immediately become outdated.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaGetBudget(DmaAllocator allocator,
                                             DmaBudget *pBudget);

#ifndef DMA_STATS_STRING_ENABLED
#define DMA_STATS_STRING_ENABLED 1
#endif

#if DMA_STATS_STRING_ENABLED

/// Builds and returns statistics as string in JSON format.
/** @param[out] ppStatsString Must be freed using dmaFreeStatsString() function.
 */
DMA_CALL_PRE void DMA_CALL_POST dmaBuildStatsString(DmaAllocator allocator,
                                                    char **ppStatsString,
                                                    bool detailedMap);

DMA_CALL_PRE void DMA_CALL_POST dmaFreeStatsString(DmaAllocator allocator,
                                                   char *pStatsString);

#endif // #if DMA_STATS_STRING_ENABLED

/** \struct DmaPool
\brief Represents custom memory pool

Fill structure DmaPoolCreateInfo and call function dmaCreatePool() to create it.
Call function dmaDestroyPool() to destroy it.

For more information see [Custom memory pools](@ref
choosing_memory_type_custom_memory_pools).
*/
DMA_DEFINE_HANDLE(DmaPool)

/// Flags to be passed as DmaAllocationCreateInfo::flags.
typedef enum DmaAllocationCreateFlagBits {
  /** \brief Set this flag if the allocation should have its own memory block.

  Use it for special, big resources, like fullscreen images used as attachments.

  You should not use this flag if DmaAllocationCreateInfo::pool is not null.
  */
  DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT = 0x00000001,

  /** \brief Set this flag to only try to allocate from existing
  `DkMemBlock` blocks and never create new such block.

  If new allocation cannot be placed in any of the existing blocks, allocation
  fails with `DkResult_OutOfMemory` error.

  You should not use #DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT and
  #DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT at the same time. It makes no sense.

  If DmaAllocationCreateInfo::pool is not null, this flag is implied and
  ignored. */
  DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT = 0x00000002,
  /** \brief Set this flag to use a memory that will be persistently mapped and
  retrieve pointer to it.

  Pointer to mapped memory will be returned through
  DmaAllocationInfo::pMappedData.

  Is it valid to use this flag for allocation made from memory type that is not
  `HOST_VISIBLE`. This flag is then ignored and memory is not mapped. This is
  useful if you need an allocation that is efficient to use on GPU
  (`DEVICE_LOCAL`) and still want to map it directly if possible on platforms
  that support it (e.g. Intel GPU).

  You should not use this flag together with
  #DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT.
  */
  DMA_ALLOCATION_CREATE_MAPPED_BIT = 0x00000004,
  /** Allocation created with this flag can become lost as a result of another
  allocation with #DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT flag, so you
  must check it before use.

  To check if allocation is not lost, call dmaGetAllocationInfo() and check if
  DmaAllocationInfo::deviceMemory is not `DMA_NULL_HANDLE`.

  For details about supporting lost allocations, see Lost Allocations
  chapter of User Guide on Main Page.

  You should not use this flag together with #DMA_ALLOCATION_CREATE_MAPPED_BIT.
  */
  DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT = 0x00000008,
  /** While creating allocation using this flag, other allocations that were
  created with flag #DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT can become lost.

  For details about supporting lost allocations, see Lost Allocations
  chapter of User Guide on Main Page.
  */
  DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT = 0x00000010,
  /** Set this flag to treat DmaAllocationCreateInfo::pUserData as pointer to a
  null-terminated string. Instead of copying pointer value, a local copy of the
  string is made and stored in allocation's `pUserData`. The string is
  automatically freed together with the allocation. It is also used in
  dmaBuildStatsString().
  */
  DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT = 0x00000020,
  /** Allocation will be created from upper stack in a double stack pool.

  This flag is only allowed for custom pools created with
  #DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT flag.
  */
  DMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT = 0x00000040,
  /** Create both buffer/image and allocation, but don't bind them together.
  It is useful when you want to bind yourself to do some more advanced binding,
  e.g. using some extensions. The flag is meaningful only with functions that
  bind by default: dmaCreateBuffer(), dmaCreateImage(). Otherwise it is ignored.
  */
  DMA_ALLOCATION_CREATE_DONT_BIND_BIT = 0x00000080,
  /** Create allocation only if additional device memory required for it, if
  any, won't exceed memory budget. Otherwise return
  `DkResult_OutOfMemory`.
  */
  DMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT = 0x00000100,

  /** Allocation strategy that chooses smallest possible free range for the
  allocation.
  */
  DMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT = 0x00010000,
  /** Allocation strategy that chooses biggest possible free range for the
  allocation.
  */
  DMA_ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT = 0x00020000,
  /** Allocation strategy that chooses first suitable free range for the
  allocation.

  "First" doesn't necessarily means the one with smallest offset in memory,
  but rather the one that is easiest and fastest to find.
  */
  DMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT = 0x00040000,

  /** Allocation strategy that tries to minimize memory usage.
   */
  DMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT =
      DMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT,
  /** Allocation strategy that tries to minimize allocation time.
   */
  DMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT =
      DMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT,
  /** Allocation strategy that tries to minimize memory fragmentation.
   */
  DMA_ALLOCATION_CREATE_STRATEGY_MIN_FRAGMENTATION_BIT =
      DMA_ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT,

  /** A bit mask to extract only `STRATEGY` bits from entire set of flags.
   */
  DMA_ALLOCATION_CREATE_STRATEGY_MASK =
      DMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT |
      DMA_ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT |
      DMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT,

  DMA_ALLOCATION_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} DmaAllocationCreateFlagBits;
typedef uint32_t DmaAllocationCreateFlags;

typedef struct DmaAllocationCreateInfo {
  /// Use #DmaAllocationCreateFlagBits enum.
  DmaAllocationCreateFlags flags;
  /** \brief Pool that this allocation should be created in.

  Leave `DMA_NULL_HANDLE` to allocate from default pool. If not null, members:
  `usage`, `requiredFlags`, `preferredFlags`, `memoryTypeBits` are ignored.
  */
  DmaPool pool;
  /** \brief Custom general-purpose pointer that will be stored in
  #DmaAllocation, can be read as DmaAllocationInfo::pUserData and changed using
  dmaSetAllocationUserData().

  If #DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT is used, it must be either
  null or pointer to a null-terminated string. The string will be then copied to
  internal buffer, so it doesn't need to be valid after allocation call.
  */
  void *pUserData;
} DmaAllocationCreateInfo;

/// Flags to be passed as DmaPoolCreateInfo::flags.
typedef enum DmaPoolCreateFlagBits {
  /** \brief Use this flag if you always allocate only buffers and linear images
  or only optimal images out of this pool and so Buffer-Image Granularity can be
  ignored.

  This is an optional optimization flag.

  If you always allocate using dmaCreateBuffer(), dmaCreateImage(),
  dmaAllocateMemoryForBuffer(), then you don't need to use it because allocator
  knows exact type of your allocations so it can handle Buffer-Image Granularity
  in the optimal way.

  If you also allocate using dmaAllocateMemoryForImage() or dmaAllocateMemory(),
  exact type of such allocations is not known, so allocator must be conservative
  in handling Buffer-Image Granularity, which can lead to suboptimal allocation
  (wasted memory). In that case, if you can make sure you always allocate only
  buffers and linear images or only optimal images out of this pool, use this
  flag to make allocator disregard Buffer-Image Granularity and so make
  allocations faster and more optimal.
  */
  DMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT = 0x00000002,

  /** \brief Enables alternative, linear allocation algorithm in this pool.

  Specify this flag to enable linear allocation algorithm, which always creates
  new allocations after last one and doesn't reuse space from allocations freed
  in between. It trades memory consumption for simplified algorithm and data
  structure, which has better performance and uses less memory for metadata.

  By using this flag, you can achieve behavior of free-at-once, stack,
  ring buffer, and double stack. For details, see documentation chapter
  \ref linear_algorithm.

  When using this flag, you must specify DmaPoolCreateInfo::maxBlockCount == 1
  (or 0 for default).

  For more details, see [Linear allocation algorithm](@ref linear_algorithm).
  */
  DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT = 0x00000004,

  /** \brief Enables alternative, buddy allocation algorithm in this pool.

  It operates on a tree of blocks, each having size that is a power of two and
  a half of its parent's size. Comparing to default algorithm, this one provides
  faster allocation and deallocation and decreased external fragmentation,
  at the expense of more memory wasted (internal fragmentation).

  For more details, see [Buddy allocation algorithm](@ref buddy_algorithm).
  */
  DMA_POOL_CREATE_BUDDY_ALGORITHM_BIT = 0x00000008,

  /** Bit mask to extract only `ALGORITHM` bits from entire set of flags.
   */
  DMA_POOL_CREATE_ALGORITHM_MASK = DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT |
                                   DMA_POOL_CREATE_BUDDY_ALGORITHM_BIT,

  DMA_POOL_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} DmaPoolCreateFlagBits;
typedef uint32_t DmaPoolCreateFlags;

/** \brief Describes parameter of created #DmaPool.
 */
typedef struct DmaPoolCreateInfo {
  /** \brief Vulkan memory type index to allocate this pool from.
   */
  uint32_t memoryTypeIndex;
  /** \brief Use combination of #DmaPoolCreateFlagBits.
   */
  DmaPoolCreateFlags flags;
  /** \brief Size of a single `DkMemBlock` block to be allocated as part of
  this pool, in bytes. Optional.

  Specify nonzero to set explicit, constant size of memory blocks used by this
  pool.

  Leave 0 to use default and let the library manage block sizes automatically.
  Sizes of particular blocks may vary.
  */
  uint32_t blockSize;
  /** \brief Minimum number of blocks to be always allocated in this pool, even
  if they stay empty.

  Set to 0 to have no preallocated blocks and allow the pool be completely
  empty.
  */
  size_t minBlockCount;
  /** \brief Maximum number of blocks that can be allocated in this pool.
  Optional.

  Set to 0 to use default, which is `SIZE_MAX`, which means no limit.

  Set to same value as DmaPoolCreateInfo::minBlockCount to have fixed amount of
  memory allocated throughout whole lifetime of this pool.
  */
  size_t maxBlockCount;
  /** \brief Maximum number of additional frames that are in use at the same
  time as current frame.

  This value is used only when you make allocations with
  #DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT flag. Such allocation cannot become
  lost if allocation.lastUseFrameIndex >= allocator.currentFrameIndex -
  frameInUseCount.

  For example, if you double-buffer your command buffers, so resources used for
  rendering in previous frame may still be in use by the GPU at the moment you
  allocate resources needed for the current frame, set this value to 1.

  If you want to allow any allocations other than used in the current frame to
  become lost, set this value to 0.
  */
  uint32_t frameInUseCount;
} DmaPoolCreateInfo;

/** \brief Describes parameter of existing #DmaPool.
 */
typedef struct DmaPoolStats {
  /** \brief Total amount of `DkMemBlock` allocated from Vulkan for this
   * pool, in bytes.
   */
  uint32_t size;
  /** \brief Total number of bytes in the pool not used by any #DmaAllocation.
   */
  uint32_t unusedSize;
  /** \brief Number of #DmaAllocation objects created from this pool that were
   * not destroyed or lost.
   */
  size_t allocationCount;
  /** \brief Number of continuous memory ranges in the pool not used by any
   * #DmaAllocation.
   */
  size_t unusedRangeCount;
  /** \brief Size of the largest continuous free memory region available for new
  allocation.

  Making a new allocation of that size is not guaranteed to succeed because of
  possible additional margin required to respect alignment and buffer/image
  granularity.
  */
  uint32_t unusedRangeSizeMax;
  /** \brief Number of `DkMemBlock` blocks allocated for this pool.
   */
  size_t blockCount;
} DmaPoolStats;

/** \brief Allocates Vulkan device memory and creates #DmaPool object.

@param allocator Allocator object.
@param pCreateInfo Parameters of pool to create.
@param[out] pPool Handle to created pool.
*/
DMA_CALL_PRE DkResult DMA_CALL_POST
dmaCreatePool(DmaAllocator allocator, const DmaPoolCreateInfo *pCreateInfo,
              DmaPool *pPool);

/** \brief Destroys #DmaPool object and frees Vulkan device memory.
 */
DMA_CALL_PRE void DMA_CALL_POST dmaDestroyPool(DmaAllocator allocator,
                                               DmaPool pool);

/** \brief Retrieves statistics of existing #DmaPool object.

@param allocator Allocator object.
@param pool Pool object.
@param[out] pPoolStats Statistics of specified pool.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaGetPoolStats(DmaAllocator allocator,
                                                DmaPool pool,
                                                DmaPoolStats *pPoolStats);

/** \brief Marks all allocations in given pool as lost if they are not used in
current frame or DmaPoolCreateInfo::frameInUseCount back from now.

@param allocator Allocator object.
@param pool Pool.
@param[out] pLostAllocationCount Number of allocations marked as lost. Optional
- pass null if you don't need this information.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaMakePoolAllocationsLost(
    DmaAllocator allocator, DmaPool pool, size_t *pLostAllocationCount);

/** \brief Checks magic number in margins around all allocations in given memory
pool in search for corruptions.

Corruption detection is enabled only when `DMA_DEBUG_DETECT_CORRUPTION` macro is
defined to nonzero, `DMA_DEBUG_MARGIN` is defined to nonzero and the pool is
created in memory type that is `HOST_VISIBLE` and `HOST_COHERENT`. For more
information, see [Corruption detection](@ref
debugging_memory_usage_corruption_detection).

Possible return values:

- `DkResult_NotImplemented` - corruption detection is not enabled for
specified pool.
- `DkResult_Success` - corruption detection has been performed and succeeded.
- `VK_ERROR_VALIDATION_FAILED_EXT` - corruption detection has been performed and
found memory corruptions around one of the allocations. `DMA_ASSERT` is also
fired in that case.
- Other value: Error returned by Vulkan, e.g. memory mapping failure.
*/
DMA_CALL_PRE DkResult DMA_CALL_POST
dmaCheckPoolCorruption(DmaAllocator allocator, DmaPool pool);

/** \brief Retrieves name of a custom pool.

After the call `ppName` is either null or points to an internally-owned
null-terminated string containing name of the pool that was previously set. The
pointer becomes invalid when the pool is destroyed or its name is changed using
dmaSetPoolName().
*/
DMA_CALL_PRE void DMA_CALL_POST dmaGetPoolName(DmaAllocator allocator,
                                               DmaPool pool,
                                               const char **ppName);

/** \brief Sets name of a custom pool.

`pName` can be either null or pointer to a null-terminated string with new name
for the pool. Function makes internal copy of the string, so it can be changed
or freed immediately after this call.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaSetPoolName(DmaAllocator allocator,
                                               DmaPool pool, const char *pName);

/** \struct VmaAllocation
\brief Represents single memory allocation.

It may be either dedicated block of `VkDeviceMemory` or a specific region of a
bigger block of this type plus unique offset.

There are multiple ways to create such object.
You need to fill structure VmaAllocationCreateInfo.
For more information see [Choosing memory type](@ref choosing_memory_type).

Although the library provides convenience functions that create Vulkan buffer or
image, allocate memory for it and bind them together, binding of the allocation
to a buffer or an image is out of scope of the allocation itself. Allocation
object can exist without buffer/image bound, binding can be done manually by the
user, and destruction of it can be done independently of destruction of the
allocation.

The object also remembers its size and some other information.
To retrieve this information, use function vmaGetAllocationInfo() and inspect
returned structure VmaAllocationInfo.

Some kinds allocations can be in lost state.
For more information, see [Lost allocations](@ref lost_allocations).
*/
DMA_DEFINE_HANDLE(DmaAllocation)

/** \brief Parameters of #DmaAllocation objects, that can be retrieved using
 * function dmaGetAllocationInfo().
 */
typedef struct DmaAllocationInfo {
  /** \brief Memory type index that this allocation was allocated from.

  It never changes.
  */
  uint32_t memoryType;
  /** \brief Handle to Vulkan memory object.

  Same memory object can be shared by multiple allocations.

  It can change after call to dmaDefragment() if this allocation is passed to
  the function, or if allocation is lost.

  If the allocation is lost, it is equal to `DMA_NULL_HANDLE`.
  */
  DkMemBlock deviceMemory;
  /** \brief Offset into deviceMemory object to the beginning of this
  allocation, in bytes. (deviceMemory, offset) pair is unique to this
  allocation.

  It can change after call to dmaDefragment() if this allocation is passed to
  the function, or if allocation is lost.
  */
  uint32_t offset;
  /** \brief Size of this allocation, in bytes.

  It never changes, unless allocation is lost.
  */
  uint32_t size;
  /** \brief Pointer to the beginning of this allocation as mapped data.

  If the allocation hasn't been mapped using dmaMapMemory() and hasn't been
  created with #DMA_ALLOCATION_CREATE_MAPPED_BIT flag, this value null.

  It can change after call to dmaMapMemory(), dmaUnmapMemory().
  It can also change after call to dmaDefragment() if this allocation is passed
  to the function.
  */
  void *pMappedData;
  /** \brief Custom general-purpose pointer that was passed as
  DmaAllocationCreateInfo::pUserData or set using dmaSetAllocationUserData().

  It can change after call to dmaSetAllocationUserData() for this allocation.
  */
  void *pUserData;
} DmaAllocationInfo;

/** \brief General purpose memory allocation.

@param[out] pAllocation Handle to allocated memory.
@param[out] pAllocationInfo Optional. Information about allocated memory. It can
be later fetched using function dmaGetAllocationInfo().

You should free the memory using dmaFreeMemory() or dmaFreeMemoryPages().

It is recommended to use dmaAllocateMemoryForBuffer(),
dmaAllocateMemoryForImage(), dmaCreateBuffer(), dmaCreateImage() instead
whenever possible.
*/
DMA_CALL_PRE DkResult DMA_CALL_POST dmaAllocateMemory(
    DmaAllocator allocator, const DmaMemoryRequirements *pDmaMemoryRequirements,
    const DmaAllocationCreateInfo *pCreateInfo, DmaAllocation *pAllocation,
    DmaAllocationInfo *pAllocationInfo);

/** \brief General purpose memory allocation for multiple allocation objects at
once.

@param allocator Allocator object.
@param pDmaMemoryRequirements Memory requirements for each allocation.
@param pCreateInfo Creation parameters for each alloction.
@param allocationCount Number of allocations to make.
@param[out] pAllocations Pointer to array that will be filled with handles to
created allocations.
@param[out] pAllocationInfo Optional. Pointer to array that will be filled with
parameters of created allocations.

You should free the memory using dmaFreeMemory() or dmaFreeMemoryPages().

Word "pages" is just a suggestion to use this function to allocate pieces of
memory needed for sparse binding. It is just a general purpose allocation
function able to make multiple allocations at once. It may be internally
optimized to be more efficient than calling dmaAllocateMemory()
`allocationCount` times.

All allocations are made using same parameters. All of them are created out of
the same memory pool and type. If any allocation fails, all allocations already
made within this function call are also freed, so that when returned result is
not `DkResult_Success`, `pAllocation` array is always entirely filled with
`DMA_NULL_HANDLE`.
*/
DMA_CALL_PRE DkResult DMA_CALL_POST dmaAllocateMemoryPages(
    DmaAllocator allocator, const DmaMemoryRequirements *pDmaMemoryRequirements,
    const DmaAllocationCreateInfo *pCreateInfo, size_t allocationCount,
    DmaAllocation *pAllocations, DmaAllocationInfo *pAllocationInfo);

/** \brief Frees memory previously allocated using dmaAllocateMemory(),
dmaAllocateMemoryForBuffer(), or dmaAllocateMemoryForImage().

Passing `DMA_NULL_HANDLE` as `allocation` is valid. Such function call is just
skipped.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaFreeMemory(DmaAllocator allocator,
                                              DmaAllocation allocation);

/** \brief Frees memory and destroys multiple allocations.

Word "pages" is just a suggestion to use this function to free pieces of memory
used for sparse binding. It is just a general purpose function to free memory
and destroy allocations made using e.g. dmaAllocateMemory(),
dmaAllocateMemoryPages() and other functions.
It may be internally optimized to be more efficient than calling dmaFreeMemory()
`allocationCount` times.

Allocations in `pAllocations` array can come from any memory pools and types.
Passing `DMA_NULL_HANDLE` as elements of `pAllocations` array is valid. Such
entries are just skipped.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaFreeMemoryPages(DmaAllocator allocator,
                                                   size_t allocationCount,
                                                   DmaAllocation *pAllocations);

/** \brief Deprecated.

In version 2.2.0 it used to try to change allocation's size without moving or
reallocating it. In current version it returns `DkResult_Success` only if
`newSize` equals current allocation's size. Otherwise returns
`VK_ERROR_OUT_OF_POOL_MEMORY`, indicating that allocation's size could not be
changed.
*/
DMA_CALL_PRE DkResult DMA_CALL_POST dmaResizeAllocation(
    DmaAllocator allocator, DmaAllocation allocation, uint32_t newSize);

/** \brief Returns current information about specified allocation and atomically
marks it as used in current frame.

Current paramters of given allocation are returned in `pAllocationInfo`.

This function also atomically "touches" allocation - marks it as used in current
frame, just like dmaTouchAllocation(). If the allocation is in lost state,
`pAllocationInfo->deviceMemory == DMA_NULL_HANDLE`.

Although this function uses atomics and doesn't lock any mutex, so it should be
quite efficient, you can avoid calling it too often.

- You can retrieve same DmaAllocationInfo structure while creating your
resource, from function dmaCreateBuffer(), dmaCreateImage(). You can remember it
if you are sure parameters don't change (e.g. due to defragmentation or
allocation becoming lost).
- If you just want to check if allocation is not lost, dmaTouchAllocation() will
work faster.
*/
DMA_CALL_PRE void DMA_CALL_POST
dmaGetAllocationInfo(DmaAllocator allocator, DmaAllocation allocation,
                     DmaAllocationInfo *pAllocationInfo);

/** \brief Returns `true` if allocation is not lost and atomically marks it
as used in current frame.

If the allocation has been created with
#DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT flag, this function returns `true`
if it's not in lost state, so it can still be used. It then also atomically
"touches" the allocation - marks it as used in current frame, so that you can be
sure it won't become lost in current frame or next `frameInUseCount` frames.

If the allocation is in lost state, the function returns `false`.
Memory of such allocation, as well as buffer or image bound to it, should not be
used. Lost allocation and the buffer/image still need to be destroyed.

If the allocation has been created without
#DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT flag, this function always returns
`true`.
*/
DMA_CALL_PRE bool DMA_CALL_POST dmaTouchAllocation(DmaAllocator allocator,
                                                   DmaAllocation allocation);

/** \brief Sets pUserData in given allocation to new value.

If the allocation was created with
DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT, pUserData must be either null,
or pointer to a null-terminated string. The function makes local copy of the
string and sets it as allocation's `pUserData`. String passed as pUserData
doesn't need to be valid for whole lifetime of the allocation - you can free it
after this call. String previously pointed by allocation's pUserData is freed
from memory.

If the flag was not used, the value of pointer `pUserData` is just copied to
allocation's `pUserData`. It is opaque, so you can use it however you want -
e.g. as a pointer, ordinal number or some handle to you own data.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaSetAllocationUserData(
    DmaAllocator allocator, DmaAllocation allocation, void *pUserData);

/** \brief Creates new allocation that is in lost state from the beginning.

It can be useful if you need a dummy, non-null allocation.

You still need to destroy created object using dmaFreeMemory().

Returned allocation is not tied to any specific memory pool or memory type and
not bound to any image or buffer. It has size = 0. It cannot be turned into
a real, non-empty allocation.
*/
DMA_CALL_PRE void DMA_CALL_POST
dmaCreateLostAllocation(DmaAllocator allocator, DmaAllocation *pAllocation);

/** \brief Maps memory represented by given allocation and returns pointer to
it.

Maps memory represented by given allocation to make it accessible to CPU code.
When succeeded, `*ppData` contains pointer to first byte of this memory.
If the allocation is part of bigger `DkMemBlock` block, the pointer is
correctly offseted to the beginning of region assigned to this particular
allocation.

Mapping is internally reference-counted and synchronized, so despite raw Vulkan
function `vkMapMemory()` cannot be used to map same block of `DkMemBlock`
multiple times simultaneously, it is safe to call this function on allocations
assigned to the same memory block. Actual Vulkan memory will be mapped on first
mapping and unmapped on last unmapping.

If the function succeeded, you must call dmaUnmapMemory() to unmap the
allocation when mapping is no longer needed or before freeing the allocation, at
the latest.

It also safe to call this function multiple times on the same allocation. You
must call dmaUnmapMemory() same number of times as you called dmaMapMemory().

It is also safe to call this function on allocation created with
#DMA_ALLOCATION_CREATE_MAPPED_BIT flag. Its memory stays mapped all the time.
You must still call dmaUnmapMemory() same number of times as you called
dmaMapMemory(). You must not call dmaUnmapMemory() additional time to free the
"0-th" mapping made automatically due to #DMA_ALLOCATION_CREATE_MAPPED_BIT flag.

This function fails when used on allocation made in memory type that is not
`HOST_VISIBLE`.

This function always fails when called for allocation that was created with
#DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT flag. Such allocations cannot be
mapped.

This function doesn't automatically flush or invalidate caches.
If the allocation is made from a memory types that is not `HOST_COHERENT`,
you also need to use dmaInvalidateAllocation() / dmaFlushAllocation(), as
required by Vulkan specification.
*/
DMA_CALL_PRE DkResult DMA_CALL_POST dmaMapMemory(DmaAllocator allocator,
                                                 DmaAllocation allocation,
                                                 void **ppData);

/** \brief Unmaps memory represented by given allocation, mapped previously
using dmaMapMemory().

For details, see description of dmaMapMemory().

This function doesn't automatically flush or invalidate caches.
If the allocation is made from a memory types that is not `HOST_COHERENT`,
you also need to use dmaInvalidateAllocation() / dmaFlushAllocation(), as
required by Vulkan specification.
*/
DMA_CALL_PRE void DMA_CALL_POST dmaUnmapMemory(DmaAllocator allocator,
                                               DmaAllocation allocation);

/** \brief Flushes memory of given allocation.

Calls `vkFlushMappedMemoryRanges()` for memory associated with given range of
given allocation. It needs to be called after writing to a mapped memory for
memory types that are not `HOST_COHERENT`. Unmap operation doesn't do that
automatically.

- `offset` must be relative to the beginning of allocation.
- `size` can be `DMA_WHOLE_SIZE`. It means all memory from `offset` the the end
of given allocation.
- `offset` and `size` don't have to be aligned.
  They are internally rounded down/up to multiply of `nonCoherentAtomSize`.
- If `size` is 0, this call is ignored.
- If memory type that the `allocation` belongs to is not `HOST_VISIBLE` or it is
`HOST_COHERENT`, this call is ignored.

Warning! `offset` and `size` are relative to the contents of given `allocation`.
If you mean whole allocation, you can pass 0 and `DMA_WHOLE_SIZE`, respectively.
Do not pass allocation's offset as `offset`!!!
*/
DMA_CALL_PRE void DMA_CALL_POST dmaFlushAllocation(DmaAllocator allocator,
                                                   DmaAllocation allocation,
                                                   uint32_t offset,
                                                   uint32_t size);

/** \brief Invalidates memory of given allocation.

Calls `vkInvalidateMappedMemoryRanges()` for memory associated with given range
of given allocation. It needs to be called before reading from a mapped memory
for memory types that are not `HOST_COHERENT`. Map operation doesn't do that
automatically.

- `offset` must be relative to the beginning of allocation.
- `size` can be `DMA_WHOLE_SIZE`. It means all memory from `offset` the the end
of given allocation.
- `offset` and `size` don't have to be aligned.
  They are internally rounded down/up to multiply of `nonCoherentAtomSize`.
- If `size` is 0, this call is ignored.
- If memory type that the `allocation` belongs to is not `HOST_VISIBLE` or it is
`HOST_COHERENT`, this call is ignored.

Warning! `offset` and `size` are relative to the contents of given `allocation`.
If you mean whole allocation, you can pass 0 and `DMA_WHOLE_SIZE`, respectively.
Do not pass allocation's offset as `offset`!!!
*/
DMA_CALL_PRE void DMA_CALL_POST
dmaInvalidateAllocation(DmaAllocator allocator, DmaAllocation allocation,
                        uint32_t offset, uint32_t size);

/** \brief Checks magic number in margins around all allocations in given memory
types (in both default and custom pools) in search for corruptions.

@param memoryTypeBits Bit mask, where each bit set means that a memory type with
that index should be checked.

Corruption detection is enabled only when `DMA_DEBUG_DETECT_CORRUPTION` macro is
defined to nonzero, `DMA_DEBUG_MARGIN` is defined to nonzero and only for memory
types that are `HOST_VISIBLE` and `HOST_COHERENT`. For more information, see
[Corruption detection](@ref debugging_memory_usage_corruption_detection).

Possible return values:

- `DkResult_NotImplemented` - corruption detection is not enabled for any
of specified memory types.
- `DkResult_Success` - corruption detection has been performed and succeeded.
- `VK_ERROR_VALIDATION_FAILED_EXT` - corruption detection has been performed and
found memory corruptions around one of the allocations. `DMA_ASSERT` is also
fired in that case.
- Other value: Error returned by Vulkan, e.g. memory mapping failure.
*/
DMA_CALL_PRE DkResult DMA_CALL_POST dmaCheckCorruption(DmaAllocator allocator,
                                                       uint32_t memoryTypeBits);

/** \struct DmaDefragmentationContext
\brief Represents Opaque object that represents started defragmentation process.

Fill structure #DmaDefragmentationInfo2 and call function
dmaDefragmentationBegin() to create it. Call function dmaDefragmentationEnd() to
destroy it.
*/
DMA_DEFINE_HANDLE(DmaDefragmentationContext)

/// Flags to be used in dmaDefragmentationBegin(). None at the moment. Reserved
/// for future use.
typedef enum DmaDefragmentationFlagBits {
  DMA_DEFRAGMENTATION_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} DmaDefragmentationFlagBits;
typedef uint32_t DmaDefragmentationFlags;

/** \brief Parameters for defragmentation.

To be used with function dmaDefragmentationBegin().
*/
typedef struct DmaDefragmentationInfo2 {
  /** \brief Reserved for future use. Should be 0.
   */
  DmaDefragmentationFlags flags;
  /** \brief Number of allocations in `pAllocations` array.
   */
  uint32_t allocationCount;
  /** \brief Pointer to array of allocations that can be defragmented.

  The array should have `allocationCount` elements.
  The array should not contain nulls.
  Elements in the array should be unique - same allocation cannot occur twice.
  It is safe to pass allocations that are in the lost state - they are ignored.
  All allocations not present in this array are considered non-moveable during
  this defragmentation.
  */
  DmaAllocation *pAllocations;
  /** \brief Optional, output. Pointer to array that will be filled with
  information whether the allocation at certain index has been changed during
  defragmentation.

  The array should have `allocationCount` elements.
  You can pass null if you are not interested in this information.
  */
  bool *pAllocationsChanged;
  /** \brief Numer of pools in `pPools` array.
   */
  uint32_t poolCount;
  /** \brief Either null or pointer to array of pools to be defragmented.

  All the allocations in the specified pools can be moved during defragmentation
  and there is no way to check if they were really moved as in
  `pAllocationsChanged`, so you must query all the allocations in all these
  pools for new `DkMemBlock` and offset using dmaGetAllocationInfo() if you
  might need to recreate buffers and images bound to them.

  The array should have `poolCount` elements.
  The array should not contain nulls.
  Elements in the array should be unique - same pool cannot occur twice.

  Using this array is equivalent to specifying all allocations from the pools in
  `pAllocations`. It might be more efficient.
  */
  DmaPool *pPools;
  /** \brief Maximum total numbers of bytes that can be copied while moving
  allocations to different places using transfers on CPU side, like `memcpy()`,
  `memmove()`.

  `DMA_WHOLE_SIZE` means no limit.
  */
  uint32_t maxCpuBytesToMove;
  /** \brief Maximum number of allocations that can be moved to a different
  place using transfers on CPU side, like `memcpy()`, `memmove()`.

  `UINT32_MAX` means no limit.
  */
  uint32_t maxCpuAllocationsToMove;
  /** \brief Maximum total numbers of bytes that can be copied while moving
  allocations to different places using transfers on GPU side, posted to
  `commandBuffer`.

  `DMA_WHOLE_SIZE` means no limit.
  */
  uint32_t maxGpuBytesToMove;
  /** \brief Maximum number of allocations that can be moved to a different
  place using transfers on GPU side, posted to `commandBuffer`.

  `UINT32_MAX` means no limit.
  */
  uint32_t maxGpuAllocationsToMove;
  /** \brief Optional. Command buffer where GPU copy commands will be posted.

  If not null, it must be a valid command buffer handle that supports Transfer
  queue type. It must be in the recording state and outside of a render pass
  instance. You need to submit it and make sure it finished execution before
  calling dmaDefragmentationEnd().

  Passing null means that only CPU defragmentation will be performed.
  */
  DkCmdBuf commandBuffer;
} DmaDefragmentationInfo2;

/** \brief Deprecated. Optional configuration parameters to be passed to
function dmaDefragment().

\deprecated This is a part of the old interface. It is recommended to use
structure #DmaDefragmentationInfo2 and function dmaDefragmentationBegin()
instead.
*/
typedef struct DmaDefragmentationInfo {
  /** \brief Maximum total numbers of bytes that can be copied while moving
  allocations to different places.

  Default is `DMA_WHOLE_SIZE`, which means no limit.
  */
  uint32_t maxBytesToMove;
  /** \brief Maximum number of allocations that can be moved to different place.

  Default is `UINT32_MAX`, which means no limit.
  */
  uint32_t maxAllocationsToMove;
} DmaDefragmentationInfo;

/** \brief Statistics returned by function dmaDefragment(). */
typedef struct DmaDefragmentationStats {
  /// Total number of bytes that have been copied while moving allocations to
  /// different places.
  uint32_t bytesMoved;
  /// Total number of bytes that have been released to the system by freeing
  /// empty `DkMemBlock` objects.
  uint32_t bytesFreed;
  /// Number of allocations that have been moved to different places.
  uint32_t allocationsMoved;
  /// Number of empty `DkMemBlock` objects that have been released to the
  /// system.
  uint32_t deviceMemoryBlocksFreed;
} DmaDefragmentationStats;

/** \brief Begins defragmentation process.

@param allocator Allocator object.
@param pInfo Structure filled with parameters of defragmentation.
@param[out] pStats Optional. Statistics of defragmentation. You can pass null if
you are not interested in this information.
@param[out] pContext Context object that must be passed to
dmaDefragmentationEnd() to finish defragmentation.
@return `DkResult_Success` and `*pContext == null` if defragmentation finished
within this function call. `DkResult_Fail` and `*pContext != null` if
defragmentation has been started and you need to call dmaDefragmentationEnd() to
finish it. Negative value in case of error.

Use this function instead of old, deprecated dmaDefragment().

Warning! Between the call to dmaDefragmentationBegin() and
dmaDefragmentationEnd():

- You should not use any of allocations passed as `pInfo->pAllocations` or
  any allocations that belong to pools passed as `pInfo->pPools`,
  including calling dmaGetAllocationInfo(), dmaTouchAllocation(), or access
  their data.
- Some mutexes protecting internal data structures may be locked, so trying to
  make or free any allocations, bind buffers or images, map memory, or launch
  another simultaneous defragmentation in between may cause stall (when done on
  another thread) or deadlock (when done on the same thread), unless you are
  100% sure that defragmented allocations are in different pools.
- Information returned via `pStats` and `pInfo->pAllocationsChanged` are
undefined. They become valid after call to dmaDefragmentationEnd().
- If `pInfo->commandBuffer` is not null, you must submit that command buffer
  and make sure it finished execution before calling dmaDefragmentationEnd().

For more information and important limitations regarding defragmentation, see
documentation chapter: [Defragmentation](@ref defragmentation).
*/
DMA_CALL_PRE DkResult DMA_CALL_POST dmaDefragmentationBegin(
    DmaAllocator allocator, const DmaDefragmentationInfo2 *pInfo,
    DmaDefragmentationStats *pStats, DmaDefragmentationContext *pContext);

/** \brief Ends defragmentation process.

Use this function to finish defragmentation started by
dmaDefragmentationBegin(). It is safe to pass `context == null`. The function
then does nothing.
*/
DMA_CALL_PRE DkResult DMA_CALL_POST dmaDefragmentationEnd(
    DmaAllocator allocator, DmaDefragmentationContext context);

/** \brief Deprecated. Compacts memory by moving allocations.

@param pAllocations Array of allocations that can be moved during this
compation.
@param allocationCount Number of elements in pAllocations and
pAllocationsChanged arrays.
@param[out] pAllocationsChanged Array of boolean values that will indicate
whether matching allocation in pAllocations array has been moved. This parameter
is optional. Pass null if you don't need this information.
@param pDefragmentationInfo Configuration parameters. Optional - pass null to
use default values.
@param[out] pDefragmentationStats Statistics returned by the function. Optional
- pass null if you don't need this information.
@return `DkResult_Success` if completed, negative error code in case of error.

\deprecated This is a part of the old interface. It is recommended to use
structure #DmaDefragmentationInfo2 and function dmaDefragmentationBegin()
instead.

This function works by moving allocations to different places (different
`DkMemBlock` objects and/or different offsets) in order to optimize memory
usage. Only allocations that are in `pAllocations` array can be moved. All other
allocations are considered nonmovable in this call. Basic rules:

- Only allocations made in memory types that have
  `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` and
`VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` flags can be compacted. You may pass
other allocations but it makes no sense - these will never be moved.
- Custom pools created with #DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT or
  #DMA_POOL_CREATE_BUDDY_ALGORITHM_BIT flag are not defragmented. Allocations
  passed to this function that come from such pools are ignored.
- Allocations created with #DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT or
  created as dedicated allocations for any other reason are also ignored.
- Both allocations made with or without #DMA_ALLOCATION_CREATE_MAPPED_BIT
  flag can be compacted. If not persistently mapped, memory will be mapped
  temporarily inside this function if needed.
- You must not pass same #DmaAllocation object multiple times in `pAllocations`
array.

The function also frees empty `DkMemBlock` blocks.

Warning: This function may be time-consuming, so you shouldn't call it too often
(like after every resource creation/destruction).
You can call it on special occasions (like when reloading a game level or
when you just destroyed a lot of objects). Calling it every frame may be OK, but
you should measure that on your platform.

For more information, see [Defragmentation](@ref defragmentation) chapter.
*/
DMA_CALL_PRE DkResult DMA_CALL_POST
dmaDefragment(DmaAllocator allocator, DmaAllocation *pAllocations,
              size_t allocationCount, bool *pAllocationsChanged,
              const DmaDefragmentationInfo *pDefragmentationInfo,
              DmaDefragmentationStats *pDefragmentationStats);

#ifdef __cplusplus
}
#endif

#endif // AMD_DEKO_MEMORY_ALLOCATOR_H

// For Visual Studio IntelliSense.
#if defined(__cplusplus) && defined(__INTELLISENSE__)
#define DMA_IMPLEMENTATION
#endif

#ifdef DMA_IMPLEMENTATION
#undef DMA_IMPLEMENTATION

#include <cstdint>
#include <cstdlib>
#include <cstring>

/*******************************************************************************
CONFIGURATION SECTION

Define some of these macros before each #include of this header or change them
here if you need other then default behavior depending on your environment.
*/

// Define this macro to 1 to make the library use STL containers instead of its
// own implementation.
//#define DMA_USE_STL_CONTAINERS 1

/* Set this macro to 1 to make the library including and using STL containers:
std::pair, std::vector, std::list, std::unordered_map.

Set it to 0 or undefined to make the library using its own implementation of
the containers.
*/
#if DMA_USE_STL_CONTAINERS
#define DMA_USE_STL_VECTOR 1
#define DMA_USE_STL_UNORDERED_MAP 1
#define DMA_USE_STL_LIST 1
#endif

#ifndef DMA_USE_STL_SHARED_MUTEX
// Compiler conforms to C++17.
#if __cplusplus >= 201703L
#define DMA_USE_STL_SHARED_MUTEX 1
// Visual studio defines __cplusplus properly only when passed additional
// parameter: /Zc:__cplusplus Otherwise it's always 199711L, despite
// shared_mutex works since Visual Studio 2015 Update 2. See:
// https://blogs.msdn.microsoft.com/vcblog/2018/04/09/msvc-now-correctly-reports-__cplusplus/
#elif defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023918 &&                  \
    __cplusplus == 199711L && _MSVC_LANG >= 201703L
#define DMA_USE_STL_SHARED_MUTEX 1
#else
#define DMA_USE_STL_SHARED_MUTEX 0
#endif
#endif

/*
THESE INCLUDES ARE NOT ENABLED BY DEFAULT.
Library has its own container implementation.
*/
#if DMA_USE_STL_VECTOR
#include <vector>
#endif

#if DMA_USE_STL_UNORDERED_MAP
#include <unordered_map>
#endif

#if DMA_USE_STL_LIST
#include <list>
#endif

/*
Following headers are used in this CONFIGURATION section only, so feel free to
remove them if not needed.
*/
#include <algorithm> // for min, max
#include <cassert>   // for assert
#include <mutex>

#ifndef DMA_NULL
// Value used as null pointer. Define it to e.g.: nullptr, NULL, 0, (void*)0.
#define DMA_NULL nullptr
#endif

#if defined(__ANDROID_API__) && (__ANDROID_API__ < 16)
#include <cstdlib>
void *aligned_alloc(size_t alignment, size_t size) {
  // alignment must be >= sizeof(void*)
  if (alignment < sizeof(void *)) {
    alignment = sizeof(void *);
  }

  return memalign(alignment, size);
}
#elif defined(__APPLE__) || defined(__ANDROID__) ||                            \
    (defined(__linux__) && defined(__GLIBCXX__) &&                             \
     !defined(_GLIBCXX_HAVE_ALIGNED_ALLOC))
#include <cstdlib>
void *aligned_alloc(size_t alignment, size_t size) {
  // alignment must be >= sizeof(void*)
  if (alignment < sizeof(void *)) {
    alignment = sizeof(void *);
  }

  void *pointer;
  if (posix_memalign(&pointer, alignment, size) == 0)
    return pointer;
  return DMA_NULL;
}
#endif

// If your compiler is not compatible with C++11 and definition of
// aligned_alloc() function is missing, uncommeting following line may help:

//#include <malloc.h>

// Normal assert to check for programmer's errors, especially in Debug
// configuration.
#ifndef DMA_ASSERT
#ifndef NDEBUG
#define DMA_ASSERT(expr) assert(expr)
#else
#define DMA_ASSERT(expr)
#endif
#endif

// Assert that will be called very often, like inside data structures e.g.
// operator[]. Making it non-empty can make program slow.
#ifndef DMA_HEAVY_ASSERT
#ifndef NDEBUG
#define DMA_HEAVY_ASSERT(expr) // DMA_ASSERT(expr)
#else
#define DMA_HEAVY_ASSERT(expr)
#endif
#endif

#ifndef DMA_ALIGN_OF
#define DMA_ALIGN_OF(type) (__alignof(type))
#endif

void *dmaDefaultAllocFunc(size_t alignment, size_t size) {
  size = (size + alignment - 1) & ~(alignment - 1);
  return aligned_alloc(alignment, size);
}

#ifndef DMA_SYSTEM_ALIGNED_MALLOC
#define DMA_SYSTEM_ALIGNED_MALLOC(size, alignment)                             \
  (dmaDefaultAllocFunc((alignment), (size)))
#endif

#ifndef DMA_SYSTEM_FREE
#define DMA_SYSTEM_FREE(ptr) free(ptr)
#endif

#ifndef DMA_MIN
#define DMA_MIN(v1, v2) (std::min((v1), (v2)))
#endif

#ifndef DMA_MAX
#define DMA_MAX(v1, v2) (std::max((v1), (v2)))
#endif

#ifndef DMA_SWAP
#define DMA_SWAP(v1, v2) std::swap((v1), (v2))
#endif

#ifndef DMA_SORT
#define DMA_SORT(beg, end, cmp) std::sort(beg, end, cmp)
#endif

#ifndef DMA_DEBUG_LOG
#define DMA_DEBUG_LOG(format, ...)
/*
#define DMA_DEBUG_LOG(format, ...) do { \
    printf(format, __VA_ARGS__); \
    printf("\n"); \
} while(false)
*/
#endif

// Define this macro to 1 to enable functions: dmaBuildStatsString,
// dmaFreeStatsString.
#if DMA_STATS_STRING_ENABLED
static inline void DmaUint32ToStr(char *outStr, size_t strLen, uint32_t num) {
  snprintf(outStr, strLen, "%u", static_cast<unsigned int>(num));
}
static inline void DmaUint64ToStr(char *outStr, size_t strLen, uint64_t num) {
  snprintf(outStr, strLen, "%llu", static_cast<unsigned long long>(num));
}
static inline void DmaPtrToStr(char *outStr, size_t strLen, const void *ptr) {
  snprintf(outStr, strLen, "%p", ptr);
}
#endif

#ifndef DMA_MUTEX
class DmaMutex {
public:
  void Lock() { m_Mutex.lock(); }
  void Unlock() { m_Mutex.unlock(); }

private:
  std::mutex m_Mutex;
};
#define DMA_MUTEX DmaMutex
#endif

// Read-write mutex, where "read" is shared access, "write" is exclusive access.
#ifndef DMA_RW_MUTEX
#if DMA_USE_STL_SHARED_MUTEX
// Use std::shared_mutex from C++17.
#include <shared_mutex>
class DmaRWMutex {
public:
  void LockRead() { m_Mutex.lock_shared(); }
  void UnlockRead() { m_Mutex.unlock_shared(); }
  void LockWrite() { m_Mutex.lock(); }
  void UnlockWrite() { m_Mutex.unlock(); }

private:
  std::shared_mutex m_Mutex;
};
#define DMA_RW_MUTEX DmaRWMutex
#elif defined(_WIN32) && defined(WINVER) && WINVER >= 0x0600
// Use SRWLOCK from WinAPI.
// Minimum supported client = Windows Vista, server = Windows Server 2008.
class DmaRWMutex {
public:
  DmaRWMutex() { InitializeSRWLock(&m_Lock); }
  void LockRead() { AcquireSRWLockShared(&m_Lock); }
  void UnlockRead() { ReleaseSRWLockShared(&m_Lock); }
  void LockWrite() { AcquireSRWLockExclusive(&m_Lock); }
  void UnlockWrite() { ReleaseSRWLockExclusive(&m_Lock); }

private:
  SRWLOCK m_Lock;
};
#define DMA_RW_MUTEX DmaRWMutex
#else
// Less efficient fallback: Use normal mutex.
class DmaRWMutex {
public:
  void LockRead() { m_Mutex.Lock(); }
  void UnlockRead() { m_Mutex.Unlock(); }
  void LockWrite() { m_Mutex.Lock(); }
  void UnlockWrite() { m_Mutex.Unlock(); }

private:
  DMA_MUTEX m_Mutex;
};
#define DMA_RW_MUTEX DmaRWMutex
#endif // #if DMA_USE_STL_SHARED_MUTEX
#endif // #ifndef DMA_RW_MUTEX

/*
If providing your own implementation, you need to implement a subset of
std::atomic.
*/
#ifndef DMA_ATOMIC_UINT32
#include <atomic>
#define DMA_ATOMIC_UINT32 std::atomic<uint32_t>
#endif

#ifndef DMA_ATOMIC_UINT64
#include <atomic>
#define DMA_ATOMIC_UINT64 std::atomic<uint64_t>
#endif

#ifndef DMA_DEBUG_ALWAYS_DEDICATED_MEMORY
/**
Every allocation will have its own memory block.
Define to 1 for debugging purposes only.
*/
#define DMA_DEBUG_ALWAYS_DEDICATED_MEMORY (0)
#endif

#ifndef DMA_DEBUG_ALIGNMENT
/**
Minimum alignment of all allocations, in bytes.
Set to more than 1 for debugging purposes only. Must be power of two.
*/
#define DMA_DEBUG_ALIGNMENT (1)
#endif

#ifndef DMA_DEBUG_MARGIN
/**
Minimum margin before and after every allocation, in bytes.
Set nonzero for debugging purposes only.
*/
#define DMA_DEBUG_MARGIN (0)
#endif

#ifndef DMA_DEBUG_INITIALIZE_ALLOCATIONS
/**
Define this macro to 1 to automatically fill new allocations and destroyed
allocations with some bit pattern.
*/
#define DMA_DEBUG_INITIALIZE_ALLOCATIONS (0)
#endif

#ifndef DMA_DEBUG_DETECT_CORRUPTION
/**
Define this macro to 1 together with non-zero value of DMA_DEBUG_MARGIN to
enable writing magic value to the margin before and after every allocation and
validating it, so that memory corruptions (out-of-bounds writes) are detected.
*/
#define DMA_DEBUG_DETECT_CORRUPTION (0)
#endif

#ifndef DMA_DEBUG_GLOBAL_MUTEX
/**
Set this to 1 for debugging purposes only, to enable single mutex protecting all
entry calls to the library. Can be useful for debugging multithreading issues.
*/
#define DMA_DEBUG_GLOBAL_MUTEX (0)
#endif

#ifndef DMA_DEBUG_MIN_BUFFER_IMAGE_GRANULARITY
/**
Minimum value for VkPhysicalDeviceLimits::bufferImageGranularity.
Set to more than 1 for debugging purposes only. Must be power of two.
*/
#define DMA_DEBUG_MIN_BUFFER_IMAGE_GRANULARITY (1)
#endif

#ifndef DMA_SMALL_HEAP_MAX_SIZE
/// Maximum size of a memory heap in Vulkan to consider it "small".
#define DMA_SMALL_HEAP_MAX_SIZE (1024ull * 1024 * 1024)
#endif

#ifndef DMA_DEFAULT_LARGE_HEAP_BLOCK_SIZE
/// Default size of a block allocated as single DkMemBlock from a "large"
/// heap.
#define DMA_DEFAULT_LARGE_HEAP_BLOCK_SIZE (256ull * 1024 * 1024)
#endif

#ifndef DMA_CLASS_NO_COPY
#define DMA_CLASS_NO_COPY(className)                                           \
private:                                                                       \
  className(const className &) = delete;                                       \
  className &operator=(const className &) = delete;
#endif

static const uint32_t DMA_FRAME_INDEX_LOST = UINT32_MAX;

// Decimal 2139416166, float NaN, little-endian binary 66 E6 84 7F.
static const uint32_t DMA_CORRUPTION_DETECTION_MAGIC_VALUE = 0x7F84E666;

static const uint8_t DMA_ALLOCATION_FILL_PATTERN_CREATED = 0xDC;
static const uint8_t DMA_ALLOCATION_FILL_PATTERN_DESTROYED = 0xEF;

/*******************************************************************************
END OF CONFIGURATION
*/

static const uint32_t DMA_ALLOCATION_INTERNAL_STRATEGY_MIN_OFFSET = 0x10000000u;

static DmaAllocationCallbacks DmaEmptyAllocationCallbacks = {DMA_NULL, DMA_NULL,
                                                             DMA_NULL};

// Returns number of bits set to 1 in (v).
static inline uint32_t DmaCountBitsSet(uint32_t v) {
  uint32_t c = v - ((v >> 1) & 0x55555555);
  c = ((c >> 2) & 0x33333333) + (c & 0x33333333);
  c = ((c >> 4) + c) & 0x0F0F0F0F;
  c = ((c >> 8) + c) & 0x00FF00FF;
  c = ((c >> 16) + c) & 0x0000FFFF;
  return c;
}

// Aligns given value up to nearest multiply of align value. For example:
// DmaAlignUp(11, 8) = 16. Use types like uint32_t, uint64_t as T.
template <typename T> static inline T DmaAlignUp(T val, T align) {
  return (val + align - 1) / align * align;
}
// Aligns given value down to nearest multiply of align value. For example:
// DmaAlignUp(11, 8) = 8. Use types like uint32_t, uint64_t as T.
template <typename T> static inline T DmaAlignDown(T val, T align) {
  return val / align * align;
}

// Division with mathematical rounding to nearest number.
template <typename T> static inline T DmaRoundDiv(T x, T y) {
  return (x + (y / (T)2)) / y;
}

/*
Returns true if given number is a power of two.
T must be unsigned integer number or signed integer but always nonnegative.
For 0 returns true.
*/
template <typename T> inline bool DmaIsPow2(T x) { return (x & (x - 1)) == 0; }

// Returns smallest power of 2 greater or equal to v.
static inline uint32_t DmaNextPow2(uint32_t v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}
static inline uint64_t DmaNextPow2(uint64_t v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  v++;
  return v;
}

// Returns largest power of 2 less or equal to v.
static inline uint32_t DmaPrevPow2(uint32_t v) {
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v = v ^ (v >> 1);
  return v;
}
static inline uint64_t DmaPrevPow2(uint64_t v) {
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  v = v ^ (v >> 1);
  return v;
}

static inline bool DmaStrIsEmpty(const char *pStr) {
  return pStr == DMA_NULL || *pStr == '\0';
}

#if DMA_STATS_STRING_ENABLED

static const char *DmaAlgorithmToStr(uint32_t algorithm) {
  switch (algorithm) {
  case DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT:
    return "Linear";
  case DMA_POOL_CREATE_BUDDY_ALGORITHM_BIT:
    return "Buddy";
  case 0:
    return "Default";
  default:
    DMA_ASSERT(0);
    return "";
  }
}

#endif // #if DMA_STATS_STRING_ENABLED

#ifndef DMA_SORT

template <typename Iterator, typename Compare>
Iterator DmaQuickSortPartition(Iterator beg, Iterator end, Compare cmp) {
  Iterator centerValue = end;
  --centerValue;
  Iterator insertIndex = beg;
  for (Iterator memTypeIndex = beg; memTypeIndex < centerValue;
       ++memTypeIndex) {
    if (cmp(*memTypeIndex, *centerValue)) {
      if (insertIndex != memTypeIndex) {
        DMA_SWAP(*memTypeIndex, *insertIndex);
      }
      ++insertIndex;
    }
  }
  if (insertIndex != centerValue) {
    DMA_SWAP(*insertIndex, *centerValue);
  }
  return insertIndex;
}

template <typename Iterator, typename Compare>
void DmaQuickSort(Iterator beg, Iterator end, Compare cmp) {
  if (beg < end) {
    Iterator it = DmaQuickSortPartition<Iterator, Compare>(beg, end, cmp);
    DmaQuickSort<Iterator, Compare>(beg, it, cmp);
    DmaQuickSort<Iterator, Compare>(it + 1, end, cmp);
  }
}

#define DMA_SORT(beg, end, cmp) DmaQuickSort(beg, end, cmp)

#endif // #ifndef DMA_SORT

/*
Returns true if two memory blocks occupy overlapping pages.
ResourceA must be in less memory offset than ResourceB.

Algorithm is based on "Vulkan 1.0.39 - A Specification (with all registered
Vulkan extensions)" chapter 11.6 "Resource Memory Association", paragraph
"Buffer-Image Granularity".
*/
static inline bool DmaBlocksOnSamePage(uint32_t resourceAOffset,
                                       uint32_t resourceASize,
                                       uint32_t resourceBOffset,
                                       uint32_t pageSize) {
  DMA_ASSERT(resourceAOffset + resourceASize <= resourceBOffset &&
             resourceASize > 0 && pageSize > 0);
  uint32_t resourceAEnd = resourceAOffset + resourceASize - 1;
  uint32_t resourceAEndPage = resourceAEnd & ~(pageSize - 1);
  uint32_t resourceBStart = resourceBOffset;
  uint32_t resourceBStartPage = resourceBStart & ~(pageSize - 1);
  return resourceAEndPage == resourceBStartPage;
}

enum DmaSuballocationType {
  DMA_SUBALLOCATION_TYPE_FREE = 0,
  DMA_SUBALLOCATION_TYPE_UNKNOWN = 1,
  DMA_SUBALLOCATION_TYPE_BUFFER = 2,
  DMA_SUBALLOCATION_TYPE_IMAGE_UNKNOWN = 3,
  DMA_SUBALLOCATION_TYPE_IMAGE_LINEAR = 4,
  DMA_SUBALLOCATION_TYPE_IMAGE_OPTIMAL = 5,
  DMA_SUBALLOCATION_TYPE_MAX_ENUM = 0x7FFFFFFF
};

static void DmaWriteMagicValue(void *pData, uint32_t offset) {
#if DMA_DEBUG_MARGIN > 0 && DMA_DEBUG_DETECT_CORRUPTION
  uint32_t *pDst = (uint32_t *)((char *)pData + offset);
  const size_t numberCount = DMA_DEBUG_MARGIN / sizeof(uint32_t);
  for (size_t i = 0; i < numberCount; ++i, ++pDst) {
    *pDst = DMA_CORRUPTION_DETECTION_MAGIC_VALUE;
  }
#else
  // no-op
#endif
}

static bool DmaValidateMagicValue(const void *pData, uint32_t offset) {
#if DMA_DEBUG_MARGIN > 0 && DMA_DEBUG_DETECT_CORRUPTION
  const uint32_t *pSrc = (const uint32_t *)((const char *)pData + offset);
  const size_t numberCount = DMA_DEBUG_MARGIN / sizeof(uint32_t);
  for (size_t i = 0; i < numberCount; ++i, ++pSrc) {
    if (*pSrc != DMA_CORRUPTION_DETECTION_MAGIC_VALUE) {
      return false;
    }
  }
#endif
  return true;
}

// Helper RAII class to lock a mutex in constructor and unlock it in destructor
// (at the end of scope).
struct DmaMutexLock {
  DMA_CLASS_NO_COPY(DmaMutexLock)
public:
  DmaMutexLock(DMA_MUTEX &mutex, bool useMutex = true)
      : m_pMutex(useMutex ? &mutex : DMA_NULL) {
    if (m_pMutex) {
      m_pMutex->Lock();
    }
  }
  ~DmaMutexLock() {
    if (m_pMutex) {
      m_pMutex->Unlock();
    }
  }

private:
  DMA_MUTEX *m_pMutex;
};

// Helper RAII class to lock a RW mutex in constructor and unlock it in
// destructor (at the end of scope), for reading.
struct DmaMutexLockRead {
  DMA_CLASS_NO_COPY(DmaMutexLockRead)
public:
  DmaMutexLockRead(DMA_RW_MUTEX &mutex, bool useMutex)
      : m_pMutex(useMutex ? &mutex : DMA_NULL) {
    if (m_pMutex) {
      m_pMutex->LockRead();
    }
  }
  ~DmaMutexLockRead() {
    if (m_pMutex) {
      m_pMutex->UnlockRead();
    }
  }

private:
  DMA_RW_MUTEX *m_pMutex;
};

// Helper RAII class to lock a RW mutex in constructor and unlock it in
// destructor (at the end of scope), for writing.
struct DmaMutexLockWrite {
  DMA_CLASS_NO_COPY(DmaMutexLockWrite)
public:
  DmaMutexLockWrite(DMA_RW_MUTEX &mutex, bool useMutex)
      : m_pMutex(useMutex ? &mutex : DMA_NULL) {
    if (m_pMutex) {
      m_pMutex->LockWrite();
    }
  }
  ~DmaMutexLockWrite() {
    if (m_pMutex) {
      m_pMutex->UnlockWrite();
    }
  }

private:
  DMA_RW_MUTEX *m_pMutex;
};

#if DMA_DEBUG_GLOBAL_MUTEX
static DMA_MUTEX gDebugGlobalMutex;
#define DMA_DEBUG_GLOBAL_MUTEX_LOCK                                            \
  DmaMutexLock debugGlobalMutexLock(gDebugGlobalMutex, true);
#else
#define DMA_DEBUG_GLOBAL_MUTEX_LOCK
#endif

// Minimum size of a free suballocation to register it in the free suballocation
// collection.
static const uint32_t DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER = 16;

/*
Performs binary search and returns iterator to first element that is greater or
equal to (key), according to comparison (cmp).

Cmp should return true if first argument is less than second argument.

Returned value is the found element, if present in the collection or place where
new element with value (key) should be inserted.
*/
template <typename CmpLess, typename IterT, typename KeyT>
static IterT DmaBinaryFindFirstNotLess(IterT beg, IterT end, const KeyT &key,
                                       const CmpLess &cmp) {
  size_t down = 0, up = (end - beg);
  while (down < up) {
    const size_t mid = (down + up) / 2;
    if (cmp(*(beg + mid), key)) {
      down = mid + 1;
    } else {
      up = mid;
    }
  }
  return beg + down;
}

template <typename CmpLess, typename IterT, typename KeyT>
IterT DmaBinaryFindSorted(const IterT &beg, const IterT &end, const KeyT &value,
                          const CmpLess &cmp) {
  IterT it =
      DmaBinaryFindFirstNotLess<CmpLess, IterT, KeyT>(beg, end, value, cmp);
  if (it == end || (!cmp(*it, value) && !cmp(value, *it))) {
    return it;
  }
  return end;
}

/*
Returns true if all pointers in the array are not-null and unique.
Warning! O(n^2) complexity. Use only inside DMA_HEAVY_ASSERT.
T must be pointer type, e.g. DmaAllocation, DmaPool.
*/
template <typename T>
static bool DmaValidatePointerArray(uint32_t count, const T *arr) {
  for (uint32_t i = 0; i < count; ++i) {
    const T iPtr = arr[i];
    if (iPtr == DMA_NULL) {
      return false;
    }
    for (uint32_t j = i + 1; j < count; ++j) {
      if (iPtr == arr[j]) {
        return false;
      }
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Memory allocation

static void *DmaMalloc(const DmaAllocationCallbacks *pAllocationCallbacks,
                       size_t size, size_t alignment) {
  if ((pAllocationCallbacks != DMA_NULL) &&
      (pAllocationCallbacks->pfnAllocation != DMA_NULL)) {
    return (*pAllocationCallbacks->pfnAllocation)(
        pAllocationCallbacks->pUserData, size, alignment);
  } else {
    return DMA_SYSTEM_ALIGNED_MALLOC(size, alignment);
  }
}

static void DmaFree(const DmaAllocationCallbacks *pAllocationCallbacks,
                    void *ptr) {
  if ((pAllocationCallbacks != DMA_NULL) &&
      (pAllocationCallbacks->pfnFree != DMA_NULL)) {
    (*pAllocationCallbacks->pfnFree)(pAllocationCallbacks->pUserData, ptr);
  } else {
    DMA_SYSTEM_FREE(ptr);
  }
}

template <typename T>
static T *DmaAllocate(const DmaAllocationCallbacks *pAllocationCallbacks) {
  return (T *)DmaMalloc(pAllocationCallbacks, sizeof(T), DMA_ALIGN_OF(T));
}

template <typename T>
static T *DmaAllocateArray(const DmaAllocationCallbacks *pAllocationCallbacks,
                           size_t count) {
  return (T *)DmaMalloc(pAllocationCallbacks, sizeof(T) * count,
                        DMA_ALIGN_OF(T));
}

#define dma_new(allocator, type) new (DmaAllocate<type>(allocator))(type)

#define dma_new_array(allocator, type, count)                                  \
  new (DmaAllocateArray<type>((allocator), (count)))(type)

template <typename T>
static void dma_delete(const DmaAllocationCallbacks *pAllocationCallbacks,
                       T *ptr) {
  ptr->~T();
  DmaFree(pAllocationCallbacks, ptr);
}

template <typename T>
static void dma_delete_array(const DmaAllocationCallbacks *pAllocationCallbacks,
                             T *ptr, size_t count) {
  if (ptr != DMA_NULL) {
    for (size_t i = count; i--;) {
      ptr[i].~T();
    }
    DmaFree(pAllocationCallbacks, ptr);
  }
}

static char *DmaCreateStringCopy(const DmaAllocationCallbacks *allocs,
                                 const char *srcStr) {
  if (srcStr != DMA_NULL) {
    const size_t len = strlen(srcStr);
    char *const result = dma_new_array(allocs, char, len + 1);
    memcpy(result, srcStr, len + 1);
    return result;
  } else {
    return DMA_NULL;
  }
}

static void DmaFreeString(const DmaAllocationCallbacks *allocs, char *str) {
  if (str != DMA_NULL) {
    const size_t len = strlen(str);
    dma_delete_array(allocs, str, len + 1);
  }
}

// STL-compatible allocator.
template <typename T> class DmaStlAllocator {
public:
  const DmaAllocationCallbacks *const m_pCallbacks;
  typedef T value_type;

  DmaStlAllocator(const DmaAllocationCallbacks *pCallbacks)
      : m_pCallbacks(pCallbacks) {}
  template <typename U>
  DmaStlAllocator(const DmaStlAllocator<U> &src)
      : m_pCallbacks(src.m_pCallbacks) {}

  T *allocate(size_t n) { return DmaAllocateArray<T>(m_pCallbacks, n); }
  void deallocate(T *p, size_t n) { DmaFree(m_pCallbacks, p); }

  template <typename U> bool operator==(const DmaStlAllocator<U> &rhs) const {
    return m_pCallbacks == rhs.m_pCallbacks;
  }
  template <typename U> bool operator!=(const DmaStlAllocator<U> &rhs) const {
    return m_pCallbacks != rhs.m_pCallbacks;
  }

  DmaStlAllocator &operator=(const DmaStlAllocator &x) = delete;
};

#if DMA_USE_STL_VECTOR

#define DmaVector std::vector

template <typename T, typename allocatorT>
static void DmaVectorInsert(std::vector<T, allocatorT> &vec, size_t index,
                            const T &item) {
  vec.insert(vec.begin() + index, item);
}

template <typename T, typename allocatorT>
static void DmaVectorRemove(std::vector<T, allocatorT> &vec, size_t index) {
  vec.erase(vec.begin() + index);
}

#else // #if DMA_USE_STL_VECTOR

/* Class with interface compatible with subset of std::vector.
T must be POD because constructors and destructors are not called and memcpy is
used for these objects. */
template <typename T, typename AllocatorT> class DmaVector {
public:
  typedef T value_type;

  DmaVector(const AllocatorT &allocator)
      : m_Allocator(allocator), m_pArray(DMA_NULL), m_Count(0), m_Capacity(0) {}

  DmaVector(size_t count, const AllocatorT &allocator)
      : m_Allocator(allocator),
        m_pArray(count ? (T *)DmaAllocateArray<T>(allocator.m_pCallbacks, count)
                       : DMA_NULL),
        m_Count(count), m_Capacity(count) {}

  // This version of the constructor is here for compatibility with pre-C++14
  // std::vector. value is unused.
  DmaVector(size_t count, const T &value, const AllocatorT &allocator)
      : DmaVector(count, allocator) {}

  DmaVector(const DmaVector<T, AllocatorT> &src)
      : m_Allocator(src.m_Allocator),
        m_pArray(src.m_Count ? (T *)DmaAllocateArray<T>(
                                   src.m_Allocator.m_pCallbacks, src.m_Count)
                             : DMA_NULL),
        m_Count(src.m_Count), m_Capacity(src.m_Count) {
    if (m_Count != 0) {
      memcpy(m_pArray, src.m_pArray, m_Count * sizeof(T));
    }
  }

  ~DmaVector() { DmaFree(m_Allocator.m_pCallbacks, m_pArray); }

  DmaVector &operator=(const DmaVector<T, AllocatorT> &rhs) {
    if (&rhs != this) {
      resize(rhs.m_Count);
      if (m_Count != 0) {
        memcpy(m_pArray, rhs.m_pArray, m_Count * sizeof(T));
      }
    }
    return *this;
  }

  bool empty() const { return m_Count == 0; }
  size_t size() const { return m_Count; }
  T *data() { return m_pArray; }
  const T *data() const { return m_pArray; }

  T &operator[](size_t index) {
    DMA_HEAVY_ASSERT(index < m_Count);
    return m_pArray[index];
  }
  const T &operator[](size_t index) const {
    DMA_HEAVY_ASSERT(index < m_Count);
    return m_pArray[index];
  }

  T &front() {
    DMA_HEAVY_ASSERT(m_Count > 0);
    return m_pArray[0];
  }
  const T &front() const {
    DMA_HEAVY_ASSERT(m_Count > 0);
    return m_pArray[0];
  }
  T &back() {
    DMA_HEAVY_ASSERT(m_Count > 0);
    return m_pArray[m_Count - 1];
  }
  const T &back() const {
    DMA_HEAVY_ASSERT(m_Count > 0);
    return m_pArray[m_Count - 1];
  }

  void reserve(size_t newCapacity, bool freeMemory = false) {
    newCapacity = DMA_MAX(newCapacity, m_Count);

    if ((newCapacity < m_Capacity) && !freeMemory) {
      newCapacity = m_Capacity;
    }

    if (newCapacity != m_Capacity) {
      T *const newArray = newCapacity
                              ? DmaAllocateArray<T>(m_Allocator, newCapacity)
                              : DMA_NULL;
      if (m_Count != 0) {
        memcpy(newArray, m_pArray, m_Count * sizeof(T));
      }
      DmaFree(m_Allocator.m_pCallbacks, m_pArray);
      m_Capacity = newCapacity;
      m_pArray = newArray;
    }
  }

  void resize(size_t newCount, bool freeMemory = false) {
    size_t newCapacity = m_Capacity;
    if (newCount > m_Capacity) {
      newCapacity = DMA_MAX(newCount, DMA_MAX(m_Capacity * 3 / 2, (size_t)8));
    } else if (freeMemory) {
      newCapacity = newCount;
    }

    if (newCapacity != m_Capacity) {
      T *const newArray =
          newCapacity
              ? DmaAllocateArray<T>(m_Allocator.m_pCallbacks, newCapacity)
              : DMA_NULL;
      const size_t elementsToCopy = DMA_MIN(m_Count, newCount);
      if (elementsToCopy != 0) {
        memcpy(newArray, m_pArray, elementsToCopy * sizeof(T));
      }
      DmaFree(m_Allocator.m_pCallbacks, m_pArray);
      m_Capacity = newCapacity;
      m_pArray = newArray;
    }

    m_Count = newCount;
  }

  void clear(bool freeMemory = false) { resize(0, freeMemory); }

  void insert(size_t index, const T &src) {
    DMA_HEAVY_ASSERT(index <= m_Count);
    const size_t oldCount = size();
    resize(oldCount + 1);
    if (index < oldCount) {
      memmove(m_pArray + (index + 1), m_pArray + index,
              (oldCount - index) * sizeof(T));
    }
    m_pArray[index] = src;
  }

  void remove(size_t index) {
    DMA_HEAVY_ASSERT(index < m_Count);
    const size_t oldCount = size();
    if (index < oldCount - 1) {
      memmove(m_pArray + index, m_pArray + (index + 1),
              (oldCount - index - 1) * sizeof(T));
    }
    resize(oldCount - 1);
  }

  void push_back(const T &src) {
    const size_t newIndex = size();
    resize(newIndex + 1);
    m_pArray[newIndex] = src;
  }

  void pop_back() {
    DMA_HEAVY_ASSERT(m_Count > 0);
    resize(size() - 1);
  }

  void push_front(const T &src) { insert(0, src); }

  void pop_front() {
    DMA_HEAVY_ASSERT(m_Count > 0);
    remove(0);
  }

  typedef T *iterator;

  iterator begin() { return m_pArray; }
  iterator end() { return m_pArray + m_Count; }

private:
  AllocatorT m_Allocator;
  T *m_pArray;
  size_t m_Count;
  size_t m_Capacity;
};

template <typename T, typename allocatorT>
static void DmaVectorInsert(DmaVector<T, allocatorT> &vec, size_t index,
                            const T &item) {
  vec.insert(index, item);
}

template <typename T, typename allocatorT>
static void DmaVectorRemove(DmaVector<T, allocatorT> &vec, size_t index) {
  vec.remove(index);
}

#endif // #if DMA_USE_STL_VECTOR

template <typename CmpLess, typename VectorT>
size_t DmaVectorInsertSorted(VectorT &vector,
                             const typename VectorT::value_type &value) {
  const size_t indexToInsert =
      DmaBinaryFindFirstNotLess(vector.data(), vector.data() + vector.size(),
                                value, CmpLess()) -
      vector.data();
  DmaVectorInsert(vector, indexToInsert, value);
  return indexToInsert;
}

template <typename CmpLess, typename VectorT>
bool DmaVectorRemoveSorted(VectorT &vector,
                           const typename VectorT::value_type &value) {
  CmpLess comparator;
  typename VectorT::iterator it = DmaBinaryFindFirstNotLess(
      vector.begin(), vector.end(), value, comparator);
  if ((it != vector.end()) && !comparator(*it, value) &&
      !comparator(value, *it)) {
    size_t indexToRemove = it - vector.begin();
    DmaVectorRemove(vector, indexToRemove);
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// class DmaPoolAllocator

/*
Allocator for objects of type T using a list of arrays (pools) to speed up
allocation. Number of elements that can be allocated is not bounded because
allocator can create multiple blocks.
*/
template <typename T> class DmaPoolAllocator {
  DMA_CLASS_NO_COPY(DmaPoolAllocator)
public:
  DmaPoolAllocator(const DmaAllocationCallbacks *pAllocationCallbacks,
                   uint32_t firstBlockCapacity);
  ~DmaPoolAllocator();
  T *Alloc();
  void Free(T *ptr);

private:
  union Item {
    uint32_t NextFreeIndex;
    alignas(T) char Value[sizeof(T)];
  };

  struct ItemBlock {
    Item *pItems;
    uint32_t Capacity;
    uint32_t FirstFreeIndex;
  };

  const DmaAllocationCallbacks *m_pAllocationCallbacks;
  const uint32_t m_FirstBlockCapacity;
  DmaVector<ItemBlock, DmaStlAllocator<ItemBlock>> m_ItemBlocks;

  ItemBlock &CreateNewBlock();
};

template <typename T>
DmaPoolAllocator<T>::DmaPoolAllocator(
    const DmaAllocationCallbacks *pAllocationCallbacks,
    uint32_t firstBlockCapacity)
    : m_pAllocationCallbacks(pAllocationCallbacks),
      m_FirstBlockCapacity(firstBlockCapacity),
      m_ItemBlocks(DmaStlAllocator<ItemBlock>(pAllocationCallbacks)) {
  DMA_ASSERT(m_FirstBlockCapacity > 1);
}

template <typename T> DmaPoolAllocator<T>::~DmaPoolAllocator() {
  for (size_t i = m_ItemBlocks.size(); i--;)
    dma_delete_array(m_pAllocationCallbacks, m_ItemBlocks[i].pItems,
                     m_ItemBlocks[i].Capacity);
  m_ItemBlocks.clear();
}

template <typename T> T *DmaPoolAllocator<T>::Alloc() {
  for (size_t i = m_ItemBlocks.size(); i--;) {
    ItemBlock &block = m_ItemBlocks[i];
    // This block has some free items: Use first one.
    if (block.FirstFreeIndex != UINT32_MAX) {
      Item *const pItem = &block.pItems[block.FirstFreeIndex];
      block.FirstFreeIndex = pItem->NextFreeIndex;
      T *result = (T *)&pItem->Value;
      new (result) T(); // Explicit constructor call.
      return result;
    }
  }

  // No block has free item: Create new one and use it.
  ItemBlock &newBlock = CreateNewBlock();
  Item *const pItem = &newBlock.pItems[0];
  newBlock.FirstFreeIndex = pItem->NextFreeIndex;
  T *result = (T *)&pItem->Value;
  new (result) T(); // Explicit constructor call.
  return result;
}

template <typename T> void DmaPoolAllocator<T>::Free(T *ptr) {
  // Search all memory blocks to find ptr.
  for (size_t i = m_ItemBlocks.size(); i--;) {
    ItemBlock &block = m_ItemBlocks[i];

    // Casting to union.
    Item *pItemPtr;
    memcpy(&pItemPtr, &ptr, sizeof(pItemPtr));

    // Check if pItemPtr is in address range of this block.
    if ((pItemPtr >= block.pItems) &&
        (pItemPtr < block.pItems + block.Capacity)) {
      ptr->~T(); // Explicit destructor call.
      const uint32_t index = static_cast<uint32_t>(pItemPtr - block.pItems);
      pItemPtr->NextFreeIndex = block.FirstFreeIndex;
      block.FirstFreeIndex = index;
      return;
    }
  }
  DMA_ASSERT(0 && "Pointer doesn't belong to this memory pool.");
}

template <typename T>
typename DmaPoolAllocator<T>::ItemBlock &DmaPoolAllocator<T>::CreateNewBlock() {
  const uint32_t newBlockCapacity = m_ItemBlocks.empty()
                                        ? m_FirstBlockCapacity
                                        : m_ItemBlocks.back().Capacity * 3 / 2;

  const ItemBlock newBlock = {
      dma_new_array(m_pAllocationCallbacks, Item, newBlockCapacity),
      newBlockCapacity, 0};

  m_ItemBlocks.push_back(newBlock);

  // Setup singly-linked list of all free items in this block.
  for (uint32_t i = 0; i < newBlockCapacity - 1; ++i)
    newBlock.pItems[i].NextFreeIndex = i + 1;
  newBlock.pItems[newBlockCapacity - 1].NextFreeIndex = UINT32_MAX;
  return m_ItemBlocks.back();
}

////////////////////////////////////////////////////////////////////////////////
// class DmaRawList, DmaList

#if DMA_USE_STL_LIST

#define DmaList std::list

#else // #if DMA_USE_STL_LIST

template <typename T> struct DmaListItem {
  DmaListItem *pPrev;
  DmaListItem *pNext;
  T Value;
};

// Doubly linked list.
template <typename T> class DmaRawList {
  DMA_CLASS_NO_COPY(DmaRawList)
public:
  typedef DmaListItem<T> ItemType;

  DmaRawList(const DmaAllocationCallbacks *pAllocationCallbacks);
  ~DmaRawList();
  void Clear();

  size_t GetCount() const { return m_Count; }
  bool IsEmpty() const { return m_Count == 0; }

  ItemType *Front() { return m_pFront; }
  const ItemType *Front() const { return m_pFront; }
  ItemType *Back() { return m_pBack; }
  const ItemType *Back() const { return m_pBack; }

  ItemType *PushBack();
  ItemType *PushFront();
  ItemType *PushBack(const T &value);
  ItemType *PushFront(const T &value);
  void PopBack();
  void PopFront();

  // Item can be null - it means PushBack.
  ItemType *InsertBefore(ItemType *pItem);
  // Item can be null - it means PushFront.
  ItemType *InsertAfter(ItemType *pItem);

  ItemType *InsertBefore(ItemType *pItem, const T &value);
  ItemType *InsertAfter(ItemType *pItem, const T &value);

  void Remove(ItemType *pItem);

private:
  const DmaAllocationCallbacks *const m_pAllocationCallbacks;
  DmaPoolAllocator<ItemType> m_ItemAllocator;
  ItemType *m_pFront;
  ItemType *m_pBack;
  size_t m_Count;
};

template <typename T>
DmaRawList<T>::DmaRawList(const DmaAllocationCallbacks *pAllocationCallbacks)
    : m_pAllocationCallbacks(pAllocationCallbacks),
      m_ItemAllocator(pAllocationCallbacks, 128), m_pFront(DMA_NULL),
      m_pBack(DMA_NULL), m_Count(0) {}

template <typename T> DmaRawList<T>::~DmaRawList() {
  // Intentionally not calling Clear, because that would be unnecessary
  // computations to return all items to m_ItemAllocator as free.
}

template <typename T> void DmaRawList<T>::Clear() {
  if (IsEmpty() == false) {
    ItemType *pItem = m_pBack;
    while (pItem != DMA_NULL) {
      ItemType *const pPrevItem = pItem->pPrev;
      m_ItemAllocator.Free(pItem);
      pItem = pPrevItem;
    }
    m_pFront = DMA_NULL;
    m_pBack = DMA_NULL;
    m_Count = 0;
  }
}

template <typename T> DmaListItem<T> *DmaRawList<T>::PushBack() {
  ItemType *const pNewItem = m_ItemAllocator.Alloc();
  pNewItem->pNext = DMA_NULL;
  if (IsEmpty()) {
    pNewItem->pPrev = DMA_NULL;
    m_pFront = pNewItem;
    m_pBack = pNewItem;
    m_Count = 1;
  } else {
    pNewItem->pPrev = m_pBack;
    m_pBack->pNext = pNewItem;
    m_pBack = pNewItem;
    ++m_Count;
  }
  return pNewItem;
}

template <typename T> DmaListItem<T> *DmaRawList<T>::PushFront() {
  ItemType *const pNewItem = m_ItemAllocator.Alloc();
  pNewItem->pPrev = DMA_NULL;
  if (IsEmpty()) {
    pNewItem->pNext = DMA_NULL;
    m_pFront = pNewItem;
    m_pBack = pNewItem;
    m_Count = 1;
  } else {
    pNewItem->pNext = m_pFront;
    m_pFront->pPrev = pNewItem;
    m_pFront = pNewItem;
    ++m_Count;
  }
  return pNewItem;
}

template <typename T> DmaListItem<T> *DmaRawList<T>::PushBack(const T &value) {
  ItemType *const pNewItem = PushBack();
  pNewItem->Value = value;
  return pNewItem;
}

template <typename T> DmaListItem<T> *DmaRawList<T>::PushFront(const T &value) {
  ItemType *const pNewItem = PushFront();
  pNewItem->Value = value;
  return pNewItem;
}

template <typename T> void DmaRawList<T>::PopBack() {
  DMA_HEAVY_ASSERT(m_Count > 0);
  ItemType *const pBackItem = m_pBack;
  ItemType *const pPrevItem = pBackItem->pPrev;
  if (pPrevItem != DMA_NULL) {
    pPrevItem->pNext = DMA_NULL;
  }
  m_pBack = pPrevItem;
  m_ItemAllocator.Free(pBackItem);
  --m_Count;
}

template <typename T> void DmaRawList<T>::PopFront() {
  DMA_HEAVY_ASSERT(m_Count > 0);
  ItemType *const pFrontItem = m_pFront;
  ItemType *const pNextItem = pFrontItem->pNext;
  if (pNextItem != DMA_NULL) {
    pNextItem->pPrev = DMA_NULL;
  }
  m_pFront = pNextItem;
  m_ItemAllocator.Free(pFrontItem);
  --m_Count;
}

template <typename T> void DmaRawList<T>::Remove(ItemType *pItem) {
  DMA_HEAVY_ASSERT(pItem != DMA_NULL);
  DMA_HEAVY_ASSERT(m_Count > 0);

  if (pItem->pPrev != DMA_NULL) {
    pItem->pPrev->pNext = pItem->pNext;
  } else {
    DMA_HEAVY_ASSERT(m_pFront == pItem);
    m_pFront = pItem->pNext;
  }

  if (pItem->pNext != DMA_NULL) {
    pItem->pNext->pPrev = pItem->pPrev;
  } else {
    DMA_HEAVY_ASSERT(m_pBack == pItem);
    m_pBack = pItem->pPrev;
  }

  m_ItemAllocator.Free(pItem);
  --m_Count;
}

template <typename T>
DmaListItem<T> *DmaRawList<T>::InsertBefore(ItemType *pItem) {
  if (pItem != DMA_NULL) {
    ItemType *const prevItem = pItem->pPrev;
    ItemType *const newItem = m_ItemAllocator.Alloc();
    newItem->pPrev = prevItem;
    newItem->pNext = pItem;
    pItem->pPrev = newItem;
    if (prevItem != DMA_NULL) {
      prevItem->pNext = newItem;
    } else {
      DMA_HEAVY_ASSERT(m_pFront == pItem);
      m_pFront = newItem;
    }
    ++m_Count;
    return newItem;
  } else
    return PushBack();
}

template <typename T>
DmaListItem<T> *DmaRawList<T>::InsertAfter(ItemType *pItem) {
  if (pItem != DMA_NULL) {
    ItemType *const nextItem = pItem->pNext;
    ItemType *const newItem = m_ItemAllocator.Alloc();
    newItem->pNext = nextItem;
    newItem->pPrev = pItem;
    pItem->pNext = newItem;
    if (nextItem != DMA_NULL) {
      nextItem->pPrev = newItem;
    } else {
      DMA_HEAVY_ASSERT(m_pBack == pItem);
      m_pBack = newItem;
    }
    ++m_Count;
    return newItem;
  } else
    return PushFront();
}

template <typename T>
DmaListItem<T> *DmaRawList<T>::InsertBefore(ItemType *pItem, const T &value) {
  ItemType *const newItem = InsertBefore(pItem);
  newItem->Value = value;
  return newItem;
}

template <typename T>
DmaListItem<T> *DmaRawList<T>::InsertAfter(ItemType *pItem, const T &value) {
  ItemType *const newItem = InsertAfter(pItem);
  newItem->Value = value;
  return newItem;
}

template <typename T, typename AllocatorT> class DmaList {
  DMA_CLASS_NO_COPY(DmaList)
public:
  class iterator {
  public:
    iterator() : m_pList(DMA_NULL), m_pItem(DMA_NULL) {}

    T &operator*() const {
      DMA_HEAVY_ASSERT(m_pItem != DMA_NULL);
      return m_pItem->Value;
    }
    T *operator->() const {
      DMA_HEAVY_ASSERT(m_pItem != DMA_NULL);
      return &m_pItem->Value;
    }

    iterator &operator++() {
      DMA_HEAVY_ASSERT(m_pItem != DMA_NULL);
      m_pItem = m_pItem->pNext;
      return *this;
    }
    iterator &operator--() {
      if (m_pItem != DMA_NULL) {
        m_pItem = m_pItem->pPrev;
      } else {
        DMA_HEAVY_ASSERT(!m_pList->IsEmpty());
        m_pItem = m_pList->Back();
      }
      return *this;
    }

    iterator operator++(int) {
      iterator result = *this;
      ++*this;
      return result;
    }
    iterator operator--(int) {
      iterator result = *this;
      --*this;
      return result;
    }

    bool operator==(const iterator &rhs) const {
      DMA_HEAVY_ASSERT(m_pList == rhs.m_pList);
      return m_pItem == rhs.m_pItem;
    }
    bool operator!=(const iterator &rhs) const {
      DMA_HEAVY_ASSERT(m_pList == rhs.m_pList);
      return m_pItem != rhs.m_pItem;
    }

  private:
    DmaRawList<T> *m_pList;
    DmaListItem<T> *m_pItem;

    iterator(DmaRawList<T> *pList, DmaListItem<T> *pItem)
        : m_pList(pList), m_pItem(pItem) {}

    friend class DmaList<T, AllocatorT>;
  };

  class const_iterator {
  public:
    const_iterator() : m_pList(DMA_NULL), m_pItem(DMA_NULL) {}

    const_iterator(const iterator &src)
        : m_pList(src.m_pList), m_pItem(src.m_pItem) {}

    const T &operator*() const {
      DMA_HEAVY_ASSERT(m_pItem != DMA_NULL);
      return m_pItem->Value;
    }
    const T *operator->() const {
      DMA_HEAVY_ASSERT(m_pItem != DMA_NULL);
      return &m_pItem->Value;
    }

    const_iterator &operator++() {
      DMA_HEAVY_ASSERT(m_pItem != DMA_NULL);
      m_pItem = m_pItem->pNext;
      return *this;
    }
    const_iterator &operator--() {
      if (m_pItem != DMA_NULL) {
        m_pItem = m_pItem->pPrev;
      } else {
        DMA_HEAVY_ASSERT(!m_pList->IsEmpty());
        m_pItem = m_pList->Back();
      }
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator result = *this;
      ++*this;
      return result;
    }
    const_iterator operator--(int) {
      const_iterator result = *this;
      --*this;
      return result;
    }

    bool operator==(const const_iterator &rhs) const {
      DMA_HEAVY_ASSERT(m_pList == rhs.m_pList);
      return m_pItem == rhs.m_pItem;
    }
    bool operator!=(const const_iterator &rhs) const {
      DMA_HEAVY_ASSERT(m_pList == rhs.m_pList);
      return m_pItem != rhs.m_pItem;
    }

  private:
    const_iterator(const DmaRawList<T> *pList, const DmaListItem<T> *pItem)
        : m_pList(pList), m_pItem(pItem) {}

    const DmaRawList<T> *m_pList;
    const DmaListItem<T> *m_pItem;

    friend class DmaList<T, AllocatorT>;
  };

  DmaList(const AllocatorT &allocator) : m_RawList(allocator.m_pCallbacks) {}

  bool empty() const { return m_RawList.IsEmpty(); }
  size_t size() const { return m_RawList.GetCount(); }

  iterator begin() { return iterator(&m_RawList, m_RawList.Front()); }
  iterator end() { return iterator(&m_RawList, DMA_NULL); }

  const_iterator cbegin() const {
    return const_iterator(&m_RawList, m_RawList.Front());
  }
  const_iterator cend() const { return const_iterator(&m_RawList, DMA_NULL); }

  void clear() { m_RawList.Clear(); }
  void push_back(const T &value) { m_RawList.PushBack(value); }
  void erase(iterator it) { m_RawList.Remove(it.m_pItem); }
  iterator insert(iterator it, const T &value) {
    return iterator(&m_RawList, m_RawList.InsertBefore(it.m_pItem, value));
  }

private:
  DmaRawList<T> m_RawList;
};

#endif // #if DMA_USE_STL_LIST

////////////////////////////////////////////////////////////////////////////////
// class DmaMap

// Unused in this version.
#if 0

#if DMA_USE_STL_UNORDERED_MAP

#define DmaPair std::pair

#define DMA_MAP_TYPE(KeyT, ValueT)                                             \
  std::unordered_map<KeyT, ValueT, std::hash<KeyT>, std::equal_to<KeyT>,       \
                     DmaStlAllocator<std::pair<KeyT, ValueT>>>

#else // #if DMA_USE_STL_UNORDERED_MAP

template<typename T1, typename T2>
struct DmaPair
{
    T1 first;
    T2 second;

    DmaPair() : first(), second() { }
    DmaPair(const T1& firstSrc, const T2& secondSrc) : first(firstSrc), second(secondSrc) { }
};

/* Class compatible with subset of interface of std::unordered_map.
KeyT, ValueT must be POD because they will be stored in DmaVector.
*/
template<typename KeyT, typename ValueT>
class DmaMap
{
public:
    typedef DmaPair<KeyT, ValueT> PairType;
    typedef PairType* iterator;

    DmaMap(const DmaStlAllocator<PairType>& allocator) : m_Vector(allocator) { }

    iterator begin() { return m_Vector.begin(); }
    iterator end() { return m_Vector.end(); }

    void insert(const PairType& pair);
    iterator find(const KeyT& key);
    void erase(iterator it);

private:
    DmaVector< PairType, DmaStlAllocator<PairType> > m_Vector;
};

#define DMA_MAP_TYPE(KeyT, ValueT) DmaMap<KeyT, ValueT>

template<typename FirstT, typename SecondT>
struct DmaPairFirstLess
{
    bool operator()(const DmaPair<FirstT, SecondT>& lhs, const DmaPair<FirstT, SecondT>& rhs) const
    {
        return lhs.first < rhs.first;
    }
    bool operator()(const DmaPair<FirstT, SecondT>& lhs, const FirstT& rhsFirst) const
    {
        return lhs.first < rhsFirst;
    }
};

template<typename KeyT, typename ValueT>
void DmaMap<KeyT, ValueT>::insert(const PairType& pair)
{
    const size_t indexToInsert = DmaBinaryFindFirstNotLess(
        m_Vector.data(),
        m_Vector.data() + m_Vector.size(),
        pair,
        DmaPairFirstLess<KeyT, ValueT>()) - m_Vector.data();
    DmaVectorInsert(m_Vector, indexToInsert, pair);
}

template<typename KeyT, typename ValueT>
DmaPair<KeyT, ValueT>* DmaMap<KeyT, ValueT>::find(const KeyT& key)
{
    PairType* it = DmaBinaryFindFirstNotLess(
        m_Vector.data(),
        m_Vector.data() + m_Vector.size(),
        key,
        DmaPairFirstLess<KeyT, ValueT>());
    if((it != m_Vector.end()) && (it->first == key))
    {
        return it;
    }
    else
    {
        return m_Vector.end();
    }
}

template<typename KeyT, typename ValueT>
void DmaMap<KeyT, ValueT>::erase(iterator it)
{
    DmaVectorRemove(m_Vector, it - m_Vector.begin());
}

#endif // #if DMA_USE_STL_UNORDERED_MAP

#endif // #if 0

////////////////////////////////////////////////////////////////////////////////

class DmaDeviceMemoryBlock;

enum DMA_CACHE_OPERATION { DMA_CACHE_FLUSH, DMA_CACHE_INVALIDATE };

struct DmaAllocation_T {
private:
  static const uint8_t MAP_COUNT_FLAG_PERSISTENT_MAP = 0x80;

  enum FLAGS {
    FLAG_USER_DATA_STRING = 0x01,
  };

public:
  enum ALLOCATION_TYPE {
    ALLOCATION_TYPE_NONE,
    ALLOCATION_TYPE_BLOCK,
    ALLOCATION_TYPE_DEDICATED,
  };

  /*
  This struct is allocated using DmaPoolAllocator.
  */

  void Ctor(uint32_t currentFrameIndex, bool userDataString) {
    m_Alignment = 1;
    m_Size = 0;
    m_MemoryTypeIndex = 0;
    m_pUserData = DMA_NULL;
    m_LastUseFrameIndex = currentFrameIndex;
    m_Type = (uint8_t)ALLOCATION_TYPE_NONE;
    m_SuballocationType = (uint8_t)DMA_SUBALLOCATION_TYPE_UNKNOWN;
    m_MapCount = 0;
    m_Flags = userDataString ? (uint8_t)FLAG_USER_DATA_STRING : 0;

#if DMA_STATS_STRING_ENABLED
    m_CreationFrameIndex = currentFrameIndex;
    m_BufferImageUsage = 0;
#endif
  }

  void Dtor() {
    DMA_ASSERT((m_MapCount & ~MAP_COUNT_FLAG_PERSISTENT_MAP) == 0 &&
               "Allocation was not unmapped before destruction.");

    // Check if owned string was freed.
    DMA_ASSERT(m_pUserData == DMA_NULL);
  }

  void InitBlockAllocation(DmaDeviceMemoryBlock *block, uint32_t offset,
                           uint32_t alignment, uint32_t size,
                           uint32_t memoryTypeIndex,
                           DmaSuballocationType suballocationType, bool mapped,
                           bool canBecomeLost) {
    DMA_ASSERT(m_Type == ALLOCATION_TYPE_NONE);
    DMA_ASSERT(block != DMA_NULL);
    m_Type = (uint8_t)ALLOCATION_TYPE_BLOCK;
    m_Alignment = alignment;
    m_Size = size;
    m_MemoryTypeIndex = memoryTypeIndex;
    m_MapCount = mapped ? MAP_COUNT_FLAG_PERSISTENT_MAP : 0;
    m_SuballocationType = (uint8_t)suballocationType;
    m_BlockAllocation.m_Block = block;
    m_BlockAllocation.m_Offset = offset;
    m_BlockAllocation.m_CanBecomeLost = canBecomeLost;
  }

  void InitLost() {
    DMA_ASSERT(m_Type == ALLOCATION_TYPE_NONE);
    DMA_ASSERT(m_LastUseFrameIndex.load() == DMA_FRAME_INDEX_LOST);
    m_Type = (uint8_t)ALLOCATION_TYPE_BLOCK;
    m_MemoryTypeIndex = 0;
    m_BlockAllocation.m_Block = DMA_NULL;
    m_BlockAllocation.m_Offset = 0;
    m_BlockAllocation.m_CanBecomeLost = true;
  }

  void ChangeBlockAllocation(DmaAllocator hAllocator,
                             DmaDeviceMemoryBlock *block, uint32_t offset);

  void ChangeOffset(uint32_t newOffset);

  // pMappedData not null means allocation is created with MAPPED flag.
  void InitDedicatedAllocation(uint32_t memoryTypeIndex, DkMemBlock hMemory,
                               DmaSuballocationType suballocationType,
                               void *pMappedData, uint32_t size) {
    DMA_ASSERT(m_Type == ALLOCATION_TYPE_NONE);
    DMA_ASSERT(hMemory != DMA_NULL_HANDLE);
    m_Type = (uint8_t)ALLOCATION_TYPE_DEDICATED;
    m_Alignment = 0;
    m_Size = size;
    m_MemoryTypeIndex = memoryTypeIndex;
    m_SuballocationType = (uint8_t)suballocationType;
    m_MapCount = (pMappedData != DMA_NULL) ? MAP_COUNT_FLAG_PERSISTENT_MAP : 0;
    m_DedicatedAllocation.m_hMemory = hMemory;
    m_DedicatedAllocation.m_pMappedData = pMappedData;
  }

  ALLOCATION_TYPE GetType() const { return (ALLOCATION_TYPE)m_Type; }
  uint32_t GetAlignment() const { return m_Alignment; }
  uint32_t GetSize() const { return m_Size; }
  bool IsUserDataString() const {
    return (m_Flags & FLAG_USER_DATA_STRING) != 0;
  }
  void *GetUserData() const { return m_pUserData; }
  void SetUserData(DmaAllocator hAllocator, void *pUserData);
  DmaSuballocationType GetSuballocationType() const {
    return (DmaSuballocationType)m_SuballocationType;
  }

  DmaDeviceMemoryBlock *GetBlock() const {
    DMA_ASSERT(m_Type == ALLOCATION_TYPE_BLOCK);
    return m_BlockAllocation.m_Block;
  }
  uint32_t GetOffset() const;
  DkMemBlock GetMemory() const;
  uint32_t GetMemoryTypeIndex() const { return m_MemoryTypeIndex; }
  bool IsPersistentMap() const {
    return (m_MapCount & MAP_COUNT_FLAG_PERSISTENT_MAP) != 0;
  }
  void *GetMappedData() const;
  bool CanBecomeLost() const;

  uint32_t GetLastUseFrameIndex() const { return m_LastUseFrameIndex.load(); }
  bool CompareExchangeLastUseFrameIndex(uint32_t &expected, uint32_t desired) {
    return m_LastUseFrameIndex.compare_exchange_weak(expected, desired);
  }
  /*
  - If hAllocation.LastUseFrameIndex + frameInUseCount <
  allocator.CurrentFrameIndex, makes it lost by setting LastUseFrameIndex =
  DMA_FRAME_INDEX_LOST and returns true.
  - Else, returns false.

  If hAllocation is already lost, assert - you should not call it then.
  If hAllocation was not created with CAN_BECOME_LOST_BIT, assert.
  */
  bool MakeLost(uint32_t currentFrameIndex, uint32_t frameInUseCount);

  void DedicatedAllocCalcStatsInfo(DmaStatInfo &outInfo) {
    DMA_ASSERT(m_Type == ALLOCATION_TYPE_DEDICATED);
    outInfo.blockCount = 1;
    outInfo.allocationCount = 1;
    outInfo.unusedRangeCount = 0;
    outInfo.usedBytes = m_Size;
    outInfo.unusedBytes = 0;
    outInfo.allocationSizeMin = outInfo.allocationSizeMax = m_Size;
    outInfo.unusedRangeSizeMin = UINT32_MAX;
    outInfo.unusedRangeSizeMax = 0;
  }

  void BlockAllocMap();
  void BlockAllocUnmap();
  DkResult DedicatedAllocMap(DmaAllocator hAllocator, void **ppData);
  void DedicatedAllocUnmap(DmaAllocator hAllocator);

#if DMA_STATS_STRING_ENABLED
  uint32_t GetCreationFrameIndex() const { return m_CreationFrameIndex; }
  uint32_t GetBufferImageUsage() const { return m_BufferImageUsage; }

  void InitBufferImageUsage(uint32_t bufferImageUsage) {
    DMA_ASSERT(m_BufferImageUsage == 0);
    m_BufferImageUsage = bufferImageUsage;
  }

  void PrintParameters(class DmaJsonWriter &json) const;
#endif

private:
  uint32_t m_Alignment;
  uint32_t m_Size;
  void *m_pUserData;
  DMA_ATOMIC_UINT32 m_LastUseFrameIndex;
  uint32_t m_MemoryTypeIndex;
  uint8_t m_Type;              // ALLOCATION_TYPE
  uint8_t m_SuballocationType; // DmaSuballocationType
  // Bit 0x80 is set when allocation was created with
  // DMA_ALLOCATION_CREATE_MAPPED_BIT. Bits with mask 0x7F are reference counter
  // for dmaMapMemory()/dmaUnmapMemory().
  uint8_t m_MapCount;
  uint8_t m_Flags; // enum FLAGS

  // Allocation out of DmaDeviceMemoryBlock.
  struct BlockAllocation {
    DmaDeviceMemoryBlock *m_Block;
    uint32_t m_Offset;
    bool m_CanBecomeLost;
  };

  // Allocation for an object that has its own private DkMemBlock.
  struct DedicatedAllocation {
    DkMemBlock m_hMemory;
    void *m_pMappedData; // Not null means memory is mapped.
  };

  union {
    // Allocation out of DmaDeviceMemoryBlock.
    BlockAllocation m_BlockAllocation;
    // Allocation for an object that has its own private DkMemBlock.
    DedicatedAllocation m_DedicatedAllocation;
  };

#if DMA_STATS_STRING_ENABLED
  uint32_t m_CreationFrameIndex;
  uint32_t m_BufferImageUsage; // 0 if unknown.
#endif

  void FreeUserDataString(DmaAllocator hAllocator);
};

/*
Represents a region of DmaDeviceMemoryBlock that is either assigned and returned
as allocated memory block or free.
*/
struct DmaSuballocation {
  uint32_t offset;
  uint32_t size;
  DmaAllocation hAllocation;
  DmaSuballocationType type;
};

// Comparator for offsets.
struct DmaSuballocationOffsetLess {
  bool operator()(const DmaSuballocation &lhs,
                  const DmaSuballocation &rhs) const {
    return lhs.offset < rhs.offset;
  }
};
struct DmaSuballocationOffsetGreater {
  bool operator()(const DmaSuballocation &lhs,
                  const DmaSuballocation &rhs) const {
    return lhs.offset > rhs.offset;
  }
};

typedef DmaList<DmaSuballocation, DmaStlAllocator<DmaSuballocation>>
    DmaSuballocationList;

// Cost of one additional allocation lost, as equivalent in bytes.
static const uint32_t DMA_LOST_ALLOCATION_COST = 1048576;

enum class DmaAllocationRequestType {
  Normal,
  // Used by "Linear" algorithm.
  UpperAddress,
  EndOf1st,
  EndOf2nd,
};

/*
Parameters of planned allocation inside a DmaDeviceMemoryBlock.

If canMakeOtherLost was false:
- item points to a FREE suballocation.
- itemsToMakeLostCount is 0.

If canMakeOtherLost was true:
- item points to first of sequence of suballocations, which are either FREE,
  or point to DmaAllocations that can become lost.
- itemsToMakeLostCount is the number of DmaAllocations that need to be made lost
for the requested allocation to succeed.
*/
struct DmaAllocationRequest {
  uint32_t offset;
  uint32_t sumFreeSize; // Sum size of free items that overlap with proposed
                        // allocation.
  uint32_t sumItemSize; // Sum size of items to make lost that overlap with
                        // proposed allocation.
  DmaSuballocationList::iterator item;
  size_t itemsToMakeLostCount;
  void *customData;
  DmaAllocationRequestType type;

  uint32_t CalcCost() const {
    return sumItemSize + itemsToMakeLostCount * DMA_LOST_ALLOCATION_COST;
  }
};

/*
Data structure used for bookkeeping of allocations and unused ranges of memory
in a single DkMemBlock block.
*/
class DmaBlockMetadata {
public:
  DmaBlockMetadata(DmaAllocator hAllocator);
  virtual ~DmaBlockMetadata() {}
  virtual void Init(uint32_t size) { m_Size = size; }

  // Validates all data structures inside this object. If not valid, returns
  // false.
  virtual bool Validate() const = 0;
  uint32_t GetSize() const { return m_Size; }
  virtual size_t GetAllocationCount() const = 0;
  virtual uint32_t GetSumFreeSize() const = 0;
  virtual uint32_t GetUnusedRangeSizeMax() const = 0;
  // Returns true if this block is empty - contains only single free
  // suballocation.
  virtual bool IsEmpty() const = 0;

  virtual void CalcAllocationStatInfo(DmaStatInfo &outInfo) const = 0;
  // Shouldn't modify blockCount.
  virtual void AddPoolStats(DmaPoolStats &inoutStats) const = 0;

#if DMA_STATS_STRING_ENABLED
  virtual void PrintDetailedMap(class DmaJsonWriter &json) const = 0;
#endif

  // Tries to find a place for suballocation with given parameters inside this
  // block. If succeeded, fills pAllocationRequest and returns true. If failed,
  // returns false.
  virtual bool CreateAllocationRequest(
      uint32_t currentFrameIndex, uint32_t frameInUseCount, uint32_t allocSize,
      uint32_t allocAlignment, bool upperAddress,
      DmaSuballocationType allocType, bool canMakeOtherLost,
      // Always one of DMA_ALLOCATION_CREATE_STRATEGY_* or
      // DMA_ALLOCATION_INTERNAL_STRATEGY_* flags.
      uint32_t strategy, DmaAllocationRequest *pAllocationRequest) = 0;

  virtual bool
  MakeRequestedAllocationsLost(uint32_t currentFrameIndex,
                               uint32_t frameInUseCount,
                               DmaAllocationRequest *pAllocationRequest) = 0;

  virtual uint32_t MakeAllocationsLost(uint32_t currentFrameIndex,
                                       uint32_t frameInUseCount) = 0;

  virtual DkResult CheckCorruption(const void *pBlockData) = 0;

  // Makes actual allocation based on request. Request must already be checked
  // and valid.
  virtual void Alloc(const DmaAllocationRequest &request,
                     DmaSuballocationType type, uint32_t allocSize,
                     DmaAllocation hAllocation) = 0;

  // Frees suballocation assigned to given memory region.
  virtual void Free(const DmaAllocation allocation) = 0;
  virtual void FreeAtOffset(uint32_t offset) = 0;

protected:
  const DmaAllocationCallbacks *GetAllocationCallbacks() const {
    return m_pAllocationCallbacks;
  }

#if DMA_STATS_STRING_ENABLED
  void PrintDetailedMap_Begin(class DmaJsonWriter &json, uint32_t unusedBytes,
                              size_t allocationCount,
                              size_t unusedRangeCount) const;
  void PrintDetailedMap_Allocation(class DmaJsonWriter &json, uint32_t offset,
                                   DmaAllocation hAllocation) const;
  void PrintDetailedMap_UnusedRange(class DmaJsonWriter &json, uint32_t offset,
                                    uint32_t size) const;
  void PrintDetailedMap_End(class DmaJsonWriter &json) const;
#endif

private:
  uint32_t m_Size;
  const DmaAllocationCallbacks *m_pAllocationCallbacks;
};

#define DMA_VALIDATE(cond)                                                     \
  do {                                                                         \
    if (!(cond)) {                                                             \
      DMA_ASSERT(0 && "Validation failed: " #cond);                            \
      return false;                                                            \
    }                                                                          \
  } while (false)

class DmaBlockMetadata_Generic : public DmaBlockMetadata {
  DMA_CLASS_NO_COPY(DmaBlockMetadata_Generic)
public:
  DmaBlockMetadata_Generic(DmaAllocator hAllocator);
  virtual ~DmaBlockMetadata_Generic();
  virtual void Init(uint32_t size);

  virtual bool Validate() const;
  virtual size_t GetAllocationCount() const {
    return m_Suballocations.size() - m_FreeCount;
  }
  virtual uint32_t GetSumFreeSize() const { return m_SumFreeSize; }
  virtual uint32_t GetUnusedRangeSizeMax() const;
  virtual bool IsEmpty() const;

  virtual void CalcAllocationStatInfo(DmaStatInfo &outInfo) const;
  virtual void AddPoolStats(DmaPoolStats &inoutStats) const;

#if DMA_STATS_STRING_ENABLED
  virtual void PrintDetailedMap(class DmaJsonWriter &json) const;
#endif

  virtual bool
  CreateAllocationRequest(uint32_t currentFrameIndex, uint32_t frameInUseCount,
                          uint32_t allocSize, uint32_t allocAlignment,
                          bool upperAddress, DmaSuballocationType allocType,
                          bool canMakeOtherLost, uint32_t strategy,
                          DmaAllocationRequest *pAllocationRequest);

  virtual bool
  MakeRequestedAllocationsLost(uint32_t currentFrameIndex,
                               uint32_t frameInUseCount,
                               DmaAllocationRequest *pAllocationRequest);

  virtual uint32_t MakeAllocationsLost(uint32_t currentFrameIndex,
                                       uint32_t frameInUseCount);

  virtual DkResult CheckCorruption(const void *pBlockData);

  virtual void Alloc(const DmaAllocationRequest &request,
                     DmaSuballocationType type, uint32_t allocSize,
                     DmaAllocation hAllocation);

  virtual void Free(const DmaAllocation allocation);
  virtual void FreeAtOffset(uint32_t offset);

  ////////////////////////////////////////////////////////////////////////////////
  // For defragmentation

private:
  friend class DmaDefragmentationAlgorithm_Generic;
  friend class DmaDefragmentationAlgorithm_Fast;

  uint32_t m_FreeCount;
  uint32_t m_SumFreeSize;
  DmaSuballocationList m_Suballocations;
  // Suballocations that are free and have size greater than certain threshold.
  // Sorted by size, ascending.
  DmaVector<DmaSuballocationList::iterator,
            DmaStlAllocator<DmaSuballocationList::iterator>>
      m_FreeSuballocationsBySize;

  bool ValidateFreeSuballocationList() const;

  // Checks if requested suballocation with given parameters can be placed in
  // given pFreeSuballocItem. If yes, fills pOffset and returns true. If no,
  // returns false.
  bool CheckAllocation(uint32_t currentFrameIndex, uint32_t frameInUseCount,
                       uint32_t allocSize, uint32_t allocAlignment,
                       DmaSuballocationType allocType,
                       DmaSuballocationList::const_iterator suballocItem,
                       bool canMakeOtherLost, uint32_t *pOffset,
                       size_t *itemsToMakeLostCount, uint32_t *pSumFreeSize,
                       uint32_t *pSumItemSize) const;
  // Given free suballocation, it merges it with following one, which must also
  // be free.
  void MergeFreeWithNext(DmaSuballocationList::iterator item);
  // Releases given suballocation, making it free.
  // Merges it with adjacent free suballocations if applicable.
  // Returns iterator to new free suballocation at this place.
  DmaSuballocationList::iterator
  FreeSuballocation(DmaSuballocationList::iterator suballocItem);
  // Given free suballocation, it inserts it into sorted list of
  // m_FreeSuballocationsBySize if it's suitable.
  void RegisterFreeSuballocation(DmaSuballocationList::iterator item);
  // Given free suballocation, it removes it from sorted list of
  // m_FreeSuballocationsBySize if it's suitable.
  void UnregisterFreeSuballocation(DmaSuballocationList::iterator item);
};

/*
Allocations and their references in internal data structure look like this:

if(m_2ndVectorMode == SECOND_VECTOR_EMPTY):

        0 +-------+
          |       |
          |       |
          |       |
          +-------+
          | Alloc |  1st[m_1stNullItemsBeginCount]
          +-------+
          | Alloc |  1st[m_1stNullItemsBeginCount + 1]
          +-------+
          |  ...  |
          +-------+
          | Alloc |  1st[1st.size() - 1]
          +-------+
          |       |
          |       |
          |       |
GetSize() +-------+

if(m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER):

        0 +-------+
          | Alloc |  2nd[0]
          +-------+
          | Alloc |  2nd[1]
          +-------+
          |  ...  |
          +-------+
          | Alloc |  2nd[2nd.size() - 1]
          +-------+
          |       |
          |       |
          |       |
          +-------+
          | Alloc |  1st[m_1stNullItemsBeginCount]
          +-------+
          | Alloc |  1st[m_1stNullItemsBeginCount + 1]
          +-------+
          |  ...  |
          +-------+
          | Alloc |  1st[1st.size() - 1]
          +-------+
          |       |
GetSize() +-------+

if(m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK):

        0 +-------+
          |       |
          |       |
          |       |
          +-------+
          | Alloc |  1st[m_1stNullItemsBeginCount]
          +-------+
          | Alloc |  1st[m_1stNullItemsBeginCount + 1]
          +-------+
          |  ...  |
          +-------+
          | Alloc |  1st[1st.size() - 1]
          +-------+
          |       |
          |       |
          |       |
          +-------+
          | Alloc |  2nd[2nd.size() - 1]
          +-------+
          |  ...  |
          +-------+
          | Alloc |  2nd[1]
          +-------+
          | Alloc |  2nd[0]
GetSize() +-------+

*/
class DmaBlockMetadata_Linear : public DmaBlockMetadata {
  DMA_CLASS_NO_COPY(DmaBlockMetadata_Linear)
public:
  DmaBlockMetadata_Linear(DmaAllocator hAllocator);
  virtual ~DmaBlockMetadata_Linear();
  virtual void Init(uint32_t size);

  virtual bool Validate() const;
  virtual size_t GetAllocationCount() const;
  virtual uint32_t GetSumFreeSize() const { return m_SumFreeSize; }
  virtual uint32_t GetUnusedRangeSizeMax() const;
  virtual bool IsEmpty() const { return GetAllocationCount() == 0; }

  virtual void CalcAllocationStatInfo(DmaStatInfo &outInfo) const;
  virtual void AddPoolStats(DmaPoolStats &inoutStats) const;

#if DMA_STATS_STRING_ENABLED
  virtual void PrintDetailedMap(class DmaJsonWriter &json) const;
#endif

  virtual bool
  CreateAllocationRequest(uint32_t currentFrameIndex, uint32_t frameInUseCount,
                          uint32_t allocSize, uint32_t allocAlignment,
                          bool upperAddress, DmaSuballocationType allocType,
                          bool canMakeOtherLost, uint32_t strategy,
                          DmaAllocationRequest *pAllocationRequest);

  virtual bool
  MakeRequestedAllocationsLost(uint32_t currentFrameIndex,
                               uint32_t frameInUseCount,
                               DmaAllocationRequest *pAllocationRequest);

  virtual uint32_t MakeAllocationsLost(uint32_t currentFrameIndex,
                                       uint32_t frameInUseCount);

  virtual DkResult CheckCorruption(const void *pBlockData);

  virtual void Alloc(const DmaAllocationRequest &request,
                     DmaSuballocationType type, uint32_t allocSize,
                     DmaAllocation hAllocation);

  virtual void Free(const DmaAllocation allocation);
  virtual void FreeAtOffset(uint32_t offset);

private:
  /*
  There are two suballocation vectors, used in ping-pong way.
  The one with index m_1stVectorIndex is called 1st.
  The one with index (m_1stVectorIndex ^ 1) is called 2nd.
  2nd can be non-empty only when 1st is not empty.
  When 2nd is not empty, m_2ndVectorMode indicates its mode of operation.
  */
  typedef DmaVector<DmaSuballocation, DmaStlAllocator<DmaSuballocation>>
      SuballocationVectorType;

  enum SECOND_VECTOR_MODE {
    SECOND_VECTOR_EMPTY,
    /*
    Suballocations in 2nd vector are created later than the ones in 1st, but
    they all have smaller offset.
    */
    SECOND_VECTOR_RING_BUFFER,
    /*
    Suballocations in 2nd vector are upper side of double stack.
    They all have offsets higher than those in 1st vector.
    Top of this stack means smaller offsets, but higher indices in this vector.
    */
    SECOND_VECTOR_DOUBLE_STACK,
  };

  uint32_t m_SumFreeSize;
  SuballocationVectorType m_Suballocations0, m_Suballocations1;
  uint32_t m_1stVectorIndex;
  SECOND_VECTOR_MODE m_2ndVectorMode;

  SuballocationVectorType &AccessSuballocations1st() {
    return m_1stVectorIndex ? m_Suballocations1 : m_Suballocations0;
  }
  SuballocationVectorType &AccessSuballocations2nd() {
    return m_1stVectorIndex ? m_Suballocations0 : m_Suballocations1;
  }
  const SuballocationVectorType &AccessSuballocations1st() const {
    return m_1stVectorIndex ? m_Suballocations1 : m_Suballocations0;
  }
  const SuballocationVectorType &AccessSuballocations2nd() const {
    return m_1stVectorIndex ? m_Suballocations0 : m_Suballocations1;
  }

  // Number of items in 1st vector with hAllocation = null at the beginning.
  size_t m_1stNullItemsBeginCount;
  // Number of other items in 1st vector with hAllocation = null somewhere in
  // the middle.
  size_t m_1stNullItemsMiddleCount;
  // Number of items in 2nd vector with hAllocation = null.
  size_t m_2ndNullItemsCount;

  bool ShouldCompact1st() const;
  void CleanupAfterFree();

  bool CreateAllocationRequest_LowerAddress(
      uint32_t currentFrameIndex, uint32_t frameInUseCount, uint32_t allocSize,
      uint32_t allocAlignment, DmaSuballocationType allocType,
      bool canMakeOtherLost, uint32_t strategy,
      DmaAllocationRequest *pAllocationRequest);
  bool CreateAllocationRequest_UpperAddress(
      uint32_t currentFrameIndex, uint32_t frameInUseCount, uint32_t allocSize,
      uint32_t allocAlignment, DmaSuballocationType allocType,
      bool canMakeOtherLost, uint32_t strategy,
      DmaAllocationRequest *pAllocationRequest);
};

/*
- GetSize() is the original size of allocated memory block.
- m_UsableSize is this size aligned down to a power of two.
  All allocations and calculations happen relative to m_UsableSize.
- GetUnusableSize() is the difference between them.
  It is repoted as separate, unused range, not available for allocations.

Node at level 0 has size = m_UsableSize.
Each next level contains nodes with size 2 times smaller than current level.
m_LevelCount is the maximum number of levels to use in the current object.
*/
class DmaBlockMetadata_Buddy : public DmaBlockMetadata {
  DMA_CLASS_NO_COPY(DmaBlockMetadata_Buddy)
public:
  DmaBlockMetadata_Buddy(DmaAllocator hAllocator);
  virtual ~DmaBlockMetadata_Buddy();
  virtual void Init(uint32_t size);

  virtual bool Validate() const;
  virtual size_t GetAllocationCount() const { return m_AllocationCount; }
  virtual uint32_t GetSumFreeSize() const {
    return m_SumFreeSize + GetUnusableSize();
  }
  virtual uint32_t GetUnusedRangeSizeMax() const;
  virtual bool IsEmpty() const { return m_Root->type == Node::TYPE_FREE; }

  virtual void CalcAllocationStatInfo(DmaStatInfo &outInfo) const;
  virtual void AddPoolStats(DmaPoolStats &inoutStats) const;

#if DMA_STATS_STRING_ENABLED
  virtual void PrintDetailedMap(class DmaJsonWriter &json) const;
#endif

  virtual bool
  CreateAllocationRequest(uint32_t currentFrameIndex, uint32_t frameInUseCount,
                          uint32_t allocSize, uint32_t allocAlignment,
                          bool upperAddress, DmaSuballocationType allocType,
                          bool canMakeOtherLost, uint32_t strategy,
                          DmaAllocationRequest *pAllocationRequest);

  virtual bool
  MakeRequestedAllocationsLost(uint32_t currentFrameIndex,
                               uint32_t frameInUseCount,
                               DmaAllocationRequest *pAllocationRequest);

  virtual uint32_t MakeAllocationsLost(uint32_t currentFrameIndex,
                                       uint32_t frameInUseCount);

  virtual DkResult CheckCorruption(const void *pBlockData) {
    return DkResult_Fail;
  }

  virtual void Alloc(const DmaAllocationRequest &request,
                     DmaSuballocationType type, uint32_t allocSize,
                     DmaAllocation hAllocation);

  virtual void Free(const DmaAllocation allocation) {
    FreeAtOffset(allocation, allocation->GetOffset());
  }
  virtual void FreeAtOffset(uint32_t offset) { FreeAtOffset(DMA_NULL, offset); }

private:
  static const uint32_t MIN_NODE_SIZE = 32;
  static const size_t MAX_LEVELS = 30;

  struct ValidationContext {
    size_t calculatedAllocationCount;
    size_t calculatedFreeCount;
    uint32_t calculatedSumFreeSize;

    ValidationContext()
        : calculatedAllocationCount(0), calculatedFreeCount(0),
          calculatedSumFreeSize(0) {}
  };

  struct Node {
    uint32_t offset;
    enum TYPE { TYPE_FREE, TYPE_ALLOCATION, TYPE_SPLIT, TYPE_COUNT } type;
    Node *parent;
    Node *buddy;

    union {
      struct {
        Node *prev;
        Node *next;
      } free;
      struct {
        DmaAllocation alloc;
      } allocation;
      struct {
        Node *leftChild;
      } split;
    };
  };

  // Size of the memory block aligned down to a power of two.
  uint32_t m_UsableSize;
  uint32_t m_LevelCount;

  Node *m_Root;
  struct {
    Node *front;
    Node *back;
  } m_FreeList[MAX_LEVELS];
  // Number of nodes in the tree with type == TYPE_ALLOCATION.
  size_t m_AllocationCount;
  // Number of nodes in the tree with type == TYPE_FREE.
  size_t m_FreeCount;
  // This includes space wasted due to internal fragmentation. Doesn't include
  // unusable size.
  uint32_t m_SumFreeSize;

  uint32_t GetUnusableSize() const { return GetSize() - m_UsableSize; }
  void DeleteNode(Node *node);
  bool ValidateNode(ValidationContext &ctx, const Node *parent,
                    const Node *curr, uint32_t level,
                    uint32_t levelNodeSize) const;
  uint32_t AllocSizeToLevel(uint32_t allocSize) const;
  inline uint32_t LevelToNodeSize(uint32_t level) const {
    return m_UsableSize >> level;
  }
  // Alloc passed just for validation. Can be null.
  void FreeAtOffset(const DmaAllocation alloc, uint32_t offset);
  void CalcAllocationStatInfoNode(DmaStatInfo &outInfo, const Node *node,
                                  uint32_t levelNodeSize) const;
  // Adds node to the front of FreeList at given level.
  // node->type must be FREE.
  // node->free.prev, next can be undefined.
  void AddToFreeListFront(uint32_t level, Node *node);
  // Removes node from FreeList at given level.
  // node->type must be FREE.
  // node->free.prev, next stay untouched.
  void RemoveFromFreeList(uint32_t level, Node *node);

#if DMA_STATS_STRING_ENABLED
  void PrintDetailedMapNode(class DmaJsonWriter &json, const Node *node,
                            uint32_t levelNodeSize) const;
#endif
};

/*
Represents a single block of device memory (`DkMemBlock`) with all the
data about its regions (aka suballocations, #DmaAllocation), assigned and free.

Thread-safety: This class must be externally synchronized.
*/
class DmaDeviceMemoryBlock {
  DMA_CLASS_NO_COPY(DmaDeviceMemoryBlock)
public:
  DmaBlockMetadata *m_pMetadata;

  DmaDeviceMemoryBlock(DmaAllocator hAllocator);

  ~DmaDeviceMemoryBlock() {
    DMA_ASSERT(m_MapCount == 0 &&
               "DkMemBlock block is being destroyed while it is still mapped.");
    DMA_ASSERT(m_hMemory == DMA_NULL_HANDLE);
  }

  // Always call after construction.
  void Init(DmaAllocator hAllocator, DmaPool hParentPool,
            uint32_t newMemoryTypeIndex, DkMemBlock newMemory, uint32_t newSize,
            uint32_t id, uint32_t algorithm);
  // Always call before destruction.
  void Destroy(DmaAllocator allocator);

  DmaPool GetParentPool() const { return m_hParentPool; }
  DkMemBlock GetDeviceMemory() const { return m_hMemory; }
  uint32_t GetMemoryTypeIndex() const { return m_MemoryTypeIndex; }
  uint32_t GetId() const { return m_Id; }
  void *GetMappedData() const { return m_pMappedData; }

  // Validates all data structures inside this object. If not valid, returns
  // false.
  bool Validate() const;

  DkResult CheckCorruption(DmaAllocator hAllocator);

  // ppData can be null.
  DkResult Map(DmaAllocator hAllocator, uint32_t count, void **ppData);
  void Unmap(DmaAllocator hAllocator, uint32_t count);

  DkResult WriteMagicValueAroundAllocation(DmaAllocator hAllocator,
                                           uint32_t allocOffset,
                                           uint32_t allocSize);
  DkResult ValidateMagicValueAroundAllocation(DmaAllocator hAllocator,
                                              uint32_t allocOffset,
                                              uint32_t allocSize);

private:
  DmaPool m_hParentPool; // DMA_NULL_HANDLE if not belongs to custom pool.
  uint32_t m_MemoryTypeIndex;
  uint32_t m_Id;
  DkMemBlock m_hMemory;

  /*
  Protects access to m_hMemory so it's not used by multiple threads
  simultaneously, e.g. vkMapMemory, vkBindBufferMemory. Also protects
  m_MapCount, m_pMappedData. Allocations, deallocations, any change in
  m_pMetadata is protected by parent's DmaBlockVector::m_Mutex.
  */
  DMA_MUTEX m_Mutex;
  uint32_t m_MapCount;
  void *m_pMappedData;
};

struct DmaPointerLess {
  bool operator()(const void *lhs, const void *rhs) const { return lhs < rhs; }
};

struct DmaDefragmentationMove {
  size_t srcBlockIndex;
  size_t dstBlockIndex;
  uint32_t srcOffset;
  uint32_t dstOffset;
  uint32_t size;
};

class DmaDefragmentationAlgorithm;

/*
Sequence of DmaDeviceMemoryBlock. Represents memory blocks allocated for a
specific Vulkan memory type.

Synchronized internally with a mutex.
*/
struct DmaBlockVector {
  DMA_CLASS_NO_COPY(DmaBlockVector)
public:
  DmaBlockVector(DmaAllocator hAllocator, DmaPool hParentPool,
                 uint32_t memoryTypeIndex, uint32_t preferredBlockSize,
                 size_t minBlockCount, size_t maxBlockCount,
                 uint32_t frameInUseCount, uint32_t algorithm);
  ~DmaBlockVector();

  DkResult CreateMinBlocks();

  DmaAllocator GetAllocator() const { return m_hAllocator; }
  DmaPool GetParentPool() const { return m_hParentPool; }
  bool IsCustomPool() const { return m_hParentPool != DMA_NULL; }
  uint32_t GetMemoryTypeIndex() const { return m_MemoryTypeIndex; }
  uint32_t GetPreferredBlockSize() const { return m_PreferredBlockSize; }
  uint32_t GetFrameInUseCount() const { return m_FrameInUseCount; }
  uint32_t GetAlgorithm() const { return m_Algorithm; }

  void GetPoolStats(DmaPoolStats *pStats);

  bool IsEmpty();
  bool IsCorruptionDetectionEnabled() const;

  DkResult Allocate(uint32_t currentFrameIndex, uint32_t size,
                    uint32_t alignment,
                    const DmaAllocationCreateInfo &createInfo,
                    DmaSuballocationType suballocType, size_t allocationCount,
                    DmaAllocation *pAllocations);

  void Free(const DmaAllocation hAllocation);

  // Adds statistics of this BlockVector to pStats.
  void AddStats(DmaStats *pStats);

#if DMA_STATS_STRING_ENABLED
  void PrintDetailedMap(class DmaJsonWriter &json);
#endif

  void MakePoolAllocationsLost(uint32_t currentFrameIndex,
                               size_t *pLostAllocationCount);
  DkResult CheckCorruption();

  // Saves results in pCtx->res.
  void Defragment(class DmaBlockVectorDefragmentationContext *pCtx,
                  DmaDefragmentationStats *pStats, uint32_t &maxCpuBytesToMove,
                  uint32_t &maxCpuAllocationsToMove,
                  uint32_t &maxGpuBytesToMove,
                  uint32_t &maxGpuAllocationsToMove, DkCmdBuf commandBuffer);
  void DefragmentationEnd(class DmaBlockVectorDefragmentationContext *pCtx,
                          DmaDefragmentationStats *pStats);

  ////////////////////////////////////////////////////////////////////////////////
  // To be used only while the m_Mutex is locked. Used during defragmentation.

  size_t GetBlockCount() const { return m_Blocks.size(); }
  DmaDeviceMemoryBlock *GetBlock(size_t index) const { return m_Blocks[index]; }
  size_t CalcAllocationCount() const;

private:
  friend class DmaDefragmentationAlgorithm_Generic;

  const DmaAllocator m_hAllocator;
  const DmaPool m_hParentPool;
  const uint32_t m_MemoryTypeIndex;
  const uint32_t m_PreferredBlockSize;
  const size_t m_MinBlockCount;
  const size_t m_MaxBlockCount;
  const uint32_t m_FrameInUseCount;
  const uint32_t m_Algorithm;
  DMA_RW_MUTEX m_Mutex;

  /* There can be at most one allocation that is completely empty (except when
  minBlockCount > 0) - a hysteresis to avoid pessimistic case of alternating
  creation and destruction of a DkMemBlock. */
  bool m_HasEmptyBlock;
  // Incrementally sorted by sumFreeSize, ascending.
  DmaVector<DmaDeviceMemoryBlock *, DmaStlAllocator<DmaDeviceMemoryBlock *>>
      m_Blocks;
  uint32_t m_NextBlockId;

  uint32_t CalcMaxBlockSize() const;

  // Finds and removes given block from vector.
  void Remove(DmaDeviceMemoryBlock *pBlock);

  // Performs single step in sorting m_Blocks. They may not be fully sorted
  // after this call.
  void IncrementallySortBlocks();

  DkResult AllocatePage(uint32_t currentFrameIndex, uint32_t size,
                        uint32_t alignment,
                        const DmaAllocationCreateInfo &createInfo,
                        DmaSuballocationType suballocType,
                        DmaAllocation *pAllocation);

  // To be used only without CAN_MAKE_OTHER_LOST flag.
  DkResult AllocateFromBlock(DmaDeviceMemoryBlock *pBlock,
                             uint32_t currentFrameIndex, uint32_t size,
                             uint32_t alignment,
                             DmaAllocationCreateFlags allocFlags,
                             void *pUserData, DmaSuballocationType suballocType,
                             uint32_t strategy, DmaAllocation *pAllocation);

  DkResult CreateBlock(uint32_t blockSize, size_t *pNewBlockIndex);

  // Saves result to pCtx->res.
  void ApplyDefragmentationMovesCpu(
      class DmaBlockVectorDefragmentationContext *pDefragCtx,
      const DmaVector<DmaDefragmentationMove,
                      DmaStlAllocator<DmaDefragmentationMove>> &moves);
  // Saves result to pCtx->res.
  void ApplyDefragmentationMovesGpu(
      class DmaBlockVectorDefragmentationContext *pDefragCtx,
      const DmaVector<DmaDefragmentationMove,
                      DmaStlAllocator<DmaDefragmentationMove>> &moves,
      DkCmdBuf commandBuffer);

  /*
  Used during defragmentation. pDefragmentationStats is optional. It's in/out
  - updated with new data.
  */
  void FreeEmptyBlocks(DmaDefragmentationStats *pDefragmentationStats);

  void UpdateHasEmptyBlock();
};

struct DmaPool_T {
  DMA_CLASS_NO_COPY(DmaPool_T)
public:
  DmaBlockVector m_BlockVector;

  DmaPool_T(DmaAllocator hAllocator, const DmaPoolCreateInfo &createInfo,
            uint32_t preferredBlockSize);
  ~DmaPool_T();

  uint32_t GetId() const { return m_Id; }
  void SetId(uint32_t id) {
    DMA_ASSERT(m_Id == 0);
    m_Id = id;
  }

  const char *GetName() const { return m_Name; }
  void SetName(const char *pName);

#if DMA_STATS_STRING_ENABLED
  // void PrintDetailedMap(class DmaStringBuilder& sb);
#endif

private:
  uint32_t m_Id;
  char *m_Name;
};

/*
Performs defragmentation:

- Updates `pBlockVector->m_pMetadata`.
- Updates allocations by calling ChangeBlockAllocation() or ChangeOffset().
- Does not move actual data, only returns requested moves as `moves`.
*/
class DmaDefragmentationAlgorithm {
  DMA_CLASS_NO_COPY(DmaDefragmentationAlgorithm)
public:
  DmaDefragmentationAlgorithm(DmaAllocator hAllocator,
                              DmaBlockVector *pBlockVector,
                              uint32_t currentFrameIndex)
      : m_hAllocator(hAllocator), m_pBlockVector(pBlockVector),
        m_CurrentFrameIndex(currentFrameIndex) {}
  virtual ~DmaDefragmentationAlgorithm() {}

  virtual void AddAllocation(DmaAllocation hAlloc, bool *pChanged) = 0;
  virtual void AddAll() = 0;

  virtual DkResult Defragment(
      DmaVector<DmaDefragmentationMove, DmaStlAllocator<DmaDefragmentationMove>>
          &moves,
      uint32_t maxBytesToMove, uint32_t maxAllocationsToMove) = 0;

  virtual uint32_t GetBytesMoved() const = 0;
  virtual uint32_t GetAllocationsMoved() const = 0;

protected:
  DmaAllocator const m_hAllocator;
  DmaBlockVector *const m_pBlockVector;
  const uint32_t m_CurrentFrameIndex;

  struct AllocationInfo {
    DmaAllocation m_hAllocation;
    bool *m_pChanged;

    AllocationInfo() : m_hAllocation(DMA_NULL_HANDLE), m_pChanged(DMA_NULL) {}
    AllocationInfo(DmaAllocation hAlloc, bool *pChanged)
        : m_hAllocation(hAlloc), m_pChanged(pChanged) {}
  };
};

class DmaDefragmentationAlgorithm_Generic : public DmaDefragmentationAlgorithm {
  DMA_CLASS_NO_COPY(DmaDefragmentationAlgorithm_Generic)
public:
  DmaDefragmentationAlgorithm_Generic(DmaAllocator hAllocator,
                                      DmaBlockVector *pBlockVector,
                                      uint32_t currentFrameIndex,
                                      bool overlappingMoveSupported);
  virtual ~DmaDefragmentationAlgorithm_Generic();

  virtual void AddAllocation(DmaAllocation hAlloc, bool *pChanged);
  virtual void AddAll() { m_AllAllocations = true; }

  virtual DkResult Defragment(
      DmaVector<DmaDefragmentationMove, DmaStlAllocator<DmaDefragmentationMove>>
          &moves,
      uint32_t maxBytesToMove, uint32_t maxAllocationsToMove);

  virtual uint32_t GetBytesMoved() const { return m_BytesMoved; }
  virtual uint32_t GetAllocationsMoved() const { return m_AllocationsMoved; }

private:
  uint32_t m_AllocationCount;
  bool m_AllAllocations;

  uint32_t m_BytesMoved;
  uint32_t m_AllocationsMoved;

  struct AllocationInfoSizeGreater {
    bool operator()(const AllocationInfo &lhs,
                    const AllocationInfo &rhs) const {
      return lhs.m_hAllocation->GetSize() > rhs.m_hAllocation->GetSize();
    }
  };

  struct AllocationInfoOffsetGreater {
    bool operator()(const AllocationInfo &lhs,
                    const AllocationInfo &rhs) const {
      return lhs.m_hAllocation->GetOffset() > rhs.m_hAllocation->GetOffset();
    }
  };

  struct BlockInfo {
    size_t m_OriginalBlockIndex;
    DmaDeviceMemoryBlock *m_pBlock;
    bool m_HasNonMovableAllocations;
    DmaVector<AllocationInfo, DmaStlAllocator<AllocationInfo>> m_Allocations;

    BlockInfo(const DmaAllocationCallbacks *pAllocationCallbacks)
        : m_OriginalBlockIndex(SIZE_MAX), m_pBlock(DMA_NULL),
          m_HasNonMovableAllocations(true),
          m_Allocations(pAllocationCallbacks) {}

    void CalcHasNonMovableAllocations() {
      const size_t blockAllocCount =
          m_pBlock->m_pMetadata->GetAllocationCount();
      const size_t defragmentAllocCount = m_Allocations.size();
      m_HasNonMovableAllocations = blockAllocCount != defragmentAllocCount;
    }

    void SortAllocationsBySizeDescending() {
      DMA_SORT(m_Allocations.begin(), m_Allocations.end(),
               AllocationInfoSizeGreater());
    }

    void SortAllocationsByOffsetDescending() {
      DMA_SORT(m_Allocations.begin(), m_Allocations.end(),
               AllocationInfoOffsetGreater());
    }
  };

  struct BlockPointerLess {
    bool operator()(const BlockInfo *pLhsBlockInfo,
                    const DmaDeviceMemoryBlock *pRhsBlock) const {
      return pLhsBlockInfo->m_pBlock < pRhsBlock;
    }
    bool operator()(const BlockInfo *pLhsBlockInfo,
                    const BlockInfo *pRhsBlockInfo) const {
      return pLhsBlockInfo->m_pBlock < pRhsBlockInfo->m_pBlock;
    }
  };

  // 1. Blocks with some non-movable allocations go first.
  // 2. Blocks with smaller sumFreeSize go first.
  struct BlockInfoCompareMoveDestination {
    bool operator()(const BlockInfo *pLhsBlockInfo,
                    const BlockInfo *pRhsBlockInfo) const {
      if (pLhsBlockInfo->m_HasNonMovableAllocations &&
          !pRhsBlockInfo->m_HasNonMovableAllocations) {
        return true;
      }
      if (!pLhsBlockInfo->m_HasNonMovableAllocations &&
          pRhsBlockInfo->m_HasNonMovableAllocations) {
        return false;
      }
      if (pLhsBlockInfo->m_pBlock->m_pMetadata->GetSumFreeSize() <
          pRhsBlockInfo->m_pBlock->m_pMetadata->GetSumFreeSize()) {
        return true;
      }
      return false;
    }
  };

  typedef DmaVector<BlockInfo *, DmaStlAllocator<BlockInfo *>> BlockInfoVector;
  BlockInfoVector m_Blocks;

  DkResult DefragmentRound(
      DmaVector<DmaDefragmentationMove, DmaStlAllocator<DmaDefragmentationMove>>
          &moves,
      uint32_t maxBytesToMove, uint32_t maxAllocationsToMove);

  size_t CalcBlocksWithNonMovableCount() const;

  static bool MoveMakesSense(size_t dstBlockIndex, uint32_t dstOffset,
                             size_t srcBlockIndex, uint32_t srcOffset);
};

class DmaDefragmentationAlgorithm_Fast : public DmaDefragmentationAlgorithm {
  DMA_CLASS_NO_COPY(DmaDefragmentationAlgorithm_Fast)
public:
  DmaDefragmentationAlgorithm_Fast(DmaAllocator hAllocator,
                                   DmaBlockVector *pBlockVector,
                                   uint32_t currentFrameIndex,
                                   bool overlappingMoveSupported);
  virtual ~DmaDefragmentationAlgorithm_Fast();

  virtual void AddAllocation(DmaAllocation hAlloc, bool *pChanged) {
    ++m_AllocationCount;
  }
  virtual void AddAll() { m_AllAllocations = true; }

  virtual DkResult Defragment(
      DmaVector<DmaDefragmentationMove, DmaStlAllocator<DmaDefragmentationMove>>
          &moves,
      uint32_t maxBytesToMove, uint32_t maxAllocationsToMove);

  virtual uint32_t GetBytesMoved() const { return m_BytesMoved; }
  virtual uint32_t GetAllocationsMoved() const { return m_AllocationsMoved; }

private:
  struct BlockInfo {
    size_t origBlockIndex;
  };

  class FreeSpaceDatabase {
  public:
    FreeSpaceDatabase() {
      FreeSpace s = {};
      s.blockInfoIndex = SIZE_MAX;
      for (size_t i = 0; i < MAX_COUNT; ++i) {
        m_FreeSpaces[i] = s;
      }
    }

    void Register(size_t blockInfoIndex, uint32_t offset, uint32_t size) {
      if (size < DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER) {
        return;
      }

      // Find first invalid or the smallest structure.
      size_t bestIndex = SIZE_MAX;
      for (size_t i = 0; i < MAX_COUNT; ++i) {
        // Empty structure.
        if (m_FreeSpaces[i].blockInfoIndex == SIZE_MAX) {
          bestIndex = i;
          break;
        }
        if (m_FreeSpaces[i].size < size &&
            (bestIndex == SIZE_MAX ||
             m_FreeSpaces[bestIndex].size > m_FreeSpaces[i].size)) {
          bestIndex = i;
        }
      }

      if (bestIndex != SIZE_MAX) {
        m_FreeSpaces[bestIndex].blockInfoIndex = blockInfoIndex;
        m_FreeSpaces[bestIndex].offset = offset;
        m_FreeSpaces[bestIndex].size = size;
      }
    }

    bool Fetch(uint32_t alignment, uint32_t size, size_t &outBlockInfoIndex,
               uint32_t &outDstOffset) {
      size_t bestIndex = SIZE_MAX;
      uint32_t bestFreeSpaceAfter = 0;
      for (size_t i = 0; i < MAX_COUNT; ++i) {
        // Structure is valid.
        if (m_FreeSpaces[i].blockInfoIndex != SIZE_MAX) {
          const uint32_t dstOffset =
              DmaAlignUp(m_FreeSpaces[i].offset, alignment);
          // Allocation fits into this structure.
          if (dstOffset + size <=
              m_FreeSpaces[i].offset + m_FreeSpaces[i].size) {
            const uint32_t freeSpaceAfter =
                (m_FreeSpaces[i].offset + m_FreeSpaces[i].size) -
                (dstOffset + size);
            if (bestIndex == SIZE_MAX || freeSpaceAfter > bestFreeSpaceAfter) {
              bestIndex = i;
              bestFreeSpaceAfter = freeSpaceAfter;
            }
          }
        }
      }

      if (bestIndex != SIZE_MAX) {
        outBlockInfoIndex = m_FreeSpaces[bestIndex].blockInfoIndex;
        outDstOffset = DmaAlignUp(m_FreeSpaces[bestIndex].offset, alignment);

        if (bestFreeSpaceAfter >= DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER) {
          // Leave this structure for remaining empty space.
          const uint32_t alignmentPlusSize =
              (outDstOffset - m_FreeSpaces[bestIndex].offset) + size;
          m_FreeSpaces[bestIndex].offset += alignmentPlusSize;
          m_FreeSpaces[bestIndex].size -= alignmentPlusSize;
        } else {
          // This structure becomes invalid.
          m_FreeSpaces[bestIndex].blockInfoIndex = SIZE_MAX;
        }

        return true;
      }

      return false;
    }

  private:
    static const size_t MAX_COUNT = 4;

    struct FreeSpace {
      size_t blockInfoIndex; // SIZE_MAX means this structure is invalid.
      uint32_t offset;
      uint32_t size;
    } m_FreeSpaces[MAX_COUNT];
  };

  const bool m_OverlappingMoveSupported;

  uint32_t m_AllocationCount;
  bool m_AllAllocations;

  uint32_t m_BytesMoved;
  uint32_t m_AllocationsMoved;

  DmaVector<BlockInfo, DmaStlAllocator<BlockInfo>> m_BlockInfos;

  void PreprocessMetadata();
  void PostprocessMetadata();
  void InsertSuballoc(DmaBlockMetadata_Generic *pMetadata,
                      const DmaSuballocation &suballoc);
};

struct DmaBlockDefragmentationContext {
  enum BLOCK_FLAG {
    BLOCK_FLAG_USED = 0x00000001,
  };
  uint32_t flags;
  DkBufExtents hBuffer;
};

class DmaBlockVectorDefragmentationContext {
  DMA_CLASS_NO_COPY(DmaBlockVectorDefragmentationContext)
public:
  DkResult res;
  bool mutexLocked;
  DmaVector<DmaBlockDefragmentationContext,
            DmaStlAllocator<DmaBlockDefragmentationContext>>
      blockContexts;

  DmaBlockVectorDefragmentationContext(DmaAllocator hAllocator,
                                       DmaPool hCustomPool, // Optional.
                                       DmaBlockVector *pBlockVector,
                                       uint32_t currFrameIndex);
  ~DmaBlockVectorDefragmentationContext();

  DmaPool GetCustomPool() const { return m_hCustomPool; }
  DmaBlockVector *GetBlockVector() const { return m_pBlockVector; }
  DmaDefragmentationAlgorithm *GetAlgorithm() const { return m_pAlgorithm; }

  void AddAllocation(DmaAllocation hAlloc, bool *pChanged);
  void AddAll() { m_AllAllocations = true; }

  void Begin(bool overlappingMoveSupported);

private:
  const DmaAllocator m_hAllocator;
  // Null if not from custom pool.
  const DmaPool m_hCustomPool;
  // Redundant, for convenience not to fetch from m_hCustomPool->m_BlockVector
  // or m_hAllocator->m_pBlockVectors.
  DmaBlockVector *const m_pBlockVector;
  const uint32_t m_CurrFrameIndex;
  // Owner of this object.
  DmaDefragmentationAlgorithm *m_pAlgorithm;

  struct AllocInfo {
    DmaAllocation hAlloc;
    bool *pChanged;
  };
  // Used between constructor and Begin.
  DmaVector<AllocInfo, DmaStlAllocator<AllocInfo>> m_Allocations;
  bool m_AllAllocations;
};

struct DmaDefragmentationContext_T {
private:
  DMA_CLASS_NO_COPY(DmaDefragmentationContext_T)
public:
  DmaDefragmentationContext_T(DmaAllocator hAllocator, uint32_t currFrameIndex,
                              uint32_t flags, DmaDefragmentationStats *pStats);
  ~DmaDefragmentationContext_T();

  void AddPools(uint32_t poolCount, DmaPool *pPools);
  void AddAllocations(uint32_t allocationCount, DmaAllocation *pAllocations,
                      bool *pAllocationsChanged);

  /*
  Returns:
  - `DkResult_Success` if succeeded and object can be destroyed immediately.
  - `DkResult_Fail` if succeeded but the object must remain alive until
  dmaDefragmentationEnd().
  - Negative value if error occured and object can be destroyed immediately.
  */
  DkResult Defragment(uint32_t maxCpuBytesToMove,
                      uint32_t maxCpuAllocationsToMove,
                      uint32_t maxGpuBytesToMove,
                      uint32_t maxGpuAllocationsToMove, DkCmdBuf commandBuffer,
                      DmaDefragmentationStats *pStats);

private:
  const DmaAllocator m_hAllocator;
  const uint32_t m_CurrFrameIndex;
  const uint32_t m_Flags;
  DmaDefragmentationStats *const m_pStats;
  // Owner of these objects.
  DmaBlockVectorDefragmentationContext
      *m_DefaultPoolContexts[DMA_NUM_MEMORY_TYPES];
  // Owner of these objects.
  DmaVector<DmaBlockVectorDefragmentationContext *,
            DmaStlAllocator<DmaBlockVectorDefragmentationContext *>>
      m_CustomPoolContexts;
};

#if DMA_RECORDING_ENABLED

class DmaRecorder {
public:
  DmaRecorder();
  DkResult Init(const DmaRecordSettings &settings, bool useMutex);
  void WriteConfiguration(const VkPhysicalDeviceProperties &devProps,
                          const VkPhysicalDeviceMemoryProperties &memProps,
                          uint32_t vulkanApiVersion,
                          bool dedicatedAllocationExtensionEnabled,
                          bool bindMemory2ExtensionEnabled,
                          bool memoryBudgetExtensionEnabled);
  ~DmaRecorder();

  void RecordCreateAllocator(uint32_t frameIndex);
  void RecordDestroyAllocator(uint32_t frameIndex);
  void RecordCreatePool(uint32_t frameIndex,
                        const DmaPoolCreateInfo &createInfo, DmaPool pool);
  void RecordDestroyPool(uint32_t frameIndex, DmaPool pool);
  void RecordAllocateMemory(uint32_t frameIndex,
                            const DmaMemoryRequirements &vkMemReq,
                            const DmaAllocationCreateInfo &createInfo,
                            DmaAllocation allocation);
  void RecordAllocateMemoryPages(uint32_t frameIndex,
                                 const DmaMemoryRequirements &vkMemReq,
                                 const DmaAllocationCreateInfo &createInfo,
                                 uint64_t allocationCount,
                                 const DmaAllocation *pAllocations);
  void RecordAllocateMemoryForBuffer(uint32_t frameIndex,
                                     const DmaMemoryRequirements &vkMemReq,
                                     bool requiresDedicatedAllocation,
                                     bool prefersDedicatedAllocation,
                                     const DmaAllocationCreateInfo &createInfo,
                                     DmaAllocation allocation);
  void RecordAllocateMemoryForImage(uint32_t frameIndex,
                                    const DmaMemoryRequirements &vkMemReq,
                                    bool requiresDedicatedAllocation,
                                    bool prefersDedicatedAllocation,
                                    const DmaAllocationCreateInfo &createInfo,
                                    DmaAllocation allocation);
  void RecordFreeMemory(uint32_t frameIndex, DmaAllocation allocation);
  void RecordFreeMemoryPages(uint32_t frameIndex, uint64_t allocationCount,
                             const DmaAllocation *pAllocations);
  void RecordSetAllocationUserData(uint32_t frameIndex,
                                   DmaAllocation allocation,
                                   const void *pUserData);
  void RecordCreateLostAllocation(uint32_t frameIndex,
                                  DmaAllocation allocation);
  void RecordMapMemory(uint32_t frameIndex, DmaAllocation allocation);
  void RecordUnmapMemory(uint32_t frameIndex, DmaAllocation allocation);
  void RecordFlushAllocation(uint32_t frameIndex, DmaAllocation allocation,
                             uint32_t offset, uint32_t size);
  void RecordInvalidateAllocation(uint32_t frameIndex, DmaAllocation allocation,
                                  uint32_t offset, uint32_t size);
  void RecordCreateBuffer(uint32_t frameIndex,
                          const VkBufferCreateInfo &bufCreateInfo,
                          const DmaAllocationCreateInfo &allocCreateInfo,
                          DmaAllocation allocation);
  void RecordCreateImage(uint32_t frameIndex,
                         const VkImageCreateInfo &imageCreateInfo,
                         const DmaAllocationCreateInfo &allocCreateInfo,
                         DmaAllocation allocation);
  void RecordDestroyBuffer(uint32_t frameIndex, DmaAllocation allocation);
  void RecordDestroyImage(uint32_t frameIndex, DmaAllocation allocation);
  void RecordTouchAllocation(uint32_t frameIndex, DmaAllocation allocation);
  void RecordGetAllocationInfo(uint32_t frameIndex, DmaAllocation allocation);
  void RecordMakePoolAllocationsLost(uint32_t frameIndex, DmaPool pool);
  void RecordDefragmentationBegin(uint32_t frameIndex,
                                  const DmaDefragmentationInfo2 &info,
                                  DmaDefragmentationContext ctx);
  void RecordDefragmentationEnd(uint32_t frameIndex,
                                DmaDefragmentationContext ctx);
  void RecordSetPoolName(uint32_t frameIndex, DmaPool pool, const char *name);

private:
  struct CallParams {
    uint32_t threadId;
    double time;
  };

  class UserDataString {
  public:
    UserDataString(DmaAllocationCreateFlags allocFlags, const void *pUserData);
    const char *GetString() const { return m_Str; }

  private:
    char m_PtrStr[17];
    const char *m_Str;
  };

  bool m_UseMutex;
  DmaRecordFlags m_Flags;
  FILE *m_File;
  DMA_MUTEX m_FileMutex;
  int64_t m_Freq;
  int64_t m_StartCounter;

  void GetBasicParams(CallParams &outParams);

  // T must be a pointer type, e.g. DmaAllocation, DmaPool.
  template <typename T> void PrintPointerList(uint64_t count, const T *pItems) {
    if (count) {
      fprintf(m_File, "%p", pItems[0]);
      for (uint64_t i = 1; i < count; ++i) {
        fprintf(m_File, " %p", pItems[i]);
      }
    }
  }

  void PrintPointerList(uint64_t count, const DmaAllocation *pItems);
  void Flush();
};

#endif // #if DMA_RECORDING_ENABLED

/*
Thread-safe wrapper over DmaPoolAllocator free list, for allocation of
DmaAllocation_T objects.
*/
class DmaAllocationObjectAllocator {
  DMA_CLASS_NO_COPY(DmaAllocationObjectAllocator)
public:
  DmaAllocationObjectAllocator(
      const DmaAllocationCallbacks *pAllocationCallbacks);

  DmaAllocation Allocate();
  void Free(DmaAllocation hAlloc);

private:
  DMA_MUTEX m_Mutex;
  DmaPoolAllocator<DmaAllocation_T> m_Allocator;
};

struct DmaCurrentBudgetData {
  DMA_ATOMIC_UINT64 m_BlockBytes[DMA_NUM_MEMORY_HEAPS];
  DMA_ATOMIC_UINT64 m_AllocationBytes[DMA_NUM_MEMORY_HEAPS];

  DmaCurrentBudgetData() {
    for (uint32_t heapIndex = 0; heapIndex < DMA_NUM_MEMORY_HEAPS;
         ++heapIndex) {
      m_BlockBytes[heapIndex] = 0;
      m_AllocationBytes[heapIndex] = 0;
    }
  }

  void AddAllocation(uint32_t heapIndex, uint32_t allocationSize) {
    m_AllocationBytes[heapIndex] += allocationSize;
  }

  void RemoveAllocation(uint32_t heapIndex, uint32_t allocationSize) {
    DMA_ASSERT(m_AllocationBytes[heapIndex] >= allocationSize); // DELME
    m_AllocationBytes[heapIndex] -= allocationSize;
  }
};

// Main allocator object.
struct DmaAllocator_T {
  DMA_CLASS_NO_COPY(DmaAllocator_T)
public:
  bool m_UseMutex;
  DkDevice m_hDevice;
  bool m_AllocationCallbacksSpecified;
  DmaAllocationCallbacks m_AllocationCallbacks;
  DmaDeviceMemoryCallbacks m_DeviceMemoryCallbacks;
  DmaAllocationObjectAllocator m_AllocationObjectAllocator;

  // Default pools.
  DmaBlockVector *m_pBlockVectors[DMA_NUM_MEMORY_TYPES];

  // Each vector is sorted by memory (handle value).
  typedef DmaVector<DmaAllocation, DmaStlAllocator<DmaAllocation>>
      AllocationVectorType;
  AllocationVectorType *m_pDedicatedAllocations[DMA_NUM_MEMORY_TYPES];
  DMA_RW_MUTEX m_DedicatedAllocationsMutex[DMA_NUM_MEMORY_TYPES];

  DmaCurrentBudgetData m_Budget;

  DmaAllocator_T(const DmaAllocatorCreateInfo *pCreateInfo);
  DkResult Init(const DmaAllocatorCreateInfo *pCreateInfo);
  ~DmaAllocator_T();

  const DmaAllocationCallbacks *GetAllocationCallbacks() const {
    return m_AllocationCallbacksSpecified ? &m_AllocationCallbacks : 0;
  }

  // Minimum alignment for all allocations in specific memory type.
  static uint32_t GetMemoryTypeMinAlignment(uint32_t memTypeIndex) {
    return DmaMemTypeMinAlignment[memTypeIndex];
  }

  static uint32_t GetMemoryTypeFlags(uint32_t memTypeIndex) {
    return DmaMemTypeFlags[memTypeIndex];
  }

#if DMA_RECORDING_ENABLED
  DmaRecorder *GetRecorder() const { return m_pRecorder; }
#endif

  // Main allocation function.
  DkResult AllocateMemory(const DmaMemoryRequirements &vkMemReq,
                          bool requiresDedicatedAllocation,
                          bool prefersDedicatedAllocation,
                          const DmaAllocationCreateInfo &createInfo,
                          DmaSuballocationType suballocType,
                          size_t allocationCount, DmaAllocation *pAllocations);

  // Main deallocation function.
  void FreeMemory(size_t allocationCount, DmaAllocation *pAllocations);

  DkResult ResizeAllocation(const DmaAllocation alloc, uint32_t newSize);

  void CalculateStats(DmaStats *pStats);

  void GetBudget(DmaBudget *outBudget, uint32_t firstHeap, uint32_t heapCount);

#if DMA_STATS_STRING_ENABLED
  void PrintDetailedMap(class DmaJsonWriter &json);
#endif

  DkResult DefragmentationBegin(const DmaDefragmentationInfo2 &info,
                                DmaDefragmentationStats *pStats,
                                DmaDefragmentationContext *pContext);
  DkResult DefragmentationEnd(DmaDefragmentationContext context);

  void GetAllocationInfo(DmaAllocation hAllocation,
                         DmaAllocationInfo *pAllocationInfo);
  bool TouchAllocation(DmaAllocation hAllocation);

  DkResult CreatePool(const DmaPoolCreateInfo *pCreateInfo, DmaPool *pPool);
  void DestroyPool(DmaPool pool);
  void GetPoolStats(DmaPool pool, DmaPoolStats *pPoolStats);

  void SetCurrentFrameIndex(uint32_t frameIndex);
  uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex.load(); }

  void MakePoolAllocationsLost(DmaPool hPool, size_t *pLostAllocationCount);
  DkResult CheckPoolCorruption(DmaPool hPool);
  DkResult CheckCorruption(uint32_t memoryTypeBits);

  void CreateLostAllocation(DmaAllocation *pAllocation);

  // Call to Vulkan function vkAllocateMemory with accompanying bookkeeping.
  DkResult AllocateVulkanMemory(const DmaMemoryAllocateInfo *pAllocateInfo,
                                DkMemBlock *pMemory);
  // Call to Vulkan function vkFreeMemory with accompanying bookkeeping.
  void FreeVulkanMemory(uint32_t memoryType, uint32_t size, DkMemBlock hMemory);

  DkResult Map(DmaAllocation hAllocation, void **ppData);
  void Unmap(DmaAllocation hAllocation);

  void FlushOrInvalidateAllocation(DmaAllocation hAllocation, uint32_t offset,
                                   uint32_t size, DMA_CACHE_OPERATION op);

  void FillAllocation(DmaAllocation hAllocation, uint8_t pattern);

  /*
  Returns bit mask of memory types that can support defragmentation on GPU as
  they support creation of required buffer for copy operations.
  */
  static uint32_t GetGpuDefragmentationMemoryTypeBits() {
    return 1 << DmaMT_Generic;
  }

private:
  uint32_t m_PreferredLargeHeapBlockSize;

  DMA_ATOMIC_UINT32 m_CurrentFrameIndex;

  DMA_RW_MUTEX m_PoolsMutex;
  // Protected by m_PoolsMutex. Sorted by pointer value.
  DmaVector<DmaPool, DmaStlAllocator<DmaPool>> m_Pools;
  uint32_t m_NextPoolId;

#if DMA_RECORDING_ENABLED
  DmaRecorder *m_pRecorder;
#endif

  DkResult AllocateMemoryOfType(uint32_t size, uint32_t alignment,
                                bool dedicatedAllocation,
                                const DmaAllocationCreateInfo &createInfo,
                                uint32_t memTypeIndex,
                                DmaSuballocationType suballocType,
                                size_t allocationCount,
                                DmaAllocation *pAllocations);

  // Helper function only to be used inside AllocateDedicatedMemory.
  DkResult AllocateDedicatedMemoryPage(
      uint32_t size, DmaSuballocationType suballocType, uint32_t memTypeIndex,
      const DmaMemoryAllocateInfo &allocInfo, bool map, bool isUserDataString,
      void *pUserData, DmaAllocation *pAllocation);

  // Allocates and registers new DkMemBlock specifically for dedicated
  // allocations.
  DkResult AllocateDedicatedMemory(uint32_t size,
                                   DmaSuballocationType suballocType,
                                   uint32_t memTypeIndex, bool withinBudget,
                                   bool map, bool isUserDataString,
                                   void *pUserData, size_t allocationCount,
                                   DmaAllocation *pAllocations);

  void FreeDedicatedMemory(DmaAllocation allocation);
};

////////////////////////////////////////////////////////////////////////////////
// Memory allocation #2 after DmaAllocator_T definition

static void *DmaMalloc(DmaAllocator hAllocator, size_t size, size_t alignment) {
  return DmaMalloc(&hAllocator->m_AllocationCallbacks, size, alignment);
}

static void DmaFree(DmaAllocator hAllocator, void *ptr) {
  DmaFree(&hAllocator->m_AllocationCallbacks, ptr);
}

template <typename T> static T *DmaAllocate(DmaAllocator hAllocator) {
  return (T *)DmaMalloc(hAllocator, sizeof(T), DMA_ALIGN_OF(T));
}

template <typename T>
static T *DmaAllocateArray(DmaAllocator hAllocator, size_t count) {
  return (T *)DmaMalloc(hAllocator, sizeof(T) * count, DMA_ALIGN_OF(T));
}

template <typename T> static void dma_delete(DmaAllocator hAllocator, T *ptr) {
  if (ptr != DMA_NULL) {
    ptr->~T();
    DmaFree(hAllocator, ptr);
  }
}

template <typename T>
static void dma_delete_array(DmaAllocator hAllocator, T *ptr, size_t count) {
  if (ptr != DMA_NULL) {
    for (size_t i = count; i--;)
      ptr[i].~T();
    DmaFree(hAllocator, ptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
// DmaStringBuilder

#if DMA_STATS_STRING_ENABLED

class DmaStringBuilder {
public:
  DmaStringBuilder(DmaAllocator alloc)
      : m_Data(DmaStlAllocator<char>(alloc->GetAllocationCallbacks())) {}
  size_t GetLength() const { return m_Data.size(); }
  const char *GetData() const { return m_Data.data(); }

  void Add(char ch) { m_Data.push_back(ch); }
  void Add(const char *pStr);
  void AddNewLine() { Add('\n'); }
  void AddNumber(uint32_t num);
  void AddNumber(uint64_t num);
  void AddPointer(const void *ptr);

private:
  DmaVector<char, DmaStlAllocator<char>> m_Data;
};

void DmaStringBuilder::Add(const char *pStr) {
  const size_t strLen = strlen(pStr);
  if (strLen > 0) {
    const size_t oldCount = m_Data.size();
    m_Data.resize(oldCount + strLen);
    memcpy(m_Data.data() + oldCount, pStr, strLen);
  }
}

void DmaStringBuilder::AddNumber(uint32_t num) {
  char buf[11];
  buf[10] = '\0';
  char *p = &buf[10];
  do {
    *--p = '0' + (num % 10);
    num /= 10;
  } while (num);
  Add(p);
}

void DmaStringBuilder::AddNumber(uint64_t num) {
  char buf[21];
  buf[20] = '\0';
  char *p = &buf[20];
  do {
    *--p = '0' + (num % 10);
    num /= 10;
  } while (num);
  Add(p);
}

void DmaStringBuilder::AddPointer(const void *ptr) {
  char buf[21];
  DmaPtrToStr(buf, sizeof(buf), ptr);
  Add(buf);
}

#endif // #if DMA_STATS_STRING_ENABLED

////////////////////////////////////////////////////////////////////////////////
// DmaJsonWriter

#if DMA_STATS_STRING_ENABLED

class DmaJsonWriter {
  DMA_CLASS_NO_COPY(DmaJsonWriter)
public:
  DmaJsonWriter(const DmaAllocationCallbacks *pAllocationCallbacks,
                DmaStringBuilder &sb);
  ~DmaJsonWriter();

  void BeginObject(bool singleLine = false);
  void EndObject();

  void BeginArray(bool singleLine = false);
  void EndArray();

  void WriteString(const char *pStr);
  void BeginString(const char *pStr = DMA_NULL);
  void ContinueString(const char *pStr);
  void ContinueString(uint32_t n);
  void ContinueString(uint64_t n);
  void ContinueString_Pointer(const void *ptr);
  void EndString(const char *pStr = DMA_NULL);

  void WriteNumber(uint32_t n);
  void WriteNumber(uint64_t n);
  void WriteBool(bool b);
  void WriteNull();

private:
  static const char *const INDENT;

  enum COLLECTION_TYPE {
    COLLECTION_TYPE_OBJECT,
    COLLECTION_TYPE_ARRAY,
  };
  struct StackItem {
    COLLECTION_TYPE type;
    uint32_t valueCount;
    bool singleLineMode;
  };

  DmaStringBuilder &m_SB;
  DmaVector<StackItem, DmaStlAllocator<StackItem>> m_Stack;
  bool m_InsideString;

  void BeginValue(bool isString);
  void WriteIndent(bool oneLess = false);
};

const char *const DmaJsonWriter::INDENT = "  ";

DmaJsonWriter::DmaJsonWriter(const DmaAllocationCallbacks *pAllocationCallbacks,
                             DmaStringBuilder &sb)
    : m_SB(sb), m_Stack(DmaStlAllocator<StackItem>(pAllocationCallbacks)),
      m_InsideString(false) {}

DmaJsonWriter::~DmaJsonWriter() {
  DMA_ASSERT(!m_InsideString);
  DMA_ASSERT(m_Stack.empty());
}

void DmaJsonWriter::BeginObject(bool singleLine) {
  DMA_ASSERT(!m_InsideString);

  BeginValue(false);
  m_SB.Add('{');

  StackItem item;
  item.type = COLLECTION_TYPE_OBJECT;
  item.valueCount = 0;
  item.singleLineMode = singleLine;
  m_Stack.push_back(item);
}

void DmaJsonWriter::EndObject() {
  DMA_ASSERT(!m_InsideString);

  WriteIndent(true);
  m_SB.Add('}');

  DMA_ASSERT(!m_Stack.empty() && m_Stack.back().type == COLLECTION_TYPE_OBJECT);
  m_Stack.pop_back();
}

void DmaJsonWriter::BeginArray(bool singleLine) {
  DMA_ASSERT(!m_InsideString);

  BeginValue(false);
  m_SB.Add('[');

  StackItem item;
  item.type = COLLECTION_TYPE_ARRAY;
  item.valueCount = 0;
  item.singleLineMode = singleLine;
  m_Stack.push_back(item);
}

void DmaJsonWriter::EndArray() {
  DMA_ASSERT(!m_InsideString);

  WriteIndent(true);
  m_SB.Add(']');

  DMA_ASSERT(!m_Stack.empty() && m_Stack.back().type == COLLECTION_TYPE_ARRAY);
  m_Stack.pop_back();
}

void DmaJsonWriter::WriteString(const char *pStr) {
  BeginString(pStr);
  EndString();
}

void DmaJsonWriter::BeginString(const char *pStr) {
  DMA_ASSERT(!m_InsideString);

  BeginValue(true);
  m_SB.Add('"');
  m_InsideString = true;
  if (pStr != DMA_NULL && pStr[0] != '\0') {
    ContinueString(pStr);
  }
}

void DmaJsonWriter::ContinueString(const char *pStr) {
  DMA_ASSERT(m_InsideString);

  const size_t strLen = strlen(pStr);
  for (size_t i = 0; i < strLen; ++i) {
    char ch = pStr[i];
    if (ch == '\\') {
      m_SB.Add("\\\\");
    } else if (ch == '"') {
      m_SB.Add("\\\"");
    } else if (ch >= 32) {
      m_SB.Add(ch);
    } else
      switch (ch) {
      case '\b':
        m_SB.Add("\\b");
        break;
      case '\f':
        m_SB.Add("\\f");
        break;
      case '\n':
        m_SB.Add("\\n");
        break;
      case '\r':
        m_SB.Add("\\r");
        break;
      case '\t':
        m_SB.Add("\\t");
        break;
      default:
        DMA_ASSERT(0 && "Character not currently supported.");
        break;
      }
  }
}

void DmaJsonWriter::ContinueString(uint32_t n) {
  DMA_ASSERT(m_InsideString);
  m_SB.AddNumber(n);
}

void DmaJsonWriter::ContinueString(uint64_t n) {
  DMA_ASSERT(m_InsideString);
  m_SB.AddNumber(n);
}

void DmaJsonWriter::ContinueString_Pointer(const void *ptr) {
  DMA_ASSERT(m_InsideString);
  m_SB.AddPointer(ptr);
}

void DmaJsonWriter::EndString(const char *pStr) {
  DMA_ASSERT(m_InsideString);
  if (pStr != DMA_NULL && pStr[0] != '\0') {
    ContinueString(pStr);
  }
  m_SB.Add('"');
  m_InsideString = false;
}

void DmaJsonWriter::WriteNumber(uint32_t n) {
  DMA_ASSERT(!m_InsideString);
  BeginValue(false);
  m_SB.AddNumber(n);
}

void DmaJsonWriter::WriteNumber(uint64_t n) {
  DMA_ASSERT(!m_InsideString);
  BeginValue(false);
  m_SB.AddNumber(n);
}

void DmaJsonWriter::WriteBool(bool b) {
  DMA_ASSERT(!m_InsideString);
  BeginValue(false);
  m_SB.Add(b ? "true" : "false");
}

void DmaJsonWriter::WriteNull() {
  DMA_ASSERT(!m_InsideString);
  BeginValue(false);
  m_SB.Add("null");
}

void DmaJsonWriter::BeginValue(bool isString) {
  if (!m_Stack.empty()) {
    StackItem &currItem = m_Stack.back();
    if (currItem.type == COLLECTION_TYPE_OBJECT &&
        currItem.valueCount % 2 == 0) {
      DMA_ASSERT(isString);
    }

    if (currItem.type == COLLECTION_TYPE_OBJECT &&
        currItem.valueCount % 2 != 0) {
      m_SB.Add(": ");
    } else if (currItem.valueCount > 0) {
      m_SB.Add(", ");
      WriteIndent();
    } else {
      WriteIndent();
    }
    ++currItem.valueCount;
  }
}

void DmaJsonWriter::WriteIndent(bool oneLess) {
  if (!m_Stack.empty() && !m_Stack.back().singleLineMode) {
    m_SB.AddNewLine();

    size_t count = m_Stack.size();
    if (count > 0 && oneLess) {
      --count;
    }
    for (size_t i = 0; i < count; ++i) {
      m_SB.Add(INDENT);
    }
  }
}

#endif // #if DMA_STATS_STRING_ENABLED

////////////////////////////////////////////////////////////////////////////////

void DmaAllocation_T::SetUserData(DmaAllocator hAllocator, void *pUserData) {
  if (IsUserDataString()) {
    DMA_ASSERT(pUserData == DMA_NULL || pUserData != m_pUserData);

    FreeUserDataString(hAllocator);

    if (pUserData != DMA_NULL) {
      m_pUserData = DmaCreateStringCopy(hAllocator->GetAllocationCallbacks(),
                                        (const char *)pUserData);
    }
  } else {
    m_pUserData = pUserData;
  }
}

void DmaAllocation_T::ChangeBlockAllocation(DmaAllocator hAllocator,
                                            DmaDeviceMemoryBlock *block,
                                            uint32_t offset) {
  DMA_ASSERT(block != DMA_NULL);
  DMA_ASSERT(m_Type == ALLOCATION_TYPE_BLOCK);

  // Move mapping reference counter from old block to new block.
  if (block != m_BlockAllocation.m_Block) {
    uint32_t mapRefCount = m_MapCount & ~MAP_COUNT_FLAG_PERSISTENT_MAP;
    if (IsPersistentMap())
      ++mapRefCount;
    m_BlockAllocation.m_Block->Unmap(hAllocator, mapRefCount);
    block->Map(hAllocator, mapRefCount, DMA_NULL);
  }

  m_BlockAllocation.m_Block = block;
  m_BlockAllocation.m_Offset = offset;
}

void DmaAllocation_T::ChangeOffset(uint32_t newOffset) {
  DMA_ASSERT(m_Type == ALLOCATION_TYPE_BLOCK);
  m_BlockAllocation.m_Offset = newOffset;
}

uint32_t DmaAllocation_T::GetOffset() const {
  switch (m_Type) {
  case ALLOCATION_TYPE_BLOCK:
    return m_BlockAllocation.m_Offset;
  case ALLOCATION_TYPE_DEDICATED:
    return 0;
  default:
    DMA_ASSERT(0);
    return 0;
  }
}

DkMemBlock DmaAllocation_T::GetMemory() const {
  switch (m_Type) {
  case ALLOCATION_TYPE_BLOCK:
    return m_BlockAllocation.m_Block->GetDeviceMemory();
  case ALLOCATION_TYPE_DEDICATED:
    return m_DedicatedAllocation.m_hMemory;
  default:
    DMA_ASSERT(0);
    return DMA_NULL_HANDLE;
  }
}

void *DmaAllocation_T::GetMappedData() const {
  switch (m_Type) {
  case ALLOCATION_TYPE_BLOCK:
    if (m_MapCount != 0) {
      void *pBlockData = m_BlockAllocation.m_Block->GetMappedData();
      DMA_ASSERT(pBlockData != DMA_NULL);
      return (char *)pBlockData + m_BlockAllocation.m_Offset;
    } else {
      return DMA_NULL;
    }
    break;
  case ALLOCATION_TYPE_DEDICATED:
    DMA_ASSERT((m_DedicatedAllocation.m_pMappedData != DMA_NULL) ==
               (m_MapCount != 0));
    return m_DedicatedAllocation.m_pMappedData;
  default:
    DMA_ASSERT(0);
    return DMA_NULL;
  }
}

bool DmaAllocation_T::CanBecomeLost() const {
  switch (m_Type) {
  case ALLOCATION_TYPE_BLOCK:
    return m_BlockAllocation.m_CanBecomeLost;
  case ALLOCATION_TYPE_DEDICATED:
    return false;
  default:
    DMA_ASSERT(0);
    return false;
  }
}

bool DmaAllocation_T::MakeLost(uint32_t currentFrameIndex,
                               uint32_t frameInUseCount) {
  DMA_ASSERT(CanBecomeLost());

  /*
  Warning: This is a carefully designed algorithm.
  Do not modify unless you really know what you're doing :)
  */
  uint32_t localLastUseFrameIndex = GetLastUseFrameIndex();
  for (;;) {
    if (localLastUseFrameIndex == DMA_FRAME_INDEX_LOST) {
      DMA_ASSERT(0);
      return false;
    } else if (localLastUseFrameIndex + frameInUseCount >= currentFrameIndex) {
      return false;
    } else // Last use time earlier than current time.
    {
      if (CompareExchangeLastUseFrameIndex(localLastUseFrameIndex,
                                           DMA_FRAME_INDEX_LOST)) {
        // Setting hAllocation.LastUseFrameIndex atomic to DMA_FRAME_INDEX_LOST
        // is enough to mark it as LOST. Calling code just needs to unregister
        // this allocation in owning DmaDeviceMemoryBlock.
        return true;
      }
    }
  }
}

#if DMA_STATS_STRING_ENABLED

// Correspond to values of enum DmaSuballocationType.
static const char *DMA_SUBALLOCATION_TYPE_NAMES[] = {
    "FREE",          "UNKNOWN",      "BUFFER",
    "IMAGE_UNKNOWN", "IMAGE_LINEAR", "IMAGE_OPTIMAL",
};

void DmaAllocation_T::PrintParameters(class DmaJsonWriter &json) const {
  json.WriteString("Type");
  json.WriteString(DMA_SUBALLOCATION_TYPE_NAMES[m_SuballocationType]);

  json.WriteString("Size");
  json.WriteNumber(m_Size);

  if (m_pUserData != DMA_NULL) {
    json.WriteString("UserData");
    if (IsUserDataString()) {
      json.WriteString((const char *)m_pUserData);
    } else {
      json.BeginString();
      json.ContinueString_Pointer(m_pUserData);
      json.EndString();
    }
  }

  json.WriteString("CreationFrameIndex");
  json.WriteNumber(m_CreationFrameIndex);

  json.WriteString("LastUseFrameIndex");
  json.WriteNumber(GetLastUseFrameIndex());

  if (m_BufferImageUsage != 0) {
    json.WriteString("Usage");
    json.WriteNumber(m_BufferImageUsage);
  }
}

#endif

void DmaAllocation_T::FreeUserDataString(DmaAllocator hAllocator) {
  DMA_ASSERT(IsUserDataString());
  DmaFreeString(hAllocator->GetAllocationCallbacks(), (char *)m_pUserData);
  m_pUserData = DMA_NULL;
}

void DmaAllocation_T::BlockAllocMap() {
  DMA_ASSERT(GetType() == ALLOCATION_TYPE_BLOCK);

  if ((m_MapCount & ~MAP_COUNT_FLAG_PERSISTENT_MAP) < 0x7F) {
    ++m_MapCount;
  } else {
    DMA_ASSERT(0 && "Allocation mapped too many times simultaneously.");
  }
}

void DmaAllocation_T::BlockAllocUnmap() {
  DMA_ASSERT(GetType() == ALLOCATION_TYPE_BLOCK);

  if ((m_MapCount & ~MAP_COUNT_FLAG_PERSISTENT_MAP) != 0) {
    --m_MapCount;
  } else {
    DMA_ASSERT(0 && "Unmapping allocation not previously mapped.");
  }
}

DkResult DmaAllocation_T::DedicatedAllocMap(DmaAllocator hAllocator,
                                            void **ppData) {
  DMA_ASSERT(GetType() == ALLOCATION_TYPE_DEDICATED);

  if (m_MapCount != 0) {
    if ((m_MapCount & ~MAP_COUNT_FLAG_PERSISTENT_MAP) < 0x7F) {
      DMA_ASSERT(m_DedicatedAllocation.m_pMappedData != DMA_NULL);
      *ppData = m_DedicatedAllocation.m_pMappedData;
      ++m_MapCount;
      return DkResult_Success;
    } else {
      DMA_ASSERT(0 &&
                 "Dedicated allocation mapped too many times simultaneously.");
      return DkResult_Fail;
    }
  } else {
    *ppData = dkMemBlockGetCpuAddr(m_DedicatedAllocation.m_hMemory);
    m_DedicatedAllocation.m_pMappedData = *ppData;
    m_MapCount = 1;
    return DkResult_Success;
  }
}

void DmaAllocation_T::DedicatedAllocUnmap(DmaAllocator hAllocator) {
  DMA_ASSERT(GetType() == ALLOCATION_TYPE_DEDICATED);

  if ((m_MapCount & ~MAP_COUNT_FLAG_PERSISTENT_MAP) != 0) {
    --m_MapCount;
    if (m_MapCount == 0) {
      m_DedicatedAllocation.m_pMappedData = DMA_NULL;
    }
  } else {
    DMA_ASSERT(0 && "Unmapping dedicated allocation not previously mapped.");
  }
}

#if DMA_STATS_STRING_ENABLED

static void DmaPrintStatInfo(DmaJsonWriter &json, const DmaStatInfo &stat) {
  json.BeginObject();

  json.WriteString("Blocks");
  json.WriteNumber(stat.blockCount);

  json.WriteString("Allocations");
  json.WriteNumber(stat.allocationCount);

  json.WriteString("UnusedRanges");
  json.WriteNumber(stat.unusedRangeCount);

  json.WriteString("UsedBytes");
  json.WriteNumber(stat.usedBytes);

  json.WriteString("UnusedBytes");
  json.WriteNumber(stat.unusedBytes);

  if (stat.allocationCount > 1) {
    json.WriteString("AllocationSize");
    json.BeginObject(true);
    json.WriteString("Min");
    json.WriteNumber(stat.allocationSizeMin);
    json.WriteString("Avg");
    json.WriteNumber(stat.allocationSizeAvg);
    json.WriteString("Max");
    json.WriteNumber(stat.allocationSizeMax);
    json.EndObject();
  }

  if (stat.unusedRangeCount > 1) {
    json.WriteString("UnusedRangeSize");
    json.BeginObject(true);
    json.WriteString("Min");
    json.WriteNumber(stat.unusedRangeSizeMin);
    json.WriteString("Avg");
    json.WriteNumber(stat.unusedRangeSizeAvg);
    json.WriteString("Max");
    json.WriteNumber(stat.unusedRangeSizeMax);
    json.EndObject();
  }

  json.EndObject();
}

#endif // #if DMA_STATS_STRING_ENABLED

struct DmaSuballocationItemSizeLess {
  bool operator()(const DmaSuballocationList::iterator lhs,
                  const DmaSuballocationList::iterator rhs) const {
    return lhs->size < rhs->size;
  }
  bool operator()(const DmaSuballocationList::iterator lhs,
                  uint32_t rhsSize) const {
    return lhs->size < rhsSize;
  }
};

////////////////////////////////////////////////////////////////////////////////
// class DmaBlockMetadata

DmaBlockMetadata::DmaBlockMetadata(DmaAllocator hAllocator)
    : m_Size(0), m_pAllocationCallbacks(hAllocator->GetAllocationCallbacks()) {}

#if DMA_STATS_STRING_ENABLED

void DmaBlockMetadata::PrintDetailedMap_Begin(class DmaJsonWriter &json,
                                              uint32_t unusedBytes,
                                              size_t allocationCount,
                                              size_t unusedRangeCount) const {
  json.BeginObject();

  json.WriteString("TotalBytes");
  json.WriteNumber(GetSize());

  json.WriteString("UnusedBytes");
  json.WriteNumber(unusedBytes);

  json.WriteString("Allocations");
  json.WriteNumber((uint64_t)allocationCount);

  json.WriteString("UnusedRanges");
  json.WriteNumber((uint64_t)unusedRangeCount);

  json.WriteString("Suballocations");
  json.BeginArray();
}

void DmaBlockMetadata::PrintDetailedMap_Allocation(
    class DmaJsonWriter &json, uint32_t offset,
    DmaAllocation hAllocation) const {
  json.BeginObject(true);

  json.WriteString("Offset");
  json.WriteNumber(offset);

  hAllocation->PrintParameters(json);

  json.EndObject();
}

void DmaBlockMetadata::PrintDetailedMap_UnusedRange(class DmaJsonWriter &json,
                                                    uint32_t offset,
                                                    uint32_t size) const {
  json.BeginObject(true);

  json.WriteString("Offset");
  json.WriteNumber(offset);

  json.WriteString("Type");
  json.WriteString(DMA_SUBALLOCATION_TYPE_NAMES[DMA_SUBALLOCATION_TYPE_FREE]);

  json.WriteString("Size");
  json.WriteNumber(size);

  json.EndObject();
}

void DmaBlockMetadata::PrintDetailedMap_End(class DmaJsonWriter &json) const {
  json.EndArray();
  json.EndObject();
}

#endif // #if DMA_STATS_STRING_ENABLED

////////////////////////////////////////////////////////////////////////////////
// class DmaBlockMetadata_Generic

DmaBlockMetadata_Generic::DmaBlockMetadata_Generic(DmaAllocator hAllocator)
    : DmaBlockMetadata(hAllocator), m_FreeCount(0), m_SumFreeSize(0),
      m_Suballocations(DmaStlAllocator<DmaSuballocation>(
          hAllocator->GetAllocationCallbacks())),
      m_FreeSuballocationsBySize(
          DmaStlAllocator<DmaSuballocationList::iterator>(
              hAllocator->GetAllocationCallbacks())) {}

DmaBlockMetadata_Generic::~DmaBlockMetadata_Generic() {}

void DmaBlockMetadata_Generic::Init(uint32_t size) {
  DmaBlockMetadata::Init(size);

  m_FreeCount = 1;
  m_SumFreeSize = size;

  DmaSuballocation suballoc = {};
  suballoc.offset = 0;
  suballoc.size = size;
  suballoc.type = DMA_SUBALLOCATION_TYPE_FREE;
  suballoc.hAllocation = DMA_NULL_HANDLE;

  DMA_ASSERT(size > DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER);
  m_Suballocations.push_back(suballoc);
  DmaSuballocationList::iterator suballocItem = m_Suballocations.end();
  --suballocItem;
  m_FreeSuballocationsBySize.push_back(suballocItem);
}

bool DmaBlockMetadata_Generic::Validate() const {
  DMA_VALIDATE(!m_Suballocations.empty());

  // Expected offset of new suballocation as calculated from previous ones.
  uint32_t calculatedOffset = 0;
  // Expected number of free suballocations as calculated from traversing their
  // list.
  uint32_t calculatedFreeCount = 0;
  // Expected sum size of free suballocations as calculated from traversing
  // their list.
  uint32_t calculatedSumFreeSize = 0;
  // Expected number of free suballocations that should be registered in
  // m_FreeSuballocationsBySize calculated from traversing their list.
  size_t freeSuballocationsToRegister = 0;
  // True if previous visited suballocation was free.
  bool prevFree = false;

  for (DmaSuballocationList::const_iterator suballocItem =
           m_Suballocations.cbegin();
       suballocItem != m_Suballocations.cend(); ++suballocItem) {
    const DmaSuballocation &subAlloc = *suballocItem;

    // Actual offset of this suballocation doesn't match expected one.
    DMA_VALIDATE(subAlloc.offset == calculatedOffset);

    const bool currFree = (subAlloc.type == DMA_SUBALLOCATION_TYPE_FREE);
    // Two adjacent free suballocations are invalid. They should be merged.
    DMA_VALIDATE(!prevFree || !currFree);

    DMA_VALIDATE(currFree == (subAlloc.hAllocation == DMA_NULL_HANDLE));

    if (currFree) {
      calculatedSumFreeSize += subAlloc.size;
      ++calculatedFreeCount;
      if (subAlloc.size >= DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER) {
        ++freeSuballocationsToRegister;
      }

      // Margin required between allocations - every free space must be at least
      // that large.
      DMA_VALIDATE(subAlloc.size >= DMA_DEBUG_MARGIN);
    } else {
      DMA_VALIDATE(subAlloc.hAllocation->GetOffset() == subAlloc.offset);
      DMA_VALIDATE(subAlloc.hAllocation->GetSize() == subAlloc.size);

      // Margin required between allocations - previous allocation must be free.
      DMA_VALIDATE(DMA_DEBUG_MARGIN == 0 || prevFree);
    }

    calculatedOffset += subAlloc.size;
    prevFree = currFree;
  }

  // Number of free suballocations registered in m_FreeSuballocationsBySize
  // doesn't match expected one.
  DMA_VALIDATE(m_FreeSuballocationsBySize.size() ==
               freeSuballocationsToRegister);

  uint32_t lastSize = 0;
  for (size_t i = 0; i < m_FreeSuballocationsBySize.size(); ++i) {
    DmaSuballocationList::iterator suballocItem = m_FreeSuballocationsBySize[i];

    // Only free suballocations can be registered in m_FreeSuballocationsBySize.
    DMA_VALIDATE(suballocItem->type == DMA_SUBALLOCATION_TYPE_FREE);
    // They must be sorted by size ascending.
    DMA_VALIDATE(suballocItem->size >= lastSize);

    lastSize = suballocItem->size;
  }

  // Check if totals match calculacted values.
  DMA_VALIDATE(ValidateFreeSuballocationList());
  DMA_VALIDATE(calculatedOffset == GetSize());
  DMA_VALIDATE(calculatedSumFreeSize == m_SumFreeSize);
  DMA_VALIDATE(calculatedFreeCount == m_FreeCount);

  return true;
}

uint32_t DmaBlockMetadata_Generic::GetUnusedRangeSizeMax() const {
  if (!m_FreeSuballocationsBySize.empty()) {
    return m_FreeSuballocationsBySize.back()->size;
  } else {
    return 0;
  }
}

bool DmaBlockMetadata_Generic::IsEmpty() const {
  return (m_Suballocations.size() == 1) && (m_FreeCount == 1);
}

void DmaBlockMetadata_Generic::CalcAllocationStatInfo(
    DmaStatInfo &outInfo) const {
  outInfo.blockCount = 1;

  const uint32_t rangeCount = (uint32_t)m_Suballocations.size();
  outInfo.allocationCount = rangeCount - m_FreeCount;
  outInfo.unusedRangeCount = m_FreeCount;

  outInfo.unusedBytes = m_SumFreeSize;
  outInfo.usedBytes = GetSize() - outInfo.unusedBytes;

  outInfo.allocationSizeMin = UINT32_MAX;
  outInfo.allocationSizeMax = 0;
  outInfo.unusedRangeSizeMin = UINT32_MAX;
  outInfo.unusedRangeSizeMax = 0;

  for (DmaSuballocationList::const_iterator suballocItem =
           m_Suballocations.cbegin();
       suballocItem != m_Suballocations.cend(); ++suballocItem) {
    const DmaSuballocation &suballoc = *suballocItem;
    if (suballoc.type != DMA_SUBALLOCATION_TYPE_FREE) {
      outInfo.allocationSizeMin =
          DMA_MIN(outInfo.allocationSizeMin, suballoc.size);
      outInfo.allocationSizeMax =
          DMA_MAX(outInfo.allocationSizeMax, suballoc.size);
    } else {
      outInfo.unusedRangeSizeMin =
          DMA_MIN(outInfo.unusedRangeSizeMin, suballoc.size);
      outInfo.unusedRangeSizeMax =
          DMA_MAX(outInfo.unusedRangeSizeMax, suballoc.size);
    }
  }
}

void DmaBlockMetadata_Generic::AddPoolStats(DmaPoolStats &inoutStats) const {
  const uint32_t rangeCount = (uint32_t)m_Suballocations.size();

  inoutStats.size += GetSize();
  inoutStats.unusedSize += m_SumFreeSize;
  inoutStats.allocationCount += rangeCount - m_FreeCount;
  inoutStats.unusedRangeCount += m_FreeCount;
  inoutStats.unusedRangeSizeMax =
      DMA_MAX(inoutStats.unusedRangeSizeMax, GetUnusedRangeSizeMax());
}

#if DMA_STATS_STRING_ENABLED

void DmaBlockMetadata_Generic::PrintDetailedMap(
    class DmaJsonWriter &json) const {
  PrintDetailedMap_Begin(json,
                         m_SumFreeSize, // unusedBytes
                         m_Suballocations.size() -
                             (size_t)m_FreeCount, // allocationCount
                         m_FreeCount);            // unusedRangeCount

  size_t i = 0;
  for (DmaSuballocationList::const_iterator suballocItem =
           m_Suballocations.cbegin();
       suballocItem != m_Suballocations.cend(); ++suballocItem, ++i) {
    if (suballocItem->type == DMA_SUBALLOCATION_TYPE_FREE) {
      PrintDetailedMap_UnusedRange(json, suballocItem->offset,
                                   suballocItem->size);
    } else {
      PrintDetailedMap_Allocation(json, suballocItem->offset,
                                  suballocItem->hAllocation);
    }
  }

  PrintDetailedMap_End(json);
}

#endif // #if DMA_STATS_STRING_ENABLED

bool DmaBlockMetadata_Generic::CreateAllocationRequest(
    uint32_t currentFrameIndex, uint32_t frameInUseCount, uint32_t allocSize,
    uint32_t allocAlignment, bool upperAddress, DmaSuballocationType allocType,
    bool canMakeOtherLost, uint32_t strategy,
    DmaAllocationRequest *pAllocationRequest) {
  DMA_ASSERT(allocSize > 0);
  DMA_ASSERT(!upperAddress);
  DMA_ASSERT(allocType != DMA_SUBALLOCATION_TYPE_FREE);
  DMA_ASSERT(pAllocationRequest != DMA_NULL);
  DMA_HEAVY_ASSERT(Validate());

  pAllocationRequest->type = DmaAllocationRequestType::Normal;

  // There is not enough total free space in this block to fullfill the request:
  // Early return.
  if (canMakeOtherLost == false &&
      m_SumFreeSize < allocSize + 2 * DMA_DEBUG_MARGIN) {
    return false;
  }

  // New algorithm, efficiently searching freeSuballocationsBySize.
  const size_t freeSuballocCount = m_FreeSuballocationsBySize.size();
  if (freeSuballocCount > 0) {
    if (strategy == DMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT) {
      // Find first free suballocation with size not less than allocSize + 2 *
      // DMA_DEBUG_MARGIN.
      DmaSuballocationList::iterator *const it = DmaBinaryFindFirstNotLess(
          m_FreeSuballocationsBySize.data(),
          m_FreeSuballocationsBySize.data() + freeSuballocCount,
          allocSize + 2 * DMA_DEBUG_MARGIN, DmaSuballocationItemSizeLess());
      size_t index = it - m_FreeSuballocationsBySize.data();
      for (; index < freeSuballocCount; ++index) {
        if (CheckAllocation(currentFrameIndex, frameInUseCount, allocSize,
                            allocAlignment, allocType,
                            m_FreeSuballocationsBySize[index],
                            false, // canMakeOtherLost
                            &pAllocationRequest->offset,
                            &pAllocationRequest->itemsToMakeLostCount,
                            &pAllocationRequest->sumFreeSize,
                            &pAllocationRequest->sumItemSize)) {
          pAllocationRequest->item = m_FreeSuballocationsBySize[index];
          return true;
        }
      }
    } else if (strategy == DMA_ALLOCATION_INTERNAL_STRATEGY_MIN_OFFSET) {
      for (DmaSuballocationList::iterator it = m_Suballocations.begin();
           it != m_Suballocations.end(); ++it) {
        if (it->type == DMA_SUBALLOCATION_TYPE_FREE &&
            CheckAllocation(currentFrameIndex, frameInUseCount, allocSize,
                            allocAlignment, allocType, it,
                            false, // canMakeOtherLost
                            &pAllocationRequest->offset,
                            &pAllocationRequest->itemsToMakeLostCount,
                            &pAllocationRequest->sumFreeSize,
                            &pAllocationRequest->sumItemSize)) {
          pAllocationRequest->item = it;
          return true;
        }
      }
    } else // WORST_FIT, FIRST_FIT
    {
      // Search staring from biggest suballocations.
      for (size_t index = freeSuballocCount; index--;) {
        if (CheckAllocation(currentFrameIndex, frameInUseCount, allocSize,
                            allocAlignment, allocType,
                            m_FreeSuballocationsBySize[index],
                            false, // canMakeOtherLost
                            &pAllocationRequest->offset,
                            &pAllocationRequest->itemsToMakeLostCount,
                            &pAllocationRequest->sumFreeSize,
                            &pAllocationRequest->sumItemSize)) {
          pAllocationRequest->item = m_FreeSuballocationsBySize[index];
          return true;
        }
      }
    }
  }

  if (canMakeOtherLost) {
    // Brute-force algorithm. TODO: Come up with something better.

    bool found = false;
    DmaAllocationRequest tmpAllocRequest = {};
    tmpAllocRequest.type = DmaAllocationRequestType::Normal;
    for (DmaSuballocationList::iterator suballocIt = m_Suballocations.begin();
         suballocIt != m_Suballocations.end(); ++suballocIt) {
      if (suballocIt->type == DMA_SUBALLOCATION_TYPE_FREE ||
          suballocIt->hAllocation->CanBecomeLost()) {
        if (CheckAllocation(
                currentFrameIndex, frameInUseCount, allocSize, allocAlignment,
                allocType, suballocIt, canMakeOtherLost,
                &tmpAllocRequest.offset, &tmpAllocRequest.itemsToMakeLostCount,
                &tmpAllocRequest.sumFreeSize, &tmpAllocRequest.sumItemSize)) {
          if (strategy == DMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT) {
            *pAllocationRequest = tmpAllocRequest;
            pAllocationRequest->item = suballocIt;
            break;
          }
          if (!found ||
              tmpAllocRequest.CalcCost() < pAllocationRequest->CalcCost()) {
            *pAllocationRequest = tmpAllocRequest;
            pAllocationRequest->item = suballocIt;
            found = true;
          }
        }
      }
    }

    return found;
  }

  return false;
}

bool DmaBlockMetadata_Generic::MakeRequestedAllocationsLost(
    uint32_t currentFrameIndex, uint32_t frameInUseCount,
    DmaAllocationRequest *pAllocationRequest) {
  DMA_ASSERT(pAllocationRequest &&
             pAllocationRequest->type == DmaAllocationRequestType::Normal);

  while (pAllocationRequest->itemsToMakeLostCount > 0) {
    if (pAllocationRequest->item->type == DMA_SUBALLOCATION_TYPE_FREE) {
      ++pAllocationRequest->item;
    }
    DMA_ASSERT(pAllocationRequest->item != m_Suballocations.end());
    DMA_ASSERT(pAllocationRequest->item->hAllocation != DMA_NULL_HANDLE);
    DMA_ASSERT(pAllocationRequest->item->hAllocation->CanBecomeLost());
    if (pAllocationRequest->item->hAllocation->MakeLost(currentFrameIndex,
                                                        frameInUseCount)) {
      pAllocationRequest->item = FreeSuballocation(pAllocationRequest->item);
      --pAllocationRequest->itemsToMakeLostCount;
    } else {
      return false;
    }
  }

  DMA_HEAVY_ASSERT(Validate());
  DMA_ASSERT(pAllocationRequest->item != m_Suballocations.end());
  DMA_ASSERT(pAllocationRequest->item->type == DMA_SUBALLOCATION_TYPE_FREE);

  return true;
}

uint32_t
DmaBlockMetadata_Generic::MakeAllocationsLost(uint32_t currentFrameIndex,
                                              uint32_t frameInUseCount) {
  uint32_t lostAllocationCount = 0;
  for (DmaSuballocationList::iterator it = m_Suballocations.begin();
       it != m_Suballocations.end(); ++it) {
    if (it->type != DMA_SUBALLOCATION_TYPE_FREE &&
        it->hAllocation->CanBecomeLost() &&
        it->hAllocation->MakeLost(currentFrameIndex, frameInUseCount)) {
      it = FreeSuballocation(it);
      ++lostAllocationCount;
    }
  }
  return lostAllocationCount;
}

DkResult DmaBlockMetadata_Generic::CheckCorruption(const void *pBlockData) {
  for (DmaSuballocationList::iterator it = m_Suballocations.begin();
       it != m_Suballocations.end(); ++it) {
    if (it->type != DMA_SUBALLOCATION_TYPE_FREE) {
      if (!DmaValidateMagicValue(pBlockData, it->offset - DMA_DEBUG_MARGIN)) {
        DMA_ASSERT(0 &&
                   "MEMORY CORRUPTION DETECTED BEFORE VALIDATED ALLOCATION!");
        return DkResult_Fail;
      }
      if (!DmaValidateMagicValue(pBlockData, it->offset + it->size)) {
        DMA_ASSERT(0 &&
                   "MEMORY CORRUPTION DETECTED AFTER VALIDATED ALLOCATION!");
        return DkResult_Fail;
      }
    }
  }

  return DkResult_Success;
}

void DmaBlockMetadata_Generic::Alloc(const DmaAllocationRequest &request,
                                     DmaSuballocationType type,
                                     uint32_t allocSize,
                                     DmaAllocation hAllocation) {
  DMA_ASSERT(request.type == DmaAllocationRequestType::Normal);
  DMA_ASSERT(request.item != m_Suballocations.end());
  DmaSuballocation &suballoc = *request.item;
  // Given suballocation is a free block.
  DMA_ASSERT(suballoc.type == DMA_SUBALLOCATION_TYPE_FREE);
  // Given offset is inside this suballocation.
  DMA_ASSERT(request.offset >= suballoc.offset);
  const uint32_t paddingBegin = request.offset - suballoc.offset;
  DMA_ASSERT(suballoc.size >= paddingBegin + allocSize);
  const uint32_t paddingEnd = suballoc.size - paddingBegin - allocSize;

  // Unregister this free suballocation from m_FreeSuballocationsBySize and
  // update it to become used.
  UnregisterFreeSuballocation(request.item);

  suballoc.offset = request.offset;
  suballoc.size = allocSize;
  suballoc.type = type;
  suballoc.hAllocation = hAllocation;

  // If there are any free bytes remaining at the end, insert new free
  // suballocation after current one.
  if (paddingEnd) {
    DmaSuballocation paddingSuballoc = {};
    paddingSuballoc.offset = request.offset + allocSize;
    paddingSuballoc.size = paddingEnd;
    paddingSuballoc.type = DMA_SUBALLOCATION_TYPE_FREE;
    DmaSuballocationList::iterator next = request.item;
    ++next;
    const DmaSuballocationList::iterator paddingEndItem =
        m_Suballocations.insert(next, paddingSuballoc);
    RegisterFreeSuballocation(paddingEndItem);
  }

  // If there are any free bytes remaining at the beginning, insert new free
  // suballocation before current one.
  if (paddingBegin) {
    DmaSuballocation paddingSuballoc = {};
    paddingSuballoc.offset = request.offset - paddingBegin;
    paddingSuballoc.size = paddingBegin;
    paddingSuballoc.type = DMA_SUBALLOCATION_TYPE_FREE;
    const DmaSuballocationList::iterator paddingBeginItem =
        m_Suballocations.insert(request.item, paddingSuballoc);
    RegisterFreeSuballocation(paddingBeginItem);
  }

  // Update totals.
  m_FreeCount = m_FreeCount - 1;
  if (paddingBegin > 0) {
    ++m_FreeCount;
  }
  if (paddingEnd > 0) {
    ++m_FreeCount;
  }
  m_SumFreeSize -= allocSize;
}

void DmaBlockMetadata_Generic::Free(const DmaAllocation allocation) {
  for (DmaSuballocationList::iterator suballocItem = m_Suballocations.begin();
       suballocItem != m_Suballocations.end(); ++suballocItem) {
    DmaSuballocation &suballoc = *suballocItem;
    if (suballoc.hAllocation == allocation) {
      FreeSuballocation(suballocItem);
      DMA_HEAVY_ASSERT(Validate());
      return;
    }
  }
  DMA_ASSERT(0 && "Not found!");
}

void DmaBlockMetadata_Generic::FreeAtOffset(uint32_t offset) {
  for (DmaSuballocationList::iterator suballocItem = m_Suballocations.begin();
       suballocItem != m_Suballocations.end(); ++suballocItem) {
    DmaSuballocation &suballoc = *suballocItem;
    if (suballoc.offset == offset) {
      FreeSuballocation(suballocItem);
      return;
    }
  }
  DMA_ASSERT(0 && "Not found!");
}

bool DmaBlockMetadata_Generic::ValidateFreeSuballocationList() const {
  uint32_t lastSize = 0;
  for (size_t i = 0, count = m_FreeSuballocationsBySize.size(); i < count;
       ++i) {
    const DmaSuballocationList::iterator it = m_FreeSuballocationsBySize[i];

    DMA_VALIDATE(it->type == DMA_SUBALLOCATION_TYPE_FREE);
    DMA_VALIDATE(it->size >= DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER);
    DMA_VALIDATE(it->size >= lastSize);
    lastSize = it->size;
  }
  return true;
}

bool DmaBlockMetadata_Generic::CheckAllocation(
    uint32_t currentFrameIndex, uint32_t frameInUseCount, uint32_t allocSize,
    uint32_t allocAlignment, DmaSuballocationType allocType,
    DmaSuballocationList::const_iterator suballocItem, bool canMakeOtherLost,
    uint32_t *pOffset, size_t *itemsToMakeLostCount, uint32_t *pSumFreeSize,
    uint32_t *pSumItemSize) const {
  DMA_ASSERT(allocSize > 0);
  DMA_ASSERT(allocType != DMA_SUBALLOCATION_TYPE_FREE);
  DMA_ASSERT(suballocItem != m_Suballocations.cend());
  DMA_ASSERT(pOffset != DMA_NULL);

  *itemsToMakeLostCount = 0;
  *pSumFreeSize = 0;
  *pSumItemSize = 0;

  if (canMakeOtherLost) {
    if (suballocItem->type == DMA_SUBALLOCATION_TYPE_FREE) {
      *pSumFreeSize = suballocItem->size;
    } else {
      if (suballocItem->hAllocation->CanBecomeLost() &&
          suballocItem->hAllocation->GetLastUseFrameIndex() + frameInUseCount <
              currentFrameIndex) {
        ++*itemsToMakeLostCount;
        *pSumItemSize = suballocItem->size;
      } else {
        return false;
      }
    }

    // Remaining size is too small for this request: Early return.
    if (GetSize() - suballocItem->offset < allocSize) {
      return false;
    }

    // Start from offset equal to beginning of this suballocation.
    *pOffset = suballocItem->offset;

    // Apply DMA_DEBUG_MARGIN at the beginning.
    if (DMA_DEBUG_MARGIN > 0) {
      *pOffset += DMA_DEBUG_MARGIN;
    }

    // Apply alignment.
    *pOffset = DmaAlignUp(*pOffset, allocAlignment);

    // Now that we have final *pOffset, check if we are past suballocItem.
    // If yes, return false - this function should be called for another
    // suballocItem as starting point.
    if (*pOffset >= suballocItem->offset + suballocItem->size) {
      return false;
    }

    // Calculate padding at the beginning based on current offset.
    const uint32_t paddingBegin = *pOffset - suballocItem->offset;

    // Calculate required margin at the end.
    const uint32_t requiredEndMargin = DMA_DEBUG_MARGIN;

    const uint32_t totalSize = paddingBegin + allocSize + requiredEndMargin;
    // Another early return check.
    if (suballocItem->offset + totalSize > GetSize()) {
      return false;
    }

    // Advance lastSuballocItem until desired size is reached.
    // Update itemsToMakeLostCount.
    DmaSuballocationList::const_iterator lastSuballocItem = suballocItem;
    if (totalSize > suballocItem->size) {
      uint32_t remainingSize = totalSize - suballocItem->size;
      while (remainingSize > 0) {
        ++lastSuballocItem;
        if (lastSuballocItem == m_Suballocations.cend()) {
          return false;
        }
        if (lastSuballocItem->type == DMA_SUBALLOCATION_TYPE_FREE) {
          *pSumFreeSize += lastSuballocItem->size;
        } else {
          DMA_ASSERT(lastSuballocItem->hAllocation != DMA_NULL_HANDLE);
          if (lastSuballocItem->hAllocation->CanBecomeLost() &&
              lastSuballocItem->hAllocation->GetLastUseFrameIndex() +
                      frameInUseCount <
                  currentFrameIndex) {
            ++*itemsToMakeLostCount;
            *pSumItemSize += lastSuballocItem->size;
          } else {
            return false;
          }
        }
        remainingSize = (lastSuballocItem->size < remainingSize)
                            ? remainingSize - lastSuballocItem->size
                            : 0;
      }
    }
  } else {
    const DmaSuballocation &suballoc = *suballocItem;
    DMA_ASSERT(suballoc.type == DMA_SUBALLOCATION_TYPE_FREE);

    *pSumFreeSize = suballoc.size;

    // Size of this suballocation is too small for this request: Early return.
    if (suballoc.size < allocSize) {
      return false;
    }

    // Start from offset equal to beginning of this suballocation.
    *pOffset = suballoc.offset;

    // Apply DMA_DEBUG_MARGIN at the beginning.
    if (DMA_DEBUG_MARGIN > 0) {
      *pOffset += DMA_DEBUG_MARGIN;
    }

    // Apply alignment.
    *pOffset = DmaAlignUp(*pOffset, allocAlignment);

    // Calculate padding at the beginning based on current offset.
    const uint32_t paddingBegin = *pOffset - suballoc.offset;

    // Calculate required margin at the end.
    const uint32_t requiredEndMargin = DMA_DEBUG_MARGIN;

    // Fail if requested size plus margin before and after is bigger than size
    // of this suballocation.
    if (paddingBegin + allocSize + requiredEndMargin > suballoc.size) {
      return false;
    }
  }

  // All tests passed: Success. pOffset is already filled.
  return true;
}

void DmaBlockMetadata_Generic::MergeFreeWithNext(
    DmaSuballocationList::iterator item) {
  DMA_ASSERT(item != m_Suballocations.end());
  DMA_ASSERT(item->type == DMA_SUBALLOCATION_TYPE_FREE);

  DmaSuballocationList::iterator nextItem = item;
  ++nextItem;
  DMA_ASSERT(nextItem != m_Suballocations.end());
  DMA_ASSERT(nextItem->type == DMA_SUBALLOCATION_TYPE_FREE);

  item->size += nextItem->size;
  --m_FreeCount;
  m_Suballocations.erase(nextItem);
}

DmaSuballocationList::iterator DmaBlockMetadata_Generic::FreeSuballocation(
    DmaSuballocationList::iterator suballocItem) {
  // Change this suballocation to be marked as free.
  DmaSuballocation &suballoc = *suballocItem;
  suballoc.type = DMA_SUBALLOCATION_TYPE_FREE;
  suballoc.hAllocation = DMA_NULL_HANDLE;

  // Update totals.
  ++m_FreeCount;
  m_SumFreeSize += suballoc.size;

  // Merge with previous and/or next suballocation if it's also free.
  bool mergeWithNext = false;
  bool mergeWithPrev = false;

  DmaSuballocationList::iterator nextItem = suballocItem;
  ++nextItem;
  if ((nextItem != m_Suballocations.end()) &&
      (nextItem->type == DMA_SUBALLOCATION_TYPE_FREE)) {
    mergeWithNext = true;
  }

  DmaSuballocationList::iterator prevItem = suballocItem;
  if (suballocItem != m_Suballocations.begin()) {
    --prevItem;
    if (prevItem->type == DMA_SUBALLOCATION_TYPE_FREE) {
      mergeWithPrev = true;
    }
  }

  if (mergeWithNext) {
    UnregisterFreeSuballocation(nextItem);
    MergeFreeWithNext(suballocItem);
  }

  if (mergeWithPrev) {
    UnregisterFreeSuballocation(prevItem);
    MergeFreeWithNext(prevItem);
    RegisterFreeSuballocation(prevItem);
    return prevItem;
  } else {
    RegisterFreeSuballocation(suballocItem);
    return suballocItem;
  }
}

void DmaBlockMetadata_Generic::RegisterFreeSuballocation(
    DmaSuballocationList::iterator item) {
  DMA_ASSERT(item->type == DMA_SUBALLOCATION_TYPE_FREE);
  DMA_ASSERT(item->size > 0);

  // You may want to enable this validation at the beginning or at the end of
  // this function, depending on what do you want to check.
  DMA_HEAVY_ASSERT(ValidateFreeSuballocationList());

  if (item->size >= DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER) {
    if (m_FreeSuballocationsBySize.empty()) {
      m_FreeSuballocationsBySize.push_back(item);
    } else {
      DmaVectorInsertSorted<DmaSuballocationItemSizeLess>(
          m_FreeSuballocationsBySize, item);
    }
  }

  // DMA_HEAVY_ASSERT(ValidateFreeSuballocationList());
}

void DmaBlockMetadata_Generic::UnregisterFreeSuballocation(
    DmaSuballocationList::iterator item) {
  DMA_ASSERT(item->type == DMA_SUBALLOCATION_TYPE_FREE);
  DMA_ASSERT(item->size > 0);

  // You may want to enable this validation at the beginning or at the end of
  // this function, depending on what do you want to check.
  DMA_HEAVY_ASSERT(ValidateFreeSuballocationList());

  if (item->size >= DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER) {
    DmaSuballocationList::iterator *const it = DmaBinaryFindFirstNotLess(
        m_FreeSuballocationsBySize.data(),
        m_FreeSuballocationsBySize.data() + m_FreeSuballocationsBySize.size(),
        item, DmaSuballocationItemSizeLess());
    for (size_t index = it - m_FreeSuballocationsBySize.data();
         index < m_FreeSuballocationsBySize.size(); ++index) {
      if (m_FreeSuballocationsBySize[index] == item) {
        DmaVectorRemove(m_FreeSuballocationsBySize, index);
        return;
      }
      DMA_ASSERT((m_FreeSuballocationsBySize[index]->size == item->size) &&
                 "Not found.");
    }
    DMA_ASSERT(0 && "Not found.");
  }

  // DMA_HEAVY_ASSERT(ValidateFreeSuballocationList());
}

////////////////////////////////////////////////////////////////////////////////
// class DmaBlockMetadata_Linear

DmaBlockMetadata_Linear::DmaBlockMetadata_Linear(DmaAllocator hAllocator)
    : DmaBlockMetadata(hAllocator), m_SumFreeSize(0),
      m_Suballocations0(DmaStlAllocator<DmaSuballocation>(
          hAllocator->GetAllocationCallbacks())),
      m_Suballocations1(DmaStlAllocator<DmaSuballocation>(
          hAllocator->GetAllocationCallbacks())),
      m_1stVectorIndex(0), m_2ndVectorMode(SECOND_VECTOR_EMPTY),
      m_1stNullItemsBeginCount(0), m_1stNullItemsMiddleCount(0),
      m_2ndNullItemsCount(0) {}

DmaBlockMetadata_Linear::~DmaBlockMetadata_Linear() {}

void DmaBlockMetadata_Linear::Init(uint32_t size) {
  DmaBlockMetadata::Init(size);
  m_SumFreeSize = size;
}

bool DmaBlockMetadata_Linear::Validate() const {
  const SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  const SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();

  DMA_VALIDATE(suballocations2nd.empty() ==
               (m_2ndVectorMode == SECOND_VECTOR_EMPTY));
  DMA_VALIDATE(!suballocations1st.empty() || suballocations2nd.empty() ||
               m_2ndVectorMode != SECOND_VECTOR_RING_BUFFER);

  if (!suballocations1st.empty()) {
    // Null item at the beginning should be accounted into
    // m_1stNullItemsBeginCount.
    DMA_VALIDATE(suballocations1st[m_1stNullItemsBeginCount].hAllocation !=
                 DMA_NULL_HANDLE);
    // Null item at the end should be just pop_back().
    DMA_VALIDATE(suballocations1st.back().hAllocation != DMA_NULL_HANDLE);
  }
  if (!suballocations2nd.empty()) {
    // Null item at the end should be just pop_back().
    DMA_VALIDATE(suballocations2nd.back().hAllocation != DMA_NULL_HANDLE);
  }

  DMA_VALIDATE(m_1stNullItemsBeginCount + m_1stNullItemsMiddleCount <=
               suballocations1st.size());
  DMA_VALIDATE(m_2ndNullItemsCount <= suballocations2nd.size());

  uint32_t sumUsedSize = 0;
  const size_t suballoc1stCount = suballocations1st.size();
  uint32_t offset = DMA_DEBUG_MARGIN;

  if (m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER) {
    const size_t suballoc2ndCount = suballocations2nd.size();
    size_t nullItem2ndCount = 0;
    for (size_t i = 0; i < suballoc2ndCount; ++i) {
      const DmaSuballocation &suballoc = suballocations2nd[i];
      const bool currFree = (suballoc.type == DMA_SUBALLOCATION_TYPE_FREE);

      DMA_VALIDATE(currFree == (suballoc.hAllocation == DMA_NULL_HANDLE));
      DMA_VALIDATE(suballoc.offset >= offset);

      if (!currFree) {
        DMA_VALIDATE(suballoc.hAllocation->GetOffset() == suballoc.offset);
        DMA_VALIDATE(suballoc.hAllocation->GetSize() == suballoc.size);
        sumUsedSize += suballoc.size;
      } else {
        ++nullItem2ndCount;
      }

      offset = suballoc.offset + suballoc.size + DMA_DEBUG_MARGIN;
    }

    DMA_VALIDATE(nullItem2ndCount == m_2ndNullItemsCount);
  }

  for (size_t i = 0; i < m_1stNullItemsBeginCount; ++i) {
    const DmaSuballocation &suballoc = suballocations1st[i];
    DMA_VALIDATE(suballoc.type == DMA_SUBALLOCATION_TYPE_FREE &&
                 suballoc.hAllocation == DMA_NULL_HANDLE);
  }

  size_t nullItem1stCount = m_1stNullItemsBeginCount;

  for (size_t i = m_1stNullItemsBeginCount; i < suballoc1stCount; ++i) {
    const DmaSuballocation &suballoc = suballocations1st[i];
    const bool currFree = (suballoc.type == DMA_SUBALLOCATION_TYPE_FREE);

    DMA_VALIDATE(currFree == (suballoc.hAllocation == DMA_NULL_HANDLE));
    DMA_VALIDATE(suballoc.offset >= offset);
    DMA_VALIDATE(i >= m_1stNullItemsBeginCount || currFree);

    if (!currFree) {
      DMA_VALIDATE(suballoc.hAllocation->GetOffset() == suballoc.offset);
      DMA_VALIDATE(suballoc.hAllocation->GetSize() == suballoc.size);
      sumUsedSize += suballoc.size;
    } else {
      ++nullItem1stCount;
    }

    offset = suballoc.offset + suballoc.size + DMA_DEBUG_MARGIN;
  }
  DMA_VALIDATE(nullItem1stCount ==
               m_1stNullItemsBeginCount + m_1stNullItemsMiddleCount);

  if (m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK) {
    const size_t suballoc2ndCount = suballocations2nd.size();
    size_t nullItem2ndCount = 0;
    for (size_t i = suballoc2ndCount; i--;) {
      const DmaSuballocation &suballoc = suballocations2nd[i];
      const bool currFree = (suballoc.type == DMA_SUBALLOCATION_TYPE_FREE);

      DMA_VALIDATE(currFree == (suballoc.hAllocation == DMA_NULL_HANDLE));
      DMA_VALIDATE(suballoc.offset >= offset);

      if (!currFree) {
        DMA_VALIDATE(suballoc.hAllocation->GetOffset() == suballoc.offset);
        DMA_VALIDATE(suballoc.hAllocation->GetSize() == suballoc.size);
        sumUsedSize += suballoc.size;
      } else {
        ++nullItem2ndCount;
      }

      offset = suballoc.offset + suballoc.size + DMA_DEBUG_MARGIN;
    }

    DMA_VALIDATE(nullItem2ndCount == m_2ndNullItemsCount);
  }

  DMA_VALIDATE(offset <= GetSize());
  DMA_VALIDATE(m_SumFreeSize == GetSize() - sumUsedSize);

  return true;
}

size_t DmaBlockMetadata_Linear::GetAllocationCount() const {
  return AccessSuballocations1st().size() -
         (m_1stNullItemsBeginCount + m_1stNullItemsMiddleCount) +
         AccessSuballocations2nd().size() - m_2ndNullItemsCount;
}

uint32_t DmaBlockMetadata_Linear::GetUnusedRangeSizeMax() const {
  const uint32_t size = GetSize();

  /*
  We don't consider gaps inside allocation vectors with freed allocations
  because they are not suitable for reuse in linear allocator. We consider only
  space that is available for new allocations.
  */
  if (IsEmpty()) {
    return size;
  }

  const SuballocationVectorType &suballocations1st = AccessSuballocations1st();

  switch (m_2ndVectorMode) {
  case SECOND_VECTOR_EMPTY:
    /*
    Available space is after end of 1st, as well as before beginning of 1st
    (which whould make it a ring buffer).
    */
    {
      const size_t suballocations1stCount = suballocations1st.size();
      DMA_ASSERT(suballocations1stCount > m_1stNullItemsBeginCount);
      const DmaSuballocation &firstSuballoc =
          suballocations1st[m_1stNullItemsBeginCount];
      const DmaSuballocation &lastSuballoc =
          suballocations1st[suballocations1stCount - 1];
      return DMA_MAX(firstSuballoc.offset,
                     size - (lastSuballoc.offset + lastSuballoc.size));
    }
    break;

  case SECOND_VECTOR_RING_BUFFER:
    /*
    Available space is only between end of 2nd and beginning of 1st.
    */
    {
      const SuballocationVectorType &suballocations2nd =
          AccessSuballocations2nd();
      const DmaSuballocation &lastSuballoc2nd = suballocations2nd.back();
      const DmaSuballocation &firstSuballoc1st =
          suballocations1st[m_1stNullItemsBeginCount];
      return firstSuballoc1st.offset -
             (lastSuballoc2nd.offset + lastSuballoc2nd.size);
    }
    break;

  case SECOND_VECTOR_DOUBLE_STACK:
    /*
    Available space is only between end of 1st and top of 2nd.
    */
    {
      const SuballocationVectorType &suballocations2nd =
          AccessSuballocations2nd();
      const DmaSuballocation &topSuballoc2nd = suballocations2nd.back();
      const DmaSuballocation &lastSuballoc1st = suballocations1st.back();
      return topSuballoc2nd.offset -
             (lastSuballoc1st.offset + lastSuballoc1st.size);
    }
    break;

  default:
    DMA_ASSERT(0);
    return 0;
  }
}

void DmaBlockMetadata_Linear::CalcAllocationStatInfo(
    DmaStatInfo &outInfo) const {
  const uint32_t size = GetSize();
  const SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  const SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();
  const size_t suballoc1stCount = suballocations1st.size();
  const size_t suballoc2ndCount = suballocations2nd.size();

  outInfo.blockCount = 1;
  outInfo.allocationCount = (uint32_t)GetAllocationCount();
  outInfo.unusedRangeCount = 0;
  outInfo.usedBytes = 0;
  outInfo.allocationSizeMin = UINT32_MAX;
  outInfo.allocationSizeMax = 0;
  outInfo.unusedRangeSizeMin = UINT32_MAX;
  outInfo.unusedRangeSizeMax = 0;

  uint32_t lastOffset = 0;

  if (m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER) {
    const uint32_t freeSpace2ndTo1stEnd =
        suballocations1st[m_1stNullItemsBeginCount].offset;
    size_t nextAlloc2ndIndex = 0;
    while (lastOffset < freeSpace2ndTo1stEnd) {
      // Find next non-null allocation or move nextAllocIndex to the end.
      while (nextAlloc2ndIndex < suballoc2ndCount &&
             suballocations2nd[nextAlloc2ndIndex].hAllocation ==
                 DMA_NULL_HANDLE) {
        ++nextAlloc2ndIndex;
      }

      // Found non-null allocation.
      if (nextAlloc2ndIndex < suballoc2ndCount) {
        const DmaSuballocation &suballoc = suballocations2nd[nextAlloc2ndIndex];

        // 1. Process free space before this allocation.
        if (lastOffset < suballoc.offset) {
          // There is free space from lastOffset to suballoc.offset.
          const uint32_t unusedRangeSize = suballoc.offset - lastOffset;
          ++outInfo.unusedRangeCount;
          outInfo.unusedBytes += unusedRangeSize;
          outInfo.unusedRangeSizeMin =
              DMA_MIN(outInfo.unusedRangeSizeMin, unusedRangeSize);
          outInfo.unusedRangeSizeMax =
              DMA_MIN(outInfo.unusedRangeSizeMax, unusedRangeSize);
        }

        // 2. Process this allocation.
        // There is allocation with suballoc.offset, suballoc.size.
        outInfo.usedBytes += suballoc.size;
        outInfo.allocationSizeMin =
            DMA_MIN(outInfo.allocationSizeMin, suballoc.size);
        outInfo.allocationSizeMax =
            DMA_MIN(outInfo.allocationSizeMax, suballoc.size);

        // 3. Prepare for next iteration.
        lastOffset = suballoc.offset + suballoc.size;
        ++nextAlloc2ndIndex;
      }
      // We are at the end.
      else {
        // There is free space from lastOffset to freeSpace2ndTo1stEnd.
        if (lastOffset < freeSpace2ndTo1stEnd) {
          const uint32_t unusedRangeSize = freeSpace2ndTo1stEnd - lastOffset;
          ++outInfo.unusedRangeCount;
          outInfo.unusedBytes += unusedRangeSize;
          outInfo.unusedRangeSizeMin =
              DMA_MIN(outInfo.unusedRangeSizeMin, unusedRangeSize);
          outInfo.unusedRangeSizeMax =
              DMA_MIN(outInfo.unusedRangeSizeMax, unusedRangeSize);
        }

        // End of loop.
        lastOffset = freeSpace2ndTo1stEnd;
      }
    }
  }

  size_t nextAlloc1stIndex = m_1stNullItemsBeginCount;
  const uint32_t freeSpace1stTo2ndEnd =
      m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK
          ? suballocations2nd.back().offset
          : size;
  while (lastOffset < freeSpace1stTo2ndEnd) {
    // Find next non-null allocation or move nextAllocIndex to the end.
    while (nextAlloc1stIndex < suballoc1stCount &&
           suballocations1st[nextAlloc1stIndex].hAllocation ==
               DMA_NULL_HANDLE) {
      ++nextAlloc1stIndex;
    }

    // Found non-null allocation.
    if (nextAlloc1stIndex < suballoc1stCount) {
      const DmaSuballocation &suballoc = suballocations1st[nextAlloc1stIndex];

      // 1. Process free space before this allocation.
      if (lastOffset < suballoc.offset) {
        // There is free space from lastOffset to suballoc.offset.
        const uint32_t unusedRangeSize = suballoc.offset - lastOffset;
        ++outInfo.unusedRangeCount;
        outInfo.unusedBytes += unusedRangeSize;
        outInfo.unusedRangeSizeMin =
            DMA_MIN(outInfo.unusedRangeSizeMin, unusedRangeSize);
        outInfo.unusedRangeSizeMax =
            DMA_MIN(outInfo.unusedRangeSizeMax, unusedRangeSize);
      }

      // 2. Process this allocation.
      // There is allocation with suballoc.offset, suballoc.size.
      outInfo.usedBytes += suballoc.size;
      outInfo.allocationSizeMin =
          DMA_MIN(outInfo.allocationSizeMin, suballoc.size);
      outInfo.allocationSizeMax =
          DMA_MIN(outInfo.allocationSizeMax, suballoc.size);

      // 3. Prepare for next iteration.
      lastOffset = suballoc.offset + suballoc.size;
      ++nextAlloc1stIndex;
    }
    // We are at the end.
    else {
      // There is free space from lastOffset to freeSpace1stTo2ndEnd.
      if (lastOffset < freeSpace1stTo2ndEnd) {
        const uint32_t unusedRangeSize = freeSpace1stTo2ndEnd - lastOffset;
        ++outInfo.unusedRangeCount;
        outInfo.unusedBytes += unusedRangeSize;
        outInfo.unusedRangeSizeMin =
            DMA_MIN(outInfo.unusedRangeSizeMin, unusedRangeSize);
        outInfo.unusedRangeSizeMax =
            DMA_MIN(outInfo.unusedRangeSizeMax, unusedRangeSize);
      }

      // End of loop.
      lastOffset = freeSpace1stTo2ndEnd;
    }
  }

  if (m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK) {
    size_t nextAlloc2ndIndex = suballocations2nd.size() - 1;
    while (lastOffset < size) {
      // Find next non-null allocation or move nextAllocIndex to the end.
      while (nextAlloc2ndIndex != SIZE_MAX &&
             suballocations2nd[nextAlloc2ndIndex].hAllocation ==
                 DMA_NULL_HANDLE) {
        --nextAlloc2ndIndex;
      }

      // Found non-null allocation.
      if (nextAlloc2ndIndex != SIZE_MAX) {
        const DmaSuballocation &suballoc = suballocations2nd[nextAlloc2ndIndex];

        // 1. Process free space before this allocation.
        if (lastOffset < suballoc.offset) {
          // There is free space from lastOffset to suballoc.offset.
          const uint32_t unusedRangeSize = suballoc.offset - lastOffset;
          ++outInfo.unusedRangeCount;
          outInfo.unusedBytes += unusedRangeSize;
          outInfo.unusedRangeSizeMin =
              DMA_MIN(outInfo.unusedRangeSizeMin, unusedRangeSize);
          outInfo.unusedRangeSizeMax =
              DMA_MIN(outInfo.unusedRangeSizeMax, unusedRangeSize);
        }

        // 2. Process this allocation.
        // There is allocation with suballoc.offset, suballoc.size.
        outInfo.usedBytes += suballoc.size;
        outInfo.allocationSizeMin =
            DMA_MIN(outInfo.allocationSizeMin, suballoc.size);
        outInfo.allocationSizeMax =
            DMA_MIN(outInfo.allocationSizeMax, suballoc.size);

        // 3. Prepare for next iteration.
        lastOffset = suballoc.offset + suballoc.size;
        --nextAlloc2ndIndex;
      }
      // We are at the end.
      else {
        // There is free space from lastOffset to size.
        if (lastOffset < size) {
          const uint32_t unusedRangeSize = size - lastOffset;
          ++outInfo.unusedRangeCount;
          outInfo.unusedBytes += unusedRangeSize;
          outInfo.unusedRangeSizeMin =
              DMA_MIN(outInfo.unusedRangeSizeMin, unusedRangeSize);
          outInfo.unusedRangeSizeMax =
              DMA_MIN(outInfo.unusedRangeSizeMax, unusedRangeSize);
        }

        // End of loop.
        lastOffset = size;
      }
    }
  }

  outInfo.unusedBytes = size - outInfo.usedBytes;
}

void DmaBlockMetadata_Linear::AddPoolStats(DmaPoolStats &inoutStats) const {
  const SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  const SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();
  const uint32_t size = GetSize();
  const size_t suballoc1stCount = suballocations1st.size();
  const size_t suballoc2ndCount = suballocations2nd.size();

  inoutStats.size += size;

  uint32_t lastOffset = 0;

  if (m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER) {
    const uint32_t freeSpace2ndTo1stEnd =
        suballocations1st[m_1stNullItemsBeginCount].offset;
    size_t nextAlloc2ndIndex = m_1stNullItemsBeginCount;
    while (lastOffset < freeSpace2ndTo1stEnd) {
      // Find next non-null allocation or move nextAlloc2ndIndex to the end.
      while (nextAlloc2ndIndex < suballoc2ndCount &&
             suballocations2nd[nextAlloc2ndIndex].hAllocation ==
                 DMA_NULL_HANDLE) {
        ++nextAlloc2ndIndex;
      }

      // Found non-null allocation.
      if (nextAlloc2ndIndex < suballoc2ndCount) {
        const DmaSuballocation &suballoc = suballocations2nd[nextAlloc2ndIndex];

        // 1. Process free space before this allocation.
        if (lastOffset < suballoc.offset) {
          // There is free space from lastOffset to suballoc.offset.
          const uint32_t unusedRangeSize = suballoc.offset - lastOffset;
          inoutStats.unusedSize += unusedRangeSize;
          ++inoutStats.unusedRangeCount;
          inoutStats.unusedRangeSizeMax =
              DMA_MAX(inoutStats.unusedRangeSizeMax, unusedRangeSize);
        }

        // 2. Process this allocation.
        // There is allocation with suballoc.offset, suballoc.size.
        ++inoutStats.allocationCount;

        // 3. Prepare for next iteration.
        lastOffset = suballoc.offset + suballoc.size;
        ++nextAlloc2ndIndex;
      }
      // We are at the end.
      else {
        if (lastOffset < freeSpace2ndTo1stEnd) {
          // There is free space from lastOffset to freeSpace2ndTo1stEnd.
          const uint32_t unusedRangeSize = freeSpace2ndTo1stEnd - lastOffset;
          inoutStats.unusedSize += unusedRangeSize;
          ++inoutStats.unusedRangeCount;
          inoutStats.unusedRangeSizeMax =
              DMA_MAX(inoutStats.unusedRangeSizeMax, unusedRangeSize);
        }

        // End of loop.
        lastOffset = freeSpace2ndTo1stEnd;
      }
    }
  }

  size_t nextAlloc1stIndex = m_1stNullItemsBeginCount;
  const uint32_t freeSpace1stTo2ndEnd =
      m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK
          ? suballocations2nd.back().offset
          : size;
  while (lastOffset < freeSpace1stTo2ndEnd) {
    // Find next non-null allocation or move nextAllocIndex to the end.
    while (nextAlloc1stIndex < suballoc1stCount &&
           suballocations1st[nextAlloc1stIndex].hAllocation ==
               DMA_NULL_HANDLE) {
      ++nextAlloc1stIndex;
    }

    // Found non-null allocation.
    if (nextAlloc1stIndex < suballoc1stCount) {
      const DmaSuballocation &suballoc = suballocations1st[nextAlloc1stIndex];

      // 1. Process free space before this allocation.
      if (lastOffset < suballoc.offset) {
        // There is free space from lastOffset to suballoc.offset.
        const uint32_t unusedRangeSize = suballoc.offset - lastOffset;
        inoutStats.unusedSize += unusedRangeSize;
        ++inoutStats.unusedRangeCount;
        inoutStats.unusedRangeSizeMax =
            DMA_MAX(inoutStats.unusedRangeSizeMax, unusedRangeSize);
      }

      // 2. Process this allocation.
      // There is allocation with suballoc.offset, suballoc.size.
      ++inoutStats.allocationCount;

      // 3. Prepare for next iteration.
      lastOffset = suballoc.offset + suballoc.size;
      ++nextAlloc1stIndex;
    }
    // We are at the end.
    else {
      if (lastOffset < freeSpace1stTo2ndEnd) {
        // There is free space from lastOffset to freeSpace1stTo2ndEnd.
        const uint32_t unusedRangeSize = freeSpace1stTo2ndEnd - lastOffset;
        inoutStats.unusedSize += unusedRangeSize;
        ++inoutStats.unusedRangeCount;
        inoutStats.unusedRangeSizeMax =
            DMA_MAX(inoutStats.unusedRangeSizeMax, unusedRangeSize);
      }

      // End of loop.
      lastOffset = freeSpace1stTo2ndEnd;
    }
  }

  if (m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK) {
    size_t nextAlloc2ndIndex = suballocations2nd.size() - 1;
    while (lastOffset < size) {
      // Find next non-null allocation or move nextAlloc2ndIndex to the end.
      while (nextAlloc2ndIndex != SIZE_MAX &&
             suballocations2nd[nextAlloc2ndIndex].hAllocation ==
                 DMA_NULL_HANDLE) {
        --nextAlloc2ndIndex;
      }

      // Found non-null allocation.
      if (nextAlloc2ndIndex != SIZE_MAX) {
        const DmaSuballocation &suballoc = suballocations2nd[nextAlloc2ndIndex];

        // 1. Process free space before this allocation.
        if (lastOffset < suballoc.offset) {
          // There is free space from lastOffset to suballoc.offset.
          const uint32_t unusedRangeSize = suballoc.offset - lastOffset;
          inoutStats.unusedSize += unusedRangeSize;
          ++inoutStats.unusedRangeCount;
          inoutStats.unusedRangeSizeMax =
              DMA_MAX(inoutStats.unusedRangeSizeMax, unusedRangeSize);
        }

        // 2. Process this allocation.
        // There is allocation with suballoc.offset, suballoc.size.
        ++inoutStats.allocationCount;

        // 3. Prepare for next iteration.
        lastOffset = suballoc.offset + suballoc.size;
        --nextAlloc2ndIndex;
      }
      // We are at the end.
      else {
        if (lastOffset < size) {
          // There is free space from lastOffset to size.
          const uint32_t unusedRangeSize = size - lastOffset;
          inoutStats.unusedSize += unusedRangeSize;
          ++inoutStats.unusedRangeCount;
          inoutStats.unusedRangeSizeMax =
              DMA_MAX(inoutStats.unusedRangeSizeMax, unusedRangeSize);
        }

        // End of loop.
        lastOffset = size;
      }
    }
  }
}

#if DMA_STATS_STRING_ENABLED
void DmaBlockMetadata_Linear::PrintDetailedMap(
    class DmaJsonWriter &json) const {
  const uint32_t size = GetSize();
  const SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  const SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();
  const size_t suballoc1stCount = suballocations1st.size();
  const size_t suballoc2ndCount = suballocations2nd.size();

  // FIRST PASS

  size_t unusedRangeCount = 0;
  uint32_t usedBytes = 0;

  uint32_t lastOffset = 0;

  size_t alloc2ndCount = 0;
  if (m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER) {
    const uint32_t freeSpace2ndTo1stEnd =
        suballocations1st[m_1stNullItemsBeginCount].offset;
    size_t nextAlloc2ndIndex = 0;
    while (lastOffset < freeSpace2ndTo1stEnd) {
      // Find next non-null allocation or move nextAlloc2ndIndex to the end.
      while (nextAlloc2ndIndex < suballoc2ndCount &&
             suballocations2nd[nextAlloc2ndIndex].hAllocation ==
                 DMA_NULL_HANDLE) {
        ++nextAlloc2ndIndex;
      }

      // Found non-null allocation.
      if (nextAlloc2ndIndex < suballoc2ndCount) {
        const DmaSuballocation &suballoc = suballocations2nd[nextAlloc2ndIndex];

        // 1. Process free space before this allocation.
        if (lastOffset < suballoc.offset) {
          // There is free space from lastOffset to suballoc.offset.
          ++unusedRangeCount;
        }

        // 2. Process this allocation.
        // There is allocation with suballoc.offset, suballoc.size.
        ++alloc2ndCount;
        usedBytes += suballoc.size;

        // 3. Prepare for next iteration.
        lastOffset = suballoc.offset + suballoc.size;
        ++nextAlloc2ndIndex;
      }
      // We are at the end.
      else {
        if (lastOffset < freeSpace2ndTo1stEnd) {
          // There is free space from lastOffset to freeSpace2ndTo1stEnd.
          ++unusedRangeCount;
        }

        // End of loop.
        lastOffset = freeSpace2ndTo1stEnd;
      }
    }
  }

  size_t nextAlloc1stIndex = m_1stNullItemsBeginCount;
  size_t alloc1stCount = 0;
  const uint32_t freeSpace1stTo2ndEnd =
      m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK
          ? suballocations2nd.back().offset
          : size;
  while (lastOffset < freeSpace1stTo2ndEnd) {
    // Find next non-null allocation or move nextAllocIndex to the end.
    while (nextAlloc1stIndex < suballoc1stCount &&
           suballocations1st[nextAlloc1stIndex].hAllocation ==
               DMA_NULL_HANDLE) {
      ++nextAlloc1stIndex;
    }

    // Found non-null allocation.
    if (nextAlloc1stIndex < suballoc1stCount) {
      const DmaSuballocation &suballoc = suballocations1st[nextAlloc1stIndex];

      // 1. Process free space before this allocation.
      if (lastOffset < suballoc.offset) {
        // There is free space from lastOffset to suballoc.offset.
        ++unusedRangeCount;
      }

      // 2. Process this allocation.
      // There is allocation with suballoc.offset, suballoc.size.
      ++alloc1stCount;
      usedBytes += suballoc.size;

      // 3. Prepare for next iteration.
      lastOffset = suballoc.offset + suballoc.size;
      ++nextAlloc1stIndex;
    }
    // We are at the end.
    else {
      if (lastOffset < size) {
        // There is free space from lastOffset to freeSpace1stTo2ndEnd.
        ++unusedRangeCount;
      }

      // End of loop.
      lastOffset = freeSpace1stTo2ndEnd;
    }
  }

  if (m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK) {
    size_t nextAlloc2ndIndex = suballocations2nd.size() - 1;
    while (lastOffset < size) {
      // Find next non-null allocation or move nextAlloc2ndIndex to the end.
      while (nextAlloc2ndIndex != SIZE_MAX &&
             suballocations2nd[nextAlloc2ndIndex].hAllocation ==
                 DMA_NULL_HANDLE) {
        --nextAlloc2ndIndex;
      }

      // Found non-null allocation.
      if (nextAlloc2ndIndex != SIZE_MAX) {
        const DmaSuballocation &suballoc = suballocations2nd[nextAlloc2ndIndex];

        // 1. Process free space before this allocation.
        if (lastOffset < suballoc.offset) {
          // There is free space from lastOffset to suballoc.offset.
          ++unusedRangeCount;
        }

        // 2. Process this allocation.
        // There is allocation with suballoc.offset, suballoc.size.
        ++alloc2ndCount;
        usedBytes += suballoc.size;

        // 3. Prepare for next iteration.
        lastOffset = suballoc.offset + suballoc.size;
        --nextAlloc2ndIndex;
      }
      // We are at the end.
      else {
        if (lastOffset < size) {
          // There is free space from lastOffset to size.
          ++unusedRangeCount;
        }

        // End of loop.
        lastOffset = size;
      }
    }
  }

  const uint32_t unusedBytes = size - usedBytes;
  PrintDetailedMap_Begin(json, unusedBytes, alloc1stCount + alloc2ndCount,
                         unusedRangeCount);

  // SECOND PASS
  lastOffset = 0;

  if (m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER) {
    const uint32_t freeSpace2ndTo1stEnd =
        suballocations1st[m_1stNullItemsBeginCount].offset;
    size_t nextAlloc2ndIndex = 0;
    while (lastOffset < freeSpace2ndTo1stEnd) {
      // Find next non-null allocation or move nextAlloc2ndIndex to the end.
      while (nextAlloc2ndIndex < suballoc2ndCount &&
             suballocations2nd[nextAlloc2ndIndex].hAllocation ==
                 DMA_NULL_HANDLE) {
        ++nextAlloc2ndIndex;
      }

      // Found non-null allocation.
      if (nextAlloc2ndIndex < suballoc2ndCount) {
        const DmaSuballocation &suballoc = suballocations2nd[nextAlloc2ndIndex];

        // 1. Process free space before this allocation.
        if (lastOffset < suballoc.offset) {
          // There is free space from lastOffset to suballoc.offset.
          const uint32_t unusedRangeSize = suballoc.offset - lastOffset;
          PrintDetailedMap_UnusedRange(json, lastOffset, unusedRangeSize);
        }

        // 2. Process this allocation.
        // There is allocation with suballoc.offset, suballoc.size.
        PrintDetailedMap_Allocation(json, suballoc.offset,
                                    suballoc.hAllocation);

        // 3. Prepare for next iteration.
        lastOffset = suballoc.offset + suballoc.size;
        ++nextAlloc2ndIndex;
      }
      // We are at the end.
      else {
        if (lastOffset < freeSpace2ndTo1stEnd) {
          // There is free space from lastOffset to freeSpace2ndTo1stEnd.
          const uint32_t unusedRangeSize = freeSpace2ndTo1stEnd - lastOffset;
          PrintDetailedMap_UnusedRange(json, lastOffset, unusedRangeSize);
        }

        // End of loop.
        lastOffset = freeSpace2ndTo1stEnd;
      }
    }
  }

  nextAlloc1stIndex = m_1stNullItemsBeginCount;
  while (lastOffset < freeSpace1stTo2ndEnd) {
    // Find next non-null allocation or move nextAllocIndex to the end.
    while (nextAlloc1stIndex < suballoc1stCount &&
           suballocations1st[nextAlloc1stIndex].hAllocation ==
               DMA_NULL_HANDLE) {
      ++nextAlloc1stIndex;
    }

    // Found non-null allocation.
    if (nextAlloc1stIndex < suballoc1stCount) {
      const DmaSuballocation &suballoc = suballocations1st[nextAlloc1stIndex];

      // 1. Process free space before this allocation.
      if (lastOffset < suballoc.offset) {
        // There is free space from lastOffset to suballoc.offset.
        const uint32_t unusedRangeSize = suballoc.offset - lastOffset;
        PrintDetailedMap_UnusedRange(json, lastOffset, unusedRangeSize);
      }

      // 2. Process this allocation.
      // There is allocation with suballoc.offset, suballoc.size.
      PrintDetailedMap_Allocation(json, suballoc.offset, suballoc.hAllocation);

      // 3. Prepare for next iteration.
      lastOffset = suballoc.offset + suballoc.size;
      ++nextAlloc1stIndex;
    }
    // We are at the end.
    else {
      if (lastOffset < freeSpace1stTo2ndEnd) {
        // There is free space from lastOffset to freeSpace1stTo2ndEnd.
        const uint32_t unusedRangeSize = freeSpace1stTo2ndEnd - lastOffset;
        PrintDetailedMap_UnusedRange(json, lastOffset, unusedRangeSize);
      }

      // End of loop.
      lastOffset = freeSpace1stTo2ndEnd;
    }
  }

  if (m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK) {
    size_t nextAlloc2ndIndex = suballocations2nd.size() - 1;
    while (lastOffset < size) {
      // Find next non-null allocation or move nextAlloc2ndIndex to the end.
      while (nextAlloc2ndIndex != SIZE_MAX &&
             suballocations2nd[nextAlloc2ndIndex].hAllocation ==
                 DMA_NULL_HANDLE) {
        --nextAlloc2ndIndex;
      }

      // Found non-null allocation.
      if (nextAlloc2ndIndex != SIZE_MAX) {
        const DmaSuballocation &suballoc = suballocations2nd[nextAlloc2ndIndex];

        // 1. Process free space before this allocation.
        if (lastOffset < suballoc.offset) {
          // There is free space from lastOffset to suballoc.offset.
          const uint32_t unusedRangeSize = suballoc.offset - lastOffset;
          PrintDetailedMap_UnusedRange(json, lastOffset, unusedRangeSize);
        }

        // 2. Process this allocation.
        // There is allocation with suballoc.offset, suballoc.size.
        PrintDetailedMap_Allocation(json, suballoc.offset,
                                    suballoc.hAllocation);

        // 3. Prepare for next iteration.
        lastOffset = suballoc.offset + suballoc.size;
        --nextAlloc2ndIndex;
      }
      // We are at the end.
      else {
        if (lastOffset < size) {
          // There is free space from lastOffset to size.
          const uint32_t unusedRangeSize = size - lastOffset;
          PrintDetailedMap_UnusedRange(json, lastOffset, unusedRangeSize);
        }

        // End of loop.
        lastOffset = size;
      }
    }
  }

  PrintDetailedMap_End(json);
}
#endif // #if DMA_STATS_STRING_ENABLED

bool DmaBlockMetadata_Linear::CreateAllocationRequest(
    uint32_t currentFrameIndex, uint32_t frameInUseCount, uint32_t allocSize,
    uint32_t allocAlignment, bool upperAddress, DmaSuballocationType allocType,
    bool canMakeOtherLost, uint32_t strategy,
    DmaAllocationRequest *pAllocationRequest) {
  DMA_ASSERT(allocSize > 0);
  DMA_ASSERT(allocType != DMA_SUBALLOCATION_TYPE_FREE);
  DMA_ASSERT(pAllocationRequest != DMA_NULL);
  DMA_HEAVY_ASSERT(Validate());
  return upperAddress ? CreateAllocationRequest_UpperAddress(
                            currentFrameIndex, frameInUseCount, allocSize,
                            allocAlignment, allocType, canMakeOtherLost,
                            strategy, pAllocationRequest)
                      : CreateAllocationRequest_LowerAddress(
                            currentFrameIndex, frameInUseCount, allocSize,
                            allocAlignment, allocType, canMakeOtherLost,
                            strategy, pAllocationRequest);
}

bool DmaBlockMetadata_Linear::CreateAllocationRequest_UpperAddress(
    uint32_t currentFrameIndex, uint32_t frameInUseCount, uint32_t allocSize,
    uint32_t allocAlignment, DmaSuballocationType allocType,
    bool canMakeOtherLost, uint32_t strategy,
    DmaAllocationRequest *pAllocationRequest) {
  const uint32_t size = GetSize();
  SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();

  if (m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER) {
    DMA_ASSERT(0 && "Trying to use pool with linear algorithm as double stack, "
                    "while it is already being used as ring buffer.");
    return false;
  }

  // Try to allocate before 2nd.back(), or end of block if 2nd.empty().
  if (allocSize > size) {
    return false;
  }
  uint32_t resultBaseOffset = size - allocSize;
  if (!suballocations2nd.empty()) {
    const DmaSuballocation &lastSuballoc = suballocations2nd.back();
    resultBaseOffset = lastSuballoc.offset - allocSize;
    if (allocSize > lastSuballoc.offset) {
      return false;
    }
  }

  // Start from offset equal to end of free space.
  uint32_t resultOffset = resultBaseOffset;

  // Apply DMA_DEBUG_MARGIN at the end.
  if (DMA_DEBUG_MARGIN > 0) {
    if (resultOffset < DMA_DEBUG_MARGIN) {
      return false;
    }
    resultOffset -= DMA_DEBUG_MARGIN;
  }

  // Apply alignment.
  resultOffset = DmaAlignDown(resultOffset, allocAlignment);

  // There is enough free space.
  const uint32_t endOf1st =
      !suballocations1st.empty()
          ? suballocations1st.back().offset + suballocations1st.back().size
          : 0;
  if (endOf1st + DMA_DEBUG_MARGIN <= resultOffset) {
    // All tests passed: Success.
    pAllocationRequest->offset = resultOffset;
    pAllocationRequest->sumFreeSize = resultBaseOffset + allocSize - endOf1st;
    pAllocationRequest->sumItemSize = 0;
    // pAllocationRequest->item unused.
    pAllocationRequest->itemsToMakeLostCount = 0;
    pAllocationRequest->type = DmaAllocationRequestType::UpperAddress;
    return true;
  }

  return false;
}

bool DmaBlockMetadata_Linear::CreateAllocationRequest_LowerAddress(
    uint32_t currentFrameIndex, uint32_t frameInUseCount, uint32_t allocSize,
    uint32_t allocAlignment, DmaSuballocationType allocType,
    bool canMakeOtherLost, uint32_t strategy,
    DmaAllocationRequest *pAllocationRequest) {
  const uint32_t size = GetSize();
  SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();

  if (m_2ndVectorMode == SECOND_VECTOR_EMPTY ||
      m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK) {
    // Try to allocate at the end of 1st vector.

    uint32_t resultBaseOffset = 0;
    if (!suballocations1st.empty()) {
      const DmaSuballocation &lastSuballoc = suballocations1st.back();
      resultBaseOffset = lastSuballoc.offset + lastSuballoc.size;
    }

    // Start from offset equal to beginning of free space.
    uint32_t resultOffset = resultBaseOffset;

    // Apply DMA_DEBUG_MARGIN at the beginning.
    if (DMA_DEBUG_MARGIN > 0) {
      resultOffset += DMA_DEBUG_MARGIN;
    }

    // Apply alignment.
    resultOffset = DmaAlignUp(resultOffset, allocAlignment);

    const uint32_t freeSpaceEnd = m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK
                                      ? suballocations2nd.back().offset
                                      : size;

    // There is enough free space at the end after alignment.
    if (resultOffset + allocSize + DMA_DEBUG_MARGIN <= freeSpaceEnd) {
      // All tests passed: Success.
      pAllocationRequest->offset = resultOffset;
      pAllocationRequest->sumFreeSize = freeSpaceEnd - resultBaseOffset;
      pAllocationRequest->sumItemSize = 0;
      // pAllocationRequest->item, customData unused.
      pAllocationRequest->type = DmaAllocationRequestType::EndOf1st;
      pAllocationRequest->itemsToMakeLostCount = 0;
      return true;
    }
  }

  // Wrap-around to end of 2nd vector. Try to allocate there, watching for the
  // beginning of 1st vector as the end of free space.
  if (m_2ndVectorMode == SECOND_VECTOR_EMPTY ||
      m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER) {
    DMA_ASSERT(!suballocations1st.empty());

    uint32_t resultBaseOffset = 0;
    if (!suballocations2nd.empty()) {
      const DmaSuballocation &lastSuballoc = suballocations2nd.back();
      resultBaseOffset = lastSuballoc.offset + lastSuballoc.size;
    }

    // Start from offset equal to beginning of free space.
    uint32_t resultOffset = resultBaseOffset;

    // Apply DMA_DEBUG_MARGIN at the beginning.
    if (DMA_DEBUG_MARGIN > 0) {
      resultOffset += DMA_DEBUG_MARGIN;
    }

    // Apply alignment.
    resultOffset = DmaAlignUp(resultOffset, allocAlignment);

    pAllocationRequest->itemsToMakeLostCount = 0;
    pAllocationRequest->sumItemSize = 0;
    size_t index1st = m_1stNullItemsBeginCount;

    if (canMakeOtherLost) {
      while (index1st < suballocations1st.size() &&
             resultOffset + allocSize + DMA_DEBUG_MARGIN >
                 suballocations1st[index1st].offset) {
        // Next colliding allocation at the beginning of 1st vector found. Try
        // to make it lost.
        const DmaSuballocation &suballoc = suballocations1st[index1st];
        if (suballoc.type == DMA_SUBALLOCATION_TYPE_FREE) {
          // No problem.
        } else {
          DMA_ASSERT(suballoc.hAllocation != DMA_NULL_HANDLE);
          if (suballoc.hAllocation->CanBecomeLost() &&
              suballoc.hAllocation->GetLastUseFrameIndex() + frameInUseCount <
                  currentFrameIndex) {
            ++pAllocationRequest->itemsToMakeLostCount;
            pAllocationRequest->sumItemSize += suballoc.size;
          } else {
            return false;
          }
        }
        ++index1st;
      }

      // Special case: There is not enough room at the end for this allocation,
      // even after making all from the 1st lost.
      if (index1st == suballocations1st.size() &&
          resultOffset + allocSize + DMA_DEBUG_MARGIN > size) {
        // TODO: This is a known bug that it's not yet implemented and the
        // allocation is failing.
        DMA_DEBUG_LOG(
            "Unsupported special case in custom pool with linear allocation "
            "algorithm used as ring buffer with allocations that can be lost.");
      }
    }

    // There is enough free space at the end after alignment.
    if ((index1st == suballocations1st.size() &&
         resultOffset + allocSize + DMA_DEBUG_MARGIN <= size) ||
        (index1st < suballocations1st.size() &&
         resultOffset + allocSize + DMA_DEBUG_MARGIN <=
             suballocations1st[index1st].offset)) {
      // All tests passed: Success.
      pAllocationRequest->offset = resultOffset;
      pAllocationRequest->sumFreeSize =
          (index1st < suballocations1st.size()
               ? suballocations1st[index1st].offset
               : size) -
          resultBaseOffset - pAllocationRequest->sumItemSize;
      pAllocationRequest->type = DmaAllocationRequestType::EndOf2nd;
      // pAllocationRequest->item, customData unused.
      return true;
    }
  }

  return false;
}

bool DmaBlockMetadata_Linear::MakeRequestedAllocationsLost(
    uint32_t currentFrameIndex, uint32_t frameInUseCount,
    DmaAllocationRequest *pAllocationRequest) {
  if (pAllocationRequest->itemsToMakeLostCount == 0) {
    return true;
  }

  DMA_ASSERT(m_2ndVectorMode == SECOND_VECTOR_EMPTY ||
             m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER);

  // We always start from 1st.
  SuballocationVectorType *suballocations = &AccessSuballocations1st();
  size_t index = m_1stNullItemsBeginCount;
  size_t madeLostCount = 0;
  while (madeLostCount < pAllocationRequest->itemsToMakeLostCount) {
    if (index == suballocations->size()) {
      index = 0;
      // If we get to the end of 1st, we wrap around to beginning of 2nd of 1st.
      if (m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER) {
        suballocations = &AccessSuballocations2nd();
      }
      // else: m_2ndVectorMode == SECOND_VECTOR_EMPTY:
      // suballocations continues pointing at AccessSuballocations1st().
      DMA_ASSERT(!suballocations->empty());
    }
    DmaSuballocation &suballoc = (*suballocations)[index];
    if (suballoc.type != DMA_SUBALLOCATION_TYPE_FREE) {
      DMA_ASSERT(suballoc.hAllocation != DMA_NULL_HANDLE);
      DMA_ASSERT(suballoc.hAllocation->CanBecomeLost());
      if (suballoc.hAllocation->MakeLost(currentFrameIndex, frameInUseCount)) {
        suballoc.type = DMA_SUBALLOCATION_TYPE_FREE;
        suballoc.hAllocation = DMA_NULL_HANDLE;
        m_SumFreeSize += suballoc.size;
        if (suballocations == &AccessSuballocations1st()) {
          ++m_1stNullItemsMiddleCount;
        } else {
          ++m_2ndNullItemsCount;
        }
        ++madeLostCount;
      } else {
        return false;
      }
    }
    ++index;
  }

  CleanupAfterFree();
  // DMA_HEAVY_ASSERT(Validate()); // Already called by ClanupAfterFree().

  return true;
}

uint32_t
DmaBlockMetadata_Linear::MakeAllocationsLost(uint32_t currentFrameIndex,
                                             uint32_t frameInUseCount) {
  uint32_t lostAllocationCount = 0;

  SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  for (size_t i = m_1stNullItemsBeginCount, count = suballocations1st.size();
       i < count; ++i) {
    DmaSuballocation &suballoc = suballocations1st[i];
    if (suballoc.type != DMA_SUBALLOCATION_TYPE_FREE &&
        suballoc.hAllocation->CanBecomeLost() &&
        suballoc.hAllocation->MakeLost(currentFrameIndex, frameInUseCount)) {
      suballoc.type = DMA_SUBALLOCATION_TYPE_FREE;
      suballoc.hAllocation = DMA_NULL_HANDLE;
      ++m_1stNullItemsMiddleCount;
      m_SumFreeSize += suballoc.size;
      ++lostAllocationCount;
    }
  }

  SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();
  for (size_t i = 0, count = suballocations2nd.size(); i < count; ++i) {
    DmaSuballocation &suballoc = suballocations2nd[i];
    if (suballoc.type != DMA_SUBALLOCATION_TYPE_FREE &&
        suballoc.hAllocation->CanBecomeLost() &&
        suballoc.hAllocation->MakeLost(currentFrameIndex, frameInUseCount)) {
      suballoc.type = DMA_SUBALLOCATION_TYPE_FREE;
      suballoc.hAllocation = DMA_NULL_HANDLE;
      ++m_2ndNullItemsCount;
      m_SumFreeSize += suballoc.size;
      ++lostAllocationCount;
    }
  }

  if (lostAllocationCount) {
    CleanupAfterFree();
  }

  return lostAllocationCount;
}

DkResult DmaBlockMetadata_Linear::CheckCorruption(const void *pBlockData) {
  SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  for (size_t i = m_1stNullItemsBeginCount, count = suballocations1st.size();
       i < count; ++i) {
    const DmaSuballocation &suballoc = suballocations1st[i];
    if (suballoc.type != DMA_SUBALLOCATION_TYPE_FREE) {
      if (!DmaValidateMagicValue(pBlockData,
                                 suballoc.offset - DMA_DEBUG_MARGIN)) {
        DMA_ASSERT(0 &&
                   "MEMORY CORRUPTION DETECTED BEFORE VALIDATED ALLOCATION!");
        return DkResult_Fail;
      }
      if (!DmaValidateMagicValue(pBlockData, suballoc.offset + suballoc.size)) {
        DMA_ASSERT(0 &&
                   "MEMORY CORRUPTION DETECTED AFTER VALIDATED ALLOCATION!");
        return DkResult_Fail;
      }
    }
  }

  SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();
  for (size_t i = 0, count = suballocations2nd.size(); i < count; ++i) {
    const DmaSuballocation &suballoc = suballocations2nd[i];
    if (suballoc.type != DMA_SUBALLOCATION_TYPE_FREE) {
      if (!DmaValidateMagicValue(pBlockData,
                                 suballoc.offset - DMA_DEBUG_MARGIN)) {
        DMA_ASSERT(0 &&
                   "MEMORY CORRUPTION DETECTED BEFORE VALIDATED ALLOCATION!");
        return DkResult_Fail;
      }
      if (!DmaValidateMagicValue(pBlockData, suballoc.offset + suballoc.size)) {
        DMA_ASSERT(0 &&
                   "MEMORY CORRUPTION DETECTED AFTER VALIDATED ALLOCATION!");
        return DkResult_Fail;
      }
    }
  }

  return DkResult_Success;
}

void DmaBlockMetadata_Linear::Alloc(const DmaAllocationRequest &request,
                                    DmaSuballocationType type,
                                    uint32_t allocSize,
                                    DmaAllocation hAllocation) {
  const DmaSuballocation newSuballoc = {request.offset, allocSize, hAllocation,
                                        type};

  switch (request.type) {
  case DmaAllocationRequestType::UpperAddress: {
    DMA_ASSERT(m_2ndVectorMode != SECOND_VECTOR_RING_BUFFER &&
               "CRITICAL ERROR: Trying to use linear allocator as double stack "
               "while it was already used as ring buffer.");
    SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();
    suballocations2nd.push_back(newSuballoc);
    m_2ndVectorMode = SECOND_VECTOR_DOUBLE_STACK;
  } break;
  case DmaAllocationRequestType::EndOf1st: {
    SuballocationVectorType &suballocations1st = AccessSuballocations1st();

    DMA_ASSERT(suballocations1st.empty() ||
               request.offset >= suballocations1st.back().offset +
                                     suballocations1st.back().size);
    // Check if it fits before the end of the block.
    DMA_ASSERT(request.offset + allocSize <= GetSize());

    suballocations1st.push_back(newSuballoc);
  } break;
  case DmaAllocationRequestType::EndOf2nd: {
    SuballocationVectorType &suballocations1st = AccessSuballocations1st();
    // New allocation at the end of 2-part ring buffer, so before first
    // allocation from 1st vector.
    DMA_ASSERT(!suballocations1st.empty() &&
               request.offset + allocSize <=
                   suballocations1st[m_1stNullItemsBeginCount].offset);
    SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();

    switch (m_2ndVectorMode) {
    case SECOND_VECTOR_EMPTY:
      // First allocation from second part ring buffer.
      DMA_ASSERT(suballocations2nd.empty());
      m_2ndVectorMode = SECOND_VECTOR_RING_BUFFER;
      break;
    case SECOND_VECTOR_RING_BUFFER:
      // 2-part ring buffer is already started.
      DMA_ASSERT(!suballocations2nd.empty());
      break;
    case SECOND_VECTOR_DOUBLE_STACK:
      DMA_ASSERT(0 && "CRITICAL ERROR: Trying to use linear allocator as ring "
                      "buffer while it was already used as double stack.");
      break;
    default:
      DMA_ASSERT(0);
    }

    suballocations2nd.push_back(newSuballoc);
  } break;
  default:
    DMA_ASSERT(0 && "CRITICAL INTERNAL ERROR.");
  }

  m_SumFreeSize -= newSuballoc.size;
}

void DmaBlockMetadata_Linear::Free(const DmaAllocation allocation) {
  FreeAtOffset(allocation->GetOffset());
}

void DmaBlockMetadata_Linear::FreeAtOffset(uint32_t offset) {
  SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();

  if (!suballocations1st.empty()) {
    // First allocation: Mark it as next empty at the beginning.
    DmaSuballocation &firstSuballoc =
        suballocations1st[m_1stNullItemsBeginCount];
    if (firstSuballoc.offset == offset) {
      firstSuballoc.type = DMA_SUBALLOCATION_TYPE_FREE;
      firstSuballoc.hAllocation = DMA_NULL_HANDLE;
      m_SumFreeSize += firstSuballoc.size;
      ++m_1stNullItemsBeginCount;
      CleanupAfterFree();
      return;
    }
  }

  // Last allocation in 2-part ring buffer or top of upper stack (same logic).
  if (m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER ||
      m_2ndVectorMode == SECOND_VECTOR_DOUBLE_STACK) {
    DmaSuballocation &lastSuballoc = suballocations2nd.back();
    if (lastSuballoc.offset == offset) {
      m_SumFreeSize += lastSuballoc.size;
      suballocations2nd.pop_back();
      CleanupAfterFree();
      return;
    }
  }
  // Last allocation in 1st vector.
  else if (m_2ndVectorMode == SECOND_VECTOR_EMPTY) {
    DmaSuballocation &lastSuballoc = suballocations1st.back();
    if (lastSuballoc.offset == offset) {
      m_SumFreeSize += lastSuballoc.size;
      suballocations1st.pop_back();
      CleanupAfterFree();
      return;
    }
  }

  // Item from the middle of 1st vector.
  {
    DmaSuballocation refSuballoc;
    refSuballoc.offset = offset;
    // Rest of members stays uninitialized intentionally for better performance.
    SuballocationVectorType::iterator it = DmaBinaryFindSorted(
        suballocations1st.begin() + m_1stNullItemsBeginCount,
        suballocations1st.end(), refSuballoc, DmaSuballocationOffsetLess());
    if (it != suballocations1st.end()) {
      it->type = DMA_SUBALLOCATION_TYPE_FREE;
      it->hAllocation = DMA_NULL_HANDLE;
      ++m_1stNullItemsMiddleCount;
      m_SumFreeSize += it->size;
      CleanupAfterFree();
      return;
    }
  }

  if (m_2ndVectorMode != SECOND_VECTOR_EMPTY) {
    // Item from the middle of 2nd vector.
    DmaSuballocation refSuballoc;
    refSuballoc.offset = offset;
    // Rest of members stays uninitialized intentionally for better performance.
    SuballocationVectorType::iterator it =
        m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER
            ? DmaBinaryFindSorted(suballocations2nd.begin(),
                                  suballocations2nd.end(), refSuballoc,
                                  DmaSuballocationOffsetLess())
            : DmaBinaryFindSorted(suballocations2nd.begin(),
                                  suballocations2nd.end(), refSuballoc,
                                  DmaSuballocationOffsetGreater());
    if (it != suballocations2nd.end()) {
      it->type = DMA_SUBALLOCATION_TYPE_FREE;
      it->hAllocation = DMA_NULL_HANDLE;
      ++m_2ndNullItemsCount;
      m_SumFreeSize += it->size;
      CleanupAfterFree();
      return;
    }
  }

  DMA_ASSERT(0 && "Allocation to free not found in linear allocator!");
}

bool DmaBlockMetadata_Linear::ShouldCompact1st() const {
  const size_t nullItemCount =
      m_1stNullItemsBeginCount + m_1stNullItemsMiddleCount;
  const size_t suballocCount = AccessSuballocations1st().size();
  return suballocCount > 32 &&
         nullItemCount * 2 >= (suballocCount - nullItemCount) * 3;
}

void DmaBlockMetadata_Linear::CleanupAfterFree() {
  SuballocationVectorType &suballocations1st = AccessSuballocations1st();
  SuballocationVectorType &suballocations2nd = AccessSuballocations2nd();

  if (IsEmpty()) {
    suballocations1st.clear();
    suballocations2nd.clear();
    m_1stNullItemsBeginCount = 0;
    m_1stNullItemsMiddleCount = 0;
    m_2ndNullItemsCount = 0;
    m_2ndVectorMode = SECOND_VECTOR_EMPTY;
  } else {
    const size_t suballoc1stCount = suballocations1st.size();
    const size_t nullItem1stCount =
        m_1stNullItemsBeginCount + m_1stNullItemsMiddleCount;
    DMA_ASSERT(nullItem1stCount <= suballoc1stCount);

    // Find more null items at the beginning of 1st vector.
    while (m_1stNullItemsBeginCount < suballoc1stCount &&
           suballocations1st[m_1stNullItemsBeginCount].hAllocation ==
               DMA_NULL_HANDLE) {
      ++m_1stNullItemsBeginCount;
      --m_1stNullItemsMiddleCount;
    }

    // Find more null items at the end of 1st vector.
    while (m_1stNullItemsMiddleCount > 0 &&
           suballocations1st.back().hAllocation == DMA_NULL_HANDLE) {
      --m_1stNullItemsMiddleCount;
      suballocations1st.pop_back();
    }

    // Find more null items at the end of 2nd vector.
    while (m_2ndNullItemsCount > 0 &&
           suballocations2nd.back().hAllocation == DMA_NULL_HANDLE) {
      --m_2ndNullItemsCount;
      suballocations2nd.pop_back();
    }

    // Find more null items at the beginning of 2nd vector.
    while (m_2ndNullItemsCount > 0 &&
           suballocations2nd[0].hAllocation == DMA_NULL_HANDLE) {
      --m_2ndNullItemsCount;
      DmaVectorRemove(suballocations2nd, 0);
    }

    if (ShouldCompact1st()) {
      const size_t nonNullItemCount = suballoc1stCount - nullItem1stCount;
      size_t srcIndex = m_1stNullItemsBeginCount;
      for (size_t dstIndex = 0; dstIndex < nonNullItemCount; ++dstIndex) {
        while (suballocations1st[srcIndex].hAllocation == DMA_NULL_HANDLE) {
          ++srcIndex;
        }
        if (dstIndex != srcIndex) {
          suballocations1st[dstIndex] = suballocations1st[srcIndex];
        }
        ++srcIndex;
      }
      suballocations1st.resize(nonNullItemCount);
      m_1stNullItemsBeginCount = 0;
      m_1stNullItemsMiddleCount = 0;
    }

    // 2nd vector became empty.
    if (suballocations2nd.empty()) {
      m_2ndVectorMode = SECOND_VECTOR_EMPTY;
    }

    // 1st vector became empty.
    if (suballocations1st.size() - m_1stNullItemsBeginCount == 0) {
      suballocations1st.clear();
      m_1stNullItemsBeginCount = 0;

      if (!suballocations2nd.empty() &&
          m_2ndVectorMode == SECOND_VECTOR_RING_BUFFER) {
        // Swap 1st with 2nd. Now 2nd is empty.
        m_2ndVectorMode = SECOND_VECTOR_EMPTY;
        m_1stNullItemsMiddleCount = m_2ndNullItemsCount;
        while (m_1stNullItemsBeginCount < suballocations2nd.size() &&
               suballocations2nd[m_1stNullItemsBeginCount].hAllocation ==
                   DMA_NULL_HANDLE) {
          ++m_1stNullItemsBeginCount;
          --m_1stNullItemsMiddleCount;
        }
        m_2ndNullItemsCount = 0;
        m_1stVectorIndex ^= 1;
      }
    }
  }

  DMA_HEAVY_ASSERT(Validate());
}

////////////////////////////////////////////////////////////////////////////////
// class DmaBlockMetadata_Buddy

DmaBlockMetadata_Buddy::DmaBlockMetadata_Buddy(DmaAllocator hAllocator)
    : DmaBlockMetadata(hAllocator), m_Root(DMA_NULL), m_AllocationCount(0),
      m_FreeCount(1), m_SumFreeSize(0) {
  memset(m_FreeList, 0, sizeof(m_FreeList));
}

DmaBlockMetadata_Buddy::~DmaBlockMetadata_Buddy() { DeleteNode(m_Root); }

void DmaBlockMetadata_Buddy::Init(uint32_t size) {
  DmaBlockMetadata::Init(size);

  m_UsableSize = DmaPrevPow2(size);
  m_SumFreeSize = m_UsableSize;

  // Calculate m_LevelCount.
  m_LevelCount = 1;
  while (m_LevelCount < MAX_LEVELS &&
         LevelToNodeSize(m_LevelCount) >= MIN_NODE_SIZE) {
    ++m_LevelCount;
  }

  Node *rootNode = dma_new(GetAllocationCallbacks(), Node)();
  rootNode->offset = 0;
  rootNode->type = Node::TYPE_FREE;
  rootNode->parent = DMA_NULL;
  rootNode->buddy = DMA_NULL;

  m_Root = rootNode;
  AddToFreeListFront(0, rootNode);
}

bool DmaBlockMetadata_Buddy::Validate() const {
  // Validate tree.
  ValidationContext ctx;
  if (!ValidateNode(ctx, DMA_NULL, m_Root, 0, LevelToNodeSize(0))) {
    DMA_VALIDATE(false && "ValidateNode failed.");
  }
  DMA_VALIDATE(m_AllocationCount == ctx.calculatedAllocationCount);
  DMA_VALIDATE(m_SumFreeSize == ctx.calculatedSumFreeSize);

  // Validate free node lists.
  for (uint32_t level = 0; level < m_LevelCount; ++level) {
    DMA_VALIDATE(m_FreeList[level].front == DMA_NULL ||
                 m_FreeList[level].front->free.prev == DMA_NULL);

    for (Node *node = m_FreeList[level].front; node != DMA_NULL;
         node = node->free.next) {
      DMA_VALIDATE(node->type == Node::TYPE_FREE);

      if (node->free.next == DMA_NULL) {
        DMA_VALIDATE(m_FreeList[level].back == node);
      } else {
        DMA_VALIDATE(node->free.next->free.prev == node);
      }
    }
  }

  // Validate that free lists ar higher levels are empty.
  for (uint32_t level = m_LevelCount; level < MAX_LEVELS; ++level) {
    DMA_VALIDATE(m_FreeList[level].front == DMA_NULL &&
                 m_FreeList[level].back == DMA_NULL);
  }

  return true;
}

uint32_t DmaBlockMetadata_Buddy::GetUnusedRangeSizeMax() const {
  for (uint32_t level = 0; level < m_LevelCount; ++level) {
    if (m_FreeList[level].front != DMA_NULL) {
      return LevelToNodeSize(level);
    }
  }
  return 0;
}

void DmaBlockMetadata_Buddy::CalcAllocationStatInfo(
    DmaStatInfo &outInfo) const {
  const uint32_t unusableSize = GetUnusableSize();

  outInfo.blockCount = 1;

  outInfo.allocationCount = outInfo.unusedRangeCount = 0;
  outInfo.usedBytes = outInfo.unusedBytes = 0;

  outInfo.allocationSizeMax = outInfo.unusedRangeSizeMax = 0;
  outInfo.allocationSizeMin = outInfo.unusedRangeSizeMin = UINT32_MAX;
  outInfo.allocationSizeAvg = outInfo.unusedRangeSizeAvg = 0; // Unused.

  CalcAllocationStatInfoNode(outInfo, m_Root, LevelToNodeSize(0));

  if (unusableSize > 0) {
    ++outInfo.unusedRangeCount;
    outInfo.unusedBytes += unusableSize;
    outInfo.unusedRangeSizeMax =
        DMA_MAX(outInfo.unusedRangeSizeMax, unusableSize);
    outInfo.unusedRangeSizeMin =
        DMA_MIN(outInfo.unusedRangeSizeMin, unusableSize);
  }
}

void DmaBlockMetadata_Buddy::AddPoolStats(DmaPoolStats &inoutStats) const {
  const uint32_t unusableSize = GetUnusableSize();

  inoutStats.size += GetSize();
  inoutStats.unusedSize += m_SumFreeSize + unusableSize;
  inoutStats.allocationCount += m_AllocationCount;
  inoutStats.unusedRangeCount += m_FreeCount;
  inoutStats.unusedRangeSizeMax =
      DMA_MAX(inoutStats.unusedRangeSizeMax, GetUnusedRangeSizeMax());

  if (unusableSize > 0) {
    ++inoutStats.unusedRangeCount;
    // Not updating inoutStats.unusedRangeSizeMax with unusableSize because this
    // space is not available for allocations.
  }
}

#if DMA_STATS_STRING_ENABLED

void DmaBlockMetadata_Buddy::PrintDetailedMap(class DmaJsonWriter &json) const {
  // TODO optimize
  DmaStatInfo stat;
  CalcAllocationStatInfo(stat);

  PrintDetailedMap_Begin(json, stat.unusedBytes, stat.allocationCount,
                         stat.unusedRangeCount);

  PrintDetailedMapNode(json, m_Root, LevelToNodeSize(0));

  const uint32_t unusableSize = GetUnusableSize();
  if (unusableSize > 0) {
    PrintDetailedMap_UnusedRange(json,
                                 m_UsableSize,  // offset
                                 unusableSize); // size
  }

  PrintDetailedMap_End(json);
}

#endif // #if DMA_STATS_STRING_ENABLED

bool DmaBlockMetadata_Buddy::CreateAllocationRequest(
    uint32_t currentFrameIndex, uint32_t frameInUseCount, uint32_t allocSize,
    uint32_t allocAlignment, bool upperAddress, DmaSuballocationType allocType,
    bool canMakeOtherLost, uint32_t strategy,
    DmaAllocationRequest *pAllocationRequest) {
  DMA_ASSERT(!upperAddress && "DMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT can be "
                              "used only with linear algorithm.");

  if (allocSize > m_UsableSize) {
    return false;
  }

  const uint32_t targetLevel = AllocSizeToLevel(allocSize);
  for (uint32_t level = targetLevel + 1; level--;) {
    for (Node *freeNode = m_FreeList[level].front; freeNode != DMA_NULL;
         freeNode = freeNode->free.next) {
      if (freeNode->offset % allocAlignment == 0) {
        pAllocationRequest->type = DmaAllocationRequestType::Normal;
        pAllocationRequest->offset = freeNode->offset;
        pAllocationRequest->sumFreeSize = LevelToNodeSize(level);
        pAllocationRequest->sumItemSize = 0;
        pAllocationRequest->itemsToMakeLostCount = 0;
        pAllocationRequest->customData = (void *)(uintptr_t)level;
        return true;
      }
    }
  }

  return false;
}

bool DmaBlockMetadata_Buddy::MakeRequestedAllocationsLost(
    uint32_t currentFrameIndex, uint32_t frameInUseCount,
    DmaAllocationRequest *pAllocationRequest) {
  /*
  Lost allocations are not supported in buddy allocator at the moment.
  Support might be added in the future.
  */
  return pAllocationRequest->itemsToMakeLostCount == 0;
}

uint32_t DmaBlockMetadata_Buddy::MakeAllocationsLost(uint32_t currentFrameIndex,
                                                     uint32_t frameInUseCount) {
  /*
  Lost allocations are not supported in buddy allocator at the moment.
  Support might be added in the future.
  */
  return 0;
}

void DmaBlockMetadata_Buddy::Alloc(const DmaAllocationRequest &request,
                                   DmaSuballocationType type,
                                   uint32_t allocSize,
                                   DmaAllocation hAllocation) {
  DMA_ASSERT(request.type == DmaAllocationRequestType::Normal);

  const uint32_t targetLevel = AllocSizeToLevel(allocSize);
  uint32_t currLevel = (uint32_t)(uintptr_t)request.customData;

  Node *currNode = m_FreeList[currLevel].front;
  DMA_ASSERT(currNode != DMA_NULL && currNode->type == Node::TYPE_FREE);
  while (currNode->offset != request.offset) {
    currNode = currNode->free.next;
    DMA_ASSERT(currNode != DMA_NULL && currNode->type == Node::TYPE_FREE);
  }

  // Go down, splitting free nodes.
  while (currLevel < targetLevel) {
    // currNode is already first free node at currLevel.
    // Remove it from list of free nodes at this currLevel.
    RemoveFromFreeList(currLevel, currNode);

    const uint32_t childrenLevel = currLevel + 1;

    // Create two free sub-nodes.
    Node *leftChild = dma_new(GetAllocationCallbacks(), Node)();
    Node *rightChild = dma_new(GetAllocationCallbacks(), Node)();

    leftChild->offset = currNode->offset;
    leftChild->type = Node::TYPE_FREE;
    leftChild->parent = currNode;
    leftChild->buddy = rightChild;

    rightChild->offset = currNode->offset + LevelToNodeSize(childrenLevel);
    rightChild->type = Node::TYPE_FREE;
    rightChild->parent = currNode;
    rightChild->buddy = leftChild;

    // Convert current currNode to split type.
    currNode->type = Node::TYPE_SPLIT;
    currNode->split.leftChild = leftChild;

    // Add child nodes to free list. Order is important!
    AddToFreeListFront(childrenLevel, rightChild);
    AddToFreeListFront(childrenLevel, leftChild);

    ++m_FreeCount;
    // m_SumFreeSize -= LevelToNodeSize(currLevel) % 2; // Useful only when
    // level node sizes can be non power of 2.
    ++currLevel;
    currNode = m_FreeList[currLevel].front;

    /*
    We can be sure that currNode, as left child of node previously split,
    also fullfills the alignment requirement.
    */
  }

  // Remove from free list.
  DMA_ASSERT(currLevel == targetLevel && currNode != DMA_NULL &&
             currNode->type == Node::TYPE_FREE);
  RemoveFromFreeList(currLevel, currNode);

  // Convert to allocation node.
  currNode->type = Node::TYPE_ALLOCATION;
  currNode->allocation.alloc = hAllocation;

  ++m_AllocationCount;
  --m_FreeCount;
  m_SumFreeSize -= allocSize;
}

void DmaBlockMetadata_Buddy::DeleteNode(Node *node) {
  if (node->type == Node::TYPE_SPLIT) {
    DeleteNode(node->split.leftChild->buddy);
    DeleteNode(node->split.leftChild);
  }

  dma_delete(GetAllocationCallbacks(), node);
}

bool DmaBlockMetadata_Buddy::ValidateNode(ValidationContext &ctx,
                                          const Node *parent, const Node *curr,
                                          uint32_t level,
                                          uint32_t levelNodeSize) const {
  DMA_VALIDATE(level < m_LevelCount);
  DMA_VALIDATE(curr->parent == parent);
  DMA_VALIDATE((curr->buddy == DMA_NULL) == (parent == DMA_NULL));
  DMA_VALIDATE(curr->buddy == DMA_NULL || curr->buddy->buddy == curr);
  switch (curr->type) {
  case Node::TYPE_FREE:
    // curr->free.prev, next are validated separately.
    ctx.calculatedSumFreeSize += levelNodeSize;
    ++ctx.calculatedFreeCount;
    break;
  case Node::TYPE_ALLOCATION:
    ++ctx.calculatedAllocationCount;
    ctx.calculatedSumFreeSize +=
        levelNodeSize - curr->allocation.alloc->GetSize();
    DMA_VALIDATE(curr->allocation.alloc != DMA_NULL_HANDLE);
    break;
  case Node::TYPE_SPLIT: {
    const uint32_t childrenLevel = level + 1;
    const uint32_t childrenLevelNodeSize = levelNodeSize / 2;
    const Node *const leftChild = curr->split.leftChild;
    DMA_VALIDATE(leftChild != DMA_NULL);
    DMA_VALIDATE(leftChild->offset == curr->offset);
    if (!ValidateNode(ctx, curr, leftChild, childrenLevel,
                      childrenLevelNodeSize)) {
      DMA_VALIDATE(false && "ValidateNode for left child failed.");
    }
    const Node *const rightChild = leftChild->buddy;
    DMA_VALIDATE(rightChild->offset == curr->offset + childrenLevelNodeSize);
    if (!ValidateNode(ctx, curr, rightChild, childrenLevel,
                      childrenLevelNodeSize)) {
      DMA_VALIDATE(false && "ValidateNode for right child failed.");
    }
  } break;
  default:
    return false;
  }

  return true;
}

uint32_t DmaBlockMetadata_Buddy::AllocSizeToLevel(uint32_t allocSize) const {
  // I know this could be optimized somehow e.g. by using std::log2p1 from
  // C++20.
  uint32_t level = 0;
  uint32_t currLevelNodeSize = m_UsableSize;
  uint32_t nextLevelNodeSize = currLevelNodeSize >> 1;
  while (allocSize <= nextLevelNodeSize && level + 1 < m_LevelCount) {
    ++level;
    currLevelNodeSize = nextLevelNodeSize;
    nextLevelNodeSize = currLevelNodeSize >> 1;
  }
  return level;
}

void DmaBlockMetadata_Buddy::FreeAtOffset(const DmaAllocation alloc,
                                          uint32_t offset) {
  // Find node and level.
  Node *node = m_Root;
  uint32_t nodeOffset = 0;
  uint32_t level = 0;
  uint32_t levelNodeSize = LevelToNodeSize(0);
  while (node->type == Node::TYPE_SPLIT) {
    const uint32_t nextLevelSize = levelNodeSize >> 1;
    if (offset < nodeOffset + nextLevelSize) {
      node = node->split.leftChild;
    } else {
      node = node->split.leftChild->buddy;
      nodeOffset += nextLevelSize;
    }
    ++level;
    levelNodeSize = nextLevelSize;
  }

  DMA_ASSERT(node != DMA_NULL && node->type == Node::TYPE_ALLOCATION);
  DMA_ASSERT(alloc == DMA_NULL_HANDLE || node->allocation.alloc == alloc);

  ++m_FreeCount;
  --m_AllocationCount;
  m_SumFreeSize += alloc->GetSize();

  node->type = Node::TYPE_FREE;

  // Join free nodes if possible.
  while (level > 0 && node->buddy->type == Node::TYPE_FREE) {
    RemoveFromFreeList(level, node->buddy);
    Node *const parent = node->parent;

    dma_delete(GetAllocationCallbacks(), node->buddy);
    dma_delete(GetAllocationCallbacks(), node);
    parent->type = Node::TYPE_FREE;

    node = parent;
    --level;
    // m_SumFreeSize += LevelToNodeSize(level) % 2; // Useful only when level
    // node sizes can be non power of 2.
    --m_FreeCount;
  }

  AddToFreeListFront(level, node);
}

void DmaBlockMetadata_Buddy::CalcAllocationStatInfoNode(
    DmaStatInfo &outInfo, const Node *node, uint32_t levelNodeSize) const {
  switch (node->type) {
  case Node::TYPE_FREE:
    ++outInfo.unusedRangeCount;
    outInfo.unusedBytes += levelNodeSize;
    outInfo.unusedRangeSizeMax =
        DMA_MAX(outInfo.unusedRangeSizeMax, levelNodeSize);
    outInfo.unusedRangeSizeMin =
        DMA_MAX(outInfo.unusedRangeSizeMin, levelNodeSize);
    break;
  case Node::TYPE_ALLOCATION: {
    const uint32_t allocSize = node->allocation.alloc->GetSize();
    ++outInfo.allocationCount;
    outInfo.usedBytes += allocSize;
    outInfo.allocationSizeMax = DMA_MAX(outInfo.allocationSizeMax, allocSize);
    outInfo.allocationSizeMin = DMA_MAX(outInfo.allocationSizeMin, allocSize);

    const uint32_t unusedRangeSize = levelNodeSize - allocSize;
    if (unusedRangeSize > 0) {
      ++outInfo.unusedRangeCount;
      outInfo.unusedBytes += unusedRangeSize;
      outInfo.unusedRangeSizeMax =
          DMA_MAX(outInfo.unusedRangeSizeMax, unusedRangeSize);
      outInfo.unusedRangeSizeMin =
          DMA_MAX(outInfo.unusedRangeSizeMin, unusedRangeSize);
    }
  } break;
  case Node::TYPE_SPLIT: {
    const uint32_t childrenNodeSize = levelNodeSize / 2;
    const Node *const leftChild = node->split.leftChild;
    CalcAllocationStatInfoNode(outInfo, leftChild, childrenNodeSize);
    const Node *const rightChild = leftChild->buddy;
    CalcAllocationStatInfoNode(outInfo, rightChild, childrenNodeSize);
  } break;
  default:
    DMA_ASSERT(0);
  }
}

void DmaBlockMetadata_Buddy::AddToFreeListFront(uint32_t level, Node *node) {
  DMA_ASSERT(node->type == Node::TYPE_FREE);

  // List is empty.
  Node *const frontNode = m_FreeList[level].front;
  if (frontNode == DMA_NULL) {
    DMA_ASSERT(m_FreeList[level].back == DMA_NULL);
    node->free.prev = node->free.next = DMA_NULL;
    m_FreeList[level].front = m_FreeList[level].back = node;
  } else {
    DMA_ASSERT(frontNode->free.prev == DMA_NULL);
    node->free.prev = DMA_NULL;
    node->free.next = frontNode;
    frontNode->free.prev = node;
    m_FreeList[level].front = node;
  }
}

void DmaBlockMetadata_Buddy::RemoveFromFreeList(uint32_t level, Node *node) {
  DMA_ASSERT(m_FreeList[level].front != DMA_NULL);

  // It is at the front.
  if (node->free.prev == DMA_NULL) {
    DMA_ASSERT(m_FreeList[level].front == node);
    m_FreeList[level].front = node->free.next;
  } else {
    Node *const prevFreeNode = node->free.prev;
    DMA_ASSERT(prevFreeNode->free.next == node);
    prevFreeNode->free.next = node->free.next;
  }

  // It is at the back.
  if (node->free.next == DMA_NULL) {
    DMA_ASSERT(m_FreeList[level].back == node);
    m_FreeList[level].back = node->free.prev;
  } else {
    Node *const nextFreeNode = node->free.next;
    DMA_ASSERT(nextFreeNode->free.prev == node);
    nextFreeNode->free.prev = node->free.prev;
  }
}

#if DMA_STATS_STRING_ENABLED
void DmaBlockMetadata_Buddy::PrintDetailedMapNode(
    class DmaJsonWriter &json, const Node *node, uint32_t levelNodeSize) const {
  switch (node->type) {
  case Node::TYPE_FREE:
    PrintDetailedMap_UnusedRange(json, node->offset, levelNodeSize);
    break;
  case Node::TYPE_ALLOCATION: {
    PrintDetailedMap_Allocation(json, node->offset, node->allocation.alloc);
    const uint32_t allocSize = node->allocation.alloc->GetSize();
    if (allocSize < levelNodeSize) {
      PrintDetailedMap_UnusedRange(json, node->offset + allocSize,
                                   levelNodeSize - allocSize);
    }
  } break;
  case Node::TYPE_SPLIT: {
    const uint32_t childrenNodeSize = levelNodeSize / 2;
    const Node *const leftChild = node->split.leftChild;
    PrintDetailedMapNode(json, leftChild, childrenNodeSize);
    const Node *const rightChild = leftChild->buddy;
    PrintDetailedMapNode(json, rightChild, childrenNodeSize);
  } break;
  default:
    DMA_ASSERT(0);
  }
}
#endif // #if DMA_STATS_STRING_ENABLED

////////////////////////////////////////////////////////////////////////////////
// class DmaDeviceMemoryBlock

DmaDeviceMemoryBlock::DmaDeviceMemoryBlock(DmaAllocator hAllocator)
    : m_pMetadata(DMA_NULL), m_MemoryTypeIndex(UINT32_MAX), m_Id(0),
      m_hMemory(DMA_NULL_HANDLE), m_MapCount(0), m_pMappedData(DMA_NULL) {}

void DmaDeviceMemoryBlock::Init(DmaAllocator hAllocator, DmaPool hParentPool,
                                uint32_t newMemoryTypeIndex,
                                DkMemBlock newMemory, uint32_t newSize,
                                uint32_t id, uint32_t algorithm) {
  DMA_ASSERT(m_hMemory == DMA_NULL_HANDLE);

  m_hParentPool = hParentPool;
  m_MemoryTypeIndex = newMemoryTypeIndex;
  m_Id = id;
  m_hMemory = newMemory;

  switch (algorithm) {
  case DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT:
    m_pMetadata = dma_new(hAllocator, DmaBlockMetadata_Linear)(hAllocator);
    break;
  case DMA_POOL_CREATE_BUDDY_ALGORITHM_BIT:
    m_pMetadata = dma_new(hAllocator, DmaBlockMetadata_Buddy)(hAllocator);
    break;
  default:
    DMA_ASSERT(0);
    // Fall-through.
  case 0:
    m_pMetadata = dma_new(hAllocator, DmaBlockMetadata_Generic)(hAllocator);
  }
  m_pMetadata->Init(newSize);
}

void DmaDeviceMemoryBlock::Destroy(DmaAllocator allocator) {
  // This is the most important assert in the entire library.
  // Hitting it means you have some memory leak - unreleased DmaAllocation
  // objects.
  DMA_ASSERT(m_pMetadata->IsEmpty() && "Some allocations were not freed before "
                                       "destruction of this memory block!");

  DMA_ASSERT(m_hMemory != DMA_NULL_HANDLE);
  allocator->FreeVulkanMemory(m_MemoryTypeIndex, m_pMetadata->GetSize(),
                              m_hMemory);
  m_hMemory = DMA_NULL_HANDLE;

  dma_delete(allocator, m_pMetadata);
  m_pMetadata = DMA_NULL;
}

bool DmaDeviceMemoryBlock::Validate() const {
  DMA_VALIDATE((m_hMemory != DMA_NULL_HANDLE) && (m_pMetadata->GetSize() != 0));

  return m_pMetadata->Validate();
}

DkResult DmaDeviceMemoryBlock::CheckCorruption(DmaAllocator hAllocator) {
  void *pData = nullptr;
  DkResult res = Map(hAllocator, 1, &pData);
  if (res != DkResult_Success) {
    return res;
  }

  res = m_pMetadata->CheckCorruption(pData);

  Unmap(hAllocator, 1);

  return res;
}

DkResult DmaDeviceMemoryBlock::Map(DmaAllocator hAllocator, uint32_t count,
                                   void **ppData) {
  if (count == 0) {
    return DkResult_Success;
  }

  DmaMutexLock lock(m_Mutex, hAllocator->m_UseMutex);
  if (m_MapCount != 0) {
    m_MapCount += count;
    DMA_ASSERT(m_pMappedData != DMA_NULL);
    if (ppData != DMA_NULL) {
      *ppData = m_pMappedData;
    }
    return DkResult_Success;
  } else {
    m_pMappedData = dkMemBlockGetCpuAddr(m_hMemory);
    if (ppData != DMA_NULL) {
      *ppData = m_pMappedData;
    }
    m_MapCount = count;
    return DkResult_Success;
  }
}

void DmaDeviceMemoryBlock::Unmap(DmaAllocator hAllocator, uint32_t count) {
  if (count == 0) {
    return;
  }

  DmaMutexLock lock(m_Mutex, hAllocator->m_UseMutex);
  if (m_MapCount >= count) {
    m_MapCount -= count;
    if (m_MapCount == 0)
      m_pMappedData = DMA_NULL;
  } else {
    DMA_ASSERT(0 && "DkMemBlock block is being unmapped while it was not "
                    "previously mapped.");
  }
}

DkResult DmaDeviceMemoryBlock::WriteMagicValueAroundAllocation(
    DmaAllocator hAllocator, uint32_t allocOffset, uint32_t allocSize) {
  DMA_ASSERT(DMA_DEBUG_MARGIN > 0 && DMA_DEBUG_MARGIN % 4 == 0 &&
             DMA_DEBUG_DETECT_CORRUPTION);
  DMA_ASSERT(allocOffset >= DMA_DEBUG_MARGIN);

  void *pData;
  DkResult res = Map(hAllocator, 1, &pData);
  if (res != DkResult_Success) {
    return res;
  }

  DmaWriteMagicValue(pData, allocOffset - DMA_DEBUG_MARGIN);
  DmaWriteMagicValue(pData, allocOffset + allocSize);

  Unmap(hAllocator, 1);

  return DkResult_Success;
}

DkResult DmaDeviceMemoryBlock::ValidateMagicValueAroundAllocation(
    DmaAllocator hAllocator, uint32_t allocOffset, uint32_t allocSize) {
  DMA_ASSERT(DMA_DEBUG_MARGIN > 0 && DMA_DEBUG_MARGIN % 4 == 0 &&
             DMA_DEBUG_DETECT_CORRUPTION);
  DMA_ASSERT(allocOffset >= DMA_DEBUG_MARGIN);

  void *pData;
  DkResult res = Map(hAllocator, 1, &pData);
  if (res != DkResult_Success) {
    return res;
  }

  if (!DmaValidateMagicValue(pData, allocOffset - DMA_DEBUG_MARGIN)) {
    DMA_ASSERT(0 && "MEMORY CORRUPTION DETECTED BEFORE FREED ALLOCATION!");
  } else if (!DmaValidateMagicValue(pData, allocOffset + allocSize)) {
    DMA_ASSERT(0 && "MEMORY CORRUPTION DETECTED AFTER FREED ALLOCATION!");
  }

  Unmap(hAllocator, 1);

  return DkResult_Success;
}

static void InitStatInfo(DmaStatInfo &outInfo) {
  memset(&outInfo, 0, sizeof(outInfo));
  outInfo.allocationSizeMin = UINT32_MAX;
  outInfo.unusedRangeSizeMin = UINT32_MAX;
}

// Adds statistics srcInfo into inoutInfo, like: inoutInfo += srcInfo.
static void DmaAddStatInfo(DmaStatInfo &inoutInfo, const DmaStatInfo &srcInfo) {
  inoutInfo.blockCount += srcInfo.blockCount;
  inoutInfo.allocationCount += srcInfo.allocationCount;
  inoutInfo.unusedRangeCount += srcInfo.unusedRangeCount;
  inoutInfo.usedBytes += srcInfo.usedBytes;
  inoutInfo.unusedBytes += srcInfo.unusedBytes;
  inoutInfo.allocationSizeMin =
      DMA_MIN(inoutInfo.allocationSizeMin, srcInfo.allocationSizeMin);
  inoutInfo.allocationSizeMax =
      DMA_MAX(inoutInfo.allocationSizeMax, srcInfo.allocationSizeMax);
  inoutInfo.unusedRangeSizeMin =
      DMA_MIN(inoutInfo.unusedRangeSizeMin, srcInfo.unusedRangeSizeMin);
  inoutInfo.unusedRangeSizeMax =
      DMA_MAX(inoutInfo.unusedRangeSizeMax, srcInfo.unusedRangeSizeMax);
}

static void DmaPostprocessCalcStatInfo(DmaStatInfo &inoutInfo) {
  inoutInfo.allocationSizeAvg =
      (inoutInfo.allocationCount > 0)
          ? DmaRoundDiv<uint32_t>(inoutInfo.usedBytes,
                                  inoutInfo.allocationCount)
          : 0;
  inoutInfo.unusedRangeSizeAvg =
      (inoutInfo.unusedRangeCount > 0)
          ? DmaRoundDiv<uint32_t>(inoutInfo.unusedBytes,
                                  inoutInfo.unusedRangeCount)
          : 0;
}

DmaPool_T::DmaPool_T(DmaAllocator hAllocator,
                     const DmaPoolCreateInfo &createInfo,
                     uint32_t preferredBlockSize)
    : m_BlockVector(
          hAllocator,
          this, // hParentPool
          createInfo.memoryTypeIndex,
          createInfo.blockSize != 0 ? createInfo.blockSize : preferredBlockSize,
          createInfo.minBlockCount, createInfo.maxBlockCount,
          createInfo.frameInUseCount,
          createInfo.flags & DMA_POOL_CREATE_ALGORITHM_MASK), // algorithm
      m_Id(0), m_Name(DMA_NULL) {}

DmaPool_T::~DmaPool_T() {}

void DmaPool_T::SetName(const char *pName) {
  const DmaAllocationCallbacks *allocs =
      m_BlockVector.GetAllocator()->GetAllocationCallbacks();
  DmaFreeString(allocs, m_Name);

  if (pName != DMA_NULL) {
    m_Name = DmaCreateStringCopy(allocs, pName);
  } else {
    m_Name = DMA_NULL;
  }
}

#if DMA_STATS_STRING_ENABLED

#endif // #if DMA_STATS_STRING_ENABLED

DmaBlockVector::DmaBlockVector(DmaAllocator hAllocator, DmaPool hParentPool,
                               uint32_t memoryTypeIndex,
                               uint32_t preferredBlockSize,
                               size_t minBlockCount, size_t maxBlockCount,
                               uint32_t frameInUseCount, uint32_t algorithm)
    : m_hAllocator(hAllocator), m_hParentPool(hParentPool),
      m_MemoryTypeIndex(memoryTypeIndex),
      m_PreferredBlockSize(preferredBlockSize), m_MinBlockCount(minBlockCount),
      m_MaxBlockCount(maxBlockCount), m_FrameInUseCount(frameInUseCount),
      m_Algorithm(algorithm), m_HasEmptyBlock(false),
      m_Blocks(DmaStlAllocator<DmaDeviceMemoryBlock *>(
          hAllocator->GetAllocationCallbacks())),
      m_NextBlockId(0) {}

DmaBlockVector::~DmaBlockVector() {
  for (size_t i = m_Blocks.size(); i--;) {
    m_Blocks[i]->Destroy(m_hAllocator);
    dma_delete(m_hAllocator, m_Blocks[i]);
  }
}

DkResult DmaBlockVector::CreateMinBlocks() {
  for (size_t i = 0; i < m_MinBlockCount; ++i) {
    DkResult res = CreateBlock(m_PreferredBlockSize, DMA_NULL);
    if (res != DkResult_Success) {
      return res;
    }
  }
  return DkResult_Success;
}

void DmaBlockVector::GetPoolStats(DmaPoolStats *pStats) {
  DmaMutexLockRead lock(m_Mutex, m_hAllocator->m_UseMutex);

  const size_t blockCount = m_Blocks.size();

  pStats->size = 0;
  pStats->unusedSize = 0;
  pStats->allocationCount = 0;
  pStats->unusedRangeCount = 0;
  pStats->unusedRangeSizeMax = 0;
  pStats->blockCount = blockCount;

  for (uint32_t blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
    const DmaDeviceMemoryBlock *const pBlock = m_Blocks[blockIndex];
    DMA_ASSERT(pBlock);
    DMA_HEAVY_ASSERT(pBlock->Validate());
    pBlock->m_pMetadata->AddPoolStats(*pStats);
  }
}

bool DmaBlockVector::IsEmpty() {
  DmaMutexLockRead lock(m_Mutex, m_hAllocator->m_UseMutex);
  return m_Blocks.empty();
}

bool DmaBlockVector::IsCorruptionDetectionEnabled() const {
  return (DMA_DEBUG_DETECT_CORRUPTION != 0) && (DMA_DEBUG_MARGIN > 0) &&
         (m_Algorithm == 0 ||
          m_Algorithm == DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT);
}

static const uint32_t DMA_ALLOCATION_TRY_COUNT = 32;

DkResult DmaBlockVector::Allocate(uint32_t currentFrameIndex, uint32_t size,
                                  uint32_t alignment,
                                  const DmaAllocationCreateInfo &createInfo,
                                  DmaSuballocationType suballocType,
                                  size_t allocationCount,
                                  DmaAllocation *pAllocations) {
  size_t allocIndex;
  DkResult res = DkResult_Success;

  if (IsCorruptionDetectionEnabled()) {
    size = DmaAlignUp<uint32_t>(size,
                                sizeof(DMA_CORRUPTION_DETECTION_MAGIC_VALUE));
    alignment = DmaAlignUp<uint32_t>(
        alignment, sizeof(DMA_CORRUPTION_DETECTION_MAGIC_VALUE));
  }

  {
    DmaMutexLockWrite lock(m_Mutex, m_hAllocator->m_UseMutex);
    for (allocIndex = 0; allocIndex < allocationCount; ++allocIndex) {
      res = AllocatePage(currentFrameIndex, size, alignment, createInfo,
                         suballocType, pAllocations + allocIndex);
      if (res != DkResult_Success) {
        break;
      }
    }
  }

  if (res != DkResult_Success) {
    // Free all already created allocations.
    while (allocIndex--) {
      Free(pAllocations[allocIndex]);
    }
    memset(pAllocations, 0, sizeof(DmaAllocation) * allocationCount);
  }

  return res;
}

DkResult DmaBlockVector::AllocatePage(uint32_t currentFrameIndex, uint32_t size,
                                      uint32_t alignment,
                                      const DmaAllocationCreateInfo &createInfo,
                                      DmaSuballocationType suballocType,
                                      DmaAllocation *pAllocation) {
  const bool isUpperAddress =
      (createInfo.flags & DMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT) != 0;
  bool canMakeOtherLost =
      (createInfo.flags & DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT) != 0;
  const bool mapped =
      (createInfo.flags & DMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;
  const bool isUserDataString =
      (createInfo.flags & DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT) != 0;

  const bool withinBudget =
      (createInfo.flags & DMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT) != 0;

  const bool canCreateNewBlock =
      ((createInfo.flags & DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT) == 0) &&
      (m_Blocks.size() < m_MaxBlockCount);
  uint32_t strategy = createInfo.flags & DMA_ALLOCATION_CREATE_STRATEGY_MASK;

  // If linearAlgorithm is used, canMakeOtherLost is available only when used as
  // ring buffer. Which in turn is available only when maxBlockCount = 1.
  if (m_Algorithm == DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT &&
      m_MaxBlockCount > 1) {
    canMakeOtherLost = false;
  }

  // Upper address can only be used with linear allocator and within single
  // memory block.
  if (isUpperAddress && (m_Algorithm != DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT ||
                         m_MaxBlockCount > 1)) {
    return DkResult_NotImplemented;
  }

  // Validate strategy.
  switch (strategy) {
  case 0:
    strategy = DMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
    break;
  case DMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT:
  case DMA_ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT:
  case DMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT:
    break;
  default:
    return DkResult_NotImplemented;
  }

  // Early reject: requested allocation size is larger that maximum block size
  // for this block vector.
  if (size + 2 * DMA_DEBUG_MARGIN > m_PreferredBlockSize) {
    return DkResult_OutOfMemory;
  }

  /*
  Under certain condition, this whole section can be skipped for optimization,
  so we move on directly to trying to allocate with canMakeOtherLost. That's the
  case e.g. for custom pools with linear algorithm.
  */
  if (!canMakeOtherLost || canCreateNewBlock) {
    // 1. Search existing allocations. Try to allocate without making other
    // allocations lost.
    DmaAllocationCreateFlags allocFlagsCopy = createInfo.flags;
    allocFlagsCopy &= ~DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT;

    if (m_Algorithm == DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT) {
      // Use only last block.
      if (!m_Blocks.empty()) {
        DmaDeviceMemoryBlock *const pCurrBlock = m_Blocks.back();
        DMA_ASSERT(pCurrBlock);
        DkResult res = AllocateFromBlock(
            pCurrBlock, currentFrameIndex, size, alignment, allocFlagsCopy,
            createInfo.pUserData, suballocType, strategy, pAllocation);
        if (res == DkResult_Success) {
          DMA_DEBUG_LOG("    Returned from last block #%u",
                        pCurrBlock->GetId());
          return DkResult_Success;
        }
      }
    } else {
      if (strategy == DMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT) {
        // Forward order in m_Blocks - prefer blocks with smallest amount of
        // free space.
        for (size_t blockIndex = 0; blockIndex < m_Blocks.size();
             ++blockIndex) {
          DmaDeviceMemoryBlock *const pCurrBlock = m_Blocks[blockIndex];
          DMA_ASSERT(pCurrBlock);
          DkResult res = AllocateFromBlock(
              pCurrBlock, currentFrameIndex, size, alignment, allocFlagsCopy,
              createInfo.pUserData, suballocType, strategy, pAllocation);
          if (res == DkResult_Success) {
            DMA_DEBUG_LOG("    Returned from existing block #%u",
                          pCurrBlock->GetId());
            return DkResult_Success;
          }
        }
      } else // WORST_FIT, FIRST_FIT
      {
        // Backward order in m_Blocks - prefer blocks with largest amount of
        // free space.
        for (size_t blockIndex = m_Blocks.size(); blockIndex--;) {
          DmaDeviceMemoryBlock *const pCurrBlock = m_Blocks[blockIndex];
          DMA_ASSERT(pCurrBlock);
          DkResult res = AllocateFromBlock(
              pCurrBlock, currentFrameIndex, size, alignment, allocFlagsCopy,
              createInfo.pUserData, suballocType, strategy, pAllocation);
          if (res == DkResult_Success) {
            DMA_DEBUG_LOG("    Returned from existing block #%u",
                          pCurrBlock->GetId());
            return DkResult_Success;
          }
        }
      }
    }

    // 2. Try to create new block.
    if (canCreateNewBlock) {
      // Calculate optimal size for new block.
      uint32_t newBlockSize = m_PreferredBlockSize;
      uint32_t newBlockSizeShift = 0;
      const uint32_t NEW_BLOCK_SIZE_SHIFT_MAX = 3;

      size_t newBlockIndex = 0;
      DkResult res = CreateBlock(newBlockSize, &newBlockIndex);

      if (res == DkResult_Success) {
        DmaDeviceMemoryBlock *const pBlock = m_Blocks[newBlockIndex];
        DMA_ASSERT(pBlock->m_pMetadata->GetSize() >= size);

        res = AllocateFromBlock(pBlock, currentFrameIndex, size, alignment,
                                allocFlagsCopy, createInfo.pUserData,
                                suballocType, strategy, pAllocation);
        if (res == DkResult_Success) {
          DMA_DEBUG_LOG("    Created new block #%u Size=%llu", pBlock->GetId(),
                        newBlockSize);
          return DkResult_Success;
        } else {
          // Allocation from new block failed, possibly due to DMA_DEBUG_MARGIN
          // or alignment.
          return DkResult_OutOfMemory;
        }
      }
    }
  }

  // 3. Try to allocate from existing blocks with making other allocations lost.
  if (canMakeOtherLost) {
    uint32_t tryIndex = 0;
    for (; tryIndex < DMA_ALLOCATION_TRY_COUNT; ++tryIndex) {
      DmaDeviceMemoryBlock *pBestRequestBlock = DMA_NULL;
      DmaAllocationRequest bestRequest = {};
      uint32_t bestRequestCost = DMA_WHOLE_SIZE;

      // 1. Search existing allocations.
      if (strategy == DMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT) {
        // Forward order in m_Blocks - prefer blocks with smallest amount of
        // free space.
        for (size_t blockIndex = 0; blockIndex < m_Blocks.size();
             ++blockIndex) {
          DmaDeviceMemoryBlock *const pCurrBlock = m_Blocks[blockIndex];
          DMA_ASSERT(pCurrBlock);
          DmaAllocationRequest currRequest = {};
          if (pCurrBlock->m_pMetadata->CreateAllocationRequest(
                  currentFrameIndex, m_FrameInUseCount, size, alignment,
                  (createInfo.flags &
                   DMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT) != 0,
                  suballocType, canMakeOtherLost, strategy, &currRequest)) {
            const uint32_t currRequestCost = currRequest.CalcCost();
            if (pBestRequestBlock == DMA_NULL ||
                currRequestCost < bestRequestCost) {
              pBestRequestBlock = pCurrBlock;
              bestRequest = currRequest;
              bestRequestCost = currRequestCost;

              if (bestRequestCost == 0) {
                break;
              }
            }
          }
        }
      } else // WORST_FIT, FIRST_FIT
      {
        // Backward order in m_Blocks - prefer blocks with largest amount of
        // free space.
        for (size_t blockIndex = m_Blocks.size(); blockIndex--;) {
          DmaDeviceMemoryBlock *const pCurrBlock = m_Blocks[blockIndex];
          DMA_ASSERT(pCurrBlock);
          DmaAllocationRequest currRequest = {};
          if (pCurrBlock->m_pMetadata->CreateAllocationRequest(
                  currentFrameIndex, m_FrameInUseCount, size, alignment,
                  (createInfo.flags &
                   DMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT) != 0,
                  suballocType, canMakeOtherLost, strategy, &currRequest)) {
            const uint32_t currRequestCost = currRequest.CalcCost();
            if (pBestRequestBlock == DMA_NULL ||
                currRequestCost < bestRequestCost ||
                strategy == DMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT) {
              pBestRequestBlock = pCurrBlock;
              bestRequest = currRequest;
              bestRequestCost = currRequestCost;

              if (bestRequestCost == 0 ||
                  strategy == DMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT) {
                break;
              }
            }
          }
        }
      }

      if (pBestRequestBlock != DMA_NULL) {
        if (mapped) {
          DkResult res = pBestRequestBlock->Map(m_hAllocator, 1, DMA_NULL);
          if (res != DkResult_Success) {
            return res;
          }
        }

        if (pBestRequestBlock->m_pMetadata->MakeRequestedAllocationsLost(
                currentFrameIndex, m_FrameInUseCount, &bestRequest)) {
          // Allocate from this pBlock.
          *pAllocation = m_hAllocator->m_AllocationObjectAllocator.Allocate();
          (*pAllocation)->Ctor(currentFrameIndex, isUserDataString);
          pBestRequestBlock->m_pMetadata->Alloc(bestRequest, suballocType, size,
                                                *pAllocation);
          UpdateHasEmptyBlock();
          (*pAllocation)
              ->InitBlockAllocation(
                  pBestRequestBlock, bestRequest.offset, alignment, size,
                  m_MemoryTypeIndex, suballocType, mapped,
                  (createInfo.flags &
                   DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT) != 0);
          DMA_HEAVY_ASSERT(pBestRequestBlock->Validate());
          DMA_DEBUG_LOG("    Returned from existing block");
          (*pAllocation)->SetUserData(m_hAllocator, createInfo.pUserData);
          m_hAllocator->m_Budget.AddAllocation(0, size);
          if (DMA_DEBUG_INITIALIZE_ALLOCATIONS) {
            m_hAllocator->FillAllocation(*pAllocation,
                                         DMA_ALLOCATION_FILL_PATTERN_CREATED);
          }
          if (IsCorruptionDetectionEnabled()) {
            DkResult res = pBestRequestBlock->WriteMagicValueAroundAllocation(
                m_hAllocator, bestRequest.offset, size);
            DMA_ASSERT(res == DkResult_Success &&
                       "Couldn't map block memory to write magic value.");
          }
          return DkResult_Success;
        }
        // else: Some allocations must have been touched while we are here. Next
        // try.
      } else {
        // Could not find place in any of the blocks - break outer loop.
        break;
      }
    }
    /* Maximum number of tries exceeded - a very unlike event when many other
    threads are simultaneously touching allocations making it impossible to make
    lost at the same time as we try to allocate. */
    if (tryIndex == DMA_ALLOCATION_TRY_COUNT) {
      return DkResult_OutOfMemory;
    }
  }

  return DkResult_OutOfMemory;
}

void DmaBlockVector::Free(const DmaAllocation hAllocation) {
  DmaDeviceMemoryBlock *pBlockToDelete = DMA_NULL;

  bool budgetExceeded = false;
  {
    DmaBudget heapBudget = {};
    m_hAllocator->GetBudget(&heapBudget, 0, 1);
    budgetExceeded = heapBudget.usage >= heapBudget.budget;
  }

  // Scope for lock.
  {
    DmaMutexLockWrite lock(m_Mutex, m_hAllocator->m_UseMutex);

    DmaDeviceMemoryBlock *pBlock = hAllocation->GetBlock();

    if (IsCorruptionDetectionEnabled()) {
      DkResult res = pBlock->ValidateMagicValueAroundAllocation(
          m_hAllocator, hAllocation->GetOffset(), hAllocation->GetSize());
      DMA_ASSERT(res == DkResult_Success &&
                 "Couldn't map block memory to validate magic value.");
    }

    if (hAllocation->IsPersistentMap()) {
      pBlock->Unmap(m_hAllocator, 1);
    }

    pBlock->m_pMetadata->Free(hAllocation);
    DMA_HEAVY_ASSERT(pBlock->Validate());

    DMA_DEBUG_LOG("  Freed from MemoryTypeIndex=%u", m_MemoryTypeIndex);

    const bool canDeleteBlock = m_Blocks.size() > m_MinBlockCount;
    // pBlock became empty after this deallocation.
    if (pBlock->m_pMetadata->IsEmpty()) {
      // Already has empty block. We don't want to have two, so delete this one.
      if ((m_HasEmptyBlock || budgetExceeded) && canDeleteBlock) {
        pBlockToDelete = pBlock;
        Remove(pBlock);
      }
      // else: We now have an empty block - leave it.
    }
    // pBlock didn't become empty, but we have another empty block - find and
    // free that one. (This is optional, heuristics.)
    else if (m_HasEmptyBlock && canDeleteBlock) {
      DmaDeviceMemoryBlock *pLastBlock = m_Blocks.back();
      if (pLastBlock->m_pMetadata->IsEmpty()) {
        pBlockToDelete = pLastBlock;
        m_Blocks.pop_back();
      }
    }

    UpdateHasEmptyBlock();
    IncrementallySortBlocks();
  }

  // Destruction of a free block. Deferred until this point, outside of mutex
  // lock, for performance reason.
  if (pBlockToDelete != DMA_NULL) {
    DMA_DEBUG_LOG("    Deleted empty block");
    pBlockToDelete->Destroy(m_hAllocator);
    dma_delete(m_hAllocator, pBlockToDelete);
  }
}

uint32_t DmaBlockVector::CalcMaxBlockSize() const {
  uint32_t result = 0;
  for (size_t i = m_Blocks.size(); i--;) {
    result = DMA_MAX(result, m_Blocks[i]->m_pMetadata->GetSize());
    if (result >= m_PreferredBlockSize) {
      break;
    }
  }
  return result;
}

void DmaBlockVector::Remove(DmaDeviceMemoryBlock *pBlock) {
  for (uint32_t blockIndex = 0; blockIndex < m_Blocks.size(); ++blockIndex) {
    if (m_Blocks[blockIndex] == pBlock) {
      DmaVectorRemove(m_Blocks, blockIndex);
      return;
    }
  }
  DMA_ASSERT(0);
}

void DmaBlockVector::IncrementallySortBlocks() {
  if (m_Algorithm != DMA_POOL_CREATE_LINEAR_ALGORITHM_BIT) {
    // Bubble sort only until first swap.
    for (size_t i = 1; i < m_Blocks.size(); ++i) {
      if (m_Blocks[i - 1]->m_pMetadata->GetSumFreeSize() >
          m_Blocks[i]->m_pMetadata->GetSumFreeSize()) {
        DMA_SWAP(m_Blocks[i - 1], m_Blocks[i]);
        return;
      }
    }
  }
}

DkResult DmaBlockVector::AllocateFromBlock(
    DmaDeviceMemoryBlock *pBlock, uint32_t currentFrameIndex, uint32_t size,
    uint32_t alignment, DmaAllocationCreateFlags allocFlags, void *pUserData,
    DmaSuballocationType suballocType, uint32_t strategy,
    DmaAllocation *pAllocation) {
  DMA_ASSERT((allocFlags & DMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT) == 0);
  const bool isUpperAddress =
      (allocFlags & DMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT) != 0;
  const bool mapped = (allocFlags & DMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;
  const bool isUserDataString =
      (allocFlags & DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT) != 0;

  DmaAllocationRequest currRequest = {};
  if (pBlock->m_pMetadata->CreateAllocationRequest(
          currentFrameIndex, m_FrameInUseCount, size, alignment, isUpperAddress,
          suballocType,
          false, // canMakeOtherLost
          strategy, &currRequest)) {
    // Allocate from pCurrBlock.
    DMA_ASSERT(currRequest.itemsToMakeLostCount == 0);

    if (mapped) {
      DkResult res = pBlock->Map(m_hAllocator, 1, DMA_NULL);
      if (res != DkResult_Success) {
        return res;
      }
    }

    *pAllocation = m_hAllocator->m_AllocationObjectAllocator.Allocate();
    (*pAllocation)->Ctor(currentFrameIndex, isUserDataString);
    pBlock->m_pMetadata->Alloc(currRequest, suballocType, size, *pAllocation);
    UpdateHasEmptyBlock();
    (*pAllocation)
        ->InitBlockAllocation(
            pBlock, currRequest.offset, alignment, size, m_MemoryTypeIndex,
            suballocType, mapped,
            (allocFlags & DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT) != 0);
    DMA_HEAVY_ASSERT(pBlock->Validate());
    (*pAllocation)->SetUserData(m_hAllocator, pUserData);
    m_hAllocator->m_Budget.AddAllocation(0, size);
    if (DMA_DEBUG_INITIALIZE_ALLOCATIONS) {
      m_hAllocator->FillAllocation(*pAllocation,
                                   DMA_ALLOCATION_FILL_PATTERN_CREATED);
    }
    if (IsCorruptionDetectionEnabled()) {
      DkResult res = pBlock->WriteMagicValueAroundAllocation(
          m_hAllocator, currRequest.offset, size);
      DMA_ASSERT(res == DkResult_Success &&
                 "Couldn't map block memory to write magic value.");
    }
    return DkResult_Success;
  }
  return DkResult_OutOfMemory;
}

DkResult DmaBlockVector::CreateBlock(uint32_t blockSize,
                                     size_t *pNewBlockIndex) {
  DMA_ASSERT(blockSize && (blockSize % DK_MEMBLOCK_ALIGNMENT == 0) &&
             "Block size not aligned for deko.");

  DmaMemoryAllocateInfo allocInfo = {};
  allocInfo.memoryTypeIndex = m_MemoryTypeIndex;
  allocInfo.allocationSize = blockSize;
  DkMemBlock mem = DMA_NULL_HANDLE;
  DkResult res = m_hAllocator->AllocateVulkanMemory(&allocInfo, &mem);
  if (res < 0) {
    return res;
  }

  // New DkMemBlock successfully created.

  // Create new Allocation for it.
  DmaDeviceMemoryBlock *const pBlock =
      dma_new(m_hAllocator, DmaDeviceMemoryBlock)(m_hAllocator);
  pBlock->Init(m_hAllocator, m_hParentPool, m_MemoryTypeIndex, mem,
               allocInfo.allocationSize, m_NextBlockId++, m_Algorithm);

  m_Blocks.push_back(pBlock);
  if (pNewBlockIndex != DMA_NULL) {
    *pNewBlockIndex = m_Blocks.size() - 1;
  }

  return DkResult_Success;
}

void DmaBlockVector::ApplyDefragmentationMovesCpu(
    class DmaBlockVectorDefragmentationContext *pDefragCtx,
    const DmaVector<DmaDefragmentationMove,
                    DmaStlAllocator<DmaDefragmentationMove>> &moves) {
  const size_t blockCount = m_Blocks.size();

  enum BLOCK_FLAG {
    BLOCK_FLAG_USED = 0x00000001,
    BLOCK_FLAG_MAPPED_FOR_DEFRAGMENTATION = 0x00000002,
  };

  struct BlockInfo {
    uint32_t flags;
    void *pMappedData;
  };
  DmaVector<BlockInfo, DmaStlAllocator<BlockInfo>> blockInfo(
      blockCount, BlockInfo(),
      DmaStlAllocator<BlockInfo>(m_hAllocator->GetAllocationCallbacks()));
  memset(blockInfo.data(), 0, blockCount * sizeof(BlockInfo));

  // Go over all moves. Mark blocks that are used with BLOCK_FLAG_USED.
  const size_t moveCount = moves.size();
  for (size_t moveIndex = 0; moveIndex < moveCount; ++moveIndex) {
    const DmaDefragmentationMove &move = moves[moveIndex];
    blockInfo[move.srcBlockIndex].flags |= BLOCK_FLAG_USED;
    blockInfo[move.dstBlockIndex].flags |= BLOCK_FLAG_USED;
  }

  DMA_ASSERT(pDefragCtx->res == DkResult_Success);

  // Go over all blocks. Get mapped pointer or map if necessary.
  for (size_t blockIndex = 0;
       pDefragCtx->res == DkResult_Success && blockIndex < blockCount;
       ++blockIndex) {
    BlockInfo &currBlockInfo = blockInfo[blockIndex];
    DmaDeviceMemoryBlock *pBlock = m_Blocks[blockIndex];
    if ((currBlockInfo.flags & BLOCK_FLAG_USED) != 0) {
      currBlockInfo.pMappedData = pBlock->GetMappedData();
      // It is not originally mapped - map it.
      if (currBlockInfo.pMappedData == DMA_NULL) {
        pDefragCtx->res =
            pBlock->Map(m_hAllocator, 1, &currBlockInfo.pMappedData);
        if (pDefragCtx->res == DkResult_Success) {
          currBlockInfo.flags |= BLOCK_FLAG_MAPPED_FOR_DEFRAGMENTATION;
        }
      }
    }
  }

  // Go over all moves. Do actual data transfer.
  if (pDefragCtx->res == DkResult_Success) {
    for (size_t moveIndex = 0; moveIndex < moveCount; ++moveIndex) {
      const DmaDefragmentationMove &move = moves[moveIndex];

      const BlockInfo &srcBlockInfo = blockInfo[move.srcBlockIndex];
      const BlockInfo &dstBlockInfo = blockInfo[move.dstBlockIndex];

      DMA_ASSERT(srcBlockInfo.pMappedData && dstBlockInfo.pMappedData);

      // THE PLACE WHERE ACTUAL DATA COPY HAPPENS.
      memmove(
          reinterpret_cast<char *>(dstBlockInfo.pMappedData) + move.dstOffset,
          reinterpret_cast<char *>(srcBlockInfo.pMappedData) + move.srcOffset,
          static_cast<size_t>(move.size));

      if (IsCorruptionDetectionEnabled()) {
        DmaWriteMagicValue(dstBlockInfo.pMappedData,
                           move.dstOffset - DMA_DEBUG_MARGIN);
        DmaWriteMagicValue(dstBlockInfo.pMappedData,
                           move.dstOffset + move.size);
      }

      // Flush destination.
      DmaDeviceMemoryBlock *const pDstBlock = m_Blocks[move.dstBlockIndex];
      dkMemBlockFlushCpuCache(
          pDstBlock->GetDeviceMemory(), move.dstOffset,
          DMA_MIN(move.size,
                  pDstBlock->m_pMetadata->GetSize() - move.dstOffset));
    }
  }

  // Go over all blocks in reverse order. Unmap those that were mapped just for
  // defragmentation. Regardless of pCtx->res == DkResult_Success.
  for (size_t blockIndex = blockCount; blockIndex--;) {
    const BlockInfo &currBlockInfo = blockInfo[blockIndex];
    if ((currBlockInfo.flags & BLOCK_FLAG_MAPPED_FOR_DEFRAGMENTATION) != 0) {
      DmaDeviceMemoryBlock *pBlock = m_Blocks[blockIndex];
      pBlock->Unmap(m_hAllocator, 1);
    }
  }
}

void DmaBlockVector::ApplyDefragmentationMovesGpu(
    class DmaBlockVectorDefragmentationContext *pDefragCtx,
    const DmaVector<DmaDefragmentationMove,
                    DmaStlAllocator<DmaDefragmentationMove>> &moves,
    DkCmdBuf commandBuffer) {
  const size_t blockCount = m_Blocks.size();

  pDefragCtx->blockContexts.resize(blockCount);
  memset(pDefragCtx->blockContexts.data(), 0,
         blockCount * sizeof(DmaBlockDefragmentationContext));

  // Go over all moves. Mark blocks that are used with BLOCK_FLAG_USED.
  const size_t moveCount = moves.size();
  for (size_t moveIndex = 0; moveIndex < moveCount; ++moveIndex) {
    const DmaDefragmentationMove &move = moves[moveIndex];
    pDefragCtx->blockContexts[move.srcBlockIndex].flags |=
        DmaBlockDefragmentationContext::BLOCK_FLAG_USED;
    pDefragCtx->blockContexts[move.dstBlockIndex].flags |=
        DmaBlockDefragmentationContext::BLOCK_FLAG_USED;
  }

  DMA_ASSERT(pDefragCtx->res == DkResult_Success);

  // Go over all blocks. Create and bind buffer for whole block if necessary.
  {
    for (size_t blockIndex = 0;
         pDefragCtx->res == DkResult_Success && blockIndex < blockCount;
         ++blockIndex) {
      DmaBlockDefragmentationContext &currBlockCtx =
          pDefragCtx->blockContexts[blockIndex];
      DmaDeviceMemoryBlock *pBlock = m_Blocks[blockIndex];
      if ((currBlockCtx.flags &
           DmaBlockDefragmentationContext::BLOCK_FLAG_USED) != 0) {
        currBlockCtx.hBuffer.addr =
            dkMemBlockGetGpuAddr(pBlock->GetDeviceMemory());
        currBlockCtx.hBuffer.size = pBlock->m_pMetadata->GetSize();
      }
    }
  }

  // Go over all moves. Post data transfer commands to command buffer.
  if (pDefragCtx->res == DkResult_Success) {
    for (size_t moveIndex = 0; moveIndex < moveCount; ++moveIndex) {
      const DmaDefragmentationMove &move = moves[moveIndex];

      const DmaBlockDefragmentationContext &srcBlockCtx =
          pDefragCtx->blockContexts[move.srcBlockIndex];
      const DmaBlockDefragmentationContext &dstBlockCtx =
          pDefragCtx->blockContexts[move.dstBlockIndex];

      dkCmdBufCopyBuffer(commandBuffer,
                         srcBlockCtx.hBuffer.addr + move.srcOffset,
                         dstBlockCtx.hBuffer.addr + move.dstOffset, move.size);
    }
  }

  // Save buffers to defrag context for later destruction.
  if (pDefragCtx->res == DkResult_Success && moveCount > 0) {
    pDefragCtx->res = DkResult_Fail;
  }
}

void DmaBlockVector::FreeEmptyBlocks(
    DmaDefragmentationStats *pDefragmentationStats) {
  for (size_t blockIndex = m_Blocks.size(); blockIndex--;) {
    DmaDeviceMemoryBlock *pBlock = m_Blocks[blockIndex];
    if (pBlock->m_pMetadata->IsEmpty()) {
      if (m_Blocks.size() > m_MinBlockCount) {
        if (pDefragmentationStats != DMA_NULL) {
          ++pDefragmentationStats->deviceMemoryBlocksFreed;
          pDefragmentationStats->bytesFreed += pBlock->m_pMetadata->GetSize();
        }

        DmaVectorRemove(m_Blocks, blockIndex);
        pBlock->Destroy(m_hAllocator);
        dma_delete(m_hAllocator, pBlock);
      } else {
        break;
      }
    }
  }
  UpdateHasEmptyBlock();
}

void DmaBlockVector::UpdateHasEmptyBlock() {
  m_HasEmptyBlock = false;
  for (size_t index = 0, count = m_Blocks.size(); index < count; ++index) {
    DmaDeviceMemoryBlock *const pBlock = m_Blocks[index];
    if (pBlock->m_pMetadata->IsEmpty()) {
      m_HasEmptyBlock = true;
      break;
    }
  }
}

#if DMA_STATS_STRING_ENABLED

void DmaBlockVector::PrintDetailedMap(class DmaJsonWriter &json) {
  DmaMutexLockRead lock(m_Mutex, m_hAllocator->m_UseMutex);

  json.BeginObject();

  if (IsCustomPool()) {
    const char *poolName = m_hParentPool->GetName();
    if (poolName != DMA_NULL && poolName[0] != '\0') {
      json.WriteString("Name");
      json.WriteString(poolName);
    }

    json.WriteString("MemoryTypeIndex");
    json.WriteNumber(m_MemoryTypeIndex);

    json.WriteString("BlockSize");
    json.WriteNumber(m_PreferredBlockSize);

    json.WriteString("BlockCount");
    json.BeginObject(true);
    if (m_MinBlockCount > 0) {
      json.WriteString("Min");
      json.WriteNumber((uint64_t)m_MinBlockCount);
    }
    if (m_MaxBlockCount < SIZE_MAX) {
      json.WriteString("Max");
      json.WriteNumber((uint64_t)m_MaxBlockCount);
    }
    json.WriteString("Cur");
    json.WriteNumber((uint64_t)m_Blocks.size());
    json.EndObject();

    if (m_FrameInUseCount > 0) {
      json.WriteString("FrameInUseCount");
      json.WriteNumber(m_FrameInUseCount);
    }

    if (m_Algorithm != 0) {
      json.WriteString("Algorithm");
      json.WriteString(DmaAlgorithmToStr(m_Algorithm));
    }
  } else {
    json.WriteString("PreferredBlockSize");
    json.WriteNumber(m_PreferredBlockSize);
  }

  json.WriteString("Blocks");
  json.BeginObject();
  for (size_t i = 0; i < m_Blocks.size(); ++i) {
    json.BeginString();
    json.ContinueString(m_Blocks[i]->GetId());
    json.EndString();

    m_Blocks[i]->m_pMetadata->PrintDetailedMap(json);
  }
  json.EndObject();

  json.EndObject();
}

#endif // #if DMA_STATS_STRING_ENABLED

void DmaBlockVector::Defragment(
    class DmaBlockVectorDefragmentationContext *pCtx,
    DmaDefragmentationStats *pStats, uint32_t &maxCpuBytesToMove,
    uint32_t &maxCpuAllocationsToMove, uint32_t &maxGpuBytesToMove,
    uint32_t &maxGpuAllocationsToMove, DkCmdBuf commandBuffer) {
  pCtx->res = DkResult_Success;

  const bool isHostVisible = true;

  const bool canDefragmentOnCpu =
      maxCpuBytesToMove > 0 && maxCpuAllocationsToMove > 0 && isHostVisible;
  const bool canDefragmentOnGpu =
      maxGpuBytesToMove > 0 && maxGpuAllocationsToMove > 0 &&
      !IsCorruptionDetectionEnabled() &&
      ((1u << m_MemoryTypeIndex) &
       m_hAllocator->GetGpuDefragmentationMemoryTypeBits()) != 0;

  // There are options to defragment this memory type.
  if (canDefragmentOnCpu || canDefragmentOnGpu) {
    bool defragmentOnGpu;
    // There is only one option to defragment this memory type.
    if (canDefragmentOnGpu != canDefragmentOnCpu) {
      defragmentOnGpu = canDefragmentOnGpu;
    }
    // Both options are available: Heuristics to choose the best one.
    else {
      defragmentOnGpu = true;
    }

    bool overlappingMoveSupported = !defragmentOnGpu;

    if (m_hAllocator->m_UseMutex) {
      m_Mutex.LockWrite();
      pCtx->mutexLocked = true;
    }

    pCtx->Begin(overlappingMoveSupported);

    // Defragment.

    const uint32_t maxBytesToMove =
        defragmentOnGpu ? maxGpuBytesToMove : maxCpuBytesToMove;
    const uint32_t maxAllocationsToMove =
        defragmentOnGpu ? maxGpuAllocationsToMove : maxCpuAllocationsToMove;
    DmaVector<DmaDefragmentationMove, DmaStlAllocator<DmaDefragmentationMove>>
        moves = DmaVector<DmaDefragmentationMove,
                          DmaStlAllocator<DmaDefragmentationMove>>(
            DmaStlAllocator<DmaDefragmentationMove>(
                m_hAllocator->GetAllocationCallbacks()));
    pCtx->res = pCtx->GetAlgorithm()->Defragment(moves, maxBytesToMove,
                                                 maxAllocationsToMove);

    // Accumulate statistics.
    if (pStats != DMA_NULL) {
      const uint32_t bytesMoved = pCtx->GetAlgorithm()->GetBytesMoved();
      const uint32_t allocationsMoved =
          pCtx->GetAlgorithm()->GetAllocationsMoved();
      pStats->bytesMoved += bytesMoved;
      pStats->allocationsMoved += allocationsMoved;
      DMA_ASSERT(bytesMoved <= maxBytesToMove);
      DMA_ASSERT(allocationsMoved <= maxAllocationsToMove);
      if (defragmentOnGpu) {
        maxGpuBytesToMove -= bytesMoved;
        maxGpuAllocationsToMove -= allocationsMoved;
      } else {
        maxCpuBytesToMove -= bytesMoved;
        maxCpuAllocationsToMove -= allocationsMoved;
      }
    }

    if (pCtx->res >= DkResult_Success) {
      if (defragmentOnGpu) {
        ApplyDefragmentationMovesGpu(pCtx, moves, commandBuffer);
      } else {
        ApplyDefragmentationMovesCpu(pCtx, moves);
      }
    }
  }
}

void DmaBlockVector::DefragmentationEnd(
    class DmaBlockVectorDefragmentationContext *pCtx,
    DmaDefragmentationStats *pStats) {
  if (pCtx->res >= DkResult_Success) {
    FreeEmptyBlocks(pStats);
  }

  if (pCtx->mutexLocked) {
    DMA_ASSERT(m_hAllocator->m_UseMutex);
    m_Mutex.UnlockWrite();
  }
}

size_t DmaBlockVector::CalcAllocationCount() const {
  size_t result = 0;
  for (size_t i = 0; i < m_Blocks.size(); ++i) {
    result += m_Blocks[i]->m_pMetadata->GetAllocationCount();
  }
  return result;
}

void DmaBlockVector::MakePoolAllocationsLost(uint32_t currentFrameIndex,
                                             size_t *pLostAllocationCount) {
  DmaMutexLockWrite lock(m_Mutex, m_hAllocator->m_UseMutex);
  size_t lostAllocationCount = 0;
  for (uint32_t blockIndex = 0; blockIndex < m_Blocks.size(); ++blockIndex) {
    DmaDeviceMemoryBlock *const pBlock = m_Blocks[blockIndex];
    DMA_ASSERT(pBlock);
    lostAllocationCount += pBlock->m_pMetadata->MakeAllocationsLost(
        currentFrameIndex, m_FrameInUseCount);
  }
  if (pLostAllocationCount != DMA_NULL) {
    *pLostAllocationCount = lostAllocationCount;
  }
}

DkResult DmaBlockVector::CheckCorruption() {
  if (!IsCorruptionDetectionEnabled()) {
    return DkResult_NotImplemented;
  }

  DmaMutexLockRead lock(m_Mutex, m_hAllocator->m_UseMutex);
  for (uint32_t blockIndex = 0; blockIndex < m_Blocks.size(); ++blockIndex) {
    DmaDeviceMemoryBlock *const pBlock = m_Blocks[blockIndex];
    DMA_ASSERT(pBlock);
    DkResult res = pBlock->CheckCorruption(m_hAllocator);
    if (res != DkResult_Success) {
      return res;
    }
  }
  return DkResult_Success;
}

void DmaBlockVector::AddStats(DmaStats *pStats) {
  const uint32_t memTypeIndex = m_MemoryTypeIndex;

  DmaMutexLockRead lock(m_Mutex, m_hAllocator->m_UseMutex);

  for (uint32_t blockIndex = 0; blockIndex < m_Blocks.size(); ++blockIndex) {
    const DmaDeviceMemoryBlock *const pBlock = m_Blocks[blockIndex];
    DMA_ASSERT(pBlock);
    DMA_HEAVY_ASSERT(pBlock->Validate());
    DmaStatInfo allocationStatInfo;
    pBlock->m_pMetadata->CalcAllocationStatInfo(allocationStatInfo);
    DmaAddStatInfo(pStats->total, allocationStatInfo);
    DmaAddStatInfo(pStats->memoryType[memTypeIndex], allocationStatInfo);
    DmaAddStatInfo(pStats->memoryHeap[0], allocationStatInfo);
  }
}

////////////////////////////////////////////////////////////////////////////////
// DmaDefragmentationAlgorithm_Generic members definition

DmaDefragmentationAlgorithm_Generic::DmaDefragmentationAlgorithm_Generic(
    DmaAllocator hAllocator, DmaBlockVector *pBlockVector,
    uint32_t currentFrameIndex, bool overlappingMoveSupported)
    : DmaDefragmentationAlgorithm(hAllocator, pBlockVector, currentFrameIndex),
      m_AllocationCount(0), m_AllAllocations(false), m_BytesMoved(0),
      m_AllocationsMoved(0), m_Blocks(DmaStlAllocator<BlockInfo *>(
                                 hAllocator->GetAllocationCallbacks())) {
  // Create block info for each block.
  const size_t blockCount = m_pBlockVector->m_Blocks.size();
  for (size_t blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
    BlockInfo *pBlockInfo = dma_new(m_hAllocator, BlockInfo)(
        m_hAllocator->GetAllocationCallbacks());
    pBlockInfo->m_OriginalBlockIndex = blockIndex;
    pBlockInfo->m_pBlock = m_pBlockVector->m_Blocks[blockIndex];
    m_Blocks.push_back(pBlockInfo);
  }

  // Sort them by m_pBlock pointer value.
  DMA_SORT(m_Blocks.begin(), m_Blocks.end(), BlockPointerLess());
}

DmaDefragmentationAlgorithm_Generic::~DmaDefragmentationAlgorithm_Generic() {
  for (size_t i = m_Blocks.size(); i--;) {
    dma_delete(m_hAllocator, m_Blocks[i]);
  }
}

void DmaDefragmentationAlgorithm_Generic::AddAllocation(DmaAllocation hAlloc,
                                                        bool *pChanged) {
  // Now as we are inside DmaBlockVector::m_Mutex, we can make final check if
  // this allocation was not lost.
  if (hAlloc->GetLastUseFrameIndex() != DMA_FRAME_INDEX_LOST) {
    DmaDeviceMemoryBlock *pBlock = hAlloc->GetBlock();
    BlockInfoVector::iterator it = DmaBinaryFindFirstNotLess(
        m_Blocks.begin(), m_Blocks.end(), pBlock, BlockPointerLess());
    if (it != m_Blocks.end() && (*it)->m_pBlock == pBlock) {
      AllocationInfo allocInfo = AllocationInfo(hAlloc, pChanged);
      (*it)->m_Allocations.push_back(allocInfo);
    } else {
      DMA_ASSERT(0);
    }

    ++m_AllocationCount;
  }
}

DkResult DmaDefragmentationAlgorithm_Generic::DefragmentRound(
    DmaVector<DmaDefragmentationMove, DmaStlAllocator<DmaDefragmentationMove>>
        &moves,
    uint32_t maxBytesToMove, uint32_t maxAllocationsToMove) {
  if (m_Blocks.empty()) {
    return DkResult_Success;
  }

  // This is a choice based on research.
  // Option 1:
  uint32_t strategy = DMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;
  // Option 2:
  // uint32_t strategy = DMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
  // Option 3:
  // uint32_t strategy = DMA_ALLOCATION_CREATE_STRATEGY_MIN_FRAGMENTATION_BIT;

  size_t srcBlockMinIndex = 0;
  // When FAST_ALGORITHM, move allocations from only last out of blocks that
  // contain non-movable allocations.
  /*
  if(m_AlgorithmFlags & DMA_DEFRAGMENTATION_FAST_ALGORITHM_BIT)
  {
      const size_t blocksWithNonMovableCount = CalcBlocksWithNonMovableCount();
      if(blocksWithNonMovableCount > 0)
      {
          srcBlockMinIndex = blocksWithNonMovableCount - 1;
      }
  }
  */

  size_t srcBlockIndex = m_Blocks.size() - 1;
  size_t srcAllocIndex = SIZE_MAX;
  for (;;) {
    // 1. Find next allocation to move.
    // 1.1. Start from last to first m_Blocks - they are sorted from most
    // "destination" to most "source". 1.2. Then start from last to first
    // m_Allocations.
    while (srcAllocIndex >= m_Blocks[srcBlockIndex]->m_Allocations.size()) {
      if (m_Blocks[srcBlockIndex]->m_Allocations.empty()) {
        // Finished: no more allocations to process.
        if (srcBlockIndex == srcBlockMinIndex) {
          return DkResult_Success;
        } else {
          --srcBlockIndex;
          srcAllocIndex = SIZE_MAX;
        }
      } else {
        srcAllocIndex = m_Blocks[srcBlockIndex]->m_Allocations.size() - 1;
      }
    }

    BlockInfo *pSrcBlockInfo = m_Blocks[srcBlockIndex];
    AllocationInfo &allocInfo = pSrcBlockInfo->m_Allocations[srcAllocIndex];

    const uint32_t size = allocInfo.m_hAllocation->GetSize();
    const uint32_t srcOffset = allocInfo.m_hAllocation->GetOffset();
    const uint32_t alignment = allocInfo.m_hAllocation->GetAlignment();
    const DmaSuballocationType suballocType =
        allocInfo.m_hAllocation->GetSuballocationType();

    // 2. Try to find new place for this allocation in preceding or current
    // block.
    for (size_t dstBlockIndex = 0; dstBlockIndex <= srcBlockIndex;
         ++dstBlockIndex) {
      BlockInfo *pDstBlockInfo = m_Blocks[dstBlockIndex];
      DmaAllocationRequest dstAllocRequest;
      if (pDstBlockInfo->m_pBlock->m_pMetadata->CreateAllocationRequest(
              m_CurrentFrameIndex, m_pBlockVector->GetFrameInUseCount(), size,
              alignment,
              false, // upperAddress
              suballocType,
              false, // canMakeOtherLost
              strategy, &dstAllocRequest) &&
          MoveMakesSense(dstBlockIndex, dstAllocRequest.offset, srcBlockIndex,
                         srcOffset)) {
        DMA_ASSERT(dstAllocRequest.itemsToMakeLostCount == 0);

        // Reached limit on number of allocations or bytes to move.
        if ((m_AllocationsMoved + 1 > maxAllocationsToMove) ||
            (m_BytesMoved + size > maxBytesToMove)) {
          return DkResult_Success;
        }

        DmaDefragmentationMove move;
        move.srcBlockIndex = pSrcBlockInfo->m_OriginalBlockIndex;
        move.dstBlockIndex = pDstBlockInfo->m_OriginalBlockIndex;
        move.srcOffset = srcOffset;
        move.dstOffset = dstAllocRequest.offset;
        move.size = size;
        moves.push_back(move);

        pDstBlockInfo->m_pBlock->m_pMetadata->Alloc(
            dstAllocRequest, suballocType, size, allocInfo.m_hAllocation);
        pSrcBlockInfo->m_pBlock->m_pMetadata->FreeAtOffset(srcOffset);

        allocInfo.m_hAllocation->ChangeBlockAllocation(
            m_hAllocator, pDstBlockInfo->m_pBlock, dstAllocRequest.offset);

        if (allocInfo.m_pChanged != DMA_NULL) {
          *allocInfo.m_pChanged = true;
        }

        ++m_AllocationsMoved;
        m_BytesMoved += size;

        DmaVectorRemove(pSrcBlockInfo->m_Allocations, srcAllocIndex);

        break;
      }
    }

    // If not processed, this allocInfo remains in pBlockInfo->m_Allocations for
    // next round.

    if (srcAllocIndex > 0) {
      --srcAllocIndex;
    } else {
      if (srcBlockIndex > 0) {
        --srcBlockIndex;
        srcAllocIndex = SIZE_MAX;
      } else {
        return DkResult_Success;
      }
    }
  }
}

size_t
DmaDefragmentationAlgorithm_Generic::CalcBlocksWithNonMovableCount() const {
  size_t result = 0;
  for (size_t i = 0; i < m_Blocks.size(); ++i) {
    if (m_Blocks[i]->m_HasNonMovableAllocations) {
      ++result;
    }
  }
  return result;
}

DkResult DmaDefragmentationAlgorithm_Generic::Defragment(
    DmaVector<DmaDefragmentationMove, DmaStlAllocator<DmaDefragmentationMove>>
        &moves,
    uint32_t maxBytesToMove, uint32_t maxAllocationsToMove) {
  if (!m_AllAllocations && m_AllocationCount == 0) {
    return DkResult_Success;
  }

  const size_t blockCount = m_Blocks.size();
  for (size_t blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
    BlockInfo *pBlockInfo = m_Blocks[blockIndex];

    if (m_AllAllocations) {
      DmaBlockMetadata_Generic *pMetadata =
          (DmaBlockMetadata_Generic *)pBlockInfo->m_pBlock->m_pMetadata;
      for (DmaSuballocationList::const_iterator it =
               pMetadata->m_Suballocations.begin();
           it != pMetadata->m_Suballocations.end(); ++it) {
        if (it->type != DMA_SUBALLOCATION_TYPE_FREE) {
          AllocationInfo allocInfo = AllocationInfo(it->hAllocation, DMA_NULL);
          pBlockInfo->m_Allocations.push_back(allocInfo);
        }
      }
    }

    pBlockInfo->CalcHasNonMovableAllocations();

    // This is a choice based on research.
    // Option 1:
    pBlockInfo->SortAllocationsByOffsetDescending();
    // Option 2:
    // pBlockInfo->SortAllocationsBySizeDescending();
  }

  // Sort m_Blocks this time by the main criterium, from most "destination" to
  // most "source" blocks.
  DMA_SORT(m_Blocks.begin(), m_Blocks.end(), BlockInfoCompareMoveDestination());

  // This is a choice based on research.
  const uint32_t roundCount = 2;

  // Execute defragmentation rounds (the main part).
  DkResult result = DkResult_Success;
  for (uint32_t round = 0; (round < roundCount) && (result == DkResult_Success);
       ++round) {
    result = DefragmentRound(moves, maxBytesToMove, maxAllocationsToMove);
  }

  return result;
}

bool DmaDefragmentationAlgorithm_Generic::MoveMakesSense(size_t dstBlockIndex,
                                                         uint32_t dstOffset,
                                                         size_t srcBlockIndex,
                                                         uint32_t srcOffset) {
  if (dstBlockIndex < srcBlockIndex) {
    return true;
  }
  if (dstBlockIndex > srcBlockIndex) {
    return false;
  }
  if (dstOffset < srcOffset) {
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// DmaDefragmentationAlgorithm_Fast

DmaDefragmentationAlgorithm_Fast::DmaDefragmentationAlgorithm_Fast(
    DmaAllocator hAllocator, DmaBlockVector *pBlockVector,
    uint32_t currentFrameIndex, bool overlappingMoveSupported)
    : DmaDefragmentationAlgorithm(hAllocator, pBlockVector, currentFrameIndex),
      m_OverlappingMoveSupported(overlappingMoveSupported),
      m_AllocationCount(0), m_AllAllocations(false), m_BytesMoved(0),
      m_AllocationsMoved(0), m_BlockInfos(DmaStlAllocator<BlockInfo>(
                                 hAllocator->GetAllocationCallbacks())) {
  DMA_ASSERT(DMA_DEBUG_MARGIN == 0);
}

DmaDefragmentationAlgorithm_Fast::~DmaDefragmentationAlgorithm_Fast() {}

DkResult DmaDefragmentationAlgorithm_Fast::Defragment(
    DmaVector<DmaDefragmentationMove, DmaStlAllocator<DmaDefragmentationMove>>
        &moves,
    uint32_t maxBytesToMove, uint32_t maxAllocationsToMove) {
  DMA_ASSERT(m_AllAllocations ||
             m_pBlockVector->CalcAllocationCount() == m_AllocationCount);

  const size_t blockCount = m_pBlockVector->GetBlockCount();
  if (blockCount == 0 || maxBytesToMove == 0 || maxAllocationsToMove == 0) {
    return DkResult_Success;
  }

  PreprocessMetadata();

  // Sort blocks in order from most destination.

  m_BlockInfos.resize(blockCount);
  for (size_t i = 0; i < blockCount; ++i) {
    m_BlockInfos[i].origBlockIndex = i;
  }

  DMA_SORT(m_BlockInfos.begin(), m_BlockInfos.end(),
           [this](const BlockInfo &lhs, const BlockInfo &rhs) -> bool {
             return m_pBlockVector->GetBlock(lhs.origBlockIndex)
                        ->m_pMetadata->GetSumFreeSize() <
                    m_pBlockVector->GetBlock(rhs.origBlockIndex)
                        ->m_pMetadata->GetSumFreeSize();
           });

  // THE MAIN ALGORITHM

  FreeSpaceDatabase freeSpaceDb;

  size_t dstBlockInfoIndex = 0;
  size_t dstOrigBlockIndex = m_BlockInfos[dstBlockInfoIndex].origBlockIndex;
  DmaDeviceMemoryBlock *pDstBlock = m_pBlockVector->GetBlock(dstOrigBlockIndex);
  DmaBlockMetadata_Generic *pDstMetadata =
      (DmaBlockMetadata_Generic *)pDstBlock->m_pMetadata;
  uint32_t dstBlockSize = pDstMetadata->GetSize();
  uint32_t dstOffset = 0;

  bool end = false;
  for (size_t srcBlockInfoIndex = 0; !end && srcBlockInfoIndex < blockCount;
       ++srcBlockInfoIndex) {
    const size_t srcOrigBlockIndex =
        m_BlockInfos[srcBlockInfoIndex].origBlockIndex;
    DmaDeviceMemoryBlock *const pSrcBlock =
        m_pBlockVector->GetBlock(srcOrigBlockIndex);
    DmaBlockMetadata_Generic *const pSrcMetadata =
        (DmaBlockMetadata_Generic *)pSrcBlock->m_pMetadata;
    for (DmaSuballocationList::iterator srcSuballocIt =
             pSrcMetadata->m_Suballocations.begin();
         !end && srcSuballocIt != pSrcMetadata->m_Suballocations.end();) {
      DmaAllocation_T *const pAlloc = srcSuballocIt->hAllocation;
      const uint32_t srcAllocAlignment = pAlloc->GetAlignment();
      const uint32_t srcAllocSize = srcSuballocIt->size;
      if (m_AllocationsMoved == maxAllocationsToMove ||
          m_BytesMoved + srcAllocSize > maxBytesToMove) {
        end = true;
        break;
      }
      const uint32_t srcAllocOffset = srcSuballocIt->offset;

      // Try to place it in one of free spaces from the database.
      size_t freeSpaceInfoIndex;
      uint32_t dstAllocOffset;
      if (freeSpaceDb.Fetch(srcAllocAlignment, srcAllocSize, freeSpaceInfoIndex,
                            dstAllocOffset)) {
        size_t freeSpaceOrigBlockIndex =
            m_BlockInfos[freeSpaceInfoIndex].origBlockIndex;
        DmaDeviceMemoryBlock *pFreeSpaceBlock =
            m_pBlockVector->GetBlock(freeSpaceOrigBlockIndex);
        DmaBlockMetadata_Generic *pFreeSpaceMetadata =
            (DmaBlockMetadata_Generic *)pFreeSpaceBlock->m_pMetadata;

        // Same block
        if (freeSpaceInfoIndex == srcBlockInfoIndex) {
          DMA_ASSERT(dstAllocOffset <= srcAllocOffset);

          // MOVE OPTION 1: Move the allocation inside the same block by
          // decreasing offset.

          DmaSuballocation suballoc = *srcSuballocIt;
          suballoc.offset = dstAllocOffset;
          suballoc.hAllocation->ChangeOffset(dstAllocOffset);
          m_BytesMoved += srcAllocSize;
          ++m_AllocationsMoved;

          DmaSuballocationList::iterator nextSuballocIt = srcSuballocIt;
          ++nextSuballocIt;
          pSrcMetadata->m_Suballocations.erase(srcSuballocIt);
          srcSuballocIt = nextSuballocIt;

          InsertSuballoc(pFreeSpaceMetadata, suballoc);

          DmaDefragmentationMove move = {
              srcOrigBlockIndex, freeSpaceOrigBlockIndex, srcAllocOffset,
              dstAllocOffset, srcAllocSize};
          moves.push_back(move);
        }
        // Different block
        else {
          // MOVE OPTION 2: Move the allocation to a different block.

          DMA_ASSERT(freeSpaceInfoIndex < srcBlockInfoIndex);

          DmaSuballocation suballoc = *srcSuballocIt;
          suballoc.offset = dstAllocOffset;
          suballoc.hAllocation->ChangeBlockAllocation(
              m_hAllocator, pFreeSpaceBlock, dstAllocOffset);
          m_BytesMoved += srcAllocSize;
          ++m_AllocationsMoved;

          DmaSuballocationList::iterator nextSuballocIt = srcSuballocIt;
          ++nextSuballocIt;
          pSrcMetadata->m_Suballocations.erase(srcSuballocIt);
          srcSuballocIt = nextSuballocIt;

          InsertSuballoc(pFreeSpaceMetadata, suballoc);

          DmaDefragmentationMove move = {
              srcOrigBlockIndex, freeSpaceOrigBlockIndex, srcAllocOffset,
              dstAllocOffset, srcAllocSize};
          moves.push_back(move);
        }
      } else {
        dstAllocOffset = DmaAlignUp(dstOffset, srcAllocAlignment);

        // If the allocation doesn't fit before the end of dstBlock, forward to
        // next block.
        while (dstBlockInfoIndex < srcBlockInfoIndex &&
               dstAllocOffset + srcAllocSize > dstBlockSize) {
          // But before that, register remaining free space at the end of dst
          // block.
          freeSpaceDb.Register(dstBlockInfoIndex, dstOffset,
                               dstBlockSize - dstOffset);

          ++dstBlockInfoIndex;
          dstOrigBlockIndex = m_BlockInfos[dstBlockInfoIndex].origBlockIndex;
          pDstBlock = m_pBlockVector->GetBlock(dstOrigBlockIndex);
          pDstMetadata = (DmaBlockMetadata_Generic *)pDstBlock->m_pMetadata;
          dstBlockSize = pDstMetadata->GetSize();
          dstOffset = 0;
          dstAllocOffset = 0;
        }

        // Same block
        if (dstBlockInfoIndex == srcBlockInfoIndex) {
          DMA_ASSERT(dstAllocOffset <= srcAllocOffset);

          const bool overlap = dstAllocOffset + srcAllocSize > srcAllocOffset;

          bool skipOver = overlap;
          if (overlap && m_OverlappingMoveSupported &&
              dstAllocOffset < srcAllocOffset) {
            // If destination and source place overlap, skip if it would move it
            // by only < 1/64 of its size.
            skipOver = (srcAllocOffset - dstAllocOffset) * 64 < srcAllocSize;
          }

          if (skipOver) {
            freeSpaceDb.Register(dstBlockInfoIndex, dstOffset,
                                 srcAllocOffset - dstOffset);

            dstOffset = srcAllocOffset + srcAllocSize;
            ++srcSuballocIt;
          }
          // MOVE OPTION 1: Move the allocation inside the same block by
          // decreasing offset.
          else {
            srcSuballocIt->offset = dstAllocOffset;
            srcSuballocIt->hAllocation->ChangeOffset(dstAllocOffset);
            dstOffset = dstAllocOffset + srcAllocSize;
            m_BytesMoved += srcAllocSize;
            ++m_AllocationsMoved;
            ++srcSuballocIt;
            DmaDefragmentationMove move = {srcOrigBlockIndex, dstOrigBlockIndex,
                                           srcAllocOffset, dstAllocOffset,
                                           srcAllocSize};
            moves.push_back(move);
          }
        }
        // Different block
        else {
          // MOVE OPTION 2: Move the allocation to a different block.

          DMA_ASSERT(dstBlockInfoIndex < srcBlockInfoIndex);
          DMA_ASSERT(dstAllocOffset + srcAllocSize <= dstBlockSize);

          DmaSuballocation suballoc = *srcSuballocIt;
          suballoc.offset = dstAllocOffset;
          suballoc.hAllocation->ChangeBlockAllocation(m_hAllocator, pDstBlock,
                                                      dstAllocOffset);
          dstOffset = dstAllocOffset + srcAllocSize;
          m_BytesMoved += srcAllocSize;
          ++m_AllocationsMoved;

          DmaSuballocationList::iterator nextSuballocIt = srcSuballocIt;
          ++nextSuballocIt;
          pSrcMetadata->m_Suballocations.erase(srcSuballocIt);
          srcSuballocIt = nextSuballocIt;

          pDstMetadata->m_Suballocations.push_back(suballoc);

          DmaDefragmentationMove move = {srcOrigBlockIndex, dstOrigBlockIndex,
                                         srcAllocOffset, dstAllocOffset,
                                         srcAllocSize};
          moves.push_back(move);
        }
      }
    }
  }

  m_BlockInfos.clear();

  PostprocessMetadata();

  return DkResult_Success;
}

void DmaDefragmentationAlgorithm_Fast::PreprocessMetadata() {
  const size_t blockCount = m_pBlockVector->GetBlockCount();
  for (size_t blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
    DmaBlockMetadata_Generic *const pMetadata =
        (DmaBlockMetadata_Generic *)m_pBlockVector->GetBlock(blockIndex)
            ->m_pMetadata;
    pMetadata->m_FreeCount = 0;
    pMetadata->m_SumFreeSize = pMetadata->GetSize();
    pMetadata->m_FreeSuballocationsBySize.clear();
    for (DmaSuballocationList::iterator it =
             pMetadata->m_Suballocations.begin();
         it != pMetadata->m_Suballocations.end();) {
      if (it->type == DMA_SUBALLOCATION_TYPE_FREE) {
        DmaSuballocationList::iterator nextIt = it;
        ++nextIt;
        pMetadata->m_Suballocations.erase(it);
        it = nextIt;
      } else {
        ++it;
      }
    }
  }
}

void DmaDefragmentationAlgorithm_Fast::PostprocessMetadata() {
  const size_t blockCount = m_pBlockVector->GetBlockCount();
  for (size_t blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
    DmaBlockMetadata_Generic *const pMetadata =
        (DmaBlockMetadata_Generic *)m_pBlockVector->GetBlock(blockIndex)
            ->m_pMetadata;
    const uint32_t blockSize = pMetadata->GetSize();

    // No allocations in this block - entire area is free.
    if (pMetadata->m_Suballocations.empty()) {
      pMetadata->m_FreeCount = 1;
      // pMetadata->m_SumFreeSize is already set to blockSize.
      DmaSuballocation suballoc = {0,         // offset
                                   blockSize, // size
                                   DMA_NULL,  // hAllocation
                                   DMA_SUBALLOCATION_TYPE_FREE};
      pMetadata->m_Suballocations.push_back(suballoc);
      pMetadata->RegisterFreeSuballocation(pMetadata->m_Suballocations.begin());
    }
    // There are some allocations in this block.
    else {
      uint32_t offset = 0;
      DmaSuballocationList::iterator it;
      for (it = pMetadata->m_Suballocations.begin();
           it != pMetadata->m_Suballocations.end(); ++it) {
        DMA_ASSERT(it->type != DMA_SUBALLOCATION_TYPE_FREE);
        DMA_ASSERT(it->offset >= offset);

        // Need to insert preceding free space.
        if (it->offset > offset) {
          ++pMetadata->m_FreeCount;
          const uint32_t freeSize = it->offset - offset;
          DmaSuballocation suballoc = {offset,   // offset
                                       freeSize, // size
                                       DMA_NULL, // hAllocation
                                       DMA_SUBALLOCATION_TYPE_FREE};
          DmaSuballocationList::iterator precedingFreeIt =
              pMetadata->m_Suballocations.insert(it, suballoc);
          if (freeSize >= DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER) {
            pMetadata->m_FreeSuballocationsBySize.push_back(precedingFreeIt);
          }
        }

        pMetadata->m_SumFreeSize -= it->size;
        offset = it->offset + it->size;
      }

      // Need to insert trailing free space.
      if (offset < blockSize) {
        ++pMetadata->m_FreeCount;
        const uint32_t freeSize = blockSize - offset;
        DmaSuballocation suballoc = {offset,   // offset
                                     freeSize, // size
                                     DMA_NULL, // hAllocation
                                     DMA_SUBALLOCATION_TYPE_FREE};
        DMA_ASSERT(it == pMetadata->m_Suballocations.end());
        DmaSuballocationList::iterator trailingFreeIt =
            pMetadata->m_Suballocations.insert(it, suballoc);
        if (freeSize > DMA_MIN_FREE_SUBALLOCATION_SIZE_TO_REGISTER) {
          pMetadata->m_FreeSuballocationsBySize.push_back(trailingFreeIt);
        }
      }

      DMA_SORT(pMetadata->m_FreeSuballocationsBySize.begin(),
               pMetadata->m_FreeSuballocationsBySize.end(),
               DmaSuballocationItemSizeLess());
    }

    DMA_HEAVY_ASSERT(pMetadata->Validate());
  }
}

void DmaDefragmentationAlgorithm_Fast::InsertSuballoc(
    DmaBlockMetadata_Generic *pMetadata, const DmaSuballocation &suballoc) {
  // TODO: Optimize somehow. Remember iterator instead of searching for it
  // linearly.
  DmaSuballocationList::iterator it = pMetadata->m_Suballocations.begin();
  while (it != pMetadata->m_Suballocations.end()) {
    if (it->offset < suballoc.offset) {
      ++it;
    }
  }
  pMetadata->m_Suballocations.insert(it, suballoc);
}

////////////////////////////////////////////////////////////////////////////////
// DmaBlockVectorDefragmentationContext

DmaBlockVectorDefragmentationContext::DmaBlockVectorDefragmentationContext(
    DmaAllocator hAllocator, DmaPool hCustomPool, DmaBlockVector *pBlockVector,
    uint32_t currFrameIndex)
    : res(DkResult_Success), mutexLocked(false),
      blockContexts(DmaStlAllocator<DmaBlockDefragmentationContext>(
          hAllocator->GetAllocationCallbacks())),
      m_hAllocator(hAllocator), m_hCustomPool(hCustomPool),
      m_pBlockVector(pBlockVector), m_CurrFrameIndex(currFrameIndex),
      m_pAlgorithm(DMA_NULL), m_Allocations(DmaStlAllocator<AllocInfo>(
                                  hAllocator->GetAllocationCallbacks())),
      m_AllAllocations(false) {}

DmaBlockVectorDefragmentationContext::~DmaBlockVectorDefragmentationContext() {
  dma_delete(m_hAllocator, m_pAlgorithm);
}

void DmaBlockVectorDefragmentationContext::AddAllocation(DmaAllocation hAlloc,
                                                         bool *pChanged) {
  AllocInfo info = {hAlloc, pChanged};
  m_Allocations.push_back(info);
}

void DmaBlockVectorDefragmentationContext::Begin(
    bool overlappingMoveSupported) {
  const bool allAllocations =
      m_AllAllocations ||
      m_Allocations.size() == m_pBlockVector->CalcAllocationCount();

  /********************************
  HERE IS THE CHOICE OF DEFRAGMENTATION ALGORITHM.
  ********************************/

  /*
  Fast algorithm is supported only when certain criteria are met:
  - DMA_DEBUG_MARGIN is 0.
  - All allocations in this block vector are moveable.
  - There is no possibility of image/buffer granularity conflict.
  */
  if (DMA_DEBUG_MARGIN == 0 && allAllocations) {
    m_pAlgorithm = dma_new(m_hAllocator, DmaDefragmentationAlgorithm_Fast)(
        m_hAllocator, m_pBlockVector, m_CurrFrameIndex,
        overlappingMoveSupported);
  } else {
    m_pAlgorithm = dma_new(m_hAllocator, DmaDefragmentationAlgorithm_Generic)(
        m_hAllocator, m_pBlockVector, m_CurrFrameIndex,
        overlappingMoveSupported);
  }

  if (allAllocations) {
    m_pAlgorithm->AddAll();
  } else {
    for (size_t i = 0, count = m_Allocations.size(); i < count; ++i) {
      m_pAlgorithm->AddAllocation(m_Allocations[i].hAlloc,
                                  m_Allocations[i].pChanged);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// DmaDefragmentationContext

DmaDefragmentationContext_T::DmaDefragmentationContext_T(
    DmaAllocator hAllocator, uint32_t currFrameIndex, uint32_t flags,
    DmaDefragmentationStats *pStats)
    : m_hAllocator(hAllocator), m_CurrFrameIndex(currFrameIndex),
      m_Flags(flags), m_pStats(pStats),
      m_CustomPoolContexts(
          DmaStlAllocator<DmaBlockVectorDefragmentationContext *>(
              hAllocator->GetAllocationCallbacks())) {
  memset(m_DefaultPoolContexts, 0, sizeof(m_DefaultPoolContexts));
}

DmaDefragmentationContext_T::~DmaDefragmentationContext_T() {
  for (size_t i = m_CustomPoolContexts.size(); i--;) {
    DmaBlockVectorDefragmentationContext *pBlockVectorCtx =
        m_CustomPoolContexts[i];
    pBlockVectorCtx->GetBlockVector()->DefragmentationEnd(pBlockVectorCtx,
                                                          m_pStats);
    dma_delete(m_hAllocator, pBlockVectorCtx);
  }
  for (size_t i = DMA_NUM_MEMORY_TYPES; i--;) {
    DmaBlockVectorDefragmentationContext *pBlockVectorCtx =
        m_DefaultPoolContexts[i];
    if (pBlockVectorCtx) {
      pBlockVectorCtx->GetBlockVector()->DefragmentationEnd(pBlockVectorCtx,
                                                            m_pStats);
      dma_delete(m_hAllocator, pBlockVectorCtx);
    }
  }
}

void DmaDefragmentationContext_T::AddPools(uint32_t poolCount,
                                           DmaPool *pPools) {
  for (uint32_t poolIndex = 0; poolIndex < poolCount; ++poolIndex) {
    DmaPool pool = pPools[poolIndex];
    DMA_ASSERT(pool);
    // Pools with algorithm other than default are not defragmented.
    if (pool->m_BlockVector.GetAlgorithm() == 0) {
      DmaBlockVectorDefragmentationContext *pBlockVectorDefragCtx = DMA_NULL;

      for (size_t i = m_CustomPoolContexts.size(); i--;) {
        if (m_CustomPoolContexts[i]->GetCustomPool() == pool) {
          pBlockVectorDefragCtx = m_CustomPoolContexts[i];
          break;
        }
      }

      if (!pBlockVectorDefragCtx) {
        pBlockVectorDefragCtx =
            dma_new(m_hAllocator, DmaBlockVectorDefragmentationContext)(
                m_hAllocator, pool, &pool->m_BlockVector, m_CurrFrameIndex);
        m_CustomPoolContexts.push_back(pBlockVectorDefragCtx);
      }

      pBlockVectorDefragCtx->AddAll();
    }
  }
}

void DmaDefragmentationContext_T::AddAllocations(uint32_t allocationCount,
                                                 DmaAllocation *pAllocations,
                                                 bool *pAllocationsChanged) {
  // Dispatch pAllocations among defragmentators. Create them when necessary.
  for (uint32_t allocIndex = 0; allocIndex < allocationCount; ++allocIndex) {
    DmaAllocation hAlloc = pAllocations[allocIndex];
    DMA_ASSERT(hAlloc);
    // DedicatedAlloc cannot be defragmented.
    if ((hAlloc->GetType() == DmaAllocation_T::ALLOCATION_TYPE_BLOCK) &&
        // Lost allocation cannot be defragmented.
        (hAlloc->GetLastUseFrameIndex() != DMA_FRAME_INDEX_LOST)) {
      DmaBlockVectorDefragmentationContext *pBlockVectorDefragCtx = DMA_NULL;

      const DmaPool hAllocPool = hAlloc->GetBlock()->GetParentPool();
      // This allocation belongs to custom pool.
      if (hAllocPool != DMA_NULL_HANDLE) {
        // Pools with algorithm other than default are not defragmented.
        if (hAllocPool->m_BlockVector.GetAlgorithm() == 0) {
          for (size_t i = m_CustomPoolContexts.size(); i--;) {
            if (m_CustomPoolContexts[i]->GetCustomPool() == hAllocPool) {
              pBlockVectorDefragCtx = m_CustomPoolContexts[i];
              break;
            }
          }
          if (!pBlockVectorDefragCtx) {
            pBlockVectorDefragCtx =
                dma_new(m_hAllocator, DmaBlockVectorDefragmentationContext)(
                    m_hAllocator, hAllocPool, &hAllocPool->m_BlockVector,
                    m_CurrFrameIndex);
            m_CustomPoolContexts.push_back(pBlockVectorDefragCtx);
          }
        }
      }
      // This allocation belongs to default pool.
      else {
        const uint32_t memTypeIndex = hAlloc->GetMemoryTypeIndex();
        pBlockVectorDefragCtx = m_DefaultPoolContexts[memTypeIndex];
        if (!pBlockVectorDefragCtx) {
          pBlockVectorDefragCtx = dma_new(m_hAllocator,
                                          DmaBlockVectorDefragmentationContext)(
              m_hAllocator,
              DMA_NULL, // hCustomPool
              m_hAllocator->m_pBlockVectors[memTypeIndex], m_CurrFrameIndex);
          m_DefaultPoolContexts[memTypeIndex] = pBlockVectorDefragCtx;
        }
      }

      if (pBlockVectorDefragCtx) {
        bool *const pChanged = (pAllocationsChanged != DMA_NULL)
                                   ? &pAllocationsChanged[allocIndex]
                                   : DMA_NULL;
        pBlockVectorDefragCtx->AddAllocation(hAlloc, pChanged);
      }
    }
  }
}

DkResult DmaDefragmentationContext_T::Defragment(
    uint32_t maxCpuBytesToMove, uint32_t maxCpuAllocationsToMove,
    uint32_t maxGpuBytesToMove, uint32_t maxGpuAllocationsToMove,
    DkCmdBuf commandBuffer, DmaDefragmentationStats *pStats) {
  if (pStats) {
    memset(pStats, 0, sizeof(DmaDefragmentationStats));
  }

  if (commandBuffer == DMA_NULL_HANDLE) {
    maxGpuBytesToMove = 0;
    maxGpuAllocationsToMove = 0;
  }

  DkResult res = DkResult_Success;

  // Process default pools.
  for (uint32_t memTypeIndex = 0;
       memTypeIndex < DMA_NUM_MEMORY_TYPES && res >= DkResult_Success;
       ++memTypeIndex) {
    DmaBlockVectorDefragmentationContext *pBlockVectorCtx =
        m_DefaultPoolContexts[memTypeIndex];
    if (pBlockVectorCtx) {
      DMA_ASSERT(pBlockVectorCtx->GetBlockVector());
      pBlockVectorCtx->GetBlockVector()->Defragment(
          pBlockVectorCtx, pStats, maxCpuBytesToMove, maxCpuAllocationsToMove,
          maxGpuBytesToMove, maxGpuAllocationsToMove, commandBuffer);
      if (pBlockVectorCtx->res != DkResult_Success) {
        res = pBlockVectorCtx->res;
      }
    }
  }

  // Process custom pools.
  for (size_t customCtxIndex = 0, customCtxCount = m_CustomPoolContexts.size();
       customCtxIndex < customCtxCount && res >= DkResult_Success;
       ++customCtxIndex) {
    DmaBlockVectorDefragmentationContext *pBlockVectorCtx =
        m_CustomPoolContexts[customCtxIndex];
    DMA_ASSERT(pBlockVectorCtx && pBlockVectorCtx->GetBlockVector());
    pBlockVectorCtx->GetBlockVector()->Defragment(
        pBlockVectorCtx, pStats, maxCpuBytesToMove, maxCpuAllocationsToMove,
        maxGpuBytesToMove, maxGpuAllocationsToMove, commandBuffer);
    if (pBlockVectorCtx->res != DkResult_Success) {
      res = pBlockVectorCtx->res;
    }
  }

  return res;
}

////////////////////////////////////////////////////////////////////////////////
// DmaRecorder

#if DMA_RECORDING_ENABLED

DmaRecorder::DmaRecorder()
    : m_UseMutex(true), m_Flags(0), m_File(DMA_NULL), m_Freq(INT64_MAX),
      m_StartCounter(INT64_MAX) {}

DkResult DmaRecorder::Init(const DmaRecordSettings &settings, bool useMutex) {
  m_UseMutex = useMutex;
  m_Flags = settings.flags;

  QueryPerformanceFrequency((LARGE_INTEGER *)&m_Freq);
  QueryPerformanceCounter((LARGE_INTEGER *)&m_StartCounter);

  // Open file for writing.
  errno_t err = fopen_s(&m_File, settings.pFilePath, "wb");
  if (err != 0) {
    return DkResult_Fail;
  }

  // Write header.
  fprintf(m_File, "%s\n", "Vulkan Memory Allocator,Calls recording");
  fprintf(m_File, "%s\n", "1,8");

  return DkResult_Success;
}

DmaRecorder::~DmaRecorder() {
  if (m_File != DMA_NULL) {
    fclose(m_File);
  }
}

void DmaRecorder::RecordCreateAllocator(uint32_t frameIndex) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaCreateAllocator\n", callParams.threadId,
          callParams.time, frameIndex);
  Flush();
}

void DmaRecorder::RecordDestroyAllocator(uint32_t frameIndex) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaDestroyAllocator\n", callParams.threadId,
          callParams.time, frameIndex);
  Flush();
}

void DmaRecorder::RecordCreatePool(uint32_t frameIndex,
                                   const DmaPoolCreateInfo &createInfo,
                                   DmaPool pool) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaCreatePool,%u,%u,%llu,%llu,%llu,%u,%p\n",
          callParams.threadId, callParams.time, frameIndex,
          createInfo.memoryTypeIndex, createInfo.flags, createInfo.blockSize,
          (uint64_t)createInfo.minBlockCount,
          (uint64_t)createInfo.maxBlockCount, createInfo.frameInUseCount, pool);
  Flush();
}

void DmaRecorder::RecordDestroyPool(uint32_t frameIndex, DmaPool pool) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaDestroyPool,%p\n", callParams.threadId,
          callParams.time, frameIndex, pool);
  Flush();
}

void DmaRecorder::RecordAllocateMemory(
    uint32_t frameIndex, const DmaMemoryRequirements &vkMemReq,
    const DmaAllocationCreateInfo &createInfo, DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  UserDataString userDataStr(createInfo.flags, createInfo.pUserData);
  fprintf(m_File,
          "%u,%.3f,%u,dmaAllocateMemory,%llu,%llu,%u,%u,%u,%u,%u,%u,%p,%p,%s\n",
          callParams.threadId, callParams.time, frameIndex, vkMemReq.size,
          vkMemReq.alignment, vkMemReq.memoryTypeBits, createInfo.flags,
          createInfo.usage, createInfo.requiredFlags, createInfo.preferredFlags,
          createInfo.memoryTypeBits, createInfo.pool, allocation,
          userDataStr.GetString());
  Flush();
}

void DmaRecorder::RecordAllocateMemoryPages(
    uint32_t frameIndex, const DmaMemoryRequirements &vkMemReq,
    const DmaAllocationCreateInfo &createInfo, uint64_t allocationCount,
    const DmaAllocation *pAllocations) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  UserDataString userDataStr(createInfo.flags, createInfo.pUserData);
  fprintf(m_File,
          "%u,%.3f,%u,dmaAllocateMemoryPages,%llu,%llu,%u,%u,%u,%u,%u,%u,%p,",
          callParams.threadId, callParams.time, frameIndex, vkMemReq.size,
          vkMemReq.alignment, vkMemReq.memoryTypeBits, createInfo.flags,
          createInfo.usage, createInfo.requiredFlags, createInfo.preferredFlags,
          createInfo.memoryTypeBits, createInfo.pool);
  PrintPointerList(allocationCount, pAllocations);
  fprintf(m_File, ",%s\n", userDataStr.GetString());
  Flush();
}

void DmaRecorder::RecordAllocateMemoryForBuffer(
    uint32_t frameIndex, const DmaMemoryRequirements &vkMemReq,
    bool requiresDedicatedAllocation, bool prefersDedicatedAllocation,
    const DmaAllocationCreateInfo &createInfo, DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  UserDataString userDataStr(createInfo.flags, createInfo.pUserData);
  fprintf(m_File,
          "%u,%.3f,%u,dmaAllocateMemoryForBuffer,%llu,%llu,%u,%u,%u,%u,%u,%u,%"
          "u,%u,%p,%p,%s\n",
          callParams.threadId, callParams.time, frameIndex, vkMemReq.size,
          vkMemReq.alignment, vkMemReq.memoryTypeBits,
          requiresDedicatedAllocation ? 1 : 0,
          prefersDedicatedAllocation ? 1 : 0, createInfo.flags,
          createInfo.usage, createInfo.requiredFlags, createInfo.preferredFlags,
          createInfo.memoryTypeBits, createInfo.pool, allocation,
          userDataStr.GetString());
  Flush();
}

void DmaRecorder::RecordAllocateMemoryForImage(
    uint32_t frameIndex, const DmaMemoryRequirements &vkMemReq,
    bool requiresDedicatedAllocation, bool prefersDedicatedAllocation,
    const DmaAllocationCreateInfo &createInfo, DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  UserDataString userDataStr(createInfo.flags, createInfo.pUserData);
  fprintf(m_File,
          "%u,%.3f,%u,dmaAllocateMemoryForImage,%llu,%llu,%u,%u,%u,%u,%u,%u,%u,"
          "%u,%p,%p,%s\n",
          callParams.threadId, callParams.time, frameIndex, vkMemReq.size,
          vkMemReq.alignment, vkMemReq.memoryTypeBits,
          requiresDedicatedAllocation ? 1 : 0,
          prefersDedicatedAllocation ? 1 : 0, createInfo.flags,
          createInfo.usage, createInfo.requiredFlags, createInfo.preferredFlags,
          createInfo.memoryTypeBits, createInfo.pool, allocation,
          userDataStr.GetString());
  Flush();
}

void DmaRecorder::RecordFreeMemory(uint32_t frameIndex,
                                   DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaFreeMemory,%p\n", callParams.threadId,
          callParams.time, frameIndex, allocation);
  Flush();
}

void DmaRecorder::RecordFreeMemoryPages(uint32_t frameIndex,
                                        uint64_t allocationCount,
                                        const DmaAllocation *pAllocations) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaFreeMemoryPages,", callParams.threadId,
          callParams.time, frameIndex);
  PrintPointerList(allocationCount, pAllocations);
  fprintf(m_File, "\n");
  Flush();
}

void DmaRecorder::RecordSetAllocationUserData(uint32_t frameIndex,
                                              DmaAllocation allocation,
                                              const void *pUserData) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  UserDataString userDataStr(
      allocation->IsUserDataString()
          ? DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT
          : 0,
      pUserData);
  fprintf(m_File, "%u,%.3f,%u,dmaSetAllocationUserData,%p,%s\n",
          callParams.threadId, callParams.time, frameIndex, allocation,
          userDataStr.GetString());
  Flush();
}

void DmaRecorder::RecordCreateLostAllocation(uint32_t frameIndex,
                                             DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaCreateLostAllocation,%p\n",
          callParams.threadId, callParams.time, frameIndex, allocation);
  Flush();
}

void DmaRecorder::RecordMapMemory(uint32_t frameIndex,
                                  DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaMapMemory,%p\n", callParams.threadId,
          callParams.time, frameIndex, allocation);
  Flush();
}

void DmaRecorder::RecordUnmapMemory(uint32_t frameIndex,
                                    DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaUnmapMemory,%p\n", callParams.threadId,
          callParams.time, frameIndex, allocation);
  Flush();
}

void DmaRecorder::RecordFlushAllocation(uint32_t frameIndex,
                                        DmaAllocation allocation,
                                        uint32_t offset, uint32_t size) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaFlushAllocation,%p,%llu,%llu\n",
          callParams.threadId, callParams.time, frameIndex, allocation, offset,
          size);
  Flush();
}

void DmaRecorder::RecordInvalidateAllocation(uint32_t frameIndex,
                                             DmaAllocation allocation,
                                             uint32_t offset, uint32_t size) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaInvalidateAllocation,%p,%llu,%llu\n",
          callParams.threadId, callParams.time, frameIndex, allocation, offset,
          size);
  Flush();
}

void DmaRecorder::RecordCreateBuffer(
    uint32_t frameIndex, const VkBufferCreateInfo &bufCreateInfo,
    const DmaAllocationCreateInfo &allocCreateInfo, DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  UserDataString userDataStr(allocCreateInfo.flags, allocCreateInfo.pUserData);
  fprintf(m_File,
          "%u,%.3f,%u,dmaCreateBuffer,%u,%llu,%u,%u,%u,%u,%u,%u,%u,%p,%p,%s\n",
          callParams.threadId, callParams.time, frameIndex, bufCreateInfo.flags,
          bufCreateInfo.size, bufCreateInfo.usage, bufCreateInfo.sharingMode,
          allocCreateInfo.flags, allocCreateInfo.usage,
          allocCreateInfo.requiredFlags, allocCreateInfo.preferredFlags,
          allocCreateInfo.memoryTypeBits, allocCreateInfo.pool, allocation,
          userDataStr.GetString());
  Flush();
}

void DmaRecorder::RecordCreateImage(
    uint32_t frameIndex, const VkImageCreateInfo &imageCreateInfo,
    const DmaAllocationCreateInfo &allocCreateInfo, DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  UserDataString userDataStr(allocCreateInfo.flags, allocCreateInfo.pUserData);
  fprintf(m_File,
          "%u,%.3f,%u,dmaCreateImage,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
          "%u,%u,%u,%u,%p,%p,%s\n",
          callParams.threadId, callParams.time, frameIndex,
          imageCreateInfo.flags, imageCreateInfo.imageType,
          imageCreateInfo.format, imageCreateInfo.extent.width,
          imageCreateInfo.extent.height, imageCreateInfo.extent.depth,
          imageCreateInfo.mipLevels, imageCreateInfo.arrayLayers,
          imageCreateInfo.samples, imageCreateInfo.tiling,
          imageCreateInfo.usage, imageCreateInfo.sharingMode,
          imageCreateInfo.initialLayout, allocCreateInfo.flags,
          allocCreateInfo.usage, allocCreateInfo.requiredFlags,
          allocCreateInfo.preferredFlags, allocCreateInfo.memoryTypeBits,
          allocCreateInfo.pool, allocation, userDataStr.GetString());
  Flush();
}

void DmaRecorder::RecordDestroyBuffer(uint32_t frameIndex,
                                      DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaDestroyBuffer,%p\n", callParams.threadId,
          callParams.time, frameIndex, allocation);
  Flush();
}

void DmaRecorder::RecordDestroyImage(uint32_t frameIndex,
                                     DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaDestroyImage,%p\n", callParams.threadId,
          callParams.time, frameIndex, allocation);
  Flush();
}

void DmaRecorder::RecordTouchAllocation(uint32_t frameIndex,
                                        DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaTouchAllocation,%p\n", callParams.threadId,
          callParams.time, frameIndex, allocation);
  Flush();
}

void DmaRecorder::RecordGetAllocationInfo(uint32_t frameIndex,
                                          DmaAllocation allocation) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaGetAllocationInfo,%p\n", callParams.threadId,
          callParams.time, frameIndex, allocation);
  Flush();
}

void DmaRecorder::RecordMakePoolAllocationsLost(uint32_t frameIndex,
                                                DmaPool pool) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaMakePoolAllocationsLost,%p\n",
          callParams.threadId, callParams.time, frameIndex, pool);
  Flush();
}

void DmaRecorder::RecordDefragmentationBegin(
    uint32_t frameIndex, const DmaDefragmentationInfo2 &info,
    DmaDefragmentationContext ctx) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaDefragmentationBegin,%u,", callParams.threadId,
          callParams.time, frameIndex, info.flags);
  PrintPointerList(info.allocationCount, info.pAllocations);
  fprintf(m_File, ",");
  PrintPointerList(info.poolCount, info.pPools);
  fprintf(m_File, ",%llu,%u,%llu,%u,%p,%p\n", info.maxCpuBytesToMove,
          info.maxCpuAllocationsToMove, info.maxGpuBytesToMove,
          info.maxGpuAllocationsToMove, info.commandBuffer, ctx);
  Flush();
}

void DmaRecorder::RecordDefragmentationEnd(uint32_t frameIndex,
                                           DmaDefragmentationContext ctx) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaDefragmentationEnd,%p\n", callParams.threadId,
          callParams.time, frameIndex, ctx);
  Flush();
}

void DmaRecorder::RecordSetPoolName(uint32_t frameIndex, DmaPool pool,
                                    const char *name) {
  CallParams callParams;
  GetBasicParams(callParams);

  DmaMutexLock lock(m_FileMutex, m_UseMutex);
  fprintf(m_File, "%u,%.3f,%u,dmaSetPoolName,%p,%s\n", callParams.threadId,
          callParams.time, frameIndex, pool, name != DMA_NULL ? name : "");
  Flush();
}

DmaRecorder::UserDataString::UserDataString(DmaAllocationCreateFlags allocFlags,
                                            const void *pUserData) {
  if (pUserData != DMA_NULL) {
    if ((allocFlags & DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT) != 0) {
      m_Str = (const char *)pUserData;
    } else {
      sprintf_s(m_PtrStr, "%p", pUserData);
      m_Str = m_PtrStr;
    }
  } else {
    m_Str = "";
  }
}

void DmaRecorder::WriteConfiguration(
    const VkPhysicalDeviceProperties &devProps,
    const VkPhysicalDeviceMemoryProperties &memProps, uint32_t vulkanApiVersion,
    bool dedicatedAllocationExtensionEnabled, bool bindMemory2ExtensionEnabled,
    bool memoryBudgetExtensionEnabled) {
  fprintf(m_File, "Config,Begin\n");

  fprintf(m_File, "VulkanApiVersion,%u,%u\n",
          VK_VERSION_MAJOR(vulkanApiVersion),
          VK_VERSION_MINOR(vulkanApiVersion));

  fprintf(m_File, "PhysicalDevice,apiVersion,%u\n", devProps.apiVersion);
  fprintf(m_File, "PhysicalDevice,driverVersion,%u\n", devProps.driverVersion);
  fprintf(m_File, "PhysicalDevice,vendorID,%u\n", devProps.vendorID);
  fprintf(m_File, "PhysicalDevice,deviceID,%u\n", devProps.deviceID);
  fprintf(m_File, "PhysicalDevice,deviceType,%u\n", devProps.deviceType);
  fprintf(m_File, "PhysicalDevice,deviceName,%s\n", devProps.deviceName);

  fprintf(m_File, "PhysicalDeviceLimits,maxMemoryAllocationCount,%u\n",
          devProps.limits.maxMemoryAllocationCount);
  fprintf(m_File, "PhysicalDeviceLimits,bufferImageGranularity,%llu\n",
          devProps.limits.bufferImageGranularity);
  fprintf(m_File, "PhysicalDeviceLimits,nonCoherentAtomSize,%llu\n",
          devProps.limits.nonCoherentAtomSize);

  fprintf(m_File, "PhysicalDeviceMemory,HeapCount,%u\n",
          memProps.memoryHeapCount);
  for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
    fprintf(m_File, "PhysicalDeviceMemory,Heap,%u,size,%llu\n", i,
            memProps.memoryHeaps[i].size);
    fprintf(m_File, "PhysicalDeviceMemory,Heap,%u,flags,%u\n", i,
            memProps.memoryHeaps[i].flags);
  }
  fprintf(m_File, "PhysicalDeviceMemory,TypeCount,%u\n",
          memProps.memoryTypeCount);
  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    fprintf(m_File, "PhysicalDeviceMemory,Type,%u,heapIndex,%u\n", i,
            memProps.memoryTypes[i].heapIndex);
    fprintf(m_File, "PhysicalDeviceMemory,Type,%u,propertyFlags,%u\n", i,
            memProps.memoryTypes[i].propertyFlags);
  }

  fprintf(m_File, "Extension,VK_KHR_dedicated_allocation,%u\n",
          dedicatedAllocationExtensionEnabled ? 1 : 0);
  fprintf(m_File, "Extension,VK_KHR_bind_memory2,%u\n",
          bindMemory2ExtensionEnabled ? 1 : 0);
  fprintf(m_File, "Extension,VK_EXT_memory_budget,%u\n",
          memoryBudgetExtensionEnabled ? 1 : 0);

  fprintf(m_File, "Macro,DMA_DEBUG_ALWAYS_DEDICATED_MEMORY,%u\n",
          DMA_DEBUG_ALWAYS_DEDICATED_MEMORY ? 1 : 0);
  fprintf(m_File, "Macro,DMA_DEBUG_ALIGNMENT,%llu\n",
          (uint32_t)DMA_DEBUG_ALIGNMENT);
  fprintf(m_File, "Macro,DMA_DEBUG_MARGIN,%llu\n", (uint32_t)DMA_DEBUG_MARGIN);
  fprintf(m_File, "Macro,DMA_DEBUG_INITIALIZE_ALLOCATIONS,%u\n",
          DMA_DEBUG_INITIALIZE_ALLOCATIONS ? 1 : 0);
  fprintf(m_File, "Macro,DMA_DEBUG_DETECT_CORRUPTION,%u\n",
          DMA_DEBUG_DETECT_CORRUPTION ? 1 : 0);
  fprintf(m_File, "Macro,DMA_DEBUG_GLOBAL_MUTEX,%u\n",
          DMA_DEBUG_GLOBAL_MUTEX ? 1 : 0);
  fprintf(m_File, "Macro,DMA_DEBUG_MIN_BUFFER_IMAGE_GRANULARITY,%llu\n",
          (uint32_t)DMA_DEBUG_MIN_BUFFER_IMAGE_GRANULARITY);
  fprintf(m_File, "Macro,DMA_SMALL_HEAP_MAX_SIZE,%llu\n",
          (uint32_t)DMA_SMALL_HEAP_MAX_SIZE);
  fprintf(m_File, "Macro,DMA_DEFAULT_LARGE_HEAP_BLOCK_SIZE,%llu\n",
          (uint32_t)DMA_DEFAULT_LARGE_HEAP_BLOCK_SIZE);

  fprintf(m_File, "Config,End\n");
}

void DmaRecorder::GetBasicParams(CallParams &outParams) {
  outParams.threadId = GetCurrentThreadId();

  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  outParams.time = (double)(counter.QuadPart - m_StartCounter) / (double)m_Freq;
}

void DmaRecorder::PrintPointerList(uint64_t count,
                                   const DmaAllocation *pItems) {
  if (count) {
    fprintf(m_File, "%p", pItems[0]);
    for (uint64_t i = 1; i < count; ++i) {
      fprintf(m_File, " %p", pItems[i]);
    }
  }
}

void DmaRecorder::Flush() {
  if ((m_Flags & DMA_RECORD_FLUSH_AFTER_CALL_BIT) != 0) {
    fflush(m_File);
  }
}

#endif // #if DMA_RECORDING_ENABLED

////////////////////////////////////////////////////////////////////////////////
// VmaAllocationObjectAllocator

DmaAllocationObjectAllocator::DmaAllocationObjectAllocator(
    const DmaAllocationCallbacks *pAllocationCallbacks)
    : m_Allocator(pAllocationCallbacks, 1024) {}

DmaAllocation DmaAllocationObjectAllocator::Allocate() {
  DmaMutexLock mutexLock(m_Mutex);
  return m_Allocator.Alloc();
}

void DmaAllocationObjectAllocator::Free(DmaAllocation hAlloc) {
  DmaMutexLock mutexLock(m_Mutex);
  m_Allocator.Free(hAlloc);
}

////////////////////////////////////////////////////////////////////////////////
// DmaAllocator_T

DmaAllocator_T::DmaAllocator_T(const DmaAllocatorCreateInfo *pCreateInfo)
    : m_UseMutex((pCreateInfo->flags &
                  DMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT) == 0),
      m_hDevice(pCreateInfo->device),
      m_AllocationCallbacksSpecified(pCreateInfo->pAllocationCallbacks !=
                                     DMA_NULL),
      m_AllocationCallbacks(pCreateInfo->pAllocationCallbacks
                                ? *pCreateInfo->pAllocationCallbacks
                                : DmaEmptyAllocationCallbacks),
      m_AllocationObjectAllocator(&m_AllocationCallbacks),
      m_PreferredLargeHeapBlockSize(0), m_CurrentFrameIndex(0),
      m_Pools(DmaStlAllocator<DmaPool>(GetAllocationCallbacks())),
      m_NextPoolId(0)
#if DMA_RECORDING_ENABLED
      ,
      m_pRecorder(DMA_NULL)
#endif
{
  if (DMA_DEBUG_DETECT_CORRUPTION) {
    // Needs to be multiply of uint32_t size because we are going to write
    // DMA_CORRUPTION_DETECTION_MAGIC_VALUE to it.
    DMA_ASSERT(DMA_DEBUG_MARGIN % sizeof(uint32_t) == 0);
  }

  DMA_ASSERT(pCreateInfo->device);

  memset(&m_DeviceMemoryCallbacks, 0, sizeof(m_DeviceMemoryCallbacks));

  memset(&m_pBlockVectors, 0, sizeof(m_pBlockVectors));
  memset(&m_pDedicatedAllocations, 0, sizeof(m_pDedicatedAllocations));

  if (pCreateInfo->pDeviceMemoryCallbacks != DMA_NULL) {
    m_DeviceMemoryCallbacks.pfnAllocate =
        pCreateInfo->pDeviceMemoryCallbacks->pfnAllocate;
    m_DeviceMemoryCallbacks.pfnFree =
        pCreateInfo->pDeviceMemoryCallbacks->pfnFree;
  }

  DMA_ASSERT(DmaIsPow2(DMA_DEBUG_ALIGNMENT));
  DMA_ASSERT(DmaIsPow2(DMA_DEBUG_MIN_BUFFER_IMAGE_GRANULARITY));

  m_PreferredLargeHeapBlockSize =
      (pCreateInfo->preferredLargeHeapBlockSize != 0)
          ? pCreateInfo->preferredLargeHeapBlockSize
          : static_cast<uint32_t>(DMA_DEFAULT_LARGE_HEAP_BLOCK_SIZE);

  for (uint32_t memTypeIndex = 0; memTypeIndex < DMA_NUM_MEMORY_TYPES;
       ++memTypeIndex) {
    const uint32_t preferredBlockSize =
        DmaMemTypePreferredBlockSize[memTypeIndex];

    m_pBlockVectors[memTypeIndex] =
        dma_new(this, DmaBlockVector)(this,
                                      DMA_NULL_HANDLE, // hParentPool
                                      memTypeIndex, preferredBlockSize, 0,
                                      SIZE_MAX, pCreateInfo->frameInUseCount,
                                      false); // linearAlgorithm
    // No need to call
    // m_pBlockVectors[memTypeIndex][blockVectorTypeIndex]->CreateMinBlocks
    // here, becase minBlockCount is 0.
    m_pDedicatedAllocations[memTypeIndex] = dma_new(this, AllocationVectorType)(
        DmaStlAllocator<DmaAllocation>(GetAllocationCallbacks()));
  }
}

DkResult DmaAllocator_T::Init(const DmaAllocatorCreateInfo *pCreateInfo) {
  DkResult res = DkResult_Success;

  if (pCreateInfo->pRecordSettings != DMA_NULL &&
      !DmaStrIsEmpty(pCreateInfo->pRecordSettings->pFilePath)) {
#if DMA_RECORDING_ENABLED
    m_pRecorder = dma_new(this, DmaRecorder)();
    res = m_pRecorder->Init(*pCreateInfo->pRecordSettings, m_UseMutex);
    if (res != DkResult_Success) {
      return res;
    }
    m_pRecorder->WriteConfiguration(
        m_PhysicalDeviceProperties, m_MemProps, m_VulkanApiVersion,
        m_UseKhrDedicatedAllocation, m_UseKhrBindMemory2, m_UseExtMemoryBudget);
    m_pRecorder->RecordCreateAllocator(GetCurrentFrameIndex());
#else
    DMA_ASSERT(0 && "DmaAllocatorCreateInfo::pRecordSettings used, but not "
                    "supported due to DMA_RECORDING_ENABLED not defined to 1.");
    return DkResult_NotImplemented;
#endif
  }

  return res;
}

DmaAllocator_T::~DmaAllocator_T() {
#if DMA_RECORDING_ENABLED
  if (m_pRecorder != DMA_NULL) {
    m_pRecorder->RecordDestroyAllocator(GetCurrentFrameIndex());
    dma_delete(this, m_pRecorder);
  }
#endif

  DMA_ASSERT(m_Pools.empty());

  for (size_t i = DMA_NUM_MEMORY_TYPES; i--;) {
    if (m_pDedicatedAllocations[i] != DMA_NULL &&
        !m_pDedicatedAllocations[i]->empty()) {
      DMA_ASSERT(0 && "Unfreed dedicated allocations found.");
    }

    dma_delete(this, m_pDedicatedAllocations[i]);
    dma_delete(this, m_pBlockVectors[i]);
  }
}

DkResult DmaAllocator_T::AllocateMemoryOfType(
    uint32_t size, uint32_t alignment, bool dedicatedAllocation,
    const DmaAllocationCreateInfo &createInfo, uint32_t memTypeIndex,
    DmaSuballocationType suballocType, size_t allocationCount,
    DmaAllocation *pAllocations) {
  DMA_ASSERT(pAllocations != DMA_NULL);
  DMA_DEBUG_LOG(
      "  AllocateMemory: MemoryTypeIndex=%u, AllocationCount=%zu, Size=%llu",
      memTypeIndex, allocationCount, size);

  DmaAllocationCreateInfo finalCreateInfo = createInfo;

  // If memory type is not HOST_VISIBLE, disable MAPPED.
  if ((finalCreateInfo.flags & DMA_ALLOCATION_CREATE_MAPPED_BIT) != 0 &&
      !DmaMemTypeMappable[memTypeIndex]) {
    finalCreateInfo.flags &= ~DMA_ALLOCATION_CREATE_MAPPED_BIT;
  }

  DmaBlockVector *const blockVector = m_pBlockVectors[memTypeIndex];
  DMA_ASSERT(blockVector);

  const uint32_t preferredBlockSize = blockVector->GetPreferredBlockSize();
  bool preferDedicatedMemory =
      DMA_DEBUG_ALWAYS_DEDICATED_MEMORY || dedicatedAllocation ||
      // Heuristics: Allocate dedicated memory if requested size if greater than
      // half of preferred block size.
      size > preferredBlockSize / 2;

  if (preferDedicatedMemory &&
      (finalCreateInfo.flags & DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT) == 0 &&
      finalCreateInfo.pool == DMA_NULL_HANDLE) {
    finalCreateInfo.flags |= DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  }

  if ((finalCreateInfo.flags & DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) !=
      0) {
    if ((finalCreateInfo.flags & DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT) !=
        0) {
      return DkResult_OutOfMemory;
    } else {
      return AllocateDedicatedMemory(
          size, suballocType, memTypeIndex,
          (finalCreateInfo.flags & DMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT) !=
              0,
          (finalCreateInfo.flags & DMA_ALLOCATION_CREATE_MAPPED_BIT) != 0,
          (finalCreateInfo.flags &
           DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT) != 0,
          finalCreateInfo.pUserData, allocationCount, pAllocations);
    }
  } else {
    DkResult res = blockVector->Allocate(
        m_CurrentFrameIndex.load(), size, alignment, finalCreateInfo,
        suballocType, allocationCount, pAllocations);
    if (res == DkResult_Success) {
      return res;
    }

    // 5. Try dedicated memory.
    if ((finalCreateInfo.flags & DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT) !=
        0) {
      return DkResult_OutOfMemory;
    } else {
      res = AllocateDedicatedMemory(
          size, suballocType, memTypeIndex,
          (finalCreateInfo.flags & DMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT) !=
              0,
          (finalCreateInfo.flags & DMA_ALLOCATION_CREATE_MAPPED_BIT) != 0,
          (finalCreateInfo.flags &
           DMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT) != 0,
          finalCreateInfo.pUserData, allocationCount, pAllocations);
      if (res == DkResult_Success) {
        // Succeeded: AllocateDedicatedMemory function already filld pMemory,
        // nothing more to do here.
        DMA_DEBUG_LOG("    Allocated as DedicatedMemory");
        return DkResult_Success;
      } else {
        // Everything failed: Return error code.
        DMA_DEBUG_LOG("    vkAllocateMemory FAILED");
        return res;
      }
    }
  }
}

DkResult DmaAllocator_T::AllocateDedicatedMemory(
    uint32_t size, DmaSuballocationType suballocType, uint32_t memTypeIndex,
    bool withinBudget, bool map, bool isUserDataString, void *pUserData,
    size_t allocationCount, DmaAllocation *pAllocations) {
  DMA_ASSERT(allocationCount > 0 && pAllocations);

  size = DmaAlignUp(size, uint32_t(DK_MEMBLOCK_ALIGNMENT));

  if (withinBudget) {
    DmaBudget heapBudget = {};
    GetBudget(&heapBudget, 0, 1);
    if (heapBudget.usage + size * allocationCount > heapBudget.budget) {
      return DkResult_OutOfMemory;
    }
  }

  DmaMemoryAllocateInfo allocInfo = {};
  allocInfo.memoryTypeIndex = memTypeIndex;
  allocInfo.allocationSize = size;

#if DMA_DEDICATED_ALLOCATION || DMA_VULKAN_VERSION >= 1001000
  VkMemoryDedicatedAllocateInfoKHR dedicatedAllocInfo = {
      VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR};
  if (m_UseKhrDedicatedAllocation ||
      m_VulkanApiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
    if (dedicatedBuffer != DMA_NULL_HANDLE) {
      DMA_ASSERT(dedicatedImage == DMA_NULL_HANDLE);
      dedicatedAllocInfo.buffer = dedicatedBuffer;
      allocInfo.pNext = &dedicatedAllocInfo;
    } else if (dedicatedImage != DMA_NULL_HANDLE) {
      dedicatedAllocInfo.image = dedicatedImage;
      allocInfo.pNext = &dedicatedAllocInfo;
    }
  }
#endif // #if DMA_DEDICATED_ALLOCATION || DMA_VULKAN_VERSION >= 1001000

  size_t allocIndex;
  DkResult res = DkResult_Success;
  for (allocIndex = 0; allocIndex < allocationCount; ++allocIndex) {
    res = AllocateDedicatedMemoryPage(size, suballocType, memTypeIndex,
                                      allocInfo, map, isUserDataString,
                                      pUserData, pAllocations + allocIndex);
    if (res != DkResult_Success) {
      break;
    }
  }

  if (res == DkResult_Success) {
    // Register them in m_pDedicatedAllocations.
    {
      DmaMutexLockWrite lock(m_DedicatedAllocationsMutex[memTypeIndex],
                             m_UseMutex);
      AllocationVectorType *pDedicatedAllocations =
          m_pDedicatedAllocations[memTypeIndex];
      DMA_ASSERT(pDedicatedAllocations);
      for (allocIndex = 0; allocIndex < allocationCount; ++allocIndex) {
        DmaVectorInsertSorted<DmaPointerLess>(*pDedicatedAllocations,
                                              pAllocations[allocIndex]);
      }
    }

    DMA_DEBUG_LOG(
        "    Allocated DedicatedMemory Count=%zu, MemoryTypeIndex=#%u",
        allocationCount, memTypeIndex);
  } else {
    // Free all already created allocations.
    while (allocIndex--) {
      DmaAllocation currAlloc = pAllocations[allocIndex];
      DkMemBlock hMemory = currAlloc->GetMemory();

      FreeVulkanMemory(memTypeIndex, currAlloc->GetSize(), hMemory);
      m_Budget.RemoveAllocation(0, currAlloc->GetSize());
      currAlloc->SetUserData(this, DMA_NULL);
      currAlloc->Dtor();
      m_AllocationObjectAllocator.Free(currAlloc);
    }

    memset(pAllocations, 0, sizeof(DmaAllocation) * allocationCount);
  }

  return res;
}

DkResult DmaAllocator_T::AllocateDedicatedMemoryPage(
    uint32_t size, DmaSuballocationType suballocType, uint32_t memTypeIndex,
    const DmaMemoryAllocateInfo &allocInfo, bool map, bool isUserDataString,
    void *pUserData, DmaAllocation *pAllocation) {
  DkMemBlock hMemory = DMA_NULL_HANDLE;
  DkResult res = AllocateVulkanMemory(&allocInfo, &hMemory);
  if (res < 0) {
    DMA_DEBUG_LOG("    vkAllocateMemory FAILED");
    return res;
  }

  void *pMappedData = DMA_NULL;
  if (map) {
    pMappedData = dkMemBlockGetCpuAddr(hMemory);
  }

  *pAllocation = m_AllocationObjectAllocator.Allocate();
  (*pAllocation)->Ctor(m_CurrentFrameIndex.load(), isUserDataString);
  (*pAllocation)
      ->InitDedicatedAllocation(memTypeIndex, hMemory, suballocType,
                                pMappedData, size);
  (*pAllocation)->SetUserData(this, pUserData);
  m_Budget.AddAllocation(0, size);
  if (DMA_DEBUG_INITIALIZE_ALLOCATIONS) {
    FillAllocation(*pAllocation, DMA_ALLOCATION_FILL_PATTERN_CREATED);
  }

  return DkResult_Success;
}

DkResult DmaAllocator_T::AllocateMemory(
    const DmaMemoryRequirements &vkMemReq, bool requiresDedicatedAllocation,
    bool prefersDedicatedAllocation, const DmaAllocationCreateInfo &createInfo,
    DmaSuballocationType suballocType, size_t allocationCount,
    DmaAllocation *pAllocations) {
  memset(pAllocations, 0, sizeof(DmaAllocation) * allocationCount);

  DMA_ASSERT(DmaIsPow2(vkMemReq.alignment));

  if (vkMemReq.size == 0) {
    return DkResult_Fail;
  }
  if ((createInfo.flags & DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) != 0 &&
      (createInfo.flags & DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT) != 0) {
    DMA_ASSERT(0 &&
               "Specifying DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT together "
               "with DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT makes no sense.");
    return DkResult_Fail;
  }
  if ((createInfo.flags & DMA_ALLOCATION_CREATE_MAPPED_BIT) != 0 &&
      (createInfo.flags & DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT) != 0) {
    DMA_ASSERT(0 && "Specifying DMA_ALLOCATION_CREATE_MAPPED_BIT together with "
                    "DMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT is invalid.");
    return DkResult_Fail;
  }
  if (requiresDedicatedAllocation) {
    if ((createInfo.flags & DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT) != 0) {
      DMA_ASSERT(0 && "DMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT specified "
                      "while dedicated allocation is required.");
      return DkResult_OutOfMemory;
    }
    if (createInfo.pool != DMA_NULL_HANDLE) {
      DMA_ASSERT(0 && "Pool specified while dedicated allocation is required.");
      return DkResult_OutOfMemory;
    }
  }
  if ((createInfo.pool != DMA_NULL_HANDLE) &&
      ((createInfo.flags & (DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)) !=
       0)) {
    DMA_ASSERT(0 && "Specifying DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT "
                    "when pool != null is invalid.");
    return DkResult_OutOfMemory;
  }

  if (createInfo.pool != DMA_NULL_HANDLE) {
    const uint32_t alignmentForPool =
        DMA_MAX(vkMemReq.alignment,
                GetMemoryTypeMinAlignment(
                    createInfo.pool->m_BlockVector.GetMemoryTypeIndex()));

    DmaAllocationCreateInfo createInfoForPool = createInfo;
    // If memory type is not HOST_VISIBLE, disable MAPPED.
    if ((createInfoForPool.flags & DMA_ALLOCATION_CREATE_MAPPED_BIT) != 0 &&
        !DmaMemTypeMappable[createInfo.pool->m_BlockVector
                                .GetMemoryTypeIndex()]) {
      createInfoForPool.flags &= ~DMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    return createInfo.pool->m_BlockVector.Allocate(
        m_CurrentFrameIndex.load(), vkMemReq.size, alignmentForPool,
        createInfoForPool, suballocType, allocationCount, pAllocations);
  } else {
    // Bit mask of memory Vulkan types acceptable for this allocation.
    uint32_t memTypeIndex = vkMemReq.memoryTypeIndex;
    uint32_t alignmentForMemType =
        DMA_MAX(vkMemReq.alignment, GetMemoryTypeMinAlignment(memTypeIndex));

    return AllocateMemoryOfType(
        vkMemReq.size, alignmentForMemType,
        requiresDedicatedAllocation || prefersDedicatedAllocation, createInfo,
        memTypeIndex, suballocType, allocationCount, pAllocations);
  }
}

void DmaAllocator_T::FreeMemory(size_t allocationCount,
                                DmaAllocation *pAllocations) {
  DMA_ASSERT(pAllocations);

  for (size_t allocIndex = allocationCount; allocIndex--;) {
    DmaAllocation allocation = pAllocations[allocIndex];

    if (allocation != DMA_NULL_HANDLE) {
      if (TouchAllocation(allocation)) {
        if (DMA_DEBUG_INITIALIZE_ALLOCATIONS) {
          FillAllocation(allocation, DMA_ALLOCATION_FILL_PATTERN_DESTROYED);
        }

        switch (allocation->GetType()) {
        case DmaAllocation_T::ALLOCATION_TYPE_BLOCK: {
          DmaBlockVector *pBlockVector = DMA_NULL;
          DmaPool hPool = allocation->GetBlock()->GetParentPool();
          if (hPool != DMA_NULL_HANDLE) {
            pBlockVector = &hPool->m_BlockVector;
          } else {
            const uint32_t memTypeIndex = allocation->GetMemoryTypeIndex();
            pBlockVector = m_pBlockVectors[memTypeIndex];
          }
          pBlockVector->Free(allocation);
        } break;
        case DmaAllocation_T::ALLOCATION_TYPE_DEDICATED:
          FreeDedicatedMemory(allocation);
          break;
        default:
          DMA_ASSERT(0);
        }
      }

      // Do this regardless of whether the allocation is lost. Lost allocations
      // still account to Budget.AllocationBytes.
      m_Budget.RemoveAllocation(0, allocation->GetSize());
      allocation->SetUserData(this, DMA_NULL);
      allocation->Dtor();
      m_AllocationObjectAllocator.Free(allocation);
    }
  }
}

DkResult DmaAllocator_T::ResizeAllocation(const DmaAllocation alloc,
                                          uint32_t newSize) {
  // This function is deprecated and so it does nothing. It's left for backward
  // compatibility.
  if (newSize == 0 || alloc->GetLastUseFrameIndex() == DMA_FRAME_INDEX_LOST) {
    return DkResult_Fail;
  }
  if (newSize == alloc->GetSize()) {
    return DkResult_Success;
  }
  return DkResult_OutOfMemory;
}

void DmaAllocator_T::CalculateStats(DmaStats *pStats) {
  // Initialize.
  InitStatInfo(pStats->total);
  for (size_t i = 0; i < DMA_NUM_MEMORY_TYPES; ++i)
    InitStatInfo(pStats->memoryType[i]);
  for (size_t i = 0; i < DMA_NUM_MEMORY_HEAPS; ++i)
    InitStatInfo(pStats->memoryHeap[i]);

  // Process default pools.
  for (uint32_t memTypeIndex = 0; memTypeIndex < DMA_NUM_MEMORY_TYPES;
       ++memTypeIndex) {
    DmaBlockVector *const pBlockVector = m_pBlockVectors[memTypeIndex];
    DMA_ASSERT(pBlockVector);
    pBlockVector->AddStats(pStats);
  }

  // Process custom pools.
  {
    DmaMutexLockRead lock(m_PoolsMutex, m_UseMutex);
    for (size_t poolIndex = 0, poolCount = m_Pools.size();
         poolIndex < poolCount; ++poolIndex) {
      m_Pools[poolIndex]->m_BlockVector.AddStats(pStats);
    }
  }

  // Process dedicated allocations.
  for (uint32_t memTypeIndex = 0; memTypeIndex < DMA_NUM_MEMORY_TYPES;
       ++memTypeIndex) {
    DmaMutexLockRead dedicatedAllocationsLock(
        m_DedicatedAllocationsMutex[memTypeIndex], m_UseMutex);
    AllocationVectorType *const pDedicatedAllocVector =
        m_pDedicatedAllocations[memTypeIndex];
    DMA_ASSERT(pDedicatedAllocVector);
    for (size_t allocIndex = 0, allocCount = pDedicatedAllocVector->size();
         allocIndex < allocCount; ++allocIndex) {
      DmaStatInfo allocationStatInfo;
      (*pDedicatedAllocVector)[allocIndex]->DedicatedAllocCalcStatsInfo(
          allocationStatInfo);
      DmaAddStatInfo(pStats->total, allocationStatInfo);
      DmaAddStatInfo(pStats->memoryType[memTypeIndex], allocationStatInfo);
      DmaAddStatInfo(pStats->memoryHeap[0], allocationStatInfo);
    }
  }

  // Postprocess.
  DmaPostprocessCalcStatInfo(pStats->total);
  for (size_t i = 0; i < DMA_NUM_MEMORY_TYPES; ++i)
    DmaPostprocessCalcStatInfo(pStats->memoryType[i]);
  for (size_t i = 0; i < DMA_NUM_MEMORY_HEAPS; ++i)
    DmaPostprocessCalcStatInfo(pStats->memoryHeap[i]);
}

void DmaAllocator_T::GetBudget(DmaBudget *outBudget, uint32_t firstHeap,
                               uint32_t heapCount) {
  {
    for (uint32_t i = 0; i < heapCount; ++i, ++outBudget) {
      const uint32_t heapIndex = firstHeap + i;

      outBudget->blockBytes = m_Budget.m_BlockBytes[heapIndex];
      outBudget->allocationBytes = m_Budget.m_AllocationBytes[heapIndex];

      outBudget->usage = outBudget->blockBytes;
      outBudget->budget = DMA_HEAP_SIZE;
    }
  }
}

static const uint32_t DMA_VENDOR_ID_AMD = 4098;

DkResult
DmaAllocator_T::DefragmentationBegin(const DmaDefragmentationInfo2 &info,
                                     DmaDefragmentationStats *pStats,
                                     DmaDefragmentationContext *pContext) {
  if (info.pAllocationsChanged != DMA_NULL) {
    memset(info.pAllocationsChanged, 0, info.allocationCount * sizeof(bool));
  }

  *pContext = dma_new(this, DmaDefragmentationContext_T)(
      this, m_CurrentFrameIndex.load(), info.flags, pStats);

  (*pContext)->AddPools(info.poolCount, info.pPools);
  (*pContext)->AddAllocations(info.allocationCount, info.pAllocations,
                              info.pAllocationsChanged);

  DkResult res = (*pContext)->Defragment(
      info.maxCpuBytesToMove, info.maxCpuAllocationsToMove,
      info.maxGpuBytesToMove, info.maxGpuAllocationsToMove, info.commandBuffer,
      pStats);

  if (res != DkResult_Fail) {
    dma_delete(this, *pContext);
    *pContext = DMA_NULL;
  }

  return res;
}

DkResult DmaAllocator_T::DefragmentationEnd(DmaDefragmentationContext context) {
  dma_delete(this, context);
  return DkResult_Success;
}

void DmaAllocator_T::GetAllocationInfo(DmaAllocation hAllocation,
                                       DmaAllocationInfo *pAllocationInfo) {
  if (hAllocation->CanBecomeLost()) {
    /*
    Warning: This is a carefully designed algorithm.
    Do not modify unless you really know what you're doing :)
    */
    const uint32_t localCurrFrameIndex = m_CurrentFrameIndex.load();
    uint32_t localLastUseFrameIndex = hAllocation->GetLastUseFrameIndex();
    for (;;) {
      if (localLastUseFrameIndex == DMA_FRAME_INDEX_LOST) {
        pAllocationInfo->memoryType = UINT32_MAX;
        pAllocationInfo->deviceMemory = DMA_NULL_HANDLE;
        pAllocationInfo->offset = 0;
        pAllocationInfo->size = hAllocation->GetSize();
        pAllocationInfo->pMappedData = DMA_NULL;
        pAllocationInfo->pUserData = hAllocation->GetUserData();
        return;
      } else if (localLastUseFrameIndex == localCurrFrameIndex) {
        pAllocationInfo->memoryType = hAllocation->GetMemoryTypeIndex();
        pAllocationInfo->deviceMemory = hAllocation->GetMemory();
        pAllocationInfo->offset = hAllocation->GetOffset();
        pAllocationInfo->size = hAllocation->GetSize();
        pAllocationInfo->pMappedData = DMA_NULL;
        pAllocationInfo->pUserData = hAllocation->GetUserData();
        return;
      } else // Last use time earlier than current time.
      {
        if (hAllocation->CompareExchangeLastUseFrameIndex(
                localLastUseFrameIndex, localCurrFrameIndex)) {
          localLastUseFrameIndex = localCurrFrameIndex;
        }
      }
    }
  } else {
#if DMA_STATS_STRING_ENABLED
    uint32_t localCurrFrameIndex = m_CurrentFrameIndex.load();
    uint32_t localLastUseFrameIndex = hAllocation->GetLastUseFrameIndex();
    for (;;) {
      DMA_ASSERT(localLastUseFrameIndex != DMA_FRAME_INDEX_LOST);
      if (localLastUseFrameIndex == localCurrFrameIndex) {
        break;
      } else // Last use time earlier than current time.
      {
        if (hAllocation->CompareExchangeLastUseFrameIndex(
                localLastUseFrameIndex, localCurrFrameIndex)) {
          localLastUseFrameIndex = localCurrFrameIndex;
        }
      }
    }
#endif

    pAllocationInfo->memoryType = hAllocation->GetMemoryTypeIndex();
    pAllocationInfo->deviceMemory = hAllocation->GetMemory();
    pAllocationInfo->offset = hAllocation->GetOffset();
    pAllocationInfo->size = hAllocation->GetSize();
    pAllocationInfo->pMappedData = hAllocation->GetMappedData();
    pAllocationInfo->pUserData = hAllocation->GetUserData();
  }
}

bool DmaAllocator_T::TouchAllocation(DmaAllocation hAllocation) {
  // This is a stripped-down version of DmaAllocator_T::GetAllocationInfo.
  if (hAllocation->CanBecomeLost()) {
    uint32_t localCurrFrameIndex = m_CurrentFrameIndex.load();
    uint32_t localLastUseFrameIndex = hAllocation->GetLastUseFrameIndex();
    for (;;) {
      if (localLastUseFrameIndex == DMA_FRAME_INDEX_LOST) {
        return false;
      } else if (localLastUseFrameIndex == localCurrFrameIndex) {
        return true;
      } else // Last use time earlier than current time.
      {
        if (hAllocation->CompareExchangeLastUseFrameIndex(
                localLastUseFrameIndex, localCurrFrameIndex)) {
          localLastUseFrameIndex = localCurrFrameIndex;
        }
      }
    }
  } else {
#if DMA_STATS_STRING_ENABLED
    uint32_t localCurrFrameIndex = m_CurrentFrameIndex.load();
    uint32_t localLastUseFrameIndex = hAllocation->GetLastUseFrameIndex();
    for (;;) {
      DMA_ASSERT(localLastUseFrameIndex != DMA_FRAME_INDEX_LOST);
      if (localLastUseFrameIndex == localCurrFrameIndex) {
        break;
      } else // Last use time earlier than current time.
      {
        if (hAllocation->CompareExchangeLastUseFrameIndex(
                localLastUseFrameIndex, localCurrFrameIndex)) {
          localLastUseFrameIndex = localCurrFrameIndex;
        }
      }
    }
#endif

    return true;
  }
}

DkResult DmaAllocator_T::CreatePool(const DmaPoolCreateInfo *pCreateInfo,
                                    DmaPool *pPool) {
  DMA_DEBUG_LOG("  CreatePool: MemoryTypeIndex=%u, flags=%u",
                pCreateInfo->memoryTypeIndex, pCreateInfo->flags);

  DmaPoolCreateInfo newCreateInfo = *pCreateInfo;

  if (newCreateInfo.maxBlockCount == 0) {
    newCreateInfo.maxBlockCount = SIZE_MAX;
  }
  if (newCreateInfo.minBlockCount > newCreateInfo.maxBlockCount) {
    return DkResult_Fail;
  }

  const uint32_t preferredBlockSize =
      DmaMemTypePreferredBlockSize[newCreateInfo.memoryTypeIndex];

  *pPool = dma_new(this, DmaPool_T)(this, newCreateInfo, preferredBlockSize);

  DkResult res = (*pPool)->m_BlockVector.CreateMinBlocks();
  if (res != DkResult_Success) {
    dma_delete(this, *pPool);
    *pPool = DMA_NULL;
    return res;
  }

  // Add to m_Pools.
  {
    DmaMutexLockWrite lock(m_PoolsMutex, m_UseMutex);
    (*pPool)->SetId(m_NextPoolId++);
    DmaVectorInsertSorted<DmaPointerLess>(m_Pools, *pPool);
  }

  return DkResult_Success;
}

void DmaAllocator_T::DestroyPool(DmaPool pool) {
  // Remove from m_Pools.
  {
    DmaMutexLockWrite lock(m_PoolsMutex, m_UseMutex);
    bool success = DmaVectorRemoveSorted<DmaPointerLess>(m_Pools, pool);
    DMA_ASSERT(success && "Pool not found in Allocator.");
  }

  dma_delete(this, pool);
}

void DmaAllocator_T::GetPoolStats(DmaPool pool, DmaPoolStats *pPoolStats) {
  pool->m_BlockVector.GetPoolStats(pPoolStats);
}

void DmaAllocator_T::SetCurrentFrameIndex(uint32_t frameIndex) {
  m_CurrentFrameIndex.store(frameIndex);
}

void DmaAllocator_T::MakePoolAllocationsLost(DmaPool hPool,
                                             size_t *pLostAllocationCount) {
  hPool->m_BlockVector.MakePoolAllocationsLost(m_CurrentFrameIndex.load(),
                                               pLostAllocationCount);
}

DkResult DmaAllocator_T::CheckPoolCorruption(DmaPool hPool) {
  return hPool->m_BlockVector.CheckCorruption();
}

DkResult DmaAllocator_T::CheckCorruption(uint32_t memoryTypeBits) {
  DkResult finalRes = DkResult_NotImplemented;

  // Process default pools.
  for (uint32_t memTypeIndex = 0; memTypeIndex < DMA_NUM_MEMORY_TYPES;
       ++memTypeIndex) {
    if (((1u << memTypeIndex) & memoryTypeBits) != 0) {
      DmaBlockVector *const pBlockVector = m_pBlockVectors[memTypeIndex];
      DMA_ASSERT(pBlockVector);
      DkResult localRes = pBlockVector->CheckCorruption();
      switch (localRes) {
      case DkResult_NotImplemented:
        break;
      case DkResult_Success:
        finalRes = DkResult_Success;
        break;
      default:
        return localRes;
      }
    }
  }

  // Process custom pools.
  {
    DmaMutexLockRead lock(m_PoolsMutex, m_UseMutex);
    for (size_t poolIndex = 0, poolCount = m_Pools.size();
         poolIndex < poolCount; ++poolIndex) {
      if (((1u << m_Pools[poolIndex]->m_BlockVector.GetMemoryTypeIndex()) &
           memoryTypeBits) != 0) {
        DkResult localRes = m_Pools[poolIndex]->m_BlockVector.CheckCorruption();
        switch (localRes) {
        case DkResult_NotImplemented:
          break;
        case DkResult_Success:
          finalRes = DkResult_Success;
          break;
        default:
          return localRes;
        }
      }
    }
  }

  return finalRes;
}

void DmaAllocator_T::CreateLostAllocation(DmaAllocation *pAllocation) {
  *pAllocation = m_AllocationObjectAllocator.Allocate();
  (*pAllocation)->Ctor(DMA_FRAME_INDEX_LOST, false);
  (*pAllocation)->InitLost();
}

DkResult
DmaAllocator_T::AllocateVulkanMemory(const DmaMemoryAllocateInfo *pAllocateInfo,
                                     DkMemBlock *pMemory) {
  DkMemBlockMaker BlockMaker = {
      m_hDevice, pAllocateInfo->allocationSize,
      GetMemoryTypeFlags(pAllocateInfo->memoryTypeIndex),
      pAllocateInfo->storage};

  // VULKAN CALL vkAllocateMemory.
  *pMemory = dkMemBlockCreate(&BlockMaker);

  if (*pMemory) {
    // Informative callback.
    if (m_DeviceMemoryCallbacks.pfnAllocate != DMA_NULL) {
      (*m_DeviceMemoryCallbacks.pfnAllocate)(
          this, pAllocateInfo->memoryTypeIndex, *pMemory,
          pAllocateInfo->allocationSize);
    }
    return DkResult_Success;
  } else {
    m_Budget.m_BlockBytes[0] -= pAllocateInfo->allocationSize;
    return DkResult_Fail;
  }
}

void DmaAllocator_T::FreeVulkanMemory(uint32_t memoryType, uint32_t size,
                                      DkMemBlock hMemory) {
  // Informative callback.
  if (m_DeviceMemoryCallbacks.pfnFree != DMA_NULL) {
    (*m_DeviceMemoryCallbacks.pfnFree)(this, memoryType, hMemory, size);
  }

  // VULKAN CALL vkFreeMemory.
  dkMemBlockDestroy(hMemory);

  m_Budget.m_BlockBytes[0] -= size;
}

DkResult DmaAllocator_T::Map(DmaAllocation hAllocation, void **ppData) {
  if (hAllocation->CanBecomeLost()) {
    return DkResult_Fail;
  }

  switch (hAllocation->GetType()) {
  case DmaAllocation_T::ALLOCATION_TYPE_BLOCK: {
    DmaDeviceMemoryBlock *const pBlock = hAllocation->GetBlock();
    char *pBytes = DMA_NULL;
    DkResult res = pBlock->Map(this, 1, (void **)&pBytes);
    if (res == DkResult_Success) {
      *ppData = pBytes + (ptrdiff_t)hAllocation->GetOffset();
      hAllocation->BlockAllocMap();
    }
    return res;
  }
  case DmaAllocation_T::ALLOCATION_TYPE_DEDICATED:
    return hAllocation->DedicatedAllocMap(this, ppData);
  default:
    DMA_ASSERT(0);
    return DkResult_Fail;
  }
}

void DmaAllocator_T::Unmap(DmaAllocation hAllocation) {
  switch (hAllocation->GetType()) {
  case DmaAllocation_T::ALLOCATION_TYPE_BLOCK: {
    DmaDeviceMemoryBlock *const pBlock = hAllocation->GetBlock();
    hAllocation->BlockAllocUnmap();
    pBlock->Unmap(this, 1);
  } break;
  case DmaAllocation_T::ALLOCATION_TYPE_DEDICATED:
    hAllocation->DedicatedAllocUnmap(this);
    break;
  default:
    DMA_ASSERT(0);
  }
}

void DmaAllocator_T::FlushOrInvalidateAllocation(DmaAllocation hAllocation,
                                                 uint32_t offset, uint32_t size,
                                                 DMA_CACHE_OPERATION op) {
  if (size > 0) {
    const uint32_t allocationSize = hAllocation->GetSize();
    DMA_ASSERT(offset <= allocationSize);

    uint32_t memOffset = 0;
    uint32_t memSize = 0;

    switch (hAllocation->GetType()) {
    case DmaAllocation_T::ALLOCATION_TYPE_DEDICATED:
      memOffset = offset;
      if (size == DMA_WHOLE_SIZE) {
        memSize = allocationSize - memOffset;
      } else {
        DMA_ASSERT(offset + size <= allocationSize);
        memSize = DMA_MIN(size, allocationSize - memOffset);
      }
      break;

    case DmaAllocation_T::ALLOCATION_TYPE_BLOCK: {
      // 1. Still within this allocation.
      memOffset = offset;
      if (size == DMA_WHOLE_SIZE) {
        size = allocationSize - offset;
      } else {
        DMA_ASSERT(offset + size <= allocationSize);
      }
      memSize = size;

      // 2. Adjust to whole block.
      const uint32_t allocationOffset = hAllocation->GetOffset();
      const uint32_t blockSize =
          hAllocation->GetBlock()->m_pMetadata->GetSize();
      memOffset += allocationOffset;
      memSize = DMA_MIN(memSize, blockSize - memOffset);

      break;
    }

    default:
      DMA_ASSERT(0);
    }

    switch (op) {
    case DMA_CACHE_FLUSH:
      dkMemBlockFlushCpuCache(hAllocation->GetMemory(), memOffset, memSize);
      break;
    default:
      DMA_ASSERT(0);
    }
  }
  // else: Just ignore this call.
}

void DmaAllocator_T::FreeDedicatedMemory(DmaAllocation allocation) {
  DMA_ASSERT(allocation && allocation->GetType() ==
                               DmaAllocation_T::ALLOCATION_TYPE_DEDICATED);

  const uint32_t memTypeIndex = allocation->GetMemoryTypeIndex();
  {
    DmaMutexLockWrite lock(m_DedicatedAllocationsMutex[memTypeIndex],
                           m_UseMutex);
    AllocationVectorType *const pDedicatedAllocations =
        m_pDedicatedAllocations[memTypeIndex];
    DMA_ASSERT(pDedicatedAllocations);
    bool success = DmaVectorRemoveSorted<DmaPointerLess>(*pDedicatedAllocations,
                                                         allocation);
    DMA_ASSERT(success);
  }

  DkMemBlock hMemory = allocation->GetMemory();

  FreeVulkanMemory(memTypeIndex, allocation->GetSize(), hMemory);

  DMA_DEBUG_LOG("    Freed DedicatedMemory MemoryTypeIndex=%u", memTypeIndex);
}

void DmaAllocator_T::FillAllocation(DmaAllocation hAllocation,
                                    uint8_t pattern) {
  if (DMA_DEBUG_INITIALIZE_ALLOCATIONS && !hAllocation->CanBecomeLost()) {
    void *pData = DMA_NULL;
    DkResult res = Map(hAllocation, &pData);
    if (res == DkResult_Success) {
      memset(pData, (int)pattern, (size_t)hAllocation->GetSize());
      FlushOrInvalidateAllocation(hAllocation, 0, DMA_WHOLE_SIZE,
                                  DMA_CACHE_FLUSH);
      Unmap(hAllocation);
    } else {
      DMA_ASSERT(0 && "DMA_DEBUG_INITIALIZE_ALLOCATIONS is enabled, but "
                      "couldn't map memory to fill allocation.");
    }
  }
}

#if DMA_STATS_STRING_ENABLED

void DmaAllocator_T::PrintDetailedMap(DmaJsonWriter &json) {
  bool dedicatedAllocationsStarted = false;
  for (uint32_t memTypeIndex = 0; memTypeIndex < DMA_NUM_MEMORY_TYPES;
       ++memTypeIndex) {
    DmaMutexLockRead dedicatedAllocationsLock(
        m_DedicatedAllocationsMutex[memTypeIndex], m_UseMutex);
    AllocationVectorType *const pDedicatedAllocVector =
        m_pDedicatedAllocations[memTypeIndex];
    DMA_ASSERT(pDedicatedAllocVector);
    if (pDedicatedAllocVector->empty() == false) {
      if (dedicatedAllocationsStarted == false) {
        dedicatedAllocationsStarted = true;
        json.WriteString("DedicatedAllocations");
        json.BeginObject();
      }

      json.BeginString("Type ");
      json.ContinueString(memTypeIndex);
      json.EndString();

      json.BeginArray();

      for (size_t i = 0; i < pDedicatedAllocVector->size(); ++i) {
        json.BeginObject(true);
        const DmaAllocation hAlloc = (*pDedicatedAllocVector)[i];
        hAlloc->PrintParameters(json);
        json.EndObject();
      }

      json.EndArray();
    }
  }
  if (dedicatedAllocationsStarted) {
    json.EndObject();
  }

  {
    bool allocationsStarted = false;
    for (uint32_t memTypeIndex = 0; memTypeIndex < DMA_NUM_MEMORY_TYPES;
         ++memTypeIndex) {
      if (m_pBlockVectors[memTypeIndex]->IsEmpty() == false) {
        if (allocationsStarted == false) {
          allocationsStarted = true;
          json.WriteString("DefaultPools");
          json.BeginObject();
        }

        json.BeginString("Type ");
        json.ContinueString(memTypeIndex);
        json.EndString();

        m_pBlockVectors[memTypeIndex]->PrintDetailedMap(json);
      }
    }
    if (allocationsStarted) {
      json.EndObject();
    }
  }

  // Custom pools
  {
    DmaMutexLockRead lock(m_PoolsMutex, m_UseMutex);
    const size_t poolCount = m_Pools.size();
    if (poolCount > 0) {
      json.WriteString("Pools");
      json.BeginObject();
      for (size_t poolIndex = 0; poolIndex < poolCount; ++poolIndex) {
        json.BeginString();
        json.ContinueString(m_Pools[poolIndex]->GetId());
        json.EndString();

        m_Pools[poolIndex]->m_BlockVector.PrintDetailedMap(json);
      }
      json.EndObject();
    }
  }
}

#endif // #if DMA_STATS_STRING_ENABLED

////////////////////////////////////////////////////////////////////////////////
// Public interface

DMA_CALL_PRE DkResult DMA_CALL_POST dmaCreateAllocator(
    const DmaAllocatorCreateInfo *pCreateInfo, DmaAllocator *pAllocator) {
  DMA_ASSERT(pCreateInfo && pAllocator);
  DMA_DEBUG_LOG("dmaCreateAllocator");
  *pAllocator =
      dma_new(pCreateInfo->pAllocationCallbacks, DmaAllocator_T)(pCreateInfo);
  return (*pAllocator)->Init(pCreateInfo);
}

DMA_CALL_PRE void DMA_CALL_POST dmaDestroyAllocator(DmaAllocator allocator) {
  if (allocator != DMA_NULL_HANDLE) {
    DMA_DEBUG_LOG("dmaDestroyAllocator");
    DmaAllocationCallbacks allocationCallbacks =
        allocator->m_AllocationCallbacks;
    dma_delete(&allocationCallbacks, allocator);
  }
}

DMA_CALL_PRE void DMA_CALL_POST dmaSetCurrentFrameIndex(DmaAllocator allocator,
                                                        uint32_t frameIndex) {
  DMA_ASSERT(allocator);
  DMA_ASSERT(frameIndex != DMA_FRAME_INDEX_LOST);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  allocator->SetCurrentFrameIndex(frameIndex);
}

DMA_CALL_PRE void DMA_CALL_POST dmaCalculateStats(DmaAllocator allocator,
                                                  DmaStats *pStats) {
  DMA_ASSERT(allocator && pStats);
  DMA_DEBUG_GLOBAL_MUTEX_LOCK
  allocator->CalculateStats(pStats);
}

DMA_CALL_PRE void DMA_CALL_POST dmaGetBudget(DmaAllocator allocator,
                                             DmaBudget *pBudget) {
  DMA_ASSERT(allocator && pBudget);
  DMA_DEBUG_GLOBAL_MUTEX_LOCK
  allocator->GetBudget(pBudget, 0, DMA_NUM_MEMORY_HEAPS);
}

#if DMA_STATS_STRING_ENABLED

DMA_CALL_PRE void DMA_CALL_POST dmaBuildStatsString(DmaAllocator allocator,
                                                    char **ppStatsString,
                                                    bool detailedMap) {
  DMA_ASSERT(allocator && ppStatsString);
  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  DmaStringBuilder sb(allocator);
  {
    DmaJsonWriter json(allocator->GetAllocationCallbacks(), sb);
    json.BeginObject();

    DmaBudget budget[DMA_NUM_MEMORY_HEAPS];
    allocator->GetBudget(budget, 0, DMA_NUM_MEMORY_HEAPS);

    DmaStats stats;
    allocator->CalculateStats(&stats);

    json.WriteString("Total");
    DmaPrintStatInfo(json, stats.total);

    for (uint32_t heapIndex = 0; heapIndex < DMA_NUM_MEMORY_HEAPS;
         ++heapIndex) {
      json.BeginString("Heap ");
      json.ContinueString(heapIndex);
      json.EndString();
      json.BeginObject();

      json.WriteString("Size");
      json.WriteNumber((uint64_t)DMA_HEAP_SIZE);

      json.WriteString("Budget");
      json.BeginObject();
      {
        json.WriteString("BlockBytes");
        json.WriteNumber(budget[heapIndex].blockBytes);
        json.WriteString("AllocationBytes");
        json.WriteNumber(budget[heapIndex].allocationBytes);
        json.WriteString("Usage");
        json.WriteNumber(budget[heapIndex].usage);
        json.WriteString("Budget");
        json.WriteNumber(budget[heapIndex].budget);
      }
      json.EndObject();

      if (stats.memoryHeap[heapIndex].blockCount > 0) {
        json.WriteString("Stats");
        DmaPrintStatInfo(json, stats.memoryHeap[heapIndex]);
      }

      for (uint32_t typeIndex = 0; typeIndex < DMA_NUM_MEMORY_TYPES;
           ++typeIndex) {
        if (0 == heapIndex) {
          json.BeginString("Type ");
          json.ContinueString(typeIndex);
          json.EndString();

          json.BeginObject();

          if (stats.memoryType[typeIndex].blockCount > 0) {
            json.WriteString("Stats");
            DmaPrintStatInfo(json, stats.memoryType[typeIndex]);
          }

          json.EndObject();
        }
      }

      json.EndObject();
    }
    if (detailedMap == true) {
      allocator->PrintDetailedMap(json);
    }

    json.EndObject();
  }

  const size_t len = sb.GetLength();
  char *const pChars = dma_new_array(allocator, char, len + 1);
  if (len > 0) {
    memcpy(pChars, sb.GetData(), len);
  }
  pChars[len] = '\0';
  *ppStatsString = pChars;
}

DMA_CALL_PRE void DMA_CALL_POST dmaFreeStatsString(DmaAllocator allocator,
                                                   char *pStatsString) {
  if (pStatsString != DMA_NULL) {
    DMA_ASSERT(allocator);
    size_t len = strlen(pStatsString);
    dma_delete_array(allocator, pStatsString, len + 1);
  }
}

#endif // #if DMA_STATS_STRING_ENABLED

DMA_CALL_PRE DkResult DMA_CALL_POST
dmaCreatePool(DmaAllocator allocator, const DmaPoolCreateInfo *pCreateInfo,
              DmaPool *pPool) {
  DMA_ASSERT(allocator && pCreateInfo && pPool);

  DMA_DEBUG_LOG("dmaCreatePool");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  DkResult res = allocator->CreatePool(pCreateInfo, pPool);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordCreatePool(
        allocator->GetCurrentFrameIndex(), *pCreateInfo, *pPool);
  }
#endif

  return res;
}

DMA_CALL_PRE void DMA_CALL_POST dmaDestroyPool(DmaAllocator allocator,
                                               DmaPool pool) {
  DMA_ASSERT(allocator);

  if (pool == DMA_NULL_HANDLE) {
    return;
  }

  DMA_DEBUG_LOG("dmaDestroyPool");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordDestroyPool(
        allocator->GetCurrentFrameIndex(), pool);
  }
#endif

  allocator->DestroyPool(pool);
}

DMA_CALL_PRE void DMA_CALL_POST dmaGetPoolStats(DmaAllocator allocator,
                                                DmaPool pool,
                                                DmaPoolStats *pPoolStats) {
  DMA_ASSERT(allocator && pool && pPoolStats);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  allocator->GetPoolStats(pool, pPoolStats);
}

DMA_CALL_PRE void DMA_CALL_POST dmaMakePoolAllocationsLost(
    DmaAllocator allocator, DmaPool pool, size_t *pLostAllocationCount) {
  DMA_ASSERT(allocator && pool);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordMakePoolAllocationsLost(
        allocator->GetCurrentFrameIndex(), pool);
  }
#endif

  allocator->MakePoolAllocationsLost(pool, pLostAllocationCount);
}

DMA_CALL_PRE DkResult DMA_CALL_POST
dmaCheckPoolCorruption(DmaAllocator allocator, DmaPool pool) {
  DMA_ASSERT(allocator && pool);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  DMA_DEBUG_LOG("dmaCheckPoolCorruption");

  return allocator->CheckPoolCorruption(pool);
}

DMA_CALL_PRE void DMA_CALL_POST dmaGetPoolName(DmaAllocator allocator,
                                               DmaPool pool,
                                               const char **ppName) {
  DMA_ASSERT(allocator && pool);

  DMA_DEBUG_LOG("dmaGetPoolName");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  *ppName = pool->GetName();
}

DMA_CALL_PRE void DMA_CALL_POST dmaSetPoolName(DmaAllocator allocator,
                                               DmaPool pool,
                                               const char *pName) {
  DMA_ASSERT(allocator && pool);

  DMA_DEBUG_LOG("dmaSetPoolName");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  pool->SetName(pName);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordSetPoolName(
        allocator->GetCurrentFrameIndex(), pool, pName);
  }
#endif
}

DMA_CALL_PRE DkResult DMA_CALL_POST dmaAllocateMemory(
    DmaAllocator allocator, const DmaMemoryRequirements *pDmaMemoryRequirements,
    const DmaAllocationCreateInfo *pCreateInfo, DmaAllocation *pAllocation,
    DmaAllocationInfo *pAllocationInfo) {
  DMA_ASSERT(allocator && pDmaMemoryRequirements && pCreateInfo && pAllocation);

  DMA_DEBUG_LOG("dmaAllocateMemory");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  DkResult result =
      allocator->AllocateMemory(*pDmaMemoryRequirements,
                                false, // requiresDedicatedAllocation
                                false, // prefersDedicatedAllocation
                                *pCreateInfo, DMA_SUBALLOCATION_TYPE_UNKNOWN,
                                1, // allocationCount
                                pAllocation);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordAllocateMemory(
        allocator->GetCurrentFrameIndex(), *pDmaMemoryRequirements,
        *pCreateInfo, *pAllocation);
  }
#endif

  if (pAllocationInfo != DMA_NULL && result == DkResult_Success) {
    allocator->GetAllocationInfo(*pAllocation, pAllocationInfo);
  }

  return result;
}

DMA_CALL_PRE DkResult DMA_CALL_POST dmaAllocateMemoryPages(
    DmaAllocator allocator, const DmaMemoryRequirements *pDmaMemoryRequirements,
    const DmaAllocationCreateInfo *pCreateInfo, size_t allocationCount,
    DmaAllocation *pAllocations, DmaAllocationInfo *pAllocationInfo) {
  if (allocationCount == 0) {
    return DkResult_Success;
  }

  DMA_ASSERT(allocator && pDmaMemoryRequirements && pCreateInfo &&
             pAllocations);

  DMA_DEBUG_LOG("dmaAllocateMemoryPages");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  DkResult result =
      allocator->AllocateMemory(*pDmaMemoryRequirements,
                                false, // requiresDedicatedAllocation
                                false, // prefersDedicatedAllocation
                                *pCreateInfo, DMA_SUBALLOCATION_TYPE_UNKNOWN,
                                allocationCount, pAllocations);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordAllocateMemoryPages(
        allocator->GetCurrentFrameIndex(), *pDmaMemoryRequirements,
        *pCreateInfo, (uint64_t)allocationCount, pAllocations);
  }
#endif

  if (pAllocationInfo != DMA_NULL && result == DkResult_Success) {
    for (size_t i = 0; i < allocationCount; ++i) {
      allocator->GetAllocationInfo(pAllocations[i], pAllocationInfo + i);
    }
  }

  return result;
}

DMA_CALL_PRE void DMA_CALL_POST dmaFreeMemory(DmaAllocator allocator,
                                              DmaAllocation allocation) {
  DMA_ASSERT(allocator);

  if (allocation == DMA_NULL_HANDLE) {
    return;
  }

  DMA_DEBUG_LOG("dmaFreeMemory");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordFreeMemory(
        allocator->GetCurrentFrameIndex(), allocation);
  }
#endif

  allocator->FreeMemory(1, // allocationCount
                        &allocation);
}

DMA_CALL_PRE void DMA_CALL_POST
dmaFreeMemoryPages(DmaAllocator allocator, size_t allocationCount,
                   DmaAllocation *pAllocations) {
  if (allocationCount == 0) {
    return;
  }

  DMA_ASSERT(allocator);

  DMA_DEBUG_LOG("dmaFreeMemoryPages");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordFreeMemoryPages(
        allocator->GetCurrentFrameIndex(), (uint64_t)allocationCount,
        pAllocations);
  }
#endif

  allocator->FreeMemory(allocationCount, pAllocations);
}

DMA_CALL_PRE DkResult DMA_CALL_POST dmaResizeAllocation(
    DmaAllocator allocator, DmaAllocation allocation, uint32_t newSize) {
  DMA_ASSERT(allocator && allocation);

  DMA_DEBUG_LOG("dmaResizeAllocation");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  return allocator->ResizeAllocation(allocation, newSize);
}

DMA_CALL_PRE void DMA_CALL_POST
dmaGetAllocationInfo(DmaAllocator allocator, DmaAllocation allocation,
                     DmaAllocationInfo *pAllocationInfo) {
  DMA_ASSERT(allocator && allocation && pAllocationInfo);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordGetAllocationInfo(
        allocator->GetCurrentFrameIndex(), allocation);
  }
#endif

  allocator->GetAllocationInfo(allocation, pAllocationInfo);
}

DMA_CALL_PRE bool DMA_CALL_POST dmaTouchAllocation(DmaAllocator allocator,
                                                   DmaAllocation allocation) {
  DMA_ASSERT(allocator && allocation);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordTouchAllocation(
        allocator->GetCurrentFrameIndex(), allocation);
  }
#endif

  return allocator->TouchAllocation(allocation);
}

DMA_CALL_PRE void DMA_CALL_POST dmaSetAllocationUserData(
    DmaAllocator allocator, DmaAllocation allocation, void *pUserData) {
  DMA_ASSERT(allocator && allocation);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  allocation->SetUserData(allocator, pUserData);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordSetAllocationUserData(
        allocator->GetCurrentFrameIndex(), allocation, pUserData);
  }
#endif
}

DMA_CALL_PRE void DMA_CALL_POST
dmaCreateLostAllocation(DmaAllocator allocator, DmaAllocation *pAllocation) {
  DMA_ASSERT(allocator && pAllocation);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK;

  allocator->CreateLostAllocation(pAllocation);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordCreateLostAllocation(
        allocator->GetCurrentFrameIndex(), *pAllocation);
  }
#endif
}

DMA_CALL_PRE DkResult DMA_CALL_POST dmaMapMemory(DmaAllocator allocator,
                                                 DmaAllocation allocation,
                                                 void **ppData) {
  DMA_ASSERT(allocator && allocation && ppData);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  DkResult res = allocator->Map(allocation, ppData);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordMapMemory(allocator->GetCurrentFrameIndex(),
                                              allocation);
  }
#endif

  return res;
}

DMA_CALL_PRE void DMA_CALL_POST dmaUnmapMemory(DmaAllocator allocator,
                                               DmaAllocation allocation) {
  DMA_ASSERT(allocator && allocation);

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordUnmapMemory(
        allocator->GetCurrentFrameIndex(), allocation);
  }
#endif

  allocator->Unmap(allocation);
}

DMA_CALL_PRE void DMA_CALL_POST dmaFlushAllocation(DmaAllocator allocator,
                                                   DmaAllocation allocation,
                                                   uint32_t offset,
                                                   uint32_t size) {
  DMA_ASSERT(allocator && allocation);

  DMA_DEBUG_LOG("dmaFlushAllocation");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  allocator->FlushOrInvalidateAllocation(allocation, offset, size,
                                         DMA_CACHE_FLUSH);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordFlushAllocation(
        allocator->GetCurrentFrameIndex(), allocation, offset, size);
  }
#endif
}

DMA_CALL_PRE void DMA_CALL_POST
dmaInvalidateAllocation(DmaAllocator allocator, DmaAllocation allocation,
                        uint32_t offset, uint32_t size) {
  DMA_ASSERT(allocator && allocation);

  DMA_DEBUG_LOG("dmaInvalidateAllocation");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  allocator->FlushOrInvalidateAllocation(allocation, offset, size,
                                         DMA_CACHE_INVALIDATE);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordInvalidateAllocation(
        allocator->GetCurrentFrameIndex(), allocation, offset, size);
  }
#endif
}

DMA_CALL_PRE DkResult DMA_CALL_POST
dmaCheckCorruption(DmaAllocator allocator, uint32_t memoryTypeBits) {
  DMA_ASSERT(allocator);

  DMA_DEBUG_LOG("dmaCheckCorruption");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  return allocator->CheckCorruption(memoryTypeBits);
}

DMA_CALL_PRE DkResult DMA_CALL_POST
dmaDefragment(DmaAllocator allocator, DmaAllocation *pAllocations,
              size_t allocationCount, bool *pAllocationsChanged,
              const DmaDefragmentationInfo *pDefragmentationInfo,
              DmaDefragmentationStats *pDefragmentationStats) {
  // Deprecated interface, reimplemented using new one.

  DmaDefragmentationInfo2 info2 = {};
  info2.allocationCount = (uint32_t)allocationCount;
  info2.pAllocations = pAllocations;
  info2.pAllocationsChanged = pAllocationsChanged;
  if (pDefragmentationInfo != DMA_NULL) {
    info2.maxCpuAllocationsToMove = pDefragmentationInfo->maxAllocationsToMove;
    info2.maxCpuBytesToMove = pDefragmentationInfo->maxBytesToMove;
  } else {
    info2.maxCpuAllocationsToMove = UINT32_MAX;
    info2.maxCpuBytesToMove = DMA_WHOLE_SIZE;
  }
  // info2.flags, maxGpuAllocationsToMove, maxGpuBytesToMove, commandBuffer
  // deliberately left zero.

  DmaDefragmentationContext ctx;
  DkResult res =
      dmaDefragmentationBegin(allocator, &info2, pDefragmentationStats, &ctx);
  if (res == DkResult_Fail) {
    res = dmaDefragmentationEnd(allocator, ctx);
  }
  return res;
}

DMA_CALL_PRE DkResult DMA_CALL_POST dmaDefragmentationBegin(
    DmaAllocator allocator, const DmaDefragmentationInfo2 *pInfo,
    DmaDefragmentationStats *pStats, DmaDefragmentationContext *pContext) {
  DMA_ASSERT(allocator && pInfo && pContext);

  // Degenerate case: Nothing to defragment.
  if (pInfo->allocationCount == 0 && pInfo->poolCount == 0) {
    return DkResult_Success;
  }

  DMA_ASSERT(pInfo->allocationCount == 0 || pInfo->pAllocations != DMA_NULL);
  DMA_ASSERT(pInfo->poolCount == 0 || pInfo->pPools != DMA_NULL);
  DMA_HEAVY_ASSERT(
      DmaValidatePointerArray(pInfo->allocationCount, pInfo->pAllocations));
  DMA_HEAVY_ASSERT(DmaValidatePointerArray(pInfo->poolCount, pInfo->pPools));

  DMA_DEBUG_LOG("dmaDefragmentationBegin");

  DMA_DEBUG_GLOBAL_MUTEX_LOCK

  DkResult res = allocator->DefragmentationBegin(*pInfo, pStats, pContext);

#if DMA_RECORDING_ENABLED
  if (allocator->GetRecorder() != DMA_NULL) {
    allocator->GetRecorder()->RecordDefragmentationBegin(
        allocator->GetCurrentFrameIndex(), *pInfo, *pContext);
  }
#endif

  return res;
}

DMA_CALL_PRE DkResult DMA_CALL_POST dmaDefragmentationEnd(
    DmaAllocator allocator, DmaDefragmentationContext context) {
  DMA_ASSERT(allocator);

  DMA_DEBUG_LOG("dmaDefragmentationEnd");

  if (context != DMA_NULL_HANDLE) {
    DMA_DEBUG_GLOBAL_MUTEX_LOCK

#if DMA_RECORDING_ENABLED
    if (allocator->GetRecorder() != DMA_NULL) {
      allocator->GetRecorder()->RecordDefragmentationEnd(
          allocator->GetCurrentFrameIndex(), context);
    }
#endif

    return allocator->DefragmentationEnd(context);
  } else {
    return DkResult_Success;
  }
}

#endif // #ifdef DMA_IMPLEMENTATION
