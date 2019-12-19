include(ExternalProject)

if(CMAKE_HOST_UNIX)
  set(LINK_OR_COPY create_symlink
          libdxcompiler.so.3.7
          "${CMAKE_BINARY_DIR}/bin/libdxcompiler.so")
else()
  set(LINK_OR_COPY copy
          "${CMAKE_BINARY_DIR}/bin/libdxcompiler.so.3.7"
          "${CMAKE_BINARY_DIR}/bin/libdxcompiler.so")
endif()

ExternalProject_Add(
  DirectXShaderCompiler
  GIT_REPOSITORY  https://github.com/microsoft/DirectXShaderCompiler
  GIT_TAG         2749458081b6211fde8fc6de5294f1460798b49b
  GIT_SHALLOW     On
  BUILD_COMMAND   "${CMAKE_COMMAND}" --build . --target dxcompiler
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
    "${CMAKE_COMMAND}" -E copy lib/libdxcompiler.so.3.7 "${CMAKE_BINARY_DIR}/bin"
  COMMAND
    "${CMAKE_COMMAND}" -E ${LINK_OR_COPY}
  CMAKE_CACHE_ARGS
    -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
    -DLLVM_TARGETS_TO_BUILD:STRING=""
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

add_clang_library(clangHsh
  HshGenerator.cpp

  LINK_LIBS
  clangTooling
  )

# Enable -fms-extensions for clang to use MS uuid extensions for COM.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set_source_files_properties(HshGenerator.cpp PROPERTIES COMPILE_FLAGS
    "-fms-extensions -Wno-language-extension-token")
endif()

add_dependencies(clangHsh DirectXShaderCompiler)

install(FILES
        "${CMAKE_BINARY_DIR}/bin/libdxcompiler.so"
        "${CMAKE_BINARY_DIR}/bin/libdxcompiler.so.3.7"
        DESTINATION bin)
