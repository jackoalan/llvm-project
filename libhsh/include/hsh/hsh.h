#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <iostream>
#include <cassert>
#include <list>
#include <map>
#include <unordered_map>
#include <sstream>
#include <fstream>

namespace hsh::detail {
// TODO: Make CMake define these in project-scope
#define HSH_MAX_UNIFORMS 8
#define HSH_MAX_IMAGES 8
#define HSH_MAX_SAMPLERS 8
#define HSH_MAX_VERTEX_BUFFERS 8
#define HSH_MAX_INDEX_BUFFERS 8
#define HSH_MAX_RENDER_TEXTURE_BINDINGS 4
#define HSH_DESCRIPTOR_POOL_SIZE 8192

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
  constexpr ArrayProxy(std::nullptr_t) noexcept : Length(0), Data(nullptr) {}

  ArrayProxy(const T &OneElt)
      : Data(&OneElt), Length(1) {}

  ArrayProxy(const T *data, size_t length)
      : Data(data), Length(length) {}

  ArrayProxy(const T *begin, const T *end)
      : Data(begin), Length(end - begin) {}

  template<typename A>
  ArrayProxy(const std::vector<T, A> &Vec)
      : Data(Vec.data()), Length(Vec.size()) {}

  template <size_t N>
  constexpr ArrayProxy(const std::array<T, N> &Arr)
      : Data(Arr.data()), Length(N) {}

  template <size_t N>
  constexpr ArrayProxy(const T (&Arr)[N]) : Data(Arr), Length(N) {}

  ArrayProxy(const std::initializer_list<T> &Vec)
      : Data(Vec.begin() == Vec.end() ? (T*)nullptr : Vec.begin()),
        Length(Vec.size()) {}

  const T *begin() const noexcept { return Data; }

  const T *end() const noexcept { return Data + Length; }

  const T &front() const noexcept {
    assert(Count && Ptr);
    return *Data;
  }

  const T &back() const noexcept {
    assert(Count && Ptr);
    return *(Data + Length - 1);
  }

  bool empty() const noexcept { return (Length == 0); }

  std::size_t size() const noexcept { return Length; }

  const T *data() const noexcept { return Data; }

private:
  std::size_t Length = 0;
  const T *Data = nullptr;
};
}

#define HSH_ENABLE_LOG 1
#define HSH_ENABLE_VULKAN 1

#if HSH_ENABLE_VULKAN
inline void hshVkAssert(const char* pred) {
  std::cerr << pred << " failed\n";
  std::abort();
}
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_NO_EXCEPTIONS
#define VK_USE_PLATFORM_XCB_KHR
#define VULKAN_HPP_ASSERT(pred) if (!(pred)) hshVkAssert(#pred)
#include <vulkan/vulkan.hpp>

#define VMA_USE_STL_CONTAINERS 1
#define VMA_USE_STL_SHARED_MUTEX 1
#define VMA_ASSERT(pred) if (!(pred)) hshVkAssert(#pred)
#include "vk_mem_alloc_hsh.h"

