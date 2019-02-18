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
            using begin_t = std::function<render_scope_t(VkRect2D)>;
            using end_t = std::function<void(
                std::vector<queue_reference_t::wait_semaphore_info_t>,
                std::vector<VkSemaphore>
            )>;
            const begin_t begin;
            const end_t end;
            const VkExtent2D size;
            const size_t depth;
            render_target_t(
                begin_t in_begin,
                end_t in_end,
                VkExtent2D in_size,
                size_t in_depth
            )
            : begin{std::move(in_begin)}
            , end{std::move(in_end)}
            , size{in_size}
            , depth{in_depth}
            , target_rect{{0, 0}, in_size}
            {
            }
            VkRect2D target_rect;
        };
    }
}
