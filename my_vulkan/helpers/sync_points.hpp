#pragma once
#include "../queue.hpp"
#include "../vector_helpers.hpp"
#include "../fence.hpp"
namespace my_vulkan
{
    struct sync_point_refs_t
    {
        using waits_t = std::vector <my_vulkan::queue_reference_t::wait_semaphore_info_t>;
        using signals_t = std::vector <VkSemaphore>;
        using fences_t = std::vector<fence_t>;
        waits_t waits;
        signals_t signals;
//        fences_t fences;
        void extend(sync_point_refs_t o);
        void extend_waits(waits_t o);
        void extend_signals(signals_t o);
    };
}
