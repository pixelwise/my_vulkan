project(vulkanHelloWorld)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++11  " )

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
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND glslc ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)

include_directories(${Vulkan_INCLUDE_DIRS})

add_library(my_vulkan
  my_vulkan/instance.cpp
  my_vulkan/utils.cpp
  my_vulkan/surface.cpp
  my_vulkan/device.cpp
  my_vulkan/image_view.cpp
  my_vulkan/image.cpp
  my_vulkan/device_memory.cpp
  my_vulkan/swap_chain.cpp
)

add_executable(${PROJECT_NAME} glfw_vulkan_test.cpp)
target_link_libraries (
  ${PROJECT_NAME}
  ${Vulkan_LIBRARIES}
  glfw
  my_vulkan
)
