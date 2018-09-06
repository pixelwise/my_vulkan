#pragma once

#include <vulkan/vulkan.h>
#include <boost/optional.hpp>

namespace my_vulkan
{
    struct device_memory_t
    {
        struct region_t
        {
            VkDeviceSize offset;
            VkDeviceSize size;
        };
        struct mapping_t
        {
            mapping_t(
                device_memory_t& memory,
                boost::optional<region_t> region = boost::none,
                VkMemoryMapFlags flags = 0
            );
            void* data();
            mapping_t(const mapping_t&) = delete;
            mapping_t(mapping_t&& other) noexcept;
            mapping_t& operator=(const mapping_t&&) = delete;
            mapping_t& operator=(mapping_t&& other) noexcept;
            ~mapping_t();
        private:
            void cleanup();
            void* _data;
            device_memory_t* _memory{0};
        };
        struct config_t
        {
            VkDeviceSize size;
            uint32_t type_index;
        };
        device_memory_t(
            VkDevice device,
            config_t config
        );
        device_memory_t(const device_memory_t&) = delete;
        device_memory_t(device_memory_t&& other) noexcept;
        device_memory_t& operator=(const device_memory_t&&) = delete;
        device_memory_t& operator=(device_memory_t&& other) noexcept;
        mapping_t map();
        size_t size() const;
        VkDeviceMemory get();
        ~device_memory_t();
    private:
        void cleanup();
        VkDevice _device{0};
        VkDeviceMemory _memory{0};
        size_t _size;
    };
}