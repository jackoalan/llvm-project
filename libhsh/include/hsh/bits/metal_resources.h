#pragma once

#include <sstream>
#include <string_view>

#if HSH_ENABLE_METAL
extern "C" const dispatch_block_t _dispatch_data_destructor_none;
#endif

namespace hsh::detail {

#if HSH_ENABLE_METAL

constexpr MTLVertexFormat HshToMTLVertexFormat(Format Format) noexcept {
  switch (Format) {
  case R8_UNORM:
    return MTLVertexFormatUCharNormalized;
  case RG8_UNORM:
    return MTLVertexFormatUChar2Normalized;
  case RGBA8_UNORM:
    return MTLVertexFormatUChar4Normalized;
  case R16_UNORM:
    return MTLVertexFormatUShortNormalized;
  case RG16_UNORM:
    return MTLVertexFormatUShort2Normalized;
  case RGBA16_UNORM:
    return MTLVertexFormatUShort4Normalized;
  case R32_UINT:
    return MTLVertexFormatUInt;
  case RG32_UINT:
    return MTLVertexFormatUInt2;
  case RGB32_UINT:
    return MTLVertexFormatUInt3;
  case RGBA32_UINT:
    return MTLVertexFormatUInt4;
  case R8_SNORM:
    return MTLVertexFormatCharNormalized;
  case RG8_SNORM:
    return MTLVertexFormatChar2Normalized;
  case RGBA8_SNORM:
    return MTLVertexFormatChar4Normalized;
  case R16_SNORM:
    return MTLVertexFormatShortNormalized;
  case RG16_SNORM:
    return MTLVertexFormatShort2Normalized;
  case RGBA16_SNORM:
    return MTLVertexFormatShort4Normalized;
  case R32_SINT:
    return MTLVertexFormatInt;
  case RG32_SINT:
    return MTLVertexFormatInt2;
  case RGB32_SINT:
    return MTLVertexFormatInt3;
  case RGBA32_SINT:
    return MTLVertexFormatInt4;
  case R32_SFLOAT:
    return MTLVertexFormatFloat;
  case RG32_SFLOAT:
    return MTLVertexFormatFloat2;
  case RGB32_SFLOAT:
    return MTLVertexFormatFloat3;
  case RGBA32_SFLOAT:
    return MTLVertexFormatFloat4;
  case BC1_UNORM:
  case BC2_UNORM:
  case BC3_UNORM:
    return MTLVertexFormatInvalid;
  }
}

constexpr MTLPixelFormat HshToMTLPixelFormat(Format Format) noexcept {
  switch (Format) {
  case R8_UNORM:
    return MTLPixelFormatR8Unorm;
  case RG8_UNORM:
    return MTLPixelFormatRG8Unorm;
  case RGBA8_UNORM:
    return MTLPixelFormatRGBA8Unorm;
  case R16_UNORM:
    return MTLPixelFormatR16Unorm;
  case RG16_UNORM:
    return MTLPixelFormatRG16Unorm;
  case RGBA16_UNORM:
    return MTLPixelFormatRGBA16Unorm;
  case R32_UINT:
    return MTLPixelFormatR32Uint;
  case RG32_UINT:
    return MTLPixelFormatRG32Uint;
  case RGB32_UINT:
  case RGBA32_UINT:
    return MTLPixelFormatRGBA32Uint;
  case R8_SNORM:
    return MTLPixelFormatR8Snorm;
  case RG8_SNORM:
    return MTLPixelFormatRG8Snorm;
  case RGBA8_SNORM:
    return MTLPixelFormatRGBA8Snorm;
  case R16_SNORM:
    return MTLPixelFormatR16Snorm;
  case RG16_SNORM:
    return MTLPixelFormatRG16Snorm;
  case RGBA16_SNORM:
    return MTLPixelFormatRGBA16Snorm;
  case R32_SINT:
    return MTLPixelFormatR32Sint;
  case RG32_SINT:
    return MTLPixelFormatRG32Sint;
  case RGB32_SINT:
  case RGBA32_SINT:
    return MTLPixelFormatRGBA32Sint;
  case R32_SFLOAT:
    return MTLPixelFormatR32Float;
  case RG32_SFLOAT:
    return MTLPixelFormatRG32Float;
  case RGB32_SFLOAT:
  case RGBA32_SFLOAT:
    return MTLPixelFormatRGBA32Float;
  case BC1_UNORM:
  case BC2_UNORM:
  case BC3_UNORM:
    return MTLPixelFormatInvalid;
  }
}

constexpr MTLVertexStepFunction
HshToMTLVertexStepFunction(InputRate InputRate) noexcept {
  switch (InputRate) {
  case PerVertex:
    return MTLVertexStepFunctionPerVertex;
  case PerInstance:
    return MTLVertexStepFunctionPerInstance;
  }
}

constexpr MTLPrimitiveType
HshToMTLPrimitiveType(enum Topology Topology) noexcept {
  switch (Topology) {
  case Points:
    return MTLPrimitiveTypePoint;
  case Lines:
    return MTLPrimitiveTypeLine;
  case LineStrip:
    return MTLPrimitiveTypeLineStrip;
  case Triangles:
    return MTLPrimitiveTypeTriangle;
  case TriangleStrip:
  case TriangleFan:
  case Patches:
    return MTLPrimitiveTypeTriangleStrip;
  }
}

constexpr MTLPrimitiveTopologyClass
HshToMTLPrimitiveTopologyClass(enum Topology Topology) noexcept {
  switch (Topology) {
  case Points:
    return MTLPrimitiveTopologyClassPoint;
  case Lines:
  case LineStrip:
    return MTLPrimitiveTopologyClassLine;
  case Triangles:
  case TriangleStrip:
  case TriangleFan:
  case Patches:
    return MTLPrimitiveTopologyClassTriangle;
  }
}

constexpr MTLCullMode HshToMTLCullMode(enum CullMode CullMode) noexcept {
  switch (CullMode) {
  case CullNone:
    return MTLCullModeNone;
  case CullFront:
    return MTLCullModeFront;
  case CullBack:
  case CullFrontAndBack:
    return MTLCullModeBack;
  }
}

constexpr MTLCompareFunction
HshToMTLCompareFunction(enum Compare Compare) noexcept {
  switch (Compare) {
  case Never:
    return MTLCompareFunctionNever;
  case Less:
    return MTLCompareFunctionLess;
  case Equal:
    return MTLCompareFunctionEqual;
  case LEqual:
    return MTLCompareFunctionLessEqual;
  case Greater:
    return MTLCompareFunctionGreater;
  case NEqual:
    return MTLCompareFunctionNotEqual;
  case GEqual:
    return MTLCompareFunctionGreaterEqual;
  case Always:
    return MTLCompareFunctionAlways;
  }
}

constexpr MTLBlendFactor
HshToMTLBlendFactor(enum BlendFactor BlendFactor) noexcept {
  switch (BlendFactor) {
  case Zero:
    return MTLBlendFactorZero;
  case One:
    return MTLBlendFactorOne;
  case SrcColor:
    return MTLBlendFactorSourceColor;
  case InvSrcColor:
    return MTLBlendFactorOneMinusSourceColor;
  case DstColor:
    return MTLBlendFactorDestinationColor;
  case InvDstColor:
    return MTLBlendFactorOneMinusDestinationColor;
  case SrcAlpha:
    return MTLBlendFactorSourceAlpha;
  case InvSrcAlpha:
    return MTLBlendFactorOneMinusSourceAlpha;
  case DstAlpha:
    return MTLBlendFactorDestinationAlpha;
  case InvDstAlpha:
    return MTLBlendFactorOneMinusDestinationAlpha;
  case Src1Color:
    return MTLBlendFactorSource1Color;
  case InvSrc1Color:
    return MTLBlendFactorOneMinusSource1Color;
  case ConstColor:
    return MTLBlendFactorBlendColor;
  case InvConstColor:
    return MTLBlendFactorOneMinusBlendColor;
  case ConstAlpha:
    return MTLBlendFactorBlendAlpha;
  case InvConstAlpha:
    return MTLBlendFactorOneMinusBlendAlpha;
  case Src1Alpha:
    return MTLBlendFactorSource1Alpha;
  case InvSrc1Alpha:
    return MTLBlendFactorOneMinusSource1Alpha;
  }
}

constexpr MTLBlendOperation
HshToMTLBlendOperation(enum BlendOp BlendOp) noexcept {
  switch (BlendOp) {
  case Add:
    return MTLBlendOperationAdd;
  case Subtract:
    return MTLBlendOperationSubtract;
  case ReverseSubtract:
    return MTLBlendOperationReverseSubtract;
  }
}

constexpr MTLSamplerMinMagFilter
HshToMTLSamplerMinMagFilter(enum Filter Filter) noexcept {
  switch (Filter) {
  case Nearest:
    return MTLSamplerMinMagFilterNearest;
  case Linear:
    return MTLSamplerMinMagFilterLinear;
  }
}

constexpr MTLSamplerMipFilter
HshToMTLSamplerMipFilter(enum Filter Filter) noexcept {
  switch (Filter) {
  case Nearest:
    return MTLSamplerMipFilterNearest;
  case Linear:
    return MTLSamplerMipFilterLinear;
  }
}

constexpr MTLSamplerAddressMode
HshToMTLSamplerAddressMode(enum SamplerAddressMode AddressMode) noexcept {
  switch (AddressMode) {
  case Repeat:
    return MTLSamplerAddressModeRepeat;
  case MirroredRepeat:
    return MTLSamplerAddressModeMirrorRepeat;
  case ClampToEdge:
    return MTLSamplerAddressModeClampToEdge;
  case ClampToBorder:
#if TARGET_OS_OSX
    return MTLSamplerAddressModeClampToBorderColor;
#else
    return MTLSamplerAddressModeClampToZero;
#endif
  case MirrorClampToEdge:
#if TARGET_OS_OSX
    return MTLSamplerAddressModeMirrorClampToEdge;
#else
    return MTLSamplerAddressModeClampToEdge;
#endif
  }
}

#if TARGET_OS_OSX
constexpr MTLSamplerBorderColor
HshToMTLSamplerBorderColor(enum BorderColor BorderColor) noexcept {
  switch (BorderColor) {
  case TransparentBlack:
    return MTLSamplerBorderColorTransparentBlack;
  case OpaqueBlack:
    return MTLSamplerBorderColorOpaqueBlack;
  case OpaqueWhite:
    return MTLSamplerBorderColorOpaqueWhite;
  }
}
#endif

constexpr MTLColorWriteMask
HshToMTLColorWriteMask(enum ColorComponentFlags Comps) noexcept {
  return MTLColorWriteMask(
      (Comps & CC_Red ? unsigned(MTLColorWriteMaskRed) : 0u) |
      (Comps & CC_Green ? unsigned(MTLColorWriteMaskGreen) : 0u) |
      (Comps & CC_Blue ? unsigned(MTLColorWriteMaskBlue) : 0u) |
      (Comps & CC_Alpha ? unsigned(MTLColorWriteMaskAlpha) : 0u));
}

constexpr MTLTextureSwizzle
HshToMTLTextureSwizzle(enum ColorSwizzle swizzle,
                       MTLTextureSwizzle ident) noexcept {
  switch (swizzle) {
  case CS_Identity:
  default:
    return ident;
  case CS_Red:
    return MTLTextureSwizzleRed;
  case CS_Green:
    return MTLTextureSwizzleGreen;
  case CS_Blue:
    return MTLTextureSwizzleBlue;
  case CS_Alpha:
    return MTLTextureSwizzleAlpha;
  }
}

template <> struct ShaderObject<Target::HSH_METAL_TARGET> {
  id<MTLFunction> ShaderFunction = nullptr;
  ShaderObject() = default;
  id<MTLFunction> Get(const ShaderCode<Target::HSH_METAL_TARGET> &Info,
                      const SourceLocation &Location) noexcept {
    if (!ShaderFunction) {
      NSError *Error = nullptr;
      dispatch_data_t Data =
          dispatch_data_create(reloc(Info.Blob.Data), Info.Blob.Size, nullptr,
                               _dispatch_data_destructor_none);
      id<MTLLibrary> Library =
          [metal::Globals.Device newLibraryWithData:Data error:&Error];
      if (!Library) {
        (*metal::Globals.ErrHandler)(Error);
        return nullptr;
      }
      ShaderFunction = [Library newFunctionWithName:@"shader_main"];
#if HSH_SOURCE_LOCATION_ENABLED
      ShaderFunction.label = @(Location.to_string().c_str());
#endif
    }
    return ShaderFunction;
  }
  void Destroy() noexcept { ShaderFunction = nullptr; }
};

struct MetalSamplerCreateInfo {
  MTLSamplerMinMagFilter minFilter;
  MTLSamplerMinMagFilter magFilter;
  MTLSamplerMipFilter mipFilter;
  MTLSamplerAddressMode sAddressMode;
  MTLSamplerAddressMode tAddressMode;
  MTLSamplerAddressMode rAddressMode;
#if TARGET_OS_OSX
  MTLSamplerBorderColor borderColor;
#endif
  MTLCompareFunction compareFunction;

