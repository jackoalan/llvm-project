#pragma once

#include <sstream>
#include <string_view>

namespace hsh::detail {

template <> struct ShaderCode<Target::VULKAN_SPIRV> {
  enum Stage Stage = Stage::Vertex;
  ShaderDataBlob<uint32_t> Blob;
  constexpr ShaderCode() noexcept = default;
  constexpr ShaderCode(enum Stage Stage, ShaderDataBlob<uint32_t> Blob) noexcept
      : Stage(Stage), Blob(Blob) {}
};

#if HSH_ENABLE_VULKAN

constexpr vk::Format HshToVkFormat(Format Format) noexcept {
  switch (Format) {
  case R8_UNORM:
  default:
    return vk::Format::eR8Unorm;
  case RG8_UNORM:
    return vk::Format::eR8G8Unorm;
  case RGBA8_UNORM:
    return vk::Format::eR8G8B8A8Unorm;
  case R16_UNORM:
    return vk::Format::eR16Unorm;
  case RG16_UNORM:
    return vk::Format::eR16G16Unorm;
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
  case RGBA8_SNORM:
    return vk::Format::eR8G8B8A8Snorm;
  case R16_SNORM:
    return vk::Format::eR16Snorm;
  case RG16_SNORM:
    return vk::Format::eR16G16Snorm;
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
  case BC1_UNORM:
    return vk::Format::eBc1RgbaUnormBlock;
  case BC2_UNORM:
    return vk::Format::eBc2UnormBlock;
  case BC3_UNORM:
    return vk::Format::eBc3UnormBlock;
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
  case ConstColor:
    return vk::BlendFactor::eConstantColor;
  case InvConstColor:
    return vk::BlendFactor::eOneMinusConstantColor;
  case ConstAlpha:
    return vk::BlendFactor::eConstantAlpha;
  case InvConstAlpha:
    return vk::BlendFactor::eOneMinusConstantAlpha;
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
  default:
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

template <> struct ShaderObject<Target::VULKAN_SPIRV> {
  vk::UniqueShaderModule ShaderModule;
  ShaderObject() = default;
  vk::ShaderModule Get(const ShaderCode<Target::VULKAN_SPIRV> &Info,
                       const SourceLocation &Location) noexcept {
    if (!ShaderModule) {
      ShaderModule = vulkan::Globals.Device
                         .createShaderModuleUnique(vk::ShaderModuleCreateInfo{
                             {}, Info.Blob.Size, reloc(Info.Blob.Data)})
                         .value;
      vulkan::Globals.SetDebugObjectName(Location, ShaderModule.get());
    }
    return ShaderModule.get();
  }
  void Destroy() noexcept { ShaderModule.reset(); }
};

template <> struct SamplerObject<Target::VULKAN_SPIRV> {
  std::array<std::array<vk::UniqueSampler, MaxMipCount - 1>, 2> Samplers;
  SamplerObject() = default;
  vk::Sampler Get(const vk::SamplerCreateInfo &Info, bool Int,
                  unsigned MipCount, const SourceLocation &Location) noexcept {
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
      vulkan::Globals.SetDebugObjectName(Location, Samp.get());
    }
    return Samp.get();
  }
  vk::Sampler Get(const vk::SamplerCreateInfo &Info, texture_typeless tex,
                  const SourceLocation &Location) noexcept {
    return Get(Info, tex.Binding.get_VULKAN_SPIRV().Integer,
               tex.Binding.get_VULKAN_SPIRV().NumMips, Location);
  }
  void Destroy() noexcept {
    for (auto &SampI : Samplers)
      for (auto &Samp : SampI)
        Samp.reset();
  }
};

namespace vulkan {
template <typename Impl> struct DescriptorPoolWrites {
  uint32_t NumWrites = 0;
  std::array<VkWriteDescriptorSet, MaxUniforms + MaxImages + MaxSamplers>
      Writes;
  std::array<VkDescriptorBufferInfo, MaxUniforms> Uniforms;
  std::array<VkDescriptorImageInfo, MaxImages> Images;
  std::array<VkDescriptorImageInfo, MaxSamplers> Samplers;
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
    (Its.Add(args), ...);
    NumWrites = uint32_t(Its.WriteIt - Its.WriteBegin);
  }

  struct Iterators {
    vk::DescriptorSet DstSet;
    typename decltype(Writes)::iterator WriteBegin;
    typename decltype(Uniforms)::iterator UniformBegin;
    typename decltype(Images)::iterator ImageBegin;
    typename decltype(Samplers)::iterator SamplerBegin;
    typename decltype(Writes)::iterator WriteIt;
    typename decltype(Uniforms)::iterator UniformIt;
    typename decltype(Images)::iterator ImageIt;
    typename decltype(Samplers)::iterator SamplerIt;
    constexpr explicit Iterators(vk::DescriptorSet DstSet,
                                 DescriptorPoolWrites &Writes) noexcept
        : DstSet(DstSet), WriteBegin(Writes.Writes.begin()),
          UniformBegin(Writes.Uniforms.begin()),
          ImageBegin(Writes.Images.begin()),
          SamplerBegin(Writes.Samplers.begin()), WriteIt(Writes.Writes.begin()),
          UniformIt(Writes.Uniforms.begin()), ImageIt(Writes.Images.begin()),
          SamplerIt(Writes.Samplers.begin()) {}
    void Add(uniform_buffer_typeless uniform) noexcept {
      auto UniformIdx = uint32_t(UniformIt - UniformBegin);
      auto &Uniform = *UniformIt++;
      const auto &Binding = uniform.Binding.get_VULKAN_SPIRV();
      Uniform =
          vk::DescriptorBufferInfo(Binding, Binding.Offset, VK_WHOLE_SIZE);
      auto &Write = *WriteIt++;
      Write = vk::WriteDescriptorSet(
          DstSet, UniformIdx, 0, 1, vk::DescriptorType::eUniformBufferDynamic,
          {}, reinterpret_cast<vk::DescriptorBufferInfo *>(&Uniform));
    }
    static void Add(vertex_buffer_typeless) noexcept {}
    static void Add(index_buffer_typeless) noexcept {}
    void Add(texture_typeless texture) noexcept {
      auto ImageIdx = uint32_t(ImageIt - ImageBegin);
      auto &Image = *ImageIt++;
      Image = vk::DescriptorImageInfo(
          {}, texture.Binding.get_VULKAN_SPIRV().ImageView,
          vk::ImageLayout::eShaderReadOnlyOptimal);
      auto &Write = *WriteIt++;
      Write = vk::WriteDescriptorSet(
          DstSet, MaxUniforms + ImageIdx, 0, 1,
          vk::DescriptorType::eSampledImage,
          reinterpret_cast<vk::DescriptorImageInfo *>(&Image));
    }
    void Add(render_texture2d texture) noexcept {
      auto ImageIdx = uint32_t(ImageIt - ImageBegin);
      auto &Image = *ImageIt++;
      Image = vk::DescriptorImageInfo(
          {}, texture.Binding.get_VULKAN_SPIRV().GetImageView(),
          vk::ImageLayout::eShaderReadOnlyOptimal);
      auto &Write = *WriteIt++;
      Write = vk::WriteDescriptorSet(
          DstSet, MaxUniforms + ImageIdx, 0, 1,
          vk::DescriptorType::eSampledImage,
          reinterpret_cast<vk::DescriptorImageInfo *>(&Image));
    }
    void Add(hsh::detail::SamplerBinding sampler) noexcept {
      auto SamplerIdx = uint32_t(SamplerIt - SamplerBegin);
      auto &Sampler = *SamplerIt++;
      Sampler = vk::DescriptorImageInfo(
          Impl::data_VULKAN_SPIRV.SamplerObjects[sampler.Idx]->Get(
              Impl::cdata_VULKAN_SPIRV.Samplers[sampler.Idx], sampler.Tex,
              Impl::cdata_VULKAN_SPIRV.Location.with_field("Sampler",
                                                           SamplerIdx)));
      auto &Write = *WriteIt++;
      Write = vk::WriteDescriptorSet(
          DstSet, MaxUniforms + MaxImages + SamplerIdx, 0, 1,
          vk::DescriptorType::eSampler,
          reinterpret_cast<vk::DescriptorImageInfo *>(&Sampler));
    }
  };
};
} // namespace vulkan

template <typename Impl, typename... Args>
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::Rebind(
    bool UpdateDescriptors, Args... args) noexcept {
  Pipeline = Impl::data_VULKAN_SPIRV.Pipeline.get();
  if (UpdateDescriptors) {
    if (!DescriptorSet)
      DescriptorSet = vulkan::Globals.DescriptorPoolChain->Allocate();
    vulkan::DescriptorPoolWrites<Impl> Writes(DescriptorSet, args...);
    vulkan::Globals.Device.updateDescriptorSets(
        Writes.NumWrites,
        reinterpret_cast<vk::WriteDescriptorSet *>(Writes.Writes.data()), 0,
        nullptr);
    Iterators Its(*this);
    (Its.Add(args), ...);
    NumVertexBuffers = uint32_t(Its.VertexBufferIt - Its.VertexBufferBegin);
  }
  OffsetIterators OffIts(*this);
  (OffIts.Add(args), ...);
}

void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::Iterators::Add(
    uniform_buffer_typeless) noexcept {}
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::Iterators::Add(
    vertex_buffer_typeless vbo) noexcept {
  *VertexBufferIt++ = vbo.Binding.get_VULKAN_SPIRV();
}
template <typename T>
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::Iterators::Add(
    index_buffer<T> ibo) noexcept {
  Index.Buffer = ibo.Binding.get_VULKAN_SPIRV();
  Index.Type = vk::IndexTypeValue<T>::value;
}
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::Iterators::Add(
    texture_typeless) noexcept {
  TextureIdx++;
}
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::Iterators::Add(
    render_texture2d texture) noexcept {
  auto &RT = *RenderTextureIt++;
  RT.RenderTextureBinding = texture.Binding.get_VULKAN_SPIRV();
  RT.KnownImageView = RT.RenderTextureBinding.GetImageView();
  RT.DescriptorBindingIdx = MaxUniforms + TextureIdx++;
}
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::Iterators::Add(
    SamplerBinding) noexcept {}

void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::OffsetIterators::Add(
    uniform_buffer_typeless ubo) noexcept {
  const auto &Uniform = ubo.Binding.get_VULKAN_SPIRV();
  *UniformOffsetsIt++ = Uniform.Offset;
}
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::OffsetIterators::Add(
    vertex_buffer_typeless vbo) noexcept {
  const auto &Vertex = vbo.Binding.get_VULKAN_SPIRV();
  *VertexOffsetsIt++ = Vertex.Offset;
}
template <typename T>
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::OffsetIterators::Add(
    index_buffer<T> ibo) noexcept {
  const auto &Idx = ibo.Binding.get_VULKAN_SPIRV();
  Index.Offset = Idx.Offset;
}
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::OffsetIterators::Add(
    texture_typeless) noexcept {}
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::OffsetIterators::Add(
    render_texture2d) noexcept {}
void TargetTraits<Target::VULKAN_SPIRV>::PipelineBinding::OffsetIterators::Add(
    SamplerBinding) noexcept {}

template <std::uint32_t NStages, std::uint32_t NBindings,
          std::uint32_t NAttributes, std::uint32_t NSamplers,
          std::uint32_t NAttachments>
struct ShaderConstData<Target::VULKAN_SPIRV, NStages, NBindings, NAttributes,
                       NSamplers, NAttachments> {
  std::array<ShaderCode<Target::VULKAN_SPIRV>, NStages> StageCodes;
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
  SourceLocation Location;
  bool DirectRenderPass;

  template <std::size_t... SSeq, std::size_t... BSeq, std::size_t... ASeq,
            std::size_t... SampSeq, std::size_t... AttSeq>
  constexpr ShaderConstData(
      std::array<ShaderCode<Target::VULKAN_SPIRV>, NStages> S,
      std::array<VertexBinding, NBindings> B,
      std::array<VertexAttribute, NAttributes> A,
      std::array<sampler, NSamplers> Samps,
      std::array<ColorAttachment, NAttachments> Atts,
      struct PipelineInfo PipelineInfo, const SourceLocation &Location,
      std::index_sequence<SSeq...>, std::index_sequence<BSeq...>,
      std::index_sequence<ASeq...>, std::index_sequence<SampSeq...>,
      std::index_sequence<AttSeq...>) noexcept
      : StageCodes(S), StageFlags{HshToVkShaderStage(
                           std::get<SSeq>(S).Stage)...},
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
                               false)}...},
        Location(Location), DirectRenderPass(PipelineInfo.DirectRenderPass) {}

  constexpr ShaderConstData(
      std::array<ShaderCode<Target::VULKAN_SPIRV>, NStages> S,
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

  static constexpr std::array<vk::DynamicState, 3> Dynamics{
      vk::DynamicState::eViewport, vk::DynamicState::eScissor,
      vk::DynamicState::eBlendConstants};
  static constexpr vk::PipelineDynamicStateCreateInfo DynamicState{
      {}, uint32_t(Dynamics.size()), Dynamics.data()};
  static constexpr vk::PipelineViewportStateCreateInfo ViewportState{
      {}, 1, {}, 1, {}};

  template <typename B>
  vk::GraphicsPipelineCreateInfo
  GetPipelineInfo(VkPipelineShaderStageCreateInfo *StageInfos) const noexcept {
    for (std::size_t i = 0; i < NStages; ++i)
      StageInfos[i] = vk::PipelineShaderStageCreateInfo {
        {}, StageFlags[i],
            B::data_VULKAN_SPIRV.ShaderObjects[i]->Get(
                StageCodes[i],
#if HSH_SOURCE_LOCATION_ENABLED
                B::cdata_VULKAN_SPIRV.Location.with_field(
                    vk::to_string(StageFlags[i]).c_str())
#else
                B::cdata_VULKAN_SPIRV.Location
#endif
                    ),
            "main"
      };

#if _MSC_VER
    /* Thanks for screwing up inter-field constexpr references MS <3 */
    static auto VertexInputStateTmp = VertexInputState;
    VertexInputStateTmp.pVertexBindingDescriptions =
        VertexBindingDescriptions.data();
    VertexInputStateTmp.pVertexAttributeDescriptions =
        VertexAttributeDescriptions.data();
    static auto ColorBlendStateTmp = ColorBlendState;
    ColorBlendStateTmp.pAttachments = TargetAttachments.data();
#endif

    return vk::GraphicsPipelineCreateInfo {
      {}, NStages,
          reinterpret_cast<vk::PipelineShaderStageCreateInfo *>(StageInfos),
#if _MSC_VER
          &VertexInputStateTmp,
#else
          &VertexInputState,
#endif
          &InputAssemblyState, &TessellationState, &ViewportState,
          &RasterizationState, &vulkan::Globals.MultisampleState,
          &DepthStencilState,
#if _MSC_VER
          &ColorBlendStateTmp,
#else
          &ColorBlendState,
#endif
          &DynamicState, vulkan::Globals.PipelineLayout,
          DirectRenderPass ? vulkan::Globals.GetDirectRenderPass()
                           : vulkan::Globals.GetRenderPass()
    };
  }
};

