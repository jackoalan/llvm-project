#pragma once

#if HSH_ENABLE_DEKO3D

#include <memory>

namespace hsh::detail::deko {
class BufferAllocation {
  friend class DeletedAllocation;

protected:
  DkBufExtents Buffer{DK_GPU_ADDR_INVALID, 0};
  DmaAllocation Allocation = DMA_NULL_HANDLE;
  BufferAllocation(DkBufExtents Buffer, DmaAllocation Allocation) noexcept
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
  DkBufExtents GetBuffer() const noexcept { return Buffer; }
  operator DkBufExtents() const noexcept { return GetBuffer(); }
  bool IsValid() const noexcept { return Buffer.addr != DK_GPU_ADDR_INVALID; }
  uint32_t GetSize() const noexcept { return Buffer.size; }
};

template <typename T> class DescriptorSetAllocation;

class UploadBufferAllocation : public BufferAllocation {
  friend UploadBufferAllocation AllocateUploadBuffer(uint32_t size) noexcept;
  friend class StaticBufferAllocation
  AllocateStaticBuffer(uint32_t size, uint32_t alignment) noexcept;
  template <typename U>
  friend DescriptorSetAllocation<U>
  AllocateDescriptorSet(uint32_t Count) noexcept;
  void *MappedData = nullptr;
  UploadBufferAllocation(DkBufExtents BufferIn, DmaAllocation AllocationIn,
                         void *MappedData) noexcept
      : BufferAllocation(BufferIn, AllocationIn), MappedData(MappedData) {}

public:
  UploadBufferAllocation() noexcept = default;
  void *GetMappedData() const noexcept { return MappedData; }
  inline void Flush() noexcept;
};

class StaticBufferAllocation : public UploadBufferAllocation {
  friend StaticBufferAllocation
  AllocateStaticBuffer(uint32_t size, uint32_t alignment) noexcept;
  using UploadBufferAllocation::UploadBufferAllocation;
};

class DynamicBufferAllocation : public BufferAllocation {
  UploadBufferAllocation UploadBuffer;
  friend DynamicBufferAllocation
  AllocateDynamicBuffer(uint32_t size, uint32_t alignment) noexcept;
  DynamicBufferAllocation(DkBufExtents BufferIn, DmaAllocation AllocationIn,
                          UploadBufferAllocation UploadBuffer) noexcept
      : BufferAllocation(BufferIn, AllocationIn),
        UploadBuffer(std::move(UploadBuffer)) {}

public:
  DynamicBufferAllocation() noexcept = default;
  void *Map() noexcept { return UploadBuffer.GetMappedData(); }
  inline void Unmap() noexcept;
};

class FifoBufferAllocation : public BufferAllocation {
  FifoBufferAllocation *Prev = nullptr, *Next = nullptr;
  void *MappedData;
  uint32_t Size;
  uint32_t Offset = 0;
  uint32_t StartOffset = 0;
  friend std::unique_ptr<FifoBufferAllocation>
  AllocateFifoBuffer(uint32_t size, uint32_t alignment) noexcept;
  inline FifoBufferAllocation(DkBufExtents BufferIn, DmaAllocation AllocationIn,
                              uint32_t Size, void *MappedData) noexcept;

public:
  inline ~FifoBufferAllocation() noexcept;
  template <typename T, typename Func>
  inline uint32_t MapUniform(Func func) noexcept;
  template <typename T, typename Func>
  inline uint32_t MapVertexIndex(std::size_t count, Func func) noexcept;
  inline void PostRender() noexcept;
  FifoBufferAllocation *GetNext() const noexcept { return Next; }
};

template <typename T>
class DescriptorSetAllocation : public UploadBufferAllocation {
  template <typename U>
  friend DescriptorSetAllocation AllocateDescriptorSet(uint32_t Count) noexcept;
  using UploadBufferAllocation::UploadBufferAllocation;

public:
  T *Map() noexcept { return reinterpret_cast<T*>(GetMappedData()); }
  void Unmap() noexcept { Flush(); }
  inline void Bind() noexcept;
};

struct TextureAllocation {
  friend class DeletedAllocation;
  friend TextureAllocation
  AllocateTexture(const dk::ImageLayoutMaker &CreateInfo,
                  bool Dedicated) noexcept;

protected:
  dk::Image Image;
  DmaAllocation Allocation = DMA_NULL_HANDLE;
  TextureAllocation(const dk::ImageLayout &ImageLayout, DkMemBlock MemBlock,
                    uint32_t Offset, DmaAllocation Allocation) noexcept
      : Allocation(Allocation) {
    Image.initialize(ImageLayout, MemBlock, Offset);
  }

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
  const dk::Image &GetImage() const noexcept { return Image; }
  operator bool() const noexcept { return Allocation != DMA_NULL_HANDLE; }
};

class SurfaceAllocation {
  friend class DeletedSurfaceAllocation;
  friend class RenderTextureAllocation;
  friend class DeletedSurfaceSwapchainImage;
  friend class DeletedResources;
  static constexpr size_t NumSwapchainImages = 2;
  SurfaceAllocation *Prev = nullptr, *Next = nullptr;
  void *Surface;
  dk::UniqueMemBlock ImageBlock;
  dk::UniqueSwapchain Swapchain;
  struct SwapchainImage {
    dk::Image Image;
    dk::ImageView ColorView{Image};
    SwapchainImage() noexcept = default;
    SwapchainImage(const SwapchainImage &other) = delete;
    SwapchainImage &operator=(const SwapchainImage &other) = delete;
    SwapchainImage(SwapchainImage &&other) = delete;
    SwapchainImage &operator=(SwapchainImage &&other) = delete;
  };
  std::array<SwapchainImage, NumSwapchainImages> SwapchainImages;
  hsh::extent2d Extent;
  int NextImage = -1;
  std::function<void()> DeleterLambda;

public:
  inline ~SurfaceAllocation() noexcept;
  inline explicit SurfaceAllocation(void *Surface,
                                    std::function<void()> &&DeleterLambda,
                                    const hsh::extent2d &Extent) noexcept;
  SurfaceAllocation(const SurfaceAllocation &other) = delete;
  SurfaceAllocation &operator=(const SurfaceAllocation &other) = delete;
  SurfaceAllocation(SurfaceAllocation &&other) noexcept = delete;
  SurfaceAllocation &operator=(SurfaceAllocation &&other) noexcept = delete;
  inline bool AcquireNextImage() noexcept;
  inline void AttachDeleterLambda(std::function<void()> &&Del) noexcept;
  inline void PostRender() noexcept;
  const hsh::extent2d &GetExtent() const noexcept { return Extent; }
  SurfaceAllocation *GetNext() const noexcept { return Next; }
};

class RenderTextureAllocation {
  friend class DeletedRenderTextureAllocation;
  RenderTextureAllocation *Prev = nullptr, *Next = nullptr;
  SurfaceAllocation *Surface = nullptr;
  hsh::extent2d Extent;
  uint32_t NumColorBindings, NumDepthBindings;
  TextureAllocation ColorTexture;
  dk::ImageView ColorView{ColorTexture.GetImage()};
  TextureAllocation DepthTexture;
  dk::ImageView DepthView{DepthTexture.GetImage()};
  struct Binding {
    TextureAllocation Texture;
    dk::ImageView ImageView{Texture.GetImage()};
  };
  std::array<Binding, MaxRenderTextureBindings> ColorBindings;
  std::array<Binding, MaxRenderTextureBindings> DepthBindings;
  inline void Prepare() noexcept;
  static inline void _Resolve(const dk::ImageView &SrcImage,
                              const dk::ImageView &DstImage,
                              const DkImageRect &SrcRect,
                              const DkImageRect &DstRect) noexcept;
  inline void Resolve(const dk::ImageView &SrcImage,
                      const dk::ImageView &DstImage, const DkImageRect &SrcRect,
                      const DkImageRect &DstRect) noexcept;

public:
  inline ~RenderTextureAllocation() noexcept;
  inline explicit RenderTextureAllocation(SurfaceAllocation *Surface,
                                          uint32_t NumColorBindings,
                                          uint32_t NumDepthBindings) noexcept;
  inline explicit RenderTextureAllocation(extent2d Extent, DkImageFormat Format,
                                          uint32_t NumColorBindings,
                                          uint32_t NumDepthBindings) noexcept;
  RenderTextureAllocation(const RenderTextureAllocation &other) = delete;
  RenderTextureAllocation &operator=(const TextureAllocation &other) = delete;
  RenderTextureAllocation(RenderTextureAllocation &&other) noexcept = delete;
  RenderTextureAllocation &
  operator=(RenderTextureAllocation &&other) noexcept = delete;
  inline void BeginRenderPass() noexcept;
  inline void ResolveSurface(SurfaceAllocation *Surface) noexcept;
  inline void ResolveColorBinding(uint32_t Idx, rect2d Region) noexcept;
  inline void ResolveDepthBinding(uint32_t Idx, rect2d Region) noexcept;
  const dk::ImageView &GetColorBindingView(uint32_t Idx) const noexcept {
    assert(Idx < NumColorBindings);
    return ColorBindings[Idx].ImageView;
  }
  const dk::ImageView &GetDepthBindingView(uint32_t Idx) const noexcept {
    assert(Idx < NumDepthBindings);
    return DepthBindings[Idx].ImageView;
  }
  template <typename... Args> inline void Attach(const Args &... args) noexcept;
  inline void ProcessAttachArgs() noexcept;
  inline void ProcessAttachArgs(const viewport &vp) noexcept;
  inline void ProcessAttachArgs(const scissor &s) noexcept;
  inline void ProcessAttachArgs(const viewport &vp, const scissor &s) noexcept;
  RenderTextureAllocation *GetNext() const noexcept { return Next; }
  extent2d GetExtent() const noexcept { return Extent; }
};

class DeletedAllocation {
  DmaAllocation Allocation = DMA_NULL_HANDLE;

public:
  explicit DeletedAllocation(BufferAllocation &&Obj) noexcept {
    std::swap(Allocation, Obj.Allocation);
  }
  DeletedAllocation &operator=(BufferAllocation &&Obj) noexcept {
    std::swap(Allocation, Obj.Allocation);
    return *this;
  }
  explicit DeletedAllocation(TextureAllocation &&other) noexcept {
    std::swap(Allocation, other.Allocation);
  }
  DeletedAllocation(const DeletedAllocation &other) = delete;
  DeletedAllocation &operator=(const DeletedAllocation &other) = delete;
  DeletedAllocation(DeletedAllocation &&other) noexcept {
    std::swap(Allocation, other.Allocation);
  }
  DeletedAllocation &operator=(DeletedAllocation &&other) noexcept {
    std::swap(Allocation, other.Allocation);
    return *this;
  }
  inline ~DeletedAllocation() noexcept;
};

class DeletedSurfaceAllocation {
  dk::UniqueMemBlock ImageBlock;
  dk::UniqueSwapchain Swapchain;
  std::function<void()> DeleterLambda;

public:
  explicit DeletedSurfaceAllocation(SurfaceAllocation &&Obj) noexcept
      : ImageBlock(std::move(Obj.ImageBlock)),
        Swapchain(std::move(Obj.Swapchain)),
        DeleterLambda(std::move(Obj.DeleterLambda)) {}

