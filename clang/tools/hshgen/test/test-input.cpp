#include <type_traits>
#include <utility>
#include <array>

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
};
struct float3 {
  float x, y, z;
  float3() = default;
  constexpr float3(float x, float y, float z) : x(x), y(y), z(z) {}
  constexpr explicit float3(float f) : x(f), y(f), z(f) {}
  float3 operator-() const { return float3{-x, -y, -z}; };
  float3(const float4& other) : x(other.x), y(other.y), z(other.z) {}
  float3 operator*(float other) { return float3{x * other, y * other, z * other}; }
};
struct float2 {
  float x, y;
  float2() = default;
  constexpr float2(float x, float y) : x(x), y(y) {}
  constexpr explicit float2(float f) : x(f), y(f) {}
  float2 operator-() const { return float2{-x, -y}; };
  float2(const float4& other) : x(other.x), y(other.y) {}
  float2(const float3& other) : x(other.x), y(other.y) {}
};
constexpr float4::float4(const hsh::float3 &other, float w) : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr float4::float4(const hsh::float2 &other, float z, float w) : x(other.x), y(other.y), z(z), w(w) {}
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
  float3x3(const float4x4& other) : cols{other.cols[0], other.cols[1], other.cols[2]} {}
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
template <typename T>
struct texture2d {
  float4 sample(float2, sampler = {}) const { return {}; }
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
  HT_GLSL,
  HT_HLSL,
  HT_DXBC,
  HT_DXIL,
  HT_SPIRV,
  HT_METAL,
  HT_METAL_BIN_MAC,
  HT_METAL_BIN_IOS,
  HT_METAL_BIN_TVOS,
};

enum Stage {
  VertexStage,
  ControlStage,
  EvaluationStage,
  GeometryStage,
  FragmentStage,
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
  std::size_t size = 0;
  const std::uint8_t *data = nullptr;
  std::uint64_t hash = 0;
  constexpr _HshShaderDataBlob() = default;
  template <typename T>
  constexpr _HshShaderDataBlob(const T data, std::uint64_t hash)
  : size(std::extent<T>::value), data(data), hash(hash) {}
};

template <Target T>
struct _HshShaderData {
  std::array<_HshShaderDataBlob, MaxStage> stages;
};

struct _HshGlobalListNode;
_HshGlobalListNode *_GlobalListHead = nullptr;
struct _HshGlobalListNode {
  typedef void(*RegisterFunc)();
  RegisterFunc func;
  _HshGlobalListNode *next;
  explicit _HshGlobalListNode(RegisterFunc func)
  : func(func), next(_GlobalListHead) {
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
    hsh::float3 finalNormal/*v*/ = normalXf/*h*/ */*v trigger host left-fetch*/ vertData.normal/*v*/;

    // lightDir becomes an rvalue after the negation operator. This rvalue will be a field of fragment uniform data.
    fragColor/*f*/ =/*f*/ hsh::float4{hsh::float3{
      hsh::float3{tex0.sample({0.f, 0.f}, TestSampler)} *
      hsh::dot(finalNormal/*v*/, -/*h*/lightDir/*h*/)/*v promoted to f via distribution test,
                                                        trigger vertex left fetch,
                                                        trigger host right fetch*/}/*f*/, 1.f}/*f*/;
  };

  // The hsh_binding object has handles of host-processed data buffers ready to draw with.
  MyBinding.bind(hsh::vertex_buffer{0, MyBuffer});
  MyBinding.draw(0, 4);
}
}
