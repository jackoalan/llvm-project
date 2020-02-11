#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <utility>

#if !defined(NDEBUG)
#if __has_include(<source_location>)
#include <source_location>
#define HSH_SOURCE_LOCATION_REP std::source_location
#elif __has_include(<experimental/source_location>)
#include <experimental/source_location>
#define HSH_SOURCE_LOCATION_REP std::experimental::source_location
#endif
#endif
#ifdef HSH_SOURCE_LOCATION_REP
namespace hsh {
class SourceLocation {
  HSH_SOURCE_LOCATION_REP m_location;
  const char *m_field = nullptr;
  std::uint32_t m_fieldIdx = UINT32_MAX;

public:
  SourceLocation(const HSH_SOURCE_LOCATION_REP &location,
                 const char *field = nullptr,
                 std::uint32_t fieldIdx = UINT32_MAX) noexcept
      : m_location(location), m_field(field), m_fieldIdx(fieldIdx) {}
  SourceLocation with_field(const char *f, std::uint32_t idx = UINT32_MAX) const
      noexcept {
    return SourceLocation(m_location, f, idx);
  }
  static SourceLocation current(const char *f = nullptr,
                                std::uint32_t idx = UINT32_MAX) noexcept {
    return SourceLocation(HSH_SOURCE_LOCATION_REP::current(), f, idx);
  }
  const char *file_name() const noexcept { return m_location.file_name(); }
  std::uint_least32_t line() const noexcept { return m_location.line(); }
  const char *function_name() const noexcept {
    return m_location.function_name();
  }
  bool has_field() const noexcept { return m_field != nullptr; }
  const char *field() const noexcept { return m_field; }
  bool has_field_idx() const noexcept { return m_fieldIdx != UINT32_MAX; }
  std::uint32_t field_idx() const noexcept { return m_fieldIdx; }

  std::string to_string() const noexcept {
    std::ostringstream ss;
    ss << file_name() << ':' << line() << ' ' << function_name();
    if (has_field()) {
      ss << " (" << field() << ')';
      if (has_field_idx())
        ss << '[' << field_idx() << ']';
    }
    return ss.str();
  }
};
} // namespace hsh
#undef HSH_SOURCE_LOCATION_REP
#define HSH_SOURCE_LOCATION_ENABLED 1
#else
namespace hsh {
class SourceLocation {
public:
  static SourceLocation current() noexcept { return {}; }
};
} // namespace hsh
#define HSH_SOURCE_LOCATION_ENABLED 0
#endif

#if !defined(NDEBUG) && defined(__GXX_RTTI)
#define HSH_ASSERT_CAST_ENABLED 1
#define HSH_ASSERT_CAST(...) assert(__VA_ARGS__)
#else
#define HSH_ASSERT_CAST_ENABLED 0
#define HSH_ASSERT_CAST(...)
#endif

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
} // namespace hsh::detail

#define HSH_ENABLE_LOG 1
#define HSH_ENABLE_VULKAN 1

