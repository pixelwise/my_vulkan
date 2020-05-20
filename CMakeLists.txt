cmake_policy(SET CMP0025 NEW) # needed for conan.cmake to work right
project(my_vulkan)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(shaders)
if(CMAKE_PROJECT_NAME STREQUAL "my_vulkan")
    set(
        _is_top_project
        TRUE
    )
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (_is_top_project)
    message(STATUS "my_vulkan is top project.")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -DDEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Werror=return-type")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Werror=return-type")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
    set(
        conan_compiler_libcxx
        "compiler.libcxx=libc++"
    )

    # --- conan ---
    include(conan)
    find_program(CONAN conan)
    if(NOT CONAN)
        message(FATAL_ERROR "conan not found!")
    endif()
    set(
        CONAN_PACKAGES
        ${CONAN_PACKAGES}
        boost/1.71.0@conan/stable
        glm/0.9.8.5@g-truc/stable
        # these are overrides due to different package sources
        libpng/1.6.37@bincrafters/stable
        zlib/1.2.11@
        bzip2/1.0.8@conan/stable
    )
    set(
        opencv_conan_options
        opencv:freetype=True
        opencv:png=True
        opencv:jpeg=True
        # switch off everything we dont need to reduce dependencies
        opencv:shared=False
        opencv:fPIC=True
        opencv:contrib=False
        opencv:jpegturbo=False
        opencv:tiff=False
        opencv:webp=False
        opencv:jasper=False
        opencv:openexr=False
        opencv:nonfree=False
        opencv:dc1394=False
        opencv:carotene=False
        opencv:cuda=False
        opencv:eigen=False
        opencv:protobuf=False
        opencv:harfbuzz=False
        opencv:glog=False
        opencv:gflags=False
        opencv:gstreamer=False
        opencv:openblas=False
        opencv:ffmpeg=False
        opencv:lapack=False
        opencv:quirc=False
    )
    list(
        APPEND
        CONAN_PACKAGES
        opencv/4.1.1@conan/stable
    )

    conan_add_remote(
        NAME bincrafters INDEX 1
        URL https://api.bintray.com/conan/bincrafters/public-conan
    )
    set(
        CONAN_OPTIONS
        ${opencv_conan_options}
    )
    if(DEBUG_CONAN)
        set(
            ENV{CONAN_TRACE_FILE}
            "${CMAKE_CURRENT_BINARY_DIR}/conan-trace.log"
        )
        set(
            ENV{CONAN_LOGGING_LEVEL}
            debug
        )
        set(
            ENV{CONAN_LOG_RUN_TO_FILE}
            1
        )
        set(
            ENV{CONAN_PRINT_RUN_COMMANDS}
            1
        )
    endif()
    message(STATUS "conan options ${CONAN_OPTIONS}")
    conan_cmake_run(
        BUILD_TYPE Release
        REQUIRES
        ${CONAN_PACKAGES}
        OPTIONS
        ${CONAN_OPTIONS}
        BASIC_SETUP
        CMAKE_TARGETS
        BUILD missing
        SETTINGS
        ${conan_compiler_libcxx}
        GENERATORS
        cmake_find_package #create FindXXX.cmake
        cmake_paths #create conan_paths.cmake
    )

    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

endif()
find_package(Vulkan REQUIRED)
find_package(glfw3 3.2)

if (USE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "*.frag"
    "*.vert"
    )
foreach(GLSL ${GLSL_SOURCE_FILES})
  compile_shader(${GLSL} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} SPIRV)
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_library(my_vulkan_offscreen
    my_vulkan/buffer.cpp
    my_vulkan/command_buffer.cpp
    my_vulkan/command_pool.cpp
    my_vulkan/descriptor_pool.cpp
    my_vulkan/descriptor_set.cpp
    my_vulkan/descriptor_set_layout.cpp
    my_vulkan/device.cpp
    my_vulkan/device_memory.cpp
    my_vulkan/fence.cpp
    my_vulkan/framebuffer.cpp
    my_vulkan/graphics_pipeline.cpp
    my_vulkan/image.cpp
    my_vulkan/image_view.cpp
    my_vulkan/instance.cpp
    my_vulkan/queue.cpp
    my_vulkan/render_pass.cpp
    my_vulkan/semaphore.cpp
    my_vulkan/shader_module.cpp
    my_vulkan/swap_chain.cpp
    my_vulkan/texture_sampler.cpp
    my_vulkan/utils.cpp
    my_vulkan/debug_callback.cpp
    my_vulkan/helpers/standard_swap_chain.cpp
    my_vulkan/helpers/offscreen_render_target.cpp
    my_vulkan/helpers/sync_points.cpp
    my_vulkan/helpers/texture_image.cpp
    my_vulkan/interop_utils.cpp
)
message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

target_include_directories(
    my_vulkan_offscreen
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    my_vulkan_offscreen
    PUBLIC
    Vulkan::Vulkan
    dl
    CONAN_PKG::boost
    CONAN_PKG::glm
    CONAN_PKG::opencv
)

if(glfw3_FOUND)
    add_library(my_vulkan
        my_vulkan/buffer.cpp
        my_vulkan/command_buffer.cpp
        my_vulkan/command_pool.cpp
        my_vulkan/descriptor_pool.cpp
        my_vulkan/descriptor_set.cpp
        my_vulkan/descriptor_set_layout.cpp
        my_vulkan/device.cpp
        my_vulkan/device_memory.cpp
        my_vulkan/fence.cpp
        my_vulkan/framebuffer.cpp
        my_vulkan/graphics_pipeline.cpp
        my_vulkan/image.cpp
        my_vulkan/image_view.cpp
        my_vulkan/instance.cpp
        my_vulkan/queue.cpp
        my_vulkan/render_pass.cpp
        my_vulkan/semaphore.cpp
        my_vulkan/shader_module.cpp
        my_vulkan/surface.cpp
        my_vulkan/swap_chain.cpp
        my_vulkan/texture_sampler.cpp
        my_vulkan/utils.cpp
        my_vulkan/debug_callback.cpp
        my_vulkan/helpers/standard_swap_chain.cpp
        my_vulkan/helpers/offscreen_render_target.cpp
        my_vulkan/helpers/texture_image.cpp
        my_vulkan/helpers/sync_points.cpp
        my_vulkan/interop_utils.cpp
    )

    target_link_libraries(
        my_vulkan
        PRIVATE
        glfw
        PUBLIC
        Vulkan::Vulkan
        dl
        CONAN_PKG::boost
        CONAN_PKG::glm
        CONAN_PKG::opencv
    )

    target_include_directories(
        my_vulkan
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

endif()

EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} --version OUTPUT_VARIABLE clang_full_version_string)
string (REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${clang_full_version_string})
message(STATUS "clang version: ${CLANG_VERSION_STRING}")
if(APPLE)
else(APPLE)
  if (CLANG_VERSION_STRING VERSION_LESS 9.0)
    set(CXX_FILESYSTEM_LIB c++fs)
  endif()
endif(APPLE)

if(glfw3_FOUND)
    add_executable(glfw_vulkan_test glfw_vulkan_test.cpp)
    target_include_directories(
        glfw_vulkan_test
        PRIVATE
    )
    target_link_libraries (
        glfw_vulkan_test
        glfw
        my_vulkan
        ${CXX_FILESYSTEM_LIB}
    )
    add_dependencies(glfw_vulkan_test Shaders)
endif(glfw3_FOUND)
