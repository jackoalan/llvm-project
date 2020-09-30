#pragma once

#include <sstream>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

/*
 * Base pointer set by ShaderFileMapper for loading file-mapped shaders.
 * If hsh-objcopy has not processed the binary, __hsh_objcopy is left as zero.
 */
struct __hsh_off {
  uint32_t offset, length;
};
extern "C" const volatile uint8_t __hsh_objcopy;
extern "C" const volatile __hsh_off
    __hsh_offsets[size_t(hsh::Target::MaxTarget)];
extern "C" const uint8_t *__hsh_reloc_base;
extern "C" size_t __hsh_reloc_length;

#ifdef HSH_IMPLEMENTATION
const volatile uint8_t __hsh_objcopy = 0;
const volatile __hsh_off __hsh_offsets[size_t(hsh::Target::MaxTarget)] = {};
const uint8_t *__hsh_reloc_base = nullptr;
size_t __hsh_reloc_length = SIZE_MAX;
#endif

namespace hsh {

#ifdef _WIN32

inline std::basic_string<TCHAR> get_exe_path() noexcept {
  std::basic_string<TCHAR> FileName;
  DWORD FileNameSize = MAX_PATH; // first guess, not an absolute limit
  DWORD Copied = 0;
  do {
    FileNameSize *= 2;
    FileName.resize(FileNameSize);
    Copied = ::GetModuleFileName(nullptr, FileName.data(), FileNameSize);
  } while (Copied >= FileNameSize);
  FileName.resize(Copied);
  return FileName;
}

inline uint8_t *mmap_file(const TCHAR *FilePath, size_t Offset,
                          size_t Length) noexcept {
  HANDLE File = ::CreateFile(FilePath, GENERIC_READ, FILE_SHARE_READ, nullptr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (File == INVALID_HANDLE_VALUE)
    return nullptr;

  HANDLE FileMapping =
      ::CreateFileMapping(File, nullptr, PAGE_READONLY, 0, 0, nullptr);
  ::CloseHandle(File);
  if (File == INVALID_HANDLE_VALUE)
    return nullptr;

  void *Mem = ::MapViewOfFile(FileMapping, FILE_MAP_READ, 0, DWORD(Offset),
                              DWORD(Length));
  ::CloseHandle(FileMapping);
  if (Mem == nullptr)
    return nullptr;

  return reinterpret_cast<uint8_t *>(Mem);
}

inline void munmap_file(uint8_t *Mem, size_t Len) noexcept {
  ::UnmapViewOfFile(Mem);
}

#else

#ifdef __linux__
inline std::string get_exe_path() noexcept {
  std::ostringstream PathSS;
  PathSS << "/proc/" << ::getpid() << "/exe";
  ssize_t LinkSize = 128;
  do {
    std::string Ret(LinkSize, '\0');
    ssize_t ReadLinkSize;
    if ((ReadLinkSize = ::readlink(PathSS.str().c_str(), &Ret[0], LinkSize)) <
        0)
      return {};
    if (ReadLinkSize < LinkSize) {
      Ret.resize(ReadLinkSize);
      return Ret;
    }
    LinkSize *= 2;
  } while (true);
}
#elif defined(__APPLE__)
inline std::string get_exe_path() noexcept {
  uint32_t ExeSize = 0;
  _NSGetExecutablePath(nullptr, &ExeSize);
  std::string ExePath(ExeSize - 1, '\0');
  _NSGetExecutablePath(&ExePath[0], &ExeSize);
  return ExePath;
}
#endif

inline uint8_t *mmap_file(const char *FilePath, size_t Offset,
                          size_t Length) noexcept {
  int Fd;
  if ((Fd = ::open(FilePath, O_RDONLY)) < 0)
    return {};
  void *Mem = ::mmap(nullptr, Length, PROT_READ, MAP_PRIVATE, Fd, Offset);
  ::close(Fd);
  if (Mem == MAP_FAILED)
    return {};
  return reinterpret_cast<uint8_t *>(Mem);
}

inline void munmap_file(uint8_t *Mem, size_t Len) noexcept {
  ::munmap(Mem, Len);
}

#endif

struct ShaderFileMapper {
  bool Good = false;
  ShaderFileMapper() noexcept {
    if (__hsh_objcopy == 0) {
      /* Unprocessed binary, no further action necessary */
      Good = true;
      return;
    }
    const volatile __hsh_off &Offset =
        __hsh_offsets[size_t(detail::CurrentTarget)];
    uint8_t *MappedBase =
        mmap_file(get_exe_path().c_str(), Offset.offset, Offset.length);
    if (MappedBase) {
      __hsh_reloc_base = MappedBase;
      __hsh_reloc_length = Offset.length;
      Good = true;
    }
  }
  ~ShaderFileMapper() noexcept {
    if (!__hsh_reloc_base)
      return;
    munmap_file(const_cast<uint8_t *>(__hsh_reloc_base), __hsh_reloc_length);
  }
};

class binding;

struct base_buffer {};

template <typename T> struct uniform_buffer;
template <typename T> struct vertex_buffer;
template <typename T> struct index_buffer;
struct uniform_fifo;
struct vertex_fifo;
struct index_fifo;

struct uniform_buffer_typeless : base_buffer {
  using MappedType = void;
  detail::TypeInfo TypeInfo;
  uniform_buffer_typeless() noexcept = default;
  template <typename... Args>
  explicit uniform_buffer_typeless(detail::TypeInfo TypeInfo,
                                   Args &&... args) noexcept
      : TypeInfo(TypeInfo), Binding(std::forward<Args>(args)...) {}
  explicit uniform_buffer_typeless(detail::TypeInfo TypeInfo,
                                   uniform_buffer_typeless other) noexcept
      : TypeInfo(TypeInfo), Binding(other.Binding) {}
  detail::ActiveTargetTraits::UniformBufferBinding Binding;
  operator bool() const noexcept { return Binding.IsValid(); }
  template <typename T> uniform_buffer<T> cast() const noexcept;
  template <typename T> operator uniform_buffer<T>() const noexcept {
    return cast<T>();
  }
  void reset() noexcept { Binding = decltype(Binding){}; }
};
struct vertex_buffer_typeless : base_buffer {
  using MappedType = void;
  detail::TypeInfo TypeInfo;
  vertex_buffer_typeless() noexcept = default;
  template <typename... Args>
  explicit vertex_buffer_typeless(detail::TypeInfo TypeInfo,
                                  Args &&... args) noexcept
      : TypeInfo(TypeInfo), Binding(std::forward<Args>(args)...) {}
  explicit vertex_buffer_typeless(detail::TypeInfo TypeInfo,
                                  vertex_buffer_typeless other) noexcept
      : TypeInfo(TypeInfo), Binding(other.Binding) {}
  detail::ActiveTargetTraits::VertexBufferBinding Binding;
  operator bool() const noexcept { return Binding.IsValid(); }
  template <typename T> vertex_buffer<T> cast() const noexcept;
  template <typename T> operator vertex_buffer<T>() const noexcept {
    return cast<T>();
  }
  void reset() noexcept { Binding = decltype(Binding){}; }
};
struct index_buffer_typeless : base_buffer {
  using MappedType = void;
  detail::TypeInfo TypeInfo;
  index_buffer_typeless() noexcept = default;
  template <typename... Args>
  explicit index_buffer_typeless(detail::TypeInfo TypeInfo,
                                 Args &&... args) noexcept
      : TypeInfo(TypeInfo), Binding(std::forward<Args>(args)...) {}
  explicit index_buffer_typeless(detail::TypeInfo TypeInfo,
                                 index_buffer_typeless other) noexcept
      : TypeInfo(TypeInfo), Binding(other.Binding) {}
  detail::ActiveTargetTraits::IndexBufferBinding Binding;
  operator bool() const noexcept { return Binding.IsValid(); }
  template <typename T> index_buffer<T> cast() const noexcept;
  template <typename T> operator index_buffer<T>() const noexcept {
    return cast<T>();
  }
  void reset() noexcept { Binding = decltype(Binding){}; }
};

#define HSH_CASTABLE_BUFFER(derived)                                           \
  template <typename T> struct derived : derived##_typeless {                  \
    using MappedType = T;                                                      \
    template <typename... Args>                                                \
    explicit derived(Args &&... args) noexcept                                 \
        : derived                                                              \
          ##_typeless(detail::TypeInfo::MakeTypeInfo<decltype(*this)>(),       \
                      std::forward<Args>(args)...) {}                          \
    const T *operator->() const noexcept {                                     \
      assert(false && "Not to be used from host!");                            \
      return nullptr;                                                          \
    }                                                                          \
    const T &operator*() const noexcept {                                      \
      assert(false && "Not to be used from host!");                            \
      return *reinterpret_cast<T *>(0);                                        \
    }                                                                          \
  };                                                                           \
  template <> struct derived<void> : derived##_typeless {                      \
    using MappedType = void;                                                   \
    template <typename... Args>                                                \
    explicit derived(Args &&... args) noexcept                                 \
        : derived                                                              \
          ##_typeless(detail::TypeInfo::MakeTypeInfo<decltype(*this)>(),       \
                      std::forward<Args>(args)...) {}                          \
  };
