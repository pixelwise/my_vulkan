#include "command_pool.hpp"
#include "utils.hpp"
#include <utility>

namespace my_vulkan
{
    command_pool_t::command_pool_t(
        VkDevice device,
        uint32_t queueFamilyIndex
    )
    : _device{device}
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        vk_require(
            vkCreateCommandPool(device, &poolInfo, nullptr, &_command_pool),
            "creating command pool"
        );
        vkGetDeviceQueue(_device, queueFamilyIndex, 0, &_queue);
    }

    command_pool_t::command_pool_t(command_pool_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    command_pool_t& command_pool_t::operator=(command_pool_t&& other) noexcept
    {
        cleanup();
        _command_pool = other._command_pool;
        std::swap(_device, other._device);
        return *this;
    }

    command_buffer_t command_pool_t::make_buffer(
        VkCommandBufferLevel level
    )
    {
        return {_device, _command_pool, level};
    }

    queue_reference_t command_pool_t::queue()
    {
        return _queue;
    }

    VkCommandPool command_pool_t::get()
    {
        return _command_pool;
    }

    command_pool_t::~command_pool_t()
    {
        cleanup();
    }

    void command_pool_t::cleanup()
    {
        if (_device)
        {
            vkDestroyCommandPool(_device, _command_pool, 0);
            _device = 0;
        }
    }
}