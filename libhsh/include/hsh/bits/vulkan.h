#pragma once

#if HSH_ENABLE_VULKAN

#define VK_NO_PROTOTYPES
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

#ifdef HSH_IMPLEMENTATION
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#define VMA_IMPLEMENTATION
#endif

#define VMA_USE_STL_CONTAINERS 1
#define VMA_USE_STL_SHARED_MUTEX 1
#include "vk_mem_alloc_hsh.h"

namespace vk {
template <typename T> struct index_type {
  static_assert(std::is_same_v<T, uint32_t>,
                "Index type not available in Vulkan spec");
};
template <> struct index_type<uint32_t> {
  static VULKAN_HPP_CONST_OR_CONSTEXPR IndexType indexType = IndexType::eUint32;
};
template <> struct index_type<uint16_t> {
  static VULKAN_HPP_CONST_OR_CONSTEXPR IndexType indexType = IndexType::eUint16;
};
template <> struct index_type<uint8_t> {
  static VULKAN_HPP_CONST_OR_CONSTEXPR IndexType indexType =
      IndexType::eUint8EXT;
};

template <IndexType value> struct cpp_index_type {
  static_assert(value == IndexType::eUint32,
                "Index type not available in Vulkan spec");
};
template <> struct cpp_index_type<IndexType::eUint32> {
  using type = uint32_t;
};
template <> struct cpp_index_type<IndexType::eUint16> {
  using type = uint16_t;
};
template <> struct cpp_index_type<IndexType::eUint8EXT> {
  using type = uint8_t;
};
} // namespace vk

#endif