HSH_CASTABLE_BUFFER(uniform_buffer)
HSH_CASTABLE_BUFFER(vertex_buffer)
HSH_CASTABLE_BUFFER(index_buffer)
#undef HSH_CASTABLE_BUFFER

#define HSH_DEFINE_BUFFER_CAST(derived)                                        \
  template <typename T> derived<T> derived##_typeless::cast() const noexcept { \
    TypeInfo.Assert<derived<T>>();                                             \
    return static_cast<derived<T>>(*this);                                     \
  }
HSH_DEFINE_BUFFER_CAST(uniform_buffer)
HSH_DEFINE_BUFFER_CAST(vertex_buffer)
HSH_DEFINE_BUFFER_CAST(index_buffer)
#undef HSH_DEFINE_BUFFER_CAST

struct base_texture {};

struct texture1d;
struct texture1d_array;
struct texture2d;
struct texture2d_array;
struct texture3d;
struct texturecube;
struct texturecube_array;

struct texture_typeless : base_texture {
  detail::TypeInfo TypeInfo;
  template <typename... Args>
  explicit texture_typeless(detail::TypeInfo TypeInfo, Args &&... args) noexcept
      : TypeInfo(TypeInfo), Binding(std::forward<Args>(args)...) {}
  explicit texture_typeless(detail::TypeInfo TypeInfo,
                            texture_typeless other) noexcept
      : TypeInfo(TypeInfo), Binding(other.Binding) {}
  detail::ActiveTargetTraits::TextureBinding Binding;
  operator bool() const noexcept { return Binding.IsValid(); }
  template <typename T,
            std::enable_if_t<std::is_base_of_v<texture_typeless, T>, int> = 0>
  T cast() const noexcept {
    TypeInfo.Assert<T>();
    return static_cast<T>(*this);
  }
  template <typename T,
            std::enable_if_t<std::is_base_of_v<texture_typeless, T>, int> = 0>
  operator T() const noexcept {
    return cast<T>();
  }
  void reset() noexcept { Binding = decltype(Binding){}; }
};

