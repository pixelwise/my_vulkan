#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

int main()
{
    if (glfwVulkanSupported())
        std::cout << "vulkan supported" << std::endl;
    else
        std::cout << "vulkan not supported" << std::endl;
    return 0;
}