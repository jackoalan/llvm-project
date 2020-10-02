#pragma once

#if HSH_ENABLE_METAL

#if defined(__MAC_11_0) || defined(__IPHONE_14_0) || defined(__TVOS_14_0)
#define HSH_METAL_BINARY_ARCHIVE 1
#define HSH_METAL_BINARY_ARCHIVE_AVAILABILITY                                  \
  @available(macOS 11.0, iOS 14, tvOS 14, *)
#endif

namespace hsh::detail::metal {
class BufferAllocation {
  friend class DeletedBufferAllocation;
  friend BufferAllocation AllocateStaticBuffer(const SourceLocation &location,
                                               NSUInteger size) noexcept;

protected:
  id<MTLBuffer> Buffer = nullptr;
  BufferAllocation(id<MTLBuffer> Buffer) noexcept : Buffer(Buffer) {}

public:
  BufferAllocation() noexcept = default;
  BufferAllocation(const BufferAllocation &other) = delete;
  BufferAllocation &operator=(const BufferAllocation &other) = delete;
  BufferAllocation(BufferAllocation &&other) noexcept {
    std::swap(Buffer, other.Buffer);
  }
  BufferAllocation &operator=(BufferAllocation &&other) noexcept {
    std::swap(Buffer, other.Buffer);
    return *this;
  }
  inline ~BufferAllocation() noexcept;
  id<MTLBuffer> GetBuffer() const noexcept { return Buffer; }
  operator id<MTLBuffer>() const noexcept { return GetBuffer(); }
  bool IsValid() const noexcept { return Buffer != nullptr; }
};

class UploadBufferAllocation : public BufferAllocation {
  friend UploadBufferAllocation
  AllocateUploadBuffer(const SourceLocation &location,
                       NSUInteger size) noexcept;
  void *MappedData;
  UploadBufferAllocation(id<MTLBuffer> BufferIn, void *MappedData) noexcept
      : BufferAllocation(BufferIn), MappedData(MappedData) {}

public:
  UploadBufferAllocation() noexcept = default;
  void *GetMappedData() const noexcept { return MappedData; }
};

class DynamicBufferAllocation : public BufferAllocation {
  UploadBufferAllocation UploadBuffer;
  NSUInteger Size;
  friend DynamicBufferAllocation
  AllocateDynamicBuffer(const SourceLocation &location,
                        NSUInteger size) noexcept;
  DynamicBufferAllocation(id<MTLBuffer> BufferIn, NSUInteger Size,
                          UploadBufferAllocation UploadBuffer) noexcept
      : BufferAllocation(BufferIn), UploadBuffer(std::move(UploadBuffer)),
        Size(Size) {}

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
  friend std::unique_ptr<class FifoBufferAllocation>
  AllocateFifoBuffer(const SourceLocation &location, NSUInteger size) noexcept;
  inline FifoBufferAllocation(id<MTLBuffer> BufferIn, NSUInteger Size,
                              void *MappedData) noexcept;

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
                  MTLTextureDescriptor *CreateInfo) noexcept;

protected:
  id<MTLTexture> Texture = nullptr;
  TextureAllocation(id<MTLTexture> Texture) noexcept : Texture(Texture) {}

public:
  TextureAllocation() noexcept = default;
  TextureAllocation(const TextureAllocation &other) = delete;
  TextureAllocation &operator=(const TextureAllocation &other) = delete;
  TextureAllocation(TextureAllocation &&other) noexcept {
    std::swap(Texture, other.Texture);
  }
  TextureAllocation &operator=(TextureAllocation &&other) noexcept {
    std::swap(Texture, other.Texture);
    return *this;
  }
  inline ~TextureAllocation() noexcept;
  id<MTLTexture> GetTexture() const noexcept { return Texture; }
};

struct RenderPassBeginInfo {
  MTLRenderPassDescriptor *RenderPassDescriptor;
  RenderPassBeginInfo() = default;
  RenderPassBeginInfo(MTLRenderPassDescriptor *RenderPassDescriptor) noexcept
      : RenderPassDescriptor(RenderPassDescriptor) {}
};

class SurfaceAllocation {
  friend class DeletedSurfaceAllocation;
  friend class RenderTextureAllocation;
  friend class DeletedSurfaceSwapchainImage;
  friend class DeletedResources;
  SurfaceAllocation *Prev = nullptr, *Next = nullptr;
  SourceLocation Location;
  CAMetalLayer *MetalLayer;
  hsh::extent2d Extent;
  int32_t MarginL = 0, MarginR = 0, MarginT = 0, MarginB = 0;
  MTLPixelFormat SurfaceFormat;
  id<CAMetalDrawable> NextImage = nullptr;
  std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
      ResizeLambda;
  std::function<void()> DecorationLambda;
  std::function<void()> DeleterLambda;

