#include <utility>
#include <vulkan/vulkan.h>
#include "image_view.hpp"

namespace my_vulkan
{
    image_view_t::image_view_t()
    {
    }

    image_view_t::image_view_t(VkDevice device, VkImageView view)
    : _device{device}
    , _view{view}
    {
    }

    image_view_t::image_view_t(image_view_t&& other) noexcept
    {
        *this = std::move(other);
    }

    image_view_t& image_view_t::operator=(image_view_t&& other) noexcept
    {
        cleanup();
        _view = other._view;
        std::swap(_device, other._device);
        return *this;
    }

    VkImageView image_view_t::get() const
    {
        return _view;
    }

    image_view_t::~image_view_t()
    {
        cleanup();
    }

    void image_view_t::cleanup()
    {
        if (_device)
        {
            vkDestroyImageView(_device, _view, 0);
            _device = 0;
        }
    }
}
