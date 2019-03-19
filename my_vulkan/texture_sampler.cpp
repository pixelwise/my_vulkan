#include "texture_sampler.hpp"
#include "utils.hpp"

namespace my_vulkan
{
    texture_sampler_t::texture_sampler_t(VkDevice device, filter_mode_t filter_mode)
    : _device{device}
    {
        VkFilter filter;
        switch(filter_mode)
        {
            case filter_mode_t::linear:
                filter = VK_FILTER_LINEAR;
                break;
            case filter_mode_t::nearest:
                filter = VK_FILTER_NEAREST;
                break;
            case filter_mode_t::cubic:
                filter = VK_FILTER_CUBIC_IMG;
                break;
        }
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.pNext = 0;
        samplerInfo.flags = 0;
        samplerInfo.magFilter = filter;
        samplerInfo.minFilter = filter;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        vk_require(
            vkCreateSampler(device, &samplerInfo, nullptr, &_sampler),
            "creating texture sampler"
        );
    }

    texture_sampler_t::texture_sampler_t(texture_sampler_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    texture_sampler_t& texture_sampler_t::operator=(texture_sampler_t&& other) noexcept
    {
        cleanup();
        _sampler = other._sampler;
        std::swap(_device, other._device);
        return *this;
    }

    texture_sampler_t::~texture_sampler_t()
    {
        cleanup();
    }

    void texture_sampler_t::cleanup()
    {
        if (_device)
        {
            vkDestroySampler(_device, _sampler, nullptr);
            _device = 0;
        }
    }

    VkSampler texture_sampler_t::get()
    {
        return _sampler;   
    }
}