  ~DeletedSurfaceAllocation() noexcept {
    Swapchain = dk::UniqueSwapchain{};
    ImageBlock = dk::UniqueMemBlock{};
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

class DeletedRenderTextureAllocation {
  DeletedAllocation ColorTexture;
  DeletedAllocation DepthTexture;
  struct Binding {
    DeletedAllocation Texture;
    Binding(RenderTextureAllocation::Binding &&Obj) noexcept
        : Texture(std::move(Obj.Texture)) {}
  };
  std::array<Binding, MaxRenderTextureBindings> ColorBindings;
  std::array<Binding, MaxRenderTextureBindings> DepthBindings;

public:
  template <std::size_t... CSeq, std::size_t... DSeq>
  explicit DeletedRenderTextureAllocation(RenderTextureAllocation &&Obj,
                                          std::index_sequence<CSeq...>,
                                          std::index_sequence<DSeq...>) noexcept
      : ColorTexture(std::move(Obj.ColorTexture)),
        DepthTexture(std::move(Obj.DepthTexture)),
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
  std::vector<DeletedAllocation> Allocations;
  std::vector<DeletedSurfaceAllocation> Surfaces;
  std::vector<DeletedRenderTextureAllocation> RenderTextures;

public:
  void DeleteLater(BufferAllocation &&Obj) noexcept {
    Allocations.emplace_back(std::move(Obj));
  }
  void DeleteLater(TextureAllocation &&Obj) noexcept {
    Allocations.emplace_back(std::move(Obj));
  }
  void DeleteLater(SurfaceAllocation &&Obj) noexcept {
    Surfaces.emplace_back(std::move(Obj));
  }
  void DeleteLater(RenderTextureAllocation &&Obj) noexcept {
    RenderTextures.emplace_back(std::move(Obj));
  }
  void Purge() noexcept {
    Allocations.clear();
    Surfaces.clear();
    RenderTextures.clear();
  }
  DeletedResources() noexcept = default;
  DeletedResources(const DeletedResources &) = delete;
  DeletedResources &operator=(const DeletedResources &) = delete;
};

struct DekoGlobals {
  dk::Device Device;
  DmaAllocator Allocator = nullptr;
  dk::Queue Queue;
  dk::MemBlock ShaderMem;
  float Anisotropy = 0.f;
  std::array<dk::CmdBuf, 2> CommandBuffers;
  std::array<dk::Fence *, 2> CommandFences{};
  dk::CmdBuf Cmd;
  dk::Fence *CmdFence = nullptr;
  dk::CmdBuf CopyCmd;
  dk::Fence *CopyFence = nullptr;
  dk::Fence *ImageAcquireSem = nullptr;
  RenderTextureAllocation *AttachedRenderTexture = nullptr;
  uint64_t Frame = 0;
  bool AcquiredImage = false;

  std::array<DeletedResources, 2> *DeletedResourcesArr = nullptr;
  DeletedResources *DeletedResourcesObj = nullptr;
  SurfaceAllocation *SurfaceHead = nullptr;
  RenderTextureAllocation *RenderTextureHead = nullptr;
  FifoBufferAllocation *FifoBufferHead = nullptr;

  constexpr DekoGlobals() = default;

  void PreRender() noexcept {
    uint32_t CurBufferIdx = Frame & 1u;
    Cmd = CommandBuffers[CurBufferIdx];
    CmdFence = CommandFences[CurBufferIdx];
    CmdFence->wait();
    DeletedResourcesObj = &(*DeletedResourcesArr)[CurBufferIdx];
    DeletedResourcesObj->Purge();
    CopyCmd.clear();
    Cmd.clear();
  }

  void PostRender() noexcept {
    AttachedRenderTexture = nullptr;
    DkCmdList CopyList = CopyCmd.finishList();
    DkCmdList CmdList = Cmd.finishList();
    for (auto *Fifo = FifoBufferHead; Fifo; Fifo = Fifo->GetNext())
      Fifo->PostRender();
    Queue.submitCommands(CopyList);
    Queue.signalFence(*CopyFence, true);
    Queue.submitCommands(CmdList);
    for (auto *Surf = SurfaceHead; Surf; Surf = Surf->GetNext())
      Surf->PostRender();
    AcquiredImage = false;
    Queue.flush();
    CopyFence->wait();
    ++Frame;
  }
};
inline DekoGlobals Globals;

bool SurfaceAllocation::AcquireNextImage() noexcept {
  Swapchain.acquireImage(NextImage, *Globals.ImageAcquireSem);
  Globals.ImageAcquireSem->wait();
  Globals.AcquiredImage = true;
  return true;
}

void SurfaceAllocation::AttachDeleterLambda(
    std::function<void()> &&Del) noexcept {
  DeleterLambda = std::move(Del);
}

void SurfaceAllocation::PostRender() noexcept {
  if (NextImage != -1) {
    if (Globals.AcquiredImage) {
      Globals.Queue.presentImage(Swapchain, NextImage);
    }
    NextImage = -1;
  }
}

BufferAllocation::~BufferAllocation() noexcept {
  if (Allocation)
    Globals.DeletedResourcesObj->DeleteLater(std::move(*this));
}

DeletedAllocation::~DeletedAllocation() noexcept {
  dmaFreeMemory(Globals.Allocator, Allocation);
}

TextureAllocation::~TextureAllocation() noexcept {
  if (Allocation)
    Globals.DeletedResourcesObj->DeleteLater(std::move(*this));
}

SurfaceAllocation::~SurfaceAllocation() noexcept {
  if (Prev) {
    Prev->Next = Next;
  } else {
    Globals.SurfaceHead = Next;
  }
  if (Next)
    Next->Prev = Prev;
  Globals.DeletedResourcesObj->DeleteLater(std::move(*this));
}

constexpr DkImageFormat FBColorFormat = DkImageFormat_RGBA8_Unorm;
constexpr DkImageFormat FBDepthFormat = DkImageFormat_ZF32;

SurfaceAllocation::SurfaceAllocation(void *Surface,
                                     std::function<void()> &&DeleterLambda,
                                     const hsh::extent2d &Extent) noexcept
    : Next(Globals.SurfaceHead), Surface(std::move(Surface)), Extent(Extent),
      DeleterLambda(std::move(DeleterLambda)) {
  Globals.SurfaceHead = this;
  if (Next)
    Next->Prev = this;

  dk::ImageLayoutMaker LayoutMaker(Globals.Device);
  LayoutMaker.setFlags(DkImageFlags_Usage2DEngine | DkImageFlags_UsagePresent |
                       DkImageFlags_UsageRender | DkImageFlags_HwCompression);
  LayoutMaker.setFormat(FBColorFormat);
  LayoutMaker.setDimensions(Extent.w, Extent.h);

  dk::ImageLayout Layout;
  LayoutMaker.initialize(Layout);

  auto ImageAlign = Layout.getAlignment();
  auto ImageSize = AlignUp(uint32_t(Layout.getSize()), ImageAlign);

  dk::MemBlockMaker MemMaker(Globals.Device, ImageSize * NumSwapchainImages);
  MemMaker.setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image);
  ImageBlock = MemMaker.create();

  std::array<const DkImage *, NumSwapchainImages> ImagePointers;
  for (size_t i = 0; i < NumSwapchainImages; ++i) {
    SwapchainImages[i].Image.initialize(Layout, ImageBlock, ImageSize * i);
    ImagePointers[i] = &SwapchainImages[i].Image;
  }

  dk::SwapchainMaker SCMaker(Globals.Device, Surface, ImagePointers);
  Swapchain = SCMaker.create();
}

RenderTextureAllocation::~RenderTextureAllocation() noexcept {
  if (Prev)
    Prev->Next = Next;
  else
    Globals.RenderTextureHead = Next;
  if (Next)
    Next->Prev = Prev;
  Globals.DeletedResourcesObj->DeleteLater(std::move(*this));
}

RenderTextureAllocation::RenderTextureAllocation(
    SurfaceAllocation *Surface, uint32_t NumColorBindings,
    uint32_t NumDepthBindings) noexcept
    : Next(Globals.RenderTextureHead), Surface(Surface),
      Extent(Surface->Extent), NumColorBindings(NumColorBindings),
      NumDepthBindings(NumDepthBindings) {
  Globals.RenderTextureHead = this;
  if (Next)
    Next->Prev = this;
  assert(Surface);
  assert(NumColorBindings <= MaxRenderTextureBindings);
  assert(NumDepthBindings <= MaxRenderTextureBindings);
  Prepare();
}

RenderTextureAllocation::RenderTextureAllocation(
    extent2d Extent, DkImageFormat Format, uint32_t NumColorBindings,
    uint32_t NumDepthBindings) noexcept
    : Next(Globals.RenderTextureHead), Extent(Extent),
      NumColorBindings(NumColorBindings), NumDepthBindings(NumDepthBindings) {
  Globals.RenderTextureHead = this;
  if (Next)
    Next->Prev = this;
  assert(Format == FBColorFormat);
  assert(NumColorBindings <= MaxRenderTextureBindings);
  assert(NumDepthBindings <= MaxRenderTextureBindings);
  Prepare();
}

void UploadBufferAllocation::Flush() noexcept {
  if (Allocation)
    dmaFlushAllocation(Globals.Allocator, Allocation, 0, Buffer.size);
}

void DynamicBufferAllocation::Unmap() noexcept {
  UploadBuffer.Flush();
  Globals.CopyCmd.copyBuffer(UploadBuffer.GetBuffer().addr, GetBuffer().addr,
                             UploadBuffer.GetSize());
}

FifoBufferAllocation::FifoBufferAllocation(DkBufExtents BufferIn,
                                           DmaAllocation AllocationIn,
                                           uint32_t Size,
                                           void *MappedData) noexcept
    : BufferAllocation(BufferIn, AllocationIn), Next(Globals.FifoBufferHead),
      MappedData(MappedData), Size(Size) {
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
                                    uint32_t(DK_UNIFORM_BUF_ALIGNMENT)));
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
  dmaFlushAllocation(Globals.Allocator, Allocation, StartOffset,
                     Offset - StartOffset);
  if (StartOffset) {
    Offset = 0;
    StartOffset = 0;
  } else {
    StartOffset = Offset;
  }
}

