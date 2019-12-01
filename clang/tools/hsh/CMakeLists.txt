add_clang_tool(hsh
  hsh.cpp
  )

clang_target_link_libraries(hsh
  PRIVATE
  clangHsh
  )

add_subdirectory(test)
