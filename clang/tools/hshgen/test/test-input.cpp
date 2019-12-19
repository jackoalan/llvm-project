#include <type_traits>
#include <utility>
#include <array>

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

namespace hsh {
namespace detail {
struct base_vertex_buffer {};
}
struct float3;
struct float2;
struct float4 {
  float x, y, z, w;
  float4() = default;
  constexpr explicit float4(const float3& other, float w = 1.f);
  constexpr explicit float4(const float2& other, float z = 0.f, float w = 1.f);
  void operator+=(const float4& other) {}
  void operator*=(const float4& other) {}
  float3 xyz() const;
};
struct float3 {
  float x, y, z;
  float3() = default;
  constexpr float3(float x, float y, float z) : x(x), y(y), z(z) {}
  constexpr explicit float3(float f) : x(f), y(f), z(f) {}
  float3 operator-() const { return float3{-x, -y, -z}; };
  float3 operator*(float other) { return float3{x * other, y * other, z * other}; }
};
float3 float4::xyz() const { return float3{x, y, z}; }
struct float2 {
  float x, y;
  float2() = default;
  constexpr float2(float x, float y) : x(x), y(y) {}
  constexpr explicit float2(float f) : x(f), y(f) {}
  float2 operator-() const { return float2{-x, -y}; };
};
constexpr float4::float4(const hsh::float3 &other, float w) : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr float4::float4(const hsh::float2 &other, float z, float w) : x(other.x), y(other.y), z(z), w(w) {}
struct int3;
struct int2;
struct int4 {
  std::int32_t x, y, z, w;
  int4() = default;
  constexpr explicit int4(const int3& other, std::int32_t w = 0);
  constexpr explicit int4(const int2& other, std::int32_t z = 0, std::int32_t w = 0);
  void operator+=(const int4& other) {}
  void operator*=(const int4& other) {}
};
struct int3 {
  std::int32_t x, y, z;
  int3() = default;
  constexpr int3(std::int32_t x, std::int32_t y, std::int32_t z) : x(x), y(y), z(z) {}
  constexpr explicit int3(std::int32_t f) : x(f), y(f), z(f) {}
  int3 operator-() const { return int3{-x, -y, -z}; };
  int3 operator*(std::int32_t other) { return int3{x * other, y * other, z * other}; }
};
struct int2 {
  std::int32_t x, y;
  int2() = default;
  constexpr int2(std::int32_t x, std::int32_t y) : x(x), y(y) {}
  constexpr explicit int2(std::int32_t f) : x(f), y(f) {}
  int2 operator-() const { return int2{-x, -y}; };
};
constexpr int4::int4(const hsh::int3 &other, std::int32_t w) : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr int4::int4(const hsh::int2 &other, std::int32_t z, std::int32_t w) : x(other.x), y(other.y), z(z), w(w) {}
struct uint3;
struct uint2;
struct uint4 {
  std::uint32_t x, y, z, w;
  uint4() = default;
  constexpr explicit uint4(const uint3& other, std::uint32_t w = 0);
  constexpr explicit uint4(const uint2& other, std::uint32_t z = 0, std::uint32_t w = 0);
  void operator+=(const uint4& other) {}
  void operator*=(const uint4& other) {}
};
struct uint3 {
  std::uint32_t x, y, z;
  uint3() = default;
  constexpr uint3(std::uint32_t x, std::uint32_t y, std::uint32_t z) : x(x), y(y), z(z) {}
  constexpr explicit uint3(std::uint32_t f) : x(f), y(f), z(f) {}
  uint3 operator-() const { return uint3{-x, -y, -z}; };
  uint3 operator*(std::uint32_t other) { return uint3{x * other, y * other, z * other}; }
};
struct uint2 {
  std::uint32_t x, y;
  uint2() = default;
  constexpr uint2(std::uint32_t x, std::uint32_t y) : x(x), y(y) {}
  constexpr explicit uint2(std::uint32_t f) : x(f), y(f) {}
  uint2 operator-() const { return uint2{-x, -y}; };
};
constexpr uint4::uint4(const hsh::uint3 &other, std::uint32_t w) : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr uint4::uint4(const hsh::uint2 &other, std::uint32_t z, std::uint32_t w) : x(other.x), y(other.y), z(z), w(w) {}
struct float4x4 {
  float4 cols[4];
  float4x4() = default;
  float4& operator[](std::size_t idx) { return cols[idx]; }
  float4x4 operator*(const float4x4& other) const { return float4x4{}; };
  float4 operator*(const float4& other) const { return float4{}; };
};
struct float3x3 {
  float3x3() = default;
  float3 cols[3];
  float3x3(const float4x4& other) : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3x3 operator*(const float3x3& other) const { return float3x3{}; };
  float3 operator*(const float3& other) const { return float3{}; };
};
enum class filter { linear, nearest };
enum class wrap { repeat, clamp_to_edge };
struct sampler {
  filter m_filter = filter::linear;
  wrap m_wrap = wrap::repeat;
  constexpr sampler() = default;
  template<typename... Rest>
  constexpr sampler(filter f, Rest... qs) : sampler(qs...) { m_filter = f; }
  template<typename... Rest>
  constexpr sampler(wrap w, Rest... qs) : sampler(qs...) { m_wrap = w; }
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
template <> struct vector_to_scalar<unsigned int> { using type = unsigned int; };
template <> struct vector_to_scalar<uint2> { using type = unsigned int; };
template <> struct vector_to_scalar<uint3> { using type = unsigned int; };
template <> struct vector_to_scalar<uint4> { using type = unsigned int; };
template <typename T> using vector_to_scalar_t = typename vector_to_scalar<T>::type;
template <typename T, int N> struct scalar_to_vector {};
template <> struct scalar_to_vector<float, 1> { using type = float; };
template <> struct scalar_to_vector<float, 2> { using type = float2; };
template <> struct scalar_to_vector<float, 3> { using type = float3; };
template <> struct scalar_to_vector<float, 4> { using type = float4; };
template <> struct scalar_to_vector<int, 1> { using type = int; };
template <> struct scalar_to_vector<int, 2> { using type = int2; };
template <> struct scalar_to_vector<int, 3> { using type = int3; };
template <> struct scalar_to_vector<int, 4> { using type = int4; };
template <> struct scalar_to_vector<unsigned int, 1> { using type = unsigned int; };
template <> struct scalar_to_vector<unsigned int, 2> { using type = uint2; };
template <> struct scalar_to_vector<unsigned int, 3> { using type = uint3; };
template <> struct scalar_to_vector<unsigned int, 4> { using type = uint4; };
template <typename T, int N> using scalar_to_vector_t = typename scalar_to_vector<T, N>::type;
template <typename T>
struct texture1d {
  scalar_to_vector_t<T, 4> sample(float, sampler = {}) const { return {}; }
};
template <typename T>
struct texture1d_array {
  scalar_to_vector_t<T, 4> sample(float2, sampler = {}) const { return {}; }
};
template <typename T>
struct texture2d {
  scalar_to_vector_t<T, 4> sample(float2, sampler = {}) const { return {}; }
};
template <typename T>
struct texture2d_array {
  scalar_to_vector_t<T, 4> sample(float3, sampler = {}) const { return {}; }
};
template <typename T>
struct texture3d {
  scalar_to_vector_t<T, 4> sample(float3, sampler = {}) const { return {}; }
};
template <typename T>
struct texturecube {
  scalar_to_vector_t<T, 4> sample(float3, sampler = {}) const { return {}; }
};
template <typename T>
struct texturecube_array {
  scalar_to_vector_t<T, 4> sample(float4, sampler = {}) const { return {}; }
};
struct vertex_format {};
template<typename T>
struct vertex_buffer : detail::base_vertex_buffer {
  const T* ref;
  std::size_t len;
  template<std::size_t N>
  vertex_buffer(std::size_t, const T (&ref)[N]) : ref(ref), len(N) {}
};
float dot(const float3&, const float3&);

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

enum Stage {
  Vertex,
  Control,
  Evaluation,
  Geometry,
  Fragment,
  MaxStage
};

class VulkanResourceHandlers {
public:
  using uniform_handle = int;
  template<Stage S, typename T>
  static uniform_handle push_uniform(const T &uniform) {
    return {};
  }
};

struct _HshShaderDataBlob {
  std::size_t Size = 0;
  const std::uint8_t *Data = nullptr;
  std::uint64_t Hash = 0;
  constexpr _HshShaderDataBlob() = default;
  template <typename T>
  constexpr _HshShaderDataBlob(const T Data, std::uint64_t Hash) noexcept
  : Size(std::extent<T>::value), Data(Data), Hash(Hash) {}
  constexpr operator bool() const { return Data != nullptr; }
};

/* Holds shader stage enum and data blob reference for
 * individual stage object compilation */
template <hsh::Target T>
struct _HshShaderObject {
  enum Stage Stage = Stage::Vertex;
  _HshShaderDataBlob Blob;
  constexpr _HshShaderObject(enum Stage Stage, _HshShaderDataBlob Blob) noexcept
      : Stage(Stage), Blob(Blob) {}
};

enum _HshInputRate : std::uint8_t {
  PerVertex,
  PerInstance
};

/* Holds constant vertex buffer binding information */
struct _HshVertexBinding {
  std::uint32_t Stride : 24;
  std::uint8_t Binding : 7;
  _HshInputRate InputRate : 1;
  constexpr _HshVertexBinding() : Stride(0), Binding(0), InputRate(PerVertex) {}
  constexpr _HshVertexBinding(std::uint8_t Binding, std::uint32_t Stride,
                              _HshInputRate InputRate) noexcept
      : Stride(Stride), Binding(Binding), InputRate(InputRate) {}
};

enum _HshFormat : std::uint8_t {
  R8_UNORM,
  RG8_UNORM,
  RGB8_UNORM,
  RGBA8_UNORM,
  R16_UNORM,
  RG16_UNORM,
  RGB16_UNORM,
  RGBA16_UNORM,
  R8_SNORM,
  RG8_SNORM,
  RGB8_SNORM,
  RGBA8_SNORM,
  R16_SNORM,
  RG16_SNORM,
  RGB16_SNORM,
  RGBA16_SNORM,
  R32_SFLOAT,
  RG32_SFLOAT,
  RGB32_SFLOAT,
  RGBA32_SFLOAT,
};

/* Holds constant vertex attribute binding information */
struct _HshVertexAttribute {
  std::uint32_t Offset = 0;
  std::uint8_t Binding = 0;
  _HshFormat Format = R8_UNORM;
  constexpr _HshVertexAttribute() = default;
  constexpr _HshVertexAttribute(std::uint8_t Binding, _HshFormat Format,
                                std::uint32_t Offset) noexcept
      : Offset(Offset), Binding(Binding), Format(Format) {}
};

template <Target T, std::uint32_t NStages, std::uint32_t NBindings, std::uint32_t NAttributes>
struct _HshShaderData {
  std::array<_HshShaderObject<T>*, NStages> StagesCode{};
  std::array<_HshVertexBinding, NBindings> Bindings;
  std::array<_HshVertexAttribute, NAttributes> Attributes;

