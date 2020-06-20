function(get_stdfilesystem_lib out_var_name)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        set(
            _TMP_RESULT
            c++fs
        )
    endif ()
    if (CMAKE_COMPILER_IS_GNUCXX)
        set(
            _TMP_RESULT
            stdc++fs
        )
    endif()
    set(${out_var_name} "${_TMP_RESULT}" PARENT_SCOPE)
endfunction()
