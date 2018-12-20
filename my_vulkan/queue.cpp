#include "queue.hpp"
#include "utils.hpp"

namespace my_vulkan
{
    queue_reference_t::queue_reference_t(VkQueue queue, uint32_t family_index)
    : _queue{queue}
    , _family_index{family_index}
    {
    }

    void queue_reference_t::set(VkQueue queue)
    {
        _queue = queue;
    }

    void queue_reference_t::submit(
        VkCommandBuffer buffer,
        std::vector<wait_semaphore_info_t> wait_semaphore_infos,
        std::vector<VkSemaphore> signal_semaphores,
        VkFence fence
    )
    {
        std::vector<VkSemaphore> wait_semaphores;
        std::vector<VkPipelineStageFlags> wait_semaphore_stages;
        for (auto&& info : wait_semaphore_infos)
        {
            wait_semaphores.push_back(info.semaphore);
            wait_semaphore_stages.push_back(info.stage);
        }
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &buffer;
        submitInfo.signalSemaphoreCount = signal_semaphores.size();
        submitInfo.pSignalSemaphores = signal_semaphores.data();
        submitInfo.waitSemaphoreCount = wait_semaphores.size();
        submitInfo.pWaitSemaphores = wait_semaphores.data();
        submitInfo.pWaitDstStageMask = wait_semaphore_stages.data();
        submit({submitInfo}, fence);
    }

    void queue_reference_t::submit(VkCommandBuffer buffer, VkFence fence)
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &buffer;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.waitSemaphoreCount = 0;
        submit({submitInfo}, fence);
    }

    void queue_reference_t::submit(std::vector<VkSubmitInfo> submits, VkFence fence)
    {
        std::unique_lock<std::mutex> lock{_mutex};
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

    boost::optional<acquisition_failure_t> queue_reference_t::present(
        swapchain_target_t swap_chain_target,
        std::vector<VkSemaphore> semaphores
    )
    {
        std::unique_lock<std::mutex> lock{_mutex};
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = semaphores.size();
        presentInfo.pWaitSemaphores = semaphores.data();
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swap_chain_target.swap_chain;
        presentInfo.pImageIndices = &swap_chain_target.image_index;
        VkResult result_code = vkQueuePresentKHR(_queue, &presentInfo);
        switch(result_code)
        {
            case VK_SUCCESS:
                break;
            case VK_SUBOPTIMAL_KHR:
                return acquisition_failure_t::suboptimal;
            case VK_NOT_READY:
                return acquisition_failure_t::not_ready;
            case VK_TIMEOUT:
                return acquisition_failure_t::timeout;
            case VK_ERROR_OUT_OF_DATE_KHR:
                return acquisition_failure_t::out_of_date;
            default:
                vk_require(result_code, "queueing image for presentation");
        }
        return boost::none;   
    }

    uint32_t queue_reference_t::family_index()
    {
        return _family_index;
    }
}
