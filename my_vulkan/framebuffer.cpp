#include "framebuffer.hpp"
#include "utils.hpp"
namespace my_vulkan
{
    framebuffer_t::framebuffer_t(
        VkDevice device,
        VkRenderPass render_pass,
        std::vector<VkImageView> attachments,
        VkExtent2D extent
    )
    : _device{device} 
    , _extent{extent}   
    {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;
        my_vulkan::vk_require(
            vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_framebuffer),
            "creating framebuffer"
        );        
    }

    framebuffer_t::framebuffer_t(framebuffer_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    framebuffer_t& framebuffer_t::operator=(framebuffer_t&& other) noexcept
    {
        cleanup();
        _framebuffer = other._framebuffer;
        std::swap(_device, other._device);
        return *this;
    }

    VkFramebuffer framebuffer_t::get()
    {
        return _framebuffer;
    }

    VkExtent2D framebuffer_t::extent() const
    {
        return _extent;
    }

    framebuffer_t::~framebuffer_t()
    {
        cleanup();
    }

    void framebuffer_t::cleanup()
    {
        if (_device)
        {
            vkDestroyFramebuffer(_device, _framebuffer, 0);
            _device = 0;
        }
    }

}