namespace VULKAN_HPP_NAMESPACE {
template <> struct ObjectDestroy<Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> {
public:
  ObjectDestroy() = default;
  ObjectDestroy(
      Instance owner, Optional<const AllocationCallbacks> allocationCallbacks,
      VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT;

protected:
  template <typename T> void destroy(T t) VULKAN_HPP_NOEXCEPT;
};
template <> struct ObjectDestroy<Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> {
public:
  ObjectDestroy() = default;
  ObjectDestroy(
      Device owner, Optional<const AllocationCallbacks> allocationCallbacks,
      VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT;

protected:
  template <typename T> void destroy(T t) VULKAN_HPP_NOEXCEPT;
};
} // namespace VULKAN_HPP_NAMESPACE

namespace hsh::detail::vulkan {
class BufferAllocation {
protected:
  friend BufferAllocation
  AllocateStaticBuffer(vk::DeviceSize size,
                       vk::BufferUsageFlagBits usage) noexcept;
  vk::Buffer Buffer;
  VmaAllocation Allocation = VK_NULL_HANDLE;
  BufferAllocation(vk::Buffer Buffer, VmaAllocation Allocation) noexcept
      : Buffer(Buffer), Allocation(Allocation) {}
public:
  BufferAllocation(const BufferAllocation& other) = delete;
  BufferAllocation &operator=(const BufferAllocation& other) = delete;
  BufferAllocation(BufferAllocation &&other) {
    std::swap(Buffer, other.Buffer);
    std::swap(Allocation, other.Allocation);
  }
  BufferAllocation &operator=(BufferAllocation&& other) {
    std::swap(Buffer, other.Buffer);
    std::swap(Allocation, other.Allocation);
    return *this;
  }
  ~BufferAllocation() noexcept;
  vk::Buffer getBuffer() const noexcept { return Buffer; }
  operator vk::Buffer() const { return getBuffer(); }
};

class DynamicBufferBinding {
  friend class DynamicBufferAllocation;
  vk::Buffer Buffer;
  vk::DeviceSize SecondOffset;
  DynamicBufferBinding(vk::Buffer Buffer, vk::DeviceSize SecondOffset)
      : Buffer(Buffer), SecondOffset(SecondOffset) {}
public:
  DynamicBufferBinding() = default;
  vk::Buffer getBuffer() const noexcept { return Buffer; }
  vk::DeviceSize getSecondOffset() const noexcept { return SecondOffset; }
};

class DynamicBufferAllocation : public BufferAllocation {
  friend DynamicBufferAllocation
  AllocateDynamicBuffer(vk::DeviceSize size, vk::BufferUsageFlagBits usage) noexcept;
  void *MappedData;
  vk::DeviceSize SecondOffset;
  DynamicBufferAllocation(vk::Buffer Buffer, VmaAllocation Allocation,
                          void *MappedData,
                          vk::DeviceSize SecondOffset) noexcept
      : BufferAllocation(Buffer, Allocation), MappedData(MappedData),
        SecondOffset(SecondOffset) {}

public:
  DynamicBufferBinding getBinding() const noexcept {
    return DynamicBufferBinding(Buffer, SecondOffset);
  }
  operator DynamicBufferBinding() const noexcept {
    return getBinding();
  }
  vk::DeviceSize getOffset() const noexcept;
  vk::DescriptorBufferInfo getDescriptorBufferInfo() const noexcept {
    return {Buffer, 0, SecondOffset};
  }
  void *map() noexcept {
    return reinterpret_cast<uint8_t*>(MappedData) + getOffset();
  }
  void unmap() noexcept;
};

class UploadBufferAllocation : public BufferAllocation {
  friend UploadBufferAllocation
  AllocateUploadBuffer(vk::DeviceSize size) noexcept;
  void *MappedData;
  UploadBufferAllocation(vk::Buffer Buffer, VmaAllocation Allocation,
                         void *MappedData) noexcept
      : BufferAllocation(Buffer, Allocation), MappedData(MappedData) {}

public:
  void *getMappedData() const noexcept { return MappedData; }
};

struct TextureAllocation {
  friend TextureAllocation
  AllocateTexture(const vk::ImageCreateInfo &CreateInfo,
                  bool Dedicated) noexcept;
protected:
  vk::Image Image;
  VmaAllocation Allocation = VK_NULL_HANDLE;
  TextureAllocation(vk::Image Image, VmaAllocation Allocation) noexcept
      : Image(Image), Allocation(Allocation) {}
public:
  TextureAllocation() = default;
  TextureAllocation(const TextureAllocation& other) = delete;
  TextureAllocation &operator=(const TextureAllocation& other) = delete;
  TextureAllocation(TextureAllocation &&other) {
    std::swap(Image, other.Image);
    std::swap(Allocation, other.Allocation);
  }
  TextureAllocation &operator=(TextureAllocation&& other) {
    std::swap(Image, other.Image);
    std::swap(Allocation, other.Allocation);
    return *this;
  }
  ~TextureAllocation() noexcept;
  vk::Image getImage() const noexcept { return Image; }
};

class SurfaceAllocation {
  SurfaceAllocation *Prev = nullptr, *Next = nullptr;
  vk::UniqueSurfaceKHR Surface;
  vk::UniqueSwapchainKHR Swapchain;
  vk::Extent2D Extent;
  vk::Format ColorFormat = vk::Format::eUndefined;
public:
  ~SurfaceAllocation() noexcept;
  explicit SurfaceAllocation(SurfaceAllocation *Next,
                             vk::UniqueSurfaceKHR &&Surface) noexcept
      : Next(Next), Surface(std::move(Surface)) {
    if (Next) Next->Prev = this;
  }
  bool EnsureReady() noexcept;
  const vk::Extent2D &getExtent() const noexcept { return Extent; }
  vk::Format getColorFormat() const noexcept { return ColorFormat; }
  SurfaceAllocation *getNext() const { return Next; }
};

class RenderTextureAllocation;
class RenderTextureBinding {
  friend class RenderTextureAllocation;
  RenderTextureAllocation *Allocation = nullptr;
  uint32_t BindingIdx = 0;
  RenderTextureBinding(RenderTextureAllocation &Allocation,
                       uint32_t BindingIdx) noexcept
      : Allocation(&Allocation), BindingIdx(BindingIdx) {}
public:
  RenderTextureBinding() noexcept = default;
  operator bool() const noexcept { return Allocation != nullptr; }
  RenderTextureAllocation *getAllocation() const noexcept { return Allocation; }
  uint32_t getBindingIdx() const noexcept { return BindingIdx; }
};

class RenderTextureAllocation {
  RenderTextureAllocation *Prev = nullptr, *Next = nullptr;
  SurfaceAllocation *Surface;
  vk::Extent2D Extent;
  vk::Format ColorFormat = vk::Format::eUndefined;
  uint32_t NumColorBindings, NumDepthBindings;
  TextureAllocation ColorTexture;
  TextureAllocation DepthTexture;
  struct Binding {
    TextureAllocation Texture;
    vk::UniqueImageView ImageView;
  };
  std::array<Binding, MaxRenderTextureBindings> ColorBindings;
  std::array<Binding, MaxRenderTextureBindings> DepthBindings;
  void Prepare() noexcept;
public:
  ~RenderTextureAllocation() noexcept;
  explicit RenderTextureAllocation(RenderTextureAllocation *Next,
                                   SurfaceAllocation *Surface,
                                   uint32_t NumColorBindings,
                                   uint32_t NumDepthBindings) noexcept
      : Next(Next), Surface(Surface), NumColorBindings(NumColorBindings),
        NumDepthBindings(NumDepthBindings) {
    if (Next) Next->Prev = this;
    assert(Surface);
    assert(NumColorBindings <= MaxRenderTextureBindings);
    assert(NumDepthBindings <= MaxRenderTextureBindings);
  }
  explicit RenderTextureAllocation(RenderTextureAllocation *Next,
                                   uint32_t width, uint32_t height,
                                   vk::Format colorFormat,
                                   uint32_t NumColorBindings,
                                   uint32_t NumDepthBindings) noexcept
      : Next(Next), Extent(width, height), ColorFormat(colorFormat),
        NumColorBindings(NumColorBindings), NumDepthBindings(NumDepthBindings) {
    if (Next) Next->Prev = this;
    assert(NumColorBindings <= MaxRenderTextureBindings);
    assert(NumDepthBindings <= MaxRenderTextureBindings);
    Prepare();
  }
  void EnsureReady() noexcept {
    if (Surface) {
      auto &SurfAlloc = *Surface;
      if (SurfAlloc.getExtent() != Extent ||
          SurfAlloc.getColorFormat() != ColorFormat) {
        Extent = SurfAlloc.getExtent();
        ColorFormat = SurfAlloc.getColorFormat();
        Prepare();
      }
    }
  }
  RenderTextureAllocation *getNext() const { return Next; }
};

inline struct {
  vk::Instance Instance;
  vk::PhysicalDevice PhysDevice;
  vk::Device Device;
  VmaAllocator Allocator = VK_NULL_HANDLE;
  std::array<vk::DescriptorSetLayout, 64> DescriptorSetLayout;
  vk::PipelineLayout PipelineLayout;
  struct DescriptorPoolChain *DescriptorPoolChain = nullptr;
  vk::DescriptorUpdateTemplate DescriptorUpdateTemplate;
  vk::RenderPass RenderPass;
  float Anisotropy = 0.f;
  unsigned DynamicBufferIndex = 0;
  vk::DeviceSize DynamicBufferMask = 0;
  vk::CommandBuffer Cmd;

  std::vector<UploadBufferAllocation> PendingUploadBuffers;
  SurfaceAllocation *SurfaceHead = nullptr;
  RenderTextureAllocation *RenderTextureHead = nullptr;

  void EnsureReady() noexcept {
    for (auto *Surf = SurfaceHead; Surf; Surf = Surf->getNext())
      Surf->EnsureReady();
    for (auto *RT = RenderTextureHead; RT; RT = RT->getNext())
      RT->EnsureReady();
  }

  void SetDescriptorSetLayout(vk::DescriptorSetLayout Layout) noexcept {
    std::fill(DescriptorSetLayout.begin(), DescriptorSetLayout.end(), Layout);
  }

  void SetBufferIndex(unsigned Idx) noexcept {
    DynamicBufferIndex = Idx;
    DynamicBufferMask = Idx ? ~VkDeviceSize(0) : 0;
  }
} Globals;

bool SurfaceAllocation::EnsureReady() noexcept {
  auto Capabilities =
      Globals.PhysDevice.getSurfaceCapabilitiesKHR(Surface.get()).value;
  if (!Swapchain || Capabilities.currentExtent != Extent) {
    struct SwapchainCreateInfo : vk::SwapchainCreateInfoKHR {
      explicit SwapchainCreateInfo(vk::PhysicalDevice PD,
                                   const vk::SurfaceCapabilitiesKHR &SC,
                                   vk::SurfaceKHR Surface, vk::Format &FmtOut,
                                   vk::SwapchainKHR OldSwapchain)
          : vk::SwapchainCreateInfoKHR({}, Surface) {
        setMinImageCount(std::max(2u, SC.minImageCount));
        vk::SurfaceFormatKHR UseFormat;
        for (auto &Format : PD.getSurfaceFormatsKHR(Surface).value) {
          if (Format.format == vk::Format::eB8G8R8A8Unorm) {
            FmtOut = vk::Format::eB8G8R8A8Unorm;
            UseFormat = Format;
            break;
          }
        }
        VULKAN_HPP_ASSERT(UseFormat.format != vk::Format::eUndefined);
        setImageFormat(UseFormat.format)
            .setImageColorSpace(UseFormat.colorSpace);
        setImageExtent(SC.currentExtent);
        setImageArrayLayers(1);
        constexpr auto WantedUsage = vk::ImageUsageFlagBits::eTransferDst |
                                     vk::ImageUsageFlagBits::eColorAttachment;
        VULKAN_HPP_ASSERT((SC.supportedUsageFlags & WantedUsage) ==
                          WantedUsage);
        setImageUsage(WantedUsage);
        setPresentMode(vk::PresentModeKHR::eFifo);
        setOldSwapchain(OldSwapchain);
      }
    };
    Swapchain = Globals.Device
                    .createSwapchainKHRUnique(SwapchainCreateInfo(
                        Globals.PhysDevice, Capabilities, Surface.get(),
                        ColorFormat, Swapchain.get()))
                    .value;
    Extent = Capabilities.currentExtent;
    return true;
  }
  return false;
}

BufferAllocation::~BufferAllocation() noexcept {
  vmaDestroyBuffer(Globals.Allocator, Buffer, Allocation);
}

TextureAllocation::~TextureAllocation() noexcept {
  vmaDestroyImage(Globals.Allocator, Image, Allocation);
}

SurfaceAllocation::~SurfaceAllocation() noexcept {
  if (Prev)
    Prev->Next = Next;
  else
    Globals.SurfaceHead = Next;
  if (Next)
    Next->Prev = Prev;
}

RenderTextureAllocation::~RenderTextureAllocation() noexcept {
  if (Prev)
    Prev->Next = Next;
  else
    Globals.RenderTextureHead = Next;
  if (Next)
    Next->Prev = Prev;
}

vk::DeviceSize DynamicBufferAllocation::getOffset() const noexcept {
  return SecondOffset & Globals.DynamicBufferMask;
}

void DynamicBufferAllocation::unmap() noexcept {
  vmaFlushAllocation(Globals.Allocator, Allocation, getOffset(), SecondOffset);
}
}

namespace VULKAN_HPP_NAMESPACE {
ObjectDestroy<Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>::ObjectDestroy(
    Instance owner, Optional<const AllocationCallbacks> allocationCallbacks,
    VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT {
  assert(owner == ::hsh::detail::vulkan::Globals.Instance);
}

template <typename T>
void ObjectDestroy<Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>::destroy(T t)
    VULKAN_HPP_NOEXCEPT {
  ::hsh::detail::vulkan::Globals.Instance.destroy(
      t, {}, VULKAN_HPP_DEFAULT_DISPATCHER);
}

ObjectDestroy<Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>::ObjectDestroy(
    Device owner, Optional<const AllocationCallbacks> allocationCallbacks,
    VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT {
  assert(owner == ::hsh::detail::vulkan::Globals.Device);
}

template <typename T>
void ObjectDestroy<Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>::destroy(T t)
    VULKAN_HPP_NOEXCEPT {
  ::hsh::detail::vulkan::Globals.Device.destroy(t, {},
                                                VULKAN_HPP_DEFAULT_DISPATCHER);
}
} // namespace VULKAN_HPP_NAMESPACE

namespace hsh {
enum Target : std::uint8_t {
#define HSH_TARGET(Enumeration, Active) Enumeration,
#include "targets.def"
  TARGET_MAX
};
struct uniform_buffer_typeless;
struct dynamic_uniform_buffer_typeless;
struct vertex_buffer_typeless;
struct dynamic_vertex_buffer_typeless;
struct texture_typeless;
} // namespace hsh

namespace hsh::detail {
template <hsh::Target T> struct SamplerObject;
}

namespace hsh::detail::vulkan {
struct DescriptorPoolWritesBase {
  std::array<vk::DescriptorBufferInfo, MaxUniforms> Uniforms;
  std::array<vk::DescriptorImageInfo, MaxImages> Images;
  std::array<vk::DescriptorImageInfo, MaxSamplers> Samplers;
  template <std::size_t... USeq, std::size_t... ISeq, std::size_t... SSeq>
  constexpr DescriptorPoolWritesBase(std::index_sequence<USeq...>,
                                     std::index_sequence<ISeq...>,
                                     std::index_sequence<SSeq...>) noexcept
      : Uniforms{vk::DescriptorBufferInfo({}, ((void)USeq, 0), VK_WHOLE_SIZE)...},
        Images{vk::DescriptorImageInfo(
            {}, {}, ((void)ISeq, vk::ImageLayout::eShaderReadOnlyOptimal))...},
        Samplers{vk::DescriptorImageInfo(
            {}, {}, ((void)SSeq, vk::ImageLayout::eUndefined))...} {}
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
struct DescriptorUpdateTemplateCreateInfo
    : vk::DescriptorUpdateTemplateCreateInfo {
  std::array<vk::DescriptorUpdateTemplateEntry, 3> Entries;
  constexpr DescriptorUpdateTemplateCreateInfo() noexcept
      : vk::DescriptorUpdateTemplateCreateInfo(
            {}, Entries.size(), Entries.data(),
            vk::DescriptorUpdateTemplateType::eDescriptorSet,
            Globals.DescriptorSetLayout[0]),
        Entries{
            vk::DescriptorUpdateTemplateEntry(
                0, 0, MaxUniforms, vk::DescriptorType::eUniformBufferDynamic,
                offsetof(DescriptorPoolWritesBase, Uniforms),
                sizeof(vk::DescriptorBufferInfo)),
            vk::DescriptorUpdateTemplateEntry(
                MaxUniforms, 0, MaxImages, vk::DescriptorType::eSampledImage,
                offsetof(DescriptorPoolWritesBase, Images),
                sizeof(vk::DescriptorImageInfo)),
            vk::DescriptorUpdateTemplateEntry(
                MaxUniforms + MaxImages, 0, MaxSamplers,
                vk::DescriptorType::eSampler,
                offsetof(DescriptorPoolWritesBase, Samplers),
                sizeof(vk::DescriptorImageInfo))} {}
};

struct DescriptorPoolCreateInfo : vk::DescriptorPoolCreateInfo {
  std::array<vk::DescriptorPoolSize, 3> PoolSizes{
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, MaxUniforms},
      vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, MaxImages},
      vk::DescriptorPoolSize{vk::DescriptorType::eSampler, MaxSamplers}};
  constexpr DescriptorPoolCreateInfo() noexcept
      : vk::DescriptorPoolCreateInfo({}, MaxDescriptorPoolSets,
                                     PoolSizes.size(), PoolSizes.data()) {}
};
#pragma GCC diagnostic pop

struct UniqueDescriptorSet {
  vk::DescriptorSet Set;
  std::uint64_t Index = UINT64_MAX;
  UniqueDescriptorSet() noexcept = default;
  UniqueDescriptorSet(vk::DescriptorSet Set,
                      std::size_t Index) noexcept
      : Set(Set), Index(Index) {}
  UniqueDescriptorSet(const UniqueDescriptorSet&) = delete;
  UniqueDescriptorSet &operator=(const UniqueDescriptorSet&) = delete;
  UniqueDescriptorSet(UniqueDescriptorSet&& Other) noexcept {
    Set = Other.Set;
    std::swap(Index, Other.Index);
  }
  UniqueDescriptorSet &operator=(UniqueDescriptorSet&& Other) noexcept {
    Set = Other.Set;
    std::swap(Index, Other.Index);
    return *this;
  }
  operator vk::DescriptorSet() const noexcept { return Set; }
  ~UniqueDescriptorSet() noexcept;
};

struct DescriptorPoolChain {
  struct DescriptorPool {
    vk::UniqueDescriptorPool Pool;
    std::size_t AllocatedSets = 0;
    static_assert(MaxDescriptorPoolSets % 64 == 0);
    std::array<uint64_t, MaxDescriptorPoolSets / 64> Bitmap{};
    DescriptorPool() noexcept {
      Pool =
          Globals.Device.createDescriptorPoolUnique(DescriptorPoolCreateInfo())
              .value;
    }
    struct DescriptorBucket {
      std::array<vk::DescriptorSet, 64> DescriptorSets;
      UniqueDescriptorSet allocate(struct DescriptorPool &pool, uint64_t &bmp,
                                   std::size_t Index) noexcept {
        VULKAN_HPP_ASSERT(bmp != UINT64_MAX && "descriptor bucket full");
        if (!DescriptorSets[0]) {
          struct DescriptorSetAllocateInfo
              : public vk::DescriptorSetAllocateInfo {
            explicit constexpr DescriptorSetAllocateInfo(
                vk::DescriptorPool pool) noexcept
                : vk::DescriptorSetAllocateInfo(
                      pool, 64, Globals.DescriptorSetLayout.data()) {}
          } AllocateInfo(pool.Pool.get());
          VULKAN_HPP_ASSERT(Globals.Device.allocateDescriptorSets(
                                &AllocateInfo, DescriptorSets.data()) ==
                            vk::Result::eSuccess);
        }
        for (unsigned i = 0; i < 64; ++i) {
          if ((bmp & (1u << i)) == 0) {
            bmp |= (1u << i);
            return UniqueDescriptorSet(DescriptorSets[i], Index + i);
          }
        }
        return {};
      }
    };
    std::array<DescriptorBucket, MaxDescriptorPoolSets / 64> Buckets;
    UniqueDescriptorSet allocate(std::size_t Index) noexcept {
      VULKAN_HPP_ASSERT(AllocatedSets < MaxDescriptorPoolSets &&
                        "descriptor pool full");
      auto BucketsIt = Buckets.begin();
      for (uint64_t &bmp : Bitmap) {
        if (bmp != UINT64_MAX) {
          ++AllocatedSets;
          return BucketsIt->allocate(*this, bmp, Index);
        }
        Index += 64;
        ++BucketsIt;
      }
      return {};
    }
    void free(std::size_t Index) noexcept {
      auto BucketIdx = Index / 64;
      auto BucketRem = Index % 64;
      VULKAN_HPP_ASSERT(AllocatedSets &&
                        "freed too many descriptor sets from pool");
      VULKAN_HPP_ASSERT(Bitmap[BucketIdx] & (1ull << BucketRem) &&
                        "double free");
      Bitmap[BucketIdx] &= ~(1ull << BucketRem);
      --AllocatedSets;
    }
  };
  std::list<DescriptorPool> Chain;
  UniqueDescriptorSet allocate() noexcept {
    std::size_t Index = 0;
    for (auto &pool : Chain) {
      if (pool.AllocatedSets != MaxDescriptorPoolSets)
        return pool.allocate(Index);
      Index += MaxDescriptorPoolSets;
    }
    return Chain.emplace_back().allocate(Index);
  }
  void free(std::size_t Index) noexcept {
    auto PoolIdx = Index / MaxDescriptorPoolSets;
    auto PoolRem = Index % MaxDescriptorPoolSets;
    auto PoolIt = Chain.begin();
    std::advance(PoolIt, PoolIdx);
    PoolIt->free(PoolRem);
  }
};

UniqueDescriptorSet::~UniqueDescriptorSet() noexcept {
  if (Index != UINT64_MAX)
    Globals.DescriptorPoolChain->free(Index);
}

inline VkResult vmaCreateAllocator(const VmaAllocatorCreateInfo& pCreateInfo,
                                   VmaAllocator* pAllocator) noexcept {
  return ::vmaCreateAllocator(
      reinterpret_cast<const VmaAllocatorCreateInfo *>(&pCreateInfo),
      pAllocator);
}

inline VkResult
vmaCreateBuffer(const vk::BufferCreateInfo &pBufferCreateInfo,
                const VmaAllocationCreateInfo &pAllocationCreateInfo,
                VkBuffer *pBuffer, VmaAllocation *pAllocation,
                VmaAllocationInfo *pAllocationInfo) noexcept {
  return ::vmaCreateBuffer(
      Globals.Allocator,
      reinterpret_cast<const VkBufferCreateInfo *>(&pBufferCreateInfo),
      reinterpret_cast<const VmaAllocationCreateInfo *>(&pAllocationCreateInfo),
      pBuffer, pAllocation, pAllocationInfo);
}

inline VkResult vmaCreateDoubleBuffer(const vk::BufferCreateInfo& pBufferCreateInfo,
                                      const VmaAllocationCreateInfo& pAllocationCreateInfo,
                                      VkBuffer* pBuffer,
                                      VmaAllocation* pAllocation,
                                      VmaAllocationInfo* pAllocationInfo,
                                      vk::DeviceSize* secondOffset) noexcept {
  return ::vmaCreateDoubleBuffer(
      Globals.Allocator,
      reinterpret_cast<const VkBufferCreateInfo *>(&pBufferCreateInfo),
      reinterpret_cast<const VmaAllocationCreateInfo *>(&pAllocationCreateInfo),
      pBuffer, pAllocation, pAllocationInfo, secondOffset);
}

inline BufferAllocation
AllocateStaticBuffer(vk::DeviceSize size,
                     vk::BufferUsageFlagBits usage) noexcept {
  struct StaticUniformBufferAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr StaticUniformBufferAllocationCreateInfo() noexcept
        : VmaAllocationCreateInfo{0, VMA_MEMORY_USAGE_GPU_ONLY} {}
  };
  VkBuffer Buffer;
  VmaAllocation Allocation;
  VULKAN_HPP_ASSERT(vmaCreateBuffer(vk::BufferCreateInfo({}, size, usage),
                                    StaticUniformBufferAllocationCreateInfo(),
                                    &Buffer, &Allocation,
                                    nullptr) == VK_SUCCESS);
  return BufferAllocation(Buffer, Allocation);
}

inline DynamicBufferAllocation
AllocateDynamicBuffer(vk::DeviceSize size,
                      vk::BufferUsageFlagBits usage) noexcept {
  struct DynamicUniformBufferAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr DynamicUniformBufferAllocationCreateInfo() noexcept
        : VmaAllocationCreateInfo{VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                  VMA_MEMORY_USAGE_CPU_TO_GPU} {}
  };
  VkBuffer Buffer;
  VmaAllocation Allocation;
  VmaAllocationInfo AllocInfo;
  VkDeviceSize SecondOffset;
  VULKAN_HPP_ASSERT(vmaCreateDoubleBuffer(
                        vk::BufferCreateInfo({}, size, usage),
                        DynamicUniformBufferAllocationCreateInfo(), &Buffer,
                        &Allocation, &AllocInfo, &SecondOffset) == VK_SUCCESS);
  return DynamicBufferAllocation(Buffer, Allocation, AllocInfo.pMappedData,
                                 SecondOffset);
}

inline UploadBufferAllocation
AllocateUploadBuffer(vk::DeviceSize size) noexcept {
  struct UploadBufferAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr UploadBufferAllocationCreateInfo() noexcept
        : VmaAllocationCreateInfo{VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                  VMA_MEMORY_USAGE_CPU_ONLY} {}
  };
  VkBuffer Buffer;
  VmaAllocation Allocation;
  VmaAllocationInfo AllocInfo;
  VULKAN_HPP_ASSERT(vmaCreateBuffer(
      vk::BufferCreateInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc),
      UploadBufferAllocationCreateInfo(), &Buffer,
      &Allocation, &AllocInfo) == VK_SUCCESS);
  return UploadBufferAllocation(Buffer, Allocation, AllocInfo.pMappedData);
}

