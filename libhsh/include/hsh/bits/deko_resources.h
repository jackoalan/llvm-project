#pragma once

#include <sstream>
#include <string_view>

namespace hsh::detail {

template <> struct ShaderCode<Target::DEKO3D> {
  enum Stage Stage = Stage::Vertex;
  const uint8_t *Control = nullptr;
  ShaderDataBlob<uint8_t> Blob;
  constexpr ShaderCode() noexcept = default;
  constexpr ShaderCode(enum Stage Stage, const uint8_t *Control,
                       ShaderDataBlob<uint8_t> Blob) noexcept
      : Stage(Stage), Control(Control), Blob(Blob) {}
};

#if HSH_ENABLE_DEKO3D

constexpr DkImageFormat HshToDkFormat(Format Format) noexcept {
  switch (Format) {
  case R8_UNORM:
  default:
    return DkImageFormat_R8_Unorm;
  case RG8_UNORM:
    return DkImageFormat_RG8_Unorm;
  case RGBA8_UNORM:
    return DkImageFormat_RGBA8_Unorm;
  case R16_UNORM:
    return DkImageFormat_R16_Unorm;
  case RG16_UNORM:
    return DkImageFormat_RG16_Unorm;
  case RGBA16_UNORM:
    return DkImageFormat_RGBA16_Unorm;
  case R32_UINT:
    return DkImageFormat_R32_Uint;
  case RG32_UINT:
    return DkImageFormat_RG32_Uint;
  case RGB32_UINT:
    return DkImageFormat_RGB32_Uint;
  case RGBA32_UINT:
    return DkImageFormat_RGBA32_Uint;
  case R8_SNORM:
    return DkImageFormat_R8_Snorm;
  case RG8_SNORM:
    return DkImageFormat_RG8_Snorm;
  case RGBA8_SNORM:
    return DkImageFormat_RGBA8_Snorm;
  case R16_SNORM:
    return DkImageFormat_R16_Snorm;
  case RG16_SNORM:
    return DkImageFormat_RG16_Snorm;
  case RGBA16_SNORM:
    return DkImageFormat_RGBA16_Snorm;
  case R32_SINT:
    return DkImageFormat_R32_Sint;
  case RG32_SINT:
    return DkImageFormat_RG32_Sint;
  case RGB32_SINT:
    return DkImageFormat_RGB32_Sint;
  case RGBA32_SINT:
    return DkImageFormat_RGBA32_Sint;
  case R32_SFLOAT:
    return DkImageFormat_R32_Float;
  case RG32_SFLOAT:
    return DkImageFormat_RG32_Float;
  case RGB32_SFLOAT:
    return DkImageFormat_RGB32_Float;
  case RGBA32_SFLOAT:
    return DkImageFormat_RGBA32_Float;
  case BC1_UNORM:
    return DkImageFormat_RGBA_BC1;
  case BC2_UNORM:
    return DkImageFormat_RGBA_BC2;
  case BC3_UNORM:
    return DkImageFormat_RGBA_BC3;
  }
}

constexpr DkVtxAttribSize HshToDkVtxAttribSize(hsh::Format Format) {
  switch (Format) {
  case R8_UNORM:
    return DkVtxAttribSize_1x8;
  case RG8_UNORM:
    return DkVtxAttribSize_2x8;
  case RGBA8_UNORM:
    return DkVtxAttribSize_4x8;
  case R16_UNORM:
    return DkVtxAttribSize_1x16;
  case RG16_UNORM:
    return DkVtxAttribSize_2x16;
  case RGBA16_UNORM:
    return DkVtxAttribSize_4x16;
  case R32_UINT:
    return DkVtxAttribSize_1x32;
  case RG32_UINT:
    return DkVtxAttribSize_2x32;
  case RGB32_UINT:
    return DkVtxAttribSize_3x32;
  case RGBA32_UINT:
    return DkVtxAttribSize_4x32;
  case R8_SNORM:
    return DkVtxAttribSize_1x8;
  case RG8_SNORM:
    return DkVtxAttribSize_2x8;
  case RGBA8_SNORM:
    return DkVtxAttribSize_4x8;
  case R16_SNORM:
    return DkVtxAttribSize_1x16;
  case RG16_SNORM:
    return DkVtxAttribSize_2x16;
  case RGBA16_SNORM:
    return DkVtxAttribSize_4x16;
  case R32_SINT:
    return DkVtxAttribSize_1x32;
  case RG32_SINT:
    return DkVtxAttribSize_2x32;
  case RGB32_SINT:
    return DkVtxAttribSize_3x32;
  case RGBA32_SINT:
    return DkVtxAttribSize_4x32;
  case R32_SFLOAT:
    return DkVtxAttribSize_1x32;
  case RG32_SFLOAT:
    return DkVtxAttribSize_2x32;
  case RGB32_SFLOAT:
    return DkVtxAttribSize_3x32;
  case RGBA32_SFLOAT:
    return DkVtxAttribSize_4x32;
  /* Irrelevant: */
  case BC1_UNORM:
  case BC2_UNORM:
  case BC3_UNORM:
    return DkVtxAttribSize_4x32;
  }
}
constexpr DkVtxAttribType HshToDkVtxAttribType(hsh::Format Format) {
  switch (Format) {
  case R8_UNORM:
    return DkVtxAttribType_Unorm;
  case RG8_UNORM:
    return DkVtxAttribType_Unorm;
  case RGBA8_UNORM:
    return DkVtxAttribType_Unorm;
  case R16_UNORM:
    return DkVtxAttribType_Unorm;
  case RG16_UNORM:
    return DkVtxAttribType_Unorm;
  case RGBA16_UNORM:
    return DkVtxAttribType_Unorm;
  case R32_UINT:
    return DkVtxAttribType_Uint;
  case RG32_UINT:
    return DkVtxAttribType_Uint;
  case RGB32_UINT:
    return DkVtxAttribType_Uint;
  case RGBA32_UINT:
    return DkVtxAttribType_Uint;
  case R8_SNORM:
    return DkVtxAttribType_Unorm;
  case RG8_SNORM:
    return DkVtxAttribType_Unorm;
  case RGBA8_SNORM:
    return DkVtxAttribType_Unorm;
  case R16_SNORM:
    return DkVtxAttribType_Unorm;
  case RG16_SNORM:
    return DkVtxAttribType_Unorm;
  case RGBA16_SNORM:
    return DkVtxAttribType_Unorm;
  case R32_SINT:
    return DkVtxAttribType_Sint;
  case RG32_SINT:
    return DkVtxAttribType_Sint;
  case RGB32_SINT:
    return DkVtxAttribType_Sint;
  case RGBA32_SINT:
    return DkVtxAttribType_Sint;
  case R32_SFLOAT:
    return DkVtxAttribType_Float;
  case RG32_SFLOAT:
    return DkVtxAttribType_Float;
  case RGB32_SFLOAT:
    return DkVtxAttribType_Float;
  case RGBA32_SFLOAT:
    return DkVtxAttribType_Float;
  /* Irrelevant: */
  case BC1_UNORM:
  case BC2_UNORM:
  case BC3_UNORM:
    return DkVtxAttribType_Float;
  }
}

constexpr uint32_t HshToDkDivisor(InputRate InputRate) noexcept {
  switch (InputRate) {
  case PerVertex:
    return 0;
  case PerInstance:
    return 1;
  }
}

constexpr DkPrimitive HshToDkPrimitive(enum Topology Topology) noexcept {
  switch (Topology) {
  case Points:
    return DkPrimitive_Points;
  case Lines:
    return DkPrimitive_Lines;
  case LineStrip:
    return DkPrimitive_LineStrip;
  case Triangles:
    return DkPrimitive_Triangles;
  case TriangleStrip:
    return DkPrimitive_TriangleStrip;
  case TriangleFan:
    return DkPrimitive_TriangleFan;
  case Patches:
    return DkPrimitive_Patches;
  }
}

constexpr DkFace HshToDkFace(enum CullMode CullMode) noexcept {
  switch (CullMode) {
  case CullNone:
    return DkFace_None;
  case CullFront:
    return DkFace_Front;
  case CullBack:
    return DkFace_Back;
  case CullFrontAndBack:
    return DkFace_FrontAndBack;
  }
}

constexpr DkCompareOp HshToDkCompare(enum Compare Compare) noexcept {
  switch (Compare) {
  case Never:
    return DkCompareOp_Never;
  case Less:
    return DkCompareOp_Less;
  case Equal:
    return DkCompareOp_Equal;
  case LEqual:
    return DkCompareOp_Lequal;
  case Greater:
    return DkCompareOp_Greater;
  case NEqual:
    return DkCompareOp_NotEqual;
  case GEqual:
    return DkCompareOp_Gequal;
  case Always:
    return DkCompareOp_Always;
  }
}

constexpr DkBlendFactor
HshToDkBlendFactor(enum BlendFactor BlendFactor) noexcept {
  switch (BlendFactor) {
  case Zero:
    return DkBlendFactor_Zero;
  case One:
    return DkBlendFactor_One;
  case SrcColor:
    return DkBlendFactor_SrcColor;
  case InvSrcColor:
    return DkBlendFactor_InvSrcColor;
  case DstColor:
    return DkBlendFactor_DstColor;
  case InvDstColor:
    return DkBlendFactor_InvDstColor;
  case SrcAlpha:
    return DkBlendFactor_SrcAlpha;
  case InvSrcAlpha:
    return DkBlendFactor_InvSrcAlpha;
  case DstAlpha:
    return DkBlendFactor_DstAlpha;
  case InvDstAlpha:
    return DkBlendFactor_InvDstAlpha;
  case ConstColor:
    return DkBlendFactor_ConstColor;
  case InvConstColor:
    return DkBlendFactor_InvConstColor;
  case ConstAlpha:
    return DkBlendFactor_ConstAlpha;
  case InvConstAlpha:
    return DkBlendFactor_InvConstAlpha;
  case Src1Color:
    return DkBlendFactor_Src1Color;
  case InvSrc1Color:
    return DkBlendFactor_InvSrc1Color;
  case Src1Alpha:
    return DkBlendFactor_Src1Alpha;
  case InvSrc1Alpha:
    return DkBlendFactor_InvSrc1Alpha;
  }
}

constexpr DkBlendOp HshToDkBlendOp(enum BlendOp BlendOp) noexcept {
  switch (BlendOp) {
  case Add:
    return DkBlendOp_Add;
  case Subtract:
    return DkBlendOp_Sub;
  case ReverseSubtract:
    return DkBlendOp_RevSub;
  }
}

constexpr DkFilter HshToDkFilter(enum Filter Filter) noexcept {
  switch (Filter) {
  case Nearest:
    return DkFilter_Nearest;
  case Linear:
    return DkFilter_Linear;
  }
}

constexpr DkMipFilter HshToDkMipFilter(enum Filter Filter) noexcept {
  switch (Filter) {
  case Nearest:
    return DkMipFilter_Nearest;
  case Linear:
    return DkMipFilter_Linear;
  }
}

constexpr DkWrapMode
HshToDkWrapMode(enum SamplerAddressMode AddressMode) noexcept {
  switch (AddressMode) {
  case Repeat:
    return DkWrapMode_Repeat;
  case MirroredRepeat:
    return DkWrapMode_MirroredRepeat;
  case ClampToEdge:
    return DkWrapMode_ClampToEdge;
  case ClampToBorder:
    return DkWrapMode_ClampToBorder;
  case MirrorClampToEdge:
    return DkWrapMode_MirrorClampToEdge;
  }
}

constexpr float HshToDkBorderColor(enum BorderColor BorderColor) noexcept {
  switch (BorderColor) {
  case TransparentBlack:
    return 0.f;
  case OpaqueBlack:
    return 0.f;
  case OpaqueWhite:
    return 1.f;
  }
}

constexpr float HshToDkBorderAlpha(enum BorderColor BorderColor) noexcept {
  switch (BorderColor) {
  case TransparentBlack:
    return 0.f;
  case OpaqueBlack:
    return 1.f;
  case OpaqueWhite:
    return 1.f;
  }
}

constexpr DkStage HshToDkStage(enum Stage Stage) noexcept {
  switch (Stage) {
  default:
  case Vertex:
    return DkStage_Vertex;
  case Control:
    return DkStage_TessCtrl;
  case Evaluation:
    return DkStage_TessEval;
  case Geometry:
    return DkStage_Geometry;
  case Fragment:
    return DkStage_Fragment;
  }
}

constexpr uint32_t
HshToDkColorComponentFlags(enum ColorComponentFlags Comps) noexcept {
  return (Comps & CC_Red ? uint32_t(DkColorMask_R) : 0u) |
         (Comps & CC_Green ? uint32_t(DkColorMask_G) : 0u) |
         (Comps & CC_Blue ? uint32_t(DkColorMask_B) : 0u) |
         (Comps & CC_Alpha ? uint32_t(DkColorMask_A) : 0u);
}

constexpr DkImageSwizzle HshToDkImageSwizzle(enum ColorSwizzle swizzle,
                                             DkImageSwizzle ident) noexcept {
  switch (swizzle) {
  case CS_Identity:
  default:
    return ident;
  case CS_Red:
    return DkImageSwizzle_Red;
  case CS_Green:
    return DkImageSwizzle_Green;
  case CS_Blue:
    return DkImageSwizzle_Blue;
  case CS_Alpha:
    return DkImageSwizzle_Alpha;
  }
}

template <> struct ShaderObject<Target::DEKO3D> {
  dk::Shader Shader;
  ShaderObject() noexcept = default;
  const dk::Shader &Get(const dk::ShaderMaker &Info) noexcept {
    if (!Shader.isValid()) {
      Info.initialize(Shader);
    }
    return Shader;
  }
};

namespace deko {
template <typename Impl> struct DescriptorPoolWrites {
  template <typename... Args>
  constexpr explicit DescriptorPoolWrites(
      deko::DescriptorSetAllocation<dk::ImageDescriptor> &ImageDescriptors,
      deko::DescriptorSetAllocation<dk::SamplerDescriptor> &SamplerDescriptors,
      Args... args) noexcept {
    Iterators Its(ImageDescriptors.Map(), SamplerDescriptors.Map());
    (Its.Add(args), ...);
    ImageDescriptors.Unmap();
    SamplerDescriptors.Unmap();
  }

  struct Iterators {
    dk::ImageDescriptor *ImageIt;
    dk::SamplerDescriptor *SamplerIt;
    constexpr explicit Iterators(dk::ImageDescriptor *ImageIt,
                                 dk::SamplerDescriptor *SamplerIt) noexcept
        : ImageIt(ImageIt), SamplerIt(SamplerIt) {}
    static void Add(uniform_buffer_typeless) noexcept {}
    static void Add(vertex_buffer_typeless) noexcept {}
    static void Add(index_buffer_typeless) noexcept {}
    void Add(texture_typeless Texture) noexcept {
      ImageIt++->initialize(Texture.Binding.get_DEKO3D().GetImageView());
    }
    void Add(render_texture2d Texture) noexcept {
      ImageIt++->initialize(Texture.Binding.get_DEKO3D().GetImageView());
    }
    void Add(hsh::detail::SamplerBinding Sampler) noexcept {
      Impl::cdata_DEKO3D.Samplers[Sampler.Idx].Initialize(
          *SamplerIt++, Sampler.Tex.Binding.get_DEKO3D().GetImageView());
    }
  };
};
} // namespace deko

template <typename Impl, typename... Args>
void TargetTraits<Target::DEKO3D>::PipelineBinding::Rebind(
    bool UpdateDescriptors, Args... args) noexcept {
  PipelineObj = &Impl::data_DEKO3D.Pipeline;
  StageMask = Impl::cdata_DEKO3D.StageMask;
  Primitive = Impl::cdata_DEKO3D.Primitive;
  NumVtxBufferStates = Impl::cdata_DEKO3D.VertexBindingDescriptions.size();
  VtxBufferStates = Impl::cdata_DEKO3D.VertexBindingDescriptions.data();
  NumVtxAttribStates = Impl::cdata_DEKO3D.VertexAttributeDescriptions.size();
  VtxAttribStates = Impl::cdata_DEKO3D.VertexAttributeDescriptions.data();
  NumBlendStates = Impl::cdata_DEKO3D.TargetAttachments.size();
  BlendStates = Impl::cdata_DEKO3D.TargetAttachments.data();
  PatchSize = Impl::cdata_DEKO3D.PatchSize;
  RasterizerState = &Impl::cdata_DEKO3D.RasterizationState;
  DepthStencilState = &Impl::cdata_DEKO3D.DepthStencilState;
  ColorState = &Impl::cdata_DEKO3D.ColorBlendState;
  ColorWriteState = &Impl::cdata_DEKO3D.ColorWriteState;
  PrimitiveRestart = Impl::cdata_DEKO3D.PrimitiveRestart;
  if (UpdateDescriptors) {
    constexpr uint32_t NumImages =
        ((std::is_base_of_v<hsh::texture_typeless, Args> ||
                  std::is_same_v<hsh::render_texture2d, Args>
              ? 1
              : 0) +
         ...);
    if (NumImages != 0 && !ImageDescriptors.IsValid())
      ImageDescriptors =
          deko::AllocateDescriptorSet<dk::ImageDescriptor>(NumImages);
    constexpr uint32_t NumSamplers =
        ((std::is_same_v<hsh::detail::SamplerBinding, Args> ? 1 : 0) + ...);
    if (NumSamplers != 0 && !SamplerDescriptors.IsValid())
      SamplerDescriptors =
          deko::AllocateDescriptorSet<dk::SamplerDescriptor>(NumSamplers);
    deko::DescriptorPoolWrites<Impl>(ImageDescriptors, SamplerDescriptors,
                                     args...);
  }
  Iterators Its(*this);
  (Its.Add(args), ...);
  NumUniformBuffers = Its.UniformBufferIt - Its.UniformBufferBegin;
  NumTextures = Its.TextureIt - Its.TextureBegin;
  NumVertexBuffers = Its.VertexBufferIt - Its.VertexBufferBegin;
}

void TargetTraits<Target::DEKO3D>::PipelineBinding::Iterators::Add(
    uniform_buffer_typeless Ubo) noexcept {
  // Optimized GCC builds fail unless this is locally bound
  auto Tmp = Ubo.Binding.get_DEKO3D().Buffer;
  *UniformBufferIt++ = Tmp;
}
void TargetTraits<Target::DEKO3D>::PipelineBinding::Iterators::Add(
    vertex_buffer_typeless Vbo) noexcept {
  // Optimized GCC builds fail unless this is locally bound
  auto Tmp = Vbo.Binding.get_DEKO3D().Buffer;
  *VertexBufferIt++ = Tmp;
}
template <typename T>
void TargetTraits<Target::DEKO3D>::PipelineBinding::Iterators::Add(
    index_buffer<T> Ibo) noexcept {
  Index.Buffer = Ibo.Binding.get_DEKO3D().Buffer.addr;
  Index.Type = dk::IndexTypeValue<T>::value;
}
void TargetTraits<Target::DEKO3D>::PipelineBinding::Iterators::Add(
    texture_typeless) noexcept {}
void TargetTraits<Target::DEKO3D>::PipelineBinding::Iterators::Add(
    render_texture2d) noexcept {}
void TargetTraits<Target::DEKO3D>::PipelineBinding::Iterators::Add(
    SamplerBinding Sampler) noexcept {
  uint32_t SamplerIdx = TextureIt - TextureBegin;
  *TextureIt++ = dkMakeTextureHandle(Sampler.TexIdx, SamplerIdx);
}

struct StageCode {
  const uint8_t *Control;
  const uint8_t *Shader;
  void Initialize(dk::Shader &ShaderOut) const noexcept {
    DkShaderMaker Maker{deko::Globals.ShaderMem, &__deko_control,
                        uint32_t(Shader - &__deko_shader_begin),
                        uint32_t((Control - &__deko_control) / 64)};
    dkShaderInitialize(&ShaderOut, &Maker);
  }
};

template <std::uint32_t NStages, std::uint32_t NBindings,
          std::uint32_t NAttributes, std::uint32_t NSamplers,
          std::uint32_t NAttachments>
struct ShaderConstData<Target::DEKO3D, NStages, NBindings, NAttributes,
                       NSamplers, NAttachments> {
  struct Sampler : DkSampler {
    void Initialize(dk::SamplerDescriptor &Out,
                    const dk::ImageView &ImageView) const noexcept {
      DkSampler ModSampler = *this;
      ModSampler.lodClampMax = ImageView.mipLevelCount;
      // TODO: Handle format-dependent border color
      dkSamplerDescriptorInitialize(&Out, &ModSampler);
    }
  };

  std::array<StageCode, NStages> StageCodes;
  uint32_t StageMask;
  std::array<DkVtxBufferState, NBindings> VertexBindingDescriptions;
  std::array<DkVtxAttribState, NAttributes> VertexAttributeDescriptions;
  std::array<DkBlendState, NAttachments> TargetAttachments;
  DkPrimitive Primitive;
  uint32_t PatchSize;
  DkRasterizerState RasterizationState;
  DkDepthStencilState DepthStencilState;
  DkColorState ColorBlendState;
  DkColorWriteState ColorWriteState;
  std::array<Sampler, NSamplers> Samplers;
  bool PrimitiveRestart;

  template <std::size_t... SSeq, std::size_t... BSeq, std::size_t... ASeq,
            std::size_t... SampSeq, std::size_t... AttSeq>
  constexpr ShaderConstData(std::array<ShaderCode<Target::DEKO3D>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A,
                            std::array<sampler, NSamplers> Samps,
                            std::array<ColorAttachment, NAttachments> Atts,
                            struct PipelineInfo PipelineInfo,
                            const SourceLocation &Location,
                            std::index_sequence<SSeq...>,
                            std::index_sequence<BSeq...>,
                            std::index_sequence<ASeq...>,
                            std::index_sequence<SampSeq...>,
                            std::index_sequence<AttSeq...>) noexcept
      : StageCodes{StageCode{std::get<SSeq>(S).Control,
                             std::get<SSeq>(S).Blob.Data}...},
        StageMask{((1u << HshToDkStage(std::get<SSeq>(S).Stage)) | ...)},
        VertexBindingDescriptions{
            DkVtxBufferState{std::get<BSeq>(B).Stride,
                             HshToDkDivisor(std::get<BSeq>(B).InputRate)}...},
        VertexAttributeDescriptions{DkVtxAttribState{
            std::get<ASeq>(A).Binding, 0, std::get<ASeq>(A).Offset,
            HshToDkVtxAttribSize(std::get<ASeq>(A).Format),
            HshToDkVtxAttribType(std::get<ASeq>(A).Format), 0}...},
        TargetAttachments{DkBlendState{
            HshToDkBlendOp(std::get<AttSeq>(Atts).ColorBlendOp),
            HshToDkBlendFactor(std::get<AttSeq>(Atts).SrcColorBlendFactor),
            HshToDkBlendFactor(std::get<AttSeq>(Atts).DstColorBlendFactor),
            HshToDkBlendOp(std::get<AttSeq>(Atts).AlphaBlendOp),
            HshToDkBlendFactor(std::get<AttSeq>(Atts).SrcAlphaBlendFactor),
            HshToDkBlendFactor(std::get<AttSeq>(Atts).DstAlphaBlendFactor)}...},
        Primitive{HshToDkPrimitive(PipelineInfo.Topology)},
        PatchSize{PipelineInfo.PatchControlPoints},
        RasterizationState{DkRasterizerState{
            1, 0, 0, DkPolygonMode_Fill, DkPolygonMode_Fill,
            HshToDkFace(PipelineInfo.CullMode), DkFrontFace_CCW, 0, 0}},
        DepthStencilState{DkDepthStencilState{
            PipelineInfo.DepthCompare != Always, PipelineInfo.DepthWrite, false,
            HshToDkCompare(PipelineInfo.DepthCompare), DkStencilOp_Keep,
            DkStencilOp_Replace, DkStencilOp_Keep, DkCompareOp_Always,
            DkStencilOp_Keep, DkStencilOp_Replace, DkStencilOp_Keep,
            DkCompareOp_Always}},
        ColorBlendState{DkColorState{
            ((uint32_t(std::get<AttSeq>(Atts).blendEnabled() ? 1 : 0)
              << AttSeq) |
             ...),
            DkLogicOp_Copy, DkCompareOp_Always}},
        ColorWriteState{
            DkColorWriteState{((HshToDkColorComponentFlags(
                                    std::get<AttSeq>(Atts).ColorWriteComponents)
                                << (AttSeq * 4)) |
                               ...)}},
        Samplers{DkSampler{
            HshToDkFilter(std::get<SampSeq>(Samps).MinFilter),
            HshToDkFilter(std::get<SampSeq>(Samps).MagFilter),
            HshToDkMipFilter(std::get<SampSeq>(Samps).MipmapMode),
            {HshToDkWrapMode(std::get<SampSeq>(Samps).AddressModeU),
             HshToDkWrapMode(std::get<SampSeq>(Samps).AddressModeV),
             HshToDkWrapMode(std::get<SampSeq>(Samps).AddressModeW)},
            0.f,
            1000.f,
            std::get<SampSeq>(Samps).MipLodBias,
            0.f,
            std::get<SampSeq>(Samps).CompareOp != Never,
            HshToDkCompare(std::get<SampSeq>(Samps).CompareOp),
            {HshToDkBorderColor(std::get<SampSeq>(Samps).BorderColor),
             HshToDkBorderColor(std::get<SampSeq>(Samps).BorderColor),
             HshToDkBorderColor(std::get<SampSeq>(Samps).BorderColor),
             HshToDkBorderAlpha(std::get<SampSeq>(Samps).BorderColor)},
            1.f,
            DkSamplerReduction_WeightedAverage}...},
        PrimitiveRestart{PipelineInfo.Topology == TriangleStrip} {}

  constexpr ShaderConstData(
      std::array<ShaderCode<Target::DEKO3D>, NStages> S,
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
};

template <std::uint32_t NStages, std::uint32_t NSamplers>
struct ShaderData<Target::DEKO3D, NStages, NSamplers> {
  using ObjectRef = ShaderObject<Target::DEKO3D> *;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = SamplerObject<Target::DEKO3D> *;
  TargetTraits<Target::DEKO3D>::Pipeline Pipeline;
  template <std::size_t... StSeq>
  constexpr ShaderData(std::array<ObjectRef, NStages> S,
                       std::array<SamplerRef, NSamplers> Samps,
                       std::index_sequence<StSeq...>) noexcept
      : ShaderObjects(S), Pipeline{NStages, {&std::get<StSeq>(S)->Shader...}} {}
  constexpr ShaderData(std::array<ObjectRef, NStages> S,
                       std::array<SamplerRef, NSamplers> Samps) noexcept
      : ShaderData(S, Samps, std::make_index_sequence<NStages>()) {}
  template <std::size_t... StSeq>
  void InitializeShaders(const std::array<StageCode, NStages> &StageCodes,
                         std::index_sequence<StSeq...>) noexcept {
    (std::get<StSeq>(StageCodes)
         .Initialize(std::get<StSeq>(ShaderObjects)->Shader),
     ...);
  }
  void
  InitializeShaders(const std::array<StageCode, NStages> &StageCodes) noexcept {
    InitializeShaders(StageCodes, std::make_index_sequence<NStages>());
  }
  void Destroy() noexcept {}
};

template <> struct PipelineBuilder<Target::DEKO3D> {
  template <typename... B, std::size_t... BSeq>
  static void CreatePipelines(std::index_sequence<BSeq...> seq) noexcept {
    (B::data_DEKO3D.InitializeShaders(B::cdata_DEKO3D.StageCodes), ...);
  }
  template <typename... B> static void CreatePipelines() noexcept {
    CreatePipelines<B...>(std::make_index_sequence<sizeof...(B)>());
  }
  template <typename... B> static void DestroyPipelines() noexcept {
    (B::data_DEKO3D.Destroy(), ...);
  }
};

namespace buffer_math::deko {
using BufferImageCopy = detail::deko::BufferImageCopy;
template <uint32_t... Idx>
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies1D(uint32_t width, uint32_t layers, uint32_t texelSize,
             uint32_t texelShift,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {BufferImageCopy{
      MipOffset1D(width, layers, texelSize, texelShift, Idx), width >> Idx, 1,
      DkImageRect{0, 0, 0, width >> Idx, 1, layers}, Idx}...};
}
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies1D(uint32_t width, uint32_t layers, uint32_t texelSize,
             uint32_t texelShift) noexcept {
  return MakeCopies1D(width, layers, texelSize, texelShift,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}

template <uint32_t... Idx>
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies2D(uint32_t width, uint32_t height, uint32_t layers,
             uint32_t texelSize, uint32_t texelShift,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {BufferImageCopy{
      MipOffset2D(width, height, layers, texelSize, texelShift, Idx),
      width >> Idx, height >> Idx,
      DkImageRect{0, 0, 0, width >> Idx, height >> Idx, layers}, Idx}...};
}
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies2D(uint32_t width, uint32_t height, uint32_t layers,
             uint32_t texelSize, uint32_t texelShift) noexcept {
  return MakeCopies2D(width, height, layers, texelSize, texelShift,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}

template <uint32_t... Idx>
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies3D(uint32_t width, uint32_t height, uint32_t depth,
             uint32_t texelSize, uint32_t texelShift,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {BufferImageCopy{
      MipOffset3D(width, height, depth, texelSize, texelShift, Idx),
      width >> Idx, height >> Idx,
      DkImageRect{0, 0, 0, width >> Idx, height >> Idx, depth >> Idx}, Idx}...};
}
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies3D(uint32_t width, uint32_t height, uint32_t depth,
             uint32_t texelSize, uint32_t texelShift) noexcept {
  return MakeCopies3D(width, height, depth, texelSize, texelShift,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}
} // namespace buffer_math::deko

template <typename CopyFunc>
inline auto CreateBufferOwner(uint32_t size, uint32_t alignment,
                              CopyFunc copyFunc) noexcept {
  auto Ret = deko::AllocateStaticBuffer(size, alignment);
  copyFunc(Ret.GetMappedData(), size);
  return Ret;
}

inline auto CreateDynamicBufferOwner(uint32_t size,
                                     uint32_t alignment) noexcept {
  return deko::AllocateDynamicBuffer(size, alignment);
}

inline auto CreateFifoOwner(uint32_t size, uint32_t alignment) noexcept {
  return deko::AllocateFifoBuffer(size, alignment);
}

template <typename T>
struct TargetTraits<Target::DEKO3D>::ResourceFactory<uniform_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location,
                     CopyFunc copyFunc) noexcept {
    return CreateBufferOwner(sizeof(T), DK_UNIFORM_BUF_ALIGNMENT, copyFunc);
  }

  static auto CreateDynamic(const SourceLocation &location) noexcept {
    return CreateDynamicBufferOwner(sizeof(T), DK_UNIFORM_BUF_ALIGNMENT);
  }

  static auto CreateDynamic(const SourceLocation &location,
                            size_t size) noexcept {
    return CreateDynamicBufferOwner(size, DK_UNIFORM_BUF_ALIGNMENT);
  }
};

template <typename T>
struct TargetTraits<Target::DEKO3D>::ResourceFactory<vertex_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, std::size_t Count,
                     CopyFunc copyFunc) noexcept {
    return CreateBufferOwner(sizeof(T) * Count, 4, copyFunc);
  }

  static auto CreateDynamic(const SourceLocation &location,
                            std::size_t Count) noexcept {
    return CreateDynamicBufferOwner(sizeof(T) * Count, 4);
  }
};

template <typename T>
struct TargetTraits<Target::DEKO3D>::ResourceFactory<index_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, std::size_t Count,
                     CopyFunc copyFunc) noexcept {
    return CreateBufferOwner(sizeof(T) * Count, 4, copyFunc);
  }

  static auto CreateDynamic(const SourceLocation &location,
                            std::size_t Count) noexcept {
    return CreateDynamicBufferOwner(sizeof(T) * Count, 4);
  }
};

