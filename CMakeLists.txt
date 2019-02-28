project(my_vulkan)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(shaders)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall" )
find_package(Vulkan REQUIRED)
find_package(glfw3 3.2)
find_package(Boost 1.64 REQUIRED)

if (USE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

find_package(OpenCV 3 REQUIRED)
find_package(glm 0.9.8 REQUIRED)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "*.frag"
    "*.vert"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  compile_shader(${GLSL} SPIRV)
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
    my_vulkan/helpers/standard_swap_chain.cpp
    my_vulkan/helpers/offscreen_render_target.cpp
)
message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

target_include_directories(
    my_vulkan_offscreen
    PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
target_include_directories(
    my_vulkan_offscreen
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    my_vulkan_offscreen
    PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(
    my_vulkan_offscreen
    PRIVATE
    -ldl
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
    )

    target_link_libraries(
        my_vulkan PRIVATE
        glfw
    )

    target_include_directories(
        my_vulkan
        PUBLIC
        ${Boost_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
    )
    target_include_directories(
        my_vulkan
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_include_directories(
        my_vulkan
        PRIVATE
        ${Vulkan_INCLUDE_DIRS}
    )
endif()
if(glfw3_FOUND)
    add_executable(glfw_vulkan_test glfw_vulkan_test.cpp)
    target_include_directories(
        glfw_vulkan_test
        PRIVATE
        ${GLM_INCLUDE_DIRS}
    )
    target_link_libraries (
        glfw_vulkan_test
        ${Vulkan_LIBRARIES}
        glfw
        my_vulkan
    )
    add_dependencies(glfw_vulkan_test Shaders)
endif(glfw3_FOUND)
