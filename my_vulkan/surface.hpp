#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace my_vulkan
{
    struct surface_t
    {
        surface_t(VkInstance instance, GLFWwindow* window);
        ~surface_t();
        surface_t(const surface_t& other) = delete;
        surface_t(surface_t&& other) = delete;
        surface_t& operator=(const surface_t& other) = delete;
        surface_t& operator=(surface_t&& other) = delete;
        VkSurfaceKHR get();
    private:
        VkInstance _instance;
        VkSurfaceKHR _surface;
    };
}