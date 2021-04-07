#define GLFW_INCLUDE_VULKAN
#include "surface.hpp"

#include <stdexcept>

namespace my_vulkan
{
    surface_t::surface_t(VkInstance instance, GLFWwindow* window)
    : _instance{instance}
    {
        switch (glfwCreateWindowSurface(instance, window, nullptr, &_surface))
        {
            case VK_SUCCESS:
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                throw std::runtime_error{
                    "glfw: could not find functional vulkan ICD for window surface"
                };
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                throw std::runtime_error{
                    "glfw: could not find required vulkan extensions"
                };
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                throw std::runtime_error{
                    "glfw: window surface cannot be api shared, try setting GLFW_NO_API hint"
                };
            default:
                throw std::runtime_error{
                    "glfw: window surface creation failed with unknown error"
                };
        }
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