#define HSH_CASTABLE_TEXTURE(derived, coordt)                                  \
  struct derived : texture_typeless {                                          \
    using MappedType = void;                                                   \
    template <typename... Args>                                                \
    explicit derived(Args &&... args) noexcept                                 \
        : texture_typeless(detail::TypeInfo::MakeTypeInfo<decltype(*this)>(),  \
                           std::forward<Args>(args)...) {}                     \
    template <typename T>                                                      \
    scalar_to_vector_t<T, 4> sample(coordt, sampler = {}) const noexcept {     \
      return {};                                                               \
    }                                                                          \
    template <typename T>                                                      \
    scalar_to_vector_t<T, 4> sample_bias(coordt, float bias = 0.f,             \
                                         sampler = {}) const noexcept {        \
      return {};                                                               \
    }                                                                          \
  };
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

  operator bool() const noexcept { return Binding.IsValid(); }
  template <typename T>
  scalar_to_vector_t<T, 4> sample(float2, sampler = {}) const noexcept {
    return {};
  }
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
  ConstColor,
  InvConstColor,
  ConstAlpha,
  InvConstAlpha,
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
  RGBA8_UNORM,
  R16_UNORM,
  RG16_UNORM,
  RGBA16_UNORM,
  R32_UINT,
  RG32_UINT,
  RGB32_UINT,
  RGBA32_UINT,
  R8_SNORM,
  RG8_SNORM,
  RGBA8_SNORM,
  R16_SNORM,
  RG16_SNORM,
  RGBA16_SNORM,
  R32_SINT,
  RG32_SINT,
  RGB32_SINT,
  RGBA32_SINT,
  R32_SFLOAT,
  RG32_SFLOAT,
  RGB32_SFLOAT,
  RGBA32_SFLOAT,
  BC1_UNORM,
  BC2_UNORM,
  BC3_UNORM
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
struct color_attachment : base_attribute<true> {
  static constexpr BlendFactor SrcColorBlend = SrcColorBlendFactor;
  static constexpr BlendFactor DstColorBlend = DstColorBlendFactor;
  static constexpr BlendFactor SrcAlphaBlend = SrcAlphaBlendFactor;
  static constexpr BlendFactor DstAlphaBlend = DstAlphaBlendFactor;
};
struct dual_source : base_attribute<true> {};
template <Topology T = Triangles> struct topology : base_attribute<> {};
template <unsigned P = 0> struct patch_control_points : base_attribute<> {};
template <CullMode CM = CullNone> struct cull_mode : base_attribute<> {};
template <Compare C = Always> struct depth_compare : base_attribute<> {};
template <bool W = true> struct depth_write : base_attribute<> {};
struct direct_render : base_attribute<> {};
struct high_priority : base_attribute<> {};
template <bool E = false>
struct early_depth_stencil : base_attribute<false, true> {};