  id<MTLSamplerState>
  Initialize(const SourceLocation &Location) const noexcept {
    MTLSamplerDescriptor *Descriptor = [MTLSamplerDescriptor new];
    Descriptor.minFilter = minFilter;
    Descriptor.magFilter = magFilter;
    Descriptor.mipFilter = mipFilter;
    Descriptor.maxAnisotropy = NSUInteger(metal::Globals.Anisotropy);
    Descriptor.sAddressMode = sAddressMode;
    Descriptor.tAddressMode = tAddressMode;
    Descriptor.rAddressMode = rAddressMode;
#if TARGET_OS_OSX
    Descriptor.borderColor = borderColor;
#endif
    Descriptor.normalizedCoordinates = true;
#if HSH_SOURCE_LOCATION_ENABLED
    Descriptor.label = @(Location.to_string().c_str());
#endif
    return [metal::Globals.Device newSamplerStateWithDescriptor:Descriptor];
  }
};

template <> struct SamplerObject<Target::HSH_METAL_TARGET> {
  id<MTLSamplerState> Sampler;
  SamplerObject() = default;
  id<MTLSamplerState> Get(const MetalSamplerCreateInfo &Info,
                          const SourceLocation &Location) noexcept {
    if (!Sampler)
      Sampler = Info.Initialize(Location);
    return Sampler;
  }
  void Destroy() noexcept { Sampler = nullptr; }
};

template <typename Impl, typename... Args>
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::Rebind(
    bool UpdateDescriptors, Args... args) noexcept {
  Pipeline = Impl::HSH_METAL_DATA.Pipeline;
  Primitive = Impl::HSH_METAL_CDATA.Primitive;
  if (UpdateDescriptors) {
    Iterators<Impl> Its(*this);
    (Its.Add(args), ...);
  }
  OffsetIterators OffIts(*this);
  (OffIts.Add(args), ...);
}

namespace metal {
template <typename T> struct IndexTypeValue {};
template <> struct IndexTypeValue<uint16_t> {
  static constexpr MTLIndexType value = MTLIndexTypeUInt16;
};
template <> struct IndexTypeValue<uint32_t> {
  static constexpr MTLIndexType value = MTLIndexTypeUInt32;
};
}

template <typename Impl>
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::Iterators<
    Impl>::Add(uniform_buffer_typeless ubo) noexcept {
  *UniformBufferIt++ = ubo.Binding.HSH_GET_METAL_TARGET();
}
template <typename Impl>
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::Iterators<
    Impl>::Add(vertex_buffer_typeless vbo) noexcept {
  *VertexBufferIt++ = vbo.Binding.HSH_GET_METAL_TARGET();
}
template <typename Impl>
template <typename T>
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::Iterators<
    Impl>::Add(index_buffer<T> ibo) noexcept {
  Index.Buffer = ibo.Binding.HSH_GET_METAL_TARGET();
  Index.Type = metal::IndexTypeValue<T>::value;
}
template <typename Impl>
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::Iterators<
    Impl>::Add(texture_typeless texture) noexcept {
  *TextureIt++ = texture.Binding.HSH_GET_METAL_TARGET().Texture;
}
template <typename Impl>
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::Iterators<
    Impl>::Add(render_texture2d texture) noexcept {
  auto &RT = *RenderTextureIt++;
  RT.RenderTextureBinding = texture.Binding.HSH_GET_METAL_TARGET();
  RT.DescriptorBindingIdx = uint32_t(TextureIt - TextureBegin);
  *TextureIt++ = RT.RenderTextureBinding.GetTexture();
}
template <typename Impl>
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::Iterators<
    Impl>::Add(SamplerBinding sampler) noexcept {
  *SamplerIt++ = Impl::HSH_METAL_DATA.SamplerObjects[sampler.Idx]->Get(
      Impl::HSH_METAL_CDATA.Samplers[sampler.Idx],
      Impl::HSH_METAL_CDATA.Location.with_field("Sampler",
                                                SamplerIt - SamplerBegin));
  *SamplerMaxLODIt++ =
      float(sampler.Tex.Binding.HSH_GET_METAL_TARGET().NumMips - 1);
}

void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::OffsetIterators::
    Add(uniform_buffer_typeless ubo) noexcept {
  const auto &Uniform = ubo.Binding.HSH_GET_METAL_TARGET();
  *UniformOffsetsIt++ = Uniform.Offset;
}
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::OffsetIterators::
    Add(vertex_buffer_typeless vbo) noexcept {
  const auto &Vertex = vbo.Binding.HSH_GET_METAL_TARGET();
  *VertexOffsetsIt++ = Vertex.Offset;
}
template <typename T>
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::OffsetIterators::
    Add(index_buffer<T> ibo) noexcept {
  const auto &Idx = ibo.Binding.HSH_GET_METAL_TARGET();
  Index.Offset = Idx.Offset;
}
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::OffsetIterators::
    Add(texture_typeless) noexcept {}
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::OffsetIterators::
    Add(render_texture2d) noexcept {}
void TargetTraits<Target::HSH_METAL_TARGET>::PipelineBinding::OffsetIterators::
    Add(SamplerBinding) noexcept {}

struct MetalVertexBufferLayoutDescriptor {
  NSUInteger stride;
  MTLVertexStepFunction stepFunction;
  NSUInteger stepRate;
  MTLVertexBufferLayoutDescriptor *Get() const noexcept {
    MTLVertexBufferLayoutDescriptor *Descriptor =
        [MTLVertexBufferLayoutDescriptor new];
    Descriptor.stride = stride;
    Descriptor.stepFunction = stepFunction;
    Descriptor.stepRate = stepRate;
    return Descriptor;
  }
};

struct MetalVertexAttributeDescriptor {
  MTLVertexFormat format;
  NSUInteger offset;
  NSUInteger bufferIndex;
  MTLVertexAttributeDescriptor *Get() const noexcept {
    MTLVertexAttributeDescriptor *Descriptor =
        [MTLVertexAttributeDescriptor new];
    Descriptor.format = format;
    Descriptor.offset = offset;
    Descriptor.bufferIndex = bufferIndex;
    return Descriptor;
  }
};

struct MetalRenderPipelineColorAttachmentDescriptor {
  BOOL blendingEnabled;
  MTLBlendFactor sourceRGBBlendFactor;
  MTLBlendFactor destinationRGBBlendFactor;
  MTLBlendOperation rgbBlendOperation;
  MTLBlendFactor sourceAlphaBlendFactor;
  MTLBlendFactor destinationAlphaBlendFactor;
  MTLBlendOperation alphaBlendOperation;
  MTLColorWriteMask writeMask;
  MTLRenderPipelineColorAttachmentDescriptor *
  Get(MTLPixelFormat pixelFormat) const noexcept {
    MTLRenderPipelineColorAttachmentDescriptor *Descriptor =
        [MTLRenderPipelineColorAttachmentDescriptor new];
    Descriptor.pixelFormat = pixelFormat;
    Descriptor.blendingEnabled = blendingEnabled;
    Descriptor.sourceRGBBlendFactor = sourceRGBBlendFactor;
    Descriptor.destinationRGBBlendFactor = destinationRGBBlendFactor;
    Descriptor.rgbBlendOperation = rgbBlendOperation;
    Descriptor.sourceAlphaBlendFactor = sourceAlphaBlendFactor;
    Descriptor.destinationAlphaBlendFactor = destinationAlphaBlendFactor;
    Descriptor.alphaBlendOperation = alphaBlendOperation;
    Descriptor.writeMask = writeMask;
    return Descriptor;
  }
};

template <std::uint32_t NStages, std::uint32_t NBindings,
          std::uint32_t NAttributes, std::uint32_t NSamplers,
          std::uint32_t NAttachments>
struct ShaderConstData<Target::HSH_METAL_TARGET, NStages, NBindings,
                       NAttributes, NSamplers, NAttachments> {
  std::array<ShaderCode<Target::HSH_METAL_TARGET>, NStages> StageCodes;
  std::array<MetalVertexBufferLayoutDescriptor, NBindings>
      VertexBufferLayoutDescriptors;
  std::array<MetalVertexAttributeDescriptor, NAttributes>
      VertexAttributeDescriptors;
  std::array<MetalRenderPipelineColorAttachmentDescriptor, NAttachments>
      TargetAttachments;
  MTLPrimitiveType Primitive;
  MTLPrimitiveTopologyClass TopologyClass;
  unsigned PatchControlPoints;
  MTLCullMode CullMode;
  MTLCompareFunction DepthCompareFunction;
  bool DepthWriteEnabled;
  std::array<MetalSamplerCreateInfo, NSamplers> Samplers;
  SourceLocation Location;
  bool DirectRenderPass;

  template <std::size_t... SSeq, std::size_t... BSeq, std::size_t... ASeq,
            std::size_t... SampSeq, std::size_t... AttSeq>
  constexpr ShaderConstData(
      std::array<ShaderCode<Target::HSH_METAL_TARGET>, NStages> S,
      std::array<VertexBinding, NBindings> B,
      std::array<VertexAttribute, NAttributes> A,
      std::array<sampler, NSamplers> Samps,
      std::array<ColorAttachment, NAttachments> Atts,
      struct PipelineInfo PipelineInfo, const SourceLocation &Location,
      std::index_sequence<SSeq...>, std::index_sequence<BSeq...>,
      std::index_sequence<ASeq...>, std::index_sequence<SampSeq...>,
      std::index_sequence<AttSeq...>) noexcept
      : StageCodes(S),
        VertexBufferLayoutDescriptors{MetalVertexBufferLayoutDescriptor{
            std::get<BSeq>(B).Stride,
            HshToMTLVertexStepFunction(std::get<BSeq>(B).InputRate), 1}...},
        VertexAttributeDescriptors{MetalVertexAttributeDescriptor{
            HshToMTLVertexFormat(std::get<ASeq>(A).Format),
            std::get<ASeq>(A).Offset, std::get<ASeq>(A).Binding}...},
        TargetAttachments{MetalRenderPipelineColorAttachmentDescriptor{
            std::get<AttSeq>(Atts).blendEnabled(),
            HshToMTLBlendFactor(std::get<AttSeq>(Atts).SrcColorBlendFactor),
            HshToMTLBlendFactor(std::get<AttSeq>(Atts).DstColorBlendFactor),
            HshToMTLBlendOperation(std::get<AttSeq>(Atts).ColorBlendOp),
            HshToMTLBlendFactor(std::get<AttSeq>(Atts).SrcAlphaBlendFactor),
            HshToMTLBlendFactor(std::get<AttSeq>(Atts).DstAlphaBlendFactor),
            HshToMTLBlendOperation(std::get<AttSeq>(Atts).AlphaBlendOp),
            HshToMTLColorWriteMask(
                std::get<AttSeq>(Atts).ColorWriteComponents)}...},
        Primitive{HshToMTLPrimitiveType(PipelineInfo.Topology)},
        TopologyClass{HshToMTLPrimitiveTopologyClass(PipelineInfo.Topology)},
        PatchControlPoints{PipelineInfo.PatchControlPoints},
        CullMode{HshToMTLCullMode(PipelineInfo.CullMode)},
        DepthCompareFunction{
            HshToMTLCompareFunction(PipelineInfo.DepthCompare)},
        DepthWriteEnabled{PipelineInfo.DepthWrite},
        Samplers{MetalSamplerCreateInfo{
            HshToMTLSamplerMinMagFilter(std::get<SampSeq>(Samps).MagFilter),
            HshToMTLSamplerMinMagFilter(std::get<SampSeq>(Samps).MinFilter),
            HshToMTLSamplerMipFilter(std::get<SampSeq>(Samps).MipmapMode),
            HshToMTLSamplerAddressMode(std::get<SampSeq>(Samps).AddressModeU),
            HshToMTLSamplerAddressMode(std::get<SampSeq>(Samps).AddressModeV),
            HshToMTLSamplerAddressMode(std::get<SampSeq>(Samps).AddressModeW),
#if TARGET_OS_OSX
            HshToMTLSamplerBorderColor(std::get<SampSeq>(Samps).BorderColor),
#endif
            HshToMTLCompareFunction(std::get<SampSeq>(Samps).CompareOp)}...},
        Location(Location), DirectRenderPass(PipelineInfo.DirectRenderPass) {
  }

  constexpr ShaderConstData(
      std::array<ShaderCode<Target::HSH_METAL_TARGET>, NStages> S,
      std::array<VertexBinding, NBindings> B,
      std::array<VertexAttribute, NAttributes> A,
      std::array<sampler, NSamplers> Samps,
      std::array<ColorAttachment, NAttachments> Atts,
      struct PipelineInfo PipelineInfo,
      const SourceLocation &Location = SourceLocation::current()) noexcept
      : ShaderConstData(S, B, A, Samps, Atts, PipelineInfo, Location,
                        std::make_index_sequence<NStages>(),
                        std::make_index_sequence<NBindings>(),
                        std::make_index_sequence<NAttributes>(),
                        std::make_index_sequence<NSamplers>(),
                        std::make_index_sequence<NAttachments>()) {}

  template <typename B>
  MTLRenderPipelineDescriptor *GetPipelineInfo() const noexcept {
    MTLRenderPipelineDescriptor *Descriptor = [MTLRenderPipelineDescriptor new];

#if HSH_SOURCE_LOCATION_ENABLED
    Descriptor.label = @(B::HSH_METAL_CDATA.Location.to_string().c_str());
#endif

    for (std::size_t i = 0; i < NStages; ++i) {
      switch (StageCodes[i].Stage) {
      case Stage::Vertex:
        Descriptor.vertexFunction = B::HSH_METAL_DATA.ShaderObjects[i]->Get(
            StageCodes[i],
#if HSH_SOURCE_LOCATION_ENABLED
            B::HSH_METAL_CDATA.Location.with_field("VertexFunction")
#else
            B::HSH_METAL_CDATA.Location
#endif
        );
        break;
      case Stage::Fragment:
        Descriptor.fragmentFunction = B::HSH_METAL_DATA.ShaderObjects[i]->Get(
            StageCodes[i],
#if HSH_SOURCE_LOCATION_ENABLED
            B::HSH_METAL_CDATA.Location.with_field("FragmentFunction")
#else
            B::HSH_METAL_CDATA.Location
#endif
        );
        break;
      default:
        break;
      }
    }

    MTLVertexDescriptor *VertexDescriptor =
        [MTLVertexDescriptor vertexDescriptor];
    for (std::size_t i = 0; i < NBindings; ++i)
      VertexDescriptor.layouts[i] = VertexBufferLayoutDescriptors[i].Get();
    for (std::size_t i = 0; i < NAttributes; ++i)
      VertexDescriptor.attributes[i] = VertexAttributeDescriptors[i].Get();
    Descriptor.vertexDescriptor = VertexDescriptor;

    for (std::size_t i = 0; i < NAttachments; ++i)
      Descriptor.colorAttachments[i] =
          TargetAttachments[i].Get(metal::Globals.TargetPixelFormat);
    Descriptor.depthAttachmentPixelFormat =
        DirectRenderPass ? MTLPixelFormatInvalid : MTLPixelFormatDepth32Float;
    Descriptor.sampleCount = metal::Globals.SampleCount;
    Descriptor.inputPrimitiveTopology = TopologyClass;

#if HSH_METAL_BINARY_ARCHIVE
    if (HSH_METAL_BINARY_ARCHIVE_AVAILABILITY) {
      if ([metal::Globals.PipelineCache
              respondsToSelector:@selector
              (addRenderPipelineFunctionsWithDescriptor:error:)]) {
        Descriptor.binaryArchives = @[ metal::Globals.PipelineCache ];
        [metal::Globals.PipelineCache
            addRenderPipelineFunctionsWithDescriptor:Descriptor
                                               error:nullptr];
      }
    }
#endif

    return Descriptor;
  }

  id<MTLDepthStencilState> GetDepthStencilState() const noexcept {
    return metal::Globals.GetDepthStencilState(DepthCompareFunction,
                                               DepthWriteEnabled);
  }
};

template <std::uint32_t NStages, std::uint32_t NSamplers>
struct ShaderData<Target::HSH_METAL_TARGET, NStages, NSamplers> {
  using ObjectRef = ShaderObject<Target::HSH_METAL_TARGET> *;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = SamplerObject<Target::HSH_METAL_TARGET> *;
  std::array<SamplerRef, NSamplers> SamplerObjects;
  id<MTLRenderPipelineState> Pipeline;
  constexpr ShaderData(std::array<ObjectRef, NStages> S,
                       std::array<SamplerRef, NSamplers> Samps) noexcept
      : ShaderObjects(S), SamplerObjects(Samps) {}
  void Destroy() noexcept {
    for (auto &Obj : ShaderObjects)
      Obj->Destroy();
    for (auto &Obj : SamplerObjects)
      Obj->Destroy();
    Pipeline = nullptr;
  }
};

template <> struct PipelineBuilder<Target::HSH_METAL_TARGET> {
  template <typename B> static void CreatePipeline() noexcept {
    NSError *Error = nullptr;
    B::HSH_METAL_DATA.Pipeline = [metal::Globals.Device
        newRenderPipelineStateWithDescriptor:B::HSH_METAL_CDATA
                                                 .template GetPipelineInfo<B>()
                                       error:&Error];
    if (!B::HSH_METAL_DATA.Pipeline)
      (*metal::Globals.ErrHandler)(Error);
  }
  template <typename... B> static void CreatePipelines() noexcept {
    (CreatePipeline<B>(), ...);
  }
  template <typename... B> static void DestroyPipelines() noexcept {
    (B::HSH_METAL_DATA.Destroy(), ...);
  }
};

namespace buffer_math::metal {
template <uint32_t... Idx>
static constexpr std::array<detail::metal::BufferImageCopy, MaxMipCount>
MakeCopies1D(uint32_t width, uint32_t layers, uint32_t texelSize,
             uint32_t texelShift,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {detail::metal::BufferImageCopy{
      MipOffset1D(width, layers, texelSize, texelShift, Idx),
      (width >> Idx) * texelSize, (width >> Idx) * texelSize,
      MTLSize{width >> Idx, 1, 1}, 0, Idx}...};
}
static constexpr std::array<detail::metal::BufferImageCopy, MaxMipCount>
MakeCopies1D(uint32_t width, uint32_t layers, uint32_t texelSize,
             uint32_t texelShift) noexcept {
  return MakeCopies1D(width, layers, texelSize, texelShift,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}

template <uint32_t... Idx>
static constexpr std::array<detail::metal::BufferImageCopy, MaxMipCount>
MakeCopies2D(uint32_t width, uint32_t height, uint32_t layers,
             uint32_t texelSize, uint32_t texelShift,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {detail::metal::BufferImageCopy{
      MipOffset2D(width, height, layers, texelSize, texelShift, Idx),
      (width >> Idx) * texelSize, (width >> Idx) * (height >> Idx) * texelSize,
      MTLSize{width >> Idx, height >> Idx, layers}, 0, Idx}...};
}
static constexpr std::array<detail::metal::BufferImageCopy, MaxMipCount>
MakeCopies2D(uint32_t width, uint32_t height, uint32_t layers,
             uint32_t texelSize, uint32_t texelShift) noexcept {
  return MakeCopies2D(width, height, layers, texelSize, texelShift,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}

template <uint32_t... Idx>
static constexpr std::array<detail::metal::BufferImageCopy, MaxMipCount>
MakeCopies3D(uint32_t width, uint32_t height, uint32_t depth,
             uint32_t texelSize, uint32_t texelShift,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {detail::metal::BufferImageCopy{
      MipOffset3D(width, height, depth, texelSize, texelShift, Idx),
      (width >> Idx) * texelSize, (width >> Idx) * (height >> Idx) * texelSize,
      MTLSize{width >> Idx, height >> Idx, depth >> Idx}, 0, Idx}...};
}
static constexpr std::array<detail::metal::BufferImageCopy, MaxMipCount>
MakeCopies3D(uint32_t width, uint32_t height, uint32_t depth,
             uint32_t texelSize, uint32_t texelShift) noexcept {
  return MakeCopies3D(width, height, depth, texelSize, texelShift,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}
} // namespace buffer_math::metal

template <typename CopyFunc>
inline auto CreateBufferOwner(const SourceLocation &location, std::size_t size,
                              CopyFunc copyFunc) noexcept {
  auto UploadBuffer = metal::AllocateUploadBuffer(location, size);
  copyFunc(UploadBuffer.GetMappedData(), size);

  auto Ret = metal::AllocateStaticBuffer(location, size);

  [metal::Globals.GetCopyCmd() copyFromBuffer:UploadBuffer.GetBuffer()
                                 sourceOffset:0
                                     toBuffer:Ret.GetBuffer()
                            destinationOffset:0
                                         size:size];

  return Ret;
}

inline auto CreateDynamicBufferOwner(const SourceLocation &location,
                                     std::size_t size) noexcept {
  return metal::AllocateDynamicBuffer(location, size);
}

inline auto CreateFifoOwner(const SourceLocation &location,
                            std::size_t size) noexcept {
  return metal::AllocateFifoBuffer(location, size);
}

template <typename T>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<
    uniform_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location,
                     CopyFunc copyFunc) noexcept {
    return CreateBufferOwner(location, sizeof(T), copyFunc);
  }

  static auto CreateDynamic(const SourceLocation &location) noexcept {
    return CreateDynamicBufferOwner(location, sizeof(T));
  }

  static auto CreateDynamic(const SourceLocation &location,
                            size_t size) noexcept {
    return CreateDynamicBufferOwner(location, size);
  }
};

template <typename T>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<
    vertex_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, std::size_t Count,
                     CopyFunc copyFunc) noexcept {
    return CreateBufferOwner(location, sizeof(T) * Count, copyFunc);
  }