#if HSH_ENABLE_VULKAN
inline void hshVkAssert(const char *pred) noexcept {
  std::cerr << pred << " failed\n";
  std::abort();
}
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_NO_EXCEPTIONS
#define VK_USE_PLATFORM_XCB_KHR
#define VULKAN_HPP_ASSERT(pred)                                                \
  if (!(pred))                                                                 \
  hshVkAssert(#pred)
#include <vulkan/vulkan.hpp>

#define VMA_USE_STL_CONTAINERS 1
#define VMA_USE_STL_SHARED_MUTEX 1
#define VMA_ASSERT(pred)                                                       \
  if (!(pred))                                                                 \
  hshVkAssert(#pred)
#include "vk_mem_alloc_hsh.h"

namespace hsh {
struct Offset2D {
  int32_t x, y;
  constexpr Offset2D(int32_t x, int32_t y) noexcept : x(x), y(y) {}
#if HSH_ENABLE_VULKAN
  operator vk::Offset2D() const noexcept { return vk::Offset2D(x, y); }
#endif
};
struct Extent2D {
  uint32_t w, h;
  constexpr Extent2D(uint32_t w, uint32_t h) noexcept : w(w), h(h) {}
#if HSH_ENABLE_VULKAN
  operator vk::Extent2D() const noexcept { return vk::Extent2D(w, h); }
#endif
};
struct Rect2D {
  Offset2D offset;
  Extent2D extent;
  constexpr Rect2D(Offset2D offset, Extent2D extent) noexcept
      : offset(offset), extent(extent) {}
#if HSH_ENABLE_VULKAN
  operator vk::Rect2D() const noexcept { return vk::Rect2D(offset, extent); }
#endif
};
} // namespace hsh

namespace VULKAN_HPP_NAMESPACE {
template <> struct ObjectDestroy<Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> {
public:
  ObjectDestroy() noexcept = default;
  inline ObjectDestroy(
      Instance owner, Optional<const AllocationCallbacks> allocationCallbacks,
      VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT;

protected:
  template <typename T> void destroy(T t) VULKAN_HPP_NOEXCEPT;
};
template <> struct ObjectDestroy<Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> {
public:
  ObjectDestroy() noexcept = default;
  inline ObjectDestroy(
      Device owner, Optional<const AllocationCallbacks> allocationCallbacks,
      VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT;

protected:
  template <typename T> void destroy(T t) VULKAN_HPP_NOEXCEPT;
};
} // namespace VULKAN_HPP_NAMESPACE

namespace hsh::detail::vulkan {
class BufferAllocation {
  friend class DeletedBufferAllocation;
  friend BufferAllocation
  AllocateStaticBuffer(const SourceLocation &location, vk::DeviceSize size,
                       vk::BufferUsageFlags usage) noexcept;

protected:
  vk::Buffer Buffer;
  VmaAllocation Allocation = VK_NULL_HANDLE;
  BufferAllocation(vk::Buffer Buffer, VmaAllocation Allocation) noexcept
      : Buffer(Buffer), Allocation(Allocation) {}

public:
  BufferAllocation() noexcept = default;
  BufferAllocation(const BufferAllocation &other) = delete;
  BufferAllocation &operator=(const BufferAllocation &other) = delete;
  BufferAllocation(BufferAllocation &&other) noexcept {
    std::swap(Buffer, other.Buffer);
    std::swap(Allocation, other.Allocation);
  }
  BufferAllocation &operator=(BufferAllocation &&other) noexcept {
    std::swap(Buffer, other.Buffer);
    std::swap(Allocation, other.Allocation);
    return *this;
  }
  inline ~BufferAllocation() noexcept;
  vk::Buffer getBuffer() const noexcept { return Buffer; }
  operator vk::Buffer() const noexcept { return getBuffer(); }
  bool isValid() const noexcept { return Buffer.operator bool(); }
};

class DynamicBufferBinding {
  friend class DynamicBufferAllocation;
  vk::Buffer Buffer;
  vk::DeviceSize SecondOffset;
  DynamicBufferBinding(vk::Buffer Buffer, vk::DeviceSize SecondOffset) noexcept
      : Buffer(Buffer), SecondOffset(SecondOffset) {}

public:
  DynamicBufferBinding() noexcept = default;
  vk::Buffer getBuffer() const noexcept { return Buffer; }
  vk::DeviceSize getSecondOffset() const noexcept { return SecondOffset; }
  bool isValid() const noexcept { return Buffer.operator bool(); }
};

class DynamicBufferAllocation : public BufferAllocation {
  friend DynamicBufferAllocation
  AllocateDynamicBuffer(const SourceLocation &location, vk::DeviceSize size,
                        vk::BufferUsageFlags usage) noexcept;
  void *MappedData = nullptr;
  vk::DeviceSize SecondOffset = 0;
  DynamicBufferAllocation(vk::Buffer Buffer, VmaAllocation Allocation,
                          void *MappedData,
                          vk::DeviceSize SecondOffset) noexcept
      : BufferAllocation(Buffer, Allocation), MappedData(MappedData),
        SecondOffset(SecondOffset) {}

public:
  DynamicBufferAllocation() noexcept = default;
  DynamicBufferBinding getBinding() const noexcept {
    return DynamicBufferBinding(Buffer, SecondOffset);
  }
  operator DynamicBufferBinding() const noexcept { return getBinding(); }
  inline vk::DeviceSize getOffset() const noexcept;
  vk::DescriptorBufferInfo getDescriptorBufferInfo() const noexcept {
    return {Buffer, 0, SecondOffset};
  }
  void *map() noexcept {
    return reinterpret_cast<uint8_t *>(MappedData) + getOffset();
  }
  inline void unmap() noexcept;
};

class UploadBufferAllocation : public BufferAllocation {
  friend UploadBufferAllocation
  AllocateUploadBuffer(const SourceLocation &location,
                       vk::DeviceSize size) noexcept;
  void *MappedData;
  UploadBufferAllocation(vk::Buffer Buffer, VmaAllocation Allocation,
                         void *MappedData) noexcept
      : BufferAllocation(Buffer, Allocation), MappedData(MappedData) {}

public:
  void *getMappedData() const noexcept { return MappedData; }
};

struct TextureAllocation {
  friend class DeletedTextureAllocation;
  friend TextureAllocation
  AllocateTexture(const SourceLocation &location,
                  const vk::ImageCreateInfo &CreateInfo,
                  bool Dedicated) noexcept;

protected:
  vk::Image Image;
  VmaAllocation Allocation = VK_NULL_HANDLE;
  TextureAllocation(vk::Image Image, VmaAllocation Allocation) noexcept
      : Image(Image), Allocation(Allocation) {}

public:
  TextureAllocation() noexcept = default;
  TextureAllocation(const TextureAllocation &other) = delete;
  TextureAllocation &operator=(const TextureAllocation &other) = delete;
  TextureAllocation(TextureAllocation &&other) noexcept {
    std::swap(Image, other.Image);
    std::swap(Allocation, other.Allocation);
  }
  TextureAllocation &operator=(TextureAllocation &&other) noexcept {
    std::swap(Image, other.Image);
    std::swap(Allocation, other.Allocation);
    return *this;
  }
  inline ~TextureAllocation() noexcept;
  vk::Image GetImage() const noexcept { return Image; }
};

class SurfaceAllocation {
  friend class RenderTextureAllocation;
  SurfaceAllocation *Prev = nullptr, *Next = nullptr;
  SourceLocation Location;
  vk::UniqueSurfaceKHR Surface;
  vk::UniqueSwapchainKHR Swapchain;
  vk::UniqueRenderPass OwnedRenderPass;
  std::vector<vk::Image> SwapchainImages;
  vk::Extent2D Extent;
  vk::Format ColorFormat = vk::Format::eUndefined;
  uint32_t NextImage = UINT32_MAX;

public:
  inline ~SurfaceAllocation() noexcept;
  inline explicit SurfaceAllocation(const SourceLocation &location,
                                    vk::UniqueSurfaceKHR &&Surface) noexcept;
  SurfaceAllocation(const SurfaceAllocation &other) = delete;
  SurfaceAllocation &operator=(const SurfaceAllocation &other) = delete;
  SurfaceAllocation(SurfaceAllocation &&other) noexcept = delete;
  SurfaceAllocation &operator=(SurfaceAllocation &&other) noexcept = delete;
  inline bool PreRender() noexcept;
  inline bool AcquireNextImage() noexcept;
  inline void PostRender() noexcept;
  const vk::Extent2D &GetExtent() const noexcept { return Extent; }
  vk::Format GetColorFormat() const noexcept { return ColorFormat; }
  SurfaceAllocation *GetNext() const noexcept { return Next; }
};

class RenderTextureAllocation {
  friend class DeletedRenderTextureAllocation;
  RenderTextureAllocation *Prev = nullptr, *Next = nullptr;
  SourceLocation Location;
  SurfaceAllocation *Surface;
  vk::Extent2D Extent;
  vk::Format ColorFormat = vk::Format::eUndefined;
  uint32_t NumColorBindings, NumDepthBindings;
  TextureAllocation ColorTexture;
  vk::UniqueImageView ColorView;
  TextureAllocation DepthTexture;
  vk::UniqueImageView DepthView;
  vk::UniqueFramebuffer Framebuffer;
  struct RenderPassBeginInfo : vk::RenderPassBeginInfo {
    std::array<vk::ClearValue, 2> ClearValues;
    RenderPassBeginInfo() = default;
    RenderPassBeginInfo(vk::RenderPass RenderPass, vk::Framebuffer Framebuffer,
                        vk::Extent2D Extent) noexcept
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
        : vk::RenderPassBeginInfo(RenderPass, Framebuffer,
                                  vk::Rect2D({}, Extent), ClearValues.size(),
                                  ClearValues.data()),
#pragma GCC diagnostic pop
          ClearValues{vk::ClearColorValue(), vk::ClearDepthStencilValue()} {
    }
  } RenderPassBegin;
  struct Binding {
    TextureAllocation Texture;
    vk::UniqueImageView ImageView;
  };
  std::array<Binding, MaxRenderTextureBindings> ColorBindings;
  std::array<Binding, MaxRenderTextureBindings> DepthBindings;
  bool FirstAttach = false;
  inline void Prepare() noexcept;
  static inline void _Resolve(vk::Image SrcImage, vk::Image DstImage,
                              vk::ImageAspectFlagBits Aspect,
                              vk::Offset3D Offset,
                              vk::Extent3D Extent) noexcept;
  inline void Resolve(vk::Image SrcImage, vk::Image DstImage,
                      vk::ImageAspectFlagBits Aspect, vk::Offset3D Offset,
                      vk::Extent3D Extent, bool Reattach) noexcept;

public:
  inline ~RenderTextureAllocation() noexcept;
  inline explicit RenderTextureAllocation(const SourceLocation &location,
                                          SurfaceAllocation *Surface,
                                          uint32_t NumColorBindings,
                                          uint32_t NumDepthBindings) noexcept;
  inline explicit RenderTextureAllocation(const SourceLocation &location,
                                          Extent2D extent,
                                          vk::Format colorFormat,
                                          uint32_t NumColorBindings,
                                          uint32_t NumDepthBindings) noexcept;
  RenderTextureAllocation(const RenderTextureAllocation &other) = delete;
  RenderTextureAllocation &operator=(const TextureAllocation &other) = delete;
  RenderTextureAllocation(RenderTextureAllocation &&other) noexcept = delete;
  RenderTextureAllocation &
  operator=(RenderTextureAllocation &&other) noexcept = delete;
  void PreRender() noexcept {
    if (Surface) {
      auto &SurfAlloc = *Surface;
      if (SurfAlloc.GetExtent() != Extent ||
          SurfAlloc.GetColorFormat() != ColorFormat) {
        Extent = SurfAlloc.GetExtent();
        ColorFormat = SurfAlloc.GetColorFormat();
        Prepare();
      }
    }
  }
  inline void BeginRenderPass() noexcept;
  inline void ResolveSurface(SurfaceAllocation *Surface,
                             bool Reattach) noexcept;
  inline void ResolveColorBinding(uint32_t Idx, Rect2D Region,
                                  bool Reattach) noexcept;
  inline void ResolveDepthBinding(uint32_t Idx, Rect2D Region,
                                  bool Reattach) noexcept;
  vk::ImageView GetColorBindingView(uint32_t Idx) const noexcept {
    assert(Idx < NumColorBindings);
    return ColorBindings[Idx].ImageView.get();
  }
  vk::ImageView GetDepthBindingView(uint32_t Idx) const noexcept {
    assert(Idx < NumDepthBindings);
    return DepthBindings[Idx].ImageView.get();
  }
  inline void Attach() noexcept;
  RenderTextureAllocation *GetNext() const noexcept { return Next; }
};

class DeletedBufferAllocation {
  vk::Buffer Buffer;
  VmaAllocation Allocation = VK_NULL_HANDLE;

public:
  explicit DeletedBufferAllocation(BufferAllocation &&Obj) noexcept {
    std::swap(Buffer, Obj.Buffer);
    std::swap(Allocation, Obj.Allocation);
  }
  DeletedBufferAllocation &operator=(BufferAllocation &&Obj) noexcept {
    std::swap(Buffer, Obj.Buffer);
    std::swap(Allocation, Obj.Allocation);
    return *this;
  }
  DeletedBufferAllocation(const DeletedBufferAllocation &other) = delete;
  DeletedBufferAllocation &
  operator=(const DeletedBufferAllocation &other) = delete;
  DeletedBufferAllocation(DeletedBufferAllocation &&other) noexcept {
    std::swap(Buffer, other.Buffer);
    std::swap(Allocation, other.Allocation);
  }
  DeletedBufferAllocation &operator=(DeletedBufferAllocation &&other) noexcept {
    std::swap(Buffer, other.Buffer);
    std::swap(Allocation, other.Allocation);
    return *this;
  }
  inline ~DeletedBufferAllocation() noexcept;
};

class DeletedTextureAllocation {
  vk::Image Image;
  VmaAllocation Allocation = VK_NULL_HANDLE;

public:
  DeletedTextureAllocation(TextureAllocation &&other) noexcept {
    std::swap(Image, other.Image);
    std::swap(Allocation, other.Allocation);
  }
  DeletedTextureAllocation(const DeletedTextureAllocation &other) = delete;
  DeletedTextureAllocation &
  operator=(const DeletedTextureAllocation &other) = delete;
  DeletedTextureAllocation(DeletedTextureAllocation &&other) noexcept {
    std::swap(Image, other.Image);
    std::swap(Allocation, other.Allocation);
  }
  DeletedTextureAllocation &
  operator=(DeletedTextureAllocation &&other) noexcept {
    std::swap(Image, other.Image);
    std::swap(Allocation, other.Allocation);
    return *this;
  }
  inline ~DeletedTextureAllocation() noexcept;
};

class DeletedRenderTextureAllocation {
  DeletedTextureAllocation ColorTexture;
  vk::UniqueImageView ColorView;
  DeletedTextureAllocation DepthTexture;
  vk::UniqueImageView DepthView;
  vk::UniqueFramebuffer Framebuffer;
  struct Binding {
    DeletedTextureAllocation Texture;
    vk::UniqueImageView ImageView;
    Binding(RenderTextureAllocation::Binding &&Obj) noexcept
        : Texture(std::move(Obj.Texture)), ImageView(std::move(Obj.ImageView)) {
    }
  };
  std::array<Binding, MaxRenderTextureBindings> ColorBindings;
  std::array<Binding, MaxRenderTextureBindings> DepthBindings;

public:
  template <std::size_t... CSeq, std::size_t... DSeq>
  explicit DeletedRenderTextureAllocation(RenderTextureAllocation &&Obj,
                                          std::index_sequence<CSeq...>,
                                          std::index_sequence<DSeq...>) noexcept
      : ColorTexture(std::move(Obj.ColorTexture)),
        ColorView(std::move(Obj.ColorView)),
        DepthTexture(std::move(Obj.DepthTexture)),
        DepthView(std::move(Obj.DepthView)),
        Framebuffer(std::move(Obj.Framebuffer)),
        ColorBindings{std::move(Obj.ColorBindings[CSeq])...},
        DepthBindings{std::move(Obj.DepthBindings[DSeq])...} {}
  explicit DeletedRenderTextureAllocation(
      RenderTextureAllocation &&Obj) noexcept
      : DeletedRenderTextureAllocation(
            std::move(Obj),
            std::make_index_sequence<MaxRenderTextureBindings>(),
            std::make_index_sequence<MaxRenderTextureBindings>()) {}

  DeletedRenderTextureAllocation(const DeletedRenderTextureAllocation &other) =
      delete;
  DeletedRenderTextureAllocation &
  operator=(const DeletedRenderTextureAllocation &other) = delete;
  DeletedRenderTextureAllocation(
      DeletedRenderTextureAllocation &&other) noexcept = default;
  DeletedRenderTextureAllocation &
  operator=(DeletedRenderTextureAllocation &&other) noexcept = default;
};

class DeletedResources {
  std::vector<DeletedBufferAllocation> Buffers;
  std::vector<DeletedTextureAllocation> Textures;
  std::vector<DeletedRenderTextureAllocation> RenderTextures;

public:
  void DeleteLater(BufferAllocation &&Obj) noexcept {
    Buffers.emplace_back(std::move(Obj));
  }
  void DeleteLater(TextureAllocation &&Obj) noexcept {
    Textures.emplace_back(std::move(Obj));
  }
  void DeleteLater(RenderTextureAllocation &&Obj) noexcept {
    RenderTextures.emplace_back(std::move(Obj));
  }
  void Purge() noexcept {
    Buffers.clear();
    Textures.clear();
    RenderTextures.clear();
  }
  DeletedResources() noexcept = default;
  DeletedResources(const DeletedResources &) = delete;
  DeletedResources &operator=(const DeletedResources &) = delete;
};

struct VulkanGlobals {
  vk::Instance Instance;
  vk::PhysicalDevice PhysDevice;
  vk::Device Device;
  vk::VmaAllocator Allocator;
  std::array<vk::DescriptorSetLayout, 64> DescriptorSetLayout;
  vk::PipelineLayout PipelineLayout;
  struct DescriptorPoolChain *DescriptorPoolChain = nullptr;
  vk::Semaphore ImageAcquireSem;
  vk::Semaphore RenderCompleteSem;
  uint32_t QueueFamilyIdx = 0;
  vk::Queue Queue;
  vk::PipelineMultisampleStateCreateInfo MultisampleState{
      {}, vk::SampleCountFlagBits::e1};
  vk::RenderPass RenderPass;
  float Anisotropy = 0.f;
  unsigned DynamicBufferIndex = 0;
  vk::DeviceSize DynamicBufferMask = 0;
  std::vector<vk::UniqueCommandBuffer> *CommandBuffers = nullptr;
  std::array<vk::UniqueFence, 2> *CommandFences = nullptr;
  vk::CommandBuffer Cmd;
  vk::Fence CmdFence;
  vk::Pipeline BoundPipeline;
  vk::DescriptorSet BoundDescriptorSet;
  RenderTextureAllocation *AttachedRenderTexture = nullptr;
  uint64_t Frame = 0;

  std::array<DeletedResources, 2> *DeletedResourcesArr;
  DeletedResources *DeletedResources = nullptr;
  SurfaceAllocation *SurfaceHead = nullptr;
  RenderTextureAllocation *RenderTextureHead = nullptr;

  void PreRender() noexcept {
    uint32_t CurBufferIdx = Frame & 1u;
    Cmd = (*CommandBuffers)[CurBufferIdx].get();
    CmdFence = (*CommandFences)[CurBufferIdx].get();
    Device.waitForFences(CmdFence, VK_TRUE, 500000000);
    DynamicBufferIndex = CurBufferIdx;
    DynamicBufferMask = CurBufferIdx ? ~VkDeviceSize(0) : 0;
    DeletedResources = &(*DeletedResourcesArr)[CurBufferIdx];
    DeletedResources->Purge();

    for (auto *Surf = SurfaceHead; Surf; Surf = Surf->GetNext())
      Surf->PreRender();
    for (auto *RT = RenderTextureHead; RT; RT = RT->GetNext())
      RT->PreRender();

    Cmd.begin(vk::CommandBufferBeginInfo(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
  }

  void PostRender() noexcept {
    if (AttachedRenderTexture) {
      AttachedRenderTexture = nullptr;
      Cmd.endRenderPass();
    }
    BoundPipeline = vk::Pipeline{};
    BoundDescriptorSet = vk::DescriptorSet{};
    Cmd.end();

    vk::PipelineStageFlags pipeStageFlags =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    Device.resetFences(CmdFence);
    Queue.submit(vk::SubmitInfo(1, &ImageAcquireSem, &pipeStageFlags, 1, &Cmd,
                                1, &RenderCompleteSem),
                 CmdFence);
    for (auto *Surf = SurfaceHead; Surf; Surf = Surf->GetNext())
      Surf->PostRender();
    ++Frame;
  }

  void SetDescriptorSetLayout(vk::DescriptorSetLayout Layout) noexcept {
    std::fill(DescriptorSetLayout.begin(), DescriptorSetLayout.end(), Layout);
  }

  vk::RenderPass GetRenderPass() const noexcept {
    assert(RenderPass && "No surfaces created yet");
    return RenderPass;
  }

  bool CheckSurfaceSupported(vk::SurfaceKHR Surface) const noexcept {
    return PhysDevice.getSurfaceSupportKHR(QueueFamilyIdx, Surface).value ==
           VK_TRUE;
  }

#if HSH_SOURCE_LOCATION_ENABLED
  template <typename T>
  void SetDebugObjectName(const char *location, T handle) noexcept {
    Device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT(
        T::objectType, uint64_t(typename T::CType(handle)), location));
  }
  template <typename T>
  void SetDebugObjectName(const SourceLocation &location, T handle) noexcept {
    SetDebugObjectName(location.to_string().c_str(), handle);
  }
#else
  template <typename T>
  static void SetDebugObjectName(const char *location, T handle) noexcept {}
  template <typename T>
  static void SetDebugObjectName(const SourceLocation &location,
                                 T handle) noexcept {}
#endif
};
inline VulkanGlobals Globals;

bool SurfaceAllocation::PreRender() noexcept {
  auto Capabilities =
      Globals.PhysDevice.getSurfaceCapabilitiesKHR(Surface.get()).value;
  if (!Swapchain || Capabilities.currentExtent != Extent) {
    struct SwapchainCreateInfo : vk::SwapchainCreateInfoKHR {
      explicit SwapchainCreateInfo(vk::PhysicalDevice PD,
                                   const vk::SurfaceCapabilitiesKHR &SC,
                                   vk::SurfaceKHR Surface, vk::Format &FmtOut,
                                   vk::SwapchainKHR OldSwapchain) noexcept
          : vk::SwapchainCreateInfoKHR({}, Surface) {
        setMinImageCount(std::max(2u, SC.minImageCount));
        vk::SurfaceFormatKHR UseFormat;
        if (FmtOut == vk::Format::eUndefined)
          FmtOut = vk::Format::eB8G8R8A8Unorm;
        for (auto &Format : PD.getSurfaceFormatsKHR(Surface).value) {
          if (Format.format == FmtOut) {
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
        setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        setPresentMode(vk::PresentModeKHR::eFifo);
        setOldSwapchain(OldSwapchain);
      }
    };
    Swapchain = Globals.Device
                    .createSwapchainKHRUnique(SwapchainCreateInfo(
                        Globals.PhysDevice, Capabilities, Surface.get(),
                        ColorFormat, Swapchain.get()))
                    .value;
    Globals.SetDebugObjectName(Location.with_field("Swapchain"),
                               Swapchain.get());
    SwapchainImages = Globals.Device.getSwapchainImagesKHR(*Swapchain).value;
    Extent = Capabilities.currentExtent;
    VULKAN_HPP_ASSERT((!Next || ColorFormat == Next->ColorFormat) &&
                      "Subsequent surfaces must have the same color format");
    return true;
  }
  return false;
}

bool SurfaceAllocation::AcquireNextImage() noexcept {
  auto ret = Globals.Device.acquireNextImageKHR(
      Swapchain.get(), UINT64_MAX, Globals.ImageAcquireSem, {}, &NextImage);
  return ret == vk::Result::eSuccess;
}

void SurfaceAllocation::PostRender() noexcept {
  if (NextImage != UINT32_MAX) {
    Globals.Queue.presentKHR(vk::PresentInfoKHR(
        1, &Globals.RenderCompleteSem, 1, &Swapchain.get(), &NextImage));
    NextImage = UINT32_MAX;
  }
}

BufferAllocation::~BufferAllocation() noexcept {
  if (Buffer)
    Globals.DeletedResources->DeleteLater(std::move(*this));
}

DeletedBufferAllocation::~DeletedBufferAllocation() noexcept {
  vmaDestroyBuffer(Globals.Allocator, Buffer, Allocation);
}

TextureAllocation::~TextureAllocation() noexcept {
  if (Image)
    Globals.DeletedResources->DeleteLater(std::move(*this));
}

DeletedTextureAllocation::~DeletedTextureAllocation() noexcept {
  vmaDestroyImage(Globals.Allocator, Image, Allocation);
}

SurfaceAllocation::~SurfaceAllocation() noexcept {
  if (OwnedRenderPass) {
    if (Prev)
      Prev->OwnedRenderPass = std::move(OwnedRenderPass);
    else if (Next)
      Next->OwnedRenderPass = std::move(OwnedRenderPass);
  }
  if (Prev) {
    Prev->Next = Next;
  } else {
    Globals.SurfaceHead = Next;
    if (!Globals.SurfaceHead)
      Globals.RenderPass = vk::RenderPass{};
  }
  if (Next)
    Next->Prev = Prev;
}

SurfaceAllocation::SurfaceAllocation(const SourceLocation &location,
                                     vk::UniqueSurfaceKHR &&Surface) noexcept
    : Next(Globals.SurfaceHead), Location(location),
      Surface(std::move(Surface)) {
  Globals.SurfaceHead = this;
  if (Next)
    Next->Prev = this;

  PreRender();

  if (!Globals.RenderPass) {
    struct RenderPassCreateInfo : vk::RenderPassCreateInfo {
      std::array<vk::AttachmentDescription, 2> Attachments;
      std::array<vk::SubpassDescription, 1> Subpasses;
      vk::AttachmentReference ColorRef{
          0, vk::ImageLayout::eColorAttachmentOptimal};
      vk::AttachmentReference DepthRef{
          1, vk::ImageLayout::eDepthStencilAttachmentOptimal};
      constexpr RenderPassCreateInfo(vk::Format colorFormat,
                                     vk::SampleCountFlagBits samples) noexcept
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
          : vk::RenderPassCreateInfo({}, Attachments.size(), Attachments.data(),
                                     Subpasses.size(), Subpasses.data()),
#pragma GCC diagnostic pop
            Attachments{
                vk::AttachmentDescription(
                    {}, colorFormat, samples, vk::AttachmentLoadOp::eLoad,
                    vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare,
                    vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::eColorAttachmentOptimal),
                vk::AttachmentDescription(
                    {}, vk::Format::eD32Sfloat, samples,
                    vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare,
                    vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal)},
            Subpasses{
                vk::SubpassDescription({}, vk::PipelineBindPoint::eGraphics, {},
                                       {}, 1, &ColorRef, {}, &DepthRef)} {
      }
    };
    OwnedRenderPass = Globals.Device
                          .createRenderPassUnique(RenderPassCreateInfo(
                              GetColorFormat(),
                              Globals.MultisampleState.rasterizationSamples))
                          .value;
    Globals.SetDebugObjectName(location.with_field("OwnedRenderPass"),
                               OwnedRenderPass.get());
    Globals.RenderPass = OwnedRenderPass.get();
  }
}

RenderTextureAllocation::~RenderTextureAllocation() noexcept {
  if (Prev)
    Prev->Next = Next;
  else
    Globals.RenderTextureHead = Next;
  if (Next)
    Next->Prev = Prev;
  Globals.DeletedResources->DeleteLater(std::move(*this));
}

RenderTextureAllocation::RenderTextureAllocation(
    const SourceLocation &location, SurfaceAllocation *Surface,
    uint32_t NumColorBindings, uint32_t NumDepthBindings) noexcept
    : Next(Globals.RenderTextureHead), Location(location), Surface(Surface),
      NumColorBindings(NumColorBindings), NumDepthBindings(NumDepthBindings) {
  Globals.RenderTextureHead = this;
  if (Next)
    Next->Prev = this;
  assert(Surface);
  assert(NumColorBindings <= MaxRenderTextureBindings);
  assert(NumDepthBindings <= MaxRenderTextureBindings);
}

RenderTextureAllocation::RenderTextureAllocation(
    const SourceLocation &location, Extent2D extent, vk::Format colorFormat,
    uint32_t NumColorBindings, uint32_t NumDepthBindings) noexcept
    : Next(Globals.RenderTextureHead), Location(location), Extent(extent),
      ColorFormat(colorFormat), NumColorBindings(NumColorBindings),
      NumDepthBindings(NumDepthBindings) {
  Globals.RenderTextureHead = this;
  if (Next)
    Next->Prev = this;
  assert(NumColorBindings <= MaxRenderTextureBindings);
  assert(NumDepthBindings <= MaxRenderTextureBindings);
  Prepare();
}

vk::DeviceSize DynamicBufferAllocation::getOffset() const noexcept {
  return SecondOffset & Globals.DynamicBufferMask;
}

void DynamicBufferAllocation::unmap() noexcept {
  vmaFlushAllocation(Globals.Allocator, Allocation, getOffset(), SecondOffset);
}
} // namespace hsh::detail::vulkan

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
enum ActiveTarget : std::uint8_t {
#define HSH_ACTIVE_TARGET(Enumeration) ACTIVE_##Enumeration,
#include "targets.def"
  ACTIVE_TARGET_MAX
};
struct uniform_buffer_typeless;
struct dynamic_uniform_buffer_typeless;
struct vertex_buffer_typeless;
struct dynamic_vertex_buffer_typeless;
struct texture_typeless;
struct render_texture2d;
} // namespace hsh

namespace hsh::detail {
template <hsh::Target T> struct SamplerObject;
}

namespace hsh::detail::vulkan {

struct DescriptorPoolCreateInfo : vk::DescriptorPoolCreateInfo {
  std::array<vk::DescriptorPoolSize, 3> PoolSizes{
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic,
                             MaxUniforms *MaxDescriptorPoolSets},
      vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage,
                             MaxImages *MaxDescriptorPoolSets},
      vk::DescriptorPoolSize{vk::DescriptorType::eSampler,
                             MaxSamplers *MaxDescriptorPoolSets}};
  constexpr DescriptorPoolCreateInfo() noexcept
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
      : vk::DescriptorPoolCreateInfo({}, MaxDescriptorPoolSets,
                                     PoolSizes.size(), PoolSizes.data()) {
  }
#pragma GCC diagnostic pop
};
struct UniqueDescriptorSet {
  vk::DescriptorSet Set;
  std::uint64_t Index = UINT64_MAX;
  UniqueDescriptorSet() noexcept = default;
  UniqueDescriptorSet(vk::DescriptorSet Set, std::size_t Index) noexcept
      : Set(Set), Index(Index) {}
  UniqueDescriptorSet(const UniqueDescriptorSet &) = delete;
  UniqueDescriptorSet &operator=(const UniqueDescriptorSet &) = delete;
  UniqueDescriptorSet(UniqueDescriptorSet &&Other) noexcept {
    Set = Other.Set;
    std::swap(Index, Other.Index);
  }
  UniqueDescriptorSet &operator=(UniqueDescriptorSet &&Other) noexcept {
    Set = Other.Set;
    std::swap(Index, Other.Index);
    return *this;
  }
  operator vk::DescriptorSet() const noexcept { return Set; }
  inline ~UniqueDescriptorSet() noexcept;
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
          auto Result = Globals.Device.allocateDescriptorSets(
              &AllocateInfo, DescriptorSets.data());
          VULKAN_HPP_ASSERT(Result == vk::Result::eSuccess);
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

inline VkResult vmaCreateAllocator(const VmaAllocatorCreateInfo &pCreateInfo,
                                   VmaAllocator *pAllocator) noexcept {
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

inline VkResult
vmaCreateDoubleBuffer(const vk::BufferCreateInfo &pBufferCreateInfo,
                      const VmaAllocationCreateInfo &pAllocationCreateInfo,
                      VkBuffer *pBuffer, VmaAllocation *pAllocation,
                      VmaAllocationInfo *pAllocationInfo,
                      vk::DeviceSize *secondOffset) noexcept {
  return ::vmaCreateDoubleBuffer(
      Globals.Allocator,
      reinterpret_cast<const VkBufferCreateInfo *>(&pBufferCreateInfo),
      reinterpret_cast<const VmaAllocationCreateInfo *>(&pAllocationCreateInfo),
      pBuffer, pAllocation, pAllocationInfo, secondOffset);
}

#if HSH_SOURCE_LOCATION_ENABLED
class VmaLocationStrSetter {
  std::string LocationStr;

public:
  VmaLocationStrSetter(VmaAllocationCreateInfo &CreateInfo,
                       const SourceLocation &Location) noexcept
      : LocationStr(Location.to_string()) {
    CreateInfo.flags |= VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    CreateInfo.pUserData = (void *)LocationStr.c_str();
  }
  operator const char *() const noexcept { return LocationStr.c_str(); }
  VmaLocationStrSetter(const VmaLocationStrSetter &) = delete;
  VmaLocationStrSetter &operator=(const VmaLocationStrSetter &) = delete;
  VmaLocationStrSetter(VmaLocationStrSetter &&) = delete;
  VmaLocationStrSetter &operator=(VmaLocationStrSetter &&) = delete;
};
#else
class VmaLocationStrSetter {
public:
  VmaLocationStrSetter(VmaAllocationCreateInfo &CreateInfo,
                       const SourceLocation &Location) noexcept {}
  operator const char *() const noexcept { return nullptr; }
  VmaLocationStrSetter(const VmaLocationStrSetter &) = delete;
  VmaLocationStrSetter &operator=(const VmaLocationStrSetter &) = delete;
  VmaLocationStrSetter(VmaLocationStrSetter &&) = delete;
  VmaLocationStrSetter &operator=(VmaLocationStrSetter &&) = delete;
};
#endif

inline BufferAllocation
AllocateStaticBuffer(const SourceLocation &location, vk::DeviceSize size,
                     vk::BufferUsageFlags usage) noexcept {
  struct StaticUniformBufferAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr StaticUniformBufferAllocationCreateInfo() noexcept
        : VmaAllocationCreateInfo{0,
                                  VMA_MEMORY_USAGE_GPU_ONLY,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  0,
                                  0,
                                  VK_NULL_HANDLE,
                                  nullptr} {}
  };
  VkBuffer Buffer;
  VmaAllocation Allocation;
  StaticUniformBufferAllocationCreateInfo CreateInfo;
  VmaLocationStrSetter LocationStr(CreateInfo, location);
  auto Result = vmaCreateBuffer(vk::BufferCreateInfo({}, size, usage),
                                CreateInfo, &Buffer, &Allocation, nullptr);
  VULKAN_HPP_ASSERT(Result == VK_SUCCESS);
  Globals.SetDebugObjectName(LocationStr, vk::Buffer(Buffer));
  return BufferAllocation(Buffer, Allocation);
}

inline DynamicBufferAllocation
AllocateDynamicBuffer(const SourceLocation &location, vk::DeviceSize size,
                      vk::BufferUsageFlags usage) noexcept {
  struct DynamicUniformBufferAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr DynamicUniformBufferAllocationCreateInfo() noexcept
        : VmaAllocationCreateInfo{VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                  VMA_MEMORY_USAGE_CPU_TO_GPU,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  0,
                                  VK_NULL_HANDLE,
                                  nullptr} {}
  };
  VkBuffer Buffer;
  VmaAllocation Allocation;
  VmaAllocationInfo AllocInfo;
  VkDeviceSize SecondOffset;
  DynamicUniformBufferAllocationCreateInfo CreateInfo;
  VmaLocationStrSetter LocationStr(CreateInfo, location);
  auto Result =
      vmaCreateDoubleBuffer(vk::BufferCreateInfo({}, size, usage), CreateInfo,
                            &Buffer, &Allocation, &AllocInfo, &SecondOffset);
  VULKAN_HPP_ASSERT(Result == VK_SUCCESS);
  Globals.SetDebugObjectName(LocationStr, vk::Buffer(Buffer));
  return DynamicBufferAllocation(Buffer, Allocation, AllocInfo.pMappedData,
                                 SecondOffset);
}

inline UploadBufferAllocation
AllocateUploadBuffer(const SourceLocation &location,
                     vk::DeviceSize size) noexcept {
  struct UploadBufferAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr UploadBufferAllocationCreateInfo() noexcept
        : VmaAllocationCreateInfo{VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                  VMA_MEMORY_USAGE_CPU_ONLY,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                  0,
                                  0,
                                  VK_NULL_HANDLE,
                                  nullptr} {}
  };
  VkBuffer Buffer;
  VmaAllocation Allocation;
  VmaAllocationInfo AllocInfo;
  UploadBufferAllocationCreateInfo CreateInfo;
  VmaLocationStrSetter LocationStr(CreateInfo, location);
  auto Result = vmaCreateBuffer(
      vk::BufferCreateInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc),
      CreateInfo, &Buffer, &Allocation, &AllocInfo);
  VULKAN_HPP_ASSERT(Result == VK_SUCCESS);
  Globals.SetDebugObjectName(LocationStr, vk::Buffer(Buffer));
  return UploadBufferAllocation(Buffer, Allocation, AllocInfo.pMappedData);
}

inline TextureAllocation AllocateTexture(const SourceLocation &location,
                                         const vk::ImageCreateInfo &CreateInfo,
                                         bool Dedicated = false) noexcept {
  struct TextureAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr TextureAllocationCreateInfo(bool Dedicated) noexcept
        : VmaAllocationCreateInfo{
              Dedicated ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
                        : VmaAllocationCreateFlagBits(0),
              VMA_MEMORY_USAGE_GPU_ONLY,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              0,
              0,
              VK_NULL_HANDLE,
              nullptr} {}
  } AllocationCreateInfo{Dedicated};
  VkImage Image;
  VmaAllocation Allocation;
  VmaLocationStrSetter LocationStr(AllocationCreateInfo, location);
  auto Result =
      vmaCreateImage(Globals.Allocator,
                     reinterpret_cast<const VkImageCreateInfo *>(&CreateInfo),
                     &AllocationCreateInfo, &Image, &Allocation, nullptr);
  VULKAN_HPP_ASSERT(Result == VK_SUCCESS);
  Globals.SetDebugObjectName(LocationStr, vk::Image(Image));
  return TextureAllocation(Image, Allocation);
}

void RenderTextureAllocation::Prepare() noexcept {
  Globals.Device.waitIdle();
  ColorTexture = AllocateTexture(
      Location.with_field("ColorTexture"),
      vk::ImageCreateInfo(
          {}, vk::ImageType::e2D, ColorFormat, vk::Extent3D(Extent, 1), 1, 1,
          vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
          vk::ImageUsageFlagBits::eColorAttachment |
              vk::ImageUsageFlagBits::eTransferSrc,
          {}, {}, {}, vk::ImageLayout::eUndefined),
      true);
  ColorView = vulkan::Globals.Device
                  .createImageViewUnique(vk::ImageViewCreateInfo(
                      {}, ColorTexture.GetImage(), vk::ImageViewType::e2D,
                      ColorFormat, {},
                      vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor,
                                                0, 1, 0, 1)))
                  .value;
  Globals.SetDebugObjectName(Location.with_field("ColorView"), ColorView.get());
  DepthTexture = AllocateTexture(
      Location.with_field("DepthTexture"),
      vk::ImageCreateInfo({}, vk::ImageType::e2D, vk::Format::eD32Sfloat,
                          vk::Extent3D(Extent, 1), 1, 1,
                          vk::SampleCountFlagBits::e1,
                          vk::ImageTiling::eOptimal,
                          vk::ImageUsageFlagBits::eDepthStencilAttachment |
                              vk::ImageUsageFlagBits::eTransferSrc,
                          {}, {}, {}, vk::ImageLayout::eUndefined),
      true);
  DepthView = vulkan::Globals.Device
                  .createImageViewUnique(vk::ImageViewCreateInfo(
                      {}, DepthTexture.GetImage(), vk::ImageViewType::e2D,
                      vk::Format::eD32Sfloat, {},
                      vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth,
                                                0, 1, 0, 1)))
                  .value;
  Globals.SetDebugObjectName(Location.with_field("DepthView"), DepthView.get());
  vk::ImageView Views[] = {ColorView.get(), DepthView.get()};
  auto RenderPass = Globals.GetRenderPass();
  Framebuffer =
      Globals.Device
          .createFramebufferUnique(vk::FramebufferCreateInfo(
              {}, RenderPass, 2, Views, Extent.width, Extent.height, 1))
          .value;
  Globals.SetDebugObjectName(Location.with_field("Framebuffer"),
                             Framebuffer.get());
  RenderPassBegin = RenderPassBeginInfo(RenderPass, Framebuffer.get(), Extent);
  for (uint32_t i = 0; i < NumColorBindings; ++i) {
    ColorBindings[i].Texture = AllocateTexture(
        Location.with_field("ColorBindings", i),
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, ColorFormat, vk::Extent3D(Extent, 1), 1, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst,
            {}, {}, {}, vk::ImageLayout::eUndefined),
        true);
    ColorBindings[i].ImageView =
        vulkan::Globals.Device
            .createImageViewUnique(vk::ImageViewCreateInfo(
                {}, ColorBindings[i].Texture.GetImage(), vk::ImageViewType::e2D,
                ColorFormat, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1,
                                          0, 1)))
            .value;
    Globals.SetDebugObjectName(Location.with_field("ColorBindings", i),
                               ColorBindings[i].ImageView.get());
  }
  for (uint32_t i = 0; i < NumDepthBindings; ++i) {
    DepthBindings[i].Texture = AllocateTexture(
        Location.with_field("DepthBindings", i),
        vk::ImageCreateInfo({}, vk::ImageType::e2D, vk::Format::eD32Sfloat,
                            vk::Extent3D(Extent, 1), 1, 1,
                            vk::SampleCountFlagBits::e1,
                            vk::ImageTiling::eOptimal,
                            vk::ImageUsageFlagBits::eSampled |
                                vk::ImageUsageFlagBits::eTransferDst,
                            {}, {}, {}, vk::ImageLayout::eUndefined),
        true);
    DepthBindings[i].ImageView =
        vulkan::Globals.Device
            .createImageViewUnique(vk::ImageViewCreateInfo(
                {}, DepthBindings[i].Texture.GetImage(), vk::ImageViewType::e2D,
                vk::Format::eD32Sfloat, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1,
                                          0, 1)))
            .value;
    Globals.SetDebugObjectName(Location.with_field("DepthBindings", i),
                               DepthBindings[i].ImageView.get());
  }
  FirstAttach = false;
}

