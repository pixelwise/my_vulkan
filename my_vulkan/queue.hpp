 #pragma once

#include "utils.hpp"

#include <vulkan/vulkan.h>

#include <boost/optional.hpp>

#include <vector>
#include <mutex>

namespace my_vulkan
{
    struct queue_reference_t
    {
        queue_reference_t(VkQueue queue, uint32_t family_index);
        void set(VkQueue queue);
        struct wait_semaphore_info_t
        {
            VkSemaphore semaphore;
            VkPipelineStageFlags stage;
        };
        void submit(VkCommandBuffer buffer, VkFence fence = 0);
        void submit(
            VkCommandBuffer buffer,
            std::vector<wait_semaphore_info_t> wait_semaphores,
            std::vector<VkSemaphore> signal_semaphores,
            VkFence fence = 0
        );
        void submit(std::vector<VkSubmitInfo> submits, VkFence fence = 0);
        struct swapchain_target_t
        {
            VkSwapchainKHR swap_chain;
            uint32_t image_index;
        };
        boost::optional<acquisition_failure_t> present(
            swapchain_target_t swap_chain_target,
            std::vector<VkSemaphore> semaphores = {}
        );
        uint32_t family_index();
        queue_reference_t(queue_reference_t&& other) noexcept
        : _queue{other._queue}
        , _family_index{other._family_index}
        {}
    private:
        std::mutex _mutex;
        VkQueue _queue;
        uint32_t _family_index;
    };
}
