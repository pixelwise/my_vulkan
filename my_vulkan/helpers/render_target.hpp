#pragma once

#include "../command_buffer.hpp"
#include "../queue.hpp"
#include <functional>

namespace my_vulkan
{
    namespace helpers
    {
        struct render_scope_t
        {
            command_buffer_t::scope_t* commands;
            size_t phase;
        };
        struct render_target_t
        {
            std::function<render_scope_t(VkRect2D)> begin;
            std::function<void(
                std::vector<queue_reference_t::wait_semaphore_info_t>,
                std::vector<VkSemaphore>
            )> end;
            VkExtent2D size;
            size_t depth;
        };
    }
}
