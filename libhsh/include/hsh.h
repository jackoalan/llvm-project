#pragma once

#include <array>
#include <type_traits>
#include <utility>

//#define HSH_ENABLE_VULKAN 0

#ifdef HSH_ENABLE_VULKAN
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#endif

namespace hsh {
namespace detail {
struct base_vertex_buffer {};
} // namespace detail
struct float3;
struct float2;
struct float4 {
  float x, y, z, w;
  float4() = default;
  constexpr float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
  constexpr explicit float4(const float3 &other, float w = 1.f);
  constexpr explicit float4(const float2 &other, float z = 0.f, float w = 1.f);
  void operator+=(const float4 &other) {}
  void operator*=(const float4 &other) {}
  float3 xyz() const;
};
struct float3 {
  float x, y, z;
  float3() = default;
  constexpr float3(float x, float y, float z) : x(x), y(y), z(z) {}
  constexpr explicit float3(float f) : x(f), y(f), z(f) {}
  float3 operator-() const { return float3{-x, -y, -z}; };
  float3 operator*(float other) {
    return float3{x * other, y * other, z * other};
  }
};
float3 float4::xyz() const { return float3{x, y, z}; }
struct float2 {
  float x, y;
  float2() = default;
  constexpr float2(float x, float y) : x(x), y(y) {}
  constexpr explicit float2(float f) : x(f), y(f) {}
  float2 operator-() const { return float2{-x, -y}; };
};
constexpr float4::float4(const hsh::float3 &other, float w)
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr float4::float4(const hsh::float2 &other, float z, float w)
    : x(other.x), y(other.y), z(z), w(w) {}
struct int3;
struct int2;
struct int4 {
  std::int32_t x, y, z, w;
  int4() = default;
  constexpr explicit int4(const int3 &other, std::int32_t w = 0);
  constexpr explicit int4(const int2 &other, std::int32_t z = 0,
                          std::int32_t w = 0);
  void operator+=(const int4 &other) {}
  void operator*=(const int4 &other) {}
};
struct int3 {
  std::int32_t x, y, z;
  int3() = default;
  constexpr int3(std::int32_t x, std::int32_t y, std::int32_t z)
      : x(x), y(y), z(z) {}
  constexpr explicit int3(std::int32_t f) : x(f), y(f), z(f) {}
  int3 operator-() const { return int3{-x, -y, -z}; };
  int3 operator*(std::int32_t other) {
    return int3{x * other, y * other, z * other};
  }
};
struct int2 {
  std::int32_t x, y;
  int2() = default;
  constexpr int2(std::int32_t x, std::int32_t y) : x(x), y(y) {}
  constexpr explicit int2(std::int32_t f) : x(f), y(f) {}
  int2 operator-() const { return int2{-x, -y}; };
};
constexpr int4::int4(const hsh::int3 &other, std::int32_t w)
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr int4::int4(const hsh::int2 &other, std::int32_t z, std::int32_t w)
    : x(other.x), y(other.y), z(z), w(w) {}
struct uint3;
struct uint2;
struct uint4 {
  std::uint32_t x, y, z, w;
  uint4() = default;
  constexpr explicit uint4(const uint3 &other, std::uint32_t w = 0);
  constexpr explicit uint4(const uint2 &other, std::uint32_t z = 0,
                           std::uint32_t w = 0);
  void operator+=(const uint4 &other) {}
  void operator*=(const uint4 &other) {}
};
struct uint3 {
  std::uint32_t x, y, z;
  uint3() = default;
  constexpr uint3(std::uint32_t x, std::uint32_t y, std::uint32_t z)
      : x(x), y(y), z(z) {}
  constexpr explicit uint3(std::uint32_t f) : x(f), y(f), z(f) {}
  uint3 operator-() const { return uint3{-x, -y, -z}; };
  uint3 operator*(std::uint32_t other) {
    return uint3{x * other, y * other, z * other};
  }
};
struct uint2 {
  std::uint32_t x, y;
  uint2() = default;
  constexpr uint2(std::uint32_t x, std::uint32_t y) : x(x), y(y) {}
  constexpr explicit uint2(std::uint32_t f) : x(f), y(f) {}
  uint2 operator-() const { return uint2{-x, -y}; };
};
constexpr uint4::uint4(const hsh::uint3 &other, std::uint32_t w)
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr uint4::uint4(const hsh::uint2 &other, std::uint32_t z,
                       std::uint32_t w)
    : x(other.x), y(other.y), z(z), w(w) {}
struct float4x4 {
  float4 cols[4];
  float4x4() = default;
  float4 &operator[](std::size_t idx) { return cols[idx]; }
  float4x4 operator*(const float4x4 &other) const { return float4x4{}; };
  float4 operator*(const float4 &other) const { return float4{}; };
};
struct float3x3 {
  float3x3() = default;
  float3 cols[3];
  float3x3(const float4x4 &other)
      : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3x3 operator*(const float3x3 &other) const { return float3x3{}; };
  float3 operator*(const float3 &other) const { return float3{}; };
};
enum class filter { linear, nearest };
enum class wrap { repeat, clamp_to_edge };
struct sampler {
  filter m_filter = filter::linear;
  wrap m_wrap = wrap::repeat;
  constexpr sampler() = default;
  template <typename... Rest>
  constexpr sampler(filter f, Rest... qs) : sampler(qs...) {
    m_filter = f;
  }
  template <typename... Rest>
  constexpr sampler(wrap w, Rest... qs) : sampler(qs...) {
    m_wrap = w;
  }
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
template <typename T> struct texture1d {
  scalar_to_vector_t<T, 4> sample(float, sampler = {}) const { return {}; }
};
template <typename T> struct texture1d_array {
  scalar_to_vector_t<T, 4> sample(float2, sampler = {}) const { return {}; }
};
template <typename T> struct texture2d {
  scalar_to_vector_t<T, 4> sample(float2, sampler = {}) const { return {}; }
};
template <typename T> struct texture2d_array {
  scalar_to_vector_t<T, 4> sample(float3, sampler = {}) const { return {}; }
};
template <typename T> struct texture3d {
  scalar_to_vector_t<T, 4> sample(float3, sampler = {}) const { return {}; }
};
template <typename T> struct texturecube {
  scalar_to_vector_t<T, 4> sample(float3, sampler = {}) const { return {}; }
};
template <typename T> struct texturecube_array {
  scalar_to_vector_t<T, 4> sample(float4, sampler = {}) const { return {}; }
};
struct vertex_format {};
template <typename T> struct vertex_buffer : detail::base_vertex_buffer {
  const T *ref;
  std::size_t len;
  template <std::size_t N>
  vertex_buffer(std::size_t, const T (&ref)[N]) : ref(ref), len(N) {}
};
float dot(const float3 &, const float3 &);

enum Target {
  GLSL,
  HLSL,
  DXBC,
  DXIL,
  VULKAN_SPIRV,
  METAL,
  METAL_BIN_MAC,
  METAL_BIN_IOS,
  METAL_BIN_TVOS,
};

enum Stage { Vertex, Control, Evaluation, Geometry, Fragment, MaxStage };

enum Topology { Points, Lines, Triangles, TriangleStrips, MaxTopology };

enum BlendFactor { Zero, One, SrcAlpha, InvSrcAlpha, DstAlpha, InvDstAlpha };

namespace detail {

template <typename WordType> struct ShaderDataBlob {
  std::size_t Size = 0;
  const WordType *Data = nullptr;
  std::uint64_t Hash = 0;
  constexpr ShaderDataBlob() = default;
  template <typename T>
  constexpr ShaderDataBlob(const T Data, std::uint64_t Hash) noexcept
      : Size(std::extent<T>::value), Data(Data), Hash(Hash) {}
  constexpr operator bool() const { return Data != nullptr; }
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

enum InputRate : std::uint8_t { PerVertex, PerInstance };

/* Holds constant vertex buffer binding information */
struct VertexBinding {
  std::uint32_t Stride : 24;
  std::uint8_t Binding : 7;
  enum InputRate InputRate : 1;
  constexpr VertexBinding() : Stride(0), Binding(0), InputRate(PerVertex) {}
  constexpr VertexBinding(std::uint8_t Binding, std::uint32_t Stride,
                          enum InputRate InputRate) noexcept
      : Stride(Stride), Binding(Binding), InputRate(InputRate) {}
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

/* Holds constant vertex attribute binding information */
struct VertexAttribute {
  std::uint32_t Offset = 0;
  std::uint8_t Binding = 0;
  enum Format Format = R8_UNORM;
  constexpr VertexAttribute() = default;
  constexpr VertexAttribute(std::uint8_t Binding, enum Format Format,
                            std::uint32_t Offset) noexcept
      : Offset(Offset), Binding(Binding), Format(Format) {}
};

template <Target T, std::uint32_t NStages, std::uint32_t NBindings,
          std::uint32_t NAttributes>
struct ShaderConstData {
  std::array<ShaderCode<T>, NStages> StageCodes;
  std::array<VertexBinding, NBindings> Bindings;
  std::array<VertexAttribute, NAttributes> Attributes;

  constexpr ShaderConstData(std::array<ShaderCode<T>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A)
      : StageCodes(S), Bindings(B), Attributes(A) {}
};

template <Target T, std::uint32_t NStages> struct ShaderData {
  constexpr ShaderData() = default;
};

#ifdef HSH_ENABLE_VULKAN
constexpr vk::Format HshToVkFormat(Format Format) {
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

constexpr vk::VertexInputRate HshToVkInputRate(InputRate InputRate) {
  switch (InputRate) {
  case PerVertex:
    return vk::VertexInputRate::eVertex;
  case PerInstance:
    return vk::VertexInputRate::eInstance;
  }
}

template <> struct ShaderCode<VULKAN_SPIRV> {
  enum Stage Stage = Stage::Vertex;
  ShaderDataBlob<uint32_t> Blob;
  constexpr ShaderCode() = default;
  constexpr ShaderCode(enum Stage Stage, ShaderDataBlob<uint32_t> Blob) noexcept
      : Stage(Stage), Blob(Blob) {}
};

template <> struct ShaderObject<VULKAN_SPIRV> {
  vk::ShaderModule ShaderModule;
  constexpr ShaderObject() = default;
};

template <std::uint32_t NStages, std::uint32_t NBindings,
          std::uint32_t NAttributes>
struct ShaderConstData<VULKAN_SPIRV, NStages, NBindings, NAttributes> {
  std::array<vk::ShaderModuleCreateInfo, NStages> StageCodes;
  std::array<vk::VertexInputBindingDescription, NBindings>
      VertexBindingDescriptions;
  std::array<vk::VertexInputAttributeDescription, NAttributes>
      VertexAttributeDescriptions;
  vk::PipelineVertexInputStateCreateInfo VertexInputState;
  vk::PipelineInputAssemblyStateCreateInfo InputAssemblyState;
  vk::PipelineTessellationStateCreateInfo TessellationState;
  vk::PipelineRasterizationStateCreateInfo RasterizationState;
  vk::PipelineDepthStencilStateCreateInfo DepthStencilState;
  vk::PipelineColorBlendStateCreateInfo ColorBlendState;

  template <std::size_t... SSeq, std::size_t... BSeq, std::size_t... ASeq>
  constexpr ShaderConstData(std::array<ShaderCode<VULKAN_SPIRV>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A,
                            std::index_sequence<SSeq...>,
                            std::index_sequence<BSeq...>,
                            std::index_sequence<ASeq...>)
      : StageCodes{vk::ShaderModuleCreateInfo{
            {}, std::get<SSeq>(S).Blob.Size, std::get<SSeq>(S).Blob.Data}...},
        VertexBindingDescriptions{vk::VertexInputBindingDescription{
            BSeq, std::get<BSeq>(B).Stride,
            HshToVkInputRate(std::get<BSeq>(B).InputRate)}...},
        VertexAttributeDescriptions{vk::VertexInputAttributeDescription{
            ASeq, std::get<ASeq>(A).Binding,
            HshToVkFormat(std::get<ASeq>(A).Format),
            std::get<ASeq>(A).Offset}...},
        VertexInputState{{},
                         NBindings,
                         VertexBindingDescriptions.data(),
                         NAttributes,
                         VertexAttributeDescriptions.data()} {}

  constexpr ShaderConstData(std::array<ShaderCode<VULKAN_SPIRV>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A)
      : ShaderConstData(S, B, A, std::make_index_sequence<NStages>(),
                        std::make_index_sequence<NBindings>(),
                        std::make_index_sequence<NAttributes>()) {}
};

template <std::uint32_t NStages> struct ShaderData<VULKAN_SPIRV, NStages> {
  using ObjectRef = std::reference_wrapper<ShaderObject<VULKAN_SPIRV>>;
  std::array<ObjectRef, NStages> ShaderObjects;
  constexpr ShaderData(std::array<ObjectRef, NStages> S) : ShaderObjects(S) {}
};
#endif

struct GlobalListNode;
GlobalListNode *GlobalListHead = nullptr;
struct GlobalListNode {
  typedef void (*RegisterFunc)();
  RegisterFunc Func;
  GlobalListNode *Next;
  explicit GlobalListNode(RegisterFunc Func) noexcept
      : Func(Func), Next(GlobalListHead) {
    GlobalListHead = this;
  }
};

template <class ImplClass> class GenBase {
  // std::array<ResourceHandlers::uniform_handle, MaxStage> UniformHandles;
protected:
  static void global_build() {}
  template <Stage S, typename T> void push_uniform(const T &uniform) {
    // UniformHandles[S] = ResourceHandlers::push_uniform<S, T>(uniform);
  }

public:
  void draw(std::size_t, std::size_t) {}
  void bind(hsh::detail::base_vertex_buffer) {}
};

class GenDummy {
public:
  void draw(std::size_t, std::size_t) {}
  void bind(hsh::detail::base_vertex_buffer) {}
};
/*
 * This macro is internally expanded within the hsh generator
 * for any identifiers prefixed with hsh_ being assigned.
 */
#define _hsh_dummy                                                             \
  ::hsh::detail::GenDummy();                                                   \
  [[hsh::generator_lambda]]
} // namespace detail
} // namespace hsh
