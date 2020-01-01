#hsh_sources(test-input.cpp)
add_library(hsh-test-lib test-input.cpp)
target_hsh(hsh-test-lib ALL_SOURCES HSH_TARGETS hlsl)
