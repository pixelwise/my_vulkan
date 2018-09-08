#pragma once

#include <vulkan/vulkan.h>
#include <vector>

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
        ~command_pool_t();
    private:
        void cleanup();
        VkDevice _device;
        VkCommandPool _command_pool;
    };
}
