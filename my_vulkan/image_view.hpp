#pragma once

#include <vulkan/vulkan.h>

namespace my_vulkan
{
    struct image_view_t
    {
        image_view_t();
        image_view_t(VkDevice device, VkImageView view);
        image_view_t(image_view_t&& other) noexcept;
        image_view_t(const image_view_t&) = delete;
        image_view_t& operator=(image_view_t&& other) noexcept;
        image_view_t& operator=(const image_view_t&) = delete;
        VkImageView get() const;
    private:
        void cleanup();
        VkDevice _device{0};
        VkImageView _view{0};
    };
}
