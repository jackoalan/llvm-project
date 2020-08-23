#pragma once

#if HSH_ENABLE_VULKAN

#ifndef NDEBUG
#define HSH_ASSERT_VK_SUCCESS(...) assert((__VA_ARGS__) == vk::Result::eSuccess)
#else
#define HSH_ASSERT_VK_SUCCESS(...) (void)(__VA_ARGS__)
#endif

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
template <>
struct ObjectDestroy<VmaAllocator, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> {
public:
  ObjectDestroy() noexcept = default;
  inline ObjectDestroy(VmaAllocator allocator,
                       Optional<const AllocationCallbacks> allocationCallbacks,
                       VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch)
      VULKAN_HPP_NOEXCEPT;

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
  vk::Buffer GetBuffer() const noexcept { return Buffer; }
  operator vk::Buffer() const noexcept { return GetBuffer(); }
  bool IsValid() const noexcept { return Buffer.operator bool(); }
};

class UploadBufferAllocation : public BufferAllocation {
  friend UploadBufferAllocation
  AllocateUploadBuffer(const SourceLocation &location,
                       vk::DeviceSize size) noexcept;
  void *MappedData;
  UploadBufferAllocation(vk::Buffer BufferIn, VmaAllocation AllocationIn,
                         void *MappedData) noexcept
      : BufferAllocation(BufferIn, AllocationIn), MappedData(MappedData) {}

public:
  UploadBufferAllocation() noexcept = default;
  void *GetMappedData() const noexcept { return MappedData; }
};

class DynamicBufferAllocation : public BufferAllocation {
  UploadBufferAllocation UploadBuffer;
  vk::DeviceSize Size;
  friend DynamicBufferAllocation
  AllocateDynamicBuffer(const SourceLocation &location, vk::DeviceSize size,
                        vk::BufferUsageFlags usage) noexcept;
  DynamicBufferAllocation(vk::Buffer BufferIn, VmaAllocation AllocationIn,
                          vk::DeviceSize Size,
                          UploadBufferAllocation UploadBuffer) noexcept
      : BufferAllocation(BufferIn, AllocationIn),
        UploadBuffer(std::move(UploadBuffer)), Size(Size) {}

public:
  DynamicBufferAllocation() noexcept = default;
  void *Map() noexcept { return UploadBuffer.GetMappedData(); }
  inline void Unmap() noexcept;
};

class FifoBufferAllocation : public BufferAllocation {
  FifoBufferAllocation *Prev = nullptr, *Next = nullptr;
  void *MappedData;
#ifndef NDEBUG
  uint32_t Size;
#endif
  uint32_t Offset = 0;
  uint32_t StartOffset = 0;
  friend std::unique_ptr<FifoBufferAllocation>
  AllocateFifoBuffer(const SourceLocation &location, vk::DeviceSize size,
                     vk::BufferUsageFlags usage) noexcept;
  inline FifoBufferAllocation(vk::Buffer BufferIn, VmaAllocation AllocationIn,
                              vk::DeviceSize Size, void *MappedData) noexcept;

public:
  inline ~FifoBufferAllocation() noexcept;
  template <typename T, typename Func>
  inline uint32_t MapUniform(Func func) noexcept;
  template <typename T, typename Func>
  inline uint32_t MapVertexIndex(std::size_t count, Func func) noexcept;
  inline void PostRender() noexcept;
  FifoBufferAllocation *GetNext() const noexcept { return Next; }
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

struct RenderPassBeginInfo : vk::RenderPassBeginInfo {
  std::array<vk::ClearValue, 2> ClearValues;
  RenderPassBeginInfo() = default;
  RenderPassBeginInfo(vk::RenderPass RenderPass, vk::Framebuffer Framebuffer,
                      vk::Extent2D Extent) noexcept
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
      : vk::RenderPassBeginInfo(RenderPass, Framebuffer, vk::Rect2D({}, Extent),
                                ClearValues.size(), ClearValues.data()),
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
        ClearValues{vk::ClearColorValue(), vk::ClearDepthStencilValue()} {
  }
};

class SurfaceAllocation {
  friend class DeletedSurfaceAllocation;
  friend class RenderTextureAllocation;
  friend class DeletedSurfaceSwapchainImage;
  friend class DeletedResources;
  SurfaceAllocation *Prev = nullptr, *Next = nullptr;
  SourceLocation Location;
  vk::UniqueSurfaceKHR Surface;
  vk::UniqueSwapchainKHR Swapchain;
  vk::UniqueRenderPass OwnedRenderPass, OwnedDirectRenderPass;
  struct SwapchainImage {
    vk::Image Image;
    vk::UniqueImageView ColorView;
    vk::UniqueFramebuffer Framebuffer;
    RenderPassBeginInfo RenderPassBegin;
    SwapchainImage() = default;
    SwapchainImage(const SwapchainImage &other) = delete;
    SwapchainImage &operator=(const SwapchainImage &other) = delete;
    SwapchainImage(SwapchainImage &&other) noexcept = default;
    SwapchainImage &operator=(SwapchainImage &&other) noexcept = default;
    inline ~SwapchainImage() noexcept;
  };
  std::vector<SwapchainImage> SwapchainImages;
  vk::Extent2D Extent;
  int32_t MarginL = 0, MarginR = 0, MarginT = 0, MarginB = 0;
  vk::SurfaceFormatKHR SurfaceFormat;
  uint32_t NextImage = UINT32_MAX;
  std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
      ResizeLambda;
  std::function<void()> DecorationLambda;
  std::function<void()> DeleterLambda;
  vk::Extent2D RequestExtent;
  bool RequestExtentPending = true;

  inline vk::Viewport ProcessMargins(vk::Viewport vp) noexcept;
  inline vk::Rect2D ProcessMargins(vk::Rect2D s) noexcept;

public:
  inline ~SurfaceAllocation() noexcept;
  inline explicit SurfaceAllocation(
      const SourceLocation &location, vk::UniqueSurfaceKHR &&Surface,
      std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
          &&ResizeLambda,
      std::function<void()> &&DeleterLambda, const hsh::extent2d &RequestExtent,
      int32_t L, int32_t R, int32_t T, int32_t B) noexcept;
  SurfaceAllocation(const SurfaceAllocation &other) = delete;
  SurfaceAllocation &operator=(const SurfaceAllocation &other) = delete;
  SurfaceAllocation(SurfaceAllocation &&other) noexcept = delete;
  SurfaceAllocation &operator=(SurfaceAllocation &&other) noexcept = delete;
  inline bool PreRender() noexcept;
  inline bool AcquireNextImage() noexcept;
  inline void AttachResizeLambda(
      std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
          &&Resize) noexcept;
  inline void AttachDecorationLambda(std::function<void()> &&Dec) noexcept;
  inline void AttachDeleterLambda(std::function<void()> &&Del) noexcept;
  inline void SetRequestExtent(const hsh::extent2d &Ext) noexcept;
  inline void PostRender() noexcept;
  inline vk::Extent2D ContentExtent() const noexcept;
  inline void DrawDecorations() noexcept;
  void SetMargins(int32_t L, int32_t R, int32_t T, int32_t B) noexcept {
    MarginL = L;
    MarginR = R;
    MarginT = T;
    MarginB = B;
  }
  const vk::Extent2D &GetExtent() const noexcept { return Extent; }
  vk::Format GetColorFormat() const noexcept { return SurfaceFormat.format; }
  SurfaceAllocation *GetNext() const noexcept { return Next; }
};

class RenderTextureAllocation {
  friend class DeletedRenderTextureAllocation;
  RenderTextureAllocation *Prev = nullptr, *Next = nullptr;
  SourceLocation Location;
  SurfaceAllocation *Surface = nullptr;
  vk::Extent2D Extent;
  vk::Format ColorFormat = vk::Format::eUndefined;
  uint32_t NumColorBindings, NumDepthBindings;
  TextureAllocation ColorTexture;
  vk::UniqueImageView ColorView;
  TextureAllocation DepthTexture;
  vk::UniqueImageView DepthView;
  vk::UniqueFramebuffer Framebuffer;
  RenderPassBeginInfo RenderPassBegin;
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
                              vk::Offset3D SrcOffset, vk::Offset3D DstOffset,
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
                                          extent2d extent,
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
      auto SurfContentExtent = SurfAlloc.ContentExtent();
      if (SurfContentExtent != Extent ||
          SurfAlloc.GetColorFormat() != ColorFormat) {
        Extent = SurfContentExtent;
        ColorFormat = SurfAlloc.GetColorFormat();
        Prepare();
      }
    }
  }
  inline void BeginRenderPass() noexcept;
  inline void ResolveSurface(SurfaceAllocation *Surface,
                             bool Reattach) noexcept;
  inline void ResolveColorBinding(uint32_t Idx, rect2d Region,
                                  bool Reattach) noexcept;
  inline void ResolveDepthBinding(uint32_t Idx, rect2d Region,
                                  bool Reattach) noexcept;
  vk::ImageView GetColorBindingView(uint32_t Idx) const noexcept {
    assert(Idx < NumColorBindings);
    return ColorBindings[Idx].ImageView.get();
  }
  vk::ImageView GetDepthBindingView(uint32_t Idx) const noexcept {
    assert(Idx < NumDepthBindings);
    return DepthBindings[Idx].ImageView.get();
  }
  template <typename... Args> inline void Attach(const Args &... args) noexcept;
  inline void ProcessAttachArgs() noexcept;
  inline void ProcessAttachArgs(const viewport &vp) noexcept;
  inline void ProcessAttachArgs(const scissor &s) noexcept;
  inline void ProcessAttachArgs(const viewport &vp, const scissor &s) noexcept;
  RenderTextureAllocation *GetNext() const noexcept { return Next; }
  extent2d GetExtent() const noexcept { return Extent; }
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

