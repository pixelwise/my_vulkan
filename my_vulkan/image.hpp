#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include "image_view.hpp"
#include "device_memory.hpp"

namespace my_vulkan
{
    struct image_t
    {
        image_t(
            VkPhysicalDevice physical_device,
            VkDevice logical_device,
            VkExtent3D extent,
            VkFormat format,
            VkImageUsageFlags usage,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
            VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        image_t(VkDevice device, VkImage image, VkFormat format);
        image_t(const image_t&) = delete;
        image_t(image_t&& other) noexcept;
        image_t& operator=(const image_t&) = delete;
        image_t& operator=(image_t&& other) noexcept;
        ~image_t();
        image_view_t view(VkImageAspectFlagBits aspect_flags) const;
        VkImage get();
        device_memory_t* memory();
        VkFormat format();
    private:
        bool _borrowed;
        void cleanup();
        VkDevice _device{0};
        VkFormat _format;
        VkImage _image;
        std::unique_ptr<device_memory_t> _memory;
    };
}