template <typename T> inline void DescriptorSetAllocation<T>::Bind() noexcept {
  assert(0 && "Invalid descriptor type");
}

template <>
inline void DescriptorSetAllocation<dk::ImageDescriptor>::Bind() noexcept {
  if (IsValid())
    deko::Globals.Cmd.bindImageDescriptorSet(
        Buffer.addr, Buffer.size / sizeof(dk::ImageDescriptor));
}

template <>
inline void DescriptorSetAllocation<dk::SamplerDescriptor>::Bind() noexcept {
  if (IsValid())
    deko::Globals.Cmd.bindSamplerDescriptorSet(
        Buffer.addr, Buffer.size / sizeof(dk::SamplerDescriptor));
}

inline DkResult dmaCreateAllocator(const DmaAllocatorCreateInfo &pCreateInfo,
                                   DmaAllocator *pAllocator) noexcept {
  return ::dmaCreateAllocator(
      reinterpret_cast<const DmaAllocatorCreateInfo *>(&pCreateInfo),
      pAllocator);
}

inline DkResult
dmaAllocateMemory(const DmaMemoryRequirements &pDmaMemoryRequirements,
                  const DmaAllocationCreateInfo &pAllocationCreateInfo,
                  DmaAllocation *pAllocation,
                  DmaAllocationInfo *pAllocationInfo) noexcept {
  return ::dmaAllocateMemory(
      Globals.Allocator,
      reinterpret_cast<const DmaMemoryRequirements *>(&pDmaMemoryRequirements),
      reinterpret_cast<const DmaAllocationCreateInfo *>(&pAllocationCreateInfo),
      pAllocation, pAllocationInfo);
}

