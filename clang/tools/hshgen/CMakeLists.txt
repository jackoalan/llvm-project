add_clang_tool(hshgen
  hshgen.cpp
  )

target_compile_definitions(hshgen PRIVATE
  HSH_RT_OUTPUT_DIR=${LLVM_LIBRARY_OUTPUT_INTDIR}/clang/${CLANG_VERSION})

clang_target_link_libraries(hshgen
  PRIVATE
  clangHsh
  )

add_subdirectory(test)
