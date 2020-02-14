#pragma once

namespace hsh {

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

class GlobalListNode {
  static GlobalListNode *Head;

public:
  static GlobalListNode *GetHead() noexcept { return Head; }
  typedef void (*BuildFunc)() noexcept;
  struct FuncPair {
    BuildFunc Create, Destroy;
  };

private:
  std::array<FuncPair, std::size_t(ActiveTarget::MaxActiveTarget)> Funcs;
  GlobalListNode *Next;

public:
  template <typename... Args>
  explicit GlobalListNode(Args... Funcs) noexcept
      : Funcs{Funcs...}, Next(Head) {
    Head = this;
  }
  GlobalListNode *GetNext() const noexcept { return Next; }
  void Create(ActiveTarget T) const noexcept {
    return Funcs[std::size_t(T)].Create();
  }
  static void CreateAll(ActiveTarget T) noexcept {
    for (auto *Node = GetHead(); Node; Node = Node->GetNext())
      Node->Create(T);
  }
  void Destroy(ActiveTarget T) const noexcept {
    return Funcs[std::size_t(T)].Destroy();
  }
  static void DestroyAll(ActiveTarget T) noexcept {
    for (auto *Node = GetHead(); Node; Node = Node->GetNext())
      Node->Destroy(T);
  }
};
inline GlobalListNode *GlobalListNode::Head = nullptr;

template <template <hsh::Target> typename Impl>
struct PipelineCoordinatorNode : GlobalListNode {
  PipelineCoordinatorNode()
      : GlobalListNode{
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  GlobalListNode::FuncPair{Impl<Target::Enumeration>::Create,                  \
                           Impl<Target::Enumeration>::Destroy},
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
} // namespace hsh