void RenderTextureAllocation::BeginRenderPass() noexcept {
  Globals.Cmd.beginRenderPass(RenderPassBegin, vk::SubpassContents::eInline);
}

void RenderTextureAllocation::_Resolve(vk::Image SrcImage, vk::Image DstImage,
                                       vk::ImageAspectFlagBits Aspect,
                                       vk::Offset3D Offset,
                                       vk::Extent3D Extent) noexcept {
  vulkan::Globals.Cmd.pipelineBarrier(
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion,
      {}, {},
      vk::ImageMemoryBarrier(
          vk::AccessFlagBits::eColorAttachmentWrite,
          vk::AccessFlagBits::eTransferRead,
          vk::ImageLayout::eColorAttachmentOptimal,
          vk::ImageLayout::eTransferSrcOptimal, VK_QUEUE_FAMILY_IGNORED,
          VK_QUEUE_FAMILY_IGNORED, SrcImage,
          vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                    VK_REMAINING_MIP_LEVELS, 0,
                                    VK_REMAINING_ARRAY_LAYERS)));
  if (Globals.MultisampleState.rasterizationSamples >
      vk::SampleCountFlagBits::e1) {
    Globals.Cmd.resolveImage(
        SrcImage, vk::ImageLayout::eTransferSrcOptimal, DstImage,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageResolve(vk::ImageSubresourceLayers(Aspect, 0, 0, 1), Offset,
                         vk::ImageSubresourceLayers(Aspect, 0, 0, 1), Offset,
                         Extent));
  } else {
    Globals.Cmd.copyImage(
        SrcImage, vk::ImageLayout::eTransferSrcOptimal, DstImage,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageCopy(vk::ImageSubresourceLayers(Aspect, 0, 0, 1), Offset,
                      vk::ImageSubresourceLayers(Aspect, 0, 0, 1), Offset,
                      Extent));
  }
  vulkan::Globals.Cmd.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::DependencyFlagBits::eByRegion, {}, {},
      vk::ImageMemoryBarrier(
          vk::AccessFlagBits::eTransferRead,
          vk::AccessFlagBits::eColorAttachmentWrite,
          vk::ImageLayout::eTransferSrcOptimal,
          vk::ImageLayout::eColorAttachmentOptimal, VK_QUEUE_FAMILY_IGNORED,
          VK_QUEUE_FAMILY_IGNORED, SrcImage,
          vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                    VK_REMAINING_MIP_LEVELS, 0,
                                    VK_REMAINING_ARRAY_LAYERS)));
}