template <std::uint32_t NStages, std::uint32_t NSamplers>
struct ShaderData<Target::VULKAN_SPIRV, NStages, NSamplers> {
  using ObjectRef = ShaderObject<Target::VULKAN_SPIRV> *;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = SamplerObject<Target::VULKAN_SPIRV> *;
  std::array<SamplerRef, NSamplers> SamplerObjects;
  vk::UniquePipeline Pipeline;
  constexpr ShaderData(std::array<ObjectRef, NStages> S,
                       std::array<SamplerRef, NSamplers> Samps) noexcept
      : ShaderObjects(S), SamplerObjects(Samps) {}
  void Destroy() noexcept {
    for (auto &Obj : ShaderObjects)
      Obj->Destroy();
    for (auto &Obj : SamplerObjects)
      Obj->Destroy();
    Pipeline.reset();
  }
};

template <> struct PipelineBuilder<Target::VULKAN_SPIRV> {
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
    vulkan::Globals.SetDebugObjectName(
        B::cdata_VULKAN_SPIRV.Location.with_field("Pipeline"), data);
    vk::ObjectDestroy<vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(
        vulkan::Globals.Device, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    B::data_VULKAN_SPIRV.Pipeline = vk::UniquePipeline(data, deleter);
  }
  template <typename... B, std::size_t... BSeq>
  static void CreatePipelines(std::index_sequence<BSeq...> seq) noexcept {
    std::array<VkPipelineShaderStageCreateInfo, (GetNumStages<B>(true) + ...)>
        ShaderStageInfos;
    std::array<vk::GraphicsPipelineCreateInfo, sizeof...(B)> Infos{
        B::cdata_VULKAN_SPIRV.template GetPipelineInfo<B>(
            ShaderStageInfos.data() + StageInfoStart<B...>(BSeq, seq))...};
    std::array<vk::Pipeline, sizeof...(B)> Pipelines;
    auto Result = vulkan::Globals.Device.createGraphicsPipelines(
        vulkan::Globals.PipelineCache, uint32_t(Infos.size()), Infos.data(),
        nullptr, Pipelines.data());
    HSH_ASSERT_VK_SUCCESS(Result);
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
template <uint32_t... Idx>
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies1D(uint32_t width, uint32_t layers, uint32_t texelSize,
             uint32_t texelShift, vk::ImageAspectFlagBits aspect,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {vk::BufferImageCopy(
      MipOffset1D(width, layers, texelSize, texelShift, Idx), width >> Idx, 1,
      {aspect, Idx, 0, layers}, {}, {width >> Idx, 1, 1})...};
}
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies1D(uint32_t width, uint32_t layers, uint32_t texelSize,
             uint32_t texelShift, vk::ImageAspectFlagBits aspect) noexcept {
  return MakeCopies1D(width, layers, texelSize, texelShift, aspect,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}

template <uint32_t... Idx>
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies2D(uint32_t width, uint32_t height, uint32_t layers,
             uint32_t texelSize, uint32_t texelShift,
             vk::ImageAspectFlagBits aspect,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {vk::BufferImageCopy(
      MipOffset2D(width, height, layers, texelSize, texelShift, Idx),
      width >> Idx, height >> Idx, {aspect, Idx, 0, layers}, {},
      {width >> Idx, height >> Idx, 1})...};
}
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies2D(uint32_t width, uint32_t height, uint32_t layers,
             uint32_t texelSize, uint32_t texelShift,
             vk::ImageAspectFlagBits aspect) noexcept {
  return MakeCopies2D(width, height, layers, texelSize, texelShift, aspect,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}

template <uint32_t... Idx>
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies3D(uint32_t width, uint32_t height, uint32_t depth,
             uint32_t texelSize, uint32_t texelShift,
             vk::ImageAspectFlagBits aspect,
             std::integer_sequence<uint32_t, Idx...>) noexcept {
  return {vk::BufferImageCopy(
      MipOffset3D(width, height, depth, texelSize, texelShift, Idx),
      width >> Idx, height >> Idx, {aspect, Idx, 0, 1}, {},
      {width >> Idx, height >> Idx, depth >> Idx})...};
}
static constexpr std::array<vk::BufferImageCopy, MaxMipCount>
MakeCopies3D(uint32_t width, uint32_t height, uint32_t depth,
             uint32_t texelSize, uint32_t texelShift,
             vk::ImageAspectFlagBits aspect) noexcept {
  return MakeCopies3D(width, height, depth, texelSize, texelShift, aspect,
                      std::make_integer_sequence<uint32_t, MaxMipCount>());
}
} // namespace buffer_math::vulkan

template <typename CopyFunc>
inline auto CreateBufferOwner(const SourceLocation &location,
                              vk::BufferUsageFlagBits bufferType,
                              std::size_t size, CopyFunc copyFunc) noexcept {
  auto UploadBuffer = vulkan::AllocateUploadBuffer(location, size);
  copyFunc(UploadBuffer.GetMappedData(), size);

  auto Ret = vulkan::AllocateStaticBuffer(
      location, size, bufferType | vk::BufferUsageFlagBits::eTransferDst);

  vulkan::Globals.CopyCmd.copyBuffer(UploadBuffer.GetBuffer(), Ret.GetBuffer(),
                                     vk::BufferCopy(0, 0, size));

  return Ret;
}

inline auto CreateDynamicBufferOwner(const SourceLocation &location,
                                     vk::BufferUsageFlagBits bufferType,
                                     std::size_t size) noexcept {
  return vulkan::AllocateDynamicBuffer(
      location, size, bufferType | vk::BufferUsageFlagBits::eTransferDst);
}

inline auto CreateFifoOwner(const SourceLocation &location,
                            vk::BufferUsageFlagBits bufferType,
                            std::size_t size) noexcept {
  return vulkan::AllocateFifoBuffer(location, size, bufferType);
}

template <typename T>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<uniform_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location,
                     CopyFunc copyFunc) noexcept {
    return CreateBufferOwner(location, vk::BufferUsageFlagBits::eUniformBuffer,
                             sizeof(T), copyFunc);
  }

  static auto CreateDynamic(const SourceLocation &location) noexcept {
    return CreateDynamicBufferOwner(
        location, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(T));
  }

  static auto CreateDynamic(const SourceLocation &location,
                            size_t size) noexcept {
    return CreateDynamicBufferOwner(
        location, vk::BufferUsageFlagBits::eUniformBuffer, size);
  }
};

template <typename T>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<vertex_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, std::size_t Count,
                     CopyFunc copyFunc) noexcept {
    return CreateBufferOwner(location, vk::BufferUsageFlagBits::eVertexBuffer,
                             sizeof(T) * Count, copyFunc);
  }

  static auto CreateDynamic(const SourceLocation &location,
                            std::size_t Count) noexcept {
    return CreateDynamicBufferOwner(
        location, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(T) * Count);
  }
};