inline TextureAllocation AllocateTexture(const vk::ImageCreateInfo &CreateInfo,
                                         bool Dedicated = false) noexcept {
  struct TextureAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr TextureAllocationCreateInfo(bool Dedicated) noexcept
        : VmaAllocationCreateInfo{
              Dedicated ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
                        : VmaAllocationCreateFlagBits(0),
              VMA_MEMORY_USAGE_GPU_ONLY} {}
  } AllocationCreateInfo{Dedicated};
  VkImage Image;
  VmaAllocation Allocation;
  VULKAN_HPP_ASSERT(
      vmaCreateImage(Globals.Allocator,
                     reinterpret_cast<const VkImageCreateInfo *>(&CreateInfo),
                     &AllocationCreateInfo, &Image, &Allocation,
                     nullptr) == VK_SUCCESS);
  return TextureAllocation(Image, Allocation);
}

void RenderTextureAllocation::Prepare() noexcept {
  ColorTexture = AllocateTexture(vk::ImageCreateInfo(
      {}, vk::ImageType::e2D, ColorFormat, vk::Extent3D(Extent),
      1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment |
      vk::ImageUsageFlagBits::eTransferSrc,
      {}, {}, {}, vk::ImageLayout::eUndefined), true);
  DepthTexture = AllocateTexture(vk::ImageCreateInfo(
      {}, vk::ImageType::e2D, vk::Format::eD32Sfloat, vk::Extent3D(Extent),
      1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment |
      vk::ImageUsageFlagBits::eTransferSrc,
      {}, {}, {}, vk::ImageLayout::eUndefined), true);
  for (uint32_t i = 0; i < NumColorBindings; ++i) {
    ColorBindings[i].Texture = AllocateTexture(
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, ColorFormat, vk::Extent3D(Extent), 1, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst,
            {}, {}, {}, vk::ImageLayout::eUndefined),
        true);
    ColorBindings[i].ImageView =
        vulkan::Globals.Device
            .createImageViewUnique(vk::ImageViewCreateInfo(
                {}, ColorBindings[i].Texture.getImage(), vk::ImageViewType::e2D,
                ColorFormat, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1,
                                          0, 1)))
            .value;
  }
  for (uint32_t i = 0; i < NumDepthBindings; ++i) {
    DepthBindings[i].Texture = AllocateTexture(
        vk::ImageCreateInfo({}, vk::ImageType::e2D, vk::Format::eD32Sfloat,
                            vk::Extent3D(Extent), 1, 1,
                            vk::SampleCountFlagBits::e1,
                            vk::ImageTiling::eOptimal,
                            vk::ImageUsageFlagBits::eSampled |
                                vk::ImageUsageFlagBits::eTransferDst,
                            {}, {}, {}, vk::ImageLayout::eUndefined),
        true);
    DepthBindings[i].ImageView =
        vulkan::Globals.Device
            .createImageViewUnique(vk::ImageViewCreateInfo(
                {}, DepthBindings[i].Texture.getImage(), vk::ImageViewType::e2D,
                vk::Format::eD32Sfloat, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1,
                                          0, 1)))
            .value;
  }
}
}
#endif

#if 0
class LogPrinter {
public:
  std::ostream &Out = std::cerr;
#if HSH_ENABLE_LOG
  template <typename T>
  LogPrinter &operator<<(const T &Obj) {
    Out << Obj;
    return *this;
  }
#else
  template <typename T> LogPrinter &operator<<(const T &Obj) {
    return *this;
  }
#endif
};

LogPrinter &logger() {
  static LogPrinter LP;
  return LP;
}
#endif

namespace hsh {
namespace detail {
template <typename T>
struct ClassWrapper {};

struct SamplerBinding;

constexpr unsigned NumStaticallyActiveTargets = 0
#define HSH_TARGET(Enumeration, Active) + unsigned(!!(Active))
#include "targets.def"
;
static_assert(NumStaticallyActiveTargets != 0, "No hsh targets are statically active");
constexpr enum Target FirstStaticallyActiveTarget() noexcept {
#define HSH_ACTIVE_TARGET(Enumeration) return Enumeration;
#include "targets.def"
}
inline enum Target ActiveTarget = FirstStaticallyActiveTarget();

template <Target T>
struct TargetTraits {
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
  template <typename ResTp>
  struct ResourceFactory {};
};
#if HSH_ENABLE_VULKAN
template <>
struct TargetTraits<VULKAN_SPIRV> {
  using UniformBufferOwner = vulkan::BufferAllocation;
  using UniformBufferBinding = vk::Buffer;
  using DynamicUniformBufferOwner = vulkan::DynamicBufferAllocation;
  using DynamicUniformBufferBinding = vulkan::DynamicBufferBinding;
  using VertexBufferOwner = vulkan::BufferAllocation;
  using VertexBufferBinding = vk::Buffer;
  using DynamicVertexBufferOwner = vulkan::DynamicBufferAllocation;
  using DynamicVertexBufferBinding = vulkan::DynamicBufferBinding;
  struct TextureBinding {
    vk::ImageView ImageView;
    std::uint8_t NumMips : 7;
    std::uint8_t Integer : 1;
  };
  struct TextureOwner {
    vulkan::TextureAllocation Allocation;
    vk::UniqueImageView ImageView;
    std::uint8_t NumMips : 7;
    std::uint8_t Integer : 1;
    TextureOwner(const TextureOwner& other) = delete;
    TextureOwner &operator=(const TextureOwner& other) = delete;
    TextureOwner(TextureOwner &&other) = default;
    TextureOwner &operator=(TextureOwner&& other) = default;

    TextureBinding getBinding() const noexcept {
      return TextureBinding{ImageView.get(), NumMips, Integer};
    }
    operator TextureBinding() const noexcept {
      return getBinding();
    }
  };
  struct RenderTextureBinding {
    vulkan::RenderTextureAllocation *Allocation = nullptr;
  };
  struct RenderTextureOwner {
    std::unique_ptr<vulkan::RenderTextureAllocation> Allocation;
    RenderTextureOwner(const RenderTextureOwner& other) = delete;
    RenderTextureOwner &operator=(const RenderTextureOwner& other) = delete;
    RenderTextureOwner(RenderTextureOwner &&other) = default;
    RenderTextureOwner &operator=(RenderTextureOwner&& other) = default;

    RenderTextureBinding getBinding() const noexcept {
      return RenderTextureBinding{Allocation.get()};
    }
    operator RenderTextureBinding() const noexcept {
      return getBinding();
    }
  };
  struct SurfaceBinding {
    vulkan::SurfaceAllocation *Allocation = nullptr;
  };
  struct SurfaceOwner {
    std::unique_ptr<vulkan::SurfaceAllocation> Allocation;
    SurfaceOwner(const SurfaceOwner& other) = delete;
    SurfaceOwner &operator=(const SurfaceOwner& other) = delete;
    SurfaceOwner(SurfaceOwner &&other) = default;
    SurfaceOwner &operator=(SurfaceOwner&& other) = default;

    SurfaceBinding getBinding() const noexcept {
      return SurfaceBinding{Allocation.get()};
    }
    operator SurfaceBinding() const noexcept {
      return getBinding();
    }
  };
  struct PipelineBinding {
    vk::Pipeline Pipeline;
    vulkan::UniqueDescriptorSet DescriptorSet;
    std::array<uint32_t, MaxUniforms> UniformOffsets{};
    std::array<vk::Buffer, MaxVertexBuffers> VertexBuffers{};
    std::array<vk::DeviceSize, MaxVertexBuffers> VertexOffsets{};
    struct Iterators {
      decltype(UniformOffsets)::iterator UniformOffsetIt;
      decltype(VertexBuffers)::iterator VertexBufferIt;
      decltype(VertexOffsets)::iterator VertexOffsetIt;
      constexpr explicit Iterators(PipelineBinding &Binding)
          : UniformOffsetIt(Binding.UniformOffsets.begin()),
            VertexBufferIt(Binding.VertexBuffers.begin()),
            VertexOffsetIt(Binding.VertexOffsets.begin()) {}

      void add(uniform_buffer_typeless uniform);
      void add(dynamic_uniform_buffer_typeless uniform);
      void add(vertex_buffer_typeless uniform);
      void add(dynamic_vertex_buffer_typeless uniform);
      static void add(texture_typeless uniform);
      static void add(SamplerBinding sampler);
    };

    PipelineBinding() = default;

    template <typename Impl, typename... Args>
    explicit PipelineBinding(ClassWrapper<Impl>, Args... args);
  };

  template <typename ResTp>
  struct ResourceFactory {};
};
#endif
template <unsigned NSTs>
struct SelectTargetTraits {
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

#define HSH_TRAIT_BINDING RenderTextureBinding
#define HSH_MULTI_TRAIT RenderTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER RenderTextureOwner
#define HSH_MULTI_TRAIT RenderTextureBinding
#include "trivial_trait.def"

#define HSH_TRAIT_BINDING SurfaceBinding
#define HSH_MULTI_TRAIT SurfaceOwner
#include "trait.def"

#define HSH_TRAIT_OWNER SurfaceOwner
#define HSH_MULTI_TRAIT SurfaceBinding
#include "trivial_trait.def"

#define HSH_MULTI_TRAIT PipelineBinding
#include "trait.def"

#define HSH_TRAIT_TEMPLATE_PARMS template<typename ResTp>
#define HSH_TRAIT_TEMPLATE_REFS <ResTp>
#define HSH_MULTI_TRAIT ResourceFactory
#include "trait.def"
};
template <>
struct SelectTargetTraits<1> {
  using TargetTraits = TargetTraits<FirstStaticallyActiveTarget()>;
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

#define HSH_TRAIT_BINDING RenderTextureBinding
#define HSH_SINGLE_TRAIT RenderTextureOwner
#include "trait.def"

#define HSH_TRAIT_OWNER RenderTextureOwner
#define HSH_SINGLE_TRAIT RenderTextureBinding
#include "trivial_trait.def"

#define HSH_TRAIT_BINDING SurfaceBinding
#define HSH_SINGLE_TRAIT SurfaceOwner
#include "trait.def"

#define HSH_TRAIT_OWNER SurfaceOwner
#define HSH_SINGLE_TRAIT SurfaceBinding
#include "trivial_trait.def"

#define HSH_SINGLE_TRAIT PipelineBinding
#include "trait.def"

#define HSH_TRAIT_TEMPLATE_PARMS template<typename ResTp>
#define HSH_TRAIT_TEMPLATE_REFS <ResTp>
#define HSH_SINGLE_TRAIT ResourceFactory
#include "trait.def"
};
using ActiveTargetTraits = SelectTargetTraits<NumStaticallyActiveTargets>;
}

struct base_buffer {};

template <typename T>
struct uniform_buffer;
template <typename T>
struct dynamic_uniform_buffer;
template <typename T>
struct vertex_buffer;
template <typename T>
struct dynamic_vertex_buffer;

struct uniform_buffer_typeless : base_buffer {
#ifndef NDEBUG
  const char *UniqueId;
  template <typename... Args>
  explicit uniform_buffer_typeless(const char *UniqueId, Args&&... args)
  : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
  explicit uniform_buffer_typeless(
      const char *UniqueId, uniform_buffer_typeless other)
      : UniqueId(UniqueId), Binding(other.Binding) {}
#else
  template <typename... Args>
  explicit uniform_buffer_typeless(Args&&... args) : Binding(std::forward<Args>(args)...) {}
#endif
  detail::ActiveTargetTraits::UniformBufferBinding Binding;
  template <typename T>
  explicit uniform_buffer_typeless(uniform_buffer<T> other) : Binding(other.Binding) {}
  template <typename T>
  uniform_buffer<T> cast() const;
  template <typename T>
  operator uniform_buffer<T>() const { return cast<T>(); }
};
struct dynamic_uniform_buffer_typeless : base_buffer {
#ifndef NDEBUG
  const char *UniqueId;
  template <typename... Args>
  explicit dynamic_uniform_buffer_typeless(const char *UniqueId, Args&&... args)
      : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
  explicit dynamic_uniform_buffer_typeless(
      const char *UniqueId, dynamic_uniform_buffer_typeless other)
      : UniqueId(UniqueId), Binding(other.Binding) {}
#else
  template <typename... Args>
  explicit dynamic_uniform_buffer_typeless(Args&&... args) : Binding(std::forward<Args>(args)...) {}
#endif
  detail::ActiveTargetTraits::DynamicUniformBufferBinding Binding;
  template <typename T>
  explicit dynamic_uniform_buffer_typeless(dynamic_uniform_buffer<T> other) : Binding(other.Binding) {}
  template <typename T>
  dynamic_uniform_buffer<T> cast() const;
  template <typename T>
  operator dynamic_uniform_buffer<T>() const { return cast<T>(); }
};
struct vertex_buffer_typeless : base_buffer {
#ifndef NDEBUG
  const char *UniqueId;
  template <typename... Args>
  explicit vertex_buffer_typeless(const char *UniqueId, Args&&... args)
      : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
  explicit vertex_buffer_typeless(
      const char *UniqueId, vertex_buffer_typeless other)
      : UniqueId(UniqueId), Binding(other.Binding) {}
#else
  template <typename... Args>
  explicit vertex_buffer_typeless(Args&&... args) : Binding(std::forward<Args>(args)...) {}
#endif
  detail::ActiveTargetTraits::VertexBufferBinding Binding;
  template <typename T>
  explicit vertex_buffer_typeless(vertex_buffer<T> other) : Binding(other.Binding) {}
  template <typename T>
  vertex_buffer<T> cast() const;
  template <typename T>
  operator vertex_buffer<T>() const { return cast<T>(); }
};
struct dynamic_vertex_buffer_typeless : base_buffer {
#ifndef NDEBUG
  const char *UniqueId;
  template <typename... Args>
  explicit dynamic_vertex_buffer_typeless(const char *UniqueId, Args&&... args)
      : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
  explicit dynamic_vertex_buffer_typeless(
      const char *UniqueId, dynamic_vertex_buffer_typeless other)
      : UniqueId(UniqueId), Binding(other.Binding) {}
#else
  template <typename... Args>
  explicit dynamic_vertex_buffer_typeless(Args&&... args) : Binding(std::forward<Args>(args)...) {}
#endif
  detail::ActiveTargetTraits::DynamicVertexBufferBinding Binding;
  template <typename T>
  explicit dynamic_vertex_buffer_typeless(dynamic_vertex_buffer<T> other) : Binding(other.Binding) {}
  template <typename T>
  dynamic_vertex_buffer<T> cast() const;
  template <typename T>
  operator dynamic_vertex_buffer<T>() const { return cast<T>(); }
};

#ifndef NDEBUG
#define HSH_CASTABLE_BUFFER(derived) \
template <typename T> struct derived : derived##_typeless { \
using MappedType = T; \
static constexpr char StaticUniqueId{}; \
template <typename... Args> \
explicit derived(Args&&... args) : derived##_typeless(&StaticUniqueId, std::forward<Args>(args)...) {} \
const T *operator->() const { assert(false && "Not to be used from host!"); return nullptr; } \
const T &operator*() const { assert(false && "Not to be used from host!"); return *reinterpret_cast<T*>(0); } \
};
#else
#define HSH_CASTABLE_BUFFER(derived) \
template <typename T> struct derived : derived##_typeless { \
using MappedType = T; \
template <typename... Args> \
explicit derived(Args&&... args) : derived##_typeless(std::forward<Args>(args)...) {} \
const T *operator->() const { assert(false && "Not to be used from host!"); return nullptr; } \
const T &operator*() const { assert(false && "Not to be used from host!"); return *reinterpret_cast<T*>(0); } \
};
#endif
HSH_CASTABLE_BUFFER(uniform_buffer)
HSH_CASTABLE_BUFFER(dynamic_uniform_buffer)
HSH_CASTABLE_BUFFER(vertex_buffer)
HSH_CASTABLE_BUFFER(dynamic_vertex_buffer)
#undef HSH_CASTABLE_BUFFER

