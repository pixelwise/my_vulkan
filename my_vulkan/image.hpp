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
        image_t(
            device_reference_t device,
            VkExtent3D extent,
            VkFormat format,
            VkImageUsageFlags usage = 0,
            VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
            VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        image_t(
            VkDevice device,
            VkImage image,
            VkFormat format,
            VkExtent3D extent,
            VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED
        );
        image_t(
            VkDevice device,
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
            VkImageAspectFlagBits aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT
        ) const;
        VkSubresourceLayout memory_layout(
            VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
            uint32_t mipLevel = 0,
            uint32_t arrayLayer = 0
        ) const;
        VkImage get();
        device_memory_t* memory();
        VkFormat format() const;
        VkExtent3D extent() const;
        VkImageLayout layout() const;
        void copy_from(
            VkBuffer buffer,
            command_buffer_t::scope_t& command_scope,
            boost::optional<VkExtent3D> extent = boost::none
        );
        void copy_from(
            VkBuffer buffer,
            command_pool_t& command_pool,
            boost::optional<VkExtent3D> extent = boost::none
        );
        void transition_layout(
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            command_buffer_t::scope_t& command_scope
        );
        void load_pixels(command_pool_t& commands, const void* pixels);
    private:
        void cleanup();
        device_reference_t _device;
        VkImage _image;
        VkFormat _format;
        VkExtent3D _extent;
        VkImageLayout _layout;
        bool _borrowed;
        std::unique_ptr<device_memory_t> _memory;
    };
}