void RenderTextureAllocation::Resolve(vk::Image SrcImage, vk::Image DstImage,
                                      vk::ImageAspectFlagBits Aspect,
                                      vk::Offset3D Offset, vk::Extent3D Extent,
                                      bool Reattach) noexcept {
  bool DelimitRenderPass = this == Globals.AttachedRenderTexture;
  if (DelimitRenderPass)
    Globals.Cmd.endRenderPass();

  _Resolve(SrcImage, DstImage, Aspect, Offset, Extent);

  if (DelimitRenderPass) {
    if (Reattach)
      BeginRenderPass();
    else
      Globals.AttachedRenderTexture = nullptr;
  }
}

void RenderTextureAllocation::ResolveSurface(SurfaceAllocation *Surface,
                                             bool Reattach) noexcept {
  assert(Surface->NextImage != UINT32_MAX &&
         "acquireNextImage not called on surface for this frame");
  assert(Surface->Extent == Extent &&
         "Mismatched render texture / surface extents");
  bool DelimitRenderPass = this == Globals.AttachedRenderTexture;
  if (DelimitRenderPass)
    Globals.Cmd.endRenderPass();
  auto DstImage = Surface->SwapchainImages[Surface->NextImage];
  vulkan::Globals.Cmd.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion,
      {}, {},
      vk::ImageMemoryBarrier(
          vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite,
          vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
          VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, DstImage,
          vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                    VK_REMAINING_MIP_LEVELS, 0,
                                    VK_REMAINING_ARRAY_LAYERS)));
  _Resolve(ColorTexture.GetImage(), DstImage, vk::ImageAspectFlagBits::eColor,
           vk::Offset3D(), vk::Extent3D(Extent, 1));
  vulkan::Globals.Cmd.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion,
      {}, {},
      vk::ImageMemoryBarrier(
          vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
          vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
          VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, DstImage,
          vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                    VK_REMAINING_MIP_LEVELS, 0,
                                    VK_REMAINING_ARRAY_LAYERS)));
  if (DelimitRenderPass) {
    if (Reattach)
      BeginRenderPass();
    else
      Globals.AttachedRenderTexture = nullptr;
  }
}

void RenderTextureAllocation::ResolveColorBinding(uint32_t Idx, Rect2D region,
                                                  bool Reattach) noexcept {
  assert(Idx < NumColorBindings);
  Resolve(ColorTexture.GetImage(), ColorBindings[Idx].Texture.GetImage(),
          vk::ImageAspectFlagBits::eColor, vk::Offset3D(region.offset),
          vk::Extent3D(region.extent, 1), Reattach);
}

void RenderTextureAllocation::ResolveDepthBinding(uint32_t Idx, Rect2D region,
                                                  bool Reattach) noexcept {
  assert(Idx < NumDepthBindings);
  Resolve(DepthTexture.GetImage(), DepthBindings[Idx].Texture.GetImage(),
          vk::ImageAspectFlagBits::eDepth, vk::Offset3D(region.offset),
          vk::Extent3D(region.extent, 1), Reattach);
}

void RenderTextureAllocation::Attach() noexcept {
  if (Globals.AttachedRenderTexture == this)
    return;
  if (Globals.AttachedRenderTexture)
    Globals.Cmd.endRenderPass();
  Globals.AttachedRenderTexture = this;

  if (!FirstAttach) {
    FirstAttach = true;
    vulkan::Globals.Cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eEarlyFragmentTests |
            vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::DependencyFlagBits::eByRegion, {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits(0),
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            DepthTexture.GetImage(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0,
                                      VK_REMAINING_MIP_LEVELS, 0,
                                      VK_REMAINING_ARRAY_LAYERS)));
    vulkan::Globals.Cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::DependencyFlagBits::eByRegion, {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits(0), vk::AccessFlagBits::eColorAttachmentWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal, VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED, ColorTexture.GetImage(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                      VK_REMAINING_MIP_LEVELS, 0,
                                      VK_REMAINING_ARRAY_LAYERS)));
  }

  BeginRenderPass();
  Globals.Cmd.setViewport(
      0, vk::Viewport(0.f, 0.f, Extent.width, Extent.height, 0.f, 1.f));
  Globals.Cmd.setScissor(0, vk::Rect2D({}, {Extent.width, Extent.height}));
}
} // namespace hsh::detail::vulkan
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
template <typename T> struct ClassWrapper {};

struct SamplerBinding;

constexpr unsigned NumStaticallyActiveTargets = 0
#define HSH_TARGET(Enumeration, Active) +unsigned(!!(Active))
#include "targets.def"
    ;
static_assert(NumStaticallyActiveTargets != 0,
              "No hsh targets are statically active");
constexpr enum Target FirstStaticallyActiveTarget() noexcept {
#define HSH_ACTIVE_TARGET(Enumeration) return Enumeration;
#include "targets.def"
}
inline enum Target ActiveTarget = FirstStaticallyActiveTarget();

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
#if HSH_ENABLE_VULKAN
template <> struct TargetTraits<VULKAN_SPIRV> {
  struct BufferWrapper {
    vk::Buffer Buffer;
    BufferWrapper() noexcept = default;
    BufferWrapper(const vulkan::BufferAllocation &Alloc) noexcept
        : Buffer(Alloc.getBuffer()) {}
    bool isValid() const noexcept { return Buffer.operator bool(); }
    operator vk::Buffer() const noexcept { return Buffer; }
  };
  using UniformBufferOwner = vulkan::BufferAllocation;
  using UniformBufferBinding = BufferWrapper;
  using DynamicUniformBufferOwner = vulkan::DynamicBufferAllocation;
  using DynamicUniformBufferBinding = vulkan::DynamicBufferBinding;
  using VertexBufferOwner = vulkan::BufferAllocation;
  using VertexBufferBinding = BufferWrapper;
  using DynamicVertexBufferOwner = vulkan::DynamicBufferAllocation;
  using DynamicVertexBufferBinding = vulkan::DynamicBufferBinding;
  struct TextureBinding {
    vk::ImageView ImageView;
    std::uint8_t NumMips : 7;
    std::uint8_t Integer : 1;
    bool isValid() const noexcept { return ImageView.operator bool(); }
  };
  struct TextureOwner {
    vulkan::TextureAllocation Allocation;
    vk::UniqueImageView ImageView;
    std::uint8_t NumMips : 7;
    std::uint8_t Integer : 1;
    TextureOwner() noexcept = default;
    TextureOwner(const TextureOwner &other) = delete;
    TextureOwner &operator=(const TextureOwner &other) = delete;
    TextureOwner(TextureOwner &&other) noexcept = default;
    TextureOwner &operator=(TextureOwner &&other) noexcept = default;

    bool isValid() const noexcept { return ImageView.operator bool(); }

    TextureBinding getBinding() const noexcept {
      return TextureBinding{ImageView.get(), NumMips, Integer};
    }
    operator TextureBinding() const noexcept { return getBinding(); }
  };
  struct RenderTextureBinding {
    vulkan::RenderTextureAllocation *Allocation = nullptr;
    uint32_t BindingIdx : 24;
    uint32_t IsDepth : 8;
    RenderTextureBinding() noexcept : BindingIdx(0), IsDepth(0) {}
    RenderTextureBinding(vulkan::RenderTextureAllocation *Allocation,
                         uint32_t BindingIdx, uint32_t IsDepth) noexcept
        : Allocation(Allocation), BindingIdx(BindingIdx), IsDepth(IsDepth) {}

    bool isValid() const noexcept { return Allocation != nullptr; }

    vk::ImageView getImageView() const noexcept {
      if (IsDepth)
        return Allocation->GetDepthBindingView(BindingIdx);
      else
        return Allocation->GetColorBindingView(BindingIdx);
    }
  };
  struct SurfaceBinding {
    vulkan::SurfaceAllocation *Allocation = nullptr;
    bool isValid() const noexcept { return Allocation != nullptr; }
  };
  struct RenderTextureOwner {
    std::unique_ptr<vulkan::RenderTextureAllocation> Allocation;
    RenderTextureOwner(const RenderTextureOwner &other) = delete;
    RenderTextureOwner &operator=(const RenderTextureOwner &other) = delete;
    RenderTextureOwner(RenderTextureOwner &&other) noexcept = default;
    RenderTextureOwner &
    operator=(RenderTextureOwner &&other) noexcept = default;

    bool isValid() const noexcept { return Allocation.operator bool(); }

    RenderTextureBinding getColor(uint32_t idx) const noexcept {
      return {Allocation.get(), idx, false};
    }
    RenderTextureBinding getDepth(uint32_t idx) const noexcept {
      return {Allocation.get(), idx, true};
    }
    void attach() noexcept { return Allocation->Attach(); }
    void resolveSurface(SurfaceBinding surface, bool reattach) noexcept {
      Allocation->ResolveSurface(surface.Allocation, reattach);
    }
    void resolveColorBinding(uint32_t idx, Rect2D region,
                             bool reattach) noexcept {
      Allocation->ResolveColorBinding(idx, region, reattach);
    }
    void resolveDepthBinding(uint32_t idx, Rect2D region,
                             bool reattach) noexcept {
      Allocation->ResolveDepthBinding(idx, region, reattach);
    }
  };
  struct SurfaceOwner {
    std::unique_ptr<vulkan::SurfaceAllocation> Allocation;
    SurfaceOwner(const SurfaceOwner &other) = delete;
    SurfaceOwner &operator=(const SurfaceOwner &other) = delete;
    SurfaceOwner(SurfaceOwner &&other) noexcept = default;
    SurfaceOwner &operator=(SurfaceOwner &&other) noexcept = default;

    bool isValid() const noexcept { return Allocation.operator bool(); }

    SurfaceBinding getBinding() const noexcept {
      return SurfaceBinding{Allocation.get()};
    }
    operator SurfaceBinding() const noexcept { return getBinding(); }
    bool acquireNextImage() noexcept { return Allocation->AcquireNextImage(); }
  };
  struct PipelineBinding {
    vk::Pipeline Pipeline;
    vulkan::UniqueDescriptorSet DescriptorSet;
    std::array<uint32_t, MaxUniforms> UniformOffsets{};
    static constexpr std::array<uint32_t, MaxUniforms> ZeroUniformOffsets{};
    uint32_t NumVertexBuffers = 0;
    std::array<vk::Buffer, MaxVertexBuffers> VertexBuffers{};
    std::array<vk::DeviceSize, MaxVertexBuffers> VertexOffsets{};
    static constexpr std::array<vk::DeviceSize, MaxVertexBuffers>
        ZeroVertexOffsets{};
    struct BoundRenderTexture {
      RenderTextureBinding RenderTextureBinding;
      vk::ImageView KnownImageView;
      uint32_t DescriptorBindingIdx = 0;
    };
    std::array<BoundRenderTexture, MaxImages> RenderTextures{};
    struct Iterators {
      decltype(UniformOffsets)::iterator UniformOffsetIt;
      decltype(VertexBuffers)::iterator VertexBufferBegin;
      decltype(VertexBuffers)::iterator VertexBufferIt;
      decltype(VertexOffsets)::iterator VertexOffsetIt;
      decltype(RenderTextures)::iterator RenderTextureIt;
      uint32_t TextureIdx = 0;
      constexpr explicit Iterators(PipelineBinding &Binding) noexcept
          : UniformOffsetIt(Binding.UniformOffsets.begin()),
            VertexBufferBegin(Binding.VertexBuffers.begin()),
            VertexBufferIt(Binding.VertexBuffers.begin()),
            VertexOffsetIt(Binding.VertexOffsets.begin()),
            RenderTextureIt(Binding.RenderTextures.begin()) {}

      inline void add(uniform_buffer_typeless uniform) noexcept;
      inline void add(dynamic_uniform_buffer_typeless uniform) noexcept;
      inline void add(vertex_buffer_typeless uniform) noexcept;
      inline void add(dynamic_vertex_buffer_typeless uniform) noexcept;
      inline void add(texture_typeless texture) noexcept;
      inline void add(render_texture2d texture) noexcept;
      static inline void add(SamplerBinding sampler) noexcept;
    };

    bool isValid() const noexcept { return Pipeline.operator bool(); }

    PipelineBinding() noexcept = default;

    template <typename Impl, typename... Args>
    explicit PipelineBinding(ClassWrapper<Impl>, Args... args) noexcept;

    void updateRenderTextures() noexcept {
      std::array<vk::DescriptorImageInfo, MaxImages> ImageInfos;
      std::array<vk::WriteDescriptorSet, MaxImages> Writes;
      uint32_t WriteCur = 0;
      for (auto &RT : RenderTextures) {
        if (!RT.RenderTextureBinding.Allocation)
          break;
        auto ImageView = RT.RenderTextureBinding.getImageView();
        if (ImageView != RT.KnownImageView) {
          Writes[WriteCur] = vk::WriteDescriptorSet(
              DescriptorSet.Set, RT.DescriptorBindingIdx, 0, 1,
              vk::DescriptorType::eSampledImage, &ImageInfos[WriteCur]);
          ImageInfos[WriteCur] = vk::DescriptorImageInfo(
              {}, ImageView, vk::ImageLayout::eShaderReadOnlyOptimal);
          ++WriteCur;
        }
      }
      vulkan::Globals.Device.updateDescriptorSets(WriteCur, Writes.data(), 0,
                                                  nullptr);
    }

    void bind() noexcept {
      for (auto &RT : RenderTextures) {
        if (!RT.RenderTextureBinding.Allocation)
          break;
        if (RT.RenderTextureBinding.getImageView() != RT.KnownImageView) {
          updateRenderTextures();
          break;
        }
      }
      if (vulkan::Globals.BoundPipeline != Pipeline) {
        vulkan::Globals.BoundPipeline = Pipeline;
        vulkan::Globals.Cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                         Pipeline);
      }
      if (vulkan::Globals.BoundDescriptorSet != DescriptorSet.Set) {
        vulkan::Globals.BoundDescriptorSet = DescriptorSet.Set;
        vulkan::Globals.Cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, vulkan::Globals.PipelineLayout, 0,
            DescriptorSet.Set,
            vulkan::Globals.DynamicBufferIndex ? UniformOffsets
                                               : ZeroUniformOffsets);
        vulkan::Globals.Cmd.bindVertexBuffers(
            0, NumVertexBuffers, VertexBuffers.data(),
            vulkan::Globals.DynamicBufferIndex ? VertexOffsets.data()
                                               : ZeroVertexOffsets.data());
      }
    }

    void draw(uint32_t start, uint32_t count) noexcept {
      bind();
      vulkan::Globals.Cmd.draw(count, 1, start, 0);
    }
  };

  template <typename ResTp> struct ResourceFactory {};
};
#endif
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
    switch (detail::ActiveTarget) {
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  case Enumeration:                                                            \
    return decltype(ResourceFactory<T>::_##Enumeration)::Create(location,      \
                                                                args...);
#include "targets.def"
    default:
      assert(false && "unhandled case");
    }
    return {};
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
};
using ActiveTargetTraits = SelectTargetTraits<NumStaticallyActiveTargets>;
} // namespace detail