#define HSH_DEFINE_BUFFER_CAST(derived) \
template <typename T> derived<T> derived##_typeless::cast() const { \
  assert(UniqueId == &derived<T>::StaticUniqueId && "bad cast"); \
  return static_cast<derived<T>>(*this); \
}
HSH_DEFINE_BUFFER_CAST(uniform_buffer)
HSH_DEFINE_BUFFER_CAST(dynamic_uniform_buffer)
HSH_DEFINE_BUFFER_CAST(vertex_buffer)
HSH_DEFINE_BUFFER_CAST(dynamic_vertex_buffer)
#undef HSH_DEFINE_BUFFER_CAST

struct float3;
struct float2;
struct float4 {
  float x, y, z, w;
  float4() = default;
  constexpr float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
  constexpr explicit float4(float f) : x(f), y(f), z(f), w(f) {}
  constexpr explicit float4(const float3 &other, float w = 1.f);
  constexpr explicit float4(const float2 &other, float z = 0.f, float w = 1.f);
  void operator+=(const float4 &other) { x += other.x; y += other.y; z += other.z; w += other.w; }
  void operator*=(const float4 &other) { x *= other.x; y *= other.y; z *= other.z; w *= other.w; }
  float4 operator/(float other) { return float4{x / other, y / other, z / other, w / other}; }
  float &operator[](std::size_t idx) { return (&x)[0]; }
  const float &operator[](std::size_t idx) const { return (&x)[0]; }
  float3 xyz() const;
  float2 xy() const;
  float2 xz() const;
  float2 xw() const;
};
struct float3 {
  float x, y, z;
  float3() = default;
  constexpr float3(float x, float y, float z) : x(x), y(y), z(z) {}
  constexpr explicit float3(float f) : x(f), y(f), z(f) {}
  float3 operator-() const { return float3{-x, -y, -z}; };
  float3 operator*(float other) const {
    return float3{x * other, y * other, z * other};
  }
  float3 operator/(float other) const {
    return float3{x / other, y / other, z / other};
  }
  float3 operator*(const float3 &other) const {
    return float3{x * other.x, y * other.y, z * other.z};
  }
  float3 &operator*=(const float3 &other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
  }
  float3 &operator*=(float other) {
    x *= other;
    y *= other;
    z *= other;
    return *this;
  }
  float3 operator+(const float3 &other) const {
    return float3{x + other.x, y + other.y, z + other.z};
  }
  float3 &operator+=(const float3 &other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }
  float &operator[](std::size_t idx) { return (&x)[0]; }
  const float &operator[](std::size_t idx) const { return (&x)[0]; }
};
float3 float4::xyz() const { return float3{x, y, z}; }
struct float2 {
  float x, y;
  float2() = default;
  constexpr float2(float x, float y) : x(x), y(y) {}
  constexpr explicit float2(float f) : x(f), y(f) {}
  float2 operator*(const float2 &other) const {
    return float2{x * other.x, y * other.y};
  }
  float2 operator/(const float2 &other) const {
    return float2{x / other.x, y / other.y};
  }
  float2 operator/(float other) const {
    return float2{x / other, y / other};
  }
  float2 operator-(const float2 &other) const {
    return float2{x - other.x, y - other.y};
  }
  float2 operator+(const float2 &other) const {
    return float2{x + other.x, y + other.y};
  }
  float2 operator-() const { return float2{-x, -y}; };
};
float2 float4::xy() const { return float2{x, y}; }
float2 float4::xz() const { return float2{x, z}; }
float2 float4::xw() const { return float2{x, w}; }
constexpr float4::float4(const hsh::float3 &other, float w)
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr float4::float4(const hsh::float2 &other, float z, float w)
    : x(other.x), y(other.y), z(z), w(w) {}
struct int3;
struct int2;
struct int4 {
  std::int32_t x, y, z, w;
  int4() = default;
  constexpr explicit int4(const int3 &other, std::int32_t w = 0);
  constexpr explicit int4(const int2 &other, std::int32_t z = 0,
                          std::int32_t w = 0);
  void operator+=(const int4 &other) {}
  void operator*=(const int4 &other) {}
};
struct int3 {
  std::int32_t x, y, z;
  int3() = default;
  constexpr int3(std::int32_t x, std::int32_t y, std::int32_t z)
      : x(x), y(y), z(z) {}
  constexpr explicit int3(std::int32_t f) : x(f), y(f), z(f) {}
  int3 operator-() const { return int3{-x, -y, -z}; };
  int3 operator*(std::int32_t other) {
    return int3{x * other, y * other, z * other};
  }
};
struct int2 {
  std::int32_t x, y;
  int2() = default;
  constexpr int2(std::int32_t x, std::int32_t y) : x(x), y(y) {}
  constexpr explicit int2(std::int32_t f) : x(f), y(f) {}
  int2 operator-() const { return int2{-x, -y}; };
};
constexpr int4::int4(const hsh::int3 &other, std::int32_t w)
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr int4::int4(const hsh::int2 &other, std::int32_t z, std::int32_t w)
    : x(other.x), y(other.y), z(z), w(w) {}
struct uint3;
struct uint2;
struct uint4 {
  std::uint32_t x, y, z, w;
  uint4() = default;
  constexpr explicit uint4(const uint3 &other, std::uint32_t w = 0);
  constexpr explicit uint4(const uint2 &other, std::uint32_t z = 0,
                           std::uint32_t w = 0);
  void operator+=(const uint4 &other) {}
  void operator*=(const uint4 &other) {}
};
struct uint3 {
  std::uint32_t x, y, z;
  uint3() = default;
  constexpr uint3(std::uint32_t x, std::uint32_t y, std::uint32_t z)
      : x(x), y(y), z(z) {}
  constexpr explicit uint3(std::uint32_t f) : x(f), y(f), z(f) {}
  uint3 operator-() const { return uint3{-x, -y, -z}; };
  uint3 operator*(std::uint32_t other) {
    return uint3{x * other, y * other, z * other};
  }
};
struct uint2 {
  std::uint32_t x, y;
  uint2() = default;
  constexpr uint2(std::uint32_t x, std::uint32_t y) : x(x), y(y) {}
  constexpr explicit uint2(std::uint32_t f) : x(f), y(f) {}
  uint2 operator-() const { return uint2{-x, -y}; };
};
constexpr uint4::uint4(const hsh::uint3 &other, std::uint32_t w)
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr uint4::uint4(const hsh::uint2 &other, std::uint32_t z,
                       std::uint32_t w)
    : x(other.x), y(other.y), z(z), w(w) {}
struct float4x4 {
  float4 cols[4];
  float4x4() = default;
  float4 &operator[](std::size_t col) { return cols[col]; }
  const float4 &operator[](std::size_t col) const { return cols[col]; }
  float4x4 operator*(const float4x4 &other) const { return float4x4{}; };
  float4 operator*(const float4 &other) const { return float4{}; };
};
struct float3x3 {
  float3x3() = default;
  float3 cols[3];
  float3x3(const float4x4 &other)
      : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3 &operator[](std::size_t col) { return cols[col]; }
  const float3 &operator[](std::size_t col) const { return cols[col]; }
  float3x3 operator*(const float3x3 &other) const { return float3x3{}; };
  float3 operator*(const float3 &other) const { return float3{}; };
};
struct aligned_float3x3 {
  aligned_float3x3() = default;
  struct col {
    col() = default;
    col(const float3 &c) : c(c) {}
    float3 c; float p;
  } cols[3];
  aligned_float3x3(const float3x3 &other)
      : cols{other.cols[0], other.cols[1], other.cols[2]} {}
  aligned_float3x3(const float4x4 &other)
      : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3 &operator[](std::size_t col) { return cols[col].c; }
  const float3 &operator[](std::size_t col) const { return cols[col].c; }
  float3x3 operator*(const float3x3 &other) const { return float3x3{}; };
  float3 operator*(const float3 &other) const { return float3{}; };
};

enum Filter : std::uint8_t {
  Nearest,
  Linear
};

enum SamplerAddressMode : std::uint8_t {
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder,
  MirrorClampToEdge
};

enum BorderColor : std::uint8_t {
  TransparentBlack,
  OpaqueBlack,
  OpaqueWhite,
};

enum Compare : std::uint8_t {
  Never,
  Less,
  Equal,
  LEqual,
  Greater,
  NEqual,
  GEqual,
  Always
};

/* Holds constant sampler information */
struct sampler {
  enum Filter MagFilter = Linear;
  enum Filter MinFilter = Linear;
  enum Filter MipmapMode = Linear;
  enum SamplerAddressMode AddressModeU = Repeat;
  enum SamplerAddressMode AddressModeV = Repeat;
  enum SamplerAddressMode AddressModeW = Repeat;
  float MipLodBias = 0.f;
  enum Compare CompareOp = Never;
  enum BorderColor BorderColor = TransparentBlack;
  constexpr sampler(
      enum Filter MagFilter = Linear,
      enum Filter MinFilter = Linear,
      enum Filter MipmapMode = Linear,
      enum SamplerAddressMode AddressModeU = Repeat,
      enum SamplerAddressMode AddressModeV = Repeat,
      enum SamplerAddressMode AddressModeW = Repeat,
      float MipLodBias = 0.f,
      enum Compare CompareOp = Never,
      enum BorderColor BorderColor = TransparentBlack
  ) : MagFilter(MagFilter),
      MinFilter(MinFilter),
      MipmapMode(MipmapMode),
      AddressModeU(AddressModeU),
      AddressModeV(AddressModeV),
      AddressModeW(AddressModeW),
      MipLodBias(MipLodBias),
      CompareOp(CompareOp),
      BorderColor(BorderColor) {}
};

template <typename T> struct vector_to_scalar {};
template <> struct vector_to_scalar<float> { using type = float; };
template <> struct vector_to_scalar<float2> { using type = float; };
template <> struct vector_to_scalar<float3> { using type = float; };
template <> struct vector_to_scalar<float4> { using type = float; };
template <> struct vector_to_scalar<int> { using type = int; };
template <> struct vector_to_scalar<int2> { using type = int; };
template <> struct vector_to_scalar<int3> { using type = int; };
template <> struct vector_to_scalar<int4> { using type = int; };
template <> struct vector_to_scalar<unsigned int> {
  using type = unsigned int;
};
template <> struct vector_to_scalar<uint2> { using type = unsigned int; };
template <> struct vector_to_scalar<uint3> { using type = unsigned int; };
template <> struct vector_to_scalar<uint4> { using type = unsigned int; };
template <typename T>
using vector_to_scalar_t = typename vector_to_scalar<T>::type;
template <typename T, int N> struct scalar_to_vector {};
template <> struct scalar_to_vector<float, 1> { using type = float; };
template <> struct scalar_to_vector<float, 2> { using type = float2; };
template <> struct scalar_to_vector<float, 3> { using type = float3; };
template <> struct scalar_to_vector<float, 4> { using type = float4; };
template <> struct scalar_to_vector<int, 1> { using type = int; };
template <> struct scalar_to_vector<int, 2> { using type = int2; };
template <> struct scalar_to_vector<int, 3> { using type = int3; };
template <> struct scalar_to_vector<int, 4> { using type = int4; };
template <> struct scalar_to_vector<unsigned int, 1> {
  using type = unsigned int;
};
template <> struct scalar_to_vector<unsigned int, 2> { using type = uint2; };
template <> struct scalar_to_vector<unsigned int, 3> { using type = uint3; };
template <> struct scalar_to_vector<unsigned int, 4> { using type = uint4; };
template <typename T, int N>
using scalar_to_vector_t = typename scalar_to_vector<T, N>::type;

struct base_texture {};

template <typename T> struct texture1d;
template <typename T> struct texture1d_array;
template <typename T> struct texture2d;
template <typename T> struct texture2d_array;
template <typename T> struct texture3d;
template <typename T> struct texturecube;
template <typename T> struct texturecube_array;

