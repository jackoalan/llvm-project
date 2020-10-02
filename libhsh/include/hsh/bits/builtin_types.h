#pragma once

namespace hsh {

struct float3;
struct float2;
struct float4 {
  float x, y, z, w;
  float4() noexcept = default;
  constexpr float4(float x, float y, float z, float w) noexcept
      : x(x), y(y), z(z), w(w) {}
  constexpr explicit float4(float f) noexcept : x(f), y(f), z(f), w(f) {}
  constexpr explicit float4(const float3 &other, float w = 1.f) noexcept;
  constexpr explicit float4(const float2 &other, float z = 0.f,
                            float w = 1.f) noexcept;
  void operator+=(const float4 &other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
  }
  void operator*=(const float4 &other) noexcept {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
  }
  float4 operator*(float other) const noexcept {
    return float4{x * other, y * other, z * other, w * other};
  }
  float4 operator*(const float4 &other) const noexcept {
    return float4{x * other.x, y * other.y, z * other.z, w * other.w};
  }
  float4 operator/(float other) const noexcept {
    return float4{x / other, y / other, z / other, w / other};
  }
  float4 operator+(const float4 &other) const noexcept {
    return float4{x + other.x, y + other.y, z + other.z, x + other.w};
  }
  float4 operator-(const float4 &other) const noexcept {
    return float4{x - other.x, y - other.y, z - other.z, w - other.w};
  }
  float &operator[](std::size_t idx) noexcept { return (&x)[idx]; }
  constexpr const float &operator[](std::size_t idx) const noexcept {
    return (&x)[idx];
  }
  constexpr float3 xyz() const noexcept;
  constexpr float2 xy() const noexcept;
  constexpr float2 xz() const noexcept;
  constexpr float2 xw() const noexcept;
  constexpr float2 zw() const noexcept;
};
struct float3 {
  float x, y, z;
  float3() noexcept = default;
  constexpr float3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
  constexpr explicit float3(float f) noexcept : x(f), y(f), z(f) {}
  float3 operator-() const noexcept { return float3{-x, -y, -z}; };
  float3 operator*(float other) const noexcept {
    return float3{x * other, y * other, z * other};
  }
  float3 operator/(float other) const noexcept {
    return float3{x / other, y / other, z / other};
  }
  float3 operator*(const float3 &other) const noexcept {
    return float3{x * other.x, y * other.y, z * other.z};
  }
  float3 &operator*=(const float3 &other) noexcept {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
  }
  float3 &operator*=(float other) noexcept {
    x *= other;
    y *= other;
    z *= other;
    return *this;
  }
  float3 operator+(const float3 &other) const noexcept {
    return float3{x + other.x, y + other.y, z + other.z};
  }
  float3 operator-(const float3 &other) const noexcept {
    return float3{x - other.x, y - other.y, z - other.z};
  }
  float3 &operator+=(const float3 &other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }
  float &operator[](std::size_t idx) noexcept { return (&x)[idx]; }
  constexpr const float &operator[](std::size_t idx) const noexcept {
    return (&x)[idx];
  }
  constexpr float2 xy() const noexcept;
  constexpr float2 xz() const noexcept;
};
constexpr float3 float4::xyz() const noexcept { return float3{x, y, z}; }
struct float2 {
  float x, y;
  float2() noexcept = default;
  constexpr float2(float x, float y) noexcept : x(x), y(y) {}
  constexpr explicit float2(float f) noexcept : x(f), y(f) {}
  float2 operator*(const float2 &other) const noexcept {
    return float2{x * other.x, y * other.y};
  }
  float2 operator/(const float2 &other) const noexcept {
    return float2{x / other.x, y / other.y};
  }
  float2 operator/(float other) const noexcept {
    return float2{x / other, y / other};
  }
  float2 operator-(const float2 &other) const noexcept {
    return float2{x - other.x, y - other.y};
  }
  float2 operator+(const float2 &other) const noexcept {
    return float2{x + other.x, y + other.y};
  }
  float2 operator-() const noexcept { return float2{-x, -y}; };
  float &operator[](std::size_t idx) noexcept { return (&x)[idx]; }
  constexpr const float &operator[](std::size_t idx) const noexcept {
    return (&x)[idx];
  }
};
constexpr float2 float4::xy() const noexcept { return float2{x, y}; }
constexpr float2 float4::xz() const noexcept { return float2{x, z}; }
constexpr float2 float4::xw() const noexcept { return float2{x, w}; }
constexpr float2 float4::zw() const noexcept { return float2{z, w}; }
constexpr float2 float3::xy() const noexcept { return float2{x, y}; }
constexpr float2 float3::xz() const noexcept { return float2{x, z}; }
constexpr float4::float4(const hsh::float3 &other, float w) noexcept
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr float4::float4(const hsh::float2 &other, float z, float w) noexcept
    : x(other.x), y(other.y), z(z), w(w) {}
struct int3;
struct int2;
struct int4 {
  std::int32_t x, y, z, w;
  int4() noexcept = default;
  constexpr explicit int4(const int3 &other, std::int32_t w = 0) noexcept;
  constexpr explicit int4(const int2 &other, std::int32_t z = 0,
                          std::int32_t w = 0) noexcept;
  void operator+=(const int4 &other) noexcept {}
  void operator*=(const int4 &other) noexcept {}
};
struct int3 {
  std::int32_t x, y, z;
  int3() noexcept = default;
  constexpr int3(std::int32_t x, std::int32_t y, std::int32_t z) noexcept
      : x(x), y(y), z(z) {}
  constexpr explicit int3(std::int32_t f) noexcept : x(f), y(f), z(f) {}
  int3 operator-() const noexcept { return int3{-x, -y, -z}; };
  int3 operator*(std::int32_t other) noexcept {
    return int3{x * other, y * other, z * other};
  }
};
struct int2 {
  std::int32_t x, y;
  int2() noexcept = default;
  constexpr int2(std::int32_t x, std::int32_t y) noexcept : x(x), y(y) {}
  constexpr explicit int2(std::int32_t f) noexcept : x(f), y(f) {}
  int2 operator-() const noexcept { return int2{-x, -y}; };
};
constexpr int4::int4(const hsh::int3 &other, std::int32_t w) noexcept
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr int4::int4(const hsh::int2 &other, std::int32_t z,
                     std::int32_t w) noexcept
    : x(other.x), y(other.y), z(z), w(w) {}
struct uint3;
struct uint2;
struct uint4 {
  std::uint32_t x, y, z, w;
  uint4() noexcept = default;
  constexpr explicit uint4(const uint3 &other, std::uint32_t w = 0) noexcept;
  constexpr explicit uint4(const uint2 &other, std::uint32_t z = 0,
                           std::uint32_t w = 0) noexcept;
  void operator+=(const uint4 &other) noexcept {}
  void operator*=(const uint4 &other) noexcept {}
};
struct uint3 {
  std::uint32_t x, y, z;
  uint3() noexcept = default;
  constexpr uint3(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept
      : x(x), y(y), z(z) {}
  constexpr explicit uint3(std::uint32_t f) noexcept : x(f), y(f), z(f) {}
  uint3 operator*(std::uint32_t other) noexcept {
    return uint3{x * other, y * other, z * other};
  }
};
struct uint2 {
  std::uint32_t x, y;
  uint2() noexcept = default;
  constexpr uint2(std::uint32_t x, std::uint32_t y) noexcept : x(x), y(y) {}
  constexpr explicit uint2(std::uint32_t f) noexcept : x(f), y(f) {}
};
constexpr uint4::uint4(const hsh::uint3 &other, std::uint32_t w) noexcept
    : x(other.x), y(other.y), z(other.z), w(w) {}
constexpr uint4::uint4(const hsh::uint2 &other, std::uint32_t z,
                       std::uint32_t w) noexcept
    : x(other.x), y(other.y), z(z), w(w) {}
struct float4x4 {
  std::array<float4, 4> cols;
  float4x4() noexcept = default;
  float4x4(const float4 &c0, const float4 &c1, const float4 &c2,
           const float4 &c3) noexcept
      : cols{c0, c1, c2, c3} {}
  float4 &operator[](std::size_t col) noexcept { return cols[col]; }
  const float4 &operator[](std::size_t col) const noexcept { return cols[col]; }
  constexpr float4x4 operator*(const float4x4 &other) const noexcept {
    return float4x4{};
  };
  constexpr float4 operator*(const float4 &other) const noexcept {
    return float4{};
  };
};
struct float3x3 {
  std::array<float3, 3> cols;
  float3x3() noexcept = default;
  float3x3(const float3 &c0, const float3 &c1, const float3 &c2) noexcept
      : cols{c0, c1, c2} {}
  float3x3(const float4x4 &other) noexcept
      : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3 &operator[](std::size_t col) noexcept { return cols[col]; }
  const float3 &operator[](std::size_t col) const noexcept { return cols[col]; }
  constexpr float3x3 operator*(const float3x3 &other) const noexcept {
    return float3x3{};
  };
  constexpr float3 operator*(const float3 &other) const noexcept {
    return float3{};
  };
};
struct aligned_float3x3 {
  aligned_float3x3() noexcept = default;
  struct col {
    col() noexcept = default;
    col(const float3 &c) noexcept : c(c) {}
    float3 c;
    float p;
  };
  std::array<col, 3> cols;
  aligned_float3x3(const float3x3 &other) noexcept
      : cols{other.cols[0], other.cols[1], other.cols[2]} {}
  aligned_float3x3(const float4x4 &other) noexcept
      : cols{other.cols[0].xyz(), other.cols[1].xyz(), other.cols[2].xyz()} {}
  float3 &operator[](std::size_t col) noexcept { return cols[col].c; }
  const float3 &operator[](std::size_t col) const noexcept {
    return cols[col].c;
  }
  constexpr float3x3 operator*(const float3x3 &other) const noexcept {
    return float3x3{};
  };
  constexpr float3 operator*(const float3 &other) const noexcept {
    return float3{};
  };
};

enum Filter : std::uint8_t { Nearest, Linear };

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
  constexpr sampler(enum Filter MagFilter = Linear,
                    enum Filter MinFilter = Linear,
                    enum Filter MipmapMode = Linear,
                    enum SamplerAddressMode AddressModeU = Repeat,
                    enum SamplerAddressMode AddressModeV = Repeat,
                    enum SamplerAddressMode AddressModeW = Repeat,
                    float MipLodBias = 0.f, enum Compare CompareOp = Never,
                    enum BorderColor BorderColor = TransparentBlack) noexcept
      : MagFilter(MagFilter), MinFilter(MinFilter), MipmapMode(MipmapMode),
        AddressModeU(AddressModeU), AddressModeV(AddressModeV),
        AddressModeW(AddressModeW), MipLodBias(MipLodBias),
        CompareOp(CompareOp), BorderColor(BorderColor) {}
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

template <typename T, std::size_t N> struct aligned_array {
  struct alignas(16) {
    T elem;
  } data[N];

  aligned_array() = default;
  template <typename... Args>
  explicit aligned_array(Args &&... args) : data{{std::forward(args)}...} {}

  const T &operator[](std::size_t pos) const { return data[pos].elem; }
  T &operator[](std::size_t pos) { return data[pos].elem; }
};

template <typename T> struct aligned_array<T, 0> {
  struct alignas(16) {
  } data;

  // Please don't actually use these
  const T &operator[](std::size_t pos) const {
    return reinterpret_cast<const T *>(&data)[pos];
  }
  T &operator[](std::size_t pos) { return reinterpret_cast<T *>(&data)[pos]; }
};

constexpr float dot(const float2 &a, const float2 &b) noexcept {
  return a.x * b.x + a.y * b.y;
}
constexpr float dot(const float3 &a, const float3 &b) noexcept {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
constexpr float dot(const float4 &a, const float4 &b) noexcept {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
inline float sqrt(float v) noexcept { return std::sqrt(v); }
inline float length(const float2 &a) noexcept { return sqrt(dot(a, a)); }
inline float length(const float3 &a) noexcept { return sqrt(dot(a, a)); }
inline float2 normalize(const float2 &a) noexcept { return a / length(a); }
inline float3 normalize(const float3 &a) noexcept { return a / length(a); }
constexpr float max(float a, float b) noexcept { return std::max(a, b); }
constexpr float min(float a, float b) noexcept { return std::min(a, b); }
constexpr float clamp(float v, float min, float max) noexcept {
  if (v > max)
    return max;
  else if (v < min)
    return min;
  else
    return v;
}
constexpr float3 clamp(const float3 &v, const float3 &min,
                       const float3 &max) noexcept {
  return float3{clamp(v[0], min[0], max[0]), clamp(v[1], min[1], max[1]),
                clamp(v[2], min[2], max[2])};
}
constexpr float saturate(float v) noexcept { return clamp(v, 0.f, 1.f); }
constexpr float3 saturate(const float3 &v) noexcept {
  return clamp(v, hsh::float3(0.f), hsh::float3(1.f));
}
inline float exp2(float v) noexcept { return std::exp2(v); }
constexpr float lerp(float a, float b, float t) noexcept {
  return b * t + a * (1.f - t);
}
constexpr float2 lerp(const float2 &a, const float2 &b, float t) noexcept {
  return float2{
      b[0] * t + a[0] * (1.f - t),
      b[1] * t + a[1] * (1.f - t),
  };
}
constexpr float2 lerp(const float2 &a, const float2 &b,
                      const float2 &t) noexcept {
  return float2{
      b[0] * t[0] + a[0] * (1.f - t[0]),
      b[1] * t[1] + a[1] * (1.f - t[1]),
  };
}
constexpr float3 lerp(const float3 &a, const float3 &b, float t) noexcept {
  return float3{
      b[0] * t + a[0] * (1.f - t),
      b[1] * t + a[1] * (1.f - t),
      b[2] * t + a[2] * (1.f - t),
  };
}
constexpr float3 lerp(const float3 &a, const float3 &b,
                      const float3 &t) noexcept {
  return float3{
      b[0] * t[0] + a[0] * (1.f - t[0]),
      b[1] * t[1] + a[1] * (1.f - t[1]),
      b[2] * t[2] + a[2] * (1.f - t[2]),
  };
}
constexpr float4 lerp(const float4 &a, const float4 &b,
                      const float4 &t) noexcept {
  return float4{
      b[0] * t[0] + a[0] * (1.f - t[0]),
      b[1] * t[1] + a[1] * (1.f - t[1]),
      b[2] * t[2] + a[2] * (1.f - t[2]),
      b[3] * t[3] + a[3] * (1.f - t[3]),
  };
}
constexpr float4 lerp(const float4 &a, const float4 &b, float t) noexcept {
  return float4{
      b[0] * t + a[0] * (1.f - t),
      b[1] * t + a[1] * (1.f - t),
      b[2] * t + a[2] * (1.f - t),
      b[3] * t + a[3] * (1.f - t),
  };
}
inline float abs(float v) noexcept { return std::abs(v); }
constexpr void discard() noexcept {}

} // namespace hsh
