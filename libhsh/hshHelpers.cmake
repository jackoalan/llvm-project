function(_hsh_canonicalize_cmake_booleans)
  foreach(var ${ARGN})
    if(${var})
      set(${var} 1 PARENT_SCOPE)
    else()
      set(${var} 0 PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

macro(_hsh_register_macro_variables)
  foreach(var ${ARGN})
    list(APPEND HSH_EXTRA_COMPILE_DEFINES ${var}=${${var}})
    message(STATUS "[hsh]: ${var}=${${var}}")
  endforeach()
endmacro()

unset(HSH_TARGETS_FOUND)
unset(HSH_EXTRA_INCLUDE_DIRS)
unset(HSH_EXTRA_COMPILE_DEFINES)

#
# Configuration macro variables
#

set(HSH_PROFILE_MODE Off CACHE BOOL "Build hsh-enabled targets for shader profile-guided specialization")
set(HSH_MAX_UNIFORMS 8 CACHE STRING "Maximum number of uniforms allowed across all hsh-enabled targets")
set(HSH_MAX_IMAGES 8 CACHE STRING "Maximum number of images allowed across all hsh-enabled targets")
set(HSH_MAX_SAMPLERS 8 CACHE STRING "Maximum number of samplers allowed across all hsh-enabled targets")
set(HSH_MAX_VERTEX_BUFFERS 8 CACHE STRING "Maximum number of vertex buffers allowed across all hsh-enabled targets")
set(HSH_MAX_INDEX_BUFFERS 8 CACHE STRING "Maximum number of index buffers allowed across all hsh-enabled targets")
set(HSH_MAX_RENDER_TEXTURE_BINDINGS 4 CACHE STRING "Maximum number of render texture bindings allowed across all hsh-enabled targets")
set(HSH_DESCRIPTOR_POOL_SIZE 8192 CACHE STRING "Maximum descriptor pool size across all hsh-enabled targets")

_hsh_canonicalize_cmake_booleans(HSH_PROFILE_MODE)
_hsh_register_macro_variables(
        HSH_PROFILE_MODE
        HSH_MAX_UNIFORMS
        HSH_MAX_IMAGES
        HSH_MAX_SAMPLERS
        HSH_MAX_VERTEX_BUFFERS
        HSH_MAX_INDEX_BUFFERS
        HSH_MAX_RENDER_TEXTURE_BINDINGS
        HSH_DESCRIPTOR_POOL_SIZE)

#
# Automatic backend enabling
#

if (WIN32)
  get_filename_component(VULKAN_SDK_DIRS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\LunarG\\VulkanSDK;VK_SDK_PATHs]" ABSOLUTE CACHE)
  if (NOT ${VULKAN_SDK_DIRS} STREQUAL "/registry")
    message(STATUS "[hsh]: Found vulkan headers")
    list(APPEND HSH_TARGETS_FOUND vulkan-spirv)
    list(GET VULKAN_SDK_DIRS 0 VULKAN_SDK_DIR)
    list(APPEND HSH_EXTRA_INCLUDE_DIRS "${VULKAN_SDK_DIR}/Include")
  endif()
else()
  find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.hpp)
  if (VULKAN_INCLUDE_DIR)
    message(STATUS "[hsh]: Found vulkan headers")
    list(APPEND HSH_TARGETS_FOUND vulkan-spirv)
  endif()
endif()

set(HSH_TARGETS ${HSH_TARGETS_FOUND} CACHE STRING "Enabled backend targets for all hsh-enabled targets")

if (vulkan-spirv IN_LIST HSH_TARGETS)
  list(APPEND HSH_EXTRA_COMPILE_DEFINES HSH_ENABLE_VULKAN=1)
  message(STATUS "[hsh]: Enabling vulkan hsh target")
endif()

#
# Assemble includes and defines into interface library
#

add_library(hsh INTERFACE)
target_include_directories(hsh INTERFACE "${HSH_INCLUDE_DIR}" ${HSH_EXTRA_INCLUDE_DIRS})
target_compile_definitions(hsh INTERFACE ${HSH_EXTRA_COMPILE_DEFINES})

#
# gather_include_directories recursively builds a list of include directories
# across all dependencies.
#

function(_hsh_gather_include_directories_impl target_name)
  get_target_property(target_dependencies ${target_name} INTERFACE_LINK_LIBRARIES)
  foreach(dep ${target_dependencies})
    if(TARGET ${dep})
      get_target_property(dep_includes ${dep} INTERFACE_INCLUDE_DIRECTORIES)
      if(dep_includes)
        list(APPEND target_includes ${dep_includes})
      endif()
      _hsh_gather_include_directories_impl(${dep})
    endif()
  endforeach()
  set(target_includes ${target_includes} PARENT_SCOPE)
endfunction()