template <typename T>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<index_buffer<T>> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, std::size_t Count,
                     CopyFunc copyFunc) noexcept {
    return CreateBufferOwner(location, vk::BufferUsageFlagBits::eIndexBuffer,
                             sizeof(T) * Count, copyFunc);
  }

  static auto CreateDynamic(const SourceLocation &location,
                            std::size_t Count) noexcept {
    return CreateDynamicBufferOwner(
        location, vk::BufferUsageFlagBits::eIndexBuffer, sizeof(T) * Count);
  }
};

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<uniform_fifo> {
  static auto Create(const SourceLocation &location,
                     std::size_t Size) noexcept {
    return TargetTraits<Target::VULKAN_SPIRV>::FifoOwner{CreateFifoOwner(
        location, vk::BufferUsageFlagBits::eUniformBuffer, Size)};
  }
};

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<vertex_fifo> {
  static auto Create(const SourceLocation &location,
                     std::size_t Size) noexcept {
    return TargetTraits<Target::VULKAN_SPIRV>::FifoOwner{CreateFifoOwner(
        location, vk::BufferUsageFlagBits::eVertexBuffer, Size)};
  }
};

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<index_fifo> {
  static auto Create(const SourceLocation &location,
                     std::size_t Size) noexcept {
    return TargetTraits<Target::VULKAN_SPIRV>::FifoOwner{
        CreateFifoOwner(location, vk::BufferUsageFlagBits::eIndexBuffer, Size)};
  }
};

template <vk::ImageType Type> struct TextureTypeTraits {};

template <> struct TextureTypeTraits<vk::ImageType::e1D> {
  static constexpr char Name[] = "Texture1D";
  using ExtentType = uint32_t;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize, uint32_t texelShift) {
    return buffer_math::vulkan::MakeCopies1D(
        extent, layers, texelSize, texelShift, vk::ImageAspectFlagBits::eColor);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t texelShift,
                                  uint32_t mips) {
    return buffer_math::MipOffset1D(extent, layers, texelSize, texelShift,
                                    mips);
  }
};

