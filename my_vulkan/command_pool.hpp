#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "command_buffer.hpp"
#include "queue.hpp"

namespace my_vulkan
{
    struct command_pool_t
    {
        struct one_time_scope_t
        {
            one_time_scope_t(command_pool_t& pool);
            command_buffer_t::scope_t& commands();
            void execute_and_wait();
        private:
            command_buffer_t _buffer;
            command_buffer_t::scope_t _scope;
            queue_reference_t* _queue;
        };
       command_pool_t(
            VkDevice device,
            queue_reference_t& queue
        );
        command_pool_t(const command_pool_t&) = delete;
        command_pool_t(command_pool_t&& other) noexcept;
        command_pool_t& operator=(const command_pool_t&) = delete;
        command_pool_t& operator=(command_pool_t&& other) noexcept;
        VkCommandPool get();
        command_buffer_t make_buffer(
            VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
        );
        one_time_scope_t begin_oneshot();
        queue_reference_t& queue();
        VkDevice device();
        ~command_pool_t();
    private:
        void cleanup();
        VkDevice _device;
        VkCommandPool _command_pool;
        queue_reference_t* _queue;
    };
}
