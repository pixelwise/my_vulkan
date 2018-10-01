function(compile_shader in_file out_var)
  get_filename_component(file_name ${in_file} NAME)
  set(shaders_dir "shaders")
  set(SPIRV "${shaders_dir}/${file_name}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${shaders_dir}"
    COMMAND glslc ${in_file} -o ${SPIRV}
    DEPENDS ${in_file})
    SET(${out_var} ${SPIRV} PARENT_SCOPE)
endfunction(compile_shader)