  /* Base case constructor */
  constexpr _HshShaderData(std::uint32_t CStage, std::uint32_t CBinding,
                           std::uint32_t CAttribute) noexcept {}

  /* Shader stage constructor */
  template <typename... Rest>
  constexpr _HshShaderData(std::uint32_t CStage, std::uint32_t CBinding,
                           std::uint32_t CAttribute,
                           _HshShaderObject<T> &S, Rest... R) noexcept
      : _HshShaderData(CStage + 1, CBinding, CAttribute, R...) {
    StagesCode[CStage] = &S;
  }

  /* Vertex binding constructor */
  template <typename... Rest>
  constexpr _HshShaderData(std::uint32_t CStage, std::uint32_t CBinding,
                           std::uint32_t CAttribute, _HshVertexBinding B,
                           Rest... R) noexcept
      : _HshShaderData(CStage, CBinding + 1, CAttribute, R...) {
    Bindings[CBinding] = B;
  }

  /* Vertex attribute constructor */
  template <typename... Rest>
  constexpr _HshShaderData(std::uint32_t CStage, std::uint32_t CBinding,
                           std::uint32_t CAttribute, _HshVertexAttribute A,
                           Rest... R) noexcept
      : _HshShaderData(CStage, CBinding, CAttribute + 1, R...) {
    Attributes[CAttribute] = A;
  }

