#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "utils.hpp"

namespace my_vulkan
{
    struct device_t
    {
        device_t(
            VkPhysicalDevice physical_device,
            queue_family_indices_t queue_indices,
            std::vector<const char*> validation_layers,
            std::vector<const char*> device_extensions        
        );
        device_t(const device_t&) = delete;
        ~device_t();
        VkDevice get();
        VkQueue graphics_queue();
        VkQueue present_queue();
        VkPhysicalDevice physical_device();
    private:
        VkPhysicalDevice _physical_device;
        VkDevice _device;
        VkQueue _graphicsQueue;
        VkQueue _presentQueue;
    };
}