function(hsh_gather_include_directories var target_name)
  unset(target_includes)
  get_directory_property(dir_includes INCLUDE_DIRECTORIES)
  if(dir_includes)
    list(APPEND target_includes ${dir_includes})
  endif()
  get_target_property(target_includes1 ${target_name} INCLUDE_DIRECTORIES)
  if(target_includes1)
    list(APPEND target_includes ${target_includes1})
  endif()
  get_target_property(target_includes2 ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
  if(target_includes2)
    list(APPEND target_includes ${target_includes2})
  endif()
  _hsh_gather_include_directories_impl(${target_name})
  list(REMOVE_DUPLICATES target_includes)
  set(${var} ${target_includes} PARENT_SCOPE)
endfunction()

function(_hsh_gather_compile_definitions_impl target_name)
  get_target_property(target_dependencies ${target_name} INTERFACE_LINK_LIBRARIES)
  foreach(dep ${target_dependencies})
    if(TARGET ${dep})
      get_target_property(dep_defines ${dep} INTERFACE_COMPILE_DEFINITIONS)
      if(dep_defines)
        list(APPEND target_defines ${dep_defines})
      endif()
      _hsh_gather_compile_definitions_impl(${dep})
    endif()
  endforeach()
  set(target_defines ${target_defines} PARENT_SCOPE)
endfunction()

function(hsh_gather_compile_definitions var target_name)
  unset(target_defines)
  get_directory_property(dir_defines COMPILE_DEFINITIONS)
  if(dir_defines)
    list(APPEND target_defines ${dir_defines})
  endif()
  get_target_property(target_defines1 ${target_name} COMPILE_DEFINITIONS)
  if(target_defines1)
    list(APPEND target_defines ${target_defines1})
  endif()
  get_target_property(target_defines2 ${target_name} INTERFACE_COMPILE_DEFINITIONS)
  if(target_defines2)
    list(APPEND target_defines ${target_defines2})
  endif()
  _hsh_gather_compile_definitions_impl(${target_name})
  list(REMOVE_DUPLICATES target_defines)
  set(${var} ${target_defines} PARENT_SCOPE)
endfunction()

#
# target_hsh makes all sources of a target individually depend on
# its own generated hshhead file.
#
# ALL_SOURCES enables hshgen for every non-header source
# HSH_TARGETS overrides the list of built hsh targets (not recommended)
#
function(target_hsh target)
  cmake_parse_arguments(ARG
          "ALL_SOURCES"
          ""
          "HSH_TARGETS"
          ${ARGN})

  # Assemble target arguments for hshgen
  unset(_hsh_args)
  foreach(target ${HSH_TARGETS})
    list(APPEND _hsh_args "--${target}")
  endforeach()
  foreach(target ${ARG_HSH_TARGETS})
    list(APPEND _hsh_args "--${target}")
  endforeach()
  list(REMOVE_DUPLICATES _hsh_targets)

  # Set includes and defines for CMake target tree
  target_link_libraries(${target} PUBLIC hsh)
  hsh_gather_include_directories(include_list ${target})
  foreach(include ${include_list})
    list(APPEND _hsh_args "-I${include}")
  endforeach()
  hsh_gather_compile_definitions(define_list ${target})
  foreach(define ${define_list})
    list(APPEND _hsh_args "$<$<BOOL:${define}>:-D${define}>")
  endforeach()

  # Process each source of target
  get_target_property(bin_dir ${target} BINARY_DIR)
  get_target_property(src_dir ${target} SOURCE_DIR)
  get_target_property(source_list ${target} SOURCES)
  foreach(source ${source_list})
    # Headers shall not pass
    get_source_file_property(is_header "${source}" HEADER_FILE_ONLY)
    if(is_header)
      continue()
    endif()

    # Non-enabled sources shall not pass
    get_source_file_property(hsh_enabled "${source}" HSH_ENABLED)
    if(NOT ARG_ALL_SOURCES AND NOT hsh_enabled)
      continue()
    endif()

    # Decompose path for determining out-of-tree intermediate location
    get_source_file_property(src_path "${source}" LOCATION)
    file(RELATIVE_PATH rel_path "${src_dir}" "${src_path}")
    get_filename_component(rel_dir "${rel_path}" DIRECTORY)
    if(rel_dir)
      set(rel_dir "${rel_dir}/")
    endif()
    string(REPLACE ".." "__" rel_dir "${rel_dir}")
    set(out_dir "HshFiles/${rel_dir}")
    set(out_path "${bin_dir}/${out_dir}${rel_path}")
    file(MAKE_DIRECTORY "${bin_dir}/${out_dir}")
    file(RELATIVE_PATH out_rel ${CMAKE_BINARY_DIR} "${out_path}")

    # Ninja can make use of a dependency file generated by hshgen
    unset(depfile_args)
    if("${CMAKE_GENERATOR}" STREQUAL "Ninja")
      list(APPEND depfile_args DEPFILE "${CMAKE_BINARY_DIR}/${out_rel}.hshhead.d")
    endif()

    # If out-of-tree hshprof does not exist, this will create an empty one to make generators like ninja happy
    add_custom_command(OUTPUT "${out_path}.hshprof" COMMAND "${CMAKE_COMMAND}"
            ARGS -E touch "${out_path}.hshprof")

    # If in-tree hshprof exists, use that instead of generating an out-of-tree one
    if (EXISTS "${src_path}.hshprof")
      set(use_prof "${src_path}.hshprof")
    else()
      set(use_prof "${out_path}.hshprof")
    endif()

    # Hshgen rule here
    add_custom_command(OUTPUT "${out_path}.hshhead" COMMAND "$<TARGET_FILE:hshgen>"
            ARGS ${_hsh_args} "${src_path}" "${out_rel}.hshhead"
            -MD -MT "${out_rel}.hshhead" -MF "${out_rel}.hshhead.d" "-hsh-profile=${use_prof}"
            DEPENDS hshgen "${src_path}" "${use_prof}" IMPLICIT_DEPENDS CXX "${src_path}"
            ${depfile_args} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

    # Make compiled object artifact depend on hshhead (only works for makefiles and ninja)
    set_source_files_properties("${source}" PROPERTIES
            INCLUDE_DIRECTORIES "${bin_dir}/${out_dir}"
            OBJECT_DEPENDS "${out_path}.hshhead")

    # Add hshhead to target
    target_sources(${target} PUBLIC "${out_path}.hshhead")
  endforeach()
endfunction()

#
# Per-source enabling of hshgen
#
function(hsh_sources)
  set_source_files_properties(${ARGN} PROPERTIES HSH_ENABLED On)
endfunction()
