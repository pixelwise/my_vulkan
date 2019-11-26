#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace my_vulkan
{
    struct framebuffer_t
    {
        framebuffer_t(
            VkDevice device,
            VkRenderPass render_pass,
            std::vector<VkImageView> attachments,
            VkExtent2D extent
        );
        framebuffer_t(const framebuffer_t&) = delete;
        framebuffer_t(framebuffer_t&& other) noexcept;
        framebuffer_t& operator=(const framebuffer_t&) = delete;
        framebuffer_t& operator=(framebuffer_t&& other) noexcept;
        VkFramebuffer get();
        VkExtent2D extent() const;
        ~framebuffer_t();
    private:
        void cleanup();
        VkDevice _device;
        VkExtent2D _extent;
        VkFramebuffer _framebuffer;
    };
}