class DeletedSurfaceAllocation {
  vk::UniqueSurfaceKHR Surface;
  vk::UniqueSwapchainKHR Swapchain;
  vk::UniqueRenderPass OwnedRenderPass;
  std::function<void()> DeleterLambda;

public:
  explicit DeletedSurfaceAllocation(SurfaceAllocation &&Obj) noexcept
      : Surface(std::move(Obj.Surface)), Swapchain(std::move(Obj.Swapchain)),
        OwnedRenderPass(std::move(Obj.OwnedRenderPass)),
        DeleterLambda(std::move(Obj.DeleterLambda)) {}

  ~DeletedSurfaceAllocation() noexcept {
    OwnedRenderPass.reset();
    Swapchain.reset();
    Surface.reset();
    if (DeleterLambda)
      DeleterLambda();
  }

  DeletedSurfaceAllocation(const DeletedSurfaceAllocation &other) = delete;
  DeletedSurfaceAllocation &
  operator=(const DeletedSurfaceAllocation &other) = delete;
  DeletedSurfaceAllocation(DeletedSurfaceAllocation &&other) noexcept = default;
  DeletedSurfaceAllocation &
  operator=(DeletedSurfaceAllocation &&other) noexcept = default;
};

class DeletedSurfaceSwapchainImage {
  vk::UniqueImageView ColorView;
  vk::UniqueFramebuffer Framebuffer;

public:
  explicit DeletedSurfaceSwapchainImage(
      SurfaceAllocation::SwapchainImage &&Obj) noexcept
      : ColorView(std::move(Obj.ColorView)),
        Framebuffer(std::move(Obj.Framebuffer)) {}

