project(my_vulkan)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(shaders)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall" )

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

if (USE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

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

include_directories(${Vulkan_INCLUDE_DIRS})

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
  my_vulkan/helpers/standard_swap_chain.cpp
)

add_executable(glfw_vulkan_test glfw_vulkan_test.cpp)
target_link_libraries (
  glfw_vulkan_test
  ${Vulkan_LIBRARIES}
  glfw
  my_vulkan
)
add_dependencies(glfw_vulkan_test Shaders)