template <> struct TargetTraits<Target::DEKO3D>::ResourceFactory<uniform_fifo> {
  static auto Create(const SourceLocation &location,
                     std::size_t Size) noexcept {
    return TargetTraits<Target::DEKO3D>::FifoOwner{
        CreateFifoOwner(Size, DK_UNIFORM_BUF_ALIGNMENT)};
  }
};

template <> struct TargetTraits<Target::DEKO3D>::ResourceFactory<vertex_fifo> {
  static auto Create(const SourceLocation &location,
                     std::size_t Size) noexcept {
    return TargetTraits<Target::DEKO3D>::FifoOwner{CreateFifoOwner(Size, 4)};
  }
};

template <> struct TargetTraits<Target::DEKO3D>::ResourceFactory<index_fifo> {
  static auto Create(const SourceLocation &location,
                     std::size_t Size) noexcept {
    return TargetTraits<Target::DEKO3D>::FifoOwner{CreateFifoOwner(Size, 4)};
  }
};

template <DkImageType Type> struct TextureTypeTraits {};

template <> struct TextureTypeTraits<DkImageType_1D> {
  static constexpr char Name[] = "Texture1D";
  using ExtentType = uint32_t;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize, uint32_t texelShift) {
    return buffer_math::deko::MakeCopies1D(extent, layers, texelSize,
                                           texelShift);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t texelShift,
                                  uint32_t mips) {
    return buffer_math::MipOffset1D(extent, layers, texelSize, texelShift,
                                    mips);
  }
  static constexpr uint32_t ExtentWidth(const ExtentType &Extent) noexcept {
    return Extent;
  }
  static constexpr uint32_t ExtentHeight(const ExtentType &Extent) noexcept {
    return 1;
  }
  static constexpr uint32_t ExtentDepth(const ExtentType &Extent) noexcept {
    return 1;
  }
};

