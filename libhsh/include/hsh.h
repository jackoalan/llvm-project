#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <iostream>
#include <cassert>
#include <map>
#include <unordered_map>
#include <sstream>

#define HSH_ENABLE_LOG 1
#define HSH_ENABLE_VULKAN 1

#if HSH_ENABLE_VULKAN
inline void hshVkAssert(const char* pred) {
  std::cerr << pred << " failed\n";
  std::abort();
}
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_NO_EXCEPTIONS
#define VK_USE_PLATFORM_XCB_KHR
#define VULKAN_HPP_ASSERT(pred) if (!(pred)) hshVkAssert(#pred)
#include <vulkan/vulkan.hpp>

namespace hsh::detail {
inline struct {
  vk::Instance Instance;
  vk::Device Device;
  vk::PipelineLayout PipelineLayout;
  vk::RenderPass RenderPass;
  float Anisotropy = 0.f;
} VulkanGlobals;
}

namespace vk {
template <> struct ObjectDestroy<Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> {
public:
  ObjectDestroy() = default;
  ObjectDestroy(
      Instance owner, Optional<const AllocationCallbacks> allocationCallbacks,
      VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT {
    assert(owner == ::hsh::detail::VulkanGlobals.Instance);
  }
protected:
  template <typename T> void destroy(T t) VULKAN_HPP_NOEXCEPT {
    ::hsh::detail::VulkanGlobals.Instance.destroy(
        t, {}, VULKAN_HPP_DEFAULT_DISPATCHER);
  }
};
template <> struct ObjectDestroy<Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> {
public:
  ObjectDestroy() = default;
  ObjectDestroy(
      Device owner, Optional<const AllocationCallbacks> allocationCallbacks,
      VULKAN_HPP_DEFAULT_DISPATCHER_TYPE const &dispatch) VULKAN_HPP_NOEXCEPT {
    assert(owner == ::hsh::detail::VulkanGlobals.Device);
  }
protected:
  template <typename T> void destroy(T t) VULKAN_HPP_NOEXCEPT {
    ::hsh::detail::VulkanGlobals.Device.destroy(t, {},
                                                VULKAN_HPP_DEFAULT_DISPATCHER);
  }
};
} // namespace vk
#endif

#if 0
class LogPrinter {
public:
  std::ostream &Out = std::cerr;
#if HSH_ENABLE_LOG
  template <typename T>
  LogPrinter &operator<<(const T &Obj) {
    Out << Obj;
    return *this;
  }
#else
  template <typename T> LogPrinter &operator<<(const T &Obj) {
    return *this;
  }
#endif
};

LogPrinter &logger() {
  static LogPrinter LP;
  return LP;
}
#endif

namespace hsh {
struct base_buffer {};

template <typename T>
struct uniform_buffer;
template <typename T>
struct dynamic_uniform_buffer;
template <typename T>
struct vertex_buffer;
template <typename T>
struct dynamic_vertex_buffer;

struct uniform_buffer_typeless : base_buffer {
#ifndef NDEBUG
  const char *UniqueId;
  uniform_buffer_typeless(const char *UniqueId) : UniqueId(UniqueId) {}
#endif
  template <typename T>
  uniform_buffer<T> cast() const;
  template <typename T>
  operator uniform_buffer<T>() const { return cast<T>(); }
};
struct dynamic_uniform_buffer_typeless : base_buffer {
#ifndef NDEBUG
  const char *UniqueId;
  dynamic_uniform_buffer_typeless(const char *UniqueId) : UniqueId(UniqueId) {}
#endif
  template <typename T>
  dynamic_uniform_buffer<T> cast() const;
  template <typename T>
  operator dynamic_uniform_buffer<T>() const { return cast<T>(); }
};
struct vertex_buffer_typeless : base_buffer {
#ifndef NDEBUG
  const char *UniqueId;
  vertex_buffer_typeless(const char *UniqueId) : UniqueId(UniqueId) {}
#endif
  template <typename T>
  vertex_buffer<T> cast() const;
  template <typename T>
  operator vertex_buffer<T>() const { return cast<T>(); }
};
struct dynamic_vertex_buffer_typeless : base_buffer {
#ifndef NDEBUG
  const char *UniqueId;
  dynamic_vertex_buffer_typeless(const char *UniqueId) : UniqueId(UniqueId) {}
#endif
  template <typename T>
  dynamic_vertex_buffer<T> cast() const;
  template <typename T>
  operator dynamic_vertex_buffer<T>() const { return cast<T>(); }
};

template <typename T>
struct uniform_buffer : uniform_buffer_typeless {
#ifndef NDEBUG
  static constexpr char StaticUniqueId{};
  uniform_buffer() : uniform_buffer_typeless(&StaticUniqueId) {}
#endif
  const T *operator->() const { assert(false && "Not to be called from host!"); return nullptr; }
  const T &operator*() const { assert(false && "Not to be called from host!"); return *reinterpret_cast<T*>(0); }
};
template <typename T>
struct dynamic_uniform_buffer : dynamic_uniform_buffer_typeless {
#ifndef NDEBUG
  static constexpr char StaticUniqueId{};
  dynamic_uniform_buffer() : dynamic_uniform_buffer_typeless(&StaticUniqueId) {}
#endif
  const T *operator->() const { assert(false && "Not to be called from host!"); return nullptr; }
  const T &operator*() const { assert(false && "Not to be called from host!"); return *reinterpret_cast<T*>(0); }
};
template <typename T>
struct vertex_buffer : vertex_buffer_typeless {
#ifndef NDEBUG
  static constexpr char StaticUniqueId{};
  vertex_buffer() : vertex_buffer_typeless(&StaticUniqueId) {}
#endif
  const T *operator->() const { assert(false && "Not to be called from host!"); return nullptr; }
  const T &operator*() const { assert(false && "Not to be called from host!"); return *reinterpret_cast<T*>(0); }
};
template <typename T>
struct dynamic_vertex_buffer : dynamic_vertex_buffer_typeless {
#ifndef NDEBUG
  static constexpr char StaticUniqueId{};
  dynamic_vertex_buffer() : dynamic_vertex_buffer_typeless(&StaticUniqueId) {}
#endif
  const T *operator->() const { assert(false && "Not to be called from host!"); return nullptr; }
  const T &operator*() const { assert(false && "Not to be called from host!"); return *reinterpret_cast<T*>(0); }
};

template <typename T>
uniform_buffer<T> uniform_buffer_typeless::cast() const {
  assert(UniqueId == &uniform_buffer<T>::StaticUniqueId && "bad cast");
  return static_cast<uniform_buffer<T>>(*this);
}
template <typename T>
dynamic_uniform_buffer<T> dynamic_uniform_buffer_typeless::cast() const {
  assert(UniqueId == &dynamic_uniform_buffer<T>::StaticUniqueId && "bad cast");
  return static_cast<dynamic_uniform_buffer<T>>(*this);
}
template <typename T>
vertex_buffer<T> vertex_buffer_typeless::cast() const {
  assert(UniqueId == &vertex_buffer<T>::StaticUniqueId && "bad cast");
  return static_cast<vertex_buffer<T>>(*this);
}
template <typename T>
dynamic_vertex_buffer<T> dynamic_vertex_buffer_typeless::cast() const {
  assert(UniqueId == &dynamic_vertex_buffer<T>::StaticUniqueId && "bad cast");
  return static_cast<dynamic_vertex_buffer<T>>(*this);
}

struct float3;
struct float2;
struct float4 {
  float x, y, z, w;
  float4() = default;
  constexpr float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
  constexpr explicit float4(float f) : x(f), y(f), z(f), w(f) {}
  constexpr explicit float4(const float3 &other, float w = 1.f);
  constexpr explicit float4(const float2 &other, float z = 0.f, float w = 1.f);
  void operator+=(const float4 &other) { x += other.x; y += other.y; z += other.z; w += other.w; }
  void operator*=(const float4 &other) { x *= other.x; y *= other.y; z *= other.z; w *= other.w; }
  float4 operator/(float other) { return float4{x / other, y / other, z / other, w / other}; }
  float &operator[](std::size_t idx) { return (&x)[0]; }
  const float &operator[](std::size_t idx) const { return (&x)[0]; }
  float3 xyz() const;
  float2 xy() const;
  float2 xz() const;
  float2 xw() const;
};
struct float3 {
  float x, y, z;
  float3() = default;
  constexpr float3(float x, float y, float z) : x(x), y(y), z(z) {}
  constexpr explicit float3(float f) : x(f), y(f), z(f) {}
  float3 operator-() const { return float3{-x, -y, -z}; };
  float3 operator*(float other) const {
    return float3{x * other, y * other, z * other};
  }
  float3 operator/(float other) const {
    return float3{x / other, y / other, z / other};
  }
  float3 operator*(const float3 &other) const {
    return float3{x * other.x, y * other.y, z * other.z};
  }
  float3 &operator*=(const float3 &other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
  }
  float3 &operator*=(float other) {
    x *= other;
    y *= other;
    z *= other;
    return *this;
  }
  float3 operator+(const float3 &other) const {
    return float3{x + other.x, y + other.y, z + other.z};
  }
  float3 &operator+=(const float3 &other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }
  float &operator[](std::size_t idx) { return (&x)[0]; }
  const float &operator[](std::size_t idx) const { return (&x)[0]; }
};
float3 float4::xyz() const { return float3{x, y, z}; }
struct float2 {
  float x, y;
  float2() = default;
  constexpr float2(float x, float y) : x(x), y(y) {}
  constexpr explicit float2(float f) : x(f), y(f) {}
  float2 operator*(const float2 &other) const {
    return float2{x * other.x, y * other.y};
  }
  float2 operator/(const float2 &other) const {
    return float2{x / other.x, y / other.y};
  }
  float2 operator/(float other) const {
    return float2{x / other, y / other};
  }
  float2 operator-(const float2 &other) const {
    return float2{x - other.x, y - other.y};
  }
  float2 operator+(const float2 &other) const {
    return float2{x + other.x, y + other.y};
  }
  float2 operator-() const { return float2{-x, -y}; };
};
float2 float4::xy() const { return float2{x, y}; }
float2 float4::xz() const { return float2{x, z}; }
float2 float4::xw() const { return float2{x, w}; }
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
  float4 &operator[](std::size_t col) { return cols[col]; }
  const float4 &operator[](std::size_t col) const { return cols[col]; }
  float4x4 operator*(const float4x4 &other) const { return float4x4{}; };
  float4 operator*(const float4 &other) const { return float4{}; };
};
struct float3x3 {
  float3x3() = default;
  float3 cols[3];
  float3x3(const float4x4 &other)
      : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3 &operator[](std::size_t col) { return cols[col]; }
  const float3 &operator[](std::size_t col) const { return cols[col]; }
  float3x3 operator*(const float3x3 &other) const { return float3x3{}; };
  float3 operator*(const float3 &other) const { return float3{}; };
};
struct aligned_float3x3 {
  aligned_float3x3() = default;
  struct col {
    col() = default;
    col(const float3 &c) : c(c) {}
    float3 c; float p;
  } cols[3];
  aligned_float3x3(const float3x3 &other)
      : cols{other.cols[0], other.cols[1], other.cols[2]} {}
  aligned_float3x3(const float4x4 &other)
      : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3 &operator[](std::size_t col) { return cols[col].c; }
  const float3 &operator[](std::size_t col) const { return cols[col].c; }
  float3x3 operator*(const float3x3 &other) const { return float3x3{}; };
  float3 operator*(const float3 &other) const { return float3{}; };
};

enum Filter : std::uint8_t {
  Nearest,
  Linear
};

enum SamplerAddressMode : std::uint8_t {
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder,
  MirrorClampToEdge
};

enum BorderColor : std::uint8_t {
  TransparentBlack,
  OpaqueBlack,
  OpaqueWhite,
};

enum Compare : std::uint8_t {
  Never,
  Less,
  Equal,
  LEqual,
  Greater,
  NEqual,
  GEqual,
  Always
};

/* Holds constant sampler information */
struct sampler {
  enum Filter MagFilter = Linear;
  enum Filter MinFilter = Linear;
  enum Filter MipmapMode = Linear;
  enum SamplerAddressMode AddressModeU = Repeat;
  enum SamplerAddressMode AddressModeV = Repeat;
  enum SamplerAddressMode AddressModeW = Repeat;
  float MipLodBias = 0.f;
  enum Compare CompareOp = Never;
  enum BorderColor BorderColor = TransparentBlack;
  constexpr sampler(
      enum Filter MagFilter = Linear,
      enum Filter MinFilter = Linear,
      enum Filter MipmapMode = Linear,
      enum SamplerAddressMode AddressModeU = Repeat,
      enum SamplerAddressMode AddressModeV = Repeat,
      enum SamplerAddressMode AddressModeW = Repeat,
      float MipLodBias = 0.f,
      enum Compare CompareOp = Never,
      enum BorderColor BorderColor = TransparentBlack
  ) : MagFilter(MagFilter),
      MinFilter(MinFilter),
      MipmapMode(MipmapMode),
      AddressModeU(AddressModeU),
      AddressModeV(AddressModeV),
      AddressModeW(AddressModeW),
      MipLodBias(MipLodBias),
      CompareOp(CompareOp),
      BorderColor(BorderColor) {}
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
float dot(const float2 &a, const float2 &b) {
  return a.x * b.x + a.y * b.y;
}
float dot(const float3 &a, const float3 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
float sqrt(float v) {
  return std::sqrt(v);
}
float length(const float2 &a) {
  return sqrt(dot(a, a));
}
float length(const float3 &a) {
  return sqrt(dot(a, a));
}
float2 normalize(const float2 &a) {
  return a / length(a);
}
float3 normalize(const float3 &a) {
  return a / length(a);
}
float max(float a, float b) {
  return std::max(a, b);
}
float min(float a, float b) {
  return std::min(a, b);
}
float clamp(float v, float min, float max) {
  if (v > max)
    return max;
  else if (v < min)
    return min;
  else
    return v;
}
float3 clamp(const float3 &v, const float3 &min, const float3 &max) {
  float3 ret;
  for (int i = 0; i < 3; ++i)
    ret[i] = clamp(v[i], min[i], max[i]);
  return ret;
}
float saturate(float v) {
  return clamp(v, 0.f, 1.f);
}
float3 saturate(const float3 &v) {
  return clamp(v, hsh::float3(0.f), hsh::float3(1.f));
}
float exp2(float v) {
  return std::exp2(v);
}
float lerp(float a, float b, float t) {
  return b * t + a * (1.f - t);
}
float3 lerp(const float3 &a, const float3 &b, float t) {
  float3 ret;
  for (int i = 0; i < 3; ++i)
    ret[i] = b[i] * t + a[i] * (1.f - t);
  return ret;
}
float4 lerp(const float4 &a, const float4 &b, const float4 &t) {
  float4 ret;
  for (int i = 0; i < 4; ++i)
    ret[i] = b[i] * t[i] + a[i] * (1.f - t[i]);
  return ret;
}
float4 lerp(const float4 &a, const float4 &b, float t) {
  float4 ret;
  for (int i = 0; i < 4; ++i)
    ret[i] = b[i] * t + a[i] * (1.f - t);
  return ret;
}
float abs(float v) {
  return std::abs(v);
}
void discard() {}

enum Target : std::uint8_t {
  GLSL,
  HLSL,
  DXBC,
  DXIL,
  VULKAN_SPIRV,
  METAL,
  METAL_BIN_MAC,
  METAL_BIN_IOS,
  METAL_BIN_TVOS,
  TARGET_MAX
};
namespace detail {
constexpr enum Target ActiveTarget = VULKAN_SPIRV;
}

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
  Red = 1,
  Green = 2,
  Blue = 4,
  Alpha = 8
};

namespace pipeline {
template <bool CA = false, bool InShader = false> struct base_attribute {
  static constexpr bool is_ca = CA;
};
template <BlendFactor SrcColorBlendFactor = One,
    BlendFactor DstColorBlendFactor = Zero,
    BlendOp ColorBlendOp = Add,
    BlendFactor SrcAlphaBlendFactor = SrcColorBlendFactor,
    BlendFactor DstAlphaBlendFactor = DstColorBlendFactor,
    BlendOp AlphaBlendOp = ColorBlendOp,
    ColorComponentFlags ColorWriteComponents =
    ColorComponentFlags(Red | Green | Blue | Alpha)>
struct color_attachment : base_attribute<true> {};
template <Topology T = Triangles>
struct topology : base_attribute<> {};
template <unsigned P = 0>
struct patch_control_points : base_attribute<> {};
template <CullMode CM = CullNone>
struct cull_mode : base_attribute<> {};
template <Compare C = Always>
struct depth_compare : base_attribute<> {};
template <bool W = true>
struct depth_write : base_attribute<> {};
template <bool E = false>
struct early_depth_stencil : base_attribute<false, true> {};
template <typename... Attrs> struct pipeline {
  hsh::float4 position;
  static constexpr std::size_t color_attachment_count = ((Attrs::is_ca ? 1 : 0) + ...);
  std::array<hsh::float4, color_attachment_count> color_out;
};
}

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

/* Max supported mip count (enough for 16K texture) */
constexpr std::size_t MaxMipCount = 14;

/* Holds sampler object as loaded into graphics API */
template <hsh::Target T> struct SamplerObject {};

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

/* Holds constant color attachment information */
struct ColorAttachment {
  enum BlendFactor SrcColorBlendFactor = One;
  enum BlendFactor DstColorBlendFactor = Zero;
  enum BlendOp ColorBlendOp = Add;
  enum BlendFactor SrcAlphaBlendFactor = One;
  enum BlendFactor DstAlphaBlendFactor = Zero;
  enum BlendOp AlphaBlendOp = Add;
  enum ColorComponentFlags ColorWriteComponents = ColorComponentFlags(Red | Green | Blue | Alpha);
  constexpr bool blendEnabled() const {
    return SrcColorBlendFactor == One && DstColorBlendFactor == Zero &&
           ColorBlendOp == Add && SrcAlphaBlendFactor == One &&
        DstAlphaBlendFactor == Zero && AlphaBlendOp == Add;
  }
  constexpr ColorAttachment() = default;
  constexpr ColorAttachment(
      enum BlendFactor SrcColorBlendFactor,
      enum BlendFactor DstColorBlendFactor,
      enum BlendOp ColorBlendOp,
      enum BlendFactor SrcAlphaBlendFactor,
      enum BlendFactor DstAlphaBlendFactor,
      enum BlendOp AlphaBlendOp,
      std::underlying_type_t<ColorComponentFlags> ColorWriteComponents
      ) : SrcColorBlendFactor(SrcColorBlendFactor),
          DstColorBlendFactor(DstColorBlendFactor),
          ColorBlendOp(ColorBlendOp),
          SrcAlphaBlendFactor(SrcAlphaBlendFactor),
          DstAlphaBlendFactor(DstAlphaBlendFactor),
          AlphaBlendOp(AlphaBlendOp),
          ColorWriteComponents(ColorComponentFlags(ColorWriteComponents)) {}
};

/* Holds constant pipeline information */
struct PipelineInfo {
  enum Topology Topology = Triangles;
  unsigned PatchControlPoints = 0;
  enum CullMode CullMode = CullNone;
  enum Compare DepthCompare = Always;
  bool DepthWrite = true;
  constexpr PipelineInfo() = default;
  constexpr PipelineInfo(enum Topology Topology,
                         unsigned PatchControlPoints,
                         enum CullMode CullMode,
                         enum Compare DepthCompare,
                         bool DepthWrite) noexcept
      : Topology(Topology), PatchControlPoints(PatchControlPoints),
        CullMode(CullMode), DepthCompare(DepthCompare), DepthWrite(DepthWrite) {}
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
                            struct PipelineInfo PipelineInfo)
      : StageCodes(S), Bindings(B), Attributes(A), Samplers(Samps), Attachments(Atts), PipelineInfo(PipelineInfo) {}
};

template <Target T, std::uint32_t NStages, std::uint32_t NSamplers> struct ShaderData {
  using ObjectRef = std::reference_wrapper<ShaderObject<T>>;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = std::reference_wrapper<SamplerObject<T>>;
  std::array<SamplerRef, NSamplers> SamplerObjects;
  constexpr ShaderData(
      std::array<ObjectRef, NStages> S,
      std::array<SamplerRef, NSamplers> Samps)
      : ShaderObjects(S), SamplerObjects(Samps) {}
};

template <hsh::Target T>
struct PipelineBuilder {
  template <typename... Bindings, std::size_t... BSeq>
  static void build_pipelines(std::index_sequence<BSeq...>) {
    assert(false && "unimplemented pipeline builder");
  }
};

struct OpaqueBindingData {
  std::uint64_t Data1 = 0;
  std::uint64_t Data2 = 0;
  bool is_valid() const { return Data1 != 0; }
  template <typename T>
  T get() { assert(false && "unimplemented get"); return {}; }
  template <typename T>
  void set(T d) { assert(false && "unimplemented set"); }
};

#if HSH_ENABLE_VULKAN
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

constexpr vk::PrimitiveTopology HshToVkTopology(enum Topology Topology) {
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

constexpr vk::CullModeFlagBits HshToVkCullMode(enum CullMode CullMode) {
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

constexpr vk::CompareOp HshToVkCompare(enum Compare Compare) {
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

constexpr vk::BlendFactor HshToVkBlendFactor(enum BlendFactor BlendFactor) {
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
  case Src1Alpha:
    return vk::BlendFactor::eSrc1Alpha;
  case InvSrc1Alpha:
    return vk::BlendFactor::eOneMinusSrc1Alpha;
  }
}

constexpr vk::BlendOp HshToVkBlendOp(enum BlendOp BlendOp) {
  switch (BlendOp) {
  case Add:
    return vk::BlendOp::eAdd;
  case Subtract:
    return vk::BlendOp::eSubtract;
  case ReverseSubtract:
    return vk::BlendOp::eReverseSubtract;
  }
}

constexpr vk::Filter HshToVkFilter(enum Filter Filter) {
  switch (Filter) {
  case Nearest:
    return vk::Filter::eNearest;
  case Linear:
    return vk::Filter::eLinear;
  }
}

constexpr vk::SamplerMipmapMode HshToVkMipMode(enum Filter Filter) {
  switch (Filter) {
  case Nearest:
    return vk::SamplerMipmapMode::eNearest;
  case Linear:
    return vk::SamplerMipmapMode::eLinear;
  }
}

constexpr vk::SamplerAddressMode HshToVkAddressMode(enum SamplerAddressMode AddressMode) {
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

constexpr vk::BorderColor HshToVkBorderColor(enum BorderColor BorderColor, bool Int) {
  switch (BorderColor) {
  case TransparentBlack:
    return Int ? vk::BorderColor::eIntTransparentBlack : vk::BorderColor::eFloatTransparentBlack;
  case OpaqueBlack:
    return Int ? vk::BorderColor::eIntOpaqueBlack : vk::BorderColor::eFloatOpaqueBlack;
  case OpaqueWhite:
    return Int ? vk::BorderColor::eIntOpaqueWhite : vk::BorderColor::eFloatOpaqueWhite;
  }
}

constexpr vk::ShaderStageFlagBits HshToVkShaderStage(enum Stage Stage) {
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

constexpr vk::ColorComponentFlagBits HshToVkColorComponentFlags(enum ColorComponentFlags Comps) {
  return vk::ColorComponentFlagBits(
      (Comps & Red ? unsigned(vk::ColorComponentFlagBits::eR) : 0u) |
      (Comps & Green ? unsigned(vk::ColorComponentFlagBits::eG) : 0u) |
      (Comps & Blue ? unsigned(vk::ColorComponentFlagBits::eB) : 0u) |
      (Comps & Alpha ? unsigned(vk::ColorComponentFlagBits::eA) : 0u));
}

template <> struct ShaderCode<VULKAN_SPIRV> {
  enum Stage Stage = Stage::Vertex;
  ShaderDataBlob<uint32_t> Blob;
  constexpr ShaderCode() noexcept = default;
  constexpr ShaderCode(enum Stage Stage, ShaderDataBlob<uint32_t> Blob) noexcept
      : Stage(Stage), Blob(Blob) {}
};

template <> struct ShaderObject<VULKAN_SPIRV> {
  vk::UniqueShaderModule ShaderModule;
  ShaderObject() noexcept = default;
  vk::ShaderModule get(const vk::ShaderModuleCreateInfo &Info) {
    if (!ShaderModule)
      ShaderModule = VulkanGlobals.Device.createShaderModuleUnique(Info).value;
    return ShaderModule.get();
  }
};

template <> struct SamplerObject<VULKAN_SPIRV> {
  std::array<std::array<vk::UniqueSampler, MaxMipCount - 1>, 2> Samplers;
  SamplerObject() noexcept = default;
  vk::Sampler get(const vk::SamplerCreateInfo &Info, bool Int, unsigned MipCount) {
    assert(MipCount && MipCount < MaxMipCount);
    vk::UniqueSampler &Samp = Samplers[Int][MipCount - 1];
    if (!Samp) {
      vk::SamplerCreateInfo ModInfo(Info);
      ModInfo.setMaxLod(float(MipCount - 1))
          .setAnisotropyEnable(VulkanGlobals.Anisotropy != 0.f)
          .setMaxAnisotropy(VulkanGlobals.Anisotropy);
      if (Int)
        ModInfo.setBorderColor(vk::BorderColor(int(Info.borderColor) + 1));
      Samp = VulkanGlobals.Device.createSamplerUnique(ModInfo).value;
    }
    return Samp.get();
  }
};

constexpr std::array<vk::DynamicState, 2> Dynamics{vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};
constexpr vk::PipelineDynamicStateCreateInfo DynamicState{
    {}, 2, Dynamics.data()};

template <std::uint32_t NStages, std::uint32_t NBindings,
          std::uint32_t NAttributes, std::uint32_t NSamplers, std::uint32_t NAttachments>
struct ShaderConstData<VULKAN_SPIRV, NStages, NBindings, NAttributes, NSamplers, NAttachments> {
  std::array<vk::ShaderModuleCreateInfo, NStages> StageCodes;
  std::array<vk::ShaderStageFlagBits, NStages> StageFlags;
  std::array<vk::VertexInputBindingDescription, NBindings>
      VertexBindingDescriptions;
  std::array<vk::VertexInputAttributeDescription, NAttributes>
      VertexAttributeDescriptions;
  std::array<vk::PipelineColorBlendAttachmentState, NAttachments> TargetAttachments;
  vk::PipelineVertexInputStateCreateInfo VertexInputState;
  vk::PipelineInputAssemblyStateCreateInfo InputAssemblyState;
  vk::PipelineTessellationStateCreateInfo TessellationState;
  vk::PipelineRasterizationStateCreateInfo RasterizationState;
  vk::PipelineDepthStencilStateCreateInfo DepthStencilState;
  vk::PipelineColorBlendStateCreateInfo ColorBlendState;
  std::array<vk::SamplerCreateInfo, NSamplers> Samplers;

  template <std::size_t... SSeq, std::size_t... BSeq, std::size_t... ASeq, std::size_t... SampSeq, std::size_t... AttSeq>
  constexpr ShaderConstData(std::array<ShaderCode<VULKAN_SPIRV>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A,
                            std::array<sampler, NSamplers> Samps,
                            std::array<ColorAttachment, NAttachments> Atts,
                            struct PipelineInfo PipelineInfo,
                            std::index_sequence<SSeq...>,
                            std::index_sequence<BSeq...>,
                            std::index_sequence<ASeq...>,
                            std::index_sequence<SampSeq...>,
                            std::index_sequence<AttSeq...>)
      : StageCodes{vk::ShaderModuleCreateInfo{
            {}, std::get<SSeq>(S).Blob.Size, std::get<SSeq>(S).Blob.Data}...},
        StageFlags{HshToVkShaderStage(std::get<SSeq>(S).Stage)...},
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
        InputAssemblyState{{}, HshToVkTopology(PipelineInfo.Topology), VK_TRUE},
        TessellationState{{}, PipelineInfo.PatchControlPoints},
        RasterizationState{{}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, HshToVkCullMode(PipelineInfo.CullMode)},
        DepthStencilState{{}, PipelineInfo.DepthCompare != Always, PipelineInfo.DepthWrite, HshToVkCompare(PipelineInfo.DepthCompare)},
        ColorBlendState{{}, VK_FALSE, vk::LogicOp ::eClear, NAttachments, TargetAttachments.data()},
        Samplers{vk::SamplerCreateInfo{
            {},
            HshToVkFilter(std::get<SampSeq>(Samps).MagFilter),
            HshToVkFilter(std::get<SampSeq>(Samps).MinFilter),
            HshToVkMipMode(std::get<SampSeq>(Samps).MipmapMode),
            HshToVkAddressMode(std::get<SampSeq>(Samps).AddressModeU),
            HshToVkAddressMode(std::get<SampSeq>(Samps).AddressModeV),
            HshToVkAddressMode(std::get<SampSeq>(Samps).AddressModeW),
            std::get<SampSeq>(Samps).MipLodBias,
            0, 0,
            std::get<SampSeq>(Samps).CompareOp != Never,
            HshToVkCompare(std::get<SampSeq>(Samps).CompareOp),
            0, 0,
            HshToVkBorderColor(std::get<SampSeq>(Samps).BorderColor, false)}...} {}

  constexpr ShaderConstData(std::array<ShaderCode<VULKAN_SPIRV>, NStages> S,
                            std::array<VertexBinding, NBindings> B,
                            std::array<VertexAttribute, NAttributes> A,
                            std::array<sampler, NSamplers> Samps,
                            std::array<ColorAttachment, NAttachments> Atts,
                            struct PipelineInfo PipelineInfo)
      : ShaderConstData(S, B, A, Samps, Atts, PipelineInfo,
                        std::make_index_sequence<NStages>(),
                        std::make_index_sequence<NBindings>(),
                        std::make_index_sequence<NAttributes>(),
                        std::make_index_sequence<NSamplers>(),
                        std::make_index_sequence<NAttachments>()) {}

  template <typename B>
  vk::GraphicsPipelineCreateInfo
  getPipelineInfo(VkPipelineShaderStageCreateInfo *StageInfos) const {
    for (std::size_t i = 0; i < NStages; ++i)
      StageInfos[i] = vk::PipelineShaderStageCreateInfo{
          {},
          StageFlags[i],
          B::template data<hsh::VULKAN_SPIRV>.ShaderObjects[i].get().get(
              StageCodes[i])};

    return vk::GraphicsPipelineCreateInfo{
        {},
        NStages,
        reinterpret_cast<vk::PipelineShaderStageCreateInfo *>(StageInfos),
        &VertexInputState,
        &InputAssemblyState,
        &TessellationState,
        nullptr,
        &RasterizationState,
        nullptr,
        &DepthStencilState,
        &ColorBlendState,
        &DynamicState,
        VulkanGlobals.PipelineLayout,
        VulkanGlobals.RenderPass};
  }
};

template <std::uint32_t NStages, std::uint32_t NSamplers>
struct ShaderData<VULKAN_SPIRV, NStages, NSamplers> {
  using ObjectRef = std::reference_wrapper<ShaderObject<VULKAN_SPIRV>>;
  std::array<ObjectRef, NStages> ShaderObjects;
  using SamplerRef = std::reference_wrapper<SamplerObject<VULKAN_SPIRV>>;
  std::array<SamplerRef, NSamplers> SamplerObjects;
  vk::UniquePipeline Pipeline;
  constexpr ShaderData(std::array<ObjectRef, NStages> S,
                       std::array<SamplerRef, NSamplers> Samps)
      : ShaderObjects(S), SamplerObjects(Samps) {}
};

template <> struct PipelineBuilder<hsh::VULKAN_SPIRV> {
  template <typename B>
  static constexpr std::size_t get_num_stages(bool NotZero) {
    return NotZero ? B::template cdata<hsh::VULKAN_SPIRV>.StageCodes.size() : 0;
  }
  template <typename... B, std::size_t... BSeq>
  static constexpr std::size_t stage_info_start(std::size_t BIdx,
                                                std::index_sequence<BSeq...>) {
    return (get_num_stages<B>(BSeq < BIdx) + ...);
  }
  template <typename B> static void set_pipeline(vk::Pipeline data) {
    vk::ObjectDestroy<vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter(
        VulkanGlobals.Device, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    B::template data<hsh::VULKAN_SPIRV>.Pipeline =
        vk::UniquePipeline(data, deleter);
  }
  template <typename... B, std::size_t... BSeq>
  static void build_pipelines(std::index_sequence<BSeq...> seq) {
    std::array<VkPipelineShaderStageCreateInfo,
               (get_num_stages<B>(true) + ...)>
        ShaderStageInfos;
    std::array<vk::GraphicsPipelineCreateInfo, sizeof...(B)> Infos{
        B::template cdata<hsh::VULKAN_SPIRV>.template getPipelineInfo<B>(
          ShaderStageInfos.data() + stage_info_start<B...>(BSeq, seq))...};
    std::array<vk::Pipeline, sizeof...(B)> Pipelines;
    VULKAN_HPP_ASSERT(VulkanGlobals.Device.createGraphicsPipelines(
                          {}, Infos.size(), Infos.data(), nullptr,
                          Pipelines.data()) == vk::Result::eSuccess);
    (set_pipeline<B>(Pipelines[BSeq]), ...);
  }
};

template <>
vk::Pipeline OpaqueBindingData::get<vk::Pipeline>() {
  return vk::Pipeline(reinterpret_cast<VkPipeline>(Data1));
}
template <>
void OpaqueBindingData::set<vk::Pipeline>(vk::Pipeline d) {
  Data1 = reinterpret_cast<std::uint64_t>(VkPipeline(d));
}

template <>
vk::DescriptorSet OpaqueBindingData::get<vk::DescriptorSet>() {
  return vk::DescriptorSet(reinterpret_cast<VkDescriptorSet>(Data2));
}
template <>
void OpaqueBindingData::set<vk::DescriptorSet>(vk::DescriptorSet d) {
  Data2 = reinterpret_cast<std::uint64_t>(VkDescriptorSet(d));
}
#endif

struct GlobalListNode;
GlobalListNode *GlobalListHead = nullptr;
struct GlobalListNode {
  typedef void (*RegisterFunc)();
  std::array<RegisterFunc, TARGET_MAX> Func;
  GlobalListNode *Next;
  template <typename... Args>
  explicit GlobalListNode(Args... Funcs) noexcept
      : Func{Funcs...}, Next(GlobalListHead) {
    GlobalListHead = this;
  }
};

template <typename... B>
struct PipelineCoordinator {
  static hsh::detail::GlobalListNode global;
  template <hsh::Target T>
  static void global_build() {
    PipelineBuilder<T>::template build_pipelines<B...>(
        std::make_index_sequence<sizeof...(B)>());
  }
};

/*
 * This macro is internally expanded within the hsh generator
 * for any identifiers prefixed with hsh_ being assigned or returned.
 */
#define _hsh_dummy(...) ::hsh::binding_typeless{};
} // namespace detail

#define HSH_PROFILE_MODE 0

class binding_typeless {
protected:
  detail::OpaqueBindingData Data;
public:
  binding_typeless() = default;
  bool is_valid() const { return Data.is_valid(); }
};

template <typename Impl>
class binding : public binding_typeless {
protected:
  template <typename... Args>
  explicit binding(Args... args) {
    switch (detail::ActiveTarget) {
#if HSH_ENABLE_VULKAN
    case VULKAN_SPIRV:
      Data.set(Impl::template data<VULKAN_SPIRV>.Pipeline.get());
      break;
#endif
    default:
      break;
    }
  }
};

#if HSH_PROFILE_MODE
struct value_formatter {
  template <typename T>
  static std::ostream &format(std::ostream &out, T val) {
    return out << val;
  }
};

class profiler {
  friend class profile_context;
public:
  struct push {
    const char *name;
    explicit push(const char *name) : name(name) {}
  };
  struct pop {};
private:
  template <typename T>
  using EnableIfNonControlArg =
  std::enable_if_t<!std::is_same_v<T, push> && !std::is_same_v<T, pop> &&
                   !std::is_same_v<T, const char *>,
      int>;
  struct node {
    std::map<std::string, node> children;
    std::string leaf;
    node &get() {
      return *this;
    }
    template <typename... Args>
    node &get(push, Args... rest) {
      return get(rest...);
    }
    template <typename... Args>
    node &get(pop, Args... rest) {
      return get(rest...);
    }
    template <typename... Args>
    node &get(const char *arg, Args... rest) {
      return get(rest...);
    }
    template <typename T, typename... Args, EnableIfNonControlArg<T> = 0>
    node &get(T arg, Args... rest) {
      std::ostringstream ss;
      hsh::value_formatter::format(ss, arg);
      return children[ss.str()].get(rest...);
    }
    static std::ostream &indent(std::ostream &out, unsigned arg) {
      for (unsigned i = 0; i < arg; ++i)
        out << "  ";
      return out;
    }
    void write(std::ostream &out, unsigned arg) const {
      if (!children.empty()) {
        indent(out, arg) << "switch (arg" << arg << ") {\n";
        for (auto [key, node] : children) {
          indent(out, arg) << "case " << key << ":\n";
          node.write(out, arg + 1);
        }
        indent(out, arg) << "}\n";
      } else {
        indent(out, arg) << "return " << leaf << "(Resources...);\n";
      }
    }
  } root;
  static void do_format_param(std::ostream &out, push p) {
    out << p.name << "<";
  }
  static void do_format_param(std::ostream &out, pop p) {
    out << ">";
  }
  static void do_format_param(std::ostream &out, const char *arg) {
    out << arg;
  }
  template <typename T, EnableIfNonControlArg<T> = 0>
  static void do_format_param(std::ostream &out, T arg) {
    hsh::value_formatter::format(out, arg);
  }
  static void format_param_next(std::ostream &out) {}
  template <typename T>
  static void format_param_next(std::ostream &out, T arg) {
    out << ", ";
    do_format_param(out, arg);
  }
  template <typename T, typename... Args>
  static void format_params(std::ostream &out, T arg, Args... rest) {
    do_format_param(out, arg);
    (format_param_next(out, rest), ...);
  }
  void write_header(std::ostream &out) const {
    root.write(out, 0);
  }
public:
  template <typename... Args>
  void add(const char *binding, Args... args) {
    node &n = root.get(args...);
    std::ostringstream ss;
    ss << binding << "<";
    format_params(ss, args...);
    ss << ">";
    n.leaf = ss.str();
  }
};

class profile_context {
  std::unordered_map<std::string, std::unordered_map<std::string, profiler>> profilers;
public:
  static profile_context instance;
  profiler &get(const char *filename, const char *macro) {
    return profilers[filename][macro];
  }
};

profile_context profile_context::instance{};
#endif
} // namespace hsh