struct base_buffer {};

template <typename T> struct uniform_buffer;
template <typename T> struct dynamic_uniform_buffer;
template <typename T> struct vertex_buffer;
template <typename T> struct dynamic_vertex_buffer;

struct uniform_buffer_typeless : base_buffer {
#if HSH_ASSERT_CAST_ENABLED
  std::size_t UniqueId;
  template <typename... Args>
  explicit uniform_buffer_typeless(std::size_t UniqueId,
                                   Args &&... args) noexcept
      : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
  explicit uniform_buffer_typeless(std::size_t UniqueId,
                                   uniform_buffer_typeless other) noexcept
      : UniqueId(UniqueId), Binding(other.Binding) {}
#else
  template <typename... Args>
  explicit uniform_buffer_typeless(Args &&... args) noexcept
      : Binding(std::forward<Args>(args)...) {}
  template <typename T>
  explicit uniform_buffer_typeless(uniform_buffer<T> other) noexcept
      : Binding(other.Binding) {}
#endif
  detail::ActiveTargetTraits::UniformBufferBinding Binding;
  template <typename T> uniform_buffer<T> cast() const noexcept;
  template <typename T> operator uniform_buffer<T>() const noexcept {
    return cast<T>();
  }
};
struct dynamic_uniform_buffer_typeless : base_buffer {
#if HSH_ASSERT_CAST_ENABLED
  std::size_t UniqueId;
  template <typename... Args>
  explicit dynamic_uniform_buffer_typeless(std::size_t UniqueId,
                                           Args &&... args) noexcept
      : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
  explicit dynamic_uniform_buffer_typeless(
      std::size_t UniqueId, dynamic_uniform_buffer_typeless other) noexcept
      : UniqueId(UniqueId), Binding(other.Binding) {}
#else
  template <typename... Args>
  explicit dynamic_uniform_buffer_typeless(Args &&... args) noexcept
      : Binding(std::forward<Args>(args)...) {}
  template <typename T>
  explicit dynamic_uniform_buffer_typeless(
      dynamic_uniform_buffer<T> other) noexcept
      : Binding(other.Binding) {}
#endif
  detail::ActiveTargetTraits::DynamicUniformBufferBinding Binding;
  template <typename T> dynamic_uniform_buffer<T> cast() const noexcept;
  template <typename T> operator dynamic_uniform_buffer<T>() const noexcept {
    return cast<T>();
  }
};
struct vertex_buffer_typeless : base_buffer {
#if HSH_ASSERT_CAST_ENABLED
  std::size_t UniqueId;
  template <typename... Args>
  explicit vertex_buffer_typeless(std::size_t UniqueId,
                                  Args &&... args) noexcept
      : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
  explicit vertex_buffer_typeless(std::size_t UniqueId,
                                  vertex_buffer_typeless other) noexcept
      : UniqueId(UniqueId), Binding(other.Binding) {}
#else
  template <typename... Args>
  explicit vertex_buffer_typeless(Args &&... args) noexcept
      : Binding(std::forward<Args>(args)...) {}
  template <typename T>
  explicit vertex_buffer_typeless(vertex_buffer<T> other) noexcept
      : Binding(other.Binding) {}
#endif
  detail::ActiveTargetTraits::VertexBufferBinding Binding;
  template <typename T> vertex_buffer<T> cast() const noexcept;
  template <typename T> operator vertex_buffer<T>() const noexcept {
    return cast<T>();
  }
};
struct dynamic_vertex_buffer_typeless : base_buffer {
#if HSH_ASSERT_CAST_ENABLED
  std::size_t UniqueId;
  template <typename... Args>
  explicit dynamic_vertex_buffer_typeless(std::size_t UniqueId,
                                          Args &&... args) noexcept
      : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
  explicit dynamic_vertex_buffer_typeless(
      std::size_t UniqueId, dynamic_vertex_buffer_typeless other) noexcept
      : UniqueId(UniqueId), Binding(other.Binding) {}
#else
  template <typename... Args>
  explicit dynamic_vertex_buffer_typeless(Args &&... args) noexcept
      : Binding(std::forward<Args>(args)...) {}
  template <typename T>
  explicit dynamic_vertex_buffer_typeless(
      dynamic_vertex_buffer<T> other) noexcept
      : Binding(other.Binding) {}
#endif
  detail::ActiveTargetTraits::DynamicVertexBufferBinding Binding;
  template <typename T> dynamic_vertex_buffer<T> cast() const noexcept;
  template <typename T> operator dynamic_vertex_buffer<T>() const noexcept {
    return cast<T>();
  }
};

#if HSH_ASSERT_CAST_ENABLED
#define HSH_CASTABLE_BUFFER(derived)                                           \
  template <typename T> struct derived : derived##_typeless {                  \
    using MappedType = T;                                                      \
    template <typename... Args>                                                \
    explicit derived(Args &&... args) noexcept                                 \
        : derived##_typeless(typeid(*this).hash_code(),                        \
                             std::forward<Args>(args)...) {}                   \
    const T *operator->() const noexcept {                                     \
      assert(false && "Not to be used from host!");                            \
      return nullptr;                                                          \
    }                                                                          \
    const T &operator*() const noexcept {                                      \
      assert(false && "Not to be used from host!");                            \
      return *reinterpret_cast<T *>(0);                                        \
    }                                                                          \
  };
#else
#define HSH_CASTABLE_BUFFER(derived)                                           \
  template <typename T> struct derived : derived##_typeless {                  \
    using MappedType = T;                                                      \
    template <typename... Args>                                                \
    explicit derived(Args &&... args) noexcept                                 \
        : derived##_typeless(std::forward<Args>(args)...) {}                   \
    const T *operator->() const noexcept {                                     \
      assert(false && "Not to be used from host!");                            \
      return nullptr;                                                          \
    }                                                                          \
    const T &operator*() const noexcept {                                      \
      assert(false && "Not to be used from host!");                            \
      return *reinterpret_cast<T *>(0);                                        \
    }                                                                          \
  };
#endif
HSH_CASTABLE_BUFFER(uniform_buffer)
HSH_CASTABLE_BUFFER(dynamic_uniform_buffer)
HSH_CASTABLE_BUFFER(vertex_buffer)
HSH_CASTABLE_BUFFER(dynamic_vertex_buffer)
#undef HSH_CASTABLE_BUFFER

#define HSH_DEFINE_BUFFER_CAST(derived)                                        \
  template <typename T> derived<T> derived##_typeless::cast() const noexcept { \
    HSH_ASSERT_CAST(UniqueId == typeid(derived<T>).hash_code() && "bad cast"); \
    return static_cast<derived<T>>(*this);                                     \
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
  float4() noexcept = default;
  constexpr float4(float x, float y, float z, float w) noexcept
      : x(x), y(y), z(z), w(w) {}
  constexpr explicit float4(float f) noexcept : x(f), y(f), z(f), w(f) {}
  constexpr explicit float4(const float3 &other, float w = 1.f) noexcept;
  constexpr explicit float4(const float2 &other, float z = 0.f,
                            float w = 1.f) noexcept;
  void operator+=(const float4 &other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
  }
  void operator*=(const float4 &other) noexcept {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
  }
  float4 operator/(float other) noexcept {
    return float4{x / other, y / other, z / other, w / other};
  }
  float &operator[](std::size_t idx) noexcept { return (&x)[0]; }
  const float &operator[](std::size_t idx) const noexcept { return (&x)[0]; }
  constexpr float3 xyz() const noexcept;
  constexpr float2 xy() const noexcept;
  constexpr float2 xz() const noexcept;
  constexpr float2 xw() const noexcept;
};
struct float3 {
  float x, y, z;
  float3() noexcept = default;
  constexpr float3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
  constexpr explicit float3(float f) noexcept : x(f), y(f), z(f) {}
  float3 operator-() const noexcept { return float3{-x, -y, -z}; };
  float3 operator*(float other) const noexcept {
    return float3{x * other, y * other, z * other};
  }
  float3 operator/(float other) const noexcept {
    return float3{x / other, y / other, z / other};
  }
  float3 operator*(const float3 &other) const noexcept {
    return float3{x * other.x, y * other.y, z * other.z};
  }
  float3 &operator*=(const float3 &other) noexcept {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
  }
  float3 &operator*=(float other) noexcept {
    x *= other;
    y *= other;
    z *= other;
    return *this;
  }
  float3 operator+(const float3 &other) const noexcept {
    return float3{x + other.x, y + other.y, z + other.z};
  }
  float3 &operator+=(const float3 &other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }
  float &operator[](std::size_t idx) noexcept { return (&x)[0]; }
  const float &operator[](std::size_t idx) const noexcept { return (&x)[0]; }
};
constexpr float3 float4::xyz() const noexcept { return float3{x, y, z}; }
struct float2 {
  float x, y;
  float2() noexcept = default;
  constexpr float2(float x, float y) noexcept : x(x), y(y) {}
  constexpr explicit float2(float f) noexcept : x(f), y(f) {}
  float2 operator*(const float2 &other) const noexcept {
    return float2{x * other.x, y * other.y};
  }
  float2 operator/(const float2 &other) const noexcept {
    return float2{x / other.x, y / other.y};
  }
  float2 operator/(float other) const noexcept {
    return float2{x / other, y / other};
  }
  float2 operator-(const float2 &other) const noexcept {
    return float2{x - other.x, y - other.y};
  }
  float2 operator+(const float2 &other) const noexcept {
    return float2{x + other.x, y + other.y};
  }
  float2 operator-() const noexcept { return float2{-x, -y}; };
};
constexpr float2 float4::xy() const noexcept { return float2{x, y}; }
constexpr float2 float4::xz() const noexcept { return float2{x, z}; }
constexpr float2 float4::xw() const noexcept { return float2{x, w}; }
constexpr float4::float4(const hsh::float3 &other, float w) noexcept
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr float4::float4(const hsh::float2 &other, float z, float w) noexcept
    : x(other.x), y(other.y), z(z), w(w) {}
struct int3;
struct int2;
struct int4 {
  std::int32_t x, y, z, w;
  int4() noexcept = default;
  constexpr explicit int4(const int3 &other, std::int32_t w = 0) noexcept;
  constexpr explicit int4(const int2 &other, std::int32_t z = 0,
                          std::int32_t w = 0) noexcept;
  void operator+=(const int4 &other) noexcept {}
  void operator*=(const int4 &other) noexcept {}
};
struct int3 {
  std::int32_t x, y, z;
  int3() noexcept = default;
  constexpr int3(std::int32_t x, std::int32_t y, std::int32_t z) noexcept
      : x(x), y(y), z(z) {}
  constexpr explicit int3(std::int32_t f) noexcept : x(f), y(f), z(f) {}
  int3 operator-() const noexcept { return int3{-x, -y, -z}; };
  int3 operator*(std::int32_t other) noexcept {
    return int3{x * other, y * other, z * other};
  }
};
struct int2 {
  std::int32_t x, y;
  int2() noexcept = default;
  constexpr int2(std::int32_t x, std::int32_t y) noexcept : x(x), y(y) {}
  constexpr explicit int2(std::int32_t f) noexcept : x(f), y(f) {}
  int2 operator-() const noexcept { return int2{-x, -y}; };
};
constexpr int4::int4(const hsh::int3 &other, std::int32_t w) noexcept
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr int4::int4(const hsh::int2 &other, std::int32_t z,
                     std::int32_t w) noexcept
    : x(other.x), y(other.y), z(z), w(w) {}
struct uint3;
struct uint2;
struct uint4 {
  std::uint32_t x, y, z, w;
  uint4() noexcept = default;
  constexpr explicit uint4(const uint3 &other, std::uint32_t w = 0) noexcept;
  constexpr explicit uint4(const uint2 &other, std::uint32_t z = 0,
                           std::uint32_t w = 0) noexcept;
  void operator+=(const uint4 &other) noexcept {}
  void operator*=(const uint4 &other) noexcept {}
};
struct uint3 {
  std::uint32_t x, y, z;
  uint3() noexcept = default;
  constexpr uint3(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
      : x(x), y(y), z(z) {}
  constexpr explicit uint3(std::uint32_t f) noexcept : x(f), y(f), z(f) {}
  uint3 operator-() const noexcept { return uint3{-x, -y, -z}; };
  uint3 operator*(std::uint32_t other) noexcept {
    return uint3{x * other, y * other, z * other};
  }
};
struct uint2 {
  std::uint32_t x, y;
  uint2() noexcept = default;
  constexpr uint2(std::uint32_t x, std::uint32_t y) noexcept : x(x), y(y) {}
  constexpr explicit uint2(std::uint32_t f) noexcept : x(f), y(f) {}
  uint2 operator-() const noexcept { return uint2{-x, -y}; };
};
constexpr uint4::uint4(const hsh::uint3 &other, std::uint32_t w) noexcept
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr uint4::uint4(const hsh::uint2 &other, std::uint32_t z,
                       std::uint32_t w) noexcept
    : x(other.x), y(other.y), z(z), w(w) {}
struct float4x4 {
  float4 cols[4];
  float4x4() noexcept = default;
  float4 &operator[](std::size_t col) noexcept { return cols[col]; }
  const float4 &operator[](std::size_t col) const noexcept { return cols[col]; }
  float4x4 operator*(const float4x4 &other) const noexcept {
    return float4x4{};
  };
  float4 operator*(const float4 &other) const noexcept { return float4{}; };
};
struct float3x3 {
  float3x3() noexcept = default;
  float3 cols[3];
  float3x3(const float4x4 &other) noexcept
      : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3 &operator[](std::size_t col) noexcept { return cols[col]; }
  const float3 &operator[](std::size_t col) const noexcept { return cols[col]; }
  float3x3 operator*(const float3x3 &other) const noexcept {
    return float3x3{};
  };
  float3 operator*(const float3 &other) const noexcept { return float3{}; };
};
struct aligned_float3x3 {
  aligned_float3x3() noexcept = default;
  struct col {
    col() noexcept = default;
    col(const float3 &c) noexcept : c(c) {}
    float3 c;
    float p;
  } cols[3];
  aligned_float3x3(const float3x3 &other) noexcept
      : cols{other.cols[0], other.cols[1], other.cols[2]} {}
  aligned_float3x3(const float4x4 &other) noexcept
      : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3 &operator[](std::size_t col) noexcept { return cols[col].c; }
  const float3 &operator[](std::size_t col) const noexcept {
    return cols[col].c;
  }
  float3x3 operator*(const float3x3 &other) const noexcept {
    return float3x3{};
  };
  float3 operator*(const float3 &other) const noexcept { return float3{}; };
};

enum Filter : std::uint8_t { Nearest, Linear };

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
  constexpr sampler(enum Filter MagFilter = Linear,
                    enum Filter MinFilter = Linear,
                    enum Filter MipmapMode = Linear,
                    enum SamplerAddressMode AddressModeU = Repeat,
                    enum SamplerAddressMode AddressModeV = Repeat,
                    enum SamplerAddressMode AddressModeW = Repeat,
                    float MipLodBias = 0.f, enum Compare CompareOp = Never,
                    enum BorderColor BorderColor = TransparentBlack) noexcept
      : MagFilter(MagFilter), MinFilter(MinFilter), MipmapMode(MipmapMode),
        AddressModeU(AddressModeU), AddressModeV(AddressModeV),
        AddressModeW(AddressModeW), MipLodBias(MipLodBias),
        CompareOp(CompareOp), BorderColor(BorderColor) {}
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
#if HSH_ASSERT_CAST_ENABLED
  std::size_t UniqueId;
  template <typename... Args>
  explicit texture_typeless(std::size_t UniqueId, Args &&... args) noexcept
      : UniqueId(UniqueId), Binding(std::forward<Args>(args)...) {}
#else
  template <typename... Args>
  explicit texture_typeless(Args &&... args) noexcept
      : Binding(std::forward<Args>(args)...) {}
  template <typename T>
  explicit texture_typeless(texture1d<T> other) noexcept
      : Binding(other.Binding) {}
  template <typename T>
  explicit texture_typeless(texture1d_array<T> other) noexcept
      : Binding(other.Binding) {}
  template <typename T>
  explicit texture_typeless(texture2d<T> other) noexcept
      : Binding(other.Binding) {}
  template <typename T>
  explicit texture_typeless(texture2d_array<T> other) noexcept
      : Binding(other.Binding) {}
  template <typename T>
  explicit texture_typeless(texture3d<T> other) noexcept
      : Binding(other.Binding) {}
  template <typename T>
  explicit texture_typeless(texturecube<T> other) noexcept
      : Binding(other.Binding) {}
  template <typename T>
  explicit texture_typeless(texturecube_array<T> other) noexcept
      : Binding(other.Binding) {}
#endif
  detail::ActiveTargetTraits::TextureBinding Binding;
  template <typename T> T cast() const noexcept {
    HSH_ASSERT_CAST(UniqueId == typeid(T).hash_code() && "bad cast");
    return static_cast<T>(*this);
  }
  template <typename T> operator T() const noexcept { return cast<T>(); }
};