template <> struct TextureTypeTraits<DkImageType_2D> {
  static constexpr char Name[] = "Texture2D";
  using ExtentType = extent2d;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize, uint32_t texelShift) {
    return buffer_math::deko::MakeCopies2D(extent.w, extent.h, layers,
                                           texelSize, texelShift);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t texelShift,
                                  uint32_t mips) {
    return buffer_math::MipOffset2D(extent.w, extent.h, layers, texelSize,
                                    texelShift, mips);
  }
  static constexpr uint32_t ExtentWidth(const ExtentType &Extent) noexcept {
    return Extent.w;
  }
  static constexpr uint32_t ExtentHeight(const ExtentType &Extent) noexcept {
    return Extent.h;
  }
  static constexpr uint32_t ExtentDepth(const ExtentType &Extent) noexcept {
    return 1;
  }
};

template <> struct TextureTypeTraits<DkImageType_3D> {
  static constexpr char Name[] = "Texture3D";
  using ExtentType = extent3d;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize, uint32_t texelShift) {
    return buffer_math::deko::MakeCopies3D(extent.w, extent.h, extent.d,
                                           texelSize, texelShift);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t texelShift,
                                  uint32_t mips) {
    return buffer_math::MipOffset3D(extent.w, extent.h, extent.d, texelSize,
                                    texelShift, mips);
  }
  static constexpr uint32_t ExtentWidth(const ExtentType &Extent) noexcept {
    return Extent.w;
  }
  static constexpr uint32_t ExtentHeight(const ExtentType &Extent) noexcept {
    return Extent.h;
  }
  static constexpr uint32_t ExtentDepth(const ExtentType &Extent) noexcept {
    return Extent.d;
  }
};