  /* Entry constructor to initialize index counters */
  template <typename Something,
            std::enable_if_t<!std::is_integral_v<Something>, int> = 0,
            typename... Rest>
  constexpr _HshShaderData(Something S, Rest... R) noexcept
      : _HshShaderData(0, 0, 0, S, R...) {}
};
#if 1
constexpr VkFormat _HshToVkFormat(_HshFormat Format) {
  switch (Format) {
  case R8_UNORM:
    return VK_FORMAT_R8_UNORM;
  case RG8_UNORM:
    return VK_FORMAT_R8G8_UNORM;
  case RGB8_UNORM:
    return VK_FORMAT_R8G8B8_UNORM;
  case RGBA8_UNORM:
    return VK_FORMAT_R8G8B8A8_UNORM;
  case R16_UNORM:
    return VK_FORMAT_R16_UNORM;
  case RG16_UNORM:
    return VK_FORMAT_R16G16_UNORM;
  case RGB16_UNORM:
    return VK_FORMAT_R16G16B16_UNORM;
  case RGBA16_UNORM:
    return VK_FORMAT_R16G16B16A16_UNORM;
  case R8_SNORM:
    return VK_FORMAT_R8_SNORM;
  case RG8_SNORM:
    return VK_FORMAT_R8G8_SNORM;
  case RGB8_SNORM:
    return VK_FORMAT_R8G8B8_SNORM;
  case RGBA8_SNORM:
    return VK_FORMAT_R8G8B8A8_SNORM;
  case R16_SNORM:
    return VK_FORMAT_R16_SNORM;
  case RG16_SNORM:
    return VK_FORMAT_R16G16_SNORM;
  case RGB16_SNORM:
    return VK_FORMAT_R16G16B16_SNORM;
  case RGBA16_SNORM:
    return VK_FORMAT_R16G16B16A16_SNORM;
  case R32_SFLOAT:
    return VK_FORMAT_R32_SFLOAT;
  case RG32_SFLOAT:
    return VK_FORMAT_R32G32_SFLOAT;
  case RGB32_SFLOAT:
    return VK_FORMAT_R32G32B32_SFLOAT;
  case RGBA32_SFLOAT:
    return VK_FORMAT_R32G32B32A32_SFLOAT;
  }
}

constexpr vk::VertexInputRate _HshToVkInputRate(_HshInputRate InputRate) {
  switch (InputRate) {
  case PerVertex:
    return vk::VertexInputRate::eVertex;
  case PerInstance:
    return vk::VertexInputRate::eInstance;
  }
}

template <>
struct _HshShaderObject<VULKAN_SPIRV> {
  enum Stage Stage = Stage::Vertex;
  _HshShaderDataBlob Blob;
  vk::ShaderModule Module;
  constexpr _HshShaderObject(enum Stage Stage, _HshShaderDataBlob Blob) noexcept
      : Stage(Stage), Blob(Blob) {}
};

template <std::uint32_t NStages, std::uint32_t NBindings, std::uint32_t NAttributes>
struct _HshShaderData<VULKAN_SPIRV, NStages, NBindings, NAttributes> {
  std::array<_HshShaderObject<VULKAN_SPIRV>*, MaxStage> StagesCode{};
  std::array<vk::VertexInputBindingDescription, NBindings> VertexBindingDescriptions;
  std::array<vk::VertexInputAttributeDescription, NAttributes> VertexAttributeDescriptions;
  vk::PipelineVertexInputStateCreateInfo VertexInputState;
  vk::PipelineInputAssemblyStateCreateInfo InputAssemblyState;
  vk::PipelineTessellationStateCreateInfo TessellationState;
  vk::PipelineRasterizationStateCreateInfo RasterizationState;
  vk::PipelineDepthStencilStateCreateInfo DepthStencilState;
  vk::PipelineColorBlendStateCreateInfo ColorBlendState;