  DeletedSurfaceSwapchainImage(const DeletedSurfaceSwapchainImage &other) =
      delete;
  DeletedSurfaceSwapchainImage &
  operator=(const DeletedSurfaceSwapchainImage &other) = delete;
  DeletedSurfaceSwapchainImage(DeletedSurfaceSwapchainImage &&other) noexcept =
      default;
  DeletedSurfaceSwapchainImage &
  operator=(DeletedSurfaceSwapchainImage &&other) noexcept = default;
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
  std::vector<DeletedSurfaceAllocation> Surfaces;
  std::vector<DeletedSurfaceSwapchainImage> SwapchainImages;
  std::vector<DeletedRenderTextureAllocation> RenderTextures;

public:
  void DeleteLater(BufferAllocation &&Obj) noexcept {
    Buffers.emplace_back(std::move(Obj));
  }
  void DeleteLater(TextureAllocation &&Obj) noexcept {
    Textures.emplace_back(std::move(Obj));
  }
  void DeleteLater(SurfaceAllocation &&Obj) noexcept {
    Surfaces.emplace_back(std::move(Obj));
  }
  void DeleteLater(SurfaceAllocation::SwapchainImage &&Obj) noexcept {
    SwapchainImages.emplace_back(std::move(Obj));
  }
  void DeleteLater(RenderTextureAllocation &&Obj) noexcept {
    RenderTextures.emplace_back(std::move(Obj));
  }
  void Purge() noexcept {
    Buffers.clear();
    Textures.clear();
    Surfaces.clear();
    SwapchainImages.clear();
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
  vk::VmaPool UploadPool;
  uint8_t PipelineCacheUUID[VK_UUID_SIZE];
  vk::PipelineCache PipelineCache;
  std::array<vk::DescriptorSetLayout, 64> DescriptorSetLayout;
  vk::PipelineLayout PipelineLayout;
  struct DescriptorPoolChain *DescriptorPoolChain = nullptr;
  vk::Semaphore ImageAcquireSem;
  vk::Semaphore RenderCompleteSem;
  uint32_t QueueFamilyIdx = 0;
  vk::Queue Queue;
  vk::PipelineMultisampleStateCreateInfo MultisampleState{
      {}, vk::SampleCountFlagBits::e1};
  vk::RenderPass RenderPass, DirectRenderPass;
  float Anisotropy = 0.f;
  std::array<vk::CommandBuffer, 2> CommandBuffers;
  std::array<vk::Fence, 2> CommandFences;
  vk::CommandBuffer Cmd;
  vk::Fence CmdFence;
  vk::CommandBuffer CopyCmd;
  vk::Fence CopyFence;
  vk::DeviceSize UniformOffsetAlignment = 0;
  vk::Pipeline BoundPipeline;
  vk::DescriptorSet BoundDescriptorSet;
  RenderTextureAllocation *AttachedRenderTexture = nullptr;
  uint64_t Frame = 0;
  bool AcquiredImage = false;

  std::array<DeletedResources, 2> *DeletedResourcesArr;
  DeletedResources *DeletedResources = nullptr;
  SurfaceAllocation *SurfaceHead = nullptr;
  RenderTextureAllocation *RenderTextureHead = nullptr;
  FifoBufferAllocation *FifoBufferHead = nullptr;

  void PreRender() noexcept {
    uint32_t CurBufferIdx = Frame & 1u;
    Cmd = CommandBuffers[CurBufferIdx];
    CmdFence = CommandFences[CurBufferIdx];
    Device.waitForFences(CmdFence, VK_TRUE, 500000000);
    DeletedResources = &(*DeletedResourcesArr)[CurBufferIdx];
    DeletedResources->Purge();

    for (auto *Surf = SurfaceHead; Surf; Surf = Surf->GetNext())
      Surf->PreRender();
    for (auto *RT = RenderTextureHead; RT; RT = RT->GetNext())
      RT->PreRender();

    CopyCmd.begin(vk::CommandBufferBeginInfo(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
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
    CopyCmd.end();
    Cmd.end();

    for (auto *Fifo = FifoBufferHead; Fifo; Fifo = Fifo->GetNext())
      Fifo->PostRender();

    vk::PipelineStageFlags pipeStageFlags =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    Device.resetFences({CopyFence, CmdFence});
    Queue.submit(vk::SubmitInfo(0, {}, &pipeStageFlags, 1, &CopyCmd),
                 CopyFence);
    Queue.submit(vk::SubmitInfo(AcquiredImage ? 1 : 0, &ImageAcquireSem,
                                &pipeStageFlags, 1, &Cmd, AcquiredImage ? 1 : 0,
                                &RenderCompleteSem),
                 CmdFence);
    for (auto *Surf = SurfaceHead; Surf; Surf = Surf->GetNext())
      Surf->PostRender();
    AcquiredImage = false;
    Device.waitForFences(CopyFence, VK_TRUE, 500000000);
    ++Frame;
  }

  void SetDescriptorSetLayout(vk::DescriptorSetLayout Layout) noexcept {
    std::fill(DescriptorSetLayout.begin(), DescriptorSetLayout.end(), Layout);
  }

  vk::RenderPass GetRenderPass() const noexcept {
    assert(RenderPass && "No surfaces created yet");
    return RenderPass;
  }

  vk::RenderPass GetDirectRenderPass() const noexcept {
    assert(DirectRenderPass && "No surfaces created yet");
    return DirectRenderPass;
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

template <typename Mgr>
inline vk::UniquePipelineCache CreatePipelineCache(Mgr &M) noexcept {
  vk::UniquePipelineCache Ret;
  M.ReadPipelineCache(
      [&](const uint8_t *Data, std::size_t Size) {
        Ret = Globals.Device
                  .createPipelineCacheUnique(
                      vk::PipelineCacheCreateInfo({}, Size, Data))
                  .value;
      },
      Globals.PipelineCacheUUID);
  if (!Ret)
    Ret = Globals.Device.createPipelineCacheUnique({}).value;
  return Ret;
}

template <typename Mgr> inline void WritePipelineCache(Mgr &M) noexcept {
  M.WritePipelineCache(
      [&](auto F) {
        auto Data =
            Globals.Device.getPipelineCacheData(Globals.PipelineCache).value;
        F(Data.data(), Data.size());
      },
      Globals.PipelineCacheUUID);
}

inline vk::Viewport
SurfaceAllocation::ProcessMargins(vk::Viewport vp) noexcept {
  vp.x += MarginL;
  vp.y += MarginT;
  vp.width -= MarginL + MarginR;
  vp.height -= MarginT + MarginB;
  return vp;
}

inline vk::Rect2D SurfaceAllocation::ProcessMargins(vk::Rect2D s) noexcept {
  s.offset.x += MarginL;
  s.offset.y += MarginT;
  s.extent.width -= MarginL + MarginR;
  s.extent.height -= MarginT + MarginB;
  return s;
}

bool SurfaceAllocation::PreRender() noexcept {
  auto Capabilities =
      Globals.PhysDevice.getSurfaceCapabilitiesKHR(Surface.get()).value;
  // On platforms like Wayland, the swapchain dimensions dictate the window
  // surface dimensions.
  if (Capabilities.currentExtent.width == UINT32_MAX)
    Capabilities.currentExtent = RequestExtent;
  if (!Swapchain || Capabilities.currentExtent != Extent ||
      RequestExtentPending) {
    RequestExtentPending = false;

    Extent = Capabilities.currentExtent;

    struct SwapchainCreateInfo : vk::SwapchainCreateInfoKHR {
      explicit SwapchainCreateInfo(const vk::SurfaceCapabilitiesKHR &SC,
                                   vk::SurfaceKHR Surface,
                                   const vk::SurfaceFormatKHR &SurfaceFormat,
                                   vk::SwapchainKHR OldSwapchain) noexcept
          : vk::SwapchainCreateInfoKHR({}, Surface) {
        setMinImageCount(std::max(2u, SC.minImageCount));
        setImageFormat(SurfaceFormat.format);
        setImageColorSpace(SurfaceFormat.colorSpace);
        setImageExtent(SC.currentExtent);
        setImageArrayLayers(1);
        constexpr auto WantedUsage = vk::ImageUsageFlagBits::eTransferDst |
                                     vk::ImageUsageFlagBits::eColorAttachment;
        assert((SC.supportedUsageFlags & WantedUsage) == WantedUsage);
        setImageUsage(WantedUsage);
        setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity);
        if (SC.supportedCompositeAlpha &
            vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
          setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::ePreMultiplied);
        else if (SC.supportedCompositeAlpha &
                 vk::CompositeAlphaFlagBitsKHR::eInherit)
          setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eInherit);
        else
          setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
        setPresentMode(vk::PresentModeKHR::eFifo);
        setOldSwapchain(OldSwapchain);
      }
    };
    Swapchain =
        Globals.Device
            .createSwapchainKHRUnique(SwapchainCreateInfo(
                Capabilities, Surface.get(), SurfaceFormat, Swapchain.get()))
            .value;
    Globals.SetDebugObjectName(Location.with_field("Swapchain"),
                               Swapchain.get());
    auto Images = Globals.Device.getSwapchainImagesKHR(*Swapchain).value;
    SwapchainImages.clear();
    SwapchainImages.reserve(Images.size());
    uint32_t Idx = 0;
    for (auto &Image : Images) {
      auto &Out = SwapchainImages.emplace_back();
      Out.Image = Image;
      Out.ColorView = Globals.Device
                          .createImageViewUnique(vk::ImageViewCreateInfo(
                              {}, Out.Image, vk::ImageViewType::e2D,
                              SurfaceFormat.format, {},
                              vk::ImageSubresourceRange(
                                  vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)))
                          .value;
      Globals.SetDebugObjectName(Location.with_field("SCColorView", Idx),
                                 Out.ColorView.get());
      vk::ImageView Views[] = {Out.ColorView.get()};
      auto RenderPass = Globals.GetDirectRenderPass();
      Out.Framebuffer =
          Globals.Device
              .createFramebufferUnique(vk::FramebufferCreateInfo(
                  {}, RenderPass, 1, Views, Extent.width, Extent.height, 1))
              .value;
      Globals.SetDebugObjectName(Location.with_field("SCFramebuffer", Idx),
                                 Out.Framebuffer.get());
      Out.RenderPassBegin =
          RenderPassBeginInfo(RenderPass, Out.Framebuffer.get(), Extent);
      ++Idx;
    }
    if (ResizeLambda)
      ResizeLambda(Extent, ContentExtent());
    return true;
  }
  return false;
}

bool SurfaceAllocation::AcquireNextImage() noexcept {
  auto ret = Globals.Device.acquireNextImageKHR(
      Swapchain.get(), UINT64_MAX, Globals.ImageAcquireSem, {}, &NextImage);
  if (ret == vk::Result::eSuccess || ret == vk::Result::eSuboptimalKHR) {
    Globals.AcquiredImage = true;
    return true;
  }
  return false;
}

void SurfaceAllocation::AttachResizeLambda(
    std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
        &&Resize) noexcept {
  ResizeLambda = std::move(Resize);
}

void SurfaceAllocation::AttachDecorationLambda(
    std::function<void()> &&Dec) noexcept {
  DecorationLambda = std::move(Dec);
}

void SurfaceAllocation::AttachDeleterLambda(
    std::function<void()> &&Del) noexcept {
  DeleterLambda = std::move(Del);
}

void SurfaceAllocation::SetRequestExtent(const hsh::extent2d &Ext) noexcept {
  RequestExtent = Ext;
  RequestExtentPending = true;
}

void SurfaceAllocation::PostRender() noexcept {
  if (NextImage != UINT32_MAX) {
    if (Globals.AcquiredImage) {
      // Present called using non-enhanced API to avoid error asserts that are
      // recoverable
      vk::PresentInfoKHR Info(1, &Globals.RenderCompleteSem, 1,
                              &Swapchain.get(), &NextImage);
      Globals.Queue.presentKHR(&Info);
    }
    NextImage = UINT32_MAX;
  }
}

vk::Extent2D SurfaceAllocation::ContentExtent() const noexcept {
  return vk::Extent2D{
      uint32_t(std::max(1, int32_t(Extent.width) - MarginL - MarginR)),
      uint32_t(std::max(1, int32_t(Extent.height) - MarginT - MarginB))};
}

void SurfaceAllocation::DrawDecorations() noexcept {
  auto &DstImage = SwapchainImages[NextImage];
  if (DecorationLambda) {
    Globals.Cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::DependencyFlagBits::eByRegion, {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eTransferWrite,
            vk::AccessFlagBits::eColorAttachmentRead |
                vk::AccessFlagBits::eColorAttachmentWrite,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eColorAttachmentOptimal, VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED, DstImage.Image,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                      VK_REMAINING_MIP_LEVELS, 0,
                                      VK_REMAINING_ARRAY_LAYERS)));
    Globals.Cmd.beginRenderPass(DstImage.RenderPassBegin,
                                vk::SubpassContents::eInline);
    Globals.Cmd.setViewport(
        0, vk::Viewport(0.f, 0.f, Extent.width, Extent.height, 0.f, 1.f));
    Globals.Cmd.setScissor(0, vk::Rect2D({}, {Extent.width, Extent.height}));
    DecorationLambda();
    Globals.Cmd.endRenderPass();
    Globals.Cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eColorAttachmentRead |
                vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR, VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED, DstImage.Image,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                      VK_REMAINING_MIP_LEVELS, 0,
                                      VK_REMAINING_ARRAY_LAYERS)));
  } else {
    Globals.Cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::ePresentSrcKHR, VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED, DstImage.Image,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                      VK_REMAINING_MIP_LEVELS, 0,
                                      VK_REMAINING_ARRAY_LAYERS)));
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
  Globals.DeletedResources->DeleteLater(std::move(*this));
}