template <DkImageType Type, typename Traits = TextureTypeTraits<Type>,
          typename CopyFunc>
inline auto CreateTextureOwner(
    DkImageType imageViewType, typename Traits::ExtentType extent,
    uint32_t numLayers, Format format, uint32_t numMips, CopyFunc copyFunc,
    ColorSwizzle redSwizzle, ColorSwizzle greenSwizzle,
    ColorSwizzle blueSwizzle, ColorSwizzle alphaSwizzle) noexcept {
  auto TexelSize = HshFormatToTexelSize(format);
  auto TexelSizeShift = HshFormatToTexelSizeShift(format);
  auto TexelFormat = HshToDkFormat(format);
  std::array<deko::BufferImageCopy, MaxMipCount> Copies =
      Traits::MakeCopies(extent, numLayers, TexelSize, TexelSizeShift);
  auto BufferSize =
      Traits::MipOffset(extent, numLayers, TexelSize, TexelSizeShift, numMips);
  auto UploadBuffer = deko::AllocateUploadBuffer(BufferSize);
  copyFunc(UploadBuffer.GetMappedData(), BufferSize);

  TargetTraits<Target::DEKO3D>::TextureOwner Ret{
      deko::AllocateTexture(
          dk::ImageLayoutMaker{deko::Globals.Device}
              .setType(imageViewType)
              .setFlags(DkImageFlags_Usage2DEngine | DkImageFlags_HwCompression)
              .setFormat(TexelFormat)
              .setDimensions(Traits::ExtentWidth(extent),
                             Traits::ExtentHeight(extent),
                             Traits::ExtentDepth(extent))
              .setMipLevels(numMips),
          false),
      dk::ImageView{Ret.Allocation.GetImage()}
          .setType(imageViewType)
          .setFormat(TexelFormat)
          .setSwizzle(HshToDkImageSwizzle(redSwizzle, DkImageSwizzle_Red),
                      HshToDkImageSwizzle(greenSwizzle, DkImageSwizzle_Green),
                      HshToDkImageSwizzle(blueSwizzle, DkImageSwizzle_Blue),
                      HshToDkImageSwizzle(alphaSwizzle, DkImageSwizzle_Alpha))
          .setLayers(0, numLayers)
          .setMipLevels(0, numMips)};

  dk::ImageView CopyImageView = Ret.ImageView;
  for (uint32_t i = 0; i < numMips; ++i)
    Copies[i].Copy(UploadBuffer.GetBuffer().addr, CopyImageView);

  return Ret;
}