struct AllocationCreateInfoMapped : DmaAllocationCreateInfo {
  constexpr AllocationCreateInfoMapped() noexcept
      : DmaAllocationCreateInfo{DMA_ALLOCATION_CREATE_MAPPED_BIT,
                                DMA_NULL_HANDLE, nullptr} {}
};

struct AllocationCreateInfoUnmapped : DmaAllocationCreateInfo {
  constexpr AllocationCreateInfoUnmapped() noexcept
      : DmaAllocationCreateInfo{0, DMA_NULL_HANDLE, nullptr} {}
};

inline UploadBufferAllocation AllocateUploadBuffer(uint32_t size) noexcept {
  DmaAllocation Allocation;
  DmaAllocationInfo AllocInfo;
  auto Result =
      dmaAllocateMemory(DmaMemoryRequirements{size, 4, DmaMT_Upload},
                        AllocationCreateInfoMapped{}, &Allocation, &AllocInfo);
  assert(Result == DkResult_Success);
  return UploadBufferAllocation(
      DkBufExtents{dkMemBlockGetGpuAddr(AllocInfo.deviceMemory) +
                       AllocInfo.offset,
                   size},
      Allocation, AllocInfo.pMappedData);
}

inline StaticBufferAllocation
AllocateStaticBuffer(uint32_t size, uint32_t alignment) noexcept {
  DmaAllocation Allocation;
  DmaAllocationInfo AllocInfo;
  auto Result = dmaAllocateMemory(
      DmaMemoryRequirements{size, alignment, DmaMT_GenericCpu},
      AllocationCreateInfoMapped{}, &Allocation, &AllocInfo);
  assert(Result == DkResult_Success);
  return StaticBufferAllocation(
      DkBufExtents{dkMemBlockGetGpuAddr(AllocInfo.deviceMemory) +
                       AllocInfo.offset,
                   size},
      Allocation, AllocInfo.pMappedData);
}

inline DynamicBufferAllocation
AllocateDynamicBuffer(uint32_t size, uint32_t alignment) noexcept {
  DmaAllocation Allocation;
  DmaAllocationInfo AllocInfo;
  auto Result = dmaAllocateMemory(
      DmaMemoryRequirements{size, alignment, DmaMT_Generic},
      AllocationCreateInfoUnmapped{}, &Allocation, &AllocInfo);
  assert(Result == DkResult_Success);
  return DynamicBufferAllocation(
      DkBufExtents{dkMemBlockGetGpuAddr(AllocInfo.deviceMemory) +
                       AllocInfo.offset,
                   size},
      Allocation, AllocateUploadBuffer(size));
}

