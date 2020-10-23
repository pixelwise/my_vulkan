#include "render_pass.hpp"
#include "utils.hpp"
#include <memory>

namespace my_vulkan
{
    render_pass_t::render_pass_t(
        VkDevice device,
        VkRenderPassCreateInfo info
    )
    : _device{device}
    {
        vk_require(
            vkCreateRenderPass(device, &info, nullptr, &_render_pass),
            "creating render pass"
        );        
    }

    render_pass_t::render_pass_t(
        VkDevice device,
        VkFormat color_format,
        VkFormat depth_format,
        VkImageLayout color_attachment_final_layout,
        VkAttachmentLoadOp attachment_loadop
    )
    : _device{device}
    {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = color_format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = attachment_loadop;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = color_attachment_final_layout;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        std::vector<VkAttachmentDescription> attachments{
            colorAttachment
        };

        std::unique_ptr<VkAttachmentReference> depthAttachmentRef;
        if (depth_format != VK_FORMAT_UNDEFINED)
        {
            VkAttachmentDescription depthAttachment = {};
            depthAttachment.format = depth_format;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = attachment_loadop;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments.push_back(depthAttachment);
            depthAttachmentRef.reset(new VkAttachmentReference{});
            depthAttachmentRef->attachment = VK_ATTACHMENT_UNUSED;
            depthAttachmentRef->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachmentRef->attachment = 1;
        }


        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = depthAttachmentRef.get();

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 0;
        renderPassInfo.pDependencies = 0;
        vk_require(
            vkCreateRenderPass(device, &renderPassInfo, nullptr, &_render_pass),
            "creating render pass"
        );
    }

    render_pass_t::render_pass_t(render_pass_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    render_pass_t& render_pass_t::operator=(render_pass_t&& other) noexcept
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

    render_pass_t::~render_pass_t()
    {
        cleanup();
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
}
