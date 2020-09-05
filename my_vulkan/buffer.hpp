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
            VkMemoryPropertyFlags properties,
            std::optional<VkExternalMemoryHandleTypeFlags> external_handle_type = std::nullopt
        );
        buffer_t(
            VkDevice device,
            VkPhysicalDevice physical_device,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            PFN_vkGetMemoryFdKHR pfn_vkGetMemoryFdKHR,
            std::optional<VkExternalMemoryHandleTypeFlags> external_handle_type = std::nullopt
        );
        buffer_t(const buffer_t&) = delete;
        buffer_t(buffer_t&& other) noexcept;
        buffer_t& operator=(const buffer_t&) = delete;
        buffer_t& operator=(buffer_t&& other) noexcept;
        device_memory_t * memory() const;
        device_memory_t * memory();
        VkBuffer get();
        VkDeviceSize size() const;
        VkMemoryPropertyFlags memory_properties() const;
        void load_data(command_pool_t& command_pool, const void* data);
        ~buffer_t();
    private:
        void cleanup();
        VkDevice _device;
        VkPhysicalDevice _physical_device;
        VkDeviceSize _size;
        VkBuffer _buffer;
        PFN_vkGetMemoryFdKHR _fpGetMemoryFdKHR;
        std::unique_ptr<device_memory_t> _memory;
        VkMemoryPropertyFlags _memory_properties;
    };
}