inline std::unique_ptr<FifoBufferAllocation>
AllocateFifoBuffer(uint32_t size, uint32_t alignment) noexcept {
  DmaAllocation Allocation;
  DmaAllocationInfo AllocInfo;
  auto Result = dmaAllocateMemory(
      DmaMemoryRequirements{size, alignment, DmaMT_GenericCpu},
      AllocationCreateInfoMapped{}, &Allocation, &AllocInfo);
  assert(Result == DkResult_Success);
  return std::unique_ptr<FifoBufferAllocation>{new FifoBufferAllocation(
      DkBufExtents{dkMemBlockGetGpuAddr(AllocInfo.deviceMemory) +
                       AllocInfo.offset,
                   size},
      Allocation, size, AllocInfo.pMappedData)};
}

template <typename T> struct DescriptorSetAlignment {};
template <> struct DescriptorSetAlignment<dk::ImageDescriptor> {
  static constexpr uint32_t Alignment = DK_IMAGE_DESCRIPTOR_ALIGNMENT;
};
template <> struct DescriptorSetAlignment<dk::SamplerDescriptor> {
  static constexpr uint32_t Alignment = DK_SAMPLER_DESCRIPTOR_ALIGNMENT;
};

template <typename T>
inline DescriptorSetAllocation<T>
AllocateDescriptorSet(uint32_t Count) noexcept {
  DmaAllocation Allocation;
  DmaAllocationInfo AllocInfo;
  auto Result = dmaAllocateMemory(
      DmaMemoryRequirements{uint32_t(Count * sizeof(T)),
                            DescriptorSetAlignment<T>::Alignment,
                            DmaMT_GenericCpu},
      AllocationCreateInfoMapped{}, &Allocation, &AllocInfo);
  assert(Result == DkResult_Success);
  return DescriptorSetAllocation<T>(
      DkBufExtents{dkMemBlockGetGpuAddr(AllocInfo.deviceMemory) +
                       AllocInfo.offset,
                   uint32_t(Count * sizeof(T))},
      Allocation, AllocInfo.pMappedData);
}

inline TextureAllocation AllocateTexture(const dk::ImageLayoutMaker &CreateInfo,
                                         bool Dedicated) noexcept {
  struct TextureAllocationCreateInfo : DmaAllocationCreateInfo {
    constexpr TextureAllocationCreateInfo(bool Dedicated) noexcept
        : DmaAllocationCreateInfo{
              Dedicated ? DMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
                        : DmaAllocationCreateFlagBits(0),
              DMA_NULL_HANDLE, nullptr} {}
  } AllocationCreateInfo{Dedicated};
  dk::ImageLayout Layout;
  CreateInfo.initialize(Layout);
  DmaAllocation Allocation;
  DmaAllocationInfo AllocInfo;
  auto Result = dmaAllocateMemory(
      DmaMemoryRequirements{uint32_t(Layout.getSize()), Layout.getAlignment(),
                            DmaMT_Image},
      AllocationCreateInfo, &Allocation, &AllocInfo);
  assert(Result == DkResult_Success);
  return TextureAllocation(Layout, AllocInfo.deviceMemory, AllocInfo.offset,
                           Allocation);
}

void RenderTextureAllocation::Prepare() noexcept {
  ColorTexture = AllocateTexture(dk::ImageLayoutMaker(Globals.Device)
                                     .setFlags(DkImageFlags_UsageRender |
                                               DkImageFlags_Usage2DEngine |
                                               DkImageFlags_HwCompression)
                                     .setFormat(FBColorFormat)
                                     .setDimensions(Extent.w, Extent.h),
                                 true);
  DepthTexture = AllocateTexture(dk::ImageLayoutMaker(Globals.Device)
                                     .setFlags(DkImageFlags_UsageRender |
                                               DkImageFlags_Usage2DEngine |
                                               DkImageFlags_HwCompression)
                                     .setFormat(FBDepthFormat)
                                     .setDimensions(Extent.w, Extent.h),
                                 true);
  for (uint32_t i = 0; i < NumColorBindings; ++i) {
    ColorBindings[i].Texture = AllocateTexture(
        dk::ImageLayoutMaker(Globals.Device)
            .setFlags(DkImageFlags_Usage2DEngine | DkImageFlags_HwCompression)
            .setFormat(FBColorFormat)
            .setDimensions(Extent.w, Extent.h),
        true);
  }
  for (uint32_t i = 0; i < NumDepthBindings; ++i) {
    DepthBindings[i].Texture = AllocateTexture(
        dk::ImageLayoutMaker(Globals.Device)
            .setFlags(DkImageFlags_Usage2DEngine | DkImageFlags_HwCompression)
            .setFormat(FBDepthFormat)
            .setDimensions(Extent.w, Extent.h),
        true);
  }
}

void RenderTextureAllocation::BeginRenderPass() noexcept {
  Globals.Cmd.bindRenderTargets(&ColorView, &DepthView);
}

void RenderTextureAllocation::_Resolve(const dk::ImageView &SrcImage,
                                       const dk::ImageView &DstImage,
                                       const DkImageRect &SrcRect,
                                       const DkImageRect &DstRect) noexcept {
  // TODO: Multisample resolve
  Globals.Cmd.blitImage(SrcImage, SrcRect, DstImage, DstRect);
}

void RenderTextureAllocation::Resolve(const dk::ImageView &SrcImage,
                                      const dk::ImageView &DstImage,
                                      const DkImageRect &SrcRect,
                                      const DkImageRect &DstRect) noexcept {
  _Resolve(SrcImage, DstImage, SrcRect, DstRect);
}

void RenderTextureAllocation::ResolveSurface(
    SurfaceAllocation *Surface) noexcept {
  assert(Surface->NextImage != UINT32_MAX &&
         "acquireNextImage not called on surface for this frame");
  assert(Surface->Extent == Extent &&
         "Mismatched render texture / surface extents");
  auto &DstImage = Surface->SwapchainImages[Surface->NextImage];
  DkImageRect Rect{0, 0, 0, Extent.w, Extent.h, 1};
  _Resolve(ColorView, DstImage.ColorView, Rect, Rect);
}

void RenderTextureAllocation::ResolveColorBinding(uint32_t Idx,
                                                  rect2d region) noexcept {
  assert(Idx < NumColorBindings);
  DkImageRect Rect{uint32_t(region.offset.x), uint32_t(region.offset.y), 0,
                   region.extent.w,           region.extent.h,           1};
  Resolve(ColorView, ColorBindings[Idx].ImageView, Rect, Rect);
}

void RenderTextureAllocation::ResolveDepthBinding(uint32_t Idx,
                                                  rect2d region) noexcept {
  assert(Idx < NumDepthBindings);
  DkImageRect Rect{uint32_t(region.offset.x), uint32_t(region.offset.y), 0,
                   region.extent.w,           region.extent.h,           1};
  Resolve(DepthView, DepthBindings[Idx].ImageView, Rect, Rect);
}