SurfaceAllocation::SwapchainImage::~SwapchainImage() noexcept {
  Globals.DeletedResources->DeleteLater(std::move(*this));
}

SurfaceAllocation::SurfaceAllocation(
    const SourceLocation &location, vk::UniqueSurfaceKHR &&Surface,
    std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
        &&ResizeLambda,
    std::function<void()> &&DeleterLambda, const hsh::extent2d &RequestExtent,
    int32_t L, int32_t R, int32_t T, int32_t B) noexcept
    : Next(Globals.SurfaceHead), Location(location),
      Surface(std::move(Surface)), MarginL(L), MarginR(R), MarginT(T),
      MarginB(B), ResizeLambda(std::move(ResizeLambda)),
      DeleterLambda(std::move(DeleterLambda)), RequestExtent(RequestExtent) {
  Globals.SurfaceHead = this;
  if (Next)
    Next->Prev = this;

  vk::Format UseFormat = vk::Format::eB8G8R8A8Unorm;
  for (auto &Format :
       Globals.PhysDevice.getSurfaceFormatsKHR(*this->Surface).value) {
    if (Format.format == UseFormat) {
      SurfaceFormat = Format;
      break;
    }
  }
  assert(SurfaceFormat.format != vk::Format::eUndefined);
  assert((!Next || SurfaceFormat.format == Next->SurfaceFormat.format) &&
         "Subsequent surfaces must have the same color format");

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
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
          : vk::RenderPassCreateInfo({}, Attachments.size(), Attachments.data(),
                                     Subpasses.size(), Subpasses.data()),
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
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

    struct DirectRenderPassCreateInfo : vk::RenderPassCreateInfo {
      std::array<vk::AttachmentDescription, 1> Attachments;
      std::array<vk::SubpassDescription, 1> Subpasses;
      vk::AttachmentReference ColorRef{
          0, vk::ImageLayout::eColorAttachmentOptimal};
      constexpr DirectRenderPassCreateInfo(vk::Format colorFormat) noexcept
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
          : vk::RenderPassCreateInfo({}, Attachments.size(), Attachments.data(),
                                     Subpasses.size(), Subpasses.data()),
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
            Attachments{vk::AttachmentDescription(
                {}, colorFormat, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eColorAttachmentOptimal)},
            Subpasses{vk::SubpassDescription(
                {}, vk::PipelineBindPoint::eGraphics, {}, {}, 1, &ColorRef)} {
      }
    };
    OwnedDirectRenderPass =
        Globals.Device
            .createRenderPassUnique(
                DirectRenderPassCreateInfo(GetColorFormat()))
            .value;
    Globals.SetDebugObjectName(location.with_field("OwnedDirectRenderPass"),
                               OwnedDirectRenderPass.get());
    Globals.DirectRenderPass = OwnedDirectRenderPass.get();
  }

  PreRender();
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
    const SourceLocation &location, extent2d extent, vk::Format colorFormat,
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

void DynamicBufferAllocation::Unmap() noexcept {
  Globals.CopyCmd.copyBuffer(UploadBuffer.GetBuffer(), GetBuffer(),
                             vk::BufferCopy{0, 0, Size});
}

FifoBufferAllocation::FifoBufferAllocation(vk::Buffer BufferIn,
                                           VmaAllocation AllocationIn,
                                           vk::DeviceSize Size,
                                           void *MappedData) noexcept
    : BufferAllocation(BufferIn, AllocationIn), Next(Globals.FifoBufferHead),
      MappedData(MappedData)
#ifndef NDEBUG
      ,
      Size(Size)
#endif
{
  Globals.FifoBufferHead = this;
  if (Next)
    Next->Prev = this;
}

FifoBufferAllocation::~FifoBufferAllocation() noexcept {
  if (Prev)
    Prev->Next = Next;
  else
    Globals.FifoBufferHead = Next;
  if (Next)
    Next->Prev = Prev;
}

template <typename T, typename Func>
uint32_t FifoBufferAllocation::MapUniform(Func func) noexcept {
  Offset = AlignUp(Offset, std::max(uint32_t(alignof(T)),
                                    uint32_t(Globals.UniformOffsetAlignment)));
  uint32_t MapOffset = Offset;
  assert(MapOffset + sizeof(T) <= Size && "Fifo buffer must be larger");
  func(*reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(MappedData) +
                              MapOffset));
  Offset += sizeof(T);
  return MapOffset;
}

template <typename T, typename Func>
uint32_t FifoBufferAllocation::MapVertexIndex(std::size_t count,
                                              Func func) noexcept {
  Offset = AlignUp(Offset, uint32_t(alignof(T)));
  uint32_t MapOffset = Offset;
  assert(MapOffset + sizeof(T) * count <= Size && "Fifo buffer must be larger");
  func(reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(MappedData) +
                             MapOffset));
  Offset += sizeof(T) * count;
  return MapOffset;
}

void FifoBufferAllocation::PostRender() noexcept {
  vmaFlushAllocation(Globals.Allocator, Allocation, StartOffset,
                     Offset - StartOffset);
  if (StartOffset) {
    Offset = 0;
    StartOffset = 0;
  } else {
    StartOffset = Offset;
  }
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

ObjectDestroy<VmaAllocator, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>::ObjectDestroy(
    VmaAllocator owner, Optional<const AllocationCallbacks> allocationCallbacks,
    VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT {
  assert(owner == ::hsh::detail::vulkan::Globals.Allocator);
}

template <typename T>
void ObjectDestroy<VmaAllocator, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>::destroy(
    T t) VULKAN_HPP_NOEXCEPT {
  ::hsh::detail::vulkan::Globals.Allocator.destroy(
      t, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
}
} // namespace VULKAN_HPP_NAMESPACE

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
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
      : vk::DescriptorPoolCreateInfo({}, MaxDescriptorPoolSets,
                                     PoolSizes.size(), PoolSizes.data()) {
  }
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
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
  operator bool() const noexcept { return Set.operator bool(); }
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
      UniqueDescriptorSet Allocate(struct DescriptorPool &pool, uint64_t &bmp,
                                   std::size_t Index) noexcept {
        assert(bmp != UINT64_MAX && "descriptor bucket full");
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
          HSH_ASSERT_VK_SUCCESS(Result);
        }
        for (uint64_t i = 0; i < 64; ++i) {
          if ((bmp & (1ull << i)) == 0) {
            bmp |= (1ull << i);
            return UniqueDescriptorSet(DescriptorSets[i], Index + i);
          }
        }
        return {};
      }
    };
    std::array<DescriptorBucket, MaxDescriptorPoolSets / 64> Buckets;
    UniqueDescriptorSet Allocate(std::size_t Index) noexcept {
      assert(AllocatedSets < MaxDescriptorPoolSets && "descriptor pool full");
      auto BucketsIt = Buckets.begin();
      for (uint64_t &bmp : Bitmap) {
        if (bmp != UINT64_MAX) {
          ++AllocatedSets;
          return BucketsIt->Allocate(*this, bmp, Index);
        }
        Index += 64;
        ++BucketsIt;
      }
      return {};
    }
    void Free(std::size_t Index) noexcept {
      auto BucketIdx = Index / 64;
      auto BucketRem = Index % 64;
      assert(AllocatedSets && "freed too many descriptor sets from pool");
      assert(Bitmap[BucketIdx] & (1ull << BucketRem) && "double free");
      Bitmap[BucketIdx] &= ~(1ull << BucketRem);
      --AllocatedSets;
    }
  };
  std::list<DescriptorPool> Chain;
  UniqueDescriptorSet Allocate() noexcept {
    std::size_t Index = 0;
    for (auto &pool : Chain) {
      if (pool.AllocatedSets != MaxDescriptorPoolSets)
        return pool.Allocate(Index);
      Index += MaxDescriptorPoolSets;
    }
    return Chain.emplace_back().Allocate(Index);
  }
  void Free(std::size_t Index) noexcept {
    auto PoolIdx = Index / MaxDescriptorPoolSets;
    auto PoolRem = Index % MaxDescriptorPoolSets;
    auto PoolIt = Chain.begin();
    std::advance(PoolIt, PoolIdx);
    PoolIt->Free(PoolRem);
  }
};