  /* Base case constructor */
  constexpr _HshShaderData(std::uint32_t CStage, std::uint32_t CBinding,
                           std::uint32_t CAttribute) noexcept {}

  /* Shader stage constructor */
  template <typename... Rest>
  constexpr _HshShaderData(std::uint32_t CStage, std::uint32_t CBinding,
                           std::uint32_t CAttribute,
                           _HshShaderObject<VULKAN_SPIRV> &S,
                           Rest... R) noexcept
      : _HshShaderData(CStage + 1, CBinding, CAttribute, R...) {
    StagesCode[CStage] = &S;
  }

  /* Vertex binding constructor */
  template <typename... Rest>
  constexpr _HshShaderData(std::uint32_t CStage, std::uint32_t CBinding,
                           std::uint32_t CAttribute, _HshVertexBinding B,
                           Rest... R) noexcept
      : _HshShaderData(CStage, CBinding + 1, CAttribute, R...) {
    VertexBindingDescriptions[CBinding]
        .setBinding(CBinding)
        .setStride(B.Stride)
        .setInputRate(_HshToVkInputRate(B.InputRate));
  }

  /* Vertex attribute constructor */
  template <typename... Rest>
  constexpr _HshShaderData(std::uint32_t CStage, std::uint32_t CBinding,
                           std::uint32_t CAttribute, _HshVertexAttribute A,
                           Rest... R) noexcept
      : _HshShaderData(CStage, CBinding, CAttribute + 1, R...) {
    VertexAttributeDescriptions[CAttribute]
        .setLocation(CAttribute)
        .setBinding(A.Binding)
        .setFormat(_HshToVkFormat(A.Format))
        .setOffset(A.Offset);
  }