template <typename... Args>
void RenderTextureAllocation::Attach(const Args &... args) noexcept {
  if (Globals.AttachedRenderTexture == this)
    return;
  Globals.AttachedRenderTexture = this;
  BeginRenderPass();
  ProcessAttachArgs(args...);
}

void RenderTextureAllocation::ProcessAttachArgs() noexcept {
  Globals.Cmd.setViewports(
      0, DkViewport{0.f, 0.f, float(Extent.w), float(Extent.h), 0.f, 1.f});
  Globals.Cmd.setScissors(0, DkScissor{0, 0, Extent.w, Extent.h});
}

void RenderTextureAllocation::ProcessAttachArgs(const viewport &vp) noexcept {
  Globals.Cmd.setViewports(
      0, DkViewport{vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth});
  Globals.Cmd.setScissors(0,
                          DkScissor{uint32_t(vp.x), uint32_t(vp.y),
                                    uint32_t(vp.width), uint32_t(vp.height)});
}

void RenderTextureAllocation::ProcessAttachArgs(const scissor &s) noexcept {
  Globals.Cmd.setViewports(
      0, DkViewport{0.f, 0.f, float(Extent.w), float(Extent.h), 0.f, 1.f});
  Globals.Cmd.setScissors(0,
                          DkScissor{uint32_t(s.offset.x), uint32_t(s.offset.y),
                                    s.extent.w, s.extent.h});
}

void RenderTextureAllocation::ProcessAttachArgs(const viewport &vp,
                                                const scissor &s) noexcept {
  Globals.Cmd.setViewports(
      0, DkViewport{vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth});
  Globals.Cmd.setScissors(0,
                          DkScissor{uint32_t(s.offset.x), uint32_t(s.offset.y),
                                    s.extent.w, s.extent.h});
}

struct BufferImageCopy {
  uint32_t SrcOffset;
  uint32_t SrcRowLength;
  uint32_t SrcImageHeight;
  DkImageRect DstRec;
  uint8_t MipOffset;

  void Copy(DkGpuAddr BaseAddr, dk::ImageView &DstView) const noexcept {
    DstView.setMipLevels(MipOffset);
    deko::Globals.Cmd.copyBufferToImage(
        DkCopyBuf{BaseAddr + SrcOffset, SrcRowLength, SrcImageHeight}, DstView,
        DstRec);
  }
};
} // namespace hsh::detail::deko

