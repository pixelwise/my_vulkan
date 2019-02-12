#pragma once
#include <vulkan/vulkan.h>
#include <limits>

namespace my_vulkan
{
    struct fence_t
    {
        fence_t(
            VkDevice device,
            VkFenceCreateFlags flags = 0
        );
        fence_t(const fence_t&) = delete;
        fence_t& operator=(const fence_t&) = delete;
        fence_t(fence_t&& other) noexcept;
        fence_t& operator=(fence_t&& other) noexcept;
        ~fence_t();
        void reset();
        void wait(
            uint64_t timeout = std::numeric_limits<uint64_t>::max()
        );
        VkFence get();
    private:
        void cleanup();
        VkDevice _device;
        VkFence _fence;
    };
}