template <DkImageType Type, typename Traits = TextureTypeTraits<Type>>
inline auto CreateDynamicTextureOwner(DkImageType imageViewType,
                                      typename Traits::ExtentType extent,
                                      uint32_t numLayers, Format format,
                                      uint32_t numMips, ColorSwizzle redSwizzle,
                                      ColorSwizzle greenSwizzle,
                                      ColorSwizzle blueSwizzle,
                                      ColorSwizzle alphaSwizzle) noexcept {
  auto TexelSize = HshFormatToTexelSize(format);
  auto TexelSizeShift = HshFormatToTexelSizeShift(format);
  auto TexelFormat = HshToDkFormat(format);
  auto BufferSize =
      Traits::MipOffset(extent, numLayers, TexelSize, TexelSizeShift, numMips);
  auto UploadBuffer = deko::AllocateUploadBuffer(BufferSize);

  TargetTraits<Target::DEKO3D>::DynamicTextureOwner Ret{
      deko::AllocateTexture(
          dk::ImageLayoutMaker{deko::Globals.Device}
              .setType(imageViewType)
              .setFlags(DkImageFlags_Usage2DEngine | DkImageFlags_HwCompression)
              .setFormat(TexelFormat)
              .setDimensions(Traits::ExtentWidth(extent),
                             Traits::ExtentHeight(extent),
                             Traits::ExtentDepth(extent))
              .setMipLevels(numMips),
          false),
      std::move(UploadBuffer),
      Traits::MakeCopies(extent, numLayers, TexelSize, TexelSizeShift),
      dk::ImageView{Ret.Allocation.GetImage()}
          .setType(imageViewType)
          .setFormat(TexelFormat)
          .setSwizzle(HshToDkImageSwizzle(redSwizzle, DkImageSwizzle_Red),
                      HshToDkImageSwizzle(greenSwizzle, DkImageSwizzle_Green),
                      HshToDkImageSwizzle(blueSwizzle, DkImageSwizzle_Blue),
                      HshToDkImageSwizzle(alphaSwizzle, DkImageSwizzle_Alpha))
          .setLayers(0, numLayers)
          .setMipLevels(0, numMips)};

  return Ret;
}

