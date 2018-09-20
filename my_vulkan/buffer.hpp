#pragma once

#include <vulkan/vulkan.h>
#include "device.hpp"
#include "device_memory.hpp"

namespace my_vulkan
{
    struct buffer_t
    {
        buffer_t(
            device_reference_t device,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties
        );
        buffer_t(const buffer_t&) = delete;
        buffer_t(buffer_t&& other) noexcept;
        buffer_t& operator=(const buffer_t&) = delete;
        buffer_t& operator=(buffer_t&& other) noexcept;
        device_memory_t* memory();
        VkBuffer get();
        ~buffer_t();
    private:
        void cleanup();
        VkDevice _device;
        VkBuffer _buffer;
        std::unique_ptr<device_memory_t> _memory;
    };
}