template <> struct TextureTypeTraits<vk::ImageType::e2D> {
  static constexpr char Name[] = "Texture2D";
  using ExtentType = extent2d;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize, uint32_t texelShift) {
    return buffer_math::vulkan::MakeCopies2D(extent.w, extent.h, layers,
                                             texelSize, texelShift,
                                             vk::ImageAspectFlagBits::eColor);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t texelShift,
                                  uint32_t mips) {
    return buffer_math::MipOffset2D(extent.w, extent.h, layers, texelSize,
                                    texelShift, mips);
  }
};

template <> struct TextureTypeTraits<vk::ImageType::e3D> {
  static constexpr char Name[] = "Texture3D";
  using ExtentType = extent3d;
  static constexpr auto MakeCopies(ExtentType extent, uint32_t layers,
                                   uint32_t texelSize, uint32_t texelShift) {
    return buffer_math::vulkan::MakeCopies3D(extent.w, extent.h, extent.d,
                                             texelSize, texelShift,
                                             vk::ImageAspectFlagBits::eColor);
  }
  static constexpr auto MipOffset(ExtentType extent, uint32_t layers,
                                  uint32_t texelSize, uint32_t texelShift,
                                  uint32_t mips) {
    return buffer_math::MipOffset3D(extent.w, extent.h, extent.d, texelSize,
                                    texelShift, mips);
  }
};

template <vk::ImageType Type, typename Traits = TextureTypeTraits<Type>,
          typename CopyFunc>