UniqueDescriptorSet::~UniqueDescriptorSet() noexcept {
  if (Index != UINT64_MAX)
    Globals.DescriptorPoolChain->Free(Index);
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

inline VkResult vmaFindMemoryTypeIndexForBufferInfo(
    const VkBufferCreateInfo &pBufferCreateInfo,
    const VmaAllocationCreateInfo &pAllocationCreateInfo,
    uint32_t *pMemoryTypeIndex) noexcept {
  return ::vmaFindMemoryTypeIndexForBufferInfo(
      Globals.Allocator,
      reinterpret_cast<const VkBufferCreateInfo *>(&pBufferCreateInfo),
      reinterpret_cast<const VmaAllocationCreateInfo *>(&pAllocationCreateInfo),
      pMemoryTypeIndex);
}

#if HSH_SOURCE_LOCATION_ENABLED
class VmaLocationStrSetter {
  std::string LocationStr;

  VmaLocationStrSetter(VmaAllocationCreateInfo &CreateInfo,
                       std::string str) noexcept
      : LocationStr(std::move(str)) {
    CreateInfo.flags |= VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    CreateInfo.pUserData = (void *)LocationStr.c_str();
  }

public:
  VmaLocationStrSetter(VmaAllocationCreateInfo &CreateInfo,
                       const SourceLocation &Location) noexcept
      : VmaLocationStrSetter(CreateInfo, Location.to_string()) {}
  VmaLocationStrSetter(VmaAllocationCreateInfo &CreateInfo,
                       const SourceLocation &Location,
                       const char *Suffix) noexcept
      : VmaLocationStrSetter(CreateInfo, Location.to_string() + Suffix) {}
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
  VmaLocationStrSetter(VmaAllocationCreateInfo &CreateInfo,
                       const SourceLocation &Location,
                       const char *Suffix) noexcept {}
  operator const char *() const noexcept { return nullptr; }
  VmaLocationStrSetter(const VmaLocationStrSetter &) = delete;
  VmaLocationStrSetter &operator=(const VmaLocationStrSetter &) = delete;
  VmaLocationStrSetter(VmaLocationStrSetter &&) = delete;
  VmaLocationStrSetter &operator=(VmaLocationStrSetter &&) = delete;
};
#endif

struct UploadBufferAllocationCreateInfo : VmaAllocationCreateInfo {
  constexpr UploadBufferAllocationCreateInfo(VmaPool PoolIn = {}) noexcept
      : VmaAllocationCreateInfo{VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                VMA_MEMORY_USAGE_CPU_ONLY,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                0,
                                0,
                                PoolIn,
                                nullptr} {}
};

inline vk::UniqueVmaPool CreateUploadPool() noexcept {
  struct UploadPoolCreateInfo : VmaPoolCreateInfo {
    constexpr UploadPoolCreateInfo() noexcept
        : VmaPoolCreateInfo{0, 0, 64ull * 1024ull * 1024ull, 0, 0, 0} {}
  };
  UploadPoolCreateInfo CreateInfo;
  auto Result = vmaFindMemoryTypeIndexForBufferInfo(
      vk::BufferCreateInfo({}, 32ull * 1024 * 1024,
                           vk::BufferUsageFlagBits::eTransferSrc),
      UploadBufferAllocationCreateInfo(), &CreateInfo.memoryTypeIndex);
  HSH_ASSERT_VK_SUCCESS(vk::Result(Result));
  return Globals.Allocator.createVmaPoolUnique(CreateInfo).value;
}

inline UploadBufferAllocation
AllocateUploadBuffer(const SourceLocation &location,
                     vk::DeviceSize size) noexcept {
  VkBuffer Buffer;
  VmaAllocation Allocation;
  VmaAllocationInfo AllocInfo;
  UploadBufferAllocationCreateInfo CreateInfo(Globals.UploadPool);
  VmaLocationStrSetter LocationStr(CreateInfo, location, "Upload");
  auto Result = vmaCreateBuffer(
      vk::BufferCreateInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc),
      CreateInfo, &Buffer, &Allocation, &AllocInfo);
  HSH_ASSERT_VK_SUCCESS(vk::Result(Result));
  Globals.SetDebugObjectName(LocationStr, vk::Buffer(Buffer));
  return UploadBufferAllocation(Buffer, Allocation, AllocInfo.pMappedData);
}

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
  HSH_ASSERT_VK_SUCCESS(vk::Result(Result));
  Globals.SetDebugObjectName(LocationStr, vk::Buffer(Buffer));
  return BufferAllocation(Buffer, Allocation);
}

inline DynamicBufferAllocation
AllocateDynamicBuffer(const SourceLocation &location, vk::DeviceSize size,
                      vk::BufferUsageFlags usage) noexcept {
  struct DynamicUniformBufferAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr DynamicUniformBufferAllocationCreateInfo() noexcept
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
  VmaAllocationInfo AllocInfo;
  DynamicUniformBufferAllocationCreateInfo CreateInfo;
  VmaLocationStrSetter LocationStr(CreateInfo, location);
  auto Result = vmaCreateBuffer(vk::BufferCreateInfo({}, size, usage),
                                CreateInfo, &Buffer, &Allocation, &AllocInfo);
  HSH_ASSERT_VK_SUCCESS(vk::Result(Result));
  Globals.SetDebugObjectName(LocationStr, vk::Buffer(Buffer));

  return DynamicBufferAllocation(Buffer, Allocation, size,
                                 AllocateUploadBuffer(location, size));
}

inline std::unique_ptr<FifoBufferAllocation>
AllocateFifoBuffer(const SourceLocation &location, vk::DeviceSize size,
                   vk::BufferUsageFlags usage) noexcept {
  struct FifoBufferAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr FifoBufferAllocationCreateInfo() noexcept
        : VmaAllocationCreateInfo{VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                  VMA_MEMORY_USAGE_CPU_TO_GPU,
                                  0,
                                  VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
                                  0,
                                  VK_NULL_HANDLE,
                                  nullptr} {}
  };

  VkBuffer Buffer;
  VmaAllocation Allocation;
  VmaAllocationInfo AllocInfo;
  FifoBufferAllocationCreateInfo CreateInfo;
  VmaLocationStrSetter LocationStr(CreateInfo, location);
  auto Result = vmaCreateBuffer(vk::BufferCreateInfo({}, size, usage),
                                CreateInfo, &Buffer, &Allocation, &AllocInfo);
  HSH_ASSERT_VK_SUCCESS(vk::Result(Result));
  Globals.SetDebugObjectName(LocationStr, vk::Buffer(Buffer));

  return std::unique_ptr<FifoBufferAllocation>{new FifoBufferAllocation(
      Buffer, Allocation, size, AllocInfo.pMappedData)};
}