  inline MTLViewport ProcessMargins(MTLViewport vp) noexcept;
  inline MTLScissorRect ProcessMargins(MTLScissorRect s) noexcept;

public:
  inline ~SurfaceAllocation() noexcept;
  inline explicit SurfaceAllocation(
      const SourceLocation &location, CAMetalLayer *MetalLayer,
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
  inline void PostRender() noexcept;
  inline hsh::extent2d ContentExtent() const noexcept;
  inline void DrawDecorations() noexcept;
  void SetMargins(int32_t L, int32_t R, int32_t T, int32_t B) noexcept {
    MarginL = L;
    MarginR = R;
    MarginT = T;
    MarginB = B;
  }
  const hsh::extent2d &GetExtent() const noexcept { return Extent; }
  MTLPixelFormat GetColorFormat() const noexcept { return SurfaceFormat; }
  SurfaceAllocation *GetNext() const noexcept { return Next; }
};

class RenderTextureAllocation {
  friend class DeletedRenderTextureAllocation;
  friend class MetalGlobals;
  RenderTextureAllocation *Prev = nullptr, *Next = nullptr;
  SourceLocation Location;
  id<MTLRenderCommandEncoder> Cmd = nullptr;
  MTLViewport Viewport{};
  MTLScissorRect ScissorRect{};
  SurfaceAllocation *Surface = nullptr;
  hsh::extent2d Extent;
  MTLPixelFormat ColorFormat = MTLPixelFormatInvalid;
  uint32_t NumColorBindings, NumDepthBindings;
  TextureAllocation ColorTexture;
  TextureAllocation DepthTexture;
  MTLRenderPassDescriptor *RenderPassBegin;
  struct Binding {
    TextureAllocation Texture;
  };
  std::array<Binding, MaxRenderTextureBindings> ColorBindings;
  std::array<Binding, MaxRenderTextureBindings> DepthBindings;
  inline void Prepare() noexcept;
  template <bool Depth>
  static inline void _Resolve(id<MTLTexture> SrcImage, id<MTLTexture> DstImage,
                              MTLOrigin SrcOffset, MTLOrigin DstOffset,
                              MTLSize Extent) noexcept;
  template <bool Depth>
  inline void Resolve(id<MTLTexture> SrcImage, id<MTLTexture> DstImage,
                      MTLOrigin Offset, MTLSize Extent, bool Reattach) noexcept;

public:
  inline ~RenderTextureAllocation() noexcept;
  inline explicit RenderTextureAllocation(const SourceLocation &location,
                                          SurfaceAllocation *Surface,
                                          uint32_t NumColorBindings,
                                          uint32_t NumDepthBindings) noexcept;
  inline explicit RenderTextureAllocation(const SourceLocation &location,
                                          extent2d extent,
                                          MTLPixelFormat colorFormat,
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
  inline id<MTLRenderCommandEncoder> GetCmd() noexcept;
  inline void BeginRenderPass() noexcept;
  inline void EndRenderPass() noexcept;
  inline void ResolveSurface(SurfaceAllocation *Surface,
                             bool Reattach) noexcept;
  inline void ResolveColorBinding(uint32_t Idx, rect2d Region,
                                  bool Reattach) noexcept;
  inline void ResolveDepthBinding(uint32_t Idx, rect2d Region,
                                  bool Reattach) noexcept;
  inline void Clear(bool Color, bool Depth) noexcept;
  id<MTLTexture> GetColorBindingTexture(uint32_t Idx) const noexcept {
    assert(Idx < NumColorBindings);
    return ColorBindings[Idx].Texture.GetTexture();
  }
  id<MTLTexture> GetDepthBindingTexture(uint32_t Idx) const noexcept {
    assert(Idx < NumDepthBindings);
    return DepthBindings[Idx].Texture.GetTexture();
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
  id<MTLBuffer> Buffer = nullptr;

public:
  explicit DeletedBufferAllocation(BufferAllocation &&Obj) noexcept {
    std::swap(Buffer, Obj.Buffer);
  }
  DeletedBufferAllocation &operator=(BufferAllocation &&Obj) noexcept {
    std::swap(Buffer, Obj.Buffer);
    return *this;
  }
  DeletedBufferAllocation(const DeletedBufferAllocation &other) = delete;
  DeletedBufferAllocation &
  operator=(const DeletedBufferAllocation &other) = delete;
  DeletedBufferAllocation(DeletedBufferAllocation &&other) noexcept {
    std::swap(Buffer, other.Buffer);
  }
  DeletedBufferAllocation &operator=(DeletedBufferAllocation &&other) noexcept {
    std::swap(Buffer, other.Buffer);
    return *this;
  }
};

class DeletedTextureAllocation {
  id<MTLTexture> Texture = nullptr;

public:
  DeletedTextureAllocation(TextureAllocation &&other) noexcept {
    std::swap(Texture, other.Texture);
  }
  DeletedTextureAllocation(const DeletedTextureAllocation &other) = delete;
  DeletedTextureAllocation &
  operator=(const DeletedTextureAllocation &other) = delete;
  DeletedTextureAllocation(DeletedTextureAllocation &&other) noexcept {
    std::swap(Texture, other.Texture);
  }
  DeletedTextureAllocation &
  operator=(DeletedTextureAllocation &&other) noexcept {
    std::swap(Texture, other.Texture);
    return *this;
  }
};

class DeletedSurfaceAllocation {
  CAMetalLayer *MetalLayer;
  std::function<void()> DeleterLambda;

public:
  explicit DeletedSurfaceAllocation(SurfaceAllocation &&Obj) noexcept
      : MetalLayer(std::move(Obj.MetalLayer)),
        DeleterLambda(std::move(Obj.DeleterLambda)) {}

  ~DeletedSurfaceAllocation() noexcept {
    MetalLayer = nullptr;
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
  DeletedTextureAllocation ColorTexture;
  DeletedTextureAllocation DepthTexture;
  struct Binding {
    DeletedTextureAllocation Texture;
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
  std::vector<DeletedBufferAllocation> Buffers;
  std::vector<DeletedTextureAllocation> Textures;
  std::vector<DeletedSurfaceAllocation> Surfaces;
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
  void DeleteLater(RenderTextureAllocation &&Obj) noexcept {
    RenderTextures.emplace_back(std::move(Obj));
  }
  void Purge() noexcept {
    Buffers.clear();
    Textures.clear();
    Surfaces.clear();
    RenderTextures.clear();
  }
  DeletedResources() noexcept = default;
  DeletedResources(const DeletedResources &) = delete;
  DeletedResources &operator=(const DeletedResources &) = delete;
};

using ErrorHandler = std::function<void(NSError *)>;

inline void WaitUntilCompleted(id<MTLCommandBuffer> CommandBuffer) noexcept {
  MTLCommandBufferStatus Status = CommandBuffer.status;
  if (Status == MTLCommandBufferStatusCommitted ||
      Status == MTLCommandBufferStatusScheduled)
    [CommandBuffer waitUntilCompleted];
}

struct MetalGlobals {
  ErrorHandler *ErrHandler = nullptr;
  id<MTLDevice> Device = nullptr;
#if HSH_METAL_BINARY_ARCHIVE
  id PipelineCache = nullptr;
#endif
  uint32_t SampleCount = 1;
  float Anisotropy = 1.f;
  MTLPixelFormat TargetPixelFormat = MTLPixelFormatInvalid;
  id<MTLCommandQueue> CommandQueue = nullptr;
  std::array<id<MTLCommandBuffer>, 2> CommandBuffers{};
  id<MTLCommandBuffer> CopyCommandBuffer = nullptr;
  id<MTLBlitCommandEncoder> CopyCmd = nullptr;
#if HSH_ENABLE_METAL_BIN_MAC
  static constexpr NSUInteger UniformOffsetAlignment = 256;
#elif HSH_ENABLE_METAL_BIN_IOS
  static constexpr NSUInteger UniformOffsetAlignment = 16;
#endif
  id<MTLRenderPipelineState> BoundPipeline = nullptr;
  RenderTextureAllocation *AttachedRenderTexture = nullptr;
  id<MTLRenderCommandEncoder> OverrideCmd = nullptr;
  uint64_t Frame = 0;
  bool AcquiredImage = false;

  std::array<DeletedResources, 2> *DeletedResourcesArr = nullptr;
  DeletedResources *DeletedResources = nullptr;
  SurfaceAllocation *SurfaceHead = nullptr;
  RenderTextureAllocation *RenderTextureHead = nullptr;
  FifoBufferAllocation *FifoBufferHead = nullptr;

  std::array<std::array<id<MTLDepthStencilState>, 2>, 8> DepthStencilStates;
  id<MTLDepthStencilState>
  GetDepthStencilState(MTLCompareFunction Compare,
                       bool DepthWriteEnabled) noexcept {
    if (!DepthStencilStates[Compare][DepthWriteEnabled]) {
      MTLDepthStencilDescriptor *Descriptor = [MTLDepthStencilDescriptor new];
      Descriptor.depthCompareFunction = Compare;
      Descriptor.depthWriteEnabled = DepthWriteEnabled;
      DepthStencilStates[Compare][DepthWriteEnabled] =
          [Device newDepthStencilStateWithDescriptor:Descriptor];
    }
    return DepthStencilStates[Compare][DepthWriteEnabled];
  }

  void PreRender() noexcept {
    assert(CopyCommandBuffer == nullptr &&
           "previous render pass not completed");
    uint32_t CurBufferIdx = Frame & 1u;
    if (CommandBuffers[CurBufferIdx]) {
      WaitUntilCompleted(CommandBuffers[CurBufferIdx]);
      CommandBuffers[CurBufferIdx] = nullptr;
    }
    DeletedResources = &(*DeletedResourcesArr)[CurBufferIdx];
    DeletedResources->Purge();

    for (auto *Surf = SurfaceHead; Surf; Surf = Surf->GetNext())
      Surf->PreRender();
    for (auto *RT = RenderTextureHead; RT; RT = RT->GetNext())
      RT->PreRender();
  }

  void PostRender() noexcept {
    uint32_t CurBufferIdx = Frame & 1u;
    BoundPipeline = nullptr;
    [CopyCmd endEncoding];
    CopyCmd = nullptr;
    if (AttachedRenderTexture) {
      AttachedRenderTexture->EndRenderPass();
      AttachedRenderTexture = nullptr;
    }

    for (auto *Fifo = FifoBufferHead; Fifo; Fifo = Fifo->GetNext())
      Fifo->PostRender();
    for (auto *Surf = SurfaceHead; Surf; Surf = Surf->GetNext())
      Surf->PostRender();

    id<MTLCommandBuffer> CommandBuffer = CommandBuffers[CurBufferIdx];
    [CopyCommandBuffer enqueue];
    auto Status1 = CommandBuffer.status;
    [CommandBuffer enqueue];
    auto Status2 = CommandBuffer.status;

    [CopyCommandBuffer commit];
    [CommandBuffer commit];
    auto Status3 = CommandBuffer.status;

    AcquiredImage = false;
    WaitUntilCompleted(CopyCommandBuffer);
    CopyCommandBuffer = nullptr;
    ++Frame;
  }

  id<MTLCommandBuffer> GetCmdBufferIfExists() noexcept {
    uint32_t CurBufferIdx = Frame & 1u;
    return CommandBuffers[CurBufferIdx];
  }

  id<MTLCommandBuffer> GetCmdBuffer() noexcept {
    uint32_t CurBufferIdx = Frame & 1u;
    if (!CommandBuffers[CurBufferIdx])
      CommandBuffers[CurBufferIdx] =
          [CommandQueue commandBufferWithUnretainedReferences];
    return CommandBuffers[CurBufferIdx];
  }

  id<MTLRenderCommandEncoder> GetCmd() noexcept {
    if (OverrideCmd)
      return OverrideCmd;
    assert(AttachedRenderTexture && "No attached render texture");
    return AttachedRenderTexture->GetCmd();
  }

  id<MTLBlitCommandEncoder> GetCopyCmd() noexcept {
    if (!CopyCommandBuffer)
      CopyCommandBuffer = [CommandQueue commandBufferWithUnretainedReferences];
    if (!CopyCmd)
      CopyCmd = [CopyCommandBuffer blitCommandEncoder];
    return CopyCmd;
  }

  void waitIdle() noexcept {
    detail::metal::WaitUntilCompleted(CommandBuffers[0]);
    detail::metal::WaitUntilCompleted(CommandBuffers[1]);
    detail::metal::WaitUntilCompleted(CopyCommandBuffer);
  }
};
inline MetalGlobals Globals;

#if HSH_METAL_BINARY_ARCHIVE

template <typename Mgr> inline id CreatePipelineCache(Mgr &M) noexcept {
  if (HSH_METAL_BINARY_ARCHIVE_AVAILABILITY) {
    if ([MTLBinaryArchiveDescriptor
            instancesRespondToSelector:@selector(setUrl:)] &&
        [metal::Globals.Device respondsToSelector:@selector
                               (newBinaryArchiveWithDescriptor:error:)]) {
      MTLBinaryArchiveDescriptor *Descriptor = [MTLBinaryArchiveDescriptor new];
      M.ReadPipelineCache([&](NSURL *url) { Descriptor.url = url; });
      NSError *Error;
      id Archive =
          [metal::Globals.Device newBinaryArchiveWithDescriptor:Descriptor
                                                          error:&Error];
      if (!Archive)
        (*metal::Globals.ErrHandler)(Error);
      return Archive;
    }
  }
  return nullptr;
}

template <typename Mgr> inline void WritePipelineCache(Mgr &M) noexcept {
  if (HSH_METAL_BINARY_ARCHIVE_AVAILABILITY) {
    if ([Globals.PipelineCache respondsToSelector:@selector(serializeToURL:
                                                                     error:)]) {
      M.WritePipelineCache([&](NSURL *url) {
        [Globals.PipelineCache serializeToURL:url error:nullptr];
      });
    }
  }
}

#else

template <typename Mgr> inline id CreatePipelineCache(Mgr &M) noexcept {
  return nullptr;
}

template <typename Mgr> inline void WritePipelineCache(Mgr &M) noexcept {}

#endif

inline MTLViewport SurfaceAllocation::ProcessMargins(MTLViewport vp) noexcept {
  vp.originX += MarginL;
  vp.originY += MarginT;
  vp.width -= MarginL + MarginR;
  vp.height -= MarginT + MarginB;
  return vp;
}

inline MTLScissorRect
SurfaceAllocation::ProcessMargins(MTLScissorRect s) noexcept {
  s.x += MarginL;
  s.y += MarginT;
  s.width -= MarginL + MarginR;
  s.height -= MarginT + MarginB;
  return s;
}

bool SurfaceAllocation::PreRender() noexcept {
  CGFloat ContentScale = MetalLayer.contentsScale;
  hsh::extent2d CurrentExtent = MetalLayer.bounds.size;
  CurrentExtent.w *= ContentScale;
  CurrentExtent.h *= ContentScale;
  if (CurrentExtent != Extent) {
    MetalLayer.drawableSize = CurrentExtent;
    Extent = CurrentExtent;
    if (ResizeLambda)
      ResizeLambda(Extent, ContentExtent());
    return true;
  }
  return false;
}

bool SurfaceAllocation::AcquireNextImage() noexcept {
  NextImage = [MetalLayer nextDrawable];
  if (NextImage) {
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

void SurfaceAllocation::PostRender() noexcept {
  if (NextImage) {
    if (Globals.AcquiredImage)
      [Globals.GetCmdBufferIfExists() presentDrawable:NextImage];
    NextImage = nullptr;
  }
}

hsh::extent2d SurfaceAllocation::ContentExtent() const noexcept {
  return hsh::extent2d{
      uint32_t(std::max(1, int32_t(Extent.w) - MarginL - MarginR)),
      uint32_t(std::max(1, int32_t(Extent.h) - MarginT - MarginB))};
}

void SurfaceAllocation::DrawDecorations() noexcept {
  if (DecorationLambda) {
    MTLRenderPassDescriptor *PassDescriptor =
        [MTLRenderPassDescriptor renderPassDescriptor];
    PassDescriptor.colorAttachments[0].texture = NextImage.texture;
    id<MTLRenderCommandEncoder> Cmd = [Globals.GetCmdBuffer()
        renderCommandEncoderWithDescriptor:PassDescriptor];
    [Cmd setViewport:MTLViewport{0.0, 0.0, double(Extent.w), double(Extent.h),
                                 0.0, 1.0}];
    [Cmd setScissorRect:MTLScissorRect{0, 0, NSUInteger(Extent.w),
                                       NSUInteger(Extent.h)}];
    Globals.OverrideCmd = Cmd;
    DecorationLambda();
    Globals.OverrideCmd = nullptr;
    [Cmd endEncoding];
  }
}

BufferAllocation::~BufferAllocation() noexcept {
  if (Buffer)
    Globals.DeletedResources->DeleteLater(std::move(*this));
}

TextureAllocation::~TextureAllocation() noexcept {
  if (Texture)
    Globals.DeletedResources->DeleteLater(std::move(*this));
}

SurfaceAllocation::~SurfaceAllocation() noexcept {
  if (Prev) {
    Prev->Next = Next;
  } else {
    Globals.SurfaceHead = Next;
  }
  if (Next)
    Next->Prev = Prev;
  Globals.DeletedResources->DeleteLater(std::move(*this));
}

SurfaceAllocation::SurfaceAllocation(
    const SourceLocation &location, CAMetalLayer *InMetalLayer,
    std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
        &&ResizeLambda,
    std::function<void()> &&DeleterLambda, const hsh::extent2d &RequestExtent,
    int32_t L, int32_t R, int32_t T, int32_t B) noexcept
    : Next(Globals.SurfaceHead), Location(location),
      MetalLayer(std::move(InMetalLayer)), MarginL(L), MarginR(R), MarginT(T),
      MarginB(B), ResizeLambda(std::move(ResizeLambda)),
      DeleterLambda(std::move(DeleterLambda)) {
  if (!Globals.SurfaceHead)
    Globals.TargetPixelFormat = MetalLayer.pixelFormat;
  Globals.SurfaceHead = this;
  if (Next)
    Next->Prev = this;

  SurfaceFormat = MetalLayer.pixelFormat;
  MetalLayer.device = metal::Globals.Device;
  MetalLayer.framebufferOnly = NO;
  assert(SurfaceFormat != MTLPixelFormatInvalid);
  assert((!Next || SurfaceFormat == Next->SurfaceFormat) &&
         "Subsequent surfaces must have the same color format");

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
    const SourceLocation &location, extent2d extent, MTLPixelFormat colorFormat,
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
  [Globals.GetCopyCmd() copyFromBuffer:UploadBuffer.GetBuffer()
                          sourceOffset:0
                              toBuffer:GetBuffer()
                     destinationOffset:0
                                  size:Size];
}

FifoBufferAllocation::FifoBufferAllocation(id<MTLBuffer> BufferIn,
                                           NSUInteger Size,
                                           void *MappedData) noexcept
    : BufferAllocation(BufferIn), Next(Globals.FifoBufferHead),
      MappedData(MappedData), Size(uint32_t(Size)) {
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
#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
  [GetBuffer() didModifyRange:NSMakeRange(StartOffset, Offset - StartOffset)];
#endif
  if (StartOffset) {
    Offset = 0;
    StartOffset = 0;
  } else {
    StartOffset = Offset;
  }
}

inline UploadBufferAllocation
AllocateUploadBuffer(const SourceLocation &location, NSUInteger size) noexcept {
  id<MTLBuffer> Buffer =
      [Globals.Device newBufferWithLength:size
                                  options:MTLResourceCPUCacheModeDefaultCache |
                                          MTLResourceStorageModeShared];
#if HSH_SOURCE_LOCATION_ENABLED
  Buffer.label = @((location.to_string() + "Upload").c_str());
#endif
  return UploadBufferAllocation(Buffer, Buffer.contents);
}

inline BufferAllocation AllocateStaticBuffer(const SourceLocation &location,
                                             NSUInteger size) noexcept {
  id<MTLBuffer> Buffer =
      [Globals.Device newBufferWithLength:size
                                  options:MTLResourceCPUCacheModeDefaultCache |
                                          MTLResourceStorageModePrivate];
#if HSH_SOURCE_LOCATION_ENABLED
  Buffer.label = @(location.to_string().c_str());
#endif
  return BufferAllocation(Buffer);
}

inline DynamicBufferAllocation
AllocateDynamicBuffer(const SourceLocation &location,
                      NSUInteger size) noexcept {
  id<MTLBuffer> Buffer =
      [Globals.Device newBufferWithLength:size
                                  options:MTLResourceCPUCacheModeDefaultCache |
                                          MTLResourceStorageModePrivate];
#if HSH_SOURCE_LOCATION_ENABLED
  Buffer.label = @(location.to_string().c_str());
#endif
  return DynamicBufferAllocation(Buffer, size,
                                 AllocateUploadBuffer(location, size));
}

inline std::unique_ptr<FifoBufferAllocation>
AllocateFifoBuffer(const SourceLocation &location, NSUInteger size) noexcept {
  id<MTLBuffer> Buffer =
      [Globals.Device newBufferWithLength:size
                                  options:MTLResourceCPUCacheModeDefaultCache |
#if HSH_ENABLE_METAL_BIN_MAC
                                          MTLResourceStorageModeManaged
#else
                                          MTLResourceStorageModeShared
#endif
  ];
#if HSH_SOURCE_LOCATION_ENABLED
  Buffer.label = @(location.to_string().c_str());
#endif
  return std::unique_ptr<FifoBufferAllocation>{
      new FifoBufferAllocation(Buffer, size, Buffer.contents)};
}

inline TextureAllocation
AllocateTexture(const SourceLocation &location,
                MTLTextureDescriptor *CreateInfo) noexcept {
  id<MTLTexture> Texture = [Globals.Device newTextureWithDescriptor:CreateInfo];
  return TextureAllocation(Texture);
}

void RenderTextureAllocation::Prepare() noexcept {
  WaitUntilCompleted(Globals.GetCmdBufferIfExists());
  MTLTextureDescriptor *ColorCreateInfo =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:ColorFormat
                                                         width:Extent.w
                                                        height:Extent.h
                                                     mipmapped:NO];
  ColorCreateInfo.storageMode = MTLStorageModePrivate;
  ColorCreateInfo.sampleCount = Globals.SampleCount;
  ColorCreateInfo.storageMode = MTLStorageModePrivate;
  ColorCreateInfo.usage = MTLTextureUsageRenderTarget;
  ColorTexture =
      AllocateTexture(Location.with_field("ColorTexture"), ColorCreateInfo);
  MTLTextureDescriptor *DepthCreateInfo = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                   width:Extent.w
                                  height:Extent.h
                               mipmapped:NO];
  DepthCreateInfo.storageMode = MTLStorageModePrivate;
  DepthCreateInfo.sampleCount = Globals.SampleCount;
  DepthCreateInfo.storageMode = MTLStorageModePrivate;
  DepthCreateInfo.usage = MTLTextureUsageRenderTarget;
  DepthTexture =
      AllocateTexture(Location.with_field("DepthTexture"), DepthCreateInfo);
  RenderPassBegin = [MTLRenderPassDescriptor renderPassDescriptor];
  MTLRenderPassColorAttachmentDescriptor *ColorAttachment =
      RenderPassBegin.colorAttachments[0];
  ColorAttachment.texture = ColorTexture.GetTexture();
  ColorAttachment.loadAction = MTLLoadActionLoad;
  ColorAttachment.storeAction = MTLStoreActionStore;
  MTLRenderPassDepthAttachmentDescriptor *DepthAttachment =
      RenderPassBegin.depthAttachment;
  DepthAttachment.texture = DepthTexture.GetTexture();
  DepthAttachment.loadAction = MTLLoadActionLoad;
  DepthAttachment.storeAction = MTLStoreActionStore;
  ColorCreateInfo.sampleCount = 1;
  ColorCreateInfo.usage = MTLTextureUsageShaderRead;
  DepthCreateInfo.sampleCount = 1;
  DepthCreateInfo.usage = MTLTextureUsageShaderRead;
  for (uint32_t i = 0; i < NumColorBindings; ++i) {
    ColorBindings[i].Texture = AllocateTexture(
        Location.with_field("ColorBindings", i), ColorCreateInfo);
  }
  for (uint32_t i = 0; i < NumDepthBindings; ++i) {
    DepthBindings[i].Texture = AllocateTexture(
        Location.with_field("DepthBindings", i), DepthCreateInfo);
  }
}

inline id<MTLRenderCommandEncoder> RenderTextureAllocation::GetCmd() noexcept {
  if (!Cmd)
    BeginRenderPass();
  return Cmd;
}

void RenderTextureAllocation::BeginRenderPass() noexcept {
  [Cmd endEncoding];
  Cmd = [Globals.GetCmdBuffer()
      renderCommandEncoderWithDescriptor:RenderPassBegin];
  [Cmd setViewport:Viewport];
  [Cmd setScissorRect:ScissorRect];
}

void RenderTextureAllocation::EndRenderPass() noexcept {
  [Cmd endEncoding];
  Cmd = nullptr;
}

template <bool Depth>
void RenderTextureAllocation::_Resolve(id<MTLTexture> SrcImage,
                                       id<MTLTexture> DstImage,
                                       MTLOrigin SrcOffset, MTLOrigin DstOffset,
                                       MTLSize Extent) noexcept {
  if (Globals.SampleCount > 1) {
    MTLRenderPassDescriptor *RenderPass =
        [MTLRenderPassDescriptor renderPassDescriptor];
    MTLRenderPassAttachmentDescriptor *Attachment;
    if constexpr (Depth)
      Attachment = RenderPass.depthAttachment;
    else
      Attachment = RenderPass.colorAttachments[0];
    Attachment.texture = SrcImage;
    Attachment.resolveTexture = DstImage;
    Attachment.loadAction = MTLLoadActionLoad;
    Attachment.storeAction = MTLStoreActionMultisampleResolve;
    [[Globals.GetCmdBuffer() renderCommandEncoderWithDescriptor:RenderPass]
        endEncoding];
  } else {
    id<MTLBlitCommandEncoder> BlitEncoder =
        [Globals.GetCmdBuffer() blitCommandEncoder];
    [BlitEncoder copyFromTexture:SrcImage
                     sourceSlice:0
                     sourceLevel:0
                    sourceOrigin:SrcOffset
                      sourceSize:Extent
                       toTexture:DstImage
                destinationSlice:0
                destinationLevel:0
               destinationOrigin:DstOffset];
    [BlitEncoder endEncoding];
  }
}

template <bool Depth>
void RenderTextureAllocation::Resolve(id<MTLTexture> SrcImage,
                                      id<MTLTexture> DstImage, MTLOrigin Offset,
                                      MTLSize ExtentIn,
                                      bool Reattach) noexcept {
  bool DelimitRenderPass = this == Globals.AttachedRenderTexture;
  if (DelimitRenderPass)
    EndRenderPass();

  _Resolve<Depth>(SrcImage, DstImage, Offset, Offset, ExtentIn);

  if (DelimitRenderPass && !Reattach)
    Globals.AttachedRenderTexture = nullptr;
}

void RenderTextureAllocation::ResolveSurface(SurfaceAllocation *Surface,
                                             bool Reattach) noexcept {
  assert(Surface->NextImage &&
         "acquireNextImage not called on surface for this frame");
  assert(Surface->ContentExtent() == Extent &&
         "Mismatched render texture / surface extents");
  bool DelimitRenderPass = this == Globals.AttachedRenderTexture;
  if (DelimitRenderPass)
    EndRenderPass();
  auto DstImage = Surface->NextImage;
  MTLOrigin DestOff{};
  if (int32_t(Surface->Extent.w) > Surface->MarginL + Surface->MarginR &&
      int32_t(Surface->Extent.h) > Surface->MarginT + Surface->MarginB)
    DestOff = MTLOrigin{NSUInteger(Surface->MarginL),
                        NSUInteger(Surface->MarginT), 0};
  _Resolve<false>(ColorTexture.GetTexture(), DstImage.texture, MTLOrigin{},
                  DestOff, Extent);
  Surface->DrawDecorations();
  if (DelimitRenderPass && !Reattach)
    Globals.AttachedRenderTexture = nullptr;
}

void RenderTextureAllocation::ResolveColorBinding(uint32_t Idx, rect2d region,
                                                  bool Reattach) noexcept {
  assert(Idx < NumColorBindings);
  Resolve<false>(ColorTexture.GetTexture(),
                 ColorBindings[Idx].Texture.GetTexture(), region.offset,
                 region.extent, Reattach);
}

void RenderTextureAllocation::ResolveDepthBinding(uint32_t Idx, rect2d region,
                                                  bool Reattach) noexcept {
  assert(Idx < NumDepthBindings);
  Resolve<true>(DepthTexture.GetTexture(),
                DepthBindings[Idx].Texture.GetTexture(), region.offset,
                region.extent, Reattach);
}

void RenderTextureAllocation::Clear(bool Color, bool Depth) noexcept {
  EndRenderPass();

  MTLRenderPassDescriptor *RenderPass =
      [MTLRenderPassDescriptor renderPassDescriptor];
  if (Color) {
    MTLRenderPassColorAttachmentDescriptor *Attachment =
        RenderPass.colorAttachments[0];
    Attachment.texture = ColorTexture.GetTexture();
    Attachment.loadAction = MTLLoadActionClear;
    Attachment.storeAction = MTLStoreActionStore;
    Attachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
  }
  if (Depth) {
    MTLRenderPassDepthAttachmentDescriptor *Attachment =
        RenderPass.depthAttachment;
    Attachment.texture = DepthTexture.GetTexture();
    Attachment.loadAction = MTLLoadActionClear;
    Attachment.storeAction = MTLStoreActionStore;
    Attachment.clearDepth = 0.0;
  }
  [[Globals.GetCmdBuffer() renderCommandEncoderWithDescriptor:RenderPass]
      endEncoding];
}

template <typename... Args>
void RenderTextureAllocation::Attach(const Args &... args) noexcept {
  if (Globals.AttachedRenderTexture == this)
    return;
  if (Globals.AttachedRenderTexture)
    Globals.AttachedRenderTexture->EndRenderPass();
  Globals.AttachedRenderTexture = this;
  ProcessAttachArgs(args...);
}

void RenderTextureAllocation::ProcessAttachArgs() noexcept {
  Viewport =
      MTLViewport{0.0, 0.0, double(Extent.w), double(Extent.h), 0.0, 1.0};
  ScissorRect =
      MTLScissorRect{0, 0, NSUInteger(Extent.w), NSUInteger(Extent.h)};
}

void RenderTextureAllocation::ProcessAttachArgs(const viewport &vp) noexcept {
  Viewport =
      MTLViewport{vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
  ScissorRect = MTLScissorRect{NSUInteger(vp.x), NSUInteger(vp.y),
                               NSUInteger(vp.width), NSUInteger(vp.height)};
}

void RenderTextureAllocation::ProcessAttachArgs(const scissor &s) noexcept {
  Viewport =
      MTLViewport{0.0, 0.0, double(Extent.w), double(Extent.h), 0.0, 1.0};
  ScissorRect = MTLScissorRect{NSUInteger(s.offset.x), NSUInteger(s.offset.y),
                               NSUInteger(s.extent.w), NSUInteger(s.extent.h)};
}

void RenderTextureAllocation::ProcessAttachArgs(const viewport &vp,
                                                const scissor &s) noexcept {
  Viewport =
      MTLViewport{vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
  ScissorRect = MTLScissorRect{NSUInteger(s.offset.x), NSUInteger(s.offset.y),
                               NSUInteger(s.extent.w), NSUInteger(s.extent.h)};
}

struct BufferImageCopy {
  NSUInteger sourceOffset;
  NSUInteger sourceBytesPerRow;
  NSUInteger sourceBytesPerImage;
  MTLSize sourceSize;
  NSUInteger destinationSlice;
  NSUInteger destinationLevel;

  void Copy(id<MTLBuffer> SrcBuffer, id<MTLTexture> DstTexture) const noexcept {
    [metal::Globals.GetCopyCmd() copyFromBuffer:SrcBuffer
                                   sourceOffset:sourceOffset
                              sourceBytesPerRow:sourceBytesPerRow
                            sourceBytesPerImage:sourceBytesPerImage
                                     sourceSize:sourceSize
                                      toTexture:DstTexture
                               destinationSlice:destinationSlice
                               destinationLevel:destinationLevel
                              destinationOrigin:MTLOriginMake(0, 0, 0)];
  }
};
} // namespace hsh::detail::metal

namespace hsh::detail {
template <> struct TargetTraits<Target::HSH_METAL_TARGET> {
  struct BufferWrapper {
    __unsafe_unretained id<MTLBuffer> Buffer = nullptr;
    uint32_t Offset = 0;
    BufferWrapper() noexcept = default;
    BufferWrapper(id<MTLBuffer> Buffer, uint32_t Offset = 0) noexcept
        : Buffer(Buffer), Offset(Offset) {}
    BufferWrapper(const metal::BufferAllocation &Alloc) noexcept
        : BufferWrapper(Alloc.GetBuffer()) {}
    bool IsValid() const noexcept { return Buffer != nullptr; }
    operator id<MTLBuffer>() const noexcept { return Buffer; }
  };
  using UniformBufferOwner = metal::BufferAllocation;
  using UniformBufferBinding = BufferWrapper;
  using DynamicUniformBufferOwner = metal::DynamicBufferAllocation;
  using VertexBufferOwner = metal::BufferAllocation;
  using VertexBufferBinding = BufferWrapper;
  using DynamicVertexBufferOwner = metal::DynamicBufferAllocation;
  using IndexBufferOwner = metal::BufferAllocation;
  using IndexBufferBinding = BufferWrapper;
  using DynamicIndexBufferOwner = metal::DynamicBufferAllocation;
  struct FifoOwner {
    std::unique_ptr<metal::FifoBufferAllocation> Allocation;
    FifoOwner() noexcept = default;
    FifoOwner(const FifoOwner &other) = delete;
    FifoOwner &operator=(const FifoOwner &other) = delete;
    FifoOwner(FifoOwner &&other) noexcept = default;
    FifoOwner &operator=(FifoOwner &&other) noexcept = default;
    explicit FifoOwner(std::unique_ptr<metal::FifoBufferAllocation> Allocation)
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
    __unsafe_unretained id<MTLTexture> Texture = nullptr;
    std::uint8_t NumMips;
    bool IsValid() const noexcept { return Texture != nullptr; }
  };
  struct TextureOwner {
    metal::TextureAllocation Allocation;
    std::uint8_t NumMips;
    TextureOwner() = default;
    TextureOwner(const TextureOwner &other) = delete;
    TextureOwner &operator=(const TextureOwner &other) = delete;
    TextureOwner(TextureOwner &&other) noexcept = default;
    TextureOwner &operator=(TextureOwner &&other) noexcept = default;

    TextureOwner(metal::TextureAllocation Allocation,
                 std::uint8_t NumMips) noexcept
        : Allocation(std::move(Allocation)), NumMips(NumMips) {}

    bool IsValid() const noexcept { return Allocation.GetTexture() != nullptr; }

    TextureBinding GetBinding() const noexcept {
      return TextureBinding{Allocation.GetTexture(), NumMips};
    }
    operator TextureBinding() const noexcept { return GetBinding(); }
  };
  struct DynamicTextureOwner : TextureOwner {
    metal::UploadBufferAllocation UploadAllocation;
    std::array<metal::BufferImageCopy, MaxMipCount> Copies;

    DynamicTextureOwner() noexcept = default;
    DynamicTextureOwner(metal::TextureAllocation AllocationIn,
                        metal::UploadBufferAllocation UploadAllocation,
                        std::array<metal::BufferImageCopy, MaxMipCount> Copies,
                        std::uint8_t NumMipsIn) noexcept
        : TextureOwner(std::move(AllocationIn), NumMipsIn),
          UploadAllocation(std::move(UploadAllocation)),
          Copies(std::move(Copies)) {}

    void MakeCopies() noexcept {
      for (auto &Copy : Copies)
        Copy.Copy(UploadAllocation.GetBuffer(), Allocation.GetTexture());
    }

    void *Map() noexcept { return UploadAllocation.GetMappedData(); }
    void Unmap() noexcept { MakeCopies(); }
  };
  struct RenderTextureBinding {
    metal::RenderTextureAllocation *Allocation = nullptr;
    uint32_t BindingIdx : 24;
    uint32_t IsDepth : 8;
    RenderTextureBinding() noexcept : BindingIdx(0), IsDepth(0) {}
    RenderTextureBinding(metal::RenderTextureAllocation *Allocation,
                         uint32_t BindingIdx, uint32_t IsDepth) noexcept
        : Allocation(Allocation), BindingIdx(BindingIdx), IsDepth(IsDepth) {}

    bool IsValid() const noexcept { return Allocation != nullptr; }

    id<MTLTexture> GetTexture() const noexcept {
      if (IsDepth)
        return Allocation->GetDepthBindingTexture(BindingIdx);
      else
        return Allocation->GetColorBindingTexture(BindingIdx);
    }
  };
  struct SurfaceBinding {
    metal::SurfaceAllocation *Allocation = nullptr;
    bool IsValid() const noexcept { return Allocation != nullptr; }
  };
  struct RenderTextureOwner {
    std::unique_ptr<metal::RenderTextureAllocation> Allocation;
    RenderTextureOwner() = default;
    RenderTextureOwner(const RenderTextureOwner &other) = delete;
    RenderTextureOwner &operator=(const RenderTextureOwner &other) = delete;
    RenderTextureOwner(RenderTextureOwner &&other) noexcept = default;
    RenderTextureOwner &
    operator=(RenderTextureOwner &&other) noexcept = default;

    explicit RenderTextureOwner(
        std::unique_ptr<metal::RenderTextureAllocation> Allocation) noexcept
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
    std::unique_ptr<metal::SurfaceAllocation> Allocation;
    SurfaceOwner() = default;
    SurfaceOwner(const SurfaceOwner &other) = delete;
    SurfaceOwner &operator=(const SurfaceOwner &other) = delete;
    SurfaceOwner(SurfaceOwner &&other) noexcept = default;
    SurfaceOwner &operator=(SurfaceOwner &&other) noexcept = default;

    explicit SurfaceOwner(
        std::unique_ptr<metal::SurfaceAllocation> Allocation) noexcept
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
    void SetRequestExtent(const hsh::extent2d &Ext) noexcept {}
    void SetMargins(int32_t L, int32_t R, int32_t T, int32_t B) noexcept {
      Allocation->SetMargins(L, R, T, B);
    }
  };
  struct PipelineBinding {
    id<MTLRenderPipelineState> Pipeline;
    MTLPrimitiveType Primitive{};
    std::array<id<MTLBuffer>, MaxUniforms> UniformBuffers{};
    std::array<NSUInteger, MaxUniforms> UniformOffsets{};
    std::array<id<MTLTexture>, MaxImages> Textures{};
    std::array<id<MTLSamplerState>, MaxSamplers> Samplers{};
    static const std::array<float, MaxSamplers> SamplerMinLODs;
    std::array<float, MaxSamplers> SamplerMaxLODs{};
    std::array<id<MTLBuffer>, MaxVertexBuffers> VertexBuffers{};
    std::array<NSUInteger, MaxVertexBuffers> VertexOffsets{};
    struct BoundIndex {
      id<MTLBuffer> Buffer{};
      NSUInteger Offset{};
      MTLIndexType Type{};
    } Index;
    struct BoundRenderTexture {
      RenderTextureBinding RenderTextureBinding;
      uint32_t DescriptorBindingIdx = 0;
    };
    std::array<BoundRenderTexture, MaxImages> RenderTextures{};
    template <typename Impl> struct Iterators {
      decltype(UniformBuffers)::iterator UniformBufferBegin;
      decltype(UniformBuffers)::iterator UniformBufferIt;
      decltype(VertexBuffers)::iterator VertexBufferBegin;
      decltype(VertexBuffers)::iterator VertexBufferIt;
      decltype(Textures)::iterator TextureBegin;
      decltype(Textures)::iterator TextureIt;
      decltype(Samplers)::iterator SamplerBegin;
      decltype(Samplers)::iterator SamplerIt;
      decltype(SamplerMaxLODs)::iterator SamplerMaxLODBegin;
      decltype(SamplerMaxLODs)::iterator SamplerMaxLODIt;
      BoundIndex &Index;
      decltype(RenderTextures)::iterator RenderTextureIt;
      constexpr explicit Iterators(PipelineBinding &Binding) noexcept
          : UniformBufferBegin(Binding.UniformBuffers.begin()),
            UniformBufferIt(Binding.UniformBuffers.begin()),
            VertexBufferBegin(Binding.VertexBuffers.begin()),
            VertexBufferIt(Binding.VertexBuffers.begin()),
            TextureBegin(Binding.Textures.begin()),
            TextureIt(Binding.Textures.begin()),
            SamplerBegin(Binding.Samplers.begin()),
            SamplerIt(Binding.Samplers.begin()),
            SamplerMaxLODBegin(Binding.SamplerMaxLODs.begin()),
            SamplerMaxLODIt(Binding.SamplerMaxLODs.begin()),
            Index(Binding.Index),
            RenderTextureIt(Binding.RenderTextures.begin()) {}

      inline void Add(uniform_buffer_typeless uniform) noexcept;
      inline void Add(vertex_buffer_typeless uniform) noexcept;
      template <typename T> inline void Add(index_buffer<T> uniform) noexcept;
      inline void Add(texture_typeless texture) noexcept;
      inline void Add(render_texture2d texture) noexcept;
      inline void Add(SamplerBinding sampler) noexcept;
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

    bool IsValid() const noexcept { return Pipeline != nullptr; }

    PipelineBinding() noexcept = default;

    template <typename Impl, typename... Args>
    void Rebind(bool UpdateDescriptors, Args... args) noexcept;

    void Bind() noexcept {
      for (auto &RT : RenderTextures) {
        if (!RT.RenderTextureBinding.Allocation)
          break;
        Textures[RT.DescriptorBindingIdx] =
            RT.RenderTextureBinding.GetTexture();
      }
      if (metal::Globals.BoundPipeline != Pipeline) {
        metal::Globals.BoundPipeline = Pipeline;
        [metal::Globals.GetCmd() setRenderPipelineState:Pipeline];
      }
      [metal::Globals.GetCmd()
          setVertexBuffers:VertexBuffers.data()
                   offsets:VertexOffsets.data()
                 withRange:NSMakeRange(0, MaxVertexBuffers)];
      [metal::Globals.GetCmd()
          setVertexBuffers:UniformBuffers.data()
                   offsets:UniformOffsets.data()
                 withRange:NSMakeRange(MaxVertexBuffers, MaxUniforms)];
      [metal::Globals.GetCmd()
          setFragmentBuffers:UniformBuffers.data()
                     offsets:UniformOffsets.data()
                   withRange:NSMakeRange(MaxVertexBuffers, MaxUniforms)];
      [metal::Globals.GetCmd() setVertexTextures:Textures.data()
                                       withRange:NSMakeRange(0, MaxImages)];
      [metal::Globals.GetCmd() setFragmentTextures:Textures.data()
                                         withRange:NSMakeRange(0, MaxImages)];
      [metal::Globals.GetCmd()
          setVertexSamplerStates:Samplers.data()
                    lodMinClamps:SamplerMinLODs.data()
                    lodMaxClamps:SamplerMaxLODs.data()
                       withRange:NSMakeRange(0, MaxSamplers)];
      [metal::Globals.GetCmd()
          setFragmentSamplerStates:Samplers.data()
                      lodMinClamps:SamplerMinLODs.data()
                      lodMaxClamps:SamplerMaxLODs.data()
                         withRange:NSMakeRange(0, MaxSamplers)];
    }

    void Draw(uint32_t start, uint32_t count) noexcept {
      Bind();
      [metal::Globals.GetCmd() drawPrimitives:Primitive
                                  vertexStart:start
                                  vertexCount:count];
    }

    void DrawIndexed(uint32_t start, uint32_t count) noexcept {
      Bind();
      [metal::Globals.GetCmd()
          drawIndexedPrimitives:Primitive
                     indexCount:count
                      indexType:Index.Type
                    indexBuffer:Index.Buffer
              indexBufferOffset:start *
                                (Index.Type == MTLIndexTypeUInt32 ? 4 : 2)];
    }

    void DrawInstanced(uint32_t start, uint32_t count,
                       uint32_t instCount) noexcept {
      Bind();
      [metal::Globals.GetCmd() drawPrimitives:Primitive
                                  vertexStart:0
                                  vertexCount:count
                                instanceCount:instCount
                                 baseInstance:start];
    }

    void DrawIndexedInstanced(uint32_t start, uint32_t count,
                              uint32_t instCount) noexcept {
      Bind();
      [metal::Globals.GetCmd()
          drawIndexedPrimitives:Primitive
                     indexCount:count
                      indexType:Index.Type
                    indexBuffer:Index.Buffer
              indexBufferOffset:start *
                                (Index.Type == MTLIndexTypeUInt32 ? 4 : 2)
                  instanceCount:instCount];
    }
  };

  static void ClearAttachments(bool color, bool depth) noexcept {
    assert(metal::Globals.AttachedRenderTexture != nullptr);
    metal::Globals.AttachedRenderTexture->Clear(color, depth);
  }

  static void SetBlendConstants(float red, float green, float blue,
                                float alpha) noexcept {
    [metal::Globals.GetCmd() setBlendColorRed:red
                                        green:green
                                         blue:blue
                                        alpha:alpha];
  }

  static void SetViewport(const viewport &vp) noexcept {
    [Cmd setViewport:MTLViewport{vp.x, vp.y, vp.width, vp.height, vp.minDepth,
                                 vp.maxDepth}];
    [Cmd setScissorRect:MTLScissorRect{NSUInteger(vp.x), NSUInteger(vp.y),
                                       NSUInteger(vp.width),
                                       NSUInteger(vp.height)}];
  }

  static void SetViewport(const viewport &vp, const scissor &s) noexcept {
    [Cmd setViewport:MTLViewport{vp.x, vp.y, vp.width, vp.height, vp.minDepth,
                                 vp.maxDepth}];
    [Cmd setScissorRect:MTLScissorRect{
                            NSUInteger(s.offset.x), NSUInteger(s.offset.y),
                            NSUInteger(s.extent.w), NSUInteger(s.extent.h)}];
  }

  static void SetScissor(const scissor &s) noexcept {
    [Cmd setScissorRect:MTLScissorRect{
                            NSUInteger(s.offset.x), NSUInteger(s.offset.y),
                            NSUInteger(s.extent.w), NSUInteger(s.extent.h)}];
  }

  template <typename ResTp> struct ResourceFactory {};
};
#ifdef HSH_IMPLEMENTATION
const std::array<float, MaxSamplers>
    TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::SamplerMinLODs{};
#endif
} // namespace hsh::detail

#endif
