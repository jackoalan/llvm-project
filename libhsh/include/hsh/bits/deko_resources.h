#pragma once

#include <sstream>
#include <string_view>

namespace hsh::detail {

template <> struct ShaderCode<Target::DEKO3D> {
  enum Stage Stage = Stage::Vertex;
  void *Control = nullptr;
  ShaderDataBlob<uint8_t> Blob;
  constexpr ShaderCode() noexcept = default;
  constexpr ShaderCode(enum Stage Stage, void *Control,
                       ShaderDataBlob<uint8_t> Blob) noexcept
      : Stage(Stage), Control(Control), Blob(Blob) {}
};

#if HSH_ENABLE_DEKO3D

constexpr DkImageFormat HshToDkFormat(Format Format) noexcept {
  switch (Format) {
  case R8_UNORM:
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
  Pipeline = Impl::data_DEKO3D.Pipeline.get();
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
    Iterators Its(*this);
    (Its.Add(args), ...);
    NumUniformBuffers = Its.UniformBufferIt - Its.UniformBufferBegin;
    NumTextures = Its.TextureIt - Its.TextureBegin;
    NumVertexBuffers = Its.VertexBufferIt - Its.VertexBufferBegin;
  }
}

void TargetTraits<Target::DEKO3D>::PipelineBinding::Iterators::Add(
    uniform_buffer_typeless Ubo) noexcept {
  *UniformBufferIt++ = Ubo.Binding.get_DEKO3D().Buffer;
}
void TargetTraits<Target::DEKO3D>::PipelineBinding::Iterators::Add(
    vertex_buffer_typeless Vbo) noexcept {
  *VertexBufferIt++ = Vbo.Binding.get_DEKO3D().Buffer;
}
template <typename T>
void TargetTraits<Target::DEKO3D>::PipelineBinding::Iterators::Add(
    index_buffer<T> Ibo) noexcept {
  Index.Buffer = Ibo.Binding.get_DEKO3D().Buffer.addr;
  Index.Type = dk::index_type<T>::indexType;
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

extern const uint8_t __deko_shader;
extern const uint8_t __deko_control;

template <std::uint32_t NStages, std::uint32_t NBindings,
          std::uint32_t NAttributes, std::uint32_t NSamplers,
          std::uint32_t NAttachments>
struct ShaderConstData<Target::DEKO3D, NStages, NBindings, NAttributes,
                       NSamplers, NAttachments> {
  struct Sampler : dk::Sampler {
    void Initialize(dk::SamplerDescriptor &Out,
                    const dk::ImageView &ImageView) const noexcept {
      dk::Sampler ModSampler = *this;
      ModSampler.setLodClamp(0.f, ImageView.mipLevelCount);
      // TODO: Handle format-dependent border color
    }
  };

  std::array<dk::ShaderMaker, NStages> StageCodes;
  std::array<DkStage, NStages> Stages;
  std::array<DkVtxBufferState, NBindings> VertexBindingDescriptions;
  std::array<DkVtxAttribState, NAttributes> VertexAttributeDescriptions;
  std::array<dk::BlendState, NAttachments> TargetAttachments;
  DkPrimitive Primitive;
  uint32_t PatchSize;
  dk::RasterizerState RasterizationState;
  dk::DepthStencilState DepthStencilState;
  dk::ColorState ColorBlendState;
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
      : StageCodes{dk::ShaderMaker{deko::Globals.ShaderMem,
                                   std::get<SSeq>(S).Blob.Data - &__deko_shader}
                       .setControl(&__deko_control)
                       .setProgramId(
                           (std::get<SSeq>(S).Blob.Control - &__deko_control) /
                           64)...},
        Stages{HshToVkShaderStage(std::get<SSeq>(S).Stage)...},
        VertexBindingDescriptions{
            DkVtxBufferState{std::get<BSeq>(B).Stride,
                             HshToDkDivisor(std::get<BSeq>(B).InputRate)}...},
        VertexAttributeDescriptions{
            DkVtxAttribState{std::get<ASeq>(A).Binding,
                             {},
                             0,
                             std::get<ASeq>(A).Offset,
                             HshToDkVtxAttribSize(std::get<ASeq>(A).Format),
                             HshToDkVtxAttribType(std::get<ASeq>(A).Format),
                             {},
                             0}...},
        TargetAttachments{
            dk::BlendState{}
                .setOps(HshToDkBlendOp(std::get<AttSeq>(Atts).ColorBlendOp),
                        HshToDkBlendOp(std::get<AttSeq>(Atts).AlphaBlendOp))
                .setFactors(
                    HshToDkBlendFactor(
                        std::get<AttSeq>(Atts).SrcColorBlendFactor),
                    HshToDkBlendFactor(
                        std::get<AttSeq>(Atts).DstColorBlendFactor),
                    HshToDkBlendFactor(
                        std::get<AttSeq>(Atts).SrcAlphaBlendFactor),
                    HshToDkBlendFactor(
                        std::get<AttSeq>(Atts).DstAlphaBlendFactor))...},
        Primitive{HshToDkPrimitive(PipelineInfo.Topology)},
        PatchSize{PipelineInfo.PatchControlPoints},
        RasterizationState{dk::RasterizerState{}.setCullMode(
            HshToDkFace(PipelineInfo.CullMode))},
        DepthStencilState{
            dk::DepthStencilState{}
                .setDepthTestEnable(PipelineInfo.DepthCompare != Always)
                .setDepthWriteEnable(PipelineInfo.DepthWrite)
                .setDepthCompareOp(HshToDkCompare(PipelineInfo.DepthCompare))},
        ColorBlendState{
            dk::ColorState{}
                .setLogicOp(DkLogicOp_Clear)
                .setBlendEnableMask(
                    ((std::get<AttSeq>(Atts).blendEnabled() << AttSeq) | ...))},
        ColorWriteState{((HshToDkColorComponentFlags(
                              std::get<AttSeq>(Atts).ColorWriteComponents)
                          << (AttSeq * 4)) |
                         ...)},
        Samplers{
            dk::Sampler{}
                .setFilter(
                    HshToDkFilter(std::get<SampSeq>(Samps).MinFilter),
                    HshToDkFilter(std::get<SampSeq>(Samps).MagFilter),
                    HshToDkMipFilter(std::get<SampSeq>(Samps).MipmapMode))
                .setWrapMode(
                    HshToDkWrapMode(std::get<SampSeq>(Samps).AddressModeU),
                    HshToDkWrapMode(std::get<SampSeq>(Samps).AddressModeV),
                    HshToDkWrapMode(std::get<SampSeq>(Samps).AddressModeW))
                .setLodBias(std::get<SampSeq>(Samps).MipLodBias)
                .setDepthCompare(
                    std::get<SampSeq>(Samps).CompareOp != Never,
                    HshToDkCompare(std::get<SampSeq>(Samps).CompareOp))
                .setBorderColor(
                    HshToDkBorderColor(std::get<SampSeq>(Samps).BorderColor),
                    HshToDkBorderColor(std::get<SampSeq>(Samps).BorderColor),
                    HshToDkBorderColor(std::get<SampSeq>(Samps).BorderColor),
                    HshToDkBorderAlpha(
                        std::get<SampSeq>(Samps).BorderColor))...},
        PrimitiveRestart(PipelineInfo.Topology == TriangleStrip) {}

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
  using ObjectRef = std::reference_wrapper<ShaderObject<Target::DEKO3D>>;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = std::reference_wrapper<SamplerObject<Target::DEKO3D>>;
  std::array<const DkShader *, NStages> ShaderPointers;
  template <std::size_t... StSeq>
  constexpr ShaderData(std::array<ObjectRef, NStages> S,
                       std::array<SamplerRef, NSamplers> Samps,
                       std::index_sequence<StSeq...>) noexcept
      : ShaderObjects(S), ShaderPointers{&std::get<StSeq>(S).get().Shader...} {}
  constexpr ShaderData(std::array<ObjectRef, NStages> S,
                       std::array<SamplerRef, NSamplers> Samps) noexcept
      : ShaderData(S, Samps, std::make_index_sequence<NStages>()) {}
  template <std::size_t... StSeq>
  void InitializeShaders(const std::array<dk::ShaderMaker, NStages> &StageCodes,
                         std::index_sequence<StSeq...>) noexcept {
    (std::get<StSeq>(StageCodes)
         .initialize(std::get<StSeq>(ShaderObjects).get().Shader),
     ...);
  }
  void InitializeShaders(
      const std::array<dk::ShaderMaker, NStages> &StageCodes) noexcept {
    InitializeShaders(StageCodes, std::make_index_sequence<NStages>());
  }
  void Destroy() noexcept {}
};

template <> struct PipelineBuilder<Target::DEKO3D> {
  template <typename... B, std::size_t... BSeq>
  static void CreatePipelines(std::index_sequence<BSeq...> seq) noexcept {
    (std::get<BSeq>(
         B::data_DEKO3D.InitializeShaders(B::cdata_DEKO3D.StageCodes)),
     ...);
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
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {BufferImageCopy{
      MipOffset1D(width, layers, texelSize, Idx), width >> Idx, 1,
      DkImageRect{0, 0, 0, width >> Idx, 1, layers}, Idx}...};
}
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies1D(uint32_t width, uint32_t layers, uint32_t texelSize) noexcept {
  return MakeCopies1D(width, layers, texelSize,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}

template <uint32_t... Idx>
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies2D(uint32_t width, uint32_t height, uint32_t layers,
             uint32_t texelSize,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {BufferImageCopy{
      MipOffset2D(width, height, layers, texelSize, Idx), width >> Idx,
      height >> Idx, DkImageRect{0, 0, 0, width >> Idx, height >> Idx, layers},
      Idx}...};
}
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies2D(uint32_t width, uint32_t height, uint32_t layers,
             uint32_t texelSize) noexcept {
  return MakeCopies2D(width, height, layers, texelSize,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}

template <uint32_t... Idx>
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies3D(uint32_t width, uint32_t height, uint32_t depth,
             uint32_t texelSize,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {BufferImageCopy{
      MipOffset3D(width, height, depth, texelSize, Idx), width >> Idx,
      height >> Idx,
      DkImageRect{0, 0, 0, width >> Idx, height >> Idx, depth >> Idx}, Idx}...};
}
static constexpr std::array<BufferImageCopy, MaxMipCount>
MakeCopies3D(uint32_t width, uint32_t height, uint32_t depth,
             uint32_t texelSize) noexcept {
  return MakeCopies3D(width, height, depth, texelSize,
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

template <DkImageType Type> struct TextureTypeTraits {};

template <> struct TextureTypeTraits<DkImageType_1D> {
  static constexpr char Name[] = "Texture1D";
  using ExtentType = uint32_t;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize) {
    return buffer_math::deko::MakeCopies1D(extent, layers, texelSize);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t mips) {
    return buffer_math::MipOffset1D(extent, layers, texelSize, mips);
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
                                   uint32_t texelSize) {
    return buffer_math::deko::MakeCopies2D(extent.w, extent.h, layers,
                                           texelSize);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t mips) {
    return buffer_math::MipOffset2D(extent.w, extent.h, layers, texelSize,
                                    mips);
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
                                   uint32_t texelSize) {
    return buffer_math::deko::MakeCopies3D(extent.w, extent.h, extent.d,
                                           texelSize);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t mips) {
    return buffer_math::MipOffset3D(extent.w, extent.h, extent.d, texelSize,
                                    mips);
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
  auto TexelFormat = HshToDkFormat(format);
  std::array<deko::BufferImageCopy, MaxMipCount> Copies =
      Traits::MakeCopies(extent, numLayers, TexelSize);
  auto BufferSize = Traits::MipOffset(extent, numLayers, TexelSize, numMips);
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
  auto TexelFormat = HshToDkFormat(format);
  auto BufferSize = Traits::MipOffset(extent, numLayers, TexelSize, numMips);
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
      std::move(UploadBuffer), Traits::MakeCopies(extent, numLayers, TexelSize),
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
};

template <> struct TargetTraits<Target::DEKO3D>::ResourceFactory<surface> {
  static auto Create(const SourceLocation &location, void *Surface,
                     std::function<void(const hsh::extent2d &,
                                        const hsh::extent2d &)> &&ResizeLambda,
                     std::function<void()> &&DeleterLambda,
                     const hsh::extent2d &RequestExtent, int32_t L, int32_t R,
                     int32_t T, int32_t B) noexcept {
    return TargetTraits<Target::DEKO3D>::SurfaceOwner{
        std::make_unique<deko::SurfaceAllocation>(
            std::move(Surface), std::move(DeleterLambda), RequestExtent)};
  }
};

namespace deko {

using namespace std::literals;

using ErrorHandler = std::function<void(const char *Context, DkResult Result,
                                        const char *Message)>;

struct MyDeviceMaker : dk::DeviceMaker {
  static void DebugCallback(ErrorHandler *UserData, const char *Context,
                            DkResult Result, const char *Message) noexcept {
    (*UserData)(Context, Result, Message);
    if (Result != DkResult_Success)
      std::abort();
  }

  explicit MyDeviceMaker(ErrorHandler &ErrHandler) noexcept {
    setUserData(&ErrHandler);
    setCbDebug(reinterpret_cast<DkDebugFunc>(DebugCallback));
  }
};

using AddMemHandler = std::function<void(dk::CmdBuf Cmdbuf, size_t MinReqSize)>;

struct MyCmdBufMaker : dk::CmdBufMaker {
  static void AddMemCallback(AddMemHandler *UserData, DkCmdBuf Cmdbuf,
                             size_t MinReqSize) noexcept {
    (*UserData)(Cmdbuf, MinReqSize);
    std::abort();
  }

  explicit MyCmdBufMaker(dk::Device Device,
                         AddMemHandler &AddMemHandler) noexcept
      : dk::CmdBufMaker(Device) {
    setUserData(&AddMemHandler);
    setCbAddMem(reinterpret_cast<DkCmdBufAddMemFunc>(AddMemCallback));
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
  friend deko_device_owner
  create_deko_device(detail::deko::ErrorHandler &ErrHandler,
                     detail::deko::AddMemHandler &MemHandler) noexcept;
  struct Data {
    dk::UniqueDevice Device;
    dk::UniqueQueue Queue;
    DmaAllocator DmaAllocator = DMA_NULL_HANDLE;
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
      dmaDestroyAllocator(DmaAllocator);
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

inline deko_device_owner
create_deko_device(detail::deko::ErrorHandler &ErrHandler,
                   detail::deko::AddMemHandler &MemHandler) noexcept {
  deko_device_owner Ret{};
  Ret.Data = std::make_unique<struct deko_device_owner::Data>();
  Ret.Data->Device = detail::deko::MyDeviceMaker{ErrHandler}.create();
  if (!Ret.Data->Device)
    return {};
  Ret.Data->Queue = dk::QueueMaker{Ret.Data->Device}.create();
  if (!Ret.Data->Queue)
    return {};
  dmaCreateAllocator(detail::deko::MyDmaAllocatorCreateInfo{Ret.Data->Device},
                     &Ret.Data->DmaAllocator);
  detail::deko::MyCmdBufMaker CmdBufMaker{Ret.Data->Device, MemHandler};
  Ret.Data->CommandBuffers[0] = CmdBufMaker.create();
  Ret.Data->CommandBuffers[1] = CmdBufMaker.create();
  Ret.Data->CommandBuffers[2] = CmdBufMaker.create();
  return Ret;
}
} // namespace hsh

#endif
