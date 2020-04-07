#pragma once

#include <vulkan/vulkan.h>

namespace my_vulkan
{
    struct render_pass_t
    {
        render_pass_t(
            VkDevice device,
            VkRenderPassCreateInfo info
        );
        render_pass_t(
            VkDevice device,
            VkFormat color_format,
            VkFormat depth_format = VK_FORMAT_UNDEFINED,
            VkImageLayout color_attachment_final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VkAttachmentLoadOp attachment_loadop = VK_ATTACHMENT_LOAD_OP_DONT_CARE
        );
        render_pass_t(render_pass_t&& other) noexcept;
        render_pass_t(const render_pass_t&) = delete;
        render_pass_t& operator=(render_pass_t&& other) noexcept;
        render_pass_t& operator=(const render_pass_t&) = delete;
        ~render_pass_t();
        VkRenderPass get();
        VkDevice device() const;
    private:
        void cleanup();
        VkDevice _device;
        VkRenderPass _render_pass;
    };
};
