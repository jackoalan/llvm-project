#pragma once

#if HSH_ENABLE_DEKO3D

#include <deko3d.hpp>

#ifdef HSH_IMPLEMENTATION
#define DMA_IMPLEMENTATION
#endif
#include "deko_mem_alloc.h"

namespace dk {
template <typename T> struct index_type {
  static_assert(std::is_same_v<T, uint32_t>,
                "Index type not available in deko");
};
template <> struct index_type<uint32_t> {
  static constexpr DkIdxFormat indexType = DkIdxFormat_Uint32;
};
template <> struct index_type<uint16_t> {
  static constexpr DkIdxFormat indexType = DkIdxFormat_Uint16;
};
template <> struct index_type<uint8_t> {
  static constexpr DkIdxFormat indexType = DkIdxFormat_Uint8;
};

template <DkIdxFormat value> struct cpp_index_type {
  static_assert(value == DkIdxFormat_Uint32,
                "Index type not available in Vulkan spec");
};
template <> struct cpp_index_type<DkIdxFormat_Uint32> {
  using type = uint32_t;
};
template <> struct cpp_index_type<DkIdxFormat_Uint16> {
  using type = uint16_t;
};
template <> struct cpp_index_type<DkIdxFormat_Uint8> { using type = uint8_t; };
} // namespace dk

#endif
