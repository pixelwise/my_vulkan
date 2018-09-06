#include "surface.hpp"

#include <stdexcept>

namespace my_vulkan
{
    surface_t::surface_t(VkInstance instance, GLFWwindow* window)
    : _instance{instance}
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &_surface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
    }
    surface_t::~surface_t()
    {
        vkDestroySurfaceKHR(_instance, _surface, 0);
    }
    VkSurfaceKHR surface_t::get()
    {
        return _surface;
    }
}