  static auto CreateDynamic(const SourceLocation &location,
                            std::size_t Count) noexcept {
    return CreateDynamicBufferOwner(location, sizeof(T) * Count);
  }
};

template <typename T>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<
    index_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, std::size_t Count,
                     CopyFunc copyFunc) noexcept {
    return CreateBufferOwner(location, sizeof(T) * Count, copyFunc);
  }

  static auto CreateDynamic(const SourceLocation &location,
                            std::size_t Count) noexcept {
    return CreateDynamicBufferOwner(location, sizeof(T) * Count);
  }
};

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<uniform_fifo> {
  static auto Create(const SourceLocation &location,
                     std::size_t Size) noexcept {
    return TargetTraits<Target::HSH_METAL_TARGET>::FifoOwner{
        CreateFifoOwner(location, Size)};
  }
};

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<vertex_fifo> {
  static auto Create(const SourceLocation &location,
                     std::size_t Size) noexcept {
    return TargetTraits<Target::HSH_METAL_TARGET>::FifoOwner{
        CreateFifoOwner(location, Size)};
  }
};

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<index_fifo> {
  static auto Create(const SourceLocation &location,
                     std::size_t Size) noexcept {
    return TargetTraits<Target::HSH_METAL_TARGET>::FifoOwner{
        CreateFifoOwner(location, Size)};
  }
};