namespace detail {
template <std::size_t TargetIdx, std::size_t CurIdx, typename... Attrs>
struct ColorAttachmentSelectorImpl {};

template <std::size_t TargetIdx, std::size_t CurIdx, typename CA,
          typename... RemAttrs>
struct ColorAttachmentSelectorImpl<TargetIdx, CurIdx, CA, RemAttrs...>
    : ColorAttachmentSelectorImpl<TargetIdx, CurIdx, RemAttrs...> {};

template <std::size_t TargetIdx, std::size_t CurIdx,
          BlendFactor SrcColorBlendFactor, BlendFactor DstColorBlendFactor,
          BlendOp ColorBlendOp, BlendFactor SrcAlphaBlendFactor,
          BlendFactor DstAlphaBlendFactor, BlendOp AlphaBlendOp,
          ColorComponentFlags ColorWriteComponents, typename... RemAttrs>
struct ColorAttachmentSelectorImpl<
    TargetIdx, CurIdx,
    color_attachment<SrcColorBlendFactor, DstColorBlendFactor, ColorBlendOp,
                     SrcAlphaBlendFactor, DstAlphaBlendFactor, AlphaBlendOp,
                     ColorWriteComponents>,
    RemAttrs...>
    : std::conditional_t<
          TargetIdx == CurIdx,
          color_attachment<SrcColorBlendFactor, DstColorBlendFactor,
                           ColorBlendOp, SrcAlphaBlendFactor,
                           DstAlphaBlendFactor, AlphaBlendOp,
                           ColorWriteComponents>,
          ColorAttachmentSelectorImpl<TargetIdx, CurIdx + 1, RemAttrs...>> {};
} // namespace detail

