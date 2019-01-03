#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "queue.hpp"
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
        void wait_idle();
        VkPhysicalDevice physical_device() const;
        queue_reference_t& graphics_queue();
        queue_reference_t& present_queue();
        queue_reference_t& transfer_queue();
        queue_family_indices_t queue_indices();
        VkDevice get() const;
    private:
        VkPhysicalDevice _physical_device;
        VkDevice _device;
        queue_family_indices_t _queue_indices;
        std::vector<queue_reference_t> _queues;
        queue_reference_t* _graphics_queue{0};
        queue_reference_t* _present_queue{0};
        queue_reference_t* _transfer_queue{0};
    };
}
