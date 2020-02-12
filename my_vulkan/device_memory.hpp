#pragma once

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

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
                std::optional<region_t> region = std::nullopt,
                VkMemoryMapFlags flags = 0
            );
            void* data();
            mapping_t() = delete;
            mapping_t(const mapping_t&) = delete;
            mapping_t(mapping_t&& other) noexcept;
            mapping_t& operator=(const mapping_t&) = delete;
            mapping_t& operator=(mapping_t&& other) noexcept;
            ~mapping_t();
            void flush();
            void invalidate();
            void unmap();
        private:
            void cleanup();
            void* _data;
            VkDeviceMemory _memory{0};
            VkDevice _device;
            region_t _region;
        };
        struct config_t
        {
            VkDeviceSize size;
            uint32_t type_index;
            std::optional<VkExternalMemoryHandleTypeFlags> external_handle_type;
        };
        device_memory_t(
            VkDevice device,
            config_t config
        );
        device_memory_t(const device_memory_t&) = delete;
        device_memory_t(device_memory_t&& other) noexcept;
        device_memory_t& operator=(const device_memory_t&&) = delete;
        device_memory_t& operator=(device_memory_t&& other) noexcept;
        ~device_memory_t(); 
        mapping_t map(
            std::optional<region_t> region = std::nullopt,
            VkMemoryMapFlags flags = 0
        );
        size_t size() const;
        template<typename T>
        void set_data(const std::vector<T>& data);
        void set_data(const void* data, size_t size);
        VkDeviceMemory get();
        [[nodiscard]] std::optional<int> get_external_handle(VkExternalMemoryHandleTypeFlagBits externalHandleType) const;
    private:
        void cleanup();
        VkDevice _device{0};
        PFN_vkGetMemoryFdKHR _fpGetMemoryFdKHR{nullptr};
        VkDeviceMemory _memory{0};
        size_t _size;
    };

    template<typename T>
    void device_memory_t::set_data(const std::vector<T>& data)
    {
        set_data(data.data(), data.size() * sizeof(T));
    }
}