struct texture_typeless : base_texture {
#ifndef NDEBUG
  const char *UniqueId;
  template <typename... Args>
  explicit texture_typeless(const char *UniqueId, Args&&... args)
      : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
#else
  template <typename... Args>
  explicit texture_typeless(Args&&... args) : Binding(std::forward<Args>(args)...) {}
#endif
  detail::ActiveTargetTraits::TextureBinding Binding;
  template <typename T> explicit texture_typeless(texture1d<T> other) : Binding(other.Binding) {}
  template <typename T> explicit texture_typeless(texture1d_array<T> other) : Binding(other.Binding) {}
  template <typename T> explicit texture_typeless(texture2d<T> other) : Binding(other.Binding) {}
  template <typename T> explicit texture_typeless(texture2d_array<T> other) : Binding(other.Binding) {}
  template <typename T> explicit texture_typeless(texture3d<T> other) : Binding(other.Binding) {}
  template <typename T> explicit texture_typeless(texturecube<T> other) : Binding(other.Binding) {}
  template <typename T> explicit texture_typeless(texturecube_array<T> other) : Binding(other.Binding) {}
  template <typename T>
  T cast() const {
    assert(UniqueId == &T::StaticUniqueId && "bad cast");
    return static_cast<T>(*this);
  }
  template <typename T>
  operator T() const { return cast<T>(); }
};

#ifndef NDEBUG
#define HSH_CASTABLE_TEXTURE(derived, coordt) \
template <typename T> struct derived : texture_typeless { \
static constexpr char StaticUniqueId{}; \
template <typename... Args> \
explicit derived(Args&&... args) : texture_typeless(&StaticUniqueId, std::forward<Args>(args)...) {} \
scalar_to_vector_t<T, 4> sample(coordt, sampler = {}) const { return {}; } \
};
#else
#define HSH_CASTABLE_TEXTURE(derived, coordt) \
template <typename T> struct derived : texture_typeless { \
using MappedType = void; \
template <typename... Args> \
explicit derived(Args&&... args) : texture_typeless(std::forward<Args>(args)...) {} \
scalar_to_vector_t<T, 4> sample(coordt, sampler = {}) const { return {}; } \
};
#endif
HSH_CASTABLE_TEXTURE(texture1d, float)
HSH_CASTABLE_TEXTURE(texture1d_array, float2)
HSH_CASTABLE_TEXTURE(texture2d, float2)
HSH_CASTABLE_TEXTURE(texture2d_array, float3)
HSH_CASTABLE_TEXTURE(texture3d, float3)
HSH_CASTABLE_TEXTURE(texturecube, float3)
HSH_CASTABLE_TEXTURE(texturecube_array, float4)
#undef HSH_CASTABLE_TEXTURE

struct render_texture2d {
  using MappedType = void;
  detail::ActiveTargetTraits::RenderTextureBinding Binding;
};

struct surface {
  using MappedType = void;
  detail::ActiveTargetTraits::SurfaceBinding Binding;
};