template <typename... Attrs> struct pipeline {
  hsh::float4 position;
  static constexpr std::size_t color_attachment_count =
      ((Attrs::is_ca ? 1 : 0) + ...);
  std::array<hsh::float4, color_attachment_count> color_out;
  template <std::size_t Idx>
  static constexpr BlendFactor SrcColorBlendFactor =
      detail::ColorAttachmentSelectorImpl<Idx, 0, Attrs...>::SrcColorBlend;
  template <std::size_t Idx>
  static constexpr BlendFactor DstColorBlendFactor =
      detail::ColorAttachmentSelectorImpl<Idx, 0, Attrs...>::DstColorBlend;
  template <std::size_t Idx>
  static constexpr BlendFactor SrcAlphaBlendFactor =
      detail::ColorAttachmentSelectorImpl<Idx, 0, Attrs...>::SrcAlphaBlend;
  template <std::size_t Idx>
  static constexpr BlendFactor DstAlphaBlendFactor =
      detail::ColorAttachmentSelectorImpl<Idx, 0, Attrs...>::DstAlphaBlend;
  uint32_t vertex_id;
  uint32_t instance_id;
};
} // namespace pipeline

namespace detail {

template <typename T> inline const T *reloc(const T *Ptr) noexcept {
  assert(reinterpret_cast<uintptr_t>(Ptr) < __hsh_reloc_length &&
         "hsh reloc mapping overrun");
  return reinterpret_cast<const T *>(__hsh_reloc_base +
                                     reinterpret_cast<uintptr_t>(Ptr));
}

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

/* Holds sampler object as loaded into graphics API */
template <hsh::Target T> struct SamplerObject {};

/* Associates texture with sampler object index in shader data. */
struct SamplerBinding {
  texture_typeless Tex; /* Reference to actual texture being sampled. */
  unsigned Idx = 0;     /* Index of sampler information in const data. */
  unsigned TexIdx =
      0; /* Index of texture being sampled (as listed in parameters). */
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
  case RGBA8_UNORM:
    return 4;
  case R16_UNORM:
    return 2;
  case RG16_UNORM:
    return 4;
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
  case RGBA8_SNORM:
    return 4;
  case R16_SNORM:
    return 2;
  case RG16_SNORM:
    return 4;
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
  case BC1_UNORM:
  case BC2_UNORM:
  case BC3_UNORM:
  default:
    return 1;
  }
}

