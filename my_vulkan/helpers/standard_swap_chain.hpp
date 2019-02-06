#pragma once

#include <vector>

#include <boost/optional.hpp>

#include <vulkan/vulkan.h>

#include "../device.hpp"
#include "../utils.hpp"
#include "../command_pool.hpp"
#include "../command_buffer.hpp"
#include "../swap_chain.hpp"
#include "../image_view.hpp"
#include "../framebuffer.hpp"
#include "../render_pass.hpp"
#include "../fence.hpp"
#include "../semaphore.hpp"
#include "../queue.hpp"

namespace my_vulkan
{
    namespace helpers
    {
        class standard_swap_chain_t : public swap_chain_t
        {
            struct frame_sync_points_t
            {
                semaphore_t image_available;
                semaphore_t render_finished;
                fence_t in_flight;
            };
            struct pipeline_resources_t
            {
                image_view_t image_view;
                framebuffer_t framebuffer;
                command_buffer_t command_buffer;
            };
        public:
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
                boost::optional<acquisition_failure_t> finish();
            private:
                standard_swap_chain_t* _parent;
                frame_sync_points_t* _sync;
                uint32_t _phase;
                command_buffer_t::scope_t _commands;
            };
            struct acquisition_outcome_t
            {
                boost::optional<working_set_t> working_set;
                boost::optional<acquisition_failure_t> failure;
            };
            standard_swap_chain_t(
                device_t& logical_device,
                VkSurfaceKHR surface,
                queue_family_indices_t queue_indices,
                VkExtent2D desired_extent
            );
            VkRenderPass render_pass();
            size_t depth() const;
            std::vector<VkFramebuffer> framebuffers();
            acquisition_outcome_t acquire(
                VkRect2D rect,
                std::vector<VkClearValue> clear_values = {}
            );
            command_pool_t& command_pool();
        private:
            queue_reference_t* _graphics_queue;
            queue_reference_t* _present_queue;
            image_t _depth_image;
            image_view_t _depth_view;
            render_pass_t _render_pass;
            command_pool_t _command_pool;
            std::vector<pipeline_resources_t> _pipeline_resources;
            std::vector<frame_sync_points_t> _frame_sync_points;
            size_t _current_frame{0};
        };
    }
}