template <> struct TargetTraits<Target::DEKO3D>::ResourceFactory<texture1d> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, uint32_t extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<DkImageType_1D>(
        DkImageType_1D, extent, 1, format, numMips, copyFunc, redSwizzle,
        greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, uint32_t extent,
                            Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<DkImageType_1D>(
        DkImageType_1D, extent, 1, format, numMips, redSwizzle, greenSwizzle,
        blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::DEKO3D>::ResourceFactory<texture1d_array> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, uint32_t extent,
                     uint32_t numLayers, Format format, uint32_t numMips,
                     CopyFunc copyFunc, ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<DkImageType_1D>(
        location, DkImageType_1DArray, extent, numLayers, format, numMips,
        copyFunc, redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, uint32_t extent,
                            uint32_t numLayers, Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<DkImageType_1D>(
        DkImageType_1DArray, extent, numLayers, format, numMips, redSwizzle,
        greenSwizzle, blueSwizzle, alphaSwizzle);
  }
};

template <> struct TargetTraits<Target::DEKO3D>::ResourceFactory<texture2d> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, extent2d extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<DkImageType_2D>(
        DkImageType_2D, extent, 1, format, numMips, copyFunc, redSwizzle,
        greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, extent2d extent,
                            Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<DkImageType_2D>(
        DkImageType_2D, extent, 1, format, numMips, redSwizzle, greenSwizzle,
        blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::DEKO3D>::ResourceFactory<texture2d_array> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, extent2d extent,
                     uint32_t numLayers, Format format, uint32_t numMips,
                     CopyFunc copyFunc, ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<DkImageType_2D>(
        location, DkImageType_2DArray, extent, numLayers, format, numMips,
        copyFunc, redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, extent2d extent,
                            uint32_t numLayers, Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<DkImageType_2D>(
        DkImageType_2DArray, extent, numLayers, format, numMips, redSwizzle,
        greenSwizzle, blueSwizzle, alphaSwizzle);
  }
};

template <> struct TargetTraits<Target::DEKO3D>::ResourceFactory<texture3d> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, extent3d extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<DkImageType_3D>(
        location, DkImageType_3D, extent, 1, format, numMips, copyFunc,
        redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, extent3d extent,
                            Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<DkImageType_3D>(
        DkImageType_3D, extent, 1, format, numMips, redSwizzle, greenSwizzle,
        blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::DEKO3D>::ResourceFactory<render_texture2d> {
  static auto Create(const SourceLocation &location, surface Surf,
                     uint32_t NumColorBindings = 0,
                     uint32_t NumDepthBindings = 0) noexcept {
    return TargetTraits<Target::DEKO3D>::RenderTextureOwner{
        std::make_unique<deko::RenderTextureAllocation>(
            Surf.Binding.get_DEKO3D().Allocation, NumColorBindings,
            NumDepthBindings)};
  }

  static auto Create(const SourceLocation &location, extent2d Extent,
                     uint32_t NumColorBindings = 0,
                     uint32_t NumDepthBindings = 0) noexcept {
    return TargetTraits<Target::DEKO3D>::RenderTextureOwner{
        std::make_unique<deko::RenderTextureAllocation>(
            Extent, NumColorBindings, NumDepthBindings)};
  }
};

template <> struct TargetTraits<Target::DEKO3D>::ResourceFactory<surface> {
  static auto Create(const SourceLocation &location, void *Surface,
                     std::function<void()> &&DeleterLambda,
                     const hsh::extent2d &RequestExtent) noexcept {
    return TargetTraits<Target::DEKO3D>::SurfaceOwner{
        std::make_unique<deko::SurfaceAllocation>(
            std::move(Surface), std::move(DeleterLambda), RequestExtent)};
  }
};

namespace deko {

using namespace std::literals;

struct MyDeviceMaker : dk::DeviceMaker {
  explicit MyDeviceMaker(DkDebugFunc ErrHandler) noexcept {
    setCbDebug(ErrHandler);
  }
};

struct MyCmdBufMaker : dk::CmdBufMaker {
  explicit MyCmdBufMaker(dk::Device Device,
                         DkCmdBufAddMemFunc AddMemHandler) noexcept
      : dk::CmdBufMaker(Device) {
    setCbAddMem(AddMemHandler);
  }
};

struct MyDmaAllocatorCreateInfo : DmaAllocatorCreateInfo {
  MyDmaAllocatorCreateInfo(dk::Device Device) noexcept
      : DmaAllocatorCreateInfo{
            DmaAllocatorCreateFlagBits(
                DMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT),
            Device,
            0,
            nullptr,
            nullptr,
            0,
            nullptr,
            nullptr} {}
};

} // namespace deko

#endif

} // namespace hsh::detail

#if HSH_ENABLE_DEKO3D

namespace hsh {
class deko_device_owner {
  friend deko_device_owner create_deko_device(DkDebugFunc ErrHandler,
                                              DkCmdBufAddMemFunc MemHandler,
                                              uint32_t CmdBufSize,
                                              uint32_t CopyCmdBufSize) noexcept;
  struct Data {
    dk::UniqueDevice Device;
    DmaAllocator DmaAllocatorObj = DMA_NULL_HANDLE;
    dk::UniqueQueue Queue;
    dk::UniqueMemBlock ShaderMem;
    dk::UniqueMemBlock CommandBufferMem;
    std::array<dk::UniqueCmdBuf, 3> CommandBuffers;
    std::array<dk::Fence, 3> CommandFences;
    dk::Fence ImageAcquireSem;
    std::array<detail::deko::DeletedResources, 2> DeletedResources;
    bool BuiltPipelines = false;

    ~Data() noexcept {
      Queue.waitIdle();
      if (BuiltPipelines) {
        hsh::detail::GlobalListNode<false>::DestroyAll(ActiveTarget::DEKO3D);
        hsh::detail::GlobalListNode<true>::DestroyAll(ActiveTarget::DEKO3D);
      }
      DeletedResources[0].Purge();
      DeletedResources[1].Purge();
      dmaDestroyAllocator(DmaAllocatorObj);
    }
  };
  std::unique_ptr<Data> Data;

public:
  deko_device_owner() noexcept = default;
  deko_device_owner(deko_device_owner &&) noexcept = default;
  deko_device_owner &operator=(deko_device_owner &&) noexcept = default;

  bool success() const noexcept { return Data.operator bool(); }
  operator bool() const noexcept { return success(); }

  void build_pipelines() noexcept {
    if (Data->BuiltPipelines)
      return;
    hsh::detail::GlobalListNode<true>::CreateAll(ActiveTarget::DEKO3D);
    hsh::detail::GlobalListNode<false>::CreateAll(ActiveTarget::DEKO3D);
    Data->BuiltPipelines = true;
  }

  using HighCompleteFunc = std::function<void()>;
  using ProgFunc = std::function<void(std::size_t, std::size_t)>;

  void build_pipelines(const HighCompleteFunc &HCF,
                       const ProgFunc &PF) noexcept {
    if (Data->BuiltPipelines)
      return;
    hsh::detail::GlobalListNode<true>::CreateAll(ActiveTarget::DEKO3D);
    HCF();
    std::size_t Count = hsh::detail::GlobalListNode<false>::CountAll();
    std::size_t I = 0;
    PF(I, Count);
    for (auto *Node = hsh::detail::GlobalListNode<false>::GetHead(); Node;
         Node = Node->GetNext()) {
      Node->Create(ActiveTarget::DEKO3D);
      PF(++I, Count);
    }
    Data->BuiltPipelines = true;
  }

  class pipeline_build_pump {
    std::size_t Count = 0;
    std::size_t I = 0;
    hsh::detail::GlobalListNode<false> *Node = nullptr;

  public:
    explicit pipeline_build_pump() noexcept
        : Count(hsh::detail::GlobalListNode<false>::CountAll()),
          Node(hsh::detail::GlobalListNode<false>::GetHead()) {}

    std::pair<std::size_t, std::size_t> get_progress() const noexcept {
      return {I, Count};
    }

    bool pump() noexcept {
      if (Node) {
        Node->Create(ActiveTarget::DEKO3D);
        Node = Node->GetNext();
        ++I;
        return Node != nullptr;
      }
      return false;
    }
  };

  pipeline_build_pump start_build_pipelines() noexcept {
    if (Data->BuiltPipelines)
      return pipeline_build_pump{};
    Data->BuiltPipelines = true;

    hsh::detail::GlobalListNode<true>::CreateAll(ActiveTarget::DEKO3D);

    return pipeline_build_pump{};
  }

  template <typename Func> void enter_draw_context(Func F) const noexcept {
    detail::deko::Globals.PreRender();
    F();
    detail::deko::Globals.PostRender();
  }

  void wait_idle() noexcept { Data->Queue.waitIdle(); }
};

inline deko_device_owner create_deko_device(DkDebugFunc ErrHandler,
                                            DkCmdBufAddMemFunc MemHandler,
                                            uint32_t CmdBufSize,
                                            uint32_t CopyCmdBufSize) noexcept {
  auto &Globals = detail::deko::Globals;
  deko_device_owner Ret{};
  Ret.Data = std::make_unique<struct deko_device_owner::Data>();
  Ret.Data->Device = detail::deko::MyDeviceMaker{ErrHandler}.create();
  if (!Ret.Data->Device) {
    ErrHandler(nullptr, "create_deko_device", DkResult_Fail,
               "unable to create device");
    return {};
  }
  Globals.Device = Ret.Data->Device;
  if (dmaCreateAllocator(
          detail::deko::MyDmaAllocatorCreateInfo{Ret.Data->Device},
          &Ret.Data->DmaAllocatorObj) != DkResult_Success) {
    ErrHandler(nullptr, "create_deko_device", DkResult_Fail,
               "unable to create allocator");
    return {};
  }
  Globals.Allocator = Ret.Data->DmaAllocatorObj;
  Ret.Data->Queue =
      dk::QueueMaker{Ret.Data->Device}
          .setFlags(DkQueueFlags_Graphics | DkQueueFlags_MediumPrio |
                    DkQueueFlags_EnableZcull)
          .create();
  if (!Ret.Data->Queue) {
    ErrHandler(nullptr, "create_deko_device", DkResult_Fail,
               "unable to create queue");
    return {};
  }
  Globals.Queue = Ret.Data->Queue;
  DkMemBlockMaker BlockMaker{Ret.Data->Device, __deko_shader_memblock_size,
                             DkMemBlockFlags_CpuUncached |
                                 DkMemBlockFlags_GpuCached |
                                 DkMemBlockFlags_Code,
                             (void *)&__deko_shader_begin};
  Ret.Data->ShaderMem = dk::MemBlock(dkMemBlockCreate(&BlockMaker));
  if (!Ret.Data->ShaderMem) {
    ErrHandler(nullptr, "create_deko_device", DkResult_Fail,
               "unable to create shader memory block");
    return {};
  }
  Globals.ShaderMem = Ret.Data->ShaderMem;
  Ret.Data->CommandBufferMem =
      dk::MemBlockMaker{Ret.Data->Device, CmdBufSize * 2 + CopyCmdBufSize}
          .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
          .create();
  if (!Ret.Data->CommandBufferMem) {
    ErrHandler(nullptr, "create_deko_device", DkResult_Fail,
               "unable to create command buffer memory block");
    return {};
  }
  detail::deko::MyCmdBufMaker CmdBufMaker{Ret.Data->Device, MemHandler};
  CmdBufMaker.setUserData((void *)"CommandBuffer0");
  Ret.Data->CommandBuffers[0] = CmdBufMaker.create();
  Ret.Data->CommandBuffers[0].addMemory(Ret.Data->CommandBufferMem, 0,
                                        CmdBufSize);
  CmdBufMaker.setUserData((void *)"CommandBuffer1");
  Ret.Data->CommandBuffers[1] = CmdBufMaker.create();
  Ret.Data->CommandBuffers[1].addMemory(Ret.Data->CommandBufferMem, CmdBufSize,
                                        CmdBufSize);
  CmdBufMaker.setUserData((void *)"CopyBuffer");
  Ret.Data->CommandBuffers[2] = CmdBufMaker.create();
  Ret.Data->CommandBuffers[2].addMemory(Ret.Data->CommandBufferMem,
                                        CmdBufSize * 2, CopyCmdBufSize);
  Globals.CommandBuffers[0] = Ret.Data->CommandBuffers[0];
  Globals.CommandBuffers[1] = Ret.Data->CommandBuffers[1];
  Globals.CommandFences[0] = &Ret.Data->CommandFences[0];
  Globals.CommandFences[1] = &Ret.Data->CommandFences[1];
  Globals.CopyCmd = Ret.Data->CommandBuffers[2];
  Globals.CopyFence = &Ret.Data->CommandFences[2];
  Globals.ImageAcquireSem = &Ret.Data->ImageAcquireSem;
  Globals.DeletedResourcesArr = &Ret.Data->DeletedResources;
  Globals.DeletedResourcesObj = &Ret.Data->DeletedResources[0];
  return Ret;
}
} // namespace hsh

#endif
