cmake_minimum_required(VERSION 3.11)

project (vulkan_tests)

find_package(glfw3 3.2 REQUIRED)

add_executable(glfw_vulkan_test
    glfw_vulkan_test.cpp
)

target_include_directories(glfw_vulkan_test
    PRIVATE
    "/opt/vulkan/include"
)

target_link_libraries(glfw_vulkan_test
    glfw
    "/opt/vulkan/lib/libvulkan.dylib"
)