template <MTLTextureType Type> struct TextureTypeTraits {};

template <> struct TextureTypeTraits<MTLTextureType1D> {
  static constexpr char Name[] = "Texture1D";
  using ExtentType = uint32_t;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize, uint32_t texelShift) {
    return buffer_math::metal::MakeCopies1D(extent, layers, texelSize,
                                            texelShift);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t texelShift,
                                  uint32_t mips) {
    return buffer_math::MipOffset1D(extent, layers, texelSize, texelShift,
                                    mips);
  }
  static void SetExtentIntoDescriptor(MTLTextureDescriptor *Descriptor,
                                      ExtentType extent) {
    Descriptor.width = extent;
  }
};
template <>
struct TextureTypeTraits<MTLTextureType1DArray>
    : TextureTypeTraits<MTLTextureType1D> {
  static constexpr char Name[] = "Texture1DArray";
};

template <> struct TextureTypeTraits<MTLTextureType2D> {
  static constexpr char Name[] = "Texture2D";
  using ExtentType = extent2d;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize, uint32_t texelShift) {
    return buffer_math::metal::MakeCopies2D(extent.w, extent.h, layers,
                                            texelSize, texelShift);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t texelShift,
                                  uint32_t mips) {
    return buffer_math::MipOffset2D(extent.w, extent.h, layers, texelSize,
                                    texelShift, mips);
  }
  static void SetExtentIntoDescriptor(MTLTextureDescriptor *Descriptor,
                                      ExtentType extent) {
    Descriptor.width = extent.w;
    Descriptor.height = extent.h;
  }
};
template <>
struct TextureTypeTraits<MTLTextureType2DArray>
    : TextureTypeTraits<MTLTextureType2D> {
  static constexpr char Name[] = "Texture2DArray";
};