  /* Entry constructor to initialize index counters */
  template <typename Something,
      std::enable_if_t<!std::is_integral_v<Something>, int> = 0,
      typename... Rest>
  constexpr _HshShaderData(Something S, Rest... R) noexcept
      : _HshShaderData(0, 0, 0, S, R...) {}
};
#endif
struct _HshGlobalListNode;
_HshGlobalListNode *_GlobalListHead = nullptr;
struct _HshGlobalListNode {
  typedef void(*RegisterFunc)();
  RegisterFunc Func;
  _HshGlobalListNode *Next;
  explicit _HshGlobalListNode(RegisterFunc Func) noexcept
  : Func(Func), Next(_GlobalListHead) {
    _GlobalListHead = this;
  }
};

using ResourceHandlers = VulkanResourceHandlers;

template<class ImplClass>
class _HshBase {
  //std::array<ResourceHandlers::uniform_handle, MaxStage> UniformHandles;
protected:
  static void global_build() {

  }
  template<Stage S, typename T>
  void push_uniform(const T &uniform) {
    //UniformHandles[S] = ResourceHandlers::push_uniform<S, T>(uniform);
  }
public:
  void draw(std::size_t, std::size_t) {}
  void bind(hsh::detail::base_vertex_buffer) {}
};

class _HshDummy {
public:
  void draw(std::size_t, std::size_t) {}
  void bind(hsh::detail::base_vertex_buffer) {}
};
/*
 * This macro is internally expanded within the hsh generator
 * for any identifiers prefixed with hsh_ being assigned.
 */
#define _hsh_dummy ::hsh::_HshDummy(); [[hsh::generator_lambda]]
}

#include "test-input.cpp.hshhead"

namespace MyNS {

struct MyFormat : hsh::vertex_format {
  hsh::float3 position;
  hsh::float3 normal;
  constexpr MyFormat(hsh::float3 position, hsh::float3 normal)
  : position(std::move(position)), normal(std::move(normal)) {}
};

enum class PostMode {
  Nothing,
  AddDynamicColor,
  MultiplyDynamicColor
};

void DrawSomething(const hsh::float4x4& xf, const hsh::float3& lightDir,
                   const hsh::float4& dynColor, PostMode postMode [[hsh::host_condition]]) {
  constexpr MyFormat MyBuffer[] = {
    {{-1.f, -1.f, 0.f}, {0.f, 0.f, 1.f}},
    {{ 1.f, -1.f, 0.f}, {0.f, 0.f, 1.f}},
    {{-1.f,  1.f, 0.f}, {0.f, 0.f, 1.f}},
    {{ 1.f,  1.f, 0.f}, {0.f, 0.f, 1.f}}
  };

  constexpr hsh::sampler TestSampler(hsh::wrap::repeat, hsh::filter::nearest);

  // Generated include defines anonymous struct and opens initialization bracket.
  // Captured values the shader is interested in are assigned to the first
  // constructor parameters bound at the end of the include.
  auto MyBinding = hsh_DrawSomething
  [&](const MyFormat& vertData [[hsh::vertex_buffer(0)]], // Stands in for current vertex (vertex shader) or
                                                          // interpolated value (fragment shader)
      hsh::texture2d<float> tex0 [[hsh::fragment_texture(0)]], // texture sampler
      hsh::float4& vertPos [[hsh::position]],            // Output of vertex shader
      hsh::float4& fragColor [[hsh::color_target(0)]]) { // Output of fragment shader

    /** Vertex Shader Pass
     * For final [[hsh::position]] assignment:
     * Post-order traverse to [[hsh::vertex_buffer(0)]] and captured host variables;
     * promoting AST nodes from host to vertex as necessary. When an operator is promoted
     * and the other side's expression must be fetched from host, find or create a uniform
     * variable in the HostToVertex RecordDecl for the expression and store its handle in
     * the AST node.
     */

    /** Fragment Shader Pass
     * For each [[hsh::color_target]] assignment:
     * Post-order traverse to [[hsh::vertex_buffer(0)]] and captured host variables;
     * promoting AST nodes from host to vertex to fragment as necessary. When an operator
     * is promoted and the other side's expression must be fetched from host or vertex, find
     * or create a uniform/interpolated variable in the HostToFragment/VertexToFragment
     * RecordDecl for the expression and store its handle in the AST node.
     */

    /** Other Shader Stage Passes
     * Essentially just combining the end of the vertex shader pass with the beginning
     * of the fragment shader pass is how these are implemented. Relevant stage-specific
     * semantic attributes are defined as well.
     */

    /** Assembly Pass
     * 1. Create new host CompoundStmt and insert all root stmts that are not promoted to *all* stages.
     * 2. Create new host VarDecl of the HostToVertex type and initialize with gathered expressions.
     * 3. Create new vertex ParmVarDecl of the HostToVertex type.
     * 4. Create new vertex CompoundStmt and insert all root stmts that are marked as vertex.
     * 5. Insert [[hsh::position]] assignment into vertex CompoundStmt.
     * 6. Create new vertex VarDecl of the VertexToFragment type.
     * 7. Create assignment operators populating the VertexToFragment with gathered expressions.
     * 8. Create new fragment ParmVarDecl of the HostToFragment/VertexToFragment types.
     * 9. Create new fragment CompoundStmt and insert all root stmts that are marked as fragment.
     * 10. Insert all [[hsh::color_target]] assignments into vertex CompoundStmt.
     */

    /** Printing Pass
     * The host lambda generator and each shading language has a printing subclass
     * responsible for printing root declarations and interface directives, the function
     * definition begin, the relevant CompoundStmt traversal (with appropriate PrintingPolicy),
     * and the function definition end.
     *
     * Clang's built-in printing functions are modified to recognise shader printing policies
     * and print with the appropriate syntax variations (types, intrinsic functions, etc...).
     */

    // When vertData is a dependency, expressions are automatically transferred to the shader code.
    // xf will be a field of vertex uniform data.
    vertPos/*v*/ =/*v*/ xf/*h*/ */*v trigger host left-fetch*/ hsh::float4{vertData.position/*v*/, 1.f}/*v*/;



    // normalXf value is not needed within the shader until its multiplication with vertData.normal.
    // The host will compute the value up until the left binary operand; which is another field of vertex uniform data.
    hsh::float3x3 normalXf/*h*/ = xf/*h*/;

    // This multiplication will occur within the vertex shader.
    hsh::float3 finalNormal/*v*/ = hsh::float3x3{xf}/*h*/ */*v trigger host left-fetch*/ vertData.normal/*v*/;

    // lightDir becomes an rvalue after the negation operator. This rvalue will be a field of fragment uniform data.
    fragColor/*f*/ =/*f*/ hsh::float4{hsh::float3{
      tex0.sample({0.f, 0.f}, TestSampler).xyz() *
      hsh::dot(finalNormal/*v*/, -/*h*/lightDir/*h*/)/*v promoted to f via distribution test,
                                                        trigger vertex left fetch,
                                                        trigger host right fetch*/}/*f*/, 1.f}/*f*/;
  };

  // The hsh_binding object has handles of host-processed data buffers ready to draw with.
  MyBinding.bind(hsh::vertex_buffer{0, MyBuffer});
  MyBinding.draw(0, 4);
}
}
