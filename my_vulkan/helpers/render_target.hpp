#pragma once

#include "../command_buffer.hpp"
#include "../queue.hpp"
#include <functional>
#include <stdexcept>

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
            begin_t begin;
            end_t end;
            VkExtent2D size;
            size_t depth;
            bool flipped;
            VkRect2D target_rect;
            render_target_t(
                begin_t in_begin,
                end_t in_end,
                VkExtent2D in_size,
                size_t in_depth,
                bool in_flipped
            )
            : render_target_t{
                in_begin,
                in_end,
                in_size,
                in_depth,
                in_flipped,
                {{0, 0}, in_size},
            }
            {
            }
            render_target_t with_target_rect(VkRect2D rect) const
            {
                if (
                    rect.offset.x < 0 ||
                    rect.offset.y < 0 ||
                    rect.extent.width + rect.offset.x > size.width ||
                    rect.extent.height + rect.offset.y > size.height
                )
                    throw std::runtime_error{"target rect not within bounds"};
                return {
                    begin,
                    end,
                    size,
                    depth,
                    flipped,
                    rect
                };
            }
        private:
            render_target_t(
                begin_t in_begin,
                end_t in_end,
                VkExtent2D in_size,
                size_t in_depth,
                bool in_flipped,
                VkRect2D in_target_rect
            )
            : begin{std::move(in_begin)}
            , end{std::move(in_end)}
            , size{in_size}
            , depth{in_depth}
            , flipped{in_flipped}
            , target_rect{in_target_rect}
            {
            }
        };
    }
}