float dot(const float2 &a, const float2 &b) {
  return a.x * b.x + a.y * b.y;
}
float dot(const float3 &a, const float3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
float sqrt(float v) {
  return std::sqrt(v);
}
float length(const float2 &a) {
  return sqrt(dot(a, a));
}
float length(const float3 &a) {
  return sqrt(dot(a, a));
}
float2 normalize(const float2 &a) {
  return a / length(a);
}
float3 normalize(const float3 &a) {
  return a / length(a);
}
float max(float a, float b) {
  return std::max(a, b);
}
float min(float a, float b) {
  return std::min(a, b);
}
float clamp(float v, float min, float max) {
  if (v > max)
    return max;
  else if (v < min)
    return min;
  else
    return v;
}
float3 clamp(const float3 &v, const float3 &min, const float3 &max) {
  float3 ret;
  for (int i = 0; i < 3; ++i)
    ret[i] = clamp(v[i], min[i], max[i]);
  return ret;
}
float saturate(float v) {
  return clamp(v, 0.f, 1.f);
}
float3 saturate(const float3 &v) {
  return clamp(v, hsh::float3(0.f), hsh::float3(1.f));
}
float exp2(float v) {
  return std::exp2(v);
}
float lerp(float a, float b, float t) {
  return b * t + a * (1.f - t);
}
float3 lerp(const float3 &a, const float3 &b, float t) {
  float3 ret;
  for (int i = 0; i < 3; ++i)
    ret[i] = b[i] * t + a[i] * (1.f - t);
  return ret;
}
float4 lerp(const float4 &a, const float4 &b, const float4 &t) {
  float4 ret;
  for (int i = 0; i < 4; ++i)
    ret[i] = b[i] * t[i] + a[i] * (1.f - t[i]);
  return ret;
}
float4 lerp(const float4 &a, const float4 &b, float t) {
  float4 ret;
  for (int i = 0; i < 4; ++i)
    ret[i] = b[i] * t + a[i] * (1.f - t);
  return ret;
}
float abs(float v) {
  return std::abs(v);
}
void discard() {}

enum Stage : std::uint8_t {
  Vertex,
  Control,
  Evaluation,
  Geometry,
  Fragment,
  MaxStage
};

enum Topology : std::uint8_t {
  Points,
  Lines,
  LineStrip,
  Triangles,
  TriangleStrip,
  TriangleFan,
  Patches
};

enum CullMode : std::uint8_t {
  CullNone,
  CullFront,
  CullBack,
  CullFrontAndBack
};

enum BlendFactor : std::uint8_t {
  Zero,
  One,
  SrcColor,
  InvSrcColor,
  DstColor,
  InvDstColor,
  SrcAlpha,
  InvSrcAlpha,
  DstAlpha,
  InvDstAlpha,
  Src1Color,
  InvSrc1Color,
  Src1Alpha,
  InvSrc1Alpha
};

enum BlendOp : std::uint8_t { Add, Subtract, ReverseSubtract };

enum ColorComponentFlags : std::uint8_t {
  Red = 1,
  Green = 2,
  Blue = 4,
  Alpha = 8
};

enum Format : std::uint8_t {
  R8_UNORM,
  RG8_UNORM,
  RGB8_UNORM,
  RGBA8_UNORM,
  R16_UNORM,
  RG16_UNORM,
  RGB16_UNORM,
  RGBA16_UNORM,
  R32_UINT,
  RG32_UINT,
  RGB32_UINT,
  RGBA32_UINT,
  R8_SNORM,
  RG8_SNORM,
  RGB8_SNORM,
  RGBA8_SNORM,
  R16_SNORM,
  RG16_SNORM,
  RGB16_SNORM,
  RGBA16_SNORM,
  R32_SINT,
  RG32_SINT,
  RGB32_SINT,
  RGBA32_SINT,
  R32_SFLOAT,
  RG32_SFLOAT,
  RGB32_SFLOAT,
  RGBA32_SFLOAT,
};

namespace pipeline {
template <bool CA = false, bool InShader = false> struct base_attribute {
  static constexpr bool is_ca = CA;
};
template <BlendFactor SrcColorBlendFactor = One,
    BlendFactor DstColorBlendFactor = Zero,
    BlendOp ColorBlendOp = Add,
    BlendFactor SrcAlphaBlendFactor = SrcColorBlendFactor,
    BlendFactor DstAlphaBlendFactor = DstColorBlendFactor,
    BlendOp AlphaBlendOp = ColorBlendOp,
    ColorComponentFlags ColorWriteComponents =
    ColorComponentFlags(Red | Green | Blue | Alpha)>
struct color_attachment : base_attribute<true> {};
template <Topology T = Triangles>
struct topology : base_attribute<> {};
template <unsigned P = 0>
struct patch_control_points : base_attribute<> {};
template <CullMode CM = CullNone>
struct cull_mode : base_attribute<> {};
template <Compare C = Always>
struct depth_compare : base_attribute<> {};
template <bool W = true>
struct depth_write : base_attribute<> {};
template <bool E = false>
struct early_depth_stencil : base_attribute<false, true> {};
template <typename... Attrs> struct pipeline {
  hsh::float4 position;
  static constexpr std::size_t color_attachment_count = ((Attrs::is_ca ? 1 : 0) + ...);
  std::array<hsh::float4, color_attachment_count> color_out;
};
}

namespace detail {

template <typename WordType> struct ShaderDataBlob {
  std::size_t Size = 0;
  const WordType *Data = nullptr;
  std::uint64_t Hash = 0;
  constexpr ShaderDataBlob() = default;
  template <typename T>
  constexpr ShaderDataBlob(const T Data, std::uint64_t Hash) noexcept
      : Size(std::extent<T>::value), Data(Data), Hash(Hash) {}
  constexpr operator bool() const { return Data != nullptr; }
};

/* Holds constant shader stage enum and data blob reference for
 * individual stage object compilation */
template <hsh::Target T> struct ShaderCode {
  enum Stage Stage = Stage::Vertex;
  ShaderDataBlob<uint8_t> Blob;
  constexpr ShaderCode(enum Stage Stage, ShaderDataBlob<uint8_t> Blob) noexcept
      : Stage(Stage), Blob(Blob) {}
};

/* Holds shader stage object as loaded into graphics API */
template <hsh::Target T> struct ShaderObject {};

/* Max supported mip count (enough for 16K texture) */
constexpr std::size_t MaxMipCount = 14;

/* Holds sampler object as loaded into graphics API */
template <hsh::Target T> struct SamplerObject {};

/* Associates texture with sampler object index in shader data. */
struct SamplerBinding {
  texture_typeless tex;
  unsigned idx = 0;
};

enum InputRate : std::uint8_t { PerVertex, PerInstance };

/* Holds constant vertex buffer binding information */
struct VertexBinding {
  std::uint32_t Stride : 24;
  std::uint8_t Binding : 7;
  enum InputRate InputRate : 1;
  constexpr VertexBinding() : Stride(0), Binding(0), InputRate(PerVertex) {}
  constexpr VertexBinding(std::uint8_t Binding, std::uint32_t Stride,
                          enum InputRate InputRate) noexcept
      : Stride(Stride), Binding(Binding), InputRate(InputRate) {}
};

constexpr std::size_t HshFormatToTexelSize(Format format) {
  switch (format) {
  case R8_UNORM:
    return 1;
  case RG8_UNORM:
    return 2;
  case RGB8_UNORM:
    return 3;
  case RGBA8_UNORM:
    return 4;
  case R16_UNORM:
    return 2;
  case RG16_UNORM:
    return 4;
  case RGB16_UNORM:
    return 6;
  case RGBA16_UNORM:
    return 8;
  case R32_UINT:
    return 4;
  case RG32_UINT:
    return 8;
  case RGB32_UINT:
    return 12;
  case RGBA32_UINT:
    return 16;
  case R8_SNORM:
    return 1;
  case RG8_SNORM:
    return 2;
  case RGB8_SNORM:
    return 3;
  case RGBA8_SNORM:
    return 4;
  case R16_SNORM:
    return 2;
  case RG16_SNORM:
    return 4;
  case RGB16_SNORM:
    return 6;
  case RGBA16_SNORM:
    return 8;
  case R32_SINT:
    return 4;
  case RG32_SINT:
    return 8;
  case RGB32_SINT:
    return 12;
  case RGBA32_SINT:
    return 16;
  case R32_SFLOAT:
    return 4;
  case RG32_SFLOAT:
    return 8;
  case RGB32_SFLOAT:
    return 12;
  case RGBA32_SFLOAT:
    return 16;
  }
}

constexpr bool HshFormatIsInteger(Format format) {
  switch (format) {
  default:
    return true;
  case R32_SFLOAT:
  case RG32_SFLOAT:
  case RGB32_SFLOAT:
  case RGBA32_SFLOAT:
    return false;
  }
}

/* Holds constant vertex attribute binding information */
struct VertexAttribute {
  std::uint32_t Offset = 0;
  std::uint8_t Binding = 0;
  enum Format Format = R8_UNORM;
  constexpr VertexAttribute() = default;
  constexpr VertexAttribute(std::uint8_t Binding, enum Format Format,
                            std::uint32_t Offset) noexcept
      : Offset(Offset), Binding(Binding), Format(Format) {}
};

/* Holds constant color attachment information */
struct ColorAttachment {
  enum BlendFactor SrcColorBlendFactor = One;
  enum BlendFactor DstColorBlendFactor = Zero;
  enum BlendOp ColorBlendOp = Add;
  enum BlendFactor SrcAlphaBlendFactor = One;
  enum BlendFactor DstAlphaBlendFactor = Zero;
  enum BlendOp AlphaBlendOp = Add;
  enum ColorComponentFlags ColorWriteComponents = ColorComponentFlags(Red | Green | Blue | Alpha);
  constexpr bool blendEnabled() const {
    return SrcColorBlendFactor == One && DstColorBlendFactor == Zero &&
           ColorBlendOp == Add && SrcAlphaBlendFactor == One &&
        DstAlphaBlendFactor == Zero && AlphaBlendOp == Add;
  }
  constexpr ColorAttachment() = default;
  constexpr ColorAttachment(
      enum BlendFactor SrcColorBlendFactor,
      enum BlendFactor DstColorBlendFactor,
      enum BlendOp ColorBlendOp,
      enum BlendFactor SrcAlphaBlendFactor,
      enum BlendFactor DstAlphaBlendFactor,
      enum BlendOp AlphaBlendOp,
      std::underlying_type_t<ColorComponentFlags> ColorWriteComponents
      ) : SrcColorBlendFactor(SrcColorBlendFactor),
          DstColorBlendFactor(DstColorBlendFactor),
          ColorBlendOp(ColorBlendOp),
          SrcAlphaBlendFactor(SrcAlphaBlendFactor),
          DstAlphaBlendFactor(DstAlphaBlendFactor),
          AlphaBlendOp(AlphaBlendOp),
          ColorWriteComponents(ColorComponentFlags(ColorWriteComponents)) {}
};

/* Holds constant pipeline information */
struct PipelineInfo {
  enum Topology Topology = Triangles;
  unsigned PatchControlPoints = 0;
  enum CullMode CullMode = CullNone;
  enum Compare DepthCompare = Always;
  bool DepthWrite = true;
  constexpr PipelineInfo() = default;
  constexpr PipelineInfo(enum Topology Topology,
                         unsigned PatchControlPoints,
                         enum CullMode CullMode,
                         enum Compare DepthCompare,
                         bool DepthWrite) noexcept
      : Topology(Topology), PatchControlPoints(PatchControlPoints),
        CullMode(CullMode), DepthCompare(DepthCompare), DepthWrite(DepthWrite) {}
};

template <Target T, std::uint32_t NStages, std::uint32_t NBindings,
          std::uint32_t NAttributes, std::uint32_t NSamplers,
          std::uint32_t NAttachments>
struct ShaderConstData {
  std::array<ShaderCode<T>, NStages> StageCodes;
  std::array<VertexBinding, NBindings> Bindings;
  std::array<VertexAttribute, NAttributes> Attributes;
  std::array<sampler, NSamplers> Samplers;
  std::array<ColorAttachment, NAttachments> Attachments;
  struct PipelineInfo PipelineInfo;

  constexpr ShaderConstData(std::array<ShaderCode<T>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A,
                            std::array<sampler, NSamplers> Samps,
                            std::array<ColorAttachment, NAttachments> Atts,
                            struct PipelineInfo PipelineInfo)
      : StageCodes(S), Bindings(B), Attributes(A), Samplers(Samps),
        Attachments(Atts), PipelineInfo(PipelineInfo) {}
};

template <Target T, std::uint32_t NStages, std::uint32_t NSamplers> struct ShaderData {
  using ObjectRef = std::reference_wrapper<ShaderObject<T>>;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = std::reference_wrapper<SamplerObject<T>>;
  std::array<SamplerRef, NSamplers> SamplerObjects;
  constexpr ShaderData(
      std::array<ObjectRef, NStages> S,
      std::array<SamplerRef, NSamplers> Samps)
      : ShaderObjects(S), SamplerObjects(Samps) {}
};

template <hsh::Target T>
struct PipelineBuilder {
  template <typename... Bindings, std::size_t... BSeq>
  static void build_pipelines(std::index_sequence<BSeq...>) {
    assert(false && "unimplemented pipeline builder");
  }
};

namespace buffer_math {
constexpr std::size_t MipSize2D(std::size_t width, std::size_t height,
                                std::size_t texelSize, bool NotZero) {
  return NotZero ? (width * height * texelSize) : 0;
}
template <std::size_t... Idx>
constexpr std::size_t MipOffset2D(std::size_t width, std::size_t height,
                                  std::size_t texelSize, std::size_t Mip,
                                  std::index_sequence<Idx...>) {
  return (MipSize2D(width >> Idx, height >> Idx, texelSize, Idx < Mip) + ...);
}
constexpr std::size_t MipOffset2D(std::size_t width, std::size_t height,
                                  std::size_t texelSize, std::size_t Mip) {
  return MipOffset2D(width, height, texelSize, Mip,
                     std::make_index_sequence<MaxMipCount>());
}
} // namespace buffer_math

#if HSH_ENABLE_VULKAN
constexpr vk::Format HshToVkFormat(Format Format) {
  switch (Format) {
  case R8_UNORM:
    return vk::Format::eR8Unorm;
  case RG8_UNORM:
    return vk::Format::eR8G8Unorm;
  case RGB8_UNORM:
    return vk::Format::eR8G8B8Unorm;
  case RGBA8_UNORM:
    return vk::Format::eR8G8B8A8Unorm;
  case R16_UNORM:
    return vk::Format::eR16Unorm;
  case RG16_UNORM:
    return vk::Format::eR16G16Unorm;
  case RGB16_UNORM:
    return vk::Format::eR16G16B16Unorm;
  case RGBA16_UNORM:
    return vk::Format::eR16G16B16A16Unorm;
  case R32_UINT:
    return vk::Format::eR32Uint;
  case RG32_UINT:
    return vk::Format::eR32G32Uint;
  case RGB32_UINT:
    return vk::Format::eR32G32B32Uint;
  case RGBA32_UINT:
    return vk::Format::eR32G32B32A32Uint;
  case R8_SNORM:
    return vk::Format::eR8Snorm;
  case RG8_SNORM:
    return vk::Format::eR8G8Snorm;
  case RGB8_SNORM:
    return vk::Format::eR8G8B8Snorm;
  case RGBA8_SNORM:
    return vk::Format::eR8G8B8A8Snorm;
  case R16_SNORM:
    return vk::Format::eR16Snorm;
  case RG16_SNORM:
    return vk::Format::eR16G16Snorm;
  case RGB16_SNORM:
    return vk::Format::eR16G16B16Snorm;
  case RGBA16_SNORM:
    return vk::Format::eR16G16B16A16Snorm;
  case R32_SINT:
    return vk::Format::eR32Sint;
  case RG32_SINT:
    return vk::Format::eR32G32Sint;
  case RGB32_SINT:
    return vk::Format::eR32G32B32Sint;
  case RGBA32_SINT:
    return vk::Format::eR32G32B32A32Sint;
  case R32_SFLOAT:
    return vk::Format::eR32Sfloat;
  case RG32_SFLOAT:
    return vk::Format::eR32G32Sfloat;
  case RGB32_SFLOAT:
    return vk::Format::eR32G32B32Sfloat;
  case RGBA32_SFLOAT:
    return vk::Format::eR32G32B32A32Sfloat;
  }
}

constexpr vk::VertexInputRate HshToVkInputRate(InputRate InputRate) {
  switch (InputRate) {
  case PerVertex:
    return vk::VertexInputRate::eVertex;
  case PerInstance:
    return vk::VertexInputRate::eInstance;
  }
}

constexpr vk::PrimitiveTopology HshToVkTopology(enum Topology Topology) {
  switch (Topology) {
  case Points:
    return vk::PrimitiveTopology::ePointList;
  case Lines:
    return vk::PrimitiveTopology::eLineList;
  case LineStrip:
    return vk::PrimitiveTopology::eLineStrip;
  case Triangles:
    return vk::PrimitiveTopology::eTriangleList;
  case TriangleStrip:
    return vk::PrimitiveTopology::eTriangleStrip;
  case TriangleFan:
    return vk::PrimitiveTopology::eTriangleFan;
  case Patches:
    return vk::PrimitiveTopology::ePatchList;
  }
}

constexpr vk::CullModeFlagBits HshToVkCullMode(enum CullMode CullMode) {
  switch (CullMode) {
  case CullNone:
    return vk::CullModeFlagBits::eNone;
  case CullFront:
    return vk::CullModeFlagBits::eFront;
  case CullBack:
    return vk::CullModeFlagBits::eBack;
  case CullFrontAndBack:
    return vk::CullModeFlagBits::eFrontAndBack;
  }
}

constexpr vk::CompareOp HshToVkCompare(enum Compare Compare) {
  switch (Compare) {
  case Never:
    return vk::CompareOp::eNever;
  case Less:
    return vk::CompareOp::eLess;
  case Equal:
    return vk::CompareOp::eEqual;
  case LEqual:
    return vk::CompareOp::eLessOrEqual;
  case Greater:
    return vk::CompareOp::eGreater;
  case NEqual:
    return vk::CompareOp::eNotEqual;
  case GEqual:
    return vk::CompareOp::eGreaterOrEqual;
  case Always:
    return vk::CompareOp::eAlways;
  }
}

constexpr vk::BlendFactor HshToVkBlendFactor(enum BlendFactor BlendFactor) {
  switch (BlendFactor) {
  case Zero:
    return vk::BlendFactor::eZero;
  case One:
    return vk::BlendFactor::eOne;
  case SrcColor:
    return vk::BlendFactor::eSrcColor;
  case InvSrcColor:
    return vk::BlendFactor::eOneMinusSrcColor;
  case DstColor:
    return vk::BlendFactor::eDstColor;
  case InvDstColor:
    return vk::BlendFactor::eOneMinusDstColor;
  case SrcAlpha:
    return vk::BlendFactor::eSrcAlpha;
  case InvSrcAlpha:
    return vk::BlendFactor::eOneMinusSrcAlpha;
  case DstAlpha:
    return vk::BlendFactor::eDstAlpha;
  case InvDstAlpha:
    return vk::BlendFactor::eOneMinusDstAlpha;
  case Src1Color:
    return vk::BlendFactor::eSrc1Color;
  case InvSrc1Color:
    return vk::BlendFactor::eOneMinusSrc1Color;
  case Src1Alpha:
    return vk::BlendFactor::eSrc1Alpha;
  case InvSrc1Alpha:
    return vk::BlendFactor::eOneMinusSrc1Alpha;
  }
}

constexpr vk::BlendOp HshToVkBlendOp(enum BlendOp BlendOp) {
  switch (BlendOp) {
  case Add:
    return vk::BlendOp::eAdd;
  case Subtract:
    return vk::BlendOp::eSubtract;
  case ReverseSubtract:
    return vk::BlendOp::eReverseSubtract;
  }
}

constexpr vk::Filter HshToVkFilter(enum Filter Filter) {
  switch (Filter) {
  case Nearest:
    return vk::Filter::eNearest;
  case Linear:
    return vk::Filter::eLinear;
  }
}

constexpr vk::SamplerMipmapMode HshToVkMipMode(enum Filter Filter) {
  switch (Filter) {
  case Nearest:
    return vk::SamplerMipmapMode::eNearest;
  case Linear:
    return vk::SamplerMipmapMode::eLinear;
  }
}

constexpr vk::SamplerAddressMode HshToVkAddressMode(enum SamplerAddressMode AddressMode) {
  switch (AddressMode) {
  case Repeat:
    return vk::SamplerAddressMode::eRepeat;
  case MirroredRepeat:
    return vk::SamplerAddressMode::eMirroredRepeat;
  case ClampToEdge:
    return vk::SamplerAddressMode::eClampToEdge;
  case ClampToBorder:
    return vk::SamplerAddressMode::eClampToBorder;
  case MirrorClampToEdge:
    return vk::SamplerAddressMode::eMirrorClampToEdge;
  }
}

constexpr vk::BorderColor HshToVkBorderColor(enum BorderColor BorderColor, bool Int) {
  switch (BorderColor) {
  case TransparentBlack:
    return Int ? vk::BorderColor::eIntTransparentBlack : vk::BorderColor::eFloatTransparentBlack;
  case OpaqueBlack:
    return Int ? vk::BorderColor::eIntOpaqueBlack : vk::BorderColor::eFloatOpaqueBlack;
  case OpaqueWhite:
    return Int ? vk::BorderColor::eIntOpaqueWhite : vk::BorderColor::eFloatOpaqueWhite;
  }
}

constexpr vk::ShaderStageFlagBits HshToVkShaderStage(enum Stage Stage) {
  switch (Stage) {
  default:
  case Vertex:
    return vk::ShaderStageFlagBits::eVertex;
  case Control:
    return vk::ShaderStageFlagBits::eTessellationControl;
  case Evaluation:
    return vk::ShaderStageFlagBits::eTessellationEvaluation;
  case Geometry:
    return vk::ShaderStageFlagBits::eGeometry;
  case Fragment:
    return vk::ShaderStageFlagBits::eFragment;
  }
}

constexpr vk::ColorComponentFlagBits HshToVkColorComponentFlags(enum ColorComponentFlags Comps) {
  return vk::ColorComponentFlagBits(
      (Comps & Red ? unsigned(vk::ColorComponentFlagBits::eR) : 0u) |
      (Comps & Green ? unsigned(vk::ColorComponentFlagBits::eG) : 0u) |
      (Comps & Blue ? unsigned(vk::ColorComponentFlagBits::eB) : 0u) |
      (Comps & Alpha ? unsigned(vk::ColorComponentFlagBits::eA) : 0u));
}

constexpr vk::ComponentSwizzle HshToVkComponentSwizzle(ColorComponentFlags flag) {
  switch (flag) {
  case Red:
    return vk::ComponentSwizzle::eR;
  case Green:
    return vk::ComponentSwizzle::eG;
  case Blue:
    return vk::ComponentSwizzle::eB;
  case Alpha:
    return vk::ComponentSwizzle::eA;
  }
}

template <> struct ShaderCode<VULKAN_SPIRV> {
  enum Stage Stage = Stage::Vertex;
  ShaderDataBlob<uint32_t> Blob;
  constexpr ShaderCode() noexcept = default;
  constexpr ShaderCode(enum Stage Stage, ShaderDataBlob<uint32_t> Blob) noexcept
      : Stage(Stage), Blob(Blob) {}
};

template <> struct ShaderObject<VULKAN_SPIRV> {
  vk::UniqueShaderModule ShaderModule;
  ShaderObject() noexcept = default;
  vk::ShaderModule get(const vk::ShaderModuleCreateInfo &Info) {
    if (!ShaderModule)
      ShaderModule = vulkan::Globals.Device.createShaderModuleUnique(Info).value;
    return ShaderModule.get();
  }
};

template <> struct SamplerObject<VULKAN_SPIRV> {
  std::array<std::array<vk::UniqueSampler, MaxMipCount - 1>, 2> Samplers;
  SamplerObject() noexcept = default;
  vk::Sampler get(const vk::SamplerCreateInfo &Info, bool Int, unsigned MipCount) {
    assert(MipCount && MipCount < MaxMipCount);
    vk::UniqueSampler &Samp = Samplers[Int][MipCount - 1];
    if (!Samp) {
      vk::SamplerCreateInfo ModInfo(Info);
      ModInfo.setMaxLod(float(MipCount - 1))
          .setAnisotropyEnable(vulkan::Globals.Anisotropy != 0.f)
          .setMaxAnisotropy(vulkan::Globals.Anisotropy);
      if (Int)
        ModInfo.setBorderColor(vk::BorderColor(int(Info.borderColor) + 1));
      Samp = vulkan::Globals.Device.createSamplerUnique(ModInfo).value;
    }
    return Samp.get();
  }
  vk::Sampler get(const vk::SamplerCreateInfo &Info, texture_typeless tex) {
    return get(Info, tex.Binding.get_VULKAN_SPIRV().Integer,
               tex.Binding.get_VULKAN_SPIRV().NumMips);
  }
};

namespace vulkan {
template <typename Impl>
struct DescriptorPoolWrites : DescriptorPoolWritesBase {
  struct Iterators {
    decltype(Uniforms)::iterator UniformIt;
    decltype(Images)::iterator ImageIt;
    decltype(Samplers)::iterator SamplerIt;
    constexpr explicit Iterators(DescriptorPoolWrites &Writes)
        : UniformIt(Writes.Uniforms.begin()),
          ImageIt(Writes.Images.begin()),
          SamplerIt(Writes.Samplers.begin()) {}
    void add(uniform_buffer_typeless uniform) {
      (UniformIt++)->setBuffer(uniform.Binding.get_VULKAN_SPIRV());
    }
    void add(dynamic_uniform_buffer_typeless uniform) {
      (UniformIt++)->setBuffer(uniform.Binding.get_VULKAN_SPIRV().getBuffer());
    }
    static void add(vertex_buffer_typeless) {}
    static void add(dynamic_vertex_buffer_typeless) {}
    void add(texture_typeless texture) {
      (ImageIt++)->setImageView(texture.Binding.get_VULKAN_SPIRV().ImageView);
    }
    void add(hsh::detail::SamplerBinding sampler) {
      (SamplerIt++)->setSampler(
          Impl::template data<VULKAN_SPIRV>.SamplerObjects[sampler.idx].get().
          get(Impl::template cdata<VULKAN_SPIRV>.Samplers[sampler.idx],
              sampler.tex));
    }
  };
  template <typename... Args>
  constexpr explicit DescriptorPoolWrites(Args... args) noexcept
      : DescriptorPoolWritesBase(std::make_index_sequence<MaxUniforms>(),
                                 std::make_index_sequence<MaxImages>(),
                                 std::make_index_sequence<MaxSamplers>()) {
    Iterators Its(*this);
    (Its.add(args), ...);
  }
};
} // namespace vulkan

template <typename Impl, typename... Args>
TargetTraits<VULKAN_SPIRV>::PipelineBinding::PipelineBinding(ClassWrapper<Impl>,
                                                             Args... args)
    : Pipeline(Impl::template data<VULKAN_SPIRV>.Pipeline.get()),
      DescriptorSet(vulkan::Globals.DescriptorPoolChain->allocate()) {
  vulkan::DescriptorPoolWrites<Impl> Writes(args...);
  vulkan::Globals.Device.updateDescriptorSetWithTemplate(
      DescriptorSet, vulkan::Globals.DescriptorUpdateTemplate, &Writes);
  Iterators Its(*this);
  (Its.add(args), ...);
}

void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(uniform_buffer_typeless) {
  UniformOffsetIt++;
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(dynamic_uniform_buffer_typeless uniform) {
  *UniformOffsetIt++ = uniform.Binding.get_VULKAN_SPIRV().getSecondOffset();
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(vertex_buffer_typeless uniform) {
  *VertexBufferIt++ = uniform.Binding.get_VULKAN_SPIRV();
  VertexOffsetIt++;
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(dynamic_vertex_buffer_typeless uniform) {
  *VertexBufferIt++ = uniform.Binding.get_VULKAN_SPIRV().getBuffer();
  *VertexOffsetIt++ = uniform.Binding.get_VULKAN_SPIRV().getSecondOffset();
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(texture_typeless) {}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(SamplerBinding) {}

constexpr std::array<vk::DynamicState, 2> Dynamics{vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};
constexpr vk::PipelineDynamicStateCreateInfo DynamicState{
    {}, 2, Dynamics.data()};

template <std::uint32_t NStages, std::uint32_t NBindings,
          std::uint32_t NAttributes, std::uint32_t NSamplers,
          std::uint32_t NAttachments>
struct ShaderConstData<VULKAN_SPIRV, NStages, NBindings, NAttributes, NSamplers,
                       NAttachments> {
  std::array<vk::ShaderModuleCreateInfo, NStages> StageCodes;
  std::array<vk::ShaderStageFlagBits, NStages> StageFlags;
  std::array<vk::VertexInputBindingDescription, NBindings>
      VertexBindingDescriptions;
  std::array<vk::VertexInputAttributeDescription, NAttributes>
      VertexAttributeDescriptions;
  std::array<vk::PipelineColorBlendAttachmentState, NAttachments>
      TargetAttachments;
  vk::PipelineVertexInputStateCreateInfo VertexInputState;
  vk::PipelineInputAssemblyStateCreateInfo InputAssemblyState;
  vk::PipelineTessellationStateCreateInfo TessellationState;
  vk::PipelineRasterizationStateCreateInfo RasterizationState;
  vk::PipelineDepthStencilStateCreateInfo DepthStencilState;
  vk::PipelineColorBlendStateCreateInfo ColorBlendState;
  std::array<vk::SamplerCreateInfo, NSamplers> Samplers;

  template <std::size_t... SSeq, std::size_t... BSeq, std::size_t... ASeq, std::size_t... SampSeq, std::size_t... AttSeq>
  constexpr ShaderConstData(std::array<ShaderCode<VULKAN_SPIRV>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A,
                            std::array<sampler, NSamplers> Samps,
                            std::array<ColorAttachment, NAttachments> Atts,
                            struct PipelineInfo PipelineInfo,
                            std::index_sequence<SSeq...>,
                            std::index_sequence<BSeq...>,
                            std::index_sequence<ASeq...>,
                            std::index_sequence<SampSeq...>,
                            std::index_sequence<AttSeq...>)
      : StageCodes{vk::ShaderModuleCreateInfo{
            {}, std::get<SSeq>(S).Blob.Size, std::get<SSeq>(S).Blob.Data}...},
        StageFlags{HshToVkShaderStage(std::get<SSeq>(S).Stage)...},
        VertexBindingDescriptions{vk::VertexInputBindingDescription{
            BSeq, std::get<BSeq>(B).Stride,
            HshToVkInputRate(std::get<BSeq>(B).InputRate)}...},
        VertexAttributeDescriptions{vk::VertexInputAttributeDescription{
            ASeq, std::get<ASeq>(A).Binding,
            HshToVkFormat(std::get<ASeq>(A).Format),
            std::get<ASeq>(A).Offset}...},
        TargetAttachments{vk::PipelineColorBlendAttachmentState{
            std::get<AttSeq>(Atts).blendEnabled(),
            HshToVkBlendFactor(std::get<AttSeq>(Atts).SrcColorBlendFactor),
            HshToVkBlendFactor(std::get<AttSeq>(Atts).DstColorBlendFactor),
            HshToVkBlendOp(std::get<AttSeq>(Atts).ColorBlendOp),
            HshToVkBlendFactor(std::get<AttSeq>(Atts).SrcAlphaBlendFactor),
            HshToVkBlendFactor(std::get<AttSeq>(Atts).DstAlphaBlendFactor),
            HshToVkBlendOp(std::get<AttSeq>(Atts).AlphaBlendOp),
            HshToVkColorComponentFlags(
                std::get<AttSeq>(Atts).ColorWriteComponents)}...},
        VertexInputState{{},
                         NBindings,
                         VertexBindingDescriptions.data(),
                         NAttributes,
                         VertexAttributeDescriptions.data()},
        InputAssemblyState{{}, HshToVkTopology(PipelineInfo.Topology), VK_TRUE},
        TessellationState{{}, PipelineInfo.PatchControlPoints},
        RasterizationState{{}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, HshToVkCullMode(PipelineInfo.CullMode)},
        DepthStencilState{{}, PipelineInfo.DepthCompare != Always, PipelineInfo.DepthWrite, HshToVkCompare(PipelineInfo.DepthCompare)},
        ColorBlendState{{}, VK_FALSE, vk::LogicOp ::eClear, NAttachments, TargetAttachments.data()},
        Samplers{vk::SamplerCreateInfo{
            {},
            HshToVkFilter(std::get<SampSeq>(Samps).MagFilter),
            HshToVkFilter(std::get<SampSeq>(Samps).MinFilter),
            HshToVkMipMode(std::get<SampSeq>(Samps).MipmapMode),
            HshToVkAddressMode(std::get<SampSeq>(Samps).AddressModeU),
            HshToVkAddressMode(std::get<SampSeq>(Samps).AddressModeV),
            HshToVkAddressMode(std::get<SampSeq>(Samps).AddressModeW),
            std::get<SampSeq>(Samps).MipLodBias,
            0, 0,
            std::get<SampSeq>(Samps).CompareOp != Never,
            HshToVkCompare(std::get<SampSeq>(Samps).CompareOp),
            0, 0,
            HshToVkBorderColor(std::get<SampSeq>(Samps).BorderColor, false)}...} {}

  constexpr ShaderConstData(std::array<ShaderCode<VULKAN_SPIRV>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A,
                            std::array<sampler, NSamplers> Samps,
                            std::array<ColorAttachment, NAttachments> Atts,
                            struct PipelineInfo PipelineInfo)
      : ShaderConstData(S, B, A, Samps, Atts, PipelineInfo,
                        std::make_index_sequence<NStages>(),
                        std::make_index_sequence<NBindings>(),
                        std::make_index_sequence<NAttributes>(),
                        std::make_index_sequence<NSamplers>(),
                        std::make_index_sequence<NAttachments>()) {}

  template <typename B>
  vk::GraphicsPipelineCreateInfo
  getPipelineInfo(VkPipelineShaderStageCreateInfo *StageInfos) const {
    for (std::size_t i = 0; i < NStages; ++i)
      StageInfos[i] = vk::PipelineShaderStageCreateInfo{
          {},
          StageFlags[i],
          B::template data<VULKAN_SPIRV>.ShaderObjects[i].get().get(
              StageCodes[i])};

    return vk::GraphicsPipelineCreateInfo{
        {},
        NStages,
        reinterpret_cast<vk::PipelineShaderStageCreateInfo *>(StageInfos),
        &VertexInputState,
        &InputAssemblyState,
        &TessellationState,
        nullptr,
        &RasterizationState,
        nullptr,
        &DepthStencilState,
        &ColorBlendState,
        &DynamicState,
        vulkan::Globals.PipelineLayout,
        vulkan::Globals.RenderPass};
  }
};

template <std::uint32_t NStages, std::uint32_t NSamplers>
struct ShaderData<VULKAN_SPIRV, NStages, NSamplers> {
  using ObjectRef = std::reference_wrapper<ShaderObject<VULKAN_SPIRV>>;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = std::reference_wrapper<SamplerObject<VULKAN_SPIRV>>;
  std::array<SamplerRef, NSamplers> SamplerObjects;
  vk::UniquePipeline Pipeline;
  constexpr ShaderData(std::array<ObjectRef, NStages> S,
                       std::array<SamplerRef, NSamplers> Samps)
      : ShaderObjects(S), SamplerObjects(Samps) {}
};

template <> struct PipelineBuilder<VULKAN_SPIRV> {
  template <typename B>
  static constexpr std::size_t GetNumStages(bool NotZero) {
    return NotZero ? B::template cdata<VULKAN_SPIRV>.StageCodes.size() : 0;
  }
  template <typename... B, std::size_t... BSeq>
  static constexpr std::size_t StageInfoStart(std::size_t BIdx,
                                              std::index_sequence<BSeq...>) {
    return (GetNumStages<B>(BSeq < BIdx) + ...);
  }
  template <typename B> static void SetPipeline(vk::Pipeline data) {
    vk::ObjectDestroy<vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(
        vulkan::Globals.Device, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    B::template data<VULKAN_SPIRV>.Pipeline = vk::UniquePipeline(data, deleter);
  }
  template <typename... B, std::size_t... BSeq>
  static void build_pipelines(std::index_sequence<BSeq...> seq) {
    std::array<VkPipelineShaderStageCreateInfo, (GetNumStages<B>(true) + ...)>
        ShaderStageInfos;
    std::array<vk::GraphicsPipelineCreateInfo, sizeof...(B)> Infos{
        B::template cdata<VULKAN_SPIRV>.template getPipelineInfo<B>(
            ShaderStageInfos.data() + StageInfoStart<B...>(BSeq, seq))...};
    std::array<vk::Pipeline, sizeof...(B)> Pipelines;
    VULKAN_HPP_ASSERT(vulkan::Globals.Device.createGraphicsPipelines(
                          {}, Infos.size(), Infos.data(), nullptr,
                          Pipelines.data()) == vk::Result::eSuccess);
    (SetPipeline<B>(Pipelines[BSeq]), ...);
  }
};

namespace buffer_math::vulkan {
template <std::size_t... Idx>
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies2D(std::size_t width, std::size_t height, std::size_t texelSize,
             vk::ImageAspectFlagBits aspect, std::index_sequence<Idx...>) {
  return {vk::BufferImageCopy(
      MipOffset2D(width, height, texelSize, Idx), width >> Idx, height >> Idx,
      {aspect, Idx, 0, 1}, {},
      {uint32_t(width >> Idx), uint32_t(height >> Idx), 1})...};
}
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies2D(std::size_t width, std::size_t height, std::size_t texelSize,
             vk::ImageAspectFlagBits aspect) {
  return MakeCopies2D(width, height, texelSize, aspect,
                      std::make_index_sequence<MaxMipCount>());
}
} // namespace buffer_math::vulkan

template <typename T>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<uniform_buffer<T>> {
  template <typename CopyFunc> static auto Create(CopyFunc copyFunc) {
    auto UploadBuffer = vulkan::AllocateUploadBuffer(sizeof(T));
    copyFunc(UploadBuffer.getMappedData(), sizeof(T));

    TargetTraits<VULKAN_SPIRV>::UniformBufferOwner Ret =
        vulkan::AllocateStaticBuffer(sizeof(T),
                                     vk::BufferUsageFlagBits::eUniformBuffer);

    vulkan::Globals.Cmd.copyBuffer(UploadBuffer.getBuffer(), Ret.getBuffer(),
                                   vk::BufferCopy(0, 0, sizeof(T)));

    vulkan::Globals.PendingUploadBuffers.push_back(std::move(UploadBuffer));

    return Ret;
  }
};

template <typename T>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<dynamic_uniform_buffer<T>> {
  static auto Create() {
    return vulkan::AllocateDynamicBuffer(
        sizeof(T), vk::BufferUsageFlagBits::eUniformBuffer);
  }
};

template <typename T>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<vertex_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(std::size_t Count, CopyFunc copyFunc) {
    std::size_t Size = sizeof(T) * Count;
    auto UploadBuffer = vulkan::AllocateUploadBuffer(Size);
    copyFunc(UploadBuffer.getMappedData(), Size);

    TargetTraits<VULKAN_SPIRV>::UniformBufferOwner Ret =
        vulkan::AllocateStaticBuffer(Size,
                                     vk::BufferUsageFlagBits::eVertexBuffer);

    vulkan::Globals.Cmd.copyBuffer(UploadBuffer.getBuffer(), Ret.getBuffer(),
                                   vk::BufferCopy(0, 0, Size));

    vulkan::Globals.PendingUploadBuffers.push_back(std::move(UploadBuffer));

    return Ret;
  }
};

template <typename T>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<dynamic_vertex_buffer<T>> {
  static auto Create(std::size_t Count) {
    return vulkan::AllocateDynamicBuffer(
        sizeof(T) * Count, vk::BufferUsageFlagBits::eVertexBuffer);
  }
};

template <typename TexelType>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<texture2d<TexelType>> {
  template <typename CopyFunc>
  static auto Create(uint32_t width, uint32_t height, Format format,
                     uint32_t numMips, CopyFunc copyFunc,
                     ColorComponentFlags redSwizzle = Red,
                     ColorComponentFlags greenSwizzle = Green,
                     ColorComponentFlags blueSwizzle = Blue,
                     ColorComponentFlags alphaSwizzle = Alpha) {
    auto TexelSize = HshFormatToTexelSize(format);
    std::array<vk::BufferImageCopy, MaxMipCount> Copies =
        buffer_math::vulkan::MakeCopies2D(width, height, TexelSize,
                                          vk::ImageAspectFlagBits::eColor);
    auto BufferSize =
        buffer_math::MipOffset2D(width, height, TexelSize, numMips);
    auto UploadBuffer = vulkan::AllocateUploadBuffer(BufferSize);
    copyFunc(UploadBuffer.getMappedData(), BufferSize);

    TargetTraits<VULKAN_SPIRV>::TextureOwner Ret{
        vulkan::AllocateTexture(vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, HshToVkFormat(format), {width, height, 1},
            numMips, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst,
            {}, {}, {}, vk::ImageLayout::eUndefined))};
    Ret.ImageView =
        vulkan::Globals.Device
            .createImageViewUnique(vk::ImageViewCreateInfo(
                {}, Ret.Allocation.getImage(), vk::ImageViewType::e2D,
                HshToVkFormat(format),
                vk::ComponentMapping(HshToVkComponentSwizzle(redSwizzle),
                                     HshToVkComponentSwizzle(greenSwizzle),
                                     HshToVkComponentSwizzle(blueSwizzle),
                                     HshToVkComponentSwizzle(alphaSwizzle)),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                          numMips, 0, 1)))
            .value;
    Ret.NumMips = numMips;
    Ret.Integer = HshFormatIsInteger(format);

    vulkan::Globals.Cmd.copyBufferToImage(
        UploadBuffer.getBuffer(), Ret.Allocation.getImage(),
        vk::ImageLayout::eTransferDstOptimal, numMips, Copies.data());

    vulkan::Globals.PendingUploadBuffers.push_back(std::move(UploadBuffer));

    return Ret;
  }
};

template <>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<render_texture2d> {
  static auto Create(surface Surf, uint32_t NumColorBindings,
                     uint32_t NumDepthBindings) {
    return TargetTraits<VULKAN_SPIRV>::RenderTextureOwner{
        std::make_unique<vulkan::RenderTextureAllocation>(
            vulkan::Globals.RenderTextureHead,
            Surf.Binding.get_VULKAN_SPIRV().Allocation, NumColorBindings,
            NumDepthBindings)};
  }
};

template <>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<surface> {
#ifdef VK_USE_PLATFORM_XCB_KHR
  static auto Create(xcb_connection_t *Conn, xcb_window_t Window) {
    auto Surface = vulkan::Globals.Instance
                       .createXcbSurfaceKHRUnique(
                           vk::XcbSurfaceCreateInfoKHR({}, Conn, Window))
                       .value;
    return TargetTraits<VULKAN_SPIRV>::SurfaceOwner{
        std::make_unique<vulkan::SurfaceAllocation>(vulkan::Globals.SurfaceHead,
                                                    std::move(Surface))};
  }
#endif
};
#endif

struct GlobalListNode;
GlobalListNode *GlobalListHead = nullptr;
struct GlobalListNode {
  typedef void (*RegisterFunc)();
  std::array<RegisterFunc, TARGET_MAX> Func;
  GlobalListNode *Next;
  template <typename... Args>
  explicit GlobalListNode(Args... Funcs) noexcept
      : Func{Funcs...}, Next(GlobalListHead) {
    GlobalListHead = this;
  }
};

template <typename... B>
struct PipelineCoordinator {
  static hsh::detail::GlobalListNode global;
  template <hsh::Target T>
  static void global_build() {
    PipelineBuilder<T>::template build_pipelines<B...>(
        std::make_index_sequence<sizeof...(B)>());
  }
};

/*
 * This macro is internally expanded within the hsh generator
 * for any identifiers prefixed with hsh_ being assigned or returned.
 */
#define _hsh_dummy(...) ::hsh::binding_typeless{};
} // namespace detail

#define HSH_PROFILE_MODE 1

template <typename T>
struct resource_owner {
  typename decltype(T::Binding)::Owner Owner;
  using MappedType = typename T::MappedType;

  resource_owner(const resource_owner& other) = delete;
  resource_owner &operator=(const resource_owner& other) = delete;
  resource_owner(resource_owner &&other) = default;
  resource_owner &operator=(resource_owner&& other) = default;

  T get() const { return T(typename decltype(Owner)::Binding(Owner)); }
  operator T() const { return get(); }
  MappedType *map() noexcept {
    return reinterpret_cast<MappedType *>(Owner.map());
  }
  void unmap() noexcept { Owner.unmap(); }
  template <typename LoadType = MappedType,
            typename std::enable_if_t<!std::is_void_v<LoadType>, int> = 0>
  void load(const LoadType &obj) {
    auto *ptr = map();
    std::memcpy(ptr, &obj, sizeof(LoadType));
    unmap();
  }
  template <typename LoadType = MappedType,
            typename std::enable_if_t<
                !std::is_void_v<LoadType> &&
                    std::is_base_of_v<dynamic_vertex_buffer_typeless, T>,
                int> = 0>
  void load(detail::ArrayProxy<LoadType> obj) {
    auto *ptr = map();
    std::memcpy(ptr, obj.data(), sizeof(LoadType) * obj.size());
    unmap();
  }
  template <typename LoadType = MappedType, std::size_t N,
            typename std::enable_if_t<
                !std::is_void_v<LoadType> &&
                    std::is_base_of_v<dynamic_vertex_buffer_typeless, T>,
                int> = 0>
  void load(const std::array<LoadType, N> &Arr) {
    load(detail::ArrayProxy<T>(Arr));
  }
};

template <typename T, typename... Args>
inline resource_owner<T> create_resource(Args... args) {
  if constexpr (detail::NumStaticallyActiveTargets == 1) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  return resource_owner<T>{                                                    \
      decltype(detail::ActiveTargetTraits::ResourceFactory<                    \
               T>::_##Enumeration)::Create(args...)};
#include "targets.def"
  } else {
    switch (detail::ActiveTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Enumeration:                                                            \
    return resource_owner<T>{                                                  \
        decltype(detail::ActiveTargetTraits::ResourceFactory<                  \
                 T>::_##Enumeration)::Create(args...)};
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
    return {};
  }
}

template <typename T, typename CopyFunc>
inline resource_owner<uniform_buffer<T>>
create_uniform_buffer(CopyFunc copyFunc) {
  return create_resource<uniform_buffer<T>>(copyFunc);
}

template <typename T>
inline resource_owner<uniform_buffer<T>> create_uniform_buffer(const T &data) {
  return create_resource<uniform_buffer<T>>(
      [&](void *buf, std::size_t size) { std::memcpy(buf, &data, sizeof(T)); });
}

template <typename T>
inline resource_owner<dynamic_uniform_buffer<T>>
create_dynamic_uniform_buffer() {
  return create_resource<dynamic_uniform_buffer<T>>();
}

template <typename T>
inline resource_owner<dynamic_uniform_buffer<T>>
create_dynamic_uniform_buffer(const T &data) {
  auto ret = create_resource<dynamic_uniform_buffer<T>>();
  ret.load(data);
  return ret;
}

template <typename T>
inline resource_owner<vertex_buffer<T>>
create_vertex_buffer(detail::ArrayProxy<T> data) {
  return create_resource<vertex_buffer<T>>(
      data.size(), [&](void *buf, std::size_t size) {
        std::memcpy(buf, data.data(), sizeof(T) * data.size());
      });
}

template <typename T, std::size_t N>
inline resource_owner<vertex_buffer<T>>
create_vertex_buffer(const std::array<T, N> &Arr) {
  return create_vertex_buffer(detail::ArrayProxy<T>(Arr));
}

template <typename T>
inline resource_owner<dynamic_vertex_buffer<T>>
create_dynamic_vertex_buffer() {
  return create_resource<dynamic_vertex_buffer<T>>();
}

template <typename T>
inline resource_owner<dynamic_vertex_buffer<T>>
create_dynamic_vertex_buffer(detail::ArrayProxy<T> data) {
  auto ret = create_resource<dynamic_vertex_buffer<T>>();
  ret.load(data);
  return ret;
}

template <typename T, std::size_t N>
inline resource_owner<dynamic_vertex_buffer<T>>
create_dynamic_vertex_buffer(const std::array<T, N> &Arr) {
  auto ret = create_resource<dynamic_vertex_buffer<T>>();
  ret.load(Arr);
  return ret;
}

template <typename TexelType, typename CopyFunc>
inline resource_owner<texture2d<TexelType>>
create_texture2d(uint32_t width, uint32_t height, Format format,
                 uint32_t numMips, CopyFunc copyFunc,
                 ColorComponentFlags redSwizzle = Red,
                 ColorComponentFlags greenSwizzle = Green,
                 ColorComponentFlags blueSwizzle = Blue,
                 ColorComponentFlags alphaSwizzle = Alpha) {
  return create_resource<texture2d<TexelType>>(
      width, height, format, numMips, copyFunc, redSwizzle, greenSwizzle,
      blueSwizzle, alphaSwizzle);
}

inline resource_owner<render_texture2d>
create_render_texture2d(surface Surf, uint32_t NumColorBindings,
                        uint32_t NumDepthBindings) {
  return create_resource<render_texture2d>(Surf, NumColorBindings,
                                           NumDepthBindings);
}

#ifdef VK_USE_PLATFORM_XCB_KHR
inline resource_owner<surface>
create_surface(xcb_connection_t* Conn, xcb_window_t Window) {
  return create_resource<surface>(Conn, Window);
}
#endif

class binding_typeless {
protected:
  detail::ActiveTargetTraits::PipelineBinding Data;
  template <typename... Args>
  explicit binding_typeless(Args... args) : Data(args...) {}
public:
  binding_typeless() = default;
};

template <typename Impl>
class binding : public binding_typeless {
protected:
  template <typename... Args>
  explicit binding(Args... args)
  : binding_typeless(detail::ClassWrapper<Impl>(), args...) {}
};

#if HSH_PROFILE_MODE
struct value_formatter {
  template <typename T>
  static std::ostream &format(std::ostream &out, T val) {
    return out << val;
  }
};

class profiler {
  friend class profile_context;
  const char *source = nullptr;
public:
  struct push {
    const char *name;
    explicit push(const char *name) : name(name) {}
  };
  struct pop {};
  struct cast_base {};
  template <typename T>
  struct cast : cast_base {
    const char *type;
    T val;
    explicit cast(const char *type, T val) : type(type), val(val) {}
  };
private:
  template <typename T>
  using EnableIfNonControlArg =
  std::enable_if_t<!std::is_same_v<T, push> && !std::is_same_v<T, pop> &&
                   !std::is_same_v<T, const char *> && !std::is_base_of_v<cast_base, T>,
      int>;
  struct node {
    std::map<std::string, node> children;
    std::string leaf;
    node &get() {
      return *this;
    }
    template <typename... Args>
    node &get(push, Args... rest) {
      return get(rest...);
    }
    template <typename... Args>
    node &get(pop, Args... rest) {
      return get(rest...);
    }
    template <typename... Args>
    node &get(const char *arg, Args... rest) {
      return get(rest...);
    }
    template <typename T, typename... Args>
    node &get(cast<T> arg, Args... rest) {
      std::ostringstream ss;
      hsh::value_formatter::format(ss, arg.val);
      return children[ss.str()].get(rest...);
    }
    template <typename T, typename... Args, EnableIfNonControlArg<T> = 0>
    node &get(T arg, Args... rest) {
      std::ostringstream ss;
      hsh::value_formatter::format(ss, arg);
      return children[ss.str()].get(rest...);
    }
    void write(std::ostream &out, const char *src, unsigned &idx) const {
      if (!children.empty()) {
        for (auto [key, node] : children)
          node.write(out, src, idx);
      } else {
        out << "using s" << idx++ << " = " << src << leaf << ";\n";
      }
    }
  } root;
  static void do_format_param(std::ostream &out, push p) {
    out << p.name << "<";
  }
  static void do_format_param(std::ostream &out, pop p) {
    out << ">";
  }
  static void do_format_param(std::ostream &out, const char *arg) {
    out << arg;
  }
  template <typename T>
  static void do_format_param(std::ostream &out, cast<T> arg) {
    out << arg.type << '(';
    hsh::value_formatter::format(out, arg.val);
    out << ')';
  }
  template <typename T, EnableIfNonControlArg<T> = 0>
  static void do_format_param(std::ostream &out, T arg) {
    hsh::value_formatter::format(out, arg);
  }
  static void format_param_next(std::ostream &out) {}
  template <typename T>
  static void format_param_next(std::ostream &out, T arg) {
    out << ", ";
    do_format_param(out, arg);
  }
  template <typename T, typename... Args>
  static void format_params(std::ostream &out, T arg, Args... rest) {
    do_format_param(out, arg);
    (format_param_next(out, rest), ...);
  }
  void write_header(std::ostream &out) const {
    unsigned idx = 0;
    root.write(out, source, idx);
  }
public:
  template <typename... Args>
  void add(Args... args) {
    node &n = root.get(args...);
    std::ostringstream ss;
    ss << '<';
    format_params(ss, args...);
    ss << '>';
    n.leaf = ss.str();
  }
};

class profile_context {
  struct File {
    const char *fwds = nullptr;
    std::unordered_map<std::string, profiler> profilers;
  };
  std::unordered_map<std::string, File> files;
public:
  static profile_context instance;
  profiler &get(const char *filename, const char *fwds, const char *binding, const char *source) {
    auto &file = files[filename];
    file.fwds = fwds;
    auto &ret = file.profilers[binding];
    ret.source = source;
    return ret;
  }
  void write_headers() {
    for (auto &[filename, file] : files) {
      std::ofstream out(filename);
      if (!out.is_open()) {
        std::cerr << "Unable to open '" << filename << "' for writing\n";
        continue;
      }
      out << file.fwds;
      for (auto &[binding, prof] : file.profilers) {
        out << "namespace " << binding << "_specializations {\n";
        prof.write_header(out);
        out << "}\n";
      }
    }
  }
};

profile_context profile_context::instance{};
#endif
} // namespace hsh
