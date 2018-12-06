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
            VkDevice device
        );
        VkDevice get() const;
        VkPhysicalDevice physical_device() const;
        void clear();
    private:
        VkPhysicalDevice _physical_device;
        VkDevice _device;
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
        queue_reference_t graphics_queue();
        queue_reference_t present_queue();
        void wait_idle();
    private:
        VkQueue _graphicsQueue{0};
        VkQueue _presentQueue{0};
    };
}
