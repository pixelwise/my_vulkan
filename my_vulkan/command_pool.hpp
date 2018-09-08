#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "command_buffer.hpp"
#include "queue.hpp"

namespace my_vulkan
{
    struct command_pool_t
    {
       command_pool_t(
            VkDevice device,
            uint32_t queueFamilyIndex
        );
        command_pool_t(const command_pool_t&) = delete;
        command_pool_t(command_pool_t&& other) noexcept;
        command_pool_t& operator=(const command_pool_t&) = delete;
        command_pool_t& operator=(command_pool_t&& other) noexcept;
        VkCommandPool get();
        command_buffer_t make_buffer(
            VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
        );
        queue_reference_t queue();
        ~command_pool_t();
    private:
        void cleanup();
        VkDevice _device;
        VkCommandPool _command_pool;
        VkQueue _queue;
    };
}
