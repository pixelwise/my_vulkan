#pragma once

#include <vulkan/vulkan.h>

namespace my_vulkan
{
    struct semaphore_t
    {
        semaphore_t(
            VkDevice device,
            VkSemaphoreCreateFlags flags = 0
        );
        semaphore_t(const semaphore_t&) = delete;
        semaphore_t& operator=(const semaphore_t&) = delete;
        semaphore_t(semaphore_t&& other) noexcept;
        semaphore_t& operator=(semaphore_t&& other) noexcept;
        VkSemaphore get();
        ~semaphore_t();
    private:
        void cleanup();
        VkDevice _device;
        VkSemaphore _semaphore;    
    };
}
