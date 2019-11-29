add_clang_tool(hosh
  hosh.cpp
  )

clang_target_link_libraries(hosh
  PRIVATE
  libhosh
  )
