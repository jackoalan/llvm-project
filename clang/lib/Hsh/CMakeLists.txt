include(ExternalProject)

set(lib_destination "${LLVM_LIBRARY_OUTPUT_INTDIR}/clang-hsh/${CLANG_VERSION}")
set(lib_install_destination "lib${LLVM_LIBDIR_SUFFIX}/clang-hsh/${CLANG_VERSION}")

if(CMAKE_HOST_UNIX)
  set(LINK_OR_COPY create_symlink
          libdxcompiler${CMAKE_SHARED_LIBRARY_SUFFIX}.3.7
          "${lib_destination}/libdxcompiler${CMAKE_SHARED_LIBRARY_SUFFIX}")
else()
  set(LINK_OR_COPY copy
          "${lib_destination}/libdxcompiler${CMAKE_SHARED_LIBRARY_SUFFIX}.3.7"
          "${lib_destination}/libdxcompiler${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

ExternalProject_Add(
        DirectXShaderCompiler
        GIT_REPOSITORY https://github.com/microsoft/DirectXShaderCompiler
        GIT_TAG e8422a4f4efec2dba14203630d7b24de906f6d42
        BUILD_COMMAND "${CMAKE_COMMAND}" --build . --target dxcompiler
        INSTALL_COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${CLANG_BINARY_DIR}/include/dxc/"
        COMMAND
        "${CMAKE_COMMAND}" -E make_directory "${CLANG_BINARY_DIR}/include/dxc/Support"
        COMMAND
        "${CMAKE_COMMAND}" -E copy ../DirectXShaderCompiler/include/dxc/dxcapi.h
        "${CLANG_BINARY_DIR}/include/dxc/dxcapi.h"
        COMMAND
        "${CMAKE_COMMAND}" -E copy ../DirectXShaderCompiler/include/dxc/Support/WinAdapter.h
        "${CLANG_BINARY_DIR}/include/dxc/Support/WinAdapter.h"
        COMMAND
        "${CMAKE_COMMAND}" -E copy lib/libdxcompiler${CMAKE_SHARED_LIBRARY_SUFFIX}.3.7 "${lib_destination}"
        COMMAND
        "${CMAKE_COMMAND}" -E ${LINK_OR_COPY}
        CMAKE_CACHE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
        -DLLVM_TARGETS_TO_BUILD:STRING=None
        -DDXC_BUILD_ARCH:STRING=x64
        -DCLANG_ENABLE_ARCMT:STRING=Off
        -DCLANG_ENABLE_STATIC_ANALYZER:STRING=Off
        -DCLANG_INCLUDE_TESTS:STRING=Off
        -DLLVM_INCLUDE_TESTS:STRING=Off
        -DHLSL_INCLUDE_TESTS:STRING=On
        -DLLVM_INCLUDE_DOCS:STRING=Off
        -DLLVM_INCLUDE_EXAMPLES:STRING=Off
        -DLIBCLANG_BUILD_STATIC:STRING=On
        -DLLVM_OPTIMIZED_TABLEGEN:STRING=Off
        -DLLVM_REQUIRES_EH:STRING=On
        -DLLVM_APPEND_VC_REV:STRING=On
        -DLLVM_ENABLE_RTTI:STRING=On
        -DLLVM_ENABLE_EH:STRING=On
        -DLLVM_DEFAULT_TARGET_TRIPLE:STRING=dxil-ms-dx
        -DCLANG_BUILD_EXAMPLES:STRING=Off
        -DLLVM_REQUIRES_RTTI:STRING=On
        -DCLANG_CL:STRING=Off)

# Use DXC's main header as a dependency barrier to defer
# building clangHsh until after DXC is built.
set(dxc_header "${CLANG_BINARY_DIR}/include/dxc/dxcapi.h")
add_custom_command(OUTPUT "${dxc_header}"
                   COMMAND "${CMAKE_COMMAND}" ARGS
                   --build . --target DirectXShaderCompiler
                   WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

add_clang_library(clangHsh
        "${CLANG_BINARY_DIR}/include/dxc/dxcapi.h"
        HshGenerator.cpp
        LINK_LIBS
        clangTooling)

# Enable -fms-extensions for clang to use MS uuid extensions for COM.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set_source_files_properties(HshGenerator.cpp PROPERTIES
          COMPILE_FLAGS "-fms-extensions -Wno-language-extension-token")
endif()

add_custom_target(DirectXShaderCompiler-libs ALL DEPENDS "${dxc_header}")
set_target_properties(DirectXShaderCompiler-libs PROPERTIES
        FOLDER "Misc"
        RUNTIME_OUTPUT_DIRECTORY "${lib_install_destination}")

install(FILES
        "${lib_destination}/libdxcompiler${CMAKE_SHARED_LIBRARY_SUFFIX}"
        "${lib_destination}/libdxcompiler${CMAKE_SHARED_LIBRARY_SUFFIX}.3.7"
        COMPONENT DirectXShaderCompiler-libs
        DESTINATION "${lib_install_destination}")

if(NOT LLVM_ENABLE_IDE)
  add_llvm_install_targets(install-DirectXShaderCompiler-libs
          DEPENDS "${dxc_header}"
          COMPONENT DirectXShaderCompiler-libs)
endif()