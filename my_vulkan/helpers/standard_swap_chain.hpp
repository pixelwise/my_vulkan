#pragma once

#include <vector>

#include <optional>

#include <vulkan/vulkan.h>

#include "../device.hpp"
#include "../utils.hpp"
#include "../command_pool.hpp"
#include "../command_buffer.hpp"
#include "../swap_chain.hpp"
#include "../image_view.hpp"
#include "../fence.hpp"
#include "../semaphore.hpp"
#include "../queue.hpp"
#include "render_target.hpp"

namespace my_vulkan
{
    namespace helpers
    {
        class standard_swap_chain_t
        {
            struct frame_sync_points_t
            {
                semaphore_t image_available;
                semaphore_t render_finished;
                fence_t in_flight;
            };
        public:
            struct pipeline_resources_t
            {
                image_view_t image_view;
                command_buffer_t command_buffer;
            };
            typedef swap_chain_t base_t;
            struct working_set_t
            {
                working_set_t(
                    standard_swap_chain_t& parent,
                    uint32_t phase,
                    frame_sync_points_t& sync
                );
                uint32_t phase() const;
                command_buffer_t::scope_t& commands();
                std::optional<acquisition_failure_t> finish(
                    std::vector<queue_reference_t::wait_semaphore_info_t> wait_semaphores = {},
                    std::vector<VkSemaphore> signal_semaphores = {}
                );
            private:
                standard_swap_chain_t* _parent;
                frame_sync_points_t* _sync;
                uint32_t _phase;
                command_buffer_t::scope_t _commands;
            };
            struct acquisition_outcome_t
            {
                std::optional<working_set_t> working_set;
                std::optional<acquisition_failure_t> failure;
            };
            standard_swap_chain_t(
                device_t& logical_device,
                VkSurfaceKHR surface,
                VkExtent2D desired_extent
            );
            size_t depth() const;
            acquisition_outcome_t acquire();
            command_pool_t& command_pool();
            void wait_for_idle();
            render_target_t render_target();
            VkFormat color_format() const;
            VkExtent2D extent() const;
            void update(VkExtent2D new_extent);
            const std::vector<pipeline_resources_t> & pipeline_resources() const;
        private:
            device_t* _device;
            VkSurfaceKHR _surface;
            queue_family_indices_t _queue_indices;
            std::unique_ptr<swap_chain_t> _swap_chain;
            queue_reference_t* _graphics_queue;
            queue_reference_t* _present_queue;
            command_pool_t _command_pool;
            std::vector<pipeline_resources_t> _pipeline_resources;
            std::vector<frame_sync_points_t> _frame_sync_points;
            size_t _current_frame{0};
        };
    }
}
