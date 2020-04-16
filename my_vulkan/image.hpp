#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include "image_view.hpp"
#include "device.hpp"
#include "device_memory.hpp"
#include "command_buffer.hpp"
#include "command_pool.hpp"

namespace my_vulkan
{
    struct image_t
    {
        struct dont_bind_memory_t {};   
        image_t(
            device_t& device,
            VkExtent3D extent,
            VkFormat format,
            VkImageUsageFlags usage,
            dont_bind_memory_t,
            VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL
        );
        image_t(
            device_t& device,
            VkExtent3D extent,
            VkFormat format,
            VkImageUsageFlags usage,
            VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
            VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            std::optional<VkExternalMemoryHandleTypeFlags> external_handle_types = std::nullopt
        );
        image_t(
            VkDevice device,
            VkPhysicalDevice physical_device,
            VkImage image,
            VkFormat format,
            VkExtent3D extent,
            VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED
        );
        image_t(
            VkDevice device,
            VkPhysicalDevice physical_device,
            VkImage image,
            VkFormat format,
            VkExtent2D extent,
            VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED
        );
        image_t(const image_t&) = delete;
        image_t(image_t&& other) noexcept;
        image_t& operator=(const image_t&) = delete;
        image_t& operator=(image_t&& other) noexcept;
        ~image_t();
        image_view_t view(
            int aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT
        ) const;
        VkSubresourceLayout memory_layout(
            int aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
            uint32_t mipLevel = 0,
            uint32_t arrayLayer = 0
        ) const;
        VkImage get();
        device_memory_t* memory();
        device_memory_t* memory() const;
        VkFormat format() const;
        VkExtent3D extent() const;
        VkImageLayout layout() const;
        void copy_from(
            VkBuffer buffer,
            command_buffer_t::scope_t& command_scope,
            uint32_t pitch = 0,
            std::optional<VkExtent3D> extent = std::nullopt
        );
        void copy_from(
            VkBuffer buffer,
            command_pool_t& command_pool,
            std::optional<VkExtent3D> extent = std::nullopt
        );
        void copy_from(
            VkImage image,
            command_buffer_t::scope_t& command_scope,
            std::optional<VkExtent3D> extent = std::nullopt,
            int src_aspects = VK_IMAGE_ASPECT_COLOR_BIT,
            int dst_aspects = VK_IMAGE_ASPECT_COLOR_BIT
        );
        void blit_from(
            image_t& image,
            command_buffer_t::scope_t& command_scope,
            int src_aspects = VK_IMAGE_ASPECT_COLOR_BIT,
            int dst_aspects = VK_IMAGE_ASPECT_COLOR_BIT
        );
        void transition_layout(
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            command_buffer_t::scope_t& command_scope
        );
        void transition_layout(
            VkImageLayout newLayout,
            command_buffer_t::scope_t& command_scope
        );
        void load_pixels(
            command_pool_t& commands,
            const void* pixels,
            std::optional<size_t> pitch = std::nullopt
        );
        std::optional<device_memory_t::external_memory_info_t> external_memory_info(VkExternalMemoryHandleTypeFlagBits externalHandleType);
    private:
        image_t(
            VkDevice device,
            VkPhysicalDevice physical_device,
            VkExtent3D extent,
            VkFormat format,
            VkImageUsageFlags usage,
            PFN_vkGetMemoryFdKHR pfn_vkGetMemoryFdKHR,
            VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
            VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            std::optional<VkExternalMemoryHandleTypeFlags> external_handle_types = std::nullopt,
            bool bind_memory = true
        );

        void cleanup();
        VkDevice _device;
        VkPhysicalDevice _physical_device;
        std::optional<VkExternalMemoryHandleTypeFlags> _external_handle_types;
        VkImage _image;
        VkFormat _format;
        VkExtent3D _extent;
        VkImageLayout _layout;
        bool _borrowed;
        std::unique_ptr<device_memory_t> _memory;
    };
}
