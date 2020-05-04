#pragma once

#if HSH_ENABLE_DEKO3D

#include <deko3d.hpp>

#ifdef HSH_IMPLEMENTATION
#define DMA_IMPLEMENTATION
#endif
#include "deko_mem_alloc.h"

namespace dk {
template <typename T> struct IndexTypeValue {};
template <typename EnumType, EnumType value> struct CppType {};

template <> struct IndexTypeValue<uint16_t> {
  static constexpr DkIdxFormat value = DkIdxFormat_Uint16;
};

template <> struct CppType<DkIdxFormat, DkIdxFormat_Uint16> {
  using Type = uint16_t;
};

template <> struct IndexTypeValue<uint32_t> {
  static constexpr DkIdxFormat value = DkIdxFormat_Uint32;
};

template <> struct CppType<DkIdxFormat, DkIdxFormat_Uint32> {
  using Type = uint32_t;
};

template <> struct IndexTypeValue<uint8_t> {
  static constexpr DkIdxFormat value = DkIdxFormat_Uint8;
};

template <> struct CppType<DkIdxFormat, DkIdxFormat_Uint8> {
  using Type = uint8_t;
};
} // namespace dk

extern "C" const uint8_t __deko_shader_begin;
extern "C" const uint8_t __deko_control;
extern "C" const uint32_t __deko_shader_memblock_size;

#endif
