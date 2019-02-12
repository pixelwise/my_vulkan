#include "fence.hpp"

#include "utils.hpp"

namespace my_vulkan
{
    fence_t::fence_t(
        VkDevice device,
        VkFenceCreateFlags flags
    )
    : _device{device}
    {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = flags;
        vk_require(
            vkCreateFence(device, &fenceInfo, nullptr, &_fence), 
            "create fence"
        );
    }

    fence_t::fence_t(fence_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    fence_t& fence_t::operator=(fence_t&& other) noexcept
    {
        cleanup();
        _fence = other._fence;
        std::swap(_device, other._device);
        return *this;
    }

    void fence_t::reset()
    {
        vkResetFences(_device, 1, &_fence);        
    }

    void fence_t::wait(
        uint64_t timeout
    )
    {
        vkWaitForFences(_device, 1, &_fence, VK_TRUE, timeout);
    }

    VkFence fence_t::get()
    {
        return _fence;
    }

    fence_t::~fence_t()
    {
        cleanup();
    }

    void fence_t::cleanup()
    {
        if (_device)
        {
            vkDestroyFence(_device, _fence, 0);
            _device = 0;
        }
    }
 }