inline TextureAllocation AllocateTexture(const SourceLocation &location,
                                         const vk::ImageCreateInfo &CreateInfo,
                                         bool Dedicated = false) noexcept {
  struct TextureAllocationCreateInfo : VmaAllocationCreateInfo {
    constexpr TextureAllocationCreateInfo(bool Dedicated) noexcept
        : VmaAllocationCreateInfo{
              Dedicated ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
                        : VmaAllocationCreateFlags(0),
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
  HSH_ASSERT_VK_SUCCESS(vk::Result(Result));
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
  ColorView = Globals.Device
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
  DepthView = Globals.Device
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
        Globals.Device
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
        Globals.Device
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
                                       vk::Offset3D SrcOffset,
                                       vk::Offset3D DstOffset,
                                       vk::Extent3D Extent) noexcept {
  Globals.Cmd.pipelineBarrier(
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
        vk::ImageResolve(vk::ImageSubresourceLayers(Aspect, 0, 0, 1), SrcOffset,
                         vk::ImageSubresourceLayers(Aspect, 0, 0, 1), DstOffset,
                         Extent));
  } else {
    Globals.Cmd.copyImage(
        SrcImage, vk::ImageLayout::eTransferSrcOptimal, DstImage,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageCopy(vk::ImageSubresourceLayers(Aspect, 0, 0, 1), SrcOffset,
                      vk::ImageSubresourceLayers(Aspect, 0, 0, 1), DstOffset,
                      Extent));
  }
  Globals.Cmd.pipelineBarrier(
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
                                      vk::Offset3D Offset,
                                      vk::Extent3D ExtentIn,
                                      bool Reattach) noexcept {
  bool DelimitRenderPass = this == Globals.AttachedRenderTexture;
  if (DelimitRenderPass)
    Globals.Cmd.endRenderPass();

  _Resolve(SrcImage, DstImage, Aspect, Offset, Offset, ExtentIn);

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
  assert(Surface->ContentExtent() == Extent &&
         "Mismatched render texture / surface extents");
  bool DelimitRenderPass = this == Globals.AttachedRenderTexture;
  if (DelimitRenderPass)
    Globals.Cmd.endRenderPass();
  auto &DstImage = Surface->SwapchainImages[Surface->NextImage];
  Globals.Cmd.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion,
      {}, {},
      vk::ImageMemoryBarrier(
          vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite,
          vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
          VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, DstImage.Image,
          vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                    VK_REMAINING_MIP_LEVELS, 0,
                                    VK_REMAINING_ARRAY_LAYERS)));
  vk::Offset3D DestOff{};
  if (int32_t(Surface->Extent.width) > Surface->MarginL + Surface->MarginR &&
      int32_t(Surface->Extent.height) > Surface->MarginT + Surface->MarginB)
    DestOff = vk::Offset3D(Surface->MarginL, Surface->MarginT);
  _Resolve(ColorTexture.GetImage(), DstImage.Image,
           vk::ImageAspectFlagBits::eColor, vk::Offset3D(), DestOff,
           vk::Extent3D(Extent, 1));
  Surface->DrawDecorations();
  if (DelimitRenderPass) {
    if (Reattach)
      BeginRenderPass();
    else
      Globals.AttachedRenderTexture = nullptr;
  }
}

void RenderTextureAllocation::ResolveColorBinding(uint32_t Idx, rect2d region,
                                                  bool Reattach) noexcept {
  assert(Idx < NumColorBindings);
  Resolve(ColorTexture.GetImage(), ColorBindings[Idx].Texture.GetImage(),
          vk::ImageAspectFlagBits::eColor, vk::Offset3D(region.offset),
          vk::Extent3D(region.extent, 1), Reattach);
}

void RenderTextureAllocation::ResolveDepthBinding(uint32_t Idx, rect2d region,
                                                  bool Reattach) noexcept {
  assert(Idx < NumDepthBindings);
  Resolve(DepthTexture.GetImage(), DepthBindings[Idx].Texture.GetImage(),
          vk::ImageAspectFlagBits::eDepth, vk::Offset3D(region.offset),
          vk::Extent3D(region.extent, 1), Reattach);
}

