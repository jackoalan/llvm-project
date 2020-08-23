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
unset(HSH_EXTRA_LIBS)
unset(HSH_EXTRA_COMPILE_OPTIONS)

#
# Configuration macro variables
#

set(HSH_PROFILE_MODE Off CACHE BOOL "Build hsh-enabled targets for shader profile-guided specialization")
set(HSH_MAX_UNIFORMS 8 CACHE STRING "Maximum number of uniforms allowed across all hsh-enabled targets")
set(HSH_MAX_IMAGES 8 CACHE STRING "Maximum number of images allowed across all hsh-enabled targets")
set(HSH_MAX_SAMPLERS 8 CACHE STRING "Maximum number of samplers allowed across all hsh-enabled targets")
set(HSH_MAX_VERTEX_BUFFERS 8 CACHE STRING "Maximum number of vertex buffers allowed across all hsh-enabled targets")
set(HSH_MAX_RENDER_TEXTURE_BINDINGS 4 CACHE STRING "Maximum number of render texture bindings allowed across all hsh-enabled targets")
set(HSH_DESCRIPTOR_POOL_SIZE 8192 CACHE STRING "Maximum descriptor pool size across all hsh-enabled targets")

_hsh_canonicalize_cmake_booleans(HSH_PROFILE_MODE)
_hsh_register_macro_variables(
        HSH_PROFILE_MODE
        HSH_MAX_UNIFORMS
        HSH_MAX_IMAGES
        HSH_MAX_SAMPLERS
        HSH_MAX_VERTEX_BUFFERS
        HSH_MAX_RENDER_TEXTURE_BINDINGS
        HSH_DESCRIPTOR_POOL_SIZE)

#
# Automatic backend enabling
#

if(WIN32)
  get_filename_component(VULKAN_SDK_DIRS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\LunarG\\VulkanSDK;VK_SDK_PATHs]" ABSOLUTE CACHE)
  if(NOT "${VULKAN_SDK_DIRS}" STREQUAL "/registry")
    message(STATUS "[hsh]: Found vulkan headers")
    list(APPEND HSH_TARGETS_FOUND vulkan-spirv)
    list(GET VULKAN_SDK_DIRS 0 VULKAN_SDK_DIR)
    list(APPEND HSH_EXTRA_INCLUDE_DIRS "${VULKAN_SDK_DIR}/Include")
    list(APPEND HSH_EXTRA_COMPILE_DEFINES VK_USE_PLATFORM_WIN32_KHR=1)
  endif()
elseif(APPLE)
  if(NOT CMAKE_OSX_SYSROOT)
    message(WARNING "[hsh]: CMAKE_OSX_SYSROOT not set; assuming macosx")
    list(APPEND HSH_TARGETS_FOUND metal-bin-mac)
  else()
    # Read PLATFORM_NAME from SDKSettings.plist to determine the type of apple platform
    execute_process(COMMAND /usr/libexec/PlistBuddy -c
            "print :DefaultProperties:PLATFORM_NAME"
            "${CMAKE_OSX_SYSROOT}/SDKSettings.plist"
            OUTPUT_VARIABLE PLATFORM_NAME
            RESULT_VARIABLE HAD_ERROR
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (NOT HAD_ERROR)
      if(PLATFORM_NAME STREQUAL "macosx")
        message(STATUS "[hsh]: macosx SDK in use")
        list(APPEND HSH_TARGETS_FOUND metal-bin-mac)
      elseif(PLATFORM_NAME STREQUAL "iphoneos")
        message(STATUS "[hsh]: iphoneos SDK in use")
        list(APPEND HSH_TARGETS_FOUND metal-bin-ios)
      elseif(PLATFORM_NAME STREQUAL "appletvos")
        message(STATUS "[hsh]: appletvos SDK in use")
        list(APPEND HSH_TARGETS_FOUND metal-bin-ios)
      else()
        message(WARNING "[hsh]: Unknown SDK platform ${PLATFORM_NAME}; assuming macosx")
        list(APPEND HSH_TARGETS_FOUND metal-bin-mac)
      endif()
    else()
      message(WARNING "[hsh]: unable to read SDKSettings.plist; assuming macosx")
      list(APPEND HSH_TARGETS_FOUND metal-bin-mac)
    endif()
    find_library(QUARTZCORE_LIB QuartzCore)
    find_library(METAL_LIB Metal)
    list(APPEND HSH_EXTRA_LIBS ${QUARTZCORE_LIB} ${METAL_LIB})
    list(APPEND HSH_EXTRA_INCLUDE_DIRS ${QUARTZCORE_LIB} ${METAL_LIB})
  endif()
elseif(NX)
  find_path(LIBNX_INCLUDE_DIR NAMES deko3d.hpp)
  if (LIBNX_INCLUDE_DIR)
    message(STATUS "[hsh]: Found libnx headers")
    list(APPEND HSH_TARGETS_FOUND deko3d)
  endif()
else()
  find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.hpp)
  if (VULKAN_INCLUDE_DIR)
    message(STATUS "[hsh]: Found vulkan headers")
    list(APPEND HSH_TARGETS_FOUND vulkan-spirv)
    include(FindPkgConfig)
    pkg_check_modules(vk-xcb IMPORTED_TARGET xcb)
    if(vk-xcb_FOUND)
      list(APPEND HSH_EXTRA_COMPILE_DEFINES VK_USE_PLATFORM_XCB_KHR=1)
      list(APPEND HSH_EXTRA_LIBS PkgConfig::vk-xcb)
    endif()
    pkg_check_modules(vk-wayland IMPORTED_TARGET wayland-client)
    if(vk-wayland_FOUND)
      list(APPEND HSH_EXTRA_COMPILE_DEFINES VK_USE_PLATFORM_WAYLAND_KHR=1)
      list(APPEND HSH_EXTRA_LIBS PkgConfig::vk-wayland)
    endif()
  endif()