#if HSH_ASSERT_CAST_ENABLED
#define HSH_CASTABLE_TEXTURE(derived, coordt)                                  \
  template <typename T> struct derived : texture_typeless {                    \
    using MappedType = void;                                                   \
    template <typename... Args>                                                \
    explicit derived(Args &&... args) noexcept                                 \
        : texture_typeless(typeid(*this).hash_code(),                          \
                           std::forward<Args>(args)...) {}                     \
    scalar_to_vector_t<T, 4> sample(coordt, sampler = {}) const noexcept {     \
      return {};                                                               \
    }                                                                          \
  };
#else
#define HSH_CASTABLE_TEXTURE(derived, coordt)                                  \
  template <typename T> struct derived : texture_typeless {                    \
    using MappedType = void;                                                   \
    template <typename... Args>                                                \
    explicit derived(Args &&... args) noexcept                                 \
        : texture_typeless(std::forward<Args>(args)...) {}                     \
    scalar_to_vector_t<T, 4> sample(coordt, sampler = {}) const noexcept {     \
      return {};                                                               \
    }                                                                          \
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
  detail::ActiveTargetTraits::RenderTextureBinding Binding;
};

struct surface {
  detail::ActiveTargetTraits::SurfaceBinding Binding;
  surface() noexcept = default;
  explicit surface(decltype(Binding) Binding) noexcept : Binding(Binding) {}
};

constexpr float dot(const float2 &a, const float2 &b) noexcept {
  return a.x * b.x + a.y * b.y;
}
constexpr float dot(const float3 &a, const float3 &b) noexcept {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
constexpr float sqrt(float v) noexcept { return std::sqrt(v); }
constexpr float length(const float2 &a) noexcept { return sqrt(dot(a, a)); }
constexpr float length(const float3 &a) noexcept { return sqrt(dot(a, a)); }
constexpr float2 normalize(const float2 &a) noexcept { return a / length(a); }
constexpr float3 normalize(const float3 &a) noexcept { return a / length(a); }
constexpr float max(float a, float b) noexcept { return std::max(a, b); }
constexpr float min(float a, float b) noexcept { return std::min(a, b); }
constexpr float clamp(float v, float min, float max) noexcept {
  if (v > max)
    return max;
  else if (v < min)
    return min;
  else
    return v;
}
constexpr float3 clamp(const float3 &v, const float3 &min,
                       const float3 &max) noexcept {
  return float3{clamp(v[0], min[0], max[0]), clamp(v[1], min[1], max[1]),
                clamp(v[2], min[2], max[2])};
}
constexpr float saturate(float v) noexcept { return clamp(v, 0.f, 1.f); }
constexpr float3 saturate(const float3 &v) noexcept {
  return clamp(v, hsh::float3(0.f), hsh::float3(1.f));
}
constexpr float exp2(float v) noexcept { return std::exp2(v); }
constexpr float lerp(float a, float b, float t) noexcept {
  return b * t + a * (1.f - t);
}
constexpr float3 lerp(const float3 &a, const float3 &b, float t) noexcept {
  return float3{
      b[0] * t + a[0] * (1.f - t),
      b[1] * t + a[1] * (1.f - t),
      b[2] * t + a[2] * (1.f - t),
  };
}
constexpr float4 lerp(const float4 &a, const float4 &b,
                      const float4 &t) noexcept {
  return float4{
      b[0] * t[0] + a[0] * (1.f - t[0]), b[1] * t[1] + a[1] * (1.f - t[1]),
      b[2] * t[2] + a[2] * (1.f - t[2]), b[3] * t[3] + a[3] * (1.f - t[3])};
}
constexpr float4 lerp(const float4 &a, const float4 &b, float t) noexcept {
  return float4{b[0] * t + a[0] * (1.f - t), b[1] * t + a[1] * (1.f - t),
                b[2] * t + a[2] * (1.f - t), b[3] * t + a[3] * (1.f - t)};
}
constexpr float abs(float v) noexcept { return std::abs(v); }
constexpr void discard() noexcept {}

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
  CC_Red = 1,
  CC_Green = 2,
  CC_Blue = 4,
  CC_Alpha = 8
};

enum ColorSwizzle : std::uint8_t {
  CS_Identity,
  CS_Red,
  CS_Green,
  CS_Blue,
  CS_Alpha
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
          BlendFactor DstColorBlendFactor = Zero, BlendOp ColorBlendOp = Add,
          BlendFactor SrcAlphaBlendFactor = SrcColorBlendFactor,
          BlendFactor DstAlphaBlendFactor = DstColorBlendFactor,
          BlendOp AlphaBlendOp = ColorBlendOp,
          ColorComponentFlags ColorWriteComponents =
              ColorComponentFlags(CC_Red | CC_Green | CC_Blue | CC_Alpha)>
struct color_attachment : base_attribute<true> {};
template <Topology T = Triangles> struct topology : base_attribute<> {};
template <unsigned P = 0> struct patch_control_points : base_attribute<> {};
template <CullMode CM = CullNone> struct cull_mode : base_attribute<> {};
template <Compare C = Always> struct depth_compare : base_attribute<> {};
template <bool W = true> struct depth_write : base_attribute<> {};
template <bool E = false>
struct early_depth_stencil : base_attribute<false, true> {};
template <typename... Attrs> struct pipeline {
  hsh::float4 position;
  static constexpr std::size_t color_attachment_count =
      ((Attrs::is_ca ? 1 : 0) + ...);
  std::array<hsh::float4, color_attachment_count> color_out;
};
} // namespace pipeline

