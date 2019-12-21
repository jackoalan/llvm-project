add_clang_tool(hshgen
  hshgen.cpp
  )

clang_target_link_libraries(hshgen
  PRIVATE
  clangHsh
  )
