#pragma once

#include "../my_vulkan.hpp"
#include "render_target.hpp"

#include <opencv2/core/core.hpp>

#include <memory>

namespace my_vulkan
{
    namespace helpers
    {

        class offscreen_render_target_t
        {
        public:
            struct phase_context_t
            {
                size_t index;
                command_buffer_t::scope_t* commands;
            };
        private:
            class slot_t
            {
            public:
                using begin_callback_t = std::function<void(
                    command_buffer_t::scope_t&
                )>;
                using end_callback_t = std::function<void(
                    command_buffer_t::scope_t&,
                    image_t*
                )>;
                slot_t(
                    VkDevice device,
                    queue_reference_t& queue,
                    VkExtent2D size,
                    VkImageView color_view,
                    bool need_readback,
                    VkPhysicalDevice physical_device,
                    begin_callback_t begin_callback = 0,
                    end_callback_t end_callback = 0
                );
                phase_context_t begin(size_t index, VkRect2D rect);
                void finish(
                    std::vector<queue_reference_t::wait_semaphore_info_t> waits,
                    std::vector<VkSemaphore> signals
                );
                cv::Mat4b read_bgra();
            private:
                queue_reference_t* _queue;
                std::unique_ptr<image_t> _readback_image;
                fence_t _fence;
                command_pool_t _command_pool;
                command_buffer_t _command_buffer;
                std::optional<command_buffer_t::scope_t> _commands;
                std::optional<device_memory_t::mapping_t> _mapping;
                begin_callback_t _begin_callback;
                end_callback_t _end_callback;
            };
        public:
            offscreen_render_target_t(
                device_t& device,
                VkFormat color_format,
                VkExtent2D size,
                bool need_readback = false,
                size_t depth = 2,
                std::optional<VkExternalMemoryHandleTypeFlags> external_handle_type = std::nullopt
            );
            offscreen_render_target_t(
                //this does not support export memory,
                // but we keep the external handle type
                // to throw an error when activated.
                device_t& device,
                std::vector<VkImageView> color_views,
                VkExtent2D size,
                std::optional<VkExternalMemoryHandleTypeFlags> external_handle_type = std::nullopt
            );
            // target need to hold a set of refs of waits, signals and fences
            // it can be done by passing getters into render target
            // then in the render target use getter(phase) to
            // get semaphores and fences after begin_phase
            // we won't know the phase before begin() get called
            // thus the getter need to be in render_target_t
            // to automatically get the sync points and pass them to end_phase
            // we have to hold the getter and the phase in render_target_t
            // we can actually create render_finish and render_start semaphore
            // member as the swap chain
            struct sync_points_t
            {
                std::vector<queue_reference_t::wait_semaphore_info_t> waits;
                std::vector<VkSemaphore> signals;
//                std::vector<fence_t> fences;
            };
            typedef std::function<sync_points_t(size_t)> sync_points_getter_t;
            render_target_t render_target(sync_points_getter_t getter);
            size_t depth() const;
            phase_context_t begin_phase(std::optional<VkRect2D> rect = std::nullopt);
            void end_phase(
                std::vector<queue_reference_t::wait_semaphore_info_t> waits = {},
                std::vector<VkSemaphore> signals = {}
            );
            std::optional<size_t> consume_read_slot(bool flush = false);
            std::optional<cv::Mat4b> read_bgra(bool flush = false);
            VkDescriptorImageInfo texture(size_t phase);
            VkExtent2D size();
        private:
            struct color_buffer_t
            {
                image_t image;
                image_view_t view;
                texture_sampler_t sampler;
                color_buffer_t(
                    VkDevice device,
                    VkPhysicalDevice physical_device,
                    VkExtent2D size,
                    VkFormat color_format,
                    std::optional<VkExternalMemoryHandleTypeFlags> external_handle_type
                );
            };
            VkExtent2D _size;
            std::vector<color_buffer_t> _color_buffers;
            std::vector<VkDescriptorImageInfo> _textures;
            std::vector<slot_t> _slots;
            size_t _write_slot{0};
            size_t _num_slots_filled{0};
        };    
    }
}
