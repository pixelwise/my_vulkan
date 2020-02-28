#pragma once

#include "../command_buffer.hpp"
#include "../queue.hpp"
#include "../device_memory.hpp"
#include <glm/vec2.hpp>
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
            VkImageView output_buffer; //this is better a shared ptr
            VkExtent2D extent;
            const device_memory_t& memory;
        };
        struct render_target_t
        {
            using begin_t = std::function<render_scope_t(VkRect2D)>;
            using end_t = std::function<void(
                std::vector<queue_reference_t::wait_semaphore_info_t>,
                std::vector<VkSemaphore>
            )>;
            using rect_t = std::function<VkRect2D()>;
            begin_t begin;
            end_t end;
            size_t depth;
            bool flipped;
            rect_t rect;
            glm::vec2 ui_multiplier;
            bool enabled = true;
            render_target_t(
                begin_t in_begin,
                end_t in_end,
                VkExtent2D in_size,
                size_t in_depth,
                bool in_flipped,
                glm::vec2 ui_multiplier = {1, 1}
            )
            : render_target_t{
                in_begin,
                in_end,
                in_depth,
                in_flipped,
                [in_size]{return VkRect2D{{0, 0}, in_size};},
                ui_multiplier
            }
            {
            }
            render_target_t with_begin(begin_t new_begin) const
            {
                return {
                    new_begin,
                    end,
                    depth,
                    flipped,
                    rect,
                    ui_multiplier,
                };                
            }
            render_target_t with_callback(std::function<void()> callback) const
            {
                return {
                    begin,
                    [
                        callback,
                        end = this->end
                    ](
                        auto wait_semaphores,
                        auto signal_semaphore
                    ){
                        end(
                            std::move(wait_semaphores),
                            std::move(signal_semaphore)
                        );
                        callback();
                    },
                    depth,
                    flipped,
                    rect,
                    ui_multiplier,
                };
            }
        private:
            render_target_t(
                begin_t in_begin,
                end_t in_end,
                size_t in_depth,
                bool in_flipped,
                rect_t in_rect,
                glm::vec2 in_ui_multiplier
            )
            : begin{std::move(in_begin)}
            , end{std::move(in_end)}
            , depth{in_depth}
            , flipped{in_flipped}
            , rect{in_rect}
            , ui_multiplier{in_ui_multiplier}
            {
            }
        };
    }
}
