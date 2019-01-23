#include "render_pass.hpp"
#include "utils.hpp"

namespace my_vulkan
{
    render_pass_t::render_pass_t(
        VkDevice device,
        VkFormat color_format,
        VkFormat depth_format,
        VkImageLayout color_attachment_final_layout
    )
    : _device{device}
    , _color_format{color_format}
    , _depth_format{depth_format}
    {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = color_format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = color_attachment_final_layout;

        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = depth_format;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        std::array<VkAttachmentDescription, 2> attachments{{
            colorAttachment,
            depthAttachment
        }};
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        vk_require(
            vkCreateRenderPass(device, &renderPassInfo, nullptr, &_render_pass),
            "creating render pass"
        );
    }

    render_pass_t::render_pass_t(render_pass_t&& other)
    : _device{0}
    {
        *this = std::move(other);
    }

    render_pass_t& render_pass_t::operator=(render_pass_t&& other)
    {
        cleanup();
        _render_pass = other._render_pass;
        std::swap(_device, other._device);
        return *this;
    }

    VkRenderPass render_pass_t::get()
    {
        return _render_pass;
    }

    void render_pass_t::cleanup()
    {
        if (_device)
        {
            vkDestroyRenderPass(_device, _render_pass, 0);
            _device = 0;
        }
    }

    VkDevice render_pass_t::device() const
    {
        return _device;
    }

    VkFormat render_pass_t::color_format() const
    {
        return _color_format;
    }

    VkFormat render_pass_t::depth_format() const
    {
        return _depth_format;
    }

}