endif()

set(HSH_TARGETS ${HSH_TARGETS_FOUND} CACHE STRING "Enabled backend targets for all hsh-enabled targets")

if(vulkan-spirv IN_LIST HSH_TARGETS)
  list(APPEND HSH_EXTRA_COMPILE_DEFINES HSH_ENABLE_VULKAN=1)
  message(STATUS "[hsh]: Enabling vulkan hsh target")
endif()

if(deko3d IN_LIST HSH_TARGETS)
  list(APPEND HSH_EXTRA_COMPILE_DEFINES HSH_ENABLE_DEKO3D=1)
  message(STATUS "[hsh]: Enabling deko3d hsh target")
endif()

if(metal-bin-mac IN_LIST HSH_TARGETS)
  list(APPEND HSH_EXTRA_COMPILE_DEFINES HSH_ENABLE_METAL=1 HSH_ENABLE_METAL_BIN_MAC=1)
  list(APPEND HSH_EXTRA_COMPILE_OPTIONS -xobjective-c++ -fobjc-arc)
  message(STATUS "[hsh]: Enabling metal-bin-mac hsh target")
endif()

if(metal-bin-ios IN_LIST HSH_TARGETS)
  list(APPEND HSH_EXTRA_COMPILE_DEFINES HSH_ENABLE_METAL=1 HSH_ENABLE_METAL_BIN_IOS=1)
  list(APPEND HSH_EXTRA_COMPILE_OPTIONS -xobjective-c++ -fobjc-arc)
  message(STATUS "[hsh]: Enabling metal-bin-ios hsh target")
endif()

#
# Assemble includes and defines into interface library
#

