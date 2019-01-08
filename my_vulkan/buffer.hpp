#pragma once

#include <vulkan/vulkan.h>
#include "device.hpp"
#include "device_memory.hpp"
#include "command_pool.hpp"
#include <memory>

namespace my_vulkan
{
    struct buffer_t
    {
        buffer_t(
            device_t& device,
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
        VkDeviceSize size() const;
        void load_data(command_pool_t& command_pool, const void* data);
        ~buffer_t();
    private:
        void cleanup();
        device_t* _device;
        VkDeviceSize _size;
        VkBuffer _buffer;
        std::unique_ptr<device_memory_t> _memory;
    };
}