template <> struct TextureTypeTraits<MTLTextureType3D> {
  static constexpr char Name[] = "Texture3D";
  using ExtentType = extent3d;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize, uint32_t texelShift) {
    return buffer_math::metal::MakeCopies3D(extent.w, extent.h, extent.d,
                                            texelSize, texelShift);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t texelShift,
                                  uint32_t mips) {
    return buffer_math::MipOffset3D(extent.w, extent.h, extent.d, texelSize,
                                    texelShift, mips);
  }
  static void SetExtentIntoDescriptor(MTLTextureDescriptor *Descriptor,
                                      ExtentType extent) {
    Descriptor.width = extent.w;
    Descriptor.height = extent.h;
    Descriptor.depth = extent.d;
  }
};

template <MTLTextureType Type, typename Traits = TextureTypeTraits<Type>,
          typename CopyFunc>
inline auto CreateTextureOwner(
    const SourceLocation &location, typename Traits::ExtentType extent,
    uint32_t numLayers, Format format, uint32_t numMips, CopyFunc copyFunc,
    ColorSwizzle redSwizzle, ColorSwizzle greenSwizzle,
    ColorSwizzle blueSwizzle, ColorSwizzle alphaSwizzle) noexcept {
  auto TexelSize = HshFormatToTexelSize(format);
  auto TexelSizeShift = HshFormatToTexelSizeShift(format);
  auto TexelFormat = HshToMTLPixelFormat(format);
  std::array<detail::metal::BufferImageCopy, MaxMipCount> Copies =
      Traits::MakeCopies(extent, numLayers, TexelSize, TexelSizeShift);
  auto BufferSize =
      Traits::MipOffset(extent, numLayers, TexelSize, TexelSizeShift, numMips);
  auto UploadBuffer = metal::AllocateUploadBuffer(location, BufferSize);
  copyFunc(UploadBuffer.GetMappedData(), BufferSize);

  MTLTextureDescriptor *Descriptor = [MTLTextureDescriptor new];
  Descriptor.textureType = Type;
  Descriptor.pixelFormat = TexelFormat;
  Traits::SetExtentIntoDescriptor(Descriptor, extent);
  Descriptor.mipmapLevelCount = numMips;
  Descriptor.arrayLength = numLayers;
  Descriptor.storageMode = MTLStorageModePrivate;
  Descriptor.usage = MTLTextureUsageShaderRead;
  Descriptor.swizzle = MTLTextureSwizzleChannels{
      HshToMTLTextureSwizzle(redSwizzle, MTLTextureSwizzleRed),
      HshToMTLTextureSwizzle(greenSwizzle, MTLTextureSwizzleGreen),
      HshToMTLTextureSwizzle(blueSwizzle, MTLTextureSwizzleBlue),
      HshToMTLTextureSwizzle(alphaSwizzle, MTLTextureSwizzleAlpha)};

  TargetTraits<Target::HSH_METAL_TARGET>::TextureOwner Ret{
      metal::AllocateTexture(location.with_field(Traits::Name), Descriptor),
      std::uint8_t(numMips)};

  for (uint32_t i = 0; i < numMips; ++i)
    Copies[i].Copy(UploadBuffer.GetBuffer(), Ret.Allocation.GetTexture());

  return Ret;
}