add_library(hsh INTERFACE)
target_include_directories(hsh INTERFACE "${HSH_INCLUDE_DIR}" ${HSH_EXTRA_INCLUDE_DIRS})
target_compile_definitions(hsh INTERFACE ${HSH_EXTRA_COMPILE_DEFINES})
target_link_libraries(hsh INTERFACE ${HSH_EXTRA_LIBS})
target_compile_options(hsh INTERFACE ${HSH_EXTRA_COMPILE_OPTIONS})

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

  # Ensure hsh has access to the active apple sdk and C++ standard library
  if(APPLE AND CMAKE_OSX_SYSROOT)
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    list(APPEND _hsh_args -isysroot "${CMAKE_OSX_SYSROOT}"
         -stdlib++-isystem "${COMPILER_DIR}/../include/c++/v1")

    # Make a best-effort attempt to mark all target's sources as Objective C++.
    # This is mainly to benefit IDEs that depend on CMake's understanding of the source files to set up syntax parsing.
    get_target_property(target_sources ${target} SOURCES)
    set_source_files_properties(${target_sources} TARGET_DIRECTORY ${target} PROPERTIES LANGUAGE OBJCXX)
    if(NOT CMAKE_OBJCXX_COMPILER)
      message(FATAL_ERROR "OBJCXX language is not enabled. Please add it to the project command for APPLE builds.")
    endif()
  endif()

  # Set includes and defines for CMake target tree
  target_link_libraries(${target} PUBLIC hsh)
  set(inc_prop "$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>")
  set(def_prop "$<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>")

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
    get_filename_component(rel_name "${rel_path}" NAME)
    if(rel_dir)
      set(rel_dir "${rel_dir}/")
    endif()
    string(REPLACE ".." "__" rel_dir "${rel_dir}")
    set(out_dir "HshFiles/${rel_dir}")
    set(out_path "${bin_dir}/${out_dir}${rel_name}")
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
    if(EXISTS "${src_path}.hshprof")
      set(use_prof "${src_path}.hshprof")
    else()
      set(use_prof "${out_path}.hshprof")
    endif()

    # Hshgen rule here
    add_custom_command(OUTPUT "${out_path}.hshhead" COMMAND "$<TARGET_FILE:hshgen>"
            "$<$<CONFIG:Debug>:-g>"
            "$<$<BOOL:${inc_prop}>:-I$<JOIN:${inc_prop},;-I>>"
            "$<$<BOOL:${def_prop}>:-D$<JOIN:${def_prop},;-D>>"
            ARGS ${_hsh_args} "${src_path}" "${out_rel}.hshhead"
            -MD -MT "${out_rel}.hshhead" -MF "${out_rel}.hshhead.d" "-hsh-profile=${use_prof}"
            DEPENDS hshgen "${src_path}" "${use_prof}" IMPLICIT_DEPENDS CXX "${src_path}"
            ${depfile_args} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" COMMAND_EXPAND_LISTS)

    # Make compiled object artifact depend on hshhead (only works for makefiles and ninja)
    set_source_files_properties("${source}" PROPERTIES
            INCLUDE_DIRECTORIES "${bin_dir}/${out_dir}"
            OBJECT_DEPENDS "${out_path}.hshhead")
  endforeach()
endfunction()

#
# Per-source enabling of hshgen
#
function(hsh_sources)
  set_source_files_properties(${ARGN} PROPERTIES HSH_ENABLED On)
endfunction()

#
# Add fixed-up hsh executable based on normal executable target
#
# This involves striping out hsh shader sections and appending them to
# the emitted binary so they do not get persistently memory-mapped.
# libhsh then has all information to temporarily map the shader data
# while they are being built at runtime.
#
function(hsh_add_executable target_name basis_target)
  get_target_property(target_type ${basis_target} TYPE)
  if(NOT ${target_type} STREQUAL "EXECUTABLE")
    message(FATAL_ERROR "${basis_target} is not an executable target")
  endif()

  set(dest_binary "$<TARGET_FILE:${basis_target}>")
  if(CMAKE_HOST_UNIX)
    set(dest_binary "$<TARGET_FILE_NAME:${basis_target}>")
  endif()
  if(CMAKE_CONFIGURATION_TYPES)
    list(GET CMAKE_CONFIGURATION_TYPES 0 first_type)
    string(TOUPPER ${first_type} first_type_upper)
    set(first_type_suffix _${first_type_upper})
  endif()
  get_target_property(ARG_OUTPUT_DIR ${basis_target} RUNTIME_OUTPUT_DIRECTORY${first_type_suffix})
  if(CMAKE_CONFIGURATION_TYPES)
    string(FIND "${ARG_OUTPUT_DIR}" "/${first_type}/" type_start REVERSE)
    string(SUBSTRING "${ARG_OUTPUT_DIR}" 0 ${type_start} path_prefix)
    string(SUBSTRING "${ARG_OUTPUT_DIR}" ${type_start} -1 path_suffix)
    string(REPLACE "/${first_type}/" "/${CMAKE_CFG_INTDIR}/"
            path_suffix ${path_suffix})
    set(ARG_OUTPUT_DIR ${path_prefix}${path_suffix})
  endif()

  set(output_path "${ARG_OUTPUT_DIR}/${target_name}${CMAKE_EXECUTABLE_SUFFIX}")

  if (APPLE)
    # TODO: Come up with some kind of bundle handling (install basis removal?)
    # To debug the fixed-up executable on macOS, a symlink is created to a potential dSYM bundle
    add_custom_command(OUTPUT ${output_path}
            COMMAND "$<TARGET_FILE:llvm-objcopy>"
            "${dest_binary}" "${output_path}"
            COMMAND "${CMAKE_COMMAND}" -E create_symlink "${dest_binary}.dSYM" "${output_path}.dSYM"
            WORKING_DIRECTORY "${ARG_OUTPUT_DIR}"
            DEPENDS ${basis_target} llvm-objcopy)
  else()
    add_custom_command(OUTPUT ${output_path}
            COMMAND "$<TARGET_FILE:llvm-objcopy>"
            "${dest_binary}" "${output_path}"
            WORKING_DIRECTORY "${ARG_OUTPUT_DIR}"
            DEPENDS ${basis_target} llvm-objcopy)
  endif()

  add_custom_target(${target_name} ALL DEPENDS ${basis_target} ${output_path})
  set_target_properties(${target_name} PROPERTIES LOCATION "${output_path}")

  if (WIN32)
    # Emit PDB with fixups
    target_link_options(${basis_target} PRIVATE /INCREMENTAL:NO /DEBUG /DEBUGTYPE:CV,FIXUP)
  elseif (APPLE)
    # A default MachO file is sufficient for fixups
  else()
    # Assume ELF-based platform otherwise
    target_link_options(${basis_target} PRIVATE -Xlinker --emit-relocs)
  endif()
endfunction()