template <typename... Args>
void RenderTextureAllocation::Attach(const Args &... args) noexcept {
  if (Globals.AttachedRenderTexture == this)
    return;
  if (Globals.AttachedRenderTexture)
    Globals.Cmd.endRenderPass();
  Globals.AttachedRenderTexture = this;

  if (!FirstAttach) {
    FirstAttach = true;
    Globals.Cmd.pipelineBarrier(
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
    Globals.Cmd.pipelineBarrier(
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
  ProcessAttachArgs(args...);
}

void RenderTextureAllocation::ProcessAttachArgs() noexcept {
  Globals.Cmd.setViewport(
      0, vk::Viewport(0.f, 0.f, Extent.width, Extent.height, 0.f, 1.f));
  Globals.Cmd.setScissor(0, vk::Rect2D({}, {Extent.width, Extent.height}));
}

void RenderTextureAllocation::ProcessAttachArgs(const viewport &vp) noexcept {
  Globals.Cmd.setViewport(0, vk::Viewport(vp.x, vp.y, vp.width, vp.height,
                                          vp.minDepth, vp.maxDepth));
  Globals.Cmd.setScissor(0,
                         vk::Rect2D({int32_t(vp.x), int32_t(vp.y)},
                                    {uint32_t(vp.width), uint32_t(vp.height)}));
}

void RenderTextureAllocation::ProcessAttachArgs(const scissor &s) noexcept {
  Globals.Cmd.setViewport(
      0, vk::Viewport(0.f, 0.f, Extent.width, Extent.height, 0.f, 1.f));
  Globals.Cmd.setScissor(0, vk::Rect2D(s));
}

void RenderTextureAllocation::ProcessAttachArgs(const viewport &vp,
                                                const scissor &s) noexcept {
  Globals.Cmd.setViewport(0, vk::Viewport(vp.x, vp.y, vp.width, vp.height,
                                          vp.minDepth, vp.maxDepth));
  Globals.Cmd.setScissor(0, vk::Rect2D(s));
}
} // namespace hsh::detail::vulkan

namespace hsh::detail {
template <> struct TargetTraits<Target::VULKAN_SPIRV> {
  struct BufferWrapper {
    vk::Buffer Buffer;
    uint32_t Offset;
    BufferWrapper() noexcept = default;
    BufferWrapper(vk::Buffer Buffer, uint32_t Offset = 0) noexcept
        : Buffer(Buffer), Offset(Offset) {}
    BufferWrapper(const vulkan::BufferAllocation &Alloc) noexcept
        : BufferWrapper(Alloc.GetBuffer()) {}
    bool IsValid() const noexcept { return Buffer.operator bool(); }
    operator vk::Buffer() const noexcept { return Buffer; }
  };
  using UniformBufferOwner = vulkan::BufferAllocation;
  using UniformBufferBinding = BufferWrapper;
  using DynamicUniformBufferOwner = vulkan::DynamicBufferAllocation;
  using VertexBufferOwner = vulkan::BufferAllocation;
  using VertexBufferBinding = BufferWrapper;
  using DynamicVertexBufferOwner = vulkan::DynamicBufferAllocation;
  using IndexBufferOwner = vulkan::BufferAllocation;
  using IndexBufferBinding = BufferWrapper;
  using DynamicIndexBufferOwner = vulkan::DynamicBufferAllocation;
  struct FifoOwner {
    std::unique_ptr<vulkan::FifoBufferAllocation> Allocation;
    FifoOwner() noexcept = default;
    FifoOwner(const FifoOwner &other) = delete;
    FifoOwner &operator=(const FifoOwner &other) = delete;
    FifoOwner(FifoOwner &&other) noexcept = default;
    FifoOwner &operator=(FifoOwner &&other) noexcept = default;
    explicit FifoOwner(std::unique_ptr<vulkan::FifoBufferAllocation> Allocation)
        : Allocation(std::move(Allocation)) {}
    template <typename T, typename Func>
    UniformBufferBinding MapUniform(Func func) noexcept {
      return {Allocation->GetBuffer(), Allocation->MapUniform<T, Func>(func)};
    }
    template <typename T, typename Func>
    VertexBufferBinding MapVertex(std::size_t count, Func func) noexcept {
      return {Allocation->GetBuffer(),
              Allocation->MapVertexIndex<T, Func>(count, func)};
    }
    template <typename T, typename Func>
    IndexBufferBinding MapIndex(std::size_t count, Func func) noexcept {
      return {Allocation->GetBuffer(),
              Allocation->MapVertexIndex<T, Func>(count, func)};
    }
    bool IsValid() const noexcept { return Allocation.operator bool(); }
  };
  struct TextureBinding {
    vk::ImageView ImageView;
    std::uint8_t NumMips : 7;
    std::uint8_t Integer : 1;
    bool IsValid() const noexcept { return ImageView.operator bool(); }
  };
  struct TextureOwner {
    vulkan::TextureAllocation Allocation;
    vk::UniqueImageView ImageView;
    std::uint8_t NumMips : 7;
    std::uint8_t Integer : 1;
    TextureOwner() = default;
    TextureOwner(const TextureOwner &other) = delete;
    TextureOwner &operator=(const TextureOwner &other) = delete;
    TextureOwner(TextureOwner &&other) noexcept = default;
    TextureOwner &operator=(TextureOwner &&other) noexcept = default;

    TextureOwner(vulkan::TextureAllocation Allocation,
                 vk::UniqueImageView ImageView, std::uint8_t NumMips,
                 std::uint8_t Integer) noexcept
        : Allocation(std::move(Allocation)), ImageView(std::move(ImageView)),
          NumMips(NumMips), Integer(Integer) {}

    bool IsValid() const noexcept { return ImageView.operator bool(); }

    TextureBinding GetBinding() const noexcept {
      return TextureBinding{ImageView.get(), NumMips, Integer};
    }
    operator TextureBinding() const noexcept { return GetBinding(); }
  };
  struct DynamicTextureOwner : TextureOwner {
    vulkan::UploadBufferAllocation UploadAllocation;
    std::array<vk::BufferImageCopy, MaxMipCount> Copies;

    DynamicTextureOwner() noexcept = default;
    DynamicTextureOwner(vulkan::TextureAllocation AllocationIn,
                        vulkan::UploadBufferAllocation UploadAllocation,
                        std::array<vk::BufferImageCopy, MaxMipCount> Copies,
                        vk::UniqueImageView ImageViewIn, std::uint8_t NumMipsIn,
                        std::uint8_t IntegerIn) noexcept
        : TextureOwner(std::move(AllocationIn), std::move(ImageViewIn),
                       NumMipsIn, IntegerIn),
          UploadAllocation(std::move(UploadAllocation)),
          Copies(std::move(Copies)) {}

    void MakeCopies() noexcept {
      vulkan::Globals.CopyCmd.pipelineBarrier(
          vk::PipelineStageFlagBits::eTopOfPipe,
          vk::PipelineStageFlagBits::eTransfer,
          vk::DependencyFlagBits::eByRegion, {}, {},
          vk::ImageMemoryBarrier(
              vk::AccessFlagBits(0), vk::AccessFlagBits::eTransferWrite,
              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
              VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
              Allocation.GetImage(),
              vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                        VK_REMAINING_MIP_LEVELS, 0,
                                        VK_REMAINING_ARRAY_LAYERS)));
      vulkan::Globals.CopyCmd.copyBufferToImage(
          UploadAllocation.GetBuffer(), Allocation.GetImage(),
          vk::ImageLayout::eTransferDstOptimal, {NumMips, Copies.data()});
      vulkan::Globals.CopyCmd.pipelineBarrier(
          vk::PipelineStageFlagBits::eTransfer,
          vk::PipelineStageFlagBits::eVertexShader |
              vk::PipelineStageFlagBits::eTessellationControlShader |
              vk::PipelineStageFlagBits::eTessellationEvaluationShader |
              vk::PipelineStageFlagBits::eGeometryShader |
              vk::PipelineStageFlagBits::eFragmentShader,
          vk::DependencyFlagBits::eByRegion, {}, {},
          vk::ImageMemoryBarrier(
              vk::AccessFlagBits::eTransferWrite,
              vk::AccessFlagBits::eShaderRead,
              vk::ImageLayout::eTransferDstOptimal,
              vk::ImageLayout::eShaderReadOnlyOptimal, VK_QUEUE_FAMILY_IGNORED,
              VK_QUEUE_FAMILY_IGNORED, Allocation.GetImage(),
              vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                        VK_REMAINING_MIP_LEVELS, 0,
                                        VK_REMAINING_ARRAY_LAYERS)));
    }

    void *Map() noexcept { return UploadAllocation.GetMappedData(); }
    void Unmap() noexcept { MakeCopies(); }
  };
  struct RenderTextureBinding {
    vulkan::RenderTextureAllocation *Allocation = nullptr;
    uint32_t BindingIdx : 24;
    uint32_t IsDepth : 8;
    RenderTextureBinding() noexcept : BindingIdx(0), IsDepth(0) {}
    RenderTextureBinding(vulkan::RenderTextureAllocation *Allocation,
                         uint32_t BindingIdx, uint32_t IsDepth) noexcept
        : Allocation(Allocation), BindingIdx(BindingIdx), IsDepth(IsDepth) {}

    bool IsValid() const noexcept { return Allocation != nullptr; }

    vk::ImageView GetImageView() const noexcept {
      if (IsDepth)
        return Allocation->GetDepthBindingView(BindingIdx);
      else
        return Allocation->GetColorBindingView(BindingIdx);
    }
  };
  struct SurfaceBinding {
    vulkan::SurfaceAllocation *Allocation = nullptr;
    bool IsValid() const noexcept { return Allocation != nullptr; }
  };
  struct RenderTextureOwner {
    std::unique_ptr<vulkan::RenderTextureAllocation> Allocation;
    RenderTextureOwner() = default;
    RenderTextureOwner(const RenderTextureOwner &other) = delete;
    RenderTextureOwner &operator=(const RenderTextureOwner &other) = delete;
    RenderTextureOwner(RenderTextureOwner &&other) noexcept = default;
    RenderTextureOwner &
    operator=(RenderTextureOwner &&other) noexcept = default;

    explicit RenderTextureOwner(
        std::unique_ptr<vulkan::RenderTextureAllocation> Allocation) noexcept
        : Allocation(std::move(Allocation)) {}

    bool IsValid() const noexcept { return Allocation.operator bool(); }

    RenderTextureBinding GetColor(uint32_t idx) const noexcept {
      return {Allocation.get(), idx, false};
    }
    RenderTextureBinding GetDepth(uint32_t idx) const noexcept {
      return {Allocation.get(), idx, true};
    }
    template <typename... Args> void Attach(const Args &... args) noexcept {
      return Allocation->Attach(args...);
    }
    void ResolveSurface(SurfaceBinding surface, bool reattach) noexcept {
      Allocation->ResolveSurface(surface.Allocation, reattach);
    }
    void ResolveColorBinding(uint32_t idx, rect2d region,
                             bool reattach) noexcept {
      Allocation->ResolveColorBinding(idx, region, reattach);
    }
    void ResolveDepthBinding(uint32_t idx, rect2d region,
                             bool reattach) noexcept {
      Allocation->ResolveDepthBinding(idx, region, reattach);
    }
  };
  struct SurfaceOwner {
    std::unique_ptr<vulkan::SurfaceAllocation> Allocation;
    SurfaceOwner() = default;
    SurfaceOwner(const SurfaceOwner &other) = delete;
    SurfaceOwner &operator=(const SurfaceOwner &other) = delete;
    SurfaceOwner(SurfaceOwner &&other) noexcept = default;
    SurfaceOwner &operator=(SurfaceOwner &&other) noexcept = default;

    explicit SurfaceOwner(
        std::unique_ptr<vulkan::SurfaceAllocation> Allocation) noexcept
        : Allocation(std::move(Allocation)) {}

    bool IsValid() const noexcept { return Allocation.operator bool(); }

    SurfaceBinding GetBinding() const noexcept {
      return SurfaceBinding{Allocation.get()};
    }
    operator SurfaceBinding() const noexcept { return GetBinding(); }
    bool AcquireNextImage() noexcept { return Allocation->AcquireNextImage(); }
    void AttachResizeLambda(
        std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
            &&Resize) noexcept {
      Allocation->AttachResizeLambda(std::move(Resize));
    }
    void AttachDecorationLambda(std::function<void()> &&Dec) noexcept {
      Allocation->AttachDecorationLambda(std::move(Dec));
    }
    void AttachDeleterLambda(std::function<void()> &&Del) noexcept {
      Allocation->AttachDeleterLambda(std::move(Del));
    }
    void SetRequestExtent(const hsh::extent2d &Ext) noexcept {
      Allocation->SetRequestExtent(Ext);
    }
    void SetMargins(int32_t L, int32_t R, int32_t T, int32_t B) noexcept {
      Allocation->SetMargins(L, R, T, B);
    }
  };
  struct PipelineBinding {
    vk::Pipeline Pipeline;
    vulkan::UniqueDescriptorSet DescriptorSet;
    uint32_t NumVertexBuffers = 0;
    std::array<uint32_t, MaxUniforms> UniformOffsets{};
    std::array<vk::Buffer, MaxVertexBuffers> VertexBuffers{};
    std::array<vk::DeviceSize, MaxVertexBuffers> VertexOffsets{};
    struct BoundIndex {
      vk::Buffer Buffer{};
      vk::DeviceSize Offset{};
      vk::IndexType Type{};
    } Index;
    struct BoundRenderTexture {
      RenderTextureBinding RenderTextureBinding;
      vk::ImageView KnownImageView;
      uint32_t DescriptorBindingIdx = 0;
    };
    std::array<BoundRenderTexture, MaxImages> RenderTextures{};
    struct Iterators {
      decltype(VertexBuffers)::iterator VertexBufferBegin;
      decltype(VertexBuffers)::iterator VertexBufferIt;
      BoundIndex &Index;
      decltype(RenderTextures)::iterator RenderTextureIt;
      uint32_t TextureIdx = 0;
      constexpr explicit Iterators(PipelineBinding &Binding) noexcept
          : VertexBufferBegin(Binding.VertexBuffers.begin()),
            VertexBufferIt(Binding.VertexBuffers.begin()), Index(Binding.Index),
            RenderTextureIt(Binding.RenderTextures.begin()) {}

      inline void Add(uniform_buffer_typeless uniform) noexcept;
      inline void Add(vertex_buffer_typeless uniform) noexcept;
      template <typename T> inline void Add(index_buffer<T> uniform) noexcept;
      inline void Add(texture_typeless texture) noexcept;
      inline void Add(render_texture2d texture) noexcept;
      static inline void Add(SamplerBinding sampler) noexcept;
    };
    struct OffsetIterators {
      decltype(UniformOffsets)::iterator UniformOffsetsIt;
      decltype(VertexOffsets)::iterator VertexOffsetsIt;
      BoundIndex &Index;
      constexpr explicit OffsetIterators(PipelineBinding &Binding) noexcept
          : UniformOffsetsIt(Binding.UniformOffsets.begin()),
            VertexOffsetsIt(Binding.VertexOffsets.begin()),
            Index(Binding.Index) {}

      inline void Add(uniform_buffer_typeless uniform) noexcept;
      inline void Add(vertex_buffer_typeless uniform) noexcept;
      template <typename T> inline void Add(index_buffer<T> uniform) noexcept;
      inline void Add(texture_typeless texture) noexcept;
      inline void Add(render_texture2d texture) noexcept;
      static inline void Add(SamplerBinding sampler) noexcept;
    };

    bool IsValid() const noexcept { return Pipeline.operator bool(); }

    PipelineBinding() noexcept = default;

    template <typename Impl, typename... Args>
    void Rebind(bool UpdateDescriptors, Args... args) noexcept;

    void UpdateRenderTextures() noexcept {
      std::array<vk::DescriptorImageInfo, MaxImages> ImageInfos;
      std::array<vk::WriteDescriptorSet, MaxImages> Writes;
      uint32_t WriteCur = 0;
      for (auto &RT : RenderTextures) {
        if (!RT.RenderTextureBinding.Allocation)
          break;
        auto ImageView = RT.RenderTextureBinding.GetImageView();
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

    void Bind() noexcept {
      for (auto &RT : RenderTextures) {
        if (!RT.RenderTextureBinding.Allocation)
          break;
        if (RT.RenderTextureBinding.GetImageView() != RT.KnownImageView) {
          UpdateRenderTextures();
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
            DescriptorSet.Set, UniformOffsets);
        vulkan::Globals.Cmd.bindVertexBuffers(
            0, NumVertexBuffers, VertexBuffers.data(),
            (const vk::DeviceSize *)VertexOffsets.data());
        if (Index.Buffer)
          vulkan::Globals.Cmd.bindIndexBuffer(Index.Buffer, Index.Offset,
                                              Index.Type);
      }
    }

    void Draw(uint32_t start, uint32_t count) noexcept {
      Bind();
      vulkan::Globals.Cmd.draw(count, 1, start, 0);
    }

    void DrawIndexed(uint32_t start, uint32_t count) noexcept {
      Bind();
      vulkan::Globals.Cmd.drawIndexed(count, 1, start, 0, 0);
    }

    void DrawInstanced(uint32_t start, uint32_t count,
                       uint32_t instCount) noexcept {
      Bind();
      vulkan::Globals.Cmd.draw(count, instCount, start, 0);
    }

    void DrawIndexedInstanced(uint32_t start, uint32_t count,
                              uint32_t instCount) noexcept {
      Bind();
      vulkan::Globals.Cmd.drawIndexed(count, instCount, start, 0, 0);
    }
  };

  static void ClearAttachments(bool color, bool depth) noexcept {
    assert(vulkan::Globals.AttachedRenderTexture != nullptr);
    vk::ClearRect Rect(vk::Rect2D({}, hsh::detail::vulkan::Globals
                                          .AttachedRenderTexture->GetExtent()),
                       0, 1);
    if (color && depth) {
      vulkan::Globals.Cmd.clearAttachments(
          {vk::ClearAttachment(vk::ImageAspectFlagBits::eColor, 0,
                               vk::ClearValue(vk::ClearColorValue())),
           vk::ClearAttachment(vk::ImageAspectFlagBits::eDepth, 0,
                               vk::ClearValue(vk::ClearDepthStencilValue()))},
          Rect);
    } else if (color) {
      vulkan::Globals.Cmd.clearAttachments(
          vk::ClearAttachment(vk::ImageAspectFlagBits::eColor, 0,
                              vk::ClearValue(vk::ClearColorValue())),
          Rect);
    } else if (depth) {
      vulkan::Globals.Cmd.clearAttachments(
          vk::ClearAttachment(vk::ImageAspectFlagBits::eDepth, 0,
                              vk::ClearValue(vk::ClearDepthStencilValue())),
          Rect);
    }
  }

  static void SetBlendConstants(float red, float green, float blue,
                                float alpha) noexcept {
    const float constants[] = {red, green, blue, alpha};
    vulkan::Globals.Cmd.setBlendConstants(constants);
  }

  template <typename ResTp> struct ResourceFactory {};
};
} // namespace hsh::detail

#endif