namespace detail {

template <typename WordType> struct ShaderDataBlob {
  std::size_t Size = 0;
  const WordType *Data = nullptr;
  std::uint64_t Hash = 0;
  constexpr ShaderDataBlob() noexcept = default;
  template <std::size_t N>
  constexpr ShaderDataBlob(const WordType (&Data)[N],
                           std::uint64_t Hash) noexcept
      : Size(N * sizeof(WordType)), Data(Data), Hash(Hash) {}
  constexpr operator bool() const noexcept { return Data != nullptr; }
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
  constexpr VertexBinding() noexcept
      : Stride(0), Binding(0), InputRate(PerVertex) {}
  constexpr VertexBinding(std::uint8_t Binding, std::uint32_t Stride,
                          enum InputRate InputRate) noexcept
      : Stride(Stride), Binding(Binding), InputRate(InputRate) {}
};

constexpr std::size_t HshFormatToTexelSize(Format format) noexcept {
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

constexpr bool HshFormatIsInteger(Format format) noexcept {
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
  constexpr VertexAttribute() noexcept = default;
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
  enum ColorComponentFlags ColorWriteComponents =
      ColorComponentFlags(CC_Red | CC_Green | CC_Blue | CC_Alpha);
  constexpr bool blendEnabled() const noexcept {
    return SrcColorBlendFactor == One && DstColorBlendFactor == Zero &&
           ColorBlendOp == Add && SrcAlphaBlendFactor == One &&
           DstAlphaBlendFactor == Zero && AlphaBlendOp == Add;
  }
  constexpr ColorAttachment() noexcept = default;
  constexpr ColorAttachment(
      enum BlendFactor SrcColorBlendFactor,
      enum BlendFactor DstColorBlendFactor, enum BlendOp ColorBlendOp,
      enum BlendFactor SrcAlphaBlendFactor,
      enum BlendFactor DstAlphaBlendFactor, enum BlendOp AlphaBlendOp,
      std::underlying_type_t<ColorComponentFlags> ColorWriteComponents) noexcept
      : SrcColorBlendFactor(SrcColorBlendFactor),
        DstColorBlendFactor(DstColorBlendFactor), ColorBlendOp(ColorBlendOp),
        SrcAlphaBlendFactor(SrcAlphaBlendFactor),
        DstAlphaBlendFactor(DstAlphaBlendFactor), AlphaBlendOp(AlphaBlendOp),
        ColorWriteComponents(ColorComponentFlags(ColorWriteComponents)) {}
};

/* Holds constant pipeline information */
struct PipelineInfo {
  enum Topology Topology = Triangles;
  unsigned PatchControlPoints = 0;
  enum CullMode CullMode = CullNone;
  enum Compare DepthCompare = Always;
  bool DepthWrite = true;
  constexpr PipelineInfo() noexcept = default;
  constexpr PipelineInfo(enum Topology Topology, unsigned PatchControlPoints,
                         enum CullMode CullMode, enum Compare DepthCompare,
                         bool DepthWrite) noexcept
      : Topology(Topology), PatchControlPoints(PatchControlPoints),
        CullMode(CullMode), DepthCompare(DepthCompare), DepthWrite(DepthWrite) {
  }
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
                            struct PipelineInfo PipelineInfo) noexcept
      : StageCodes(S), Bindings(B), Attributes(A), Samplers(Samps),
        Attachments(Atts), PipelineInfo(PipelineInfo) {}
};

template <Target T, std::uint32_t NStages, std::uint32_t NSamplers>
struct ShaderData {
  using ObjectRef = std::reference_wrapper<ShaderObject<T>>;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = std::reference_wrapper<SamplerObject<T>>;
  std::array<SamplerRef, NSamplers> SamplerObjects;
  constexpr ShaderData(std::array<ObjectRef, NStages> S,
                       std::array<SamplerRef, NSamplers> Samps) noexcept
      : ShaderObjects(S), SamplerObjects(Samps) {}
};

template <hsh::Target T> struct PipelineBuilder {
  template <typename... B> static void CreatePipelines() noexcept {
    assert(false && "unimplemented pipeline builder");
  }
  template <typename... B> static void DestroyPipelines() noexcept {
    assert(false && "unimplemented pipeline builder");
  }
};

namespace buffer_math {
constexpr std::size_t MipSize2D(std::size_t width, std::size_t height,
                                std::size_t texelSize, bool NotZero) noexcept {
  return NotZero ? (width * height * texelSize) : 0;
}
template <std::size_t... Idx>
constexpr std::size_t MipOffset2D(std::size_t width, std::size_t height,
                                  std::size_t texelSize, std::size_t Mip,
                                  std::index_sequence<Idx...>) noexcept {
  return (MipSize2D(width >> Idx, height >> Idx, texelSize, Idx < Mip) + ...);
}
constexpr std::size_t MipOffset2D(std::size_t width, std::size_t height,
                                  std::size_t texelSize,
                                  std::size_t Mip) noexcept {
  return MipOffset2D(width, height, texelSize, Mip,
                     std::make_index_sequence<MaxMipCount>());
}
} // namespace buffer_math

#if HSH_ENABLE_VULKAN
constexpr vk::Format HshToVkFormat(Format Format) noexcept {
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

constexpr vk::VertexInputRate HshToVkInputRate(InputRate InputRate) noexcept {
  switch (InputRate) {
  case PerVertex:
    return vk::VertexInputRate::eVertex;
  case PerInstance:
    return vk::VertexInputRate::eInstance;
  }
}

constexpr vk::PrimitiveTopology
HshToVkTopology(enum Topology Topology) noexcept {
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

constexpr vk::CullModeFlagBits
HshToVkCullMode(enum CullMode CullMode) noexcept {
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

constexpr vk::CompareOp HshToVkCompare(enum Compare Compare) noexcept {
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

constexpr vk::BlendFactor
HshToVkBlendFactor(enum BlendFactor BlendFactor) noexcept {
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

constexpr vk::BlendOp HshToVkBlendOp(enum BlendOp BlendOp) noexcept {
  switch (BlendOp) {
  case Add:
    return vk::BlendOp::eAdd;
  case Subtract:
    return vk::BlendOp::eSubtract;
  case ReverseSubtract:
    return vk::BlendOp::eReverseSubtract;
  }
}

constexpr vk::Filter HshToVkFilter(enum Filter Filter) noexcept {
  switch (Filter) {
  case Nearest:
    return vk::Filter::eNearest;
  case Linear:
    return vk::Filter::eLinear;
  }
}

constexpr vk::SamplerMipmapMode HshToVkMipMode(enum Filter Filter) noexcept {
  switch (Filter) {
  case Nearest:
    return vk::SamplerMipmapMode::eNearest;
  case Linear:
    return vk::SamplerMipmapMode::eLinear;
  }
}

constexpr vk::SamplerAddressMode
HshToVkAddressMode(enum SamplerAddressMode AddressMode) noexcept {
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

constexpr vk::BorderColor HshToVkBorderColor(enum BorderColor BorderColor,
                                             bool Int) noexcept {
  switch (BorderColor) {
  case TransparentBlack:
    return Int ? vk::BorderColor::eIntTransparentBlack
               : vk::BorderColor::eFloatTransparentBlack;
  case OpaqueBlack:
    return Int ? vk::BorderColor::eIntOpaqueBlack
               : vk::BorderColor::eFloatOpaqueBlack;
  case OpaqueWhite:
    return Int ? vk::BorderColor::eIntOpaqueWhite
               : vk::BorderColor::eFloatOpaqueWhite;
  }
}

constexpr vk::ShaderStageFlagBits
HshToVkShaderStage(enum Stage Stage) noexcept {
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

constexpr vk::ColorComponentFlagBits
HshToVkColorComponentFlags(enum ColorComponentFlags Comps) noexcept {
  return vk::ColorComponentFlagBits(
      (Comps & CC_Red ? unsigned(vk::ColorComponentFlagBits::eR) : 0u) |
      (Comps & CC_Green ? unsigned(vk::ColorComponentFlagBits::eG) : 0u) |
      (Comps & CC_Blue ? unsigned(vk::ColorComponentFlagBits::eB) : 0u) |
      (Comps & CC_Alpha ? unsigned(vk::ColorComponentFlagBits::eA) : 0u));
}

constexpr vk::ComponentSwizzle
HshToVkComponentSwizzle(enum ColorSwizzle swizzle) noexcept {
  switch (swizzle) {
  case CS_Identity:
    return vk::ComponentSwizzle::eIdentity;
  case CS_Red:
    return vk::ComponentSwizzle::eR;
  case CS_Green:
    return vk::ComponentSwizzle::eG;
  case CS_Blue:
    return vk::ComponentSwizzle::eB;
  case CS_Alpha:
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
  vk::ShaderModule Get(const vk::ShaderModuleCreateInfo &Info) noexcept {
    if (!ShaderModule)
      ShaderModule =
          vulkan::Globals.Device.createShaderModuleUnique(Info).value;
    return ShaderModule.get();
  }
  void Destroy() noexcept { ShaderModule.reset(); }
};

template <> struct SamplerObject<VULKAN_SPIRV> {
  std::array<std::array<vk::UniqueSampler, MaxMipCount - 1>, 2> Samplers;
  SamplerObject() noexcept = default;
  vk::Sampler Get(const vk::SamplerCreateInfo &Info, bool Int,
                  unsigned MipCount) noexcept {
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
  vk::Sampler Get(const vk::SamplerCreateInfo &Info,
                  texture_typeless tex) noexcept {
    return Get(Info, tex.Binding.get_VULKAN_SPIRV().Integer,
               tex.Binding.get_VULKAN_SPIRV().NumMips);
  }
  void Destroy() noexcept {
    for (auto &SampI : Samplers)
      for (auto &Samp : SampI)
        Samp.reset();
  }
};

namespace vulkan {
template <typename Impl> struct DescriptorPoolWrites {
  std::size_t NumWrites = 0;
  std::array<vk::WriteDescriptorSet, MaxUniforms + MaxImages + MaxSamplers>
      Writes;
  std::array<vk::DescriptorBufferInfo, MaxUniforms> Uniforms;
  std::array<vk::DescriptorImageInfo, MaxImages> Images;
  std::array<vk::DescriptorImageInfo, MaxSamplers> Samplers;
  template <std::size_t... USeq, std::size_t... ISeq, std::size_t... SSeq>
  constexpr DescriptorPoolWrites(std::index_sequence<USeq...>,
                                 std::index_sequence<ISeq...>,
                                 std::index_sequence<SSeq...>) noexcept
      : Uniforms{vk::DescriptorBufferInfo({}, ((void)USeq, 0),
                                          VK_WHOLE_SIZE)...},
        Images{vk::DescriptorImageInfo(
            {}, {}, ((void)ISeq, vk::ImageLayout::eShaderReadOnlyOptimal))...},
        Samplers{vk::DescriptorImageInfo(
            {}, {}, ((void)SSeq, vk::ImageLayout::eUndefined))...} {}

  template <typename... Args>
  constexpr explicit DescriptorPoolWrites(vk::DescriptorSet DstSet,
                                          Args... args) noexcept
      : DescriptorPoolWrites(std::make_index_sequence<MaxUniforms>(),
                             std::make_index_sequence<MaxImages>(),
                             std::make_index_sequence<MaxSamplers>()) {
    Iterators Its(DstSet, *this);
    (Its.add(args), ...);
    NumWrites = Its.WriteIt - Its.WriteBegin;
  }

  struct Iterators {
    vk::DescriptorSet DstSet;
    decltype(Writes)::iterator WriteBegin;
    decltype(Uniforms)::iterator UniformBegin;
    decltype(Images)::iterator ImageBegin;
    decltype(Samplers)::iterator SamplerBegin;
    decltype(Writes)::iterator WriteIt;
    decltype(Uniforms)::iterator UniformIt;
    decltype(Images)::iterator ImageIt;
    decltype(Samplers)::iterator SamplerIt;
    constexpr explicit Iterators(vk::DescriptorSet DstSet,
                                 DescriptorPoolWrites &Writes) noexcept
        : DstSet(DstSet), WriteBegin(Writes.Writes.begin()),
          UniformBegin(Writes.Uniforms.begin()),
          ImageBegin(Writes.Images.begin()),
          SamplerBegin(Writes.Samplers.begin()), WriteIt(Writes.Writes.begin()),
          UniformIt(Writes.Uniforms.begin()), ImageIt(Writes.Images.begin()),
          SamplerIt(Writes.Samplers.begin()) {}
    void add(uniform_buffer_typeless uniform) noexcept {
      auto UniformIdx = UniformIt - UniformBegin;
      auto &Uniform = *UniformIt++;
      Uniform.setBuffer(uniform.Binding.get_VULKAN_SPIRV());
      auto &Write = *WriteIt++;
      Write.setDstSet(DstSet);
      Write.setDstBinding(UniformIdx);
      Write.setDescriptorCount(1);
      Write.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic);
      Write.setPBufferInfo(&Uniform);
    }
    void add(dynamic_uniform_buffer_typeless uniform) noexcept {
      auto UniformIdx = UniformIt - UniformBegin;
      auto &Uniform = *UniformIt++;
      Uniform.setBuffer(uniform.Binding.get_VULKAN_SPIRV().getBuffer());
      auto &Write = *WriteIt++;
      Write.setDstSet(DstSet);
      Write.setDstBinding(UniformIdx);
      Write.setDescriptorCount(1);
      Write.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic);
      Write.setPBufferInfo(&Uniform);
    }
    static void add(vertex_buffer_typeless) noexcept {}
    static void add(dynamic_vertex_buffer_typeless) noexcept {}
    void add(texture_typeless texture) noexcept {
      auto ImageIdx = ImageIt - ImageBegin;
      auto &Image = *ImageIt++;
      Image.setImageView(texture.Binding.get_VULKAN_SPIRV().ImageView);
      auto &Write = *WriteIt++;
      Write.setDstSet(DstSet);
      Write.setDstBinding(MaxUniforms + ImageIdx);
      Write.setDescriptorCount(1);
      Write.setDescriptorType(vk::DescriptorType::eSampledImage);
      Write.setPImageInfo(&Image);
    }
    void add(render_texture2d texture) noexcept {
      auto ImageIdx = ImageIt - ImageBegin;
      auto &Image = *ImageIt++;
      Image.setImageView(texture.Binding.get_VULKAN_SPIRV().getImageView());
      auto &Write = *WriteIt++;
      Write.setDstSet(DstSet);
      Write.setDstBinding(MaxUniforms + ImageIdx);
      Write.setDescriptorCount(1);
      Write.setDescriptorType(vk::DescriptorType::eSampledImage);
      Write.setPImageInfo(&Image);
    }
    void add(hsh::detail::SamplerBinding sampler) noexcept {
      auto SamplerIdx = SamplerIt - SamplerBegin;
      auto &Sampler = *SamplerIt++;
      Sampler.setSampler(
          Impl::data_VULKAN_SPIRV.SamplerObjects[sampler.idx].get().Get(
              Impl::cdata_VULKAN_SPIRV.Samplers[sampler.idx], sampler.tex));
      auto &Write = *WriteIt++;
      Write.setDstSet(DstSet);
      Write.setDstBinding(MaxUniforms + MaxImages + SamplerIdx);
      Write.setDescriptorCount(1);
      Write.setDescriptorType(vk::DescriptorType::eSampler);
      Write.setPImageInfo(&Sampler);
    }
  };
};
} // namespace vulkan

template <typename Impl, typename... Args>
TargetTraits<VULKAN_SPIRV>::PipelineBinding::PipelineBinding(
    ClassWrapper<Impl>, Args... args) noexcept
    : Pipeline(Impl::data_VULKAN_SPIRV.Pipeline.get()),
      DescriptorSet(vulkan::Globals.DescriptorPoolChain->allocate()) {
  vulkan::DescriptorPoolWrites<Impl> Writes(DescriptorSet, args...);
  vulkan::Globals.Device.updateDescriptorSets(Writes.NumWrites,
                                              Writes.Writes.data(), 0, nullptr);
  Iterators Its(*this);
  (Its.add(args), ...);
  NumVertexBuffers = Its.VertexBufferIt - Its.VertexBufferBegin;
}

void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(
    uniform_buffer_typeless) noexcept {
  UniformOffsetIt++;
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(
    dynamic_uniform_buffer_typeless uniform) noexcept {
  *UniformOffsetIt++ = uniform.Binding.get_VULKAN_SPIRV().getSecondOffset();
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(
    vertex_buffer_typeless uniform) noexcept {
  *VertexBufferIt++ = uniform.Binding.get_VULKAN_SPIRV();
  VertexOffsetIt++;
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(
    dynamic_vertex_buffer_typeless uniform) noexcept {
  *VertexBufferIt++ = uniform.Binding.get_VULKAN_SPIRV().getBuffer();
  *VertexOffsetIt++ = uniform.Binding.get_VULKAN_SPIRV().getSecondOffset();
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(
    texture_typeless) noexcept {
  TextureIdx++;
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(
    render_texture2d texture) noexcept {
  auto &RT = *RenderTextureIt++;
  RT.RenderTextureBinding = texture.Binding.get_VULKAN_SPIRV();
  RT.KnownImageView = RT.RenderTextureBinding.getImageView();
  RT.DescriptorBindingIdx = MaxUniforms + TextureIdx++;
}
void TargetTraits<VULKAN_SPIRV>::PipelineBinding::Iterators::add(
    SamplerBinding) noexcept {}

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

  template <std::size_t... SSeq, std::size_t... BSeq, std::size_t... ASeq,
            std::size_t... SampSeq, std::size_t... AttSeq>
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
                            std::index_sequence<AttSeq...>) noexcept
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
        InputAssemblyState{{},
                           HshToVkTopology(PipelineInfo.Topology),
                           PipelineInfo.Topology == TriangleStrip},
        TessellationState{{}, PipelineInfo.PatchControlPoints},
        RasterizationState{{},
                           VK_FALSE,
                           VK_FALSE,
                           vk::PolygonMode::eFill,
                           HshToVkCullMode(PipelineInfo.CullMode),
                           vk::FrontFace::eCounterClockwise,
                           {},
                           {},
                           {},
                           {},
                           1.f},
        DepthStencilState{{},
                          PipelineInfo.DepthCompare != Always,
                          PipelineInfo.DepthWrite,
                          HshToVkCompare(PipelineInfo.DepthCompare)},
        ColorBlendState{{},
                        VK_FALSE,
                        vk::LogicOp ::eClear,
                        NAttachments,
                        TargetAttachments.data()},
        Samplers{vk::SamplerCreateInfo{
            {},
            HshToVkFilter(std::get<SampSeq>(Samps).MagFilter),
            HshToVkFilter(std::get<SampSeq>(Samps).MinFilter),
            HshToVkMipMode(std::get<SampSeq>(Samps).MipmapMode),
            HshToVkAddressMode(std::get<SampSeq>(Samps).AddressModeU),
            HshToVkAddressMode(std::get<SampSeq>(Samps).AddressModeV),
            HshToVkAddressMode(std::get<SampSeq>(Samps).AddressModeW),
            std::get<SampSeq>(Samps).MipLodBias,
            0,
            0,
            std::get<SampSeq>(Samps).CompareOp != Never,
            HshToVkCompare(std::get<SampSeq>(Samps).CompareOp),
            0,
            0,
            HshToVkBorderColor(std::get<SampSeq>(Samps).BorderColor,
                               false)}...} {}

  constexpr ShaderConstData(std::array<ShaderCode<VULKAN_SPIRV>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A,
                            std::array<sampler, NSamplers> Samps,
                            std::array<ColorAttachment, NAttachments> Atts,
                            struct PipelineInfo PipelineInfo) noexcept
      : ShaderConstData(S, B, A, Samps, Atts, PipelineInfo,
                        std::make_index_sequence<NStages>(),
                        std::make_index_sequence<NBindings>(),
                        std::make_index_sequence<NAttributes>(),
                        std::make_index_sequence<NSamplers>(),
                        std::make_index_sequence<NAttachments>()) {}

  static constexpr std::array<vk::DynamicState, 2> Dynamics{
      vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  static constexpr vk::PipelineDynamicStateCreateInfo DynamicState{
      {}, 2, Dynamics.data()};
  static constexpr vk::PipelineViewportStateCreateInfo ViewportState{
      {}, 1, {}, 1, {}};

  template <typename B>
  vk::GraphicsPipelineCreateInfo
  getPipelineInfo(VkPipelineShaderStageCreateInfo *StageInfos) const noexcept {
    for (std::size_t i = 0; i < NStages; ++i)
      StageInfos[i] = vk::PipelineShaderStageCreateInfo{
          {},
          StageFlags[i],
          B::data_VULKAN_SPIRV.ShaderObjects[i].get().Get(StageCodes[i]),
          "main"};

    return vk::GraphicsPipelineCreateInfo{
        {},
        NStages,
        reinterpret_cast<vk::PipelineShaderStageCreateInfo *>(StageInfos),
        &VertexInputState,
        &InputAssemblyState,
        &TessellationState,
        &ViewportState,
        &RasterizationState,
        &vulkan::Globals.MultisampleState,
        &DepthStencilState,
        &ColorBlendState,
        &DynamicState,
        vulkan::Globals.PipelineLayout,
        vulkan::Globals.GetRenderPass()};
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
                       std::array<SamplerRef, NSamplers> Samps) noexcept
      : ShaderObjects(S), SamplerObjects(Samps) {}
  void Destroy() noexcept {
    for (auto &Obj : ShaderObjects)
      Obj.get().Destroy();
    for (auto &Obj : SamplerObjects)
      Obj.get().Destroy();
    Pipeline.reset();
  }
};

template <> struct PipelineBuilder<VULKAN_SPIRV> {
  template <typename B>
  static constexpr std::size_t GetNumStages(bool NotZero) noexcept {
    return NotZero ? B::cdata_VULKAN_SPIRV.StageCodes.size() : 0;
  }
  template <typename... B, std::size_t... BSeq>
  static constexpr std::size_t
  StageInfoStart(std::size_t BIdx, std::index_sequence<BSeq...>) noexcept {
    return (GetNumStages<B>(BSeq < BIdx) + ...);
  }
  template <typename B> static void SetPipeline(vk::Pipeline data) noexcept {
    vk::ObjectDestroy<vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(
        vulkan::Globals.Device, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    B::data_VULKAN_SPIRV.Pipeline = vk::UniquePipeline(data, deleter);
  }
  template <typename... B, std::size_t... BSeq>
  static void CreatePipelines(std::index_sequence<BSeq...> seq) noexcept {
    std::array<VkPipelineShaderStageCreateInfo, (GetNumStages<B>(true) + ...)>
        ShaderStageInfos;
    std::array<vk::GraphicsPipelineCreateInfo, sizeof...(B)> Infos{
        B::cdata_VULKAN_SPIRV.template getPipelineInfo<B>(
            ShaderStageInfos.data() + StageInfoStart<B...>(BSeq, seq))...};
    std::array<vk::Pipeline, sizeof...(B)> Pipelines;
    auto Result = vulkan::Globals.Device.createGraphicsPipelines(
        {}, Infos.size(), Infos.data(), nullptr, Pipelines.data());
    VULKAN_HPP_ASSERT(Result == vk::Result::eSuccess);
    (SetPipeline<B>(Pipelines[BSeq]), ...);
  }
  template <typename... B> static void CreatePipelines() noexcept {
    CreatePipelines<B...>(std::make_index_sequence<sizeof...(B)>());
  }
  template <typename... B> static void DestroyPipelines() noexcept {
    (B::data_VULKAN_SPIRV.Destroy(), ...);
  }
};

namespace buffer_math::vulkan {
template <std::size_t... Idx>
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies2D(std::size_t width, std::size_t height, std::size_t texelSize,
             vk::ImageAspectFlagBits aspect,
             std::index_sequence<Idx...>) noexcept {
  return {vk::BufferImageCopy(
      MipOffset2D(width, height, texelSize, Idx), width >> Idx, height >> Idx,
      {aspect, Idx, 0, 1}, {},
      {uint32_t(width >> Idx), uint32_t(height >> Idx), 1})...};
}
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies2D(std::size_t width, std::size_t height, std::size_t texelSize,
             vk::ImageAspectFlagBits aspect) noexcept {
  return MakeCopies2D(width, height, texelSize, aspect,
                      std::make_index_sequence<MaxMipCount>());
}
} // namespace buffer_math::vulkan

template <typename T>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<uniform_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location,
                     CopyFunc copyFunc) noexcept {
    auto UploadBuffer = vulkan::AllocateUploadBuffer(
        location.with_field("UniformBufferUpload"), sizeof(T));
    copyFunc(UploadBuffer.getMappedData(), sizeof(T));

    TargetTraits<VULKAN_SPIRV>::UniformBufferOwner Ret =
        vulkan::AllocateStaticBuffer(location, sizeof(T),
                                     vk::BufferUsageFlagBits::eUniformBuffer);

    vulkan::Globals.Cmd.copyBuffer(UploadBuffer.getBuffer(), Ret.getBuffer(),
                                   vk::BufferCopy(0, 0, sizeof(T)));

    return Ret;
  }
};

template <typename T>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<dynamic_uniform_buffer<T>> {
  static auto Create(const SourceLocation &location) noexcept {
    return vulkan::AllocateDynamicBuffer(
        location, sizeof(T), vk::BufferUsageFlagBits::eUniformBuffer);
  }
};

template <typename T>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<vertex_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, std::size_t Count,
                     CopyFunc copyFunc) noexcept {
    std::size_t Size = sizeof(T) * Count;
    auto UploadBuffer = vulkan::AllocateUploadBuffer(
        location.with_field("VertexBufferUpload"), Size);
    copyFunc(UploadBuffer.getMappedData(), Size);

    TargetTraits<VULKAN_SPIRV>::UniformBufferOwner Ret =
        vulkan::AllocateStaticBuffer(location, Size,
                                     vk::BufferUsageFlagBits::eVertexBuffer |
                                         vk::BufferUsageFlagBits::eTransferDst);

    vulkan::Globals.Cmd.copyBuffer(UploadBuffer.getBuffer(), Ret.getBuffer(),
                                   vk::BufferCopy(0, 0, Size));

    return Ret;
  }
};

template <typename T>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<dynamic_vertex_buffer<T>> {
  static auto Create(const SourceLocation &location,
                     std::size_t Count) noexcept {
    return vulkan::AllocateDynamicBuffer(
        location, sizeof(T) * Count, vk::BufferUsageFlagBits::eVertexBuffer);
  }
};

template <typename TexelType>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<texture2d<TexelType>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, Extent2D extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    auto TexelSize = HshFormatToTexelSize(format);
    auto TexelFormat = HshToVkFormat(format);
    std::array<vk::BufferImageCopy, MaxMipCount> Copies =
        buffer_math::vulkan::MakeCopies2D(extent.w, extent.h, TexelSize,
                                          vk::ImageAspectFlagBits::eColor);
    auto BufferSize =
        buffer_math::MipOffset2D(extent.w, extent.h, TexelSize, numMips);
    auto UploadBuffer = vulkan::AllocateUploadBuffer(
        location.with_field("TextureUpload"), BufferSize);
    copyFunc(UploadBuffer.getMappedData(), BufferSize);

    TargetTraits<VULKAN_SPIRV>::TextureOwner Ret{
        vulkan::AllocateTexture(
            location,
            vk::ImageCreateInfo({}, vk::ImageType::e2D, TexelFormat,
                                {extent.w, extent.h, 1}, numMips, 1,
                                vk::SampleCountFlagBits::e1,
                                vk::ImageTiling::eOptimal,
                                vk::ImageUsageFlagBits::eSampled |
                                    vk::ImageUsageFlagBits::eTransferDst,
                                {}, {}, {}, vk::ImageLayout::eUndefined)),
        {},
        std::uint8_t(numMips),
        HshFormatIsInteger(format)};
    Ret.ImageView =
        vulkan::Globals.Device
            .createImageViewUnique(vk::ImageViewCreateInfo(
                {}, Ret.Allocation.GetImage(), vk::ImageViewType::e2D,
                TexelFormat,
                vk::ComponentMapping(HshToVkComponentSwizzle(redSwizzle),
                                     HshToVkComponentSwizzle(greenSwizzle),
                                     HshToVkComponentSwizzle(blueSwizzle),
                                     HshToVkComponentSwizzle(alphaSwizzle)),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                          numMips, 0, 1)))
            .value;
    vulkan::Globals.SetDebugObjectName(location.with_field("ImageView"),
                                       Ret.ImageView.get());

    vulkan::Globals.Cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits(0), vk::AccessFlagBits::eTransferWrite,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            Ret.Allocation.GetImage(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                      VK_REMAINING_MIP_LEVELS, 0,
                                      VK_REMAINING_ARRAY_LAYERS)));
    vulkan::Globals.Cmd.copyBufferToImage(
        UploadBuffer.getBuffer(), Ret.Allocation.GetImage(),
        vk::ImageLayout::eTransferDstOptimal, numMips, Copies.data());
    vulkan::Globals.Cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eVertexShader |
            vk::PipelineStageFlagBits::eTessellationControlShader |
            vk::PipelineStageFlagBits::eTessellationEvaluationShader |
            vk::PipelineStageFlagBits::eGeometryShader |
            vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlagBits::eByRegion, {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal, VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED, Ret.Allocation.GetImage(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                      VK_REMAINING_MIP_LEVELS, 0,
                                      VK_REMAINING_ARRAY_LAYERS)));

    return Ret;
  }
};

template <>
struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<render_texture2d> {
  static auto Create(const SourceLocation &location, surface Surf,
                     uint32_t NumColorBindings = 0,
                     uint32_t NumDepthBindings = 0) noexcept {
    return TargetTraits<VULKAN_SPIRV>::RenderTextureOwner{
        std::make_unique<vulkan::RenderTextureAllocation>(
            location, Surf.Binding.get_VULKAN_SPIRV().Allocation,
            NumColorBindings, NumDepthBindings)};
  }
};

template <> struct TargetTraits<VULKAN_SPIRV>::ResourceFactory<surface> {
  static auto Create(const SourceLocation &location,
                     vk::UniqueSurfaceKHR Surface) noexcept {
    vulkan::Globals.SetDebugObjectName(location.with_field("Surface"),
                                       Surface.get());
    if (!vulkan::Globals.CheckSurfaceSupported(Surface.get())) {
      // TODO: make this return a detectable null object
      assert(false && "surface not supported");
      return TargetTraits<VULKAN_SPIRV>::SurfaceOwner{};
    }
    return TargetTraits<VULKAN_SPIRV>::SurfaceOwner{
        std::make_unique<vulkan::SurfaceAllocation>(location,
                                                    std::move(Surface))};
  }

#ifdef VK_USE_PLATFORM_XCB_KHR
  static auto Create(const SourceLocation &location, xcb_connection_t *Conn,
                     xcb_window_t Window) noexcept {
    return Create(location,
                  vulkan::Globals.Instance
                      .createXcbSurfaceKHRUnique(
                          vk::XcbSurfaceCreateInfoKHR({}, Conn, Window))
                      .value);
  }
#endif
};
#endif

struct GlobalListNode {
  static GlobalListNode *Head;
  typedef void (*BuildFunc)() noexcept;
  struct FuncPair {
    BuildFunc Create, Destroy;
  };
  std::array<FuncPair, ACTIVE_TARGET_MAX> Funcs;
  GlobalListNode *Next;
  template <typename... Args>
  explicit GlobalListNode(Args... Funcs) noexcept
      : Funcs{Funcs...}, Next(Head) {
    Head = this;
  }
};
inline GlobalListNode *GlobalListNode::Head = nullptr;

template <template <hsh::Target> typename Impl>
struct PipelineCoordinatorNode : GlobalListNode {
  PipelineCoordinatorNode()
      : GlobalListNode{
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  GlobalListNode::FuncPair{Impl<Enumeration>::Create,                          \
                           Impl<Enumeration>::Destroy},
#include "targets.def"
        } {
  }
};

template <typename... B> struct PipelineCoordinator {
  template <hsh::Target T> struct Impl {
    static void Create() noexcept {
      PipelineBuilder<T>::template CreatePipelines<B...>();
    }
    static void Destroy() noexcept {
      PipelineBuilder<T>::template DestroyPipelines<B...>();
    }
  };
  static PipelineCoordinatorNode<Impl> global;
};

/*
 * This macro is internally expanded within the hsh generator
 * for any identifiers prefixed with hsh_ being assigned or returned.
 */
#define _hsh_dummy(...) ::hsh::binding_typeless{};
} // namespace detail

#define HSH_PROFILE_MODE 1

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

  T get() const noexcept { return T(typename decltype(Owner)::Binding(Owner)); }
  operator T() const noexcept { return get(); }
};

template <typename T>
struct dynamic_resource_owner_base : resource_owner_base<T> {
  using resource_owner_base<T>::resource_owner_base;
  using MappedType = typename T::MappedType;
  MappedType *map() noexcept {
    return reinterpret_cast<MappedType *>(resource_owner_base<T>::Owner.map());
  }
  void unmap() noexcept { resource_owner_base<T>::Owner.unmap(); }
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
                                         Args... args) noexcept {
  return resource_owner<T>(
      detail::ActiveTargetTraits::CreateResource<T>(location, args...));
}

template <typename T, typename... Args>
inline resource_owner<T> create_resource(Args... args) noexcept {
  return resource_owner<T>(detail::ActiveTargetTraits::CreateResource<T>(
      SourceLocation::current(), args...));
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
    Extent2D extent, Format format, uint32_t numMips, CopyFunc copyFunc,
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

  render_texture2d getColor(uint32_t idx) const noexcept {
    return {Owner.getColor(idx)};
  }
  render_texture2d getDepth(uint32_t idx) const noexcept {
    return {Owner.getDepth(idx)};
  }
  void attach() noexcept { Owner.attach(); }
  void resolveSurface(surface surface, bool reattach = false) noexcept {
    Owner.resolveSurface(surface.Binding, reattach);
  }
  void resolveColorBinding(uint32_t idx, Rect2D region,
                           bool reattach = true) noexcept {
    Owner.resolveColorBinding(idx, region, reattach);
  }
  void resolveDepthBinding(uint32_t idx, Rect2D region,
                           bool reattach = true) noexcept {
    Owner.resolveDepthBinding(idx, region, reattach);
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
  bool acquireNextImage() noexcept {
    return resource_owner_base<surface>::Owner.acquireNextImage();
  }
};

#ifdef VK_USE_PLATFORM_XCB_KHR
inline resource_owner<surface> create_surface(
    xcb_connection_t *Conn, xcb_window_t Window,
    const SourceLocation &location = SourceLocation::current()) noexcept {
  return create_resource<surface>(location, Conn, Window);
}
#endif

class binding_typeless {
protected:
  detail::ActiveTargetTraits::PipelineBinding Data;
  template <typename... Args>
  explicit binding_typeless(Args... args) noexcept : Data(args...) {}

public:
  operator bool() const noexcept { return Data.isValid(); }
  binding_typeless() noexcept = default;
  void draw(uint32_t start, uint32_t count) noexcept {
    Data.draw(start, count);
  }
};

template <typename Impl> class binding : public binding_typeless {
protected:
  template <typename... Args>
  explicit binding(Args... args) noexcept
      : binding_typeless(detail::ClassWrapper<Impl>(), args...) {}
};

#if HSH_PROFILE_MODE
struct value_formatter {
  template <typename T>
  static std::ostream &format(std::ostream &out, T val) noexcept {
    return out << val;
  }
};

class profiler {
  friend class profile_context;
  const char *source = nullptr;

public:
  struct push {
    const char *name;
    explicit push(const char *name) noexcept : name(name) {}
  };
  struct pop {};
  struct cast_base {};
  template <typename T> struct cast : cast_base {
    const char *type;
    T val;
    explicit cast(const char *type, T val) noexcept : type(type), val(val) {}
  };

private:
  template <typename T>
  using EnableIfNonControlArg =
      std::enable_if_t<!std::is_same_v<T, push> && !std::is_same_v<T, pop> &&
                           !std::is_same_v<T, const char *> &&
                           !std::is_base_of_v<cast_base, T>,
                       int>;
  struct node {
    std::map<std::string, node> children;
    std::string leaf;
    node &get() noexcept { return *this; }
    template <typename... Args> node &get(push, Args... rest) noexcept {
      return get(rest...);
    }
    template <typename... Args> node &get(pop, Args... rest) noexcept {
      return get(rest...);
    }
    template <typename... Args>
    node &get(const char *arg, Args... rest) noexcept {
      return get(rest...);
    }
    template <typename T, typename... Args>
    node &get(cast<T> arg, Args... rest) noexcept {
      std::ostringstream ss;
      hsh::value_formatter::format(ss, arg.val);
      return children[ss.str()].get(rest...);
    }
    template <typename T, typename... Args, EnableIfNonControlArg<T> = 0>
    node &get(T arg, Args... rest) noexcept {
      std::ostringstream ss;
      hsh::value_formatter::format(ss, arg);
      return children[ss.str()].get(rest...);
    }
    void write(std::ostream &out, const char *src, unsigned &idx) const
        noexcept {
      if (!children.empty()) {
        for (auto [key, node] : children)
          node.write(out, src, idx);
      } else {
        out << "using s" << idx++ << " = " << src << leaf << ";\n";
      }
    }
  } root;
  static void do_format_param(std::ostream &out, push p) noexcept {
    out << p.name << "<";
  }
  static void do_format_param(std::ostream &out, pop p) noexcept { out << ">"; }
  static void do_format_param(std::ostream &out, const char *arg) noexcept {
    out << arg;
  }
  template <typename T>
  static void do_format_param(std::ostream &out, cast<T> arg) noexcept {
    out << arg.type << '(';
    hsh::value_formatter::format(out, arg.val);
    out << ')';
  }
  template <typename T, EnableIfNonControlArg<T> = 0>
  static void do_format_param(std::ostream &out, T arg) noexcept {
    hsh::value_formatter::format(out, arg);
  }
  static void format_param_next(std::ostream &out) noexcept {}
  template <typename T>
  static void format_param_next(std::ostream &out, T arg) noexcept {
    out << ", ";
    do_format_param(out, arg);
  }
  template <typename T, typename... Args>
  static void format_params(std::ostream &out, T arg, Args... rest) noexcept {
    do_format_param(out, arg);
    (format_param_next(out, rest), ...);
  }
  void write_header(std::ostream &out) const noexcept {
    unsigned idx = 0;
    root.write(out, source, idx);
  }

public:
  template <typename... Args> void add(Args... args) noexcept {
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
  profiler &get(const char *filename, const char *fwds, const char *binding,
                const char *source) noexcept {
    auto &file = files[filename];
    file.fwds = fwds;
    auto &ret = file.profilers[binding];
    ret.source = source;
    return ret;
  }
  void write_headers() noexcept {
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

inline profile_context profile_context::instance{};
#endif
} // namespace hsh
