function(target_hsh target)
  get_target_property(bin_dir ${target} BINARY_DIR)
  get_target_property(src_dir ${target} SOURCE_DIR)
  get_target_property(source_list ${target} SOURCES)
  foreach(source "${source_list}")
    get_source_file_property(src_path "${source}" LOCATION)
    file(RELATIVE_PATH rel_path "${src_dir}" "${src_path}")
    get_filename_component(rel_dir "${rel_path}" DIRECTORY)
    if (rel_dir)
      set(rel_dir "${rel_dir}/")
    endif()
    string(REPLACE ".." "__" rel_dir "${rel_dir}")
    get_filename_component(rel_file "${rel_path}" NAME_WE)
    set(out_dir "HshFiles/${rel_dir}")
    set(out_path "${bin_dir}/${out_dir}hsh_${rel_file}.cpp")
    file(MAKE_DIRECTORY "${bin_dir}/${out_dir}")
    add_custom_command(OUTPUT "${out_path}" COMMAND "$<TARGET_FILE:hsh>"
                       ARGS "hsh_${rel_file}.cpp" "${src_path}"
                       DEPENDS hsh "${src_path}"
                       WORKING_DIRECTORY "${bin_dir}/${out_dir}")
    set_source_files_properties("${source}" PROPERTIES
            INCLUDE_DIRECTORIES "${bin_dir}/${out_dir}"
            OBJECT_DEPENDS "${out_path}"
            COMPILE_FLAGS "-Wno-unknown-attributes -Wno-unused-value")
    target_sources(${target} PUBLIC "${out_path}")
  endforeach()
endfunction()

add_library(hsh-test-lib test-input.cpp)
target_hsh(hsh-test-lib)