constexpr std::size_t HshFormatToTexelSizeShift(Format format) noexcept {
  switch (format) {
  default:
    return 0;
  case BC1_UNORM:
    return 1;
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
    return !(SrcColorBlendFactor == One && DstColorBlendFactor == Zero &&
             ColorBlendOp == Add && SrcAlphaBlendFactor == One &&
             DstAlphaBlendFactor == Zero && AlphaBlendOp == Add);
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
  bool DirectRenderPass = false;
  constexpr PipelineInfo() noexcept = default;
  constexpr PipelineInfo(enum Topology Topology, unsigned PatchControlPoints,
                         enum CullMode CullMode, enum Compare DepthCompare,
                         bool DepthWrite, bool DirectRenderPass) noexcept
      : Topology(Topology), PatchControlPoints(PatchControlPoints),
        CullMode(CullMode), DepthCompare(DepthCompare), DepthWrite(DepthWrite),
        DirectRenderPass(DirectRenderPass) {}
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
  using ObjectRef = ShaderObject<T> *;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = SamplerObject<T> *;
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
constexpr uint32_t MipSize1D(uint32_t width, uint32_t layers,
                             uint32_t texelSize, uint32_t texelShift,
                             bool NotZero) noexcept {
  return NotZero ? (width * layers * texelSize >> texelShift) : 0;
}
template <uint32_t... Idx>
constexpr uint32_t
MipOffset1D(uint32_t width, uint32_t layers, uint32_t texelSize,
            uint32_t texelShift, uint32_t Mip,
            std::integer_sequence<uint32_t, Idx...>) noexcept {
  return (MipSize1D(width >> Idx, layers, texelSize, texelShift, Idx < Mip) +
          ...);
}
constexpr uint32_t MipOffset1D(uint32_t width, uint32_t layers,
                               uint32_t texelSize, uint32_t texelShift,
                               uint32_t Mip) noexcept {
  return MipOffset1D(width, layers, texelSize, texelShift, Mip,
                     std::make_integer_sequence<uint32_t, MaxMipCount>());
}

constexpr uint32_t MipSize2D(uint32_t width, uint32_t height, uint32_t layers,
                             uint32_t texelSize, uint32_t texelShift,
                             bool NotZero) noexcept {
  return NotZero ? (width * height * layers * texelSize >> texelShift) : 0;
}
template <uint32_t... Idx>
constexpr uint32_t
MipOffset2D(uint32_t width, uint32_t height, uint32_t layers,
            uint32_t texelSize, uint32_t texelShift, uint32_t Mip,
            std::integer_sequence<uint32_t, Idx...>) noexcept {
  return (MipSize2D(width >> Idx, height >> Idx, layers, texelSize, texelShift,
                    Idx < Mip) +
          ...);
}
constexpr uint32_t MipOffset2D(uint32_t width, uint32_t height, uint32_t layers,
                               uint32_t texelSize, uint32_t texelShift,
                               uint32_t Mip) noexcept {
  return MipOffset2D(width, height, layers, texelSize, texelShift, Mip,
                     std::make_integer_sequence<uint32_t, MaxMipCount>());
}

constexpr uint32_t MipSize3D(uint32_t width, uint32_t height, uint32_t depth,
                             uint32_t texelSize, uint32_t texelShift,
                             bool NotZero) noexcept {
  return NotZero ? (width * height * depth * texelSize >> texelShift) : 0;
}
template <uint32_t... Idx>
constexpr uint32_t
MipOffset3D(uint32_t width, uint32_t height, uint32_t depth, uint32_t texelSize,
            uint32_t texelShift, uint32_t Mip,
            std::integer_sequence<uint32_t, Idx...>) noexcept {
  return (MipSize3D(width >> Idx, height >> Idx, depth >> Idx, texelSize,
                    texelShift, Idx < Mip) +
          ...);
}
constexpr uint32_t MipOffset3D(uint32_t width, uint32_t height, uint32_t depth,
                               uint32_t texelSize, uint32_t texelShift,
                               uint32_t Mip) noexcept {
  return MipOffset3D(width, height, depth, texelSize, texelShift, Mip,
                     std::make_integer_sequence<uint32_t, MaxMipCount>());
}
} // namespace buffer_math

typedef void (*BuildFunc)() noexcept;
struct FuncPair {
  BuildFunc Create, Destroy;
};

template <bool HighPrio> class GlobalListNode {
  static GlobalListNode *Head;
  std::array<FuncPair, std::size_t(ActiveTarget::MaxActiveTarget)> Funcs;
  GlobalListNode *Next;

public:
  static GlobalListNode *GetHead() noexcept { return Head; }

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
  static std::size_t CountAll() noexcept {
    std::size_t Ret = 0;
    for (auto *Node = GetHead(); Node; Node = Node->GetNext())
      ++Ret;
    return Ret;
  }
};
template <> inline GlobalListNode<true> *GlobalListNode<true>::Head = nullptr;
template <> inline GlobalListNode<false> *GlobalListNode<false>::Head = nullptr;

template <bool HighPrio, template <hsh::Target> typename Impl>
struct PipelineCoordinatorNode : GlobalListNode<HighPrio> {
  using base = GlobalListNode<HighPrio>;
  PipelineCoordinatorNode()
      : GlobalListNode<HighPrio>{
#define HSH_ACTIVE_TARGET(Enumeration)                                         \
  FuncPair{Impl<Target::Enumeration>::Create,                                  \
           Impl<Target::Enumeration>::Destroy},
#include "targets.def"
        } {
  }
};

template <bool HighPrio, typename... B> struct PipelineCoordinator {
  template <hsh::Target T> struct Impl {
    static void Create() noexcept {
      PipelineBuilder<T>::template CreatePipelines<B...>();
    }
    static void Destroy() noexcept {
      PipelineBuilder<T>::template DestroyPipelines<B...>();
    }
  };
  static PipelineCoordinatorNode<HighPrio, Impl> global;
};

/*
 * This macro is internally expanded within the hsh generator
 * for any identifiers prefixed with hsh_ being assigned or returned.
 */
struct _DummyBinder {
  static void Bind(hsh::binding &) noexcept {}
};
#define _hsh_dummy(...) _bind<hsh::detail::_DummyBinder>()

} // namespace detail
} // namespace hsh