namespace hsh::detail {
#ifndef NDEBUG
// TODO: Make this increment
constexpr uint64_t ExpectedScribble = 0xfefefefeefefefef;
#endif
template <> struct TargetTraits<Target::DEKO3D> {
  struct BufferWrapper {
    DkBufExtents Buffer{DK_GPU_ADDR_INVALID, 0};
    BufferWrapper() noexcept = default;
    BufferWrapper(const deko::BufferAllocation &Alloc) noexcept
        : Buffer(Alloc.GetBuffer()) {}
    bool IsValid() const noexcept { return Buffer.addr != DK_GPU_ADDR_INVALID; }
    operator const DkBufExtents &() const noexcept { return Buffer; }
  };
  using UniformBufferOwner = deko::BufferAllocation;
  using UniformBufferBinding = BufferWrapper;
  using DynamicUniformBufferOwner = deko::DynamicBufferAllocation;
  using VertexBufferOwner = deko::BufferAllocation;
  using VertexBufferBinding = BufferWrapper;
  using DynamicVertexBufferOwner = deko::DynamicBufferAllocation;
  using IndexBufferOwner = deko::BufferAllocation;
  using IndexBufferBinding = BufferWrapper;
  using DynamicIndexBufferOwner = deko::DynamicBufferAllocation;
  struct FifoOwner {
    std::unique_ptr<deko::FifoBufferAllocation> Allocation;
    FifoOwner() noexcept = default;
    FifoOwner(const FifoOwner &other) = delete;
    FifoOwner &operator=(const FifoOwner &other) = delete;
    FifoOwner(FifoOwner &&other) noexcept = default;
    FifoOwner &operator=(FifoOwner &&other) noexcept = default;
    explicit FifoOwner(std::unique_ptr<deko::FifoBufferAllocation> Allocation)
        : Allocation(std::move(Allocation)) {}
    template <typename T, typename Func>
    UniformBufferBinding MapUniform(Func func) noexcept {
      UniformBufferBinding Ret{*Allocation};
      uint32_t Offset = Allocation->MapUniform<T, Func>(func);
      Ret.Buffer.addr += Offset;
      return Ret;
    }
    template <typename T, typename Func>
    VertexBufferBinding MapVertex(std::size_t count, Func func) noexcept {
      VertexBufferBinding Ret{*Allocation};
      uint32_t Offset = Allocation->MapVertexIndex<T, Func>(count, func);
      Ret.Buffer.addr += Offset;
      return Ret;
    }
    template <typename T, typename Func>
    IndexBufferBinding MapIndex(std::size_t count, Func func) noexcept {
      IndexBufferBinding Ret{*Allocation};
      uint32_t Offset = Allocation->MapVertexIndex<T, Func>(count, func);
      Ret.Buffer.addr += Offset;
      return Ret;
    }
    bool IsValid() const noexcept { return Allocation.operator bool(); }
  };
  struct TextureOwner;
  struct TextureBinding {
    const TextureOwner *Owner = nullptr;
    bool IsValid() const noexcept { return Owner != nullptr; }
    inline const dk::ImageView &GetImageView() const noexcept;
  };
  struct TextureOwner {
#ifndef NDEBUG
    uint64_t Scribble = ExpectedScribble;
#endif
    deko::TextureAllocation Allocation;
    dk::ImageView ImageView{Allocation.GetImage()};

    TextureOwner() noexcept = default;
    TextureOwner(const TextureOwner &other) = delete;
    TextureOwner &operator=(const TextureOwner &other) = delete;
    TextureOwner(TextureOwner &&other) noexcept
        : Allocation(std::move(other.Allocation)), ImageView(other.ImageView) {
      ImageView.pImage = &Allocation.GetImage();
#ifndef NDEBUG
      other.Scribble = 0;
#endif
    }
    TextureOwner &operator=(TextureOwner &&other) noexcept {
      std::swap(Allocation, other.Allocation);
      ImageView = other.ImageView;
      ImageView.pImage = &Allocation.GetImage();
#ifndef NDEBUG
      other.Scribble = 0;
#endif
      return *this;
    }
    ~TextureOwner() noexcept {
#ifndef NDEBUG
      Scribble = 0;
#endif
    }

    TextureOwner(deko::TextureAllocation AllocationIn,
                 dk::ImageView ImageViewIn) noexcept
        : Allocation(std::move(AllocationIn)),
          ImageView(std::move(ImageViewIn)) {
      ImageView.pImage = &Allocation.GetImage();
    }

    bool IsValid() const noexcept { return Allocation.operator bool(); }

    TextureBinding GetBinding() const noexcept { return TextureBinding{this}; }
    operator TextureBinding() const noexcept { return GetBinding(); }
  };
  struct DynamicTextureOwner : TextureOwner {
    deko::UploadBufferAllocation UploadAllocation;
    std::array<deko::BufferImageCopy, MaxMipCount> Copies;

    DynamicTextureOwner(deko::TextureAllocation AllocationIn,
                        deko::UploadBufferAllocation UploadAllocation,
                        std::array<deko::BufferImageCopy, MaxMipCount> Copies,
                        dk::ImageView ImageViewIn) noexcept
        : TextureOwner(std::move(AllocationIn), std::move(ImageViewIn)),
          UploadAllocation(std::move(UploadAllocation)),
          Copies(std::move(Copies)) {}

    void MakeCopies() noexcept {
      dk::ImageView DstView{Allocation.GetImage()};
      for (auto &Copy : Copies)
        Copy.Copy(UploadAllocation.GetBuffer().addr, DstView);
    }

    void *Map() noexcept { return UploadAllocation.GetMappedData(); }
    void Unmap() noexcept {
      UploadAllocation.Flush();
      MakeCopies();
    }
  };
  struct RenderTextureBinding {
    deko::RenderTextureAllocation *Allocation = nullptr;
    uint32_t BindingIdx : 24;
    uint32_t IsDepth : 8;
    RenderTextureBinding() noexcept : BindingIdx(0), IsDepth(0) {}
    RenderTextureBinding(deko::RenderTextureAllocation *Allocation,
                         uint32_t BindingIdx, uint32_t IsDepth) noexcept
        : Allocation(Allocation), BindingIdx(BindingIdx), IsDepth(IsDepth) {}

    bool IsValid() const noexcept { return Allocation != nullptr; }

    const dk::ImageView &GetImageView() const noexcept {
      if (IsDepth)
        return Allocation->GetDepthBindingView(BindingIdx);
      else
        return Allocation->GetColorBindingView(BindingIdx);
    }
  };
  struct SurfaceBinding {
    deko::SurfaceAllocation *Allocation = nullptr;
    bool IsValid() const noexcept { return Allocation != nullptr; }
  };
  struct RenderTextureOwner {
    std::unique_ptr<deko::RenderTextureAllocation> Allocation;
    RenderTextureOwner() = default;
    RenderTextureOwner(const RenderTextureOwner &other) = delete;
    RenderTextureOwner &operator=(const RenderTextureOwner &other) = delete;
    RenderTextureOwner(RenderTextureOwner &&other) noexcept = default;
    RenderTextureOwner &
    operator=(RenderTextureOwner &&other) noexcept = default;

    explicit RenderTextureOwner(
        std::unique_ptr<deko::RenderTextureAllocation> Allocation) noexcept
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
      Allocation->ResolveSurface(surface.Allocation);
    }
    void ResolveColorBinding(uint32_t idx, rect2d region,
                             bool reattach) noexcept {
      Allocation->ResolveColorBinding(idx, region);
    }
    void ResolveDepthBinding(uint32_t idx, rect2d region,
                             bool reattach) noexcept {
      Allocation->ResolveDepthBinding(idx, region);
    }
  };
  struct SurfaceOwner {
    std::unique_ptr<deko::SurfaceAllocation> Allocation;
    SurfaceOwner() = default;
    SurfaceOwner(const SurfaceOwner &other) = delete;
    SurfaceOwner &operator=(const SurfaceOwner &other) = delete;
    SurfaceOwner(SurfaceOwner &&other) noexcept = default;
    SurfaceOwner &operator=(SurfaceOwner &&other) noexcept = default;

    explicit SurfaceOwner(
        std::unique_ptr<deko::SurfaceAllocation> Allocation) noexcept
        : Allocation(std::move(Allocation)) {}

    bool IsValid() const noexcept { return Allocation.operator bool(); }

    SurfaceBinding GetBinding() const noexcept {
      return SurfaceBinding{Allocation.get()};
    }
    operator SurfaceBinding() const noexcept { return GetBinding(); }
    bool AcquireNextImage() noexcept { return Allocation->AcquireNextImage(); }
    void AttachResizeLambda(
        std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
            &&Resize) noexcept {}
    void AttachDecorationLambda(std::function<void()> &&Dec) noexcept {}
    void AttachDeleterLambda(std::function<void()> &&Del) noexcept {
      Allocation->AttachDeleterLambda(std::move(Del));
    }
    void SetRequestExtent(const hsh::extent2d &Ext) noexcept {}
    void SetMargins(int32_t L, int32_t R, int32_t T, int32_t B) noexcept {}
  };
  struct Pipeline {
    uint32_t NumShaders = 0;
    std::array<const DkShader *, 5> Shaders;

    void Bind(uint32_t StageMask) const noexcept {
      deko::Globals.Cmd.bindShaders(StageMask, {NumShaders, Shaders.data()});
    }
  };
  struct PipelineBinding {
    const Pipeline *PipelineObj = nullptr;
    uint32_t StageMask = 0;
    DkPrimitive Primitive{};
    uint32_t NumVtxBufferStates = 0;
    uint32_t NumVtxAttribStates = 0;
    const DkVtxBufferState *VtxBufferStates = nullptr;
    const DkVtxAttribState *VtxAttribStates = nullptr;
    deko::DescriptorSetAllocation<dk::ImageDescriptor> ImageDescriptors;
    deko::DescriptorSetAllocation<dk::SamplerDescriptor> SamplerDescriptors;
    uint32_t NumUniformBuffers = 0;
    uint32_t NumTextures = 0;
    uint32_t NumVertexBuffers = 0;
    std::array<DkBufExtents, MaxUniforms> UniformBuffers{};
    std::array<DkResHandle, MaxImages> Textures{};
    std::array<DkBufExtents, MaxVertexBuffers> VertexBuffers{};
    struct BoundIndex {
      DkGpuAddr Buffer = DK_GPU_ADDR_INVALID;
      DkIdxFormat Type;
    } Index;
    uint32_t PatchSize = 0;
    uint32_t NumBlendStates = 0;
    const DkBlendState *BlendStates = nullptr;
    const DkRasterizerState *RasterizerState = nullptr;
    const DkDepthStencilState *DepthStencilState = nullptr;
    const DkColorState *ColorState = nullptr;
    const DkColorWriteState *ColorWriteState = nullptr;
    bool PrimitiveRestart = false;

