#hsh_sources(test-input.cpp)
add_library(hsh-test-lib test-input.cpp urde-test.cpp)
target_hsh(hsh-test-lib ALL_SOURCES HSH_TARGETS hlsl)

add_executable(hsh-test main.cpp)
target_include_directories(hsh-test PRIVATE "${HSH_INCLUDE_DIR}")
target_link_libraries(hsh-test PRIVATE xcb ${CMAKE_DL_LIBS})
