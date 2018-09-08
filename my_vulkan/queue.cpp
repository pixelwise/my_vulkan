#include "queue.hpp"
#include "utils.hpp"

namespace my_vulkan
{
    queue_reference_t::queue_reference_t(VkQueue queue)
    : _queue{queue}
    {
    }

    void queue_reference_t::submit(VkCommandBuffer buffer, VkFence fence)
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &buffer;
        submit({submitInfo}, fence);
    }

    void queue_reference_t::submit(std::vector<VkSubmitInfo> submits, VkFence fence)
    {
        vk_require(
            vkQueueSubmit(
                _queue,
                submits.size(),
                submits.data(),
                fence
            ),
            "submitting to queue"
        );
    }

    void queue_reference_t::wait_idle()
    {
        vk_require(
            vkQueueWaitIdle(_queue),
            "waiting for queue to idle"
        );
    }

    VkQueue queue_reference_t::get()
    {
        return _queue;
    }
}
