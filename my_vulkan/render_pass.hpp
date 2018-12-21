#pragma once

#include <vulkan/vulkan.hpp>

namespace my_vulkan
{
    struct render_pass_t
    {
        render_pass_t(
            VkDevice device,
            VkFormat image_format,
            VkFormat depth_format,
            VkImageLayout color_attachment_final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );
        render_pass_t(render_pass_t&& other);
        render_pass_t(const render_pass_t&) = delete;
        render_pass_t& operator=(render_pass_t&& other);
        render_pass_t& operator=(const render_pass_t&) = delete;
        VkRenderPass get();
    private:
        void cleanup();
        VkDevice _device;
        VkRenderPass _render_pass;
    };
};