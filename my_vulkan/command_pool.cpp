#include "command_pool.hpp"
#include "utils.hpp"
#include <utility>

namespace my_vulkan
{
    command_pool_t::one_time_scope_t::one_time_scope_t(command_pool_t& pool)
    : _buffer{pool.make_buffer()}
    , _scope{_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)}
    , _queue{&pool.queue()}
    {
    }

    command_buffer_t::scope_t& command_pool_t::one_time_scope_t::commands()
    {
        return _scope;
    }

    void command_pool_t::one_time_scope_t::execute_and_wait()
    {
        commands().end();
        _queue->submit(_buffer.get());
        _queue->wait_idle();        
    }

    command_pool_t::command_pool_t(
        VkDevice device,
        queue_reference_t& queue
    )
    : _device{device}
    , _queue{&queue}
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queue.family_index();
        vk_require(
            vkCreateCommandPool(device, &poolInfo, nullptr, &_command_pool),
            "creating command pool"
        );
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
        _queue = other._queue;
        std::swap(_device, other._device);
        return *this;
    }

    command_buffer_t command_pool_t::make_buffer(
        VkCommandBufferLevel level
    )
    {
        return {_device, _command_pool, level};
    }

    queue_reference_t& command_pool_t::queue()
    {
        return *_queue;
    }

    VkCommandPool command_pool_t::get()
    {
        return _command_pool;
    }

    command_pool_t::~command_pool_t()
    {
        cleanup();
    }

    command_pool_t::one_time_scope_t command_pool_t::begin_oneshot()
    {
        return {*this};
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