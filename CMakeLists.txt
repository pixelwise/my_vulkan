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

add_executable(${PROJECT_NAME} glfw_vulkan_test.cpp)
target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw)

add_executable(tutorial_06 06_swap_chain_creation.cpp)
target_link_libraries (tutorial_06 ${Vulkan_LIBRARIES} glfw)

add_executable(tutorial_15 15_hello_triangle.cpp)
add_dependencies(tutorial_15 Shaders)
target_link_libraries (tutorial_15 ${Vulkan_LIBRARIES} glfw)

add_executable(tutorial_22 22_descriptor_sets.cpp)
add_dependencies(tutorial_22 Shaders)
target_link_libraries (tutorial_22 ${Vulkan_LIBRARIES} glfw)

add_executable(tutorial_29 29_multisampling.cpp)
add_dependencies(tutorial_29 Shaders)
target_link_libraries (tutorial_29 ${Vulkan_LIBRARIES} glfw)