    struct Iterators {
      decltype(UniformBuffers)::iterator UniformBufferBegin;
      decltype(UniformBuffers)::iterator UniformBufferIt;
      decltype(Textures)::iterator TextureBegin;
      decltype(Textures)::iterator TextureIt;
      decltype(VertexBuffers)::iterator VertexBufferBegin;
      decltype(VertexBuffers)::iterator VertexBufferIt;
      BoundIndex &Index;
      constexpr explicit Iterators(PipelineBinding &Binding) noexcept
          : UniformBufferBegin(Binding.UniformBuffers.begin()),
            UniformBufferIt(Binding.UniformBuffers.begin()),
            TextureBegin(Binding.Textures.begin()),
            TextureIt(Binding.Textures.begin()),
            VertexBufferBegin(Binding.VertexBuffers.begin()),
            VertexBufferIt(Binding.VertexBuffers.begin()),
            Index(Binding.Index) {}

      inline void Add(uniform_buffer_typeless uniform) noexcept;
      inline void Add(vertex_buffer_typeless uniform) noexcept;
      template <typename T> inline void Add(index_buffer<T> uniform) noexcept;
      static inline void Add(texture_typeless texture) noexcept;
      static inline void Add(render_texture2d texture) noexcept;
      inline void Add(SamplerBinding sampler) noexcept;
    };

    bool IsValid() const noexcept { return PipelineObj != nullptr; }

    PipelineBinding() noexcept = default;

    template <typename Impl, typename... Args>
    void Rebind(bool UpdateDescriptors, Args... args) noexcept;

    void Bind() noexcept {
      PipelineObj->Bind(StageMask);
      ImageDescriptors.Bind();
      SamplerDescriptors.Bind();
      for (DkStage s = DkStage_Vertex; s <= DkStage_Fragment;
           s = DkStage(int(s) + 1)) {
        if (StageMask & (1 << s)) {
          deko::Globals.Cmd.bindUniformBuffers(
              s, 0, {NumUniformBuffers, UniformBuffers.data()});
          deko::Globals.Cmd.bindTextures(s, 0, {NumTextures, Textures.data()});
        }
      }
      deko::Globals.Cmd.bindVtxBuffers(
          0, {NumVertexBuffers, VertexBuffers.data()});
      deko::Globals.Cmd.bindVtxBufferState(
          {NumVtxBufferStates, VtxBufferStates});
      deko::Globals.Cmd.bindVtxAttribState(
          {NumVtxAttribStates, VtxAttribStates});
      if (Index.Buffer != DK_GPU_ADDR_INVALID)
        deko::Globals.Cmd.bindIdxBuffer(Index.Type, Index.Buffer);
      if (Primitive == DkPrimitive_Patches)
        deko::Globals.Cmd.setPatchSize(PatchSize);
      deko::Globals.Cmd.bindBlendStates(0, {NumBlendStates, BlendStates});
      deko::Globals.Cmd.bindRasterizerState(*RasterizerState);
      deko::Globals.Cmd.bindDepthStencilState(*DepthStencilState);
      deko::Globals.Cmd.bindColorState(*ColorState);
      deko::Globals.Cmd.bindColorWriteState(*ColorWriteState);
      deko::Globals.Cmd.setPrimitiveRestart(PrimitiveRestart, UINT32_MAX);
    }

    void Draw(uint32_t start, uint32_t count) noexcept {
      Bind();
      deko::Globals.Cmd.draw(Primitive, count, 1, start, 0);
    }

    void DrawIndexed(uint32_t start, uint32_t count) noexcept {
      Bind();
      deko::Globals.Cmd.drawIndexed(Primitive, count, 1, start, 0, 0);
    }

    void DrawInstanced(uint32_t start, uint32_t count,
                       uint32_t instCount) noexcept {
      Bind();
      deko::Globals.Cmd.draw(Primitive, count, instCount, start, 0);
    }

    void DrawIndexedInstanced(uint32_t start, uint32_t count,
                              uint32_t instCount) noexcept {
      Bind();
      deko::Globals.Cmd.drawIndexed(Primitive, count, instCount, start, 0, 0);
    }
  };

  static void ClearAttachments(bool color, bool depth) noexcept {
    assert(deko::Globals.AttachedRenderTexture != nullptr);
    if (color)
      deko::Globals.Cmd.clearColor(0, DkColorMask_RGBA, 0, 0, 0, 0);
    if (depth)
      deko::Globals.Cmd.clearDepthStencil(true, 0.f, 0, 0);
  }

  static void SetBlendConstants(float red, float green, float blue,
                                float alpha) noexcept {
    deko::Globals.Cmd.setBlendConst(red, green, blue, alpha);
  }

  static void SetViewport(const viewport &vp) noexcept {
    Globals.Cmd.setViewports(0, DkViewport{vp.x, vp.y, vp.width, vp.height,
                                           vp.minDepth, vp.maxDepth});
    Globals.Cmd.setScissors(0,
                            DkScissor{uint32_t(vp.x), uint32_t(vp.y),
                                      uint32_t(vp.width), uint32_t(vp.height)});
  }

  static void SetViewport(const viewport &vp, const scissor &s) noexcept {
    Globals.Cmd.setViewports(0, DkViewport{vp.x, vp.y, vp.width, vp.height,
                                           vp.minDepth, vp.maxDepth});
    Globals.Cmd.setScissors(0, DkScissor{uint32_t(s.offset.x),
                                         uint32_t(s.offset.y), s.extent.w,
                                         s.extent.h});
  }

  static void SetScissor(const scissor &s) noexcept {
    Globals.Cmd.setScissors(0, DkScissor{uint32_t(s.offset.x),
                                         uint32_t(s.offset.y), s.extent.w,
                                         s.extent.h});
  }

  template <typename ResTp> struct ResourceFactory {};
};
const dk::ImageView &
TargetTraits<Target::DEKO3D>::TextureBinding::GetImageView() const noexcept {
  assert(Owner->Scribble == ExpectedScribble &&
         "TextureOwner was moved or destroyed after TextureBinding reference");
  return Owner->ImageView;
}
} // namespace hsh::detail

#endif