template <MTLTextureType Type, typename Traits = TextureTypeTraits<Type>>
inline auto CreateDynamicTextureOwner(const SourceLocation &location,
                                      typename Traits::ExtentType extent,
                                      uint32_t numLayers, Format format,
                                      uint32_t numMips, ColorSwizzle redSwizzle,
                                      ColorSwizzle greenSwizzle,
                                      ColorSwizzle blueSwizzle,
                                      ColorSwizzle alphaSwizzle) noexcept {
  auto TexelSize = HshFormatToTexelSize(format);
  auto TexelSizeShift = HshFormatToTexelSizeShift(format);
  auto TexelFormat = HshToMTLPixelFormat(format);
  auto BufferSize =
      Traits::MipOffset(extent, numLayers, TexelSize, TexelSizeShift, numMips);
  auto UploadBuffer = metal::AllocateUploadBuffer(location, BufferSize);

  MTLTextureDescriptor *Descriptor = [MTLTextureDescriptor new];
  Descriptor.textureType = Type;
  Descriptor.pixelFormat = TexelFormat;
  Traits::SetExtentIntoDescriptor(Descriptor, extent);
  Descriptor.mipmapLevelCount = numMips;
  Descriptor.arrayLength = numLayers;
  Descriptor.storageMode = MTLStorageModePrivate;
  Descriptor.usage = MTLTextureUsageShaderRead;
  Descriptor.swizzle = MTLTextureSwizzleChannels{
      HshToMTLTextureSwizzle(redSwizzle, MTLTextureSwizzleRed),
      HshToMTLTextureSwizzle(greenSwizzle, MTLTextureSwizzleGreen),
      HshToMTLTextureSwizzle(blueSwizzle, MTLTextureSwizzleBlue),
      HshToMTLTextureSwizzle(alphaSwizzle, MTLTextureSwizzleAlpha)};

  TargetTraits<Target::HSH_METAL_TARGET>::DynamicTextureOwner Ret{
      metal::AllocateTexture(location.with_field(Traits::Name), Descriptor),
      std::move(UploadBuffer),
      Traits::MakeCopies(extent, numLayers, TexelSize, TexelSizeShift),
      std::uint8_t(numMips)};

  return Ret;
}

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<texture1d> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, uint32_t extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<MTLTextureType1D>(
        location, extent, 1, format, numMips, copyFunc, redSwizzle,
        greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, uint32_t extent,
                            Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<MTLTextureType1D>(
        location, extent, 1, format, numMips, redSwizzle, greenSwizzle,
        blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<
    texture1d_array> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, uint32_t extent,
                     uint32_t numLayers, Format format, uint32_t numMips,
                     CopyFunc copyFunc, ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<MTLTextureType1DArray>(
        location, extent, numLayers, format, numMips, copyFunc, redSwizzle,
        greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, uint32_t extent,
                            uint32_t numLayers, Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<MTLTextureType1DArray>(
        location, extent, numLayers, format, numMips, redSwizzle, greenSwizzle,
        blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<texture2d> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, extent2d extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<MTLTextureType2D>(
        location, extent, 1, format, numMips, copyFunc, redSwizzle,
        greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, extent2d extent,
                            Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<MTLTextureType2D>(
        location, extent, 1, format, numMips, redSwizzle, greenSwizzle,
        blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<
    texture2d_array> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, extent2d extent,
                     uint32_t numLayers, Format format, uint32_t numMips,
                     CopyFunc copyFunc, ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<MTLTextureType2DArray>(
        location, extent, numLayers, format, numMips, copyFunc, redSwizzle,
        greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, extent2d extent,
                            uint32_t numLayers, Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<MTLTextureType2DArray>(
        location, extent, numLayers, format, numMips, redSwizzle, greenSwizzle,
        blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<texture3d> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, extent3d extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<MTLTextureType3D>(
        location, extent, 1, format, numMips, copyFunc, redSwizzle,
        greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, extent3d extent,
                            Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<MTLTextureType3D>(
        location, extent, 1, format, numMips, redSwizzle, greenSwizzle,
        blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<
    render_texture2d> {
  static auto Create(const SourceLocation &location, surface Surf,
                     uint32_t NumColorBindings = 0,
                     uint32_t NumDepthBindings = 0) noexcept {
    return TargetTraits<Target::HSH_METAL_TARGET>::RenderTextureOwner{
        std::make_unique<metal::RenderTextureAllocation>(
            location.with_field("RenderTexture2D"),
            Surf.Binding.HSH_GET_METAL_TARGET().Allocation, NumColorBindings,
            NumDepthBindings)};
  }

  static auto Create(const SourceLocation &location, extent2d Extent,
                     uint32_t NumColorBindings = 0,
                     uint32_t NumDepthBindings = 0) noexcept {
    return TargetTraits<Target::HSH_METAL_TARGET>::RenderTextureOwner{
        std::make_unique<metal::RenderTextureAllocation>(
            location.with_field("RenderTexture2D"), Extent, NumColorBindings,
            NumDepthBindings)};
  }
};

template <>
struct TargetTraits<Target::HSH_METAL_TARGET>::ResourceFactory<surface> {
  static auto Create(const SourceLocation &location, CAMetalLayer *MetalLayer,
                     std::function<void(const hsh::extent2d &,
                                        const hsh::extent2d &)> &&ResizeLambda,
                     std::function<void()> &&DeleterLambda,
                     const hsh::extent2d &RequestExtent, int32_t L, int32_t R,
                     int32_t T, int32_t B) noexcept {
    return TargetTraits<Target::HSH_METAL_TARGET>::SurfaceOwner{
        std::make_unique<metal::SurfaceAllocation>(
            location, std::move(MetalLayer), std::move(ResizeLambda),
            std::move(DeleterLambda), RequestExtent, L, R, T, B)};
  }
};

#endif

} // namespace hsh::detail

#if HSH_ENABLE_METAL

namespace hsh {
class metal_device_owner {
  friend class metal_instance_owner;
  struct Data {
    id<MTLDevice> Device = nullptr;
#if HSH_METAL_BINARY_ARCHIVE
    id PipelineCache = nullptr;
#endif
    id<MTLCommandQueue> CommandQueue = nullptr;
    std::array<detail::metal::DeletedResources, 2> DeletedResources;
    bool BuiltPipelines = false;

    ~Data() noexcept {
      detail::metal::Globals.waitIdle();
      if (BuiltPipelines) {
        hsh::detail::GlobalListNode<false>::DestroyAll(
            ActiveTarget::HSH_METAL_TARGET);
        hsh::detail::GlobalListNode<true>::DestroyAll(
            ActiveTarget::HSH_METAL_TARGET);
      }
    }
  };
  std::unique_ptr<Data> Data;

public:
  metal_device_owner() noexcept = default;
  metal_device_owner(metal_device_owner &&) noexcept = default;
  metal_device_owner &operator=(metal_device_owner &&) noexcept = default;

  bool success() const noexcept { return Data.operator bool(); }
  operator bool() const noexcept { return success(); }

  void build_pipelines() noexcept {
    if (Data->BuiltPipelines)
      return;
    hsh::detail::GlobalListNode<true>::CreateAll(
        ActiveTarget::HSH_METAL_TARGET);
    hsh::detail::GlobalListNode<false>::CreateAll(
        ActiveTarget::HSH_METAL_TARGET);
    Data->BuiltPipelines = true;
  }

#if HSH_METAL_BINARY_ARCHIVE
  template <typename CacheFileMgr>
  void build_pipelines(CacheFileMgr &CFM) noexcept {
    if (Data->BuiltPipelines)
      return;
    Data->PipelineCache = hsh::detail::metal::CreatePipelineCache(CFM);
    detail::metal::Globals.PipelineCache = Data->PipelineCache;
    hsh::detail::GlobalListNode<true>::CreateAll(
        ActiveTarget::HSH_METAL_TARGET);
    hsh::detail::GlobalListNode<false>::CreateAll(
        ActiveTarget::HSH_METAL_TARGET);
    hsh::detail::metal::WritePipelineCache(CFM);
    Data->BuiltPipelines = true;
  }
#else
  template <typename CacheFileMgr>
  void build_pipelines(CacheFileMgr &CFM) noexcept {
    build_pipelines();
  }
#endif

  using HighCompleteFunc = std::function<void()>;
  using ProgFunc = std::function<void(std::size_t, std::size_t)>;

  void build_pipelines(const HighCompleteFunc &HCF,
                       const ProgFunc &PF) noexcept {
    if (Data->BuiltPipelines)
      return;
    hsh::detail::GlobalListNode<true>::CreateAll(
        ActiveTarget::HSH_METAL_TARGET);
    HCF();
    std::size_t Count = hsh::detail::GlobalListNode<false>::CountAll();
    std::size_t I = 0;
    PF(I, Count);
    for (auto *Node = hsh::detail::GlobalListNode<false>::GetHead(); Node;
         Node = Node->GetNext()) {
      Node->Create(ActiveTarget::HSH_METAL_TARGET);
      PF(++I, Count);
    }
    Data->BuiltPipelines = true;
  }

#if HSH_METAL_BINARY_ARCHIVE
  template <typename CacheFileMgr>
  void build_pipelines(CacheFileMgr &CFM, const HighCompleteFunc &HCF,
                       const ProgFunc &PF) noexcept {
    if (Data->BuiltPipelines)
      return;
    Data->PipelineCache = hsh::detail::metal::CreatePipelineCache(CFM);
    detail::metal::Globals.PipelineCache = Data->PipelineCache;
    hsh::detail::GlobalListNode<true>::CreateAll(
        ActiveTarget::HSH_METAL_TARGET);
    HCF();
    std::size_t Count = hsh::detail::GlobalListNode<false>::CountAll();
    std::size_t I = 0;
    PF(I, Count);
    for (auto *Node = hsh::detail::GlobalListNode<false>::GetHead(); Node;
         Node = Node->GetNext()) {
      Node->Create(ActiveTarget::HSH_METAL_TARGET);
      PF(++I, Count);
    }
    hsh::detail::metal::WritePipelineCache(CFM);
    Data->BuiltPipelines = true;
  }

  template <typename CacheFileMgr> class pipeline_build_pump {
    CacheFileMgr *CFM = nullptr;
    std::size_t Count = 0;
    std::size_t I = 0;
    hsh::detail::GlobalListNode<false> *Node = nullptr;

  public:
    pipeline_build_pump() noexcept = default;

    explicit pipeline_build_pump(CacheFileMgr &CFM) noexcept
        : CFM(&CFM), Count(hsh::detail::GlobalListNode<false>::CountAll()),
          Node(hsh::detail::GlobalListNode<false>::GetHead()) {
      if (Node == nullptr)
        hsh::detail::metal::WritePipelineCache(CFM);
    }

    std::pair<std::size_t, std::size_t> get_progress() const noexcept {
      return {I, Count};
    }

    bool pump() noexcept {
      if (Node) {
        Node->Create(ActiveTarget::HSH_METAL_TARGET);
        Node = Node->GetNext();
        ++I;
        if (Node == nullptr) {
          hsh::detail::metal::WritePipelineCache(*CFM);
          return false;
        }
        return true;
      }
      return false;
    }

    operator bool() const noexcept { return CFM; }
  };

  template <typename CacheFileMgr>
  pipeline_build_pump<CacheFileMgr>
  start_build_pipelines(CacheFileMgr &CFM) noexcept {
    if (Data->BuiltPipelines)
      return {};
    Data->BuiltPipelines = true;

    Data->PipelineCache = hsh::detail::metal::CreatePipelineCache(CFM);
    detail::metal::Globals.PipelineCache = Data->PipelineCache;
    hsh::detail::GlobalListNode<true>::CreateAll(
        ActiveTarget::HSH_METAL_TARGET);

    return pipeline_build_pump(CFM);
  }
#else
  template <typename CacheFileMgr>
  void build_pipelines(CacheFileMgr &CFM, const HighCompleteFunc &HCF,
                       const ProgFunc &PF) noexcept {
    build_pipelines(HCF, PF);
  }
#endif

  template <typename Func> void enter_draw_context(Func F) const noexcept {
    detail::metal::Globals.PreRender();
    F();
    detail::metal::Globals.PostRender();
  }

  static void wait_idle() noexcept { detail::metal::Globals.waitIdle(); }
};

class metal_instance_owner {
  friend metal_instance_owner
  create_metal_instance(detail::metal::ErrorHandler &&ErrHandler) noexcept;
  struct Data {
    detail::metal::ErrorHandler ErrHandler;
  };
  std::unique_ptr<Data> Data;

public:
  bool success() const noexcept { return Data.operator bool(); }
  operator bool() const noexcept { return success(); }

  template <typename Func>
  metal_device_owner
  enumerate_metal_devices(Func Acceptor,
                          CAMetalLayer *CheckSurface = {}) const noexcept {
    metal_device_owner Ret;
    auto &ErrHandler = Data->ErrHandler;

    id<MTLDevice> UseDevice = nullptr;
#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
    for (id<MTLDevice> Device in MTLCopyAllDevices()) {
      detail::metal::Globals.Device = Device;
      if (Acceptor(Device,
                   CheckSurface && CheckSurface.preferredDevice == Device)) {
        UseDevice = Device;
        break;
      }
    }
#else
    if (id<MTLDevice> Device = MTLCreateSystemDefaultDevice()) {
      detail::metal::Globals.Device = Device;
      if (Acceptor(Device,
                   CheckSurface && CheckSurface.preferredDevice == Device))
        UseDevice = Device;
    }
#endif

    if (!UseDevice)
      return {};

    Ret.Data = std::make_unique<struct metal_device_owner::Data>();
    auto &Data = *Ret.Data;
    Data.Device = UseDevice;
    Data.CommandQueue = [UseDevice newCommandQueue];

    detail::metal::Globals.Device = Data.Device;
    detail::metal::Globals.CommandQueue = Data.CommandQueue;

    detail::metal::Globals.DeletedResourcesArr = &Data.DeletedResources;
    detail::metal::Globals.DeletedResources = &Data.DeletedResources[0];

    return Ret;
  }
};

inline metal_instance_owner
create_metal_instance(detail::metal::ErrorHandler &&ErrHandler) noexcept {
  metal_instance_owner Ret;
  Ret.Data = std::make_unique<struct metal_instance_owner::Data>();
  auto &Data = *Ret.Data;
  Data.ErrHandler = std::move(ErrHandler);
  detail::metal::Globals.ErrHandler = &Data.ErrHandler;
  return Ret;
}
} // namespace hsh

#undef HSH_METAL_TARGET
#undef HSH_GET_METAL_TARGET
#undef HSH_METAL_DATA
#undef HSH_METAL_CDATA

#endif
