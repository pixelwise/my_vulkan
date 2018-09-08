#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace my_vulkan
{
    struct queue_reference_t
    {
        queue_reference_t(VkQueue queue);
        void submit(VkCommandBuffer buffer, VkFence fence = 0);
        void submit(std::vector<VkSubmitInfo> submits, VkFence fence = 0);
        void wait_idle();
        VkQueue get();
    private:
        VkQueue _queue;
    };
}
