#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "queue.hpp"
#include "utils.hpp"

namespace my_vulkan
{
    struct device_reference_t
    {
        device_reference_t(
            VkPhysicalDevice physical_device,
            VkDevice device,
            queue_family_indices_t queue_indices = {},
            VkQueue graphicsQueue = 0,
            VkQueue presentQueue = 0
        );
        VkDevice get() const;
        VkPhysicalDevice physical_device() const;
        void clear();
        queue_reference_t graphics_queue();
        queue_reference_t present_queue();
        queue_family_indices_t queue_indices();
    protected:
        VkPhysicalDevice _physical_device;
        VkDevice _device;
        queue_family_indices_t _queue_indices;
        VkQueue _graphicsQueue;
        VkQueue _presentQueue;
    };

    struct device_t : public device_reference_t
    {
        device_t(
            VkPhysicalDevice physical_device,
            queue_family_indices_t queue_indices,
            std::vector<const char*> validation_layers,
            std::vector<const char*> device_extensions        
        );
        device_t(const device_t&) = delete;
        ~device_t();
        void wait_idle();
    };
}