inline auto CreateTextureOwner(
    const SourceLocation &location, vk::ImageViewType imageViewType,
    typename Traits::ExtentType extent, uint32_t numLayers, Format format,
    uint32_t numMips, CopyFunc copyFunc, ColorSwizzle redSwizzle,
    ColorSwizzle greenSwizzle, ColorSwizzle blueSwizzle,
    ColorSwizzle alphaSwizzle) noexcept {
  auto TexelSize = uint32_t(HshFormatToTexelSize(format));
  auto TexelSizeShift = uint32_t(HshFormatToTexelSizeShift(format));
  auto TexelFormat = HshToVkFormat(format);
  std::array<vk::BufferImageCopy, MaxMipCount> Copies =
      Traits::MakeCopies(extent, numLayers, TexelSize, TexelSizeShift);
  auto BufferSize =
      Traits::MipOffset(extent, numLayers, TexelSize, TexelSizeShift, numMips);
  auto UploadBuffer = vulkan::AllocateUploadBuffer(location, BufferSize);
  copyFunc(UploadBuffer.GetMappedData(), BufferSize);

  TargetTraits<Target::VULKAN_SPIRV>::TextureOwner Ret{
      vulkan::AllocateTexture(
          location.with_field(Traits::Name),
          vk::ImageCreateInfo({}, Type, TexelFormat, extent3d(extent), numMips,
                              numLayers, vk::SampleCountFlagBits::e1,
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
              {}, Ret.Allocation.GetImage(), imageViewType, TexelFormat,
              vk::ComponentMapping(HshToVkComponentSwizzle(redSwizzle),
                                   HshToVkComponentSwizzle(greenSwizzle),
                                   HshToVkComponentSwizzle(blueSwizzle),
                                   HshToVkComponentSwizzle(alphaSwizzle)),
              vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                        numMips, 0, numLayers)))
          .value;
  vulkan::Globals.SetDebugObjectName(location, Ret.ImageView.get());

  vulkan::Globals.CopyCmd.pipelineBarrier(
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
  vulkan::Globals.CopyCmd.copyBufferToImage(
      UploadBuffer.GetBuffer(), Ret.Allocation.GetImage(),
      vk::ImageLayout::eTransferDstOptimal, {numMips, Copies.data()});
  vulkan::Globals.CopyCmd.pipelineBarrier(
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

template <vk::ImageType Type, typename Traits = TextureTypeTraits<Type>>
inline auto CreateDynamicTextureOwner(
    const SourceLocation &location, vk::ImageViewType imageViewType,
    typename Traits::ExtentType extent, uint32_t numLayers, Format format,
    uint32_t numMips, ColorSwizzle redSwizzle, ColorSwizzle greenSwizzle,
    ColorSwizzle blueSwizzle, ColorSwizzle alphaSwizzle) noexcept {
  auto TexelSize = uint32_t(HshFormatToTexelSize(format));
  auto TexelSizeShift = uint32_t(HshFormatToTexelSizeShift(format));
  auto TexelFormat = HshToVkFormat(format);
  auto BufferSize =
      Traits::MipOffset(extent, numLayers, TexelSize, TexelSizeShift, numMips);
  auto UploadBuffer = vulkan::AllocateUploadBuffer(location, BufferSize);

  TargetTraits<Target::VULKAN_SPIRV>::DynamicTextureOwner Ret{
      vulkan::AllocateTexture(
          location.with_field(Traits::Name),
          vk::ImageCreateInfo({}, Type, TexelFormat, extent3d(extent), numMips,
                              numLayers, vk::SampleCountFlagBits::e1,
                              vk::ImageTiling::eOptimal,
                              vk::ImageUsageFlagBits::eSampled |
                                  vk::ImageUsageFlagBits::eTransferDst,
                              {}, {}, {}, vk::ImageLayout::eUndefined)),
      std::move(UploadBuffer),
      Traits::MakeCopies(extent, numLayers, TexelSize, TexelSizeShift),
      {},
      std::uint8_t(numMips),
      HshFormatIsInteger(format)};
  Ret.ImageView =
      vulkan::Globals.Device
          .createImageViewUnique(vk::ImageViewCreateInfo(
              {}, Ret.Allocation.GetImage(), imageViewType, TexelFormat,
              vk::ComponentMapping(HshToVkComponentSwizzle(redSwizzle),
                                   HshToVkComponentSwizzle(greenSwizzle),
                                   HshToVkComponentSwizzle(blueSwizzle),
                                   HshToVkComponentSwizzle(alphaSwizzle)),
              vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0,
                                        numMips, 0, numLayers)))
          .value;
  vulkan::Globals.SetDebugObjectName(location, Ret.ImageView.get());

  return Ret;
}

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<texture1d> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, uint32_t extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<vk::ImageType::e1D>(
        location, vk::ImageViewType::e1D, extent, 1, format, numMips, copyFunc,
        redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, uint32_t extent,
                            Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<vk::ImageType::e1D>(
        location, vk::ImageViewType::e1D, extent, 1, format, numMips,
        redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<texture1d_array> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, uint32_t extent,
                     uint32_t numLayers, Format format, uint32_t numMips,
                     CopyFunc copyFunc, ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<vk::ImageType::e1D>(
        location, vk::ImageViewType::e1DArray, extent, numLayers, format,
        numMips, copyFunc, redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, uint32_t extent,
                            uint32_t numLayers, Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<vk::ImageType::e1D>(
        location, vk::ImageViewType::e1DArray, extent, numLayers, format,
        numMips, redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<texture2d> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, extent2d extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<vk::ImageType::e2D>(
        location, vk::ImageViewType::e2D, extent, 1, format, numMips, copyFunc,
        redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, extent2d extent,
                            Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<vk::ImageType::e2D>(
        location, vk::ImageViewType::e2D, extent, 1, format, numMips,
        redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<texture2d_array> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, extent2d extent,
                     uint32_t numLayers, Format format, uint32_t numMips,
                     CopyFunc copyFunc, ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<vk::ImageType::e2D>(
        location, vk::ImageViewType::e2DArray, extent, numLayers, format,
        numMips, copyFunc, redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, extent2d extent,
                            uint32_t numLayers, Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<vk::ImageType::e2D>(
        location, vk::ImageViewType::e2DArray, extent, numLayers, format,
        numMips, redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<texture3d> {
  template <typename CopyFunc>
  static auto Create(const SourceLocation &location, extent3d extent,
                     Format format, uint32_t numMips, CopyFunc copyFunc,
                     ColorSwizzle redSwizzle = CS_Identity,
                     ColorSwizzle greenSwizzle = CS_Identity,
                     ColorSwizzle blueSwizzle = CS_Identity,
                     ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateTextureOwner<vk::ImageType::e3D>(
        location, vk::ImageViewType::e3D, extent, 1, format, numMips, copyFunc,
        redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }

  static auto CreateDynamic(const SourceLocation &location, extent3d extent,
                            Format format, uint32_t numMips,
                            ColorSwizzle redSwizzle = CS_Identity,
                            ColorSwizzle greenSwizzle = CS_Identity,
                            ColorSwizzle blueSwizzle = CS_Identity,
                            ColorSwizzle alphaSwizzle = CS_Identity) noexcept {
    return CreateDynamicTextureOwner<vk::ImageType::e3D>(
        location, vk::ImageViewType::e3D, extent, 1, format, numMips,
        redSwizzle, greenSwizzle, blueSwizzle, alphaSwizzle);
  }
};

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<render_texture2d> {
  static auto Create(const SourceLocation &location, surface Surf,
                     uint32_t NumColorBindings = 0,
                     uint32_t NumDepthBindings = 0) noexcept {
    return TargetTraits<Target::VULKAN_SPIRV>::RenderTextureOwner{
        std::make_unique<vulkan::RenderTextureAllocation>(
            location.with_field("RenderTexture2D"),
            Surf.Binding.get_VULKAN_SPIRV().Allocation, NumColorBindings,
            NumDepthBindings)};
  }

  static auto Create(const SourceLocation &location, extent2d Extent,
                     uint32_t NumColorBindings = 0,
                     uint32_t NumDepthBindings = 0) noexcept {
    return TargetTraits<Target::VULKAN_SPIRV>::RenderTextureOwner{
        std::make_unique<vulkan::RenderTextureAllocation>(
            location.with_field("RenderTexture2D"), Extent, NumColorBindings,
            NumDepthBindings)};
  }
};

template <>
struct TargetTraits<Target::VULKAN_SPIRV>::ResourceFactory<surface> {
  static auto
  Create(const SourceLocation &location, vk::UniqueSurfaceKHR &&Surface,
         std::function<void(const hsh::extent2d &, const hsh::extent2d &)>
             &&ResizeLambda,
         std::function<void()> &&DeleterLambda,
         const hsh::extent2d &RequestExtent, int32_t L, int32_t R, int32_t T,
         int32_t B) noexcept {
    vulkan::Globals.SetDebugObjectName(location.with_field("Surface"),
                                       Surface.get());
    if (!vulkan::Globals.CheckSurfaceSupported(Surface.get()))
      return TargetTraits<Target::VULKAN_SPIRV>::SurfaceOwner{};
    return TargetTraits<Target::VULKAN_SPIRV>::SurfaceOwner{
        std::make_unique<vulkan::SurfaceAllocation>(
            location, std::move(Surface), std::move(ResizeLambda),
            std::move(DeleterLambda), RequestExtent, L, R, T, B)};
  }
};

namespace vulkan {

using namespace std::literals;

struct MyInstanceCreateInfo : vk::InstanceCreateInfo {
  struct MyApplicationInfo : vk::ApplicationInfo {
    constexpr MyApplicationInfo(const char *AppName, uint32_t AppVersion,
                                const char *EngineName,
                                uint32_t EngineVersion) noexcept
        : vk::ApplicationInfo(AppName, AppVersion, EngineName, EngineVersion,
                              VK_API_VERSION_1_1) {}
  } AppInfo;
  std::vector<vk::LayerProperties> Layers;
  std::vector<vk::ExtensionProperties> Extensions;
  std::vector<const char *> EnabledLayers;
  std::vector<const char *> EnabledExtensions;
  bool Success = true;

  bool enableLayer(std::string_view Name) noexcept {
    for (const auto &L : Layers) {
      if (!Name.compare(L.layerName.data())) {
        EnabledLayers.push_back(Name.data());
        return true;
      }
    }
    return false;
  }

  bool enableExtension(std::string_view Name) noexcept {
    for (const auto &E : Extensions) {
      if (!Name.compare(E.extensionName.data())) {
        EnabledExtensions.push_back(Name.data());
        return true;
      }
    }
    return false;
  }

#if !defined(NDEBUG)
  static constexpr std::string_view WantedLayers[] = {
      "VK_LAYER_KHRONOS_validation"sv,
  };
#endif

  static constexpr std::string_view WantedExtensions[] = {
    "VK_KHR_surface"sv,
#ifdef VK_USE_PLATFORM_XCB_KHR
    "VK_KHR_xcb_surface"sv,
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    "VK_KHR_wayland_surface"sv,
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    "VK_KHR_win32_surface"sv,
#endif
#if !defined(NDEBUG)
    "VK_EXT_debug_utils"sv,
#endif
  };

  MyInstanceCreateInfo(
      const char *AppName, uint32_t AppVersion, const char *EngineName,
      uint32_t EngineVersion, bool &HasGetPhysicalProps2,
      const std::function<void(std::string_view)> &MissingLayer,
      const std::function<void(std::string_view)> &MissingExtension) noexcept
      : vk::InstanceCreateInfo({}, &AppInfo),
        AppInfo(AppName, AppVersion, EngineName, EngineVersion) {
    Layers = vk::enumerateInstanceLayerProperties().value;
    Extensions = vk::enumerateInstanceExtensionProperties().value;

#if !defined(NDEBUG)
    for (auto WL : WantedLayers) {
      if (!enableLayer(WL)) {
        MissingLayer(WL);
        Success = false;
      }
    }
#endif

    for (auto WE : WantedExtensions) {
      if (!enableExtension(WE)) {
        MissingExtension(WE);
        Success = false;
      }
    }

    HasGetPhysicalProps2 =
        enableExtension("VK_KHR_get_physical_device_properties2"sv);

    setEnabledLayerCount(uint32_t(EnabledLayers.size()));
    setPpEnabledLayerNames(EnabledLayers.data());
    setEnabledExtensionCount(uint32_t(EnabledExtensions.size()));
    setPpEnabledExtensionNames(EnabledExtensions.data());
  }
};

struct MyDeviceCreateInfo : vk::DeviceCreateInfo {
  float QueuePriority = 1.f;
  vk::DeviceQueueCreateInfo QueueCreateInfo{{}, 0, 1, &QueuePriority};
  std::vector<vk::LayerProperties> Layers;
  std::vector<vk::ExtensionProperties> Extensions;
  std::vector<const char *> EnabledLayers;
  std::vector<const char *> EnabledExtensions;
  vk::PhysicalDeviceFeatures EnabledFeatures;
  bool Success = true;

  bool enableLayer(std::string_view Name) noexcept {
    for (const auto &L : Layers) {
      if (!Name.compare(L.layerName.data())) {
        EnabledLayers.push_back(Name.data());
        return true;
      }
    }
    return false;
  }

  bool enableExtension(std::string_view Name) noexcept {
    for (const auto &E : Extensions) {
      if (!Name.compare(E.extensionName.data())) {
        EnabledExtensions.push_back(Name.data());
        return true;
      }
    }
    return false;
  }

#if !defined(NDEBUG)
  static constexpr std::string_view WantedLayers[] = {
      "VK_LAYER_KHRONOS_validation"sv,
  };
#endif

  static constexpr std::string_view WantedExtensions[] = {
      "VK_KHR_swapchain"sv, "VK_KHR_get_memory_requirements2"sv,
      "VK_KHR_dedicated_allocation"sv};

  explicit MyDeviceCreateInfo(
      vk::PhysicalDevice PD, uint32_t &QFIdxOut, bool &HasExtMemoryBudget,
      uint8_t PipelineCacheUUIDOut[VK_UUID_SIZE],
      const std::function<void(std::string_view)> &MissingLayer,
      const std::function<void(std::string_view)> &MissingExtension,
      const std::function<void()> &NoGraphicsQueueFamily) noexcept
      : vk::DeviceCreateInfo({}, 1, &QueueCreateInfo, 0, nullptr, 0, nullptr,
                             &EnabledFeatures) {
    auto Properties = PD.getProperties();
    std::memcpy(PipelineCacheUUIDOut, Properties.pipelineCacheUUID,
                VK_UUID_SIZE);

    Layers = PD.enumerateDeviceLayerProperties().value;
    Extensions = PD.enumerateDeviceExtensionProperties().value;

#if !defined(NDEBUG)
    for (auto WL : WantedLayers) {
      if (!enableLayer(WL)) {
        MissingLayer(WL);
        Success = false;
      }
    }
#endif

    for (auto WE : WantedExtensions) {
      if (!enableExtension(WE)) {
        MissingExtension(WE);
        Success = false;
      }
    }

    HasExtMemoryBudget = enableExtension("VK_EXT_memory_budget"sv);

    setEnabledLayerCount(uint32_t(EnabledLayers.size()));
    setPpEnabledLayerNames(EnabledLayers.data());
    setEnabledExtensionCount(uint32_t(EnabledExtensions.size()));
    setPpEnabledExtensionNames(EnabledExtensions.data());

    uint32_t QFIdx = 0;
    bool FoundQF = false;
    for (const auto &QF : PD.getQueueFamilyProperties()) {
      if (QF.queueFlags & vk::QueueFlagBits::eGraphics) {
        FoundQF = true;
        break;
      }
      ++QFIdx;
    }
    if (!FoundQF) {
      NoGraphicsQueueFamily();
      Success = false;
    }
    QFIdxOut = QFIdx;
    QueueCreateInfo.setQueueFamilyIndex(QFIdx);

    auto Features = PD.getFeatures();
    EnabledFeatures.geometryShader = Features.geometryShader;
    EnabledFeatures.tessellationShader = Features.tessellationShader;
    EnabledFeatures.samplerAnisotropy = Features.samplerAnisotropy;
    EnabledFeatures.textureCompressionBC = Features.textureCompressionBC;
    EnabledFeatures.dualSrcBlend = Features.dualSrcBlend;
  }
};

using ErrorHandler = std::function<void(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagBitsEXT messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT &pCallbackData)>;

struct MyDebugUtilsMessengerCreateInfo : vk::DebugUtilsMessengerCreateInfoEXT {
  static VkBool32
  Callback(vk::DebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
           vk::DebugUtilsMessageTypeFlagBitsEXT MessageTypes,
           const vk::DebugUtilsMessengerCallbackDataEXT *CallbackData,
           ErrorHandler *UserData) noexcept {
    (*UserData)(MessageSeverity, MessageTypes, *CallbackData);
    if (MessageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
      std::abort();
    return VK_FALSE;
  }

  explicit MyDebugUtilsMessengerCreateInfo(
      vk::DebugUtilsMessageSeverityFlagsEXT WantedFlags,
      vk::DebugUtilsMessageTypeFlagsEXT WantedTypes,
      ErrorHandler &ErrHandler) noexcept
      : vk::DebugUtilsMessengerCreateInfoEXT(
            {}, WantedFlags, WantedTypes,
            PFN_vkDebugUtilsMessengerCallbackEXT(&Callback),
            reinterpret_cast<void *>(&ErrHandler)) {}
};

struct MyDescriptorSetLayoutCreateInfo : vk::DescriptorSetLayoutCreateInfo {
  std::array<vk::DescriptorSetLayoutBinding, hsh::detail::MaxUniforms +
                                                 hsh::detail::MaxImages +
                                                 hsh::detail::MaxSamplers>
      Bindings;
  template <std::size_t... USeq, std::size_t... ISeq, std::size_t... SSeq>
  constexpr MyDescriptorSetLayoutCreateInfo(
      std::index_sequence<USeq...>, std::index_sequence<ISeq...>,
      std::index_sequence<SSeq...>) noexcept
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
      : vk::DescriptorSetLayoutCreateInfo({}, uint32_t(Bindings.size()),
                                          Bindings.data()),
        Bindings{vk::DescriptorSetLayoutBinding(
                     USeq, vk::DescriptorType::eUniformBufferDynamic, 1,
                     vk::ShaderStageFlagBits::eAllGraphics)...,
                 vk::DescriptorSetLayoutBinding(
                     hsh::detail::MaxUniforms + ISeq,
                     vk::DescriptorType::eSampledImage, 1,
                     vk::ShaderStageFlagBits::eAllGraphics)...,
                 vk::DescriptorSetLayoutBinding(
                     hsh::detail::MaxUniforms + hsh::detail::MaxImages + SSeq,
                     vk::DescriptorType::eSampler, 1,
                     vk::ShaderStageFlagBits::eAllGraphics)...} {
  }
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
  constexpr MyDescriptorSetLayoutCreateInfo() noexcept
      : MyDescriptorSetLayoutCreateInfo(
            std::make_index_sequence<hsh::detail::MaxUniforms>(),
            std::make_index_sequence<hsh::detail::MaxImages>(),
            std::make_index_sequence<hsh::detail::MaxSamplers>()) {}
};

struct MyPipelineLayoutCreateInfo : vk::PipelineLayoutCreateInfo {
  std::array<vk::DescriptorSetLayout, 1> Layouts;
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
  constexpr MyPipelineLayoutCreateInfo(vk::DescriptorSetLayout layout) noexcept
      : vk::PipelineLayoutCreateInfo({}, uint32_t(Layouts.size()),
                                     Layouts.data()),
        Layouts{layout} {}
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
};

struct MyCommandPoolCreateInfo : vk::CommandPoolCreateInfo {
  constexpr MyCommandPoolCreateInfo(uint32_t qfIdx) noexcept
      : vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer, qfIdx) {}
};

struct MyCommandBufferAllocateInfo : vk::CommandBufferAllocateInfo {
  constexpr MyCommandBufferAllocateInfo(vk::CommandPool cmdPool) noexcept
      : vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary,
                                      3) {}
};

struct MyVmaAllocatorCreateInfo : VmaAllocatorCreateInfo {
  VmaVulkanFunctions Funcs;
  MyVmaAllocatorCreateInfo(VkInstance Instance, VkPhysicalDevice PhysDev,
                           VkDevice Device, bool HasExtMemoryBudget) noexcept
      : VmaAllocatorCreateInfo{
            VmaAllocatorCreateFlags(
                VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT |
                (HasExtMemoryBudget ? VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
                                    : 0)),
            PhysDev,
            Device,
            0,
            nullptr,
            nullptr,
            0,
            nullptr,
            &Funcs,
            nullptr,
            Instance,
            VK_API_VERSION_1_1} {
#define COPY_FUNC(funcName)                                                    \
  Funcs.funcName = VULKAN_HPP_DEFAULT_DISPATCHER.funcName;
#define COPY_1_1_FUNC(funcName)                                                \
  Funcs.funcName##KHR = VULKAN_HPP_DEFAULT_DISPATCHER.funcName;
    COPY_FUNC(vkGetPhysicalDeviceProperties);
    COPY_FUNC(vkGetPhysicalDeviceMemoryProperties);
    COPY_FUNC(vkAllocateMemory);
    COPY_FUNC(vkFreeMemory);
    COPY_FUNC(vkMapMemory);
    COPY_FUNC(vkUnmapMemory);
    COPY_FUNC(vkFlushMappedMemoryRanges);
    COPY_FUNC(vkInvalidateMappedMemoryRanges);
    COPY_FUNC(vkBindBufferMemory);
    COPY_FUNC(vkBindImageMemory);
    COPY_FUNC(vkGetBufferMemoryRequirements);
    COPY_FUNC(vkGetImageMemoryRequirements);
    COPY_FUNC(vkCreateBuffer);
    COPY_FUNC(vkDestroyBuffer);
    COPY_FUNC(vkCreateImage);
    COPY_FUNC(vkDestroyImage);
    COPY_FUNC(vkCmdCopyBuffer);
    COPY_1_1_FUNC(vkGetBufferMemoryRequirements2);
    COPY_1_1_FUNC(vkGetImageMemoryRequirements2);
    COPY_1_1_FUNC(vkBindBufferMemory2);
    COPY_1_1_FUNC(vkBindImageMemory2);
    COPY_1_1_FUNC(vkGetPhysicalDeviceMemoryProperties2);
#undef COPY_FUNC
#undef COPY_1_1_FUNC
  }
};

} // namespace vulkan

#endif

} // namespace hsh::detail

#if HSH_ENABLE_VULKAN

namespace hsh {
class vulkan_device_owner {
  friend class vulkan_instance_owner;
  struct Data {
    vk::UniqueDevice Device;
    vk::UniqueVmaAllocator VmaAllocator;
    vk::UniqueVmaPool UploadPool;
    vk::UniquePipelineCache PipelineCache;
    vk::UniqueDescriptorSetLayout DescriptorSetLayout;
    vk::UniquePipelineLayout PipelineLayout;
    detail::vulkan::DescriptorPoolChain DescriptorPoolChain;
    vk::UniqueCommandPool CommandPool;
    std::vector<vk::UniqueCommandBuffer> CommandBuffers;
    std::array<vk::UniqueFence, 3> CommandFences;
    vk::UniqueSemaphore ImageAcquireSem;
    vk::UniqueSemaphore RenderCompleteSem;
    std::array<detail::vulkan::DeletedResources, 2> DeletedResources;
    bool BuiltPipelines = false;

    ~Data() noexcept {
      Device->waitIdle();
      if (BuiltPipelines) {
        hsh::detail::GlobalListNode<false>::DestroyAll(
            ActiveTarget::VULKAN_SPIRV);
        hsh::detail::GlobalListNode<true>::DestroyAll(
            ActiveTarget::VULKAN_SPIRV);
      }
    }
  };
  std::unique_ptr<Data> Data;

public:
  vulkan_device_owner() noexcept = default;
  vulkan_device_owner(vulkan_device_owner &&) noexcept = default;
  vulkan_device_owner &operator=(vulkan_device_owner &&) noexcept = default;

  bool success() const noexcept { return Data.operator bool(); }
  operator bool() const noexcept { return success(); }

  void build_pipelines() noexcept {
    if (Data->BuiltPipelines)
      return;
    hsh::detail::GlobalListNode<true>::CreateAll(ActiveTarget::VULKAN_SPIRV);
    hsh::detail::GlobalListNode<false>::CreateAll(ActiveTarget::VULKAN_SPIRV);
    Data->BuiltPipelines = true;
  }

  template <typename CacheFileMgr>
  void build_pipelines(CacheFileMgr &CFM) noexcept {
    if (Data->BuiltPipelines)
      return;
    Data->PipelineCache = hsh::detail::vulkan::CreatePipelineCache(CFM);
    detail::vulkan::Globals.PipelineCache = Data->PipelineCache.get();
    hsh::detail::GlobalListNode<true>::CreateAll(ActiveTarget::VULKAN_SPIRV);
    hsh::detail::GlobalListNode<false>::CreateAll(ActiveTarget::VULKAN_SPIRV);
    hsh::detail::vulkan::WritePipelineCache(CFM);
    Data->BuiltPipelines = true;
  }

  using HighCompleteFunc = std::function<void()>;
  using ProgFunc = std::function<void(std::size_t, std::size_t)>;

  void build_pipelines(const HighCompleteFunc &HCF,
                       const ProgFunc &PF) noexcept {
    if (Data->BuiltPipelines)
      return;
    hsh::detail::GlobalListNode<true>::CreateAll(ActiveTarget::VULKAN_SPIRV);
    HCF();
    std::size_t Count = hsh::detail::GlobalListNode<false>::CountAll();
    std::size_t I = 0;
    PF(I, Count);
    for (auto *Node = hsh::detail::GlobalListNode<false>::GetHead(); Node;
         Node = Node->GetNext()) {
      Node->Create(ActiveTarget::VULKAN_SPIRV);
      PF(++I, Count);
    }
    Data->BuiltPipelines = true;
  }

  template <typename CacheFileMgr>
  void build_pipelines(CacheFileMgr &CFM, const HighCompleteFunc &HCF,
                       const ProgFunc &PF) noexcept {
    if (Data->BuiltPipelines)
      return;
    Data->PipelineCache = hsh::detail::vulkan::CreatePipelineCache(CFM);
    detail::vulkan::Globals.PipelineCache = Data->PipelineCache.get();
    hsh::detail::GlobalListNode<true>::CreateAll(ActiveTarget::VULKAN_SPIRV);
    HCF();
    std::size_t Count = hsh::detail::GlobalListNode<false>::CountAll();
    std::size_t I = 0;
    PF(I, Count);
    for (auto *Node = hsh::detail::GlobalListNode<false>::GetHead(); Node;
         Node = Node->GetNext()) {
      Node->Create(ActiveTarget::VULKAN_SPIRV);
      PF(++I, Count);
    }
    hsh::detail::vulkan::WritePipelineCache(CFM);
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
        hsh::detail::vulkan::WritePipelineCache(CFM);
    }

    std::pair<std::size_t, std::size_t> get_progress() const noexcept {
      return {I, Count};
    }

    bool pump() noexcept {
      if (Node) {
        Node->Create(ActiveTarget::VULKAN_SPIRV);
        Node = Node->GetNext();
        ++I;
        if (Node == nullptr) {
          hsh::detail::vulkan::WritePipelineCache(*CFM);
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

    Data->PipelineCache = hsh::detail::vulkan::CreatePipelineCache(CFM);
    detail::vulkan::Globals.PipelineCache = Data->PipelineCache.get();
    hsh::detail::GlobalListNode<true>::CreateAll(ActiveTarget::VULKAN_SPIRV);

    return pipeline_build_pump(CFM);
  }

  template <typename Func> void enter_draw_context(Func F) const noexcept {
    detail::vulkan::Globals.PreRender();
    F();
    detail::vulkan::Globals.PostRender();
  }

  void wait_idle() noexcept { Data->Device->waitIdle(); }
};

class vulkan_instance_owner {
  friend vulkan_instance_owner create_vulkan_instance(
      const char *AppName, uint32_t AppVersion, const char *EngineName,
      uint32_t EngineVersion, detail::vulkan::ErrorHandler &&ErrHandler,
      vk::DebugUtilsMessageSeverityFlagsEXT WantedFlags,
      vk::DebugUtilsMessageTypeFlagsEXT WantedTypes) noexcept;
  struct Data {
    vk::DynamicLoader Loader;
    vk::UniqueInstance Instance;
    detail::vulkan::ErrorHandler ErrHandler;
#ifndef NDEBUG
    vk::UniqueDebugUtilsMessengerEXT Messenger;
#endif
  };
  std::unique_ptr<Data> Data;

public:
  bool success() const noexcept { return Data.operator bool(); }
  operator bool() const noexcept { return success(); }

  vk::Instance getInstance() const { return Data->Instance.get(); }

  template <typename Func>
  vulkan_device_owner
  enumerate_vulkan_devices(Func Acceptor,
                           vk::SurfaceKHR CheckSurface = {}) const noexcept {
    vulkan_device_owner Ret;
    vk::Instance Instance = Data->Instance.get();
    auto &ErrHandler = Data->ErrHandler;

    auto PhysDevices = Instance.enumeratePhysicalDevices().value;
    for (auto PD : PhysDevices) {
      auto Properties = PD.getProperties();
      if (Properties.apiVersion < VK_VERSION_1_1)
        continue;

      detail::vulkan::Globals.PhysDevice = PD;
      uint32_t QFIdx = 0;
      bool HasExtMemoryBudget = false;
      detail::vulkan::MyDeviceCreateInfo DeviceCreateInfo(
          PD, QFIdx, HasExtMemoryBudget,
          detail::vulkan::Globals.PipelineCacheUUID,
          [&](std::string_view MissingLayer) {
            std::ostringstream ss;
            ss << "Required instance layer '" << MissingLayer
               << "' not available in " << Properties.deviceName << ".";
            ErrHandler(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
                       vk::DebugUtilsMessengerCallbackDataEXT(
                           {}, "Missing instance layer", {}, ss.str().c_str()));
          },
          [&](std::string_view MissingExtension) {
            std::ostringstream ss;
            ss << "Required instance extension '" << MissingExtension
               << "' not available in " << Properties.deviceName << ".";
            ErrHandler(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
                vk::DebugUtilsMessengerCallbackDataEXT(
                    {}, "Missing instance extension", {}, ss.str().c_str()));
          },
          [&]() {
            std::ostringstream ss;
            ss << "No graphics queue family in " << Properties.deviceName
               << ".";
            ErrHandler(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
                vk::DebugUtilsMessengerCallbackDataEXT(
                    {}, "Missing graphics queue family", {}, ss.str().c_str()));
          });
      if (!DeviceCreateInfo.Success)
        continue;

      if (CheckSurface && !PD.getSurfaceSupportKHR(QFIdx, CheckSurface).value) {
        std::ostringstream ss;
        ss << "Surface is not supported by " << Properties.deviceName << ".";
        ErrHandler(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                   vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
                   vk::DebugUtilsMessengerCallbackDataEXT(
                       {}, "Incompatible surface", {}, ss.str().c_str()));
        continue;
      }

      if (!Acceptor(
              Properties,
              (VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties2
                   ? PD.getProperties2<vk::PhysicalDeviceProperties2,
                                       vk::PhysicalDeviceDriverProperties>()
                         .get<vk::PhysicalDeviceDriverProperties>()
                   : vk::PhysicalDeviceDriverProperties{})))
        continue;

      Ret.Data = std::make_unique<struct vulkan_device_owner::Data>();
      auto &Data = *Ret.Data;

      Data.Device = PD.createDeviceUnique(DeviceCreateInfo).value;
      VULKAN_HPP_DEFAULT_DISPATCHER.init(*Data.Device);
      detail::vulkan::Globals.Device = Data.Device.get();
      detail::vulkan::Globals.QueueFamilyIdx = QFIdx;

      Data.VmaAllocator =
          vk::createVmaAllocatorUnique(
              detail::vulkan::MyVmaAllocatorCreateInfo(
                  Instance, PD, Data.Device.get(), HasExtMemoryBudget))
              .value;
      detail::vulkan::Globals.Allocator = Data.VmaAllocator.get();
      Data.UploadPool = detail::vulkan::CreateUploadPool();
      detail::vulkan::Globals.UploadPool = Data.UploadPool.get();

      detail::vulkan::Globals.DeletedResourcesArr = &Data.DeletedResources;
      detail::vulkan::Globals.DeletedResources = &Data.DeletedResources[0];

      Data.DescriptorSetLayout =
          Data.Device
              ->createDescriptorSetLayoutUnique(
                  detail::vulkan::MyDescriptorSetLayoutCreateInfo())
              .value;
      detail::vulkan::Globals.SetDescriptorSetLayout(
          Data.DescriptorSetLayout.get());
      Data.PipelineLayout = Data.Device
                                ->createPipelineLayoutUnique(
                                    detail::vulkan::MyPipelineLayoutCreateInfo(
                                        Data.DescriptorSetLayout.get()))
                                .value;
      detail::vulkan::Globals.PipelineLayout = Data.PipelineLayout.get();
      detail::vulkan::Globals.DescriptorPoolChain = &Data.DescriptorPoolChain;
      detail::vulkan::Globals.Queue = Data.Device->getQueue(QFIdx, 0);
      Data.CommandPool = Data.Device
                             ->createCommandPoolUnique(
                                 detail::vulkan::MyCommandPoolCreateInfo(QFIdx))
                             .value;
      Data.CommandBuffers = Data.Device
                                ->allocateCommandBuffersUnique(
                                    detail::vulkan::MyCommandBufferAllocateInfo(
                                        Data.CommandPool.get()))
                                .value;
      for (int i = 0; i < 2; ++i)
        detail::vulkan::Globals.CommandBuffers[i] =
            Data.CommandBuffers[i].get();
      Data.CommandFences = {
          Data.Device
              ->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlags{}))
              .value,
          Data.Device
              ->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlags{}))
              .value,
          Data.Device
              ->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlags{}))
              .value,
      };
      for (int i = 0; i < 2; ++i)
        detail::vulkan::Globals.CommandFences[i] = Data.CommandFences[i].get();
      detail::vulkan::Globals.CopyCmd = Data.CommandBuffers[2].get();
      detail::vulkan::Globals.CopyFence = Data.CommandFences[2].get();
      Data.ImageAcquireSem = Data.Device->createSemaphoreUnique({}).value;
      detail::vulkan::Globals.ImageAcquireSem = Data.ImageAcquireSem.get();
      Data.RenderCompleteSem = Data.Device->createSemaphoreUnique({}).value;
      detail::vulkan::Globals.RenderCompleteSem = Data.RenderCompleteSem.get();

      detail::vulkan::Globals.UniformOffsetAlignment =
          Properties.limits.minUniformBufferOffsetAlignment;

      return Ret;
    }

    return {};
  }

#ifdef VK_USE_PLATFORM_XCB_KHR
  vk::UniqueSurfaceKHR create_phys_surface(xcb_connection_t *Connection,
                                           xcb_window_t Window) noexcept {
    return Data->Instance
        ->createXcbSurfaceKHRUnique(
            vk::XcbSurfaceCreateInfoKHR({}, Connection, Window))
        .value;
  }
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  vk::UniqueSurfaceKHR create_phys_surface(wl_display *Display,
                                           wl_surface *Surface) noexcept {
    return Data->Instance
        ->createWaylandSurfaceKHRUnique(
            vk::WaylandSurfaceCreateInfoKHR({}, Display, Surface))
        .value;
  }
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
  vk::UniqueSurfaceKHR create_phys_surface(HINSTANCE Instance,
                                           HWND Window) noexcept {
    return Data->Instance
        ->createWin32SurfaceKHRUnique(
            vk::Win32SurfaceCreateInfoKHR({}, Instance, Window))
        .value;
  }
#endif
};

inline vulkan_instance_owner create_vulkan_instance(
    const char *AppName, uint32_t AppVersion, const char *EngineName,
    uint32_t EngineVersion, detail::vulkan::ErrorHandler &&ErrHandler,
    vk::DebugUtilsMessageSeverityFlagsEXT WantedFlags =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
    vk::DebugUtilsMessageTypeFlagsEXT WantedTypes =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance) noexcept {
  vulkan_instance_owner Ret;
  Ret.Data = std::make_unique<struct vulkan_instance_owner::Data>();
  auto &Data = *Ret.Data;
  Data.ErrHandler = std::move(ErrHandler);

  if (!Data.Loader.success()) {
    Data.ErrHandler(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
        vk::DebugUtilsMessengerCallbackDataEXT({}, "Missing vulkan runtime", {},
                                               "Unable to load vulkan loader"));
    return {};
  }
  auto GetInstanceProcAddr =
      Data.Loader.getProcAddress<PFN_vkGetInstanceProcAddr>(
          "vkGetInstanceProcAddr");
  if (!GetInstanceProcAddr) {
    Data.ErrHandler(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
                    vk::DebugUtilsMessengerCallbackDataEXT(
                        {}, "Missing vkGetInstanceProcAddr", {},
                        "Unable to find vkGetInstanceProcAddr"));
    return {};
  }
  VULKAN_HPP_DEFAULT_DISPATCHER.init(GetInstanceProcAddr);

  bool HasGetPhysicalProps2 = false;
  detail::vulkan::MyInstanceCreateInfo InstanceCreateInfo(
      AppName, AppVersion, EngineName, EngineVersion, HasGetPhysicalProps2,
      [&](std::string_view MissingLayer) {
        std::ostringstream ss;
        ss << "Required instance layer '" << MissingLayer << "' not available.";
        Data.ErrHandler(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
            vk::DebugUtilsMessengerCallbackDataEXT({}, "Missing instance layer",
                                                   {}, ss.str().c_str()));
      },
      [&](std::string_view MissingExtension) {
        std::ostringstream ss;
        ss << "Required instance extension '" << MissingExtension
           << "' not available.";
        Data.ErrHandler(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
            vk::DebugUtilsMessengerCallbackDataEXT(
                {}, "Missing instance extension", {}, ss.str().c_str()));
      });
  if (!InstanceCreateInfo.Success)
    return {};
  Data.Instance = vk::createInstanceUnique(InstanceCreateInfo).value;
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*Data.Instance);
  detail::vulkan::Globals.Instance = Data.Instance.get();

#ifndef NDEBUG
  Data.Messenger = Data.Instance
                       ->createDebugUtilsMessengerEXTUnique(
                           detail::vulkan::MyDebugUtilsMessengerCreateInfo(
                               WantedFlags, WantedTypes, Data.ErrHandler))
                       .value;
#endif

  return Ret;
}
} // namespace hsh

#endif

#undef HSH_ASSERT_VK_SUCCESS
