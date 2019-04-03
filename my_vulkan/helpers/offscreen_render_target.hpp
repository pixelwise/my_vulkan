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
                VkFramebuffer framebuffer;
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
                        image_t&
                )>;
                slot_t(
                    VkDevice device,
                    VkRenderPass render_pass,
                    queue_reference_t& queue,
                    VkExtent2D size,
                    VkImageView color_view,
                    VkFormat depth_format,
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
                VkRenderPass _render_pass;
                image_t _depth_image;
                image_view_t _depth_view;
                framebuffer_t _framebuffer;
                std::unique_ptr<image_t> _readback_image;
                fence_t _fence;
                command_pool_t _command_pool;
                command_buffer_t _command_buffer;
                boost::optional<command_buffer_t::scope_t> _commands;
                boost::optional<device_memory_t::mapping_t> _mapping;
                begin_callback_t _begin_callback;
                end_callback_t _end_callback;
            };
        public:
            offscreen_render_target_t(
                device_t& device,
                VkRenderPass render_pass,
                VkFormat color_format,
                VkFormat depth_format,
                VkExtent2D size,
                bool need_readback = false,
                size_t depth = 2
            );
            offscreen_render_target_t(
                device_t& device,
                VkRenderPass render_pass,
                std::vector<VkImageView> color_views,
                VkFormat depth_format,
                VkExtent2D size
            );
            render_target_t render_target();
            size_t depth() const;
            VkRenderPass render_pass();
            phase_context_t begin_phase(boost::optional<VkRect2D> rect = boost::none);
            void end_phase(
                std::vector<queue_reference_t::wait_semaphore_info_t> waits = {},
                std::vector<VkSemaphore> signals = {}
            );
            boost::optional<size_t> consume_read_slot(bool flush = false);
            boost::optional<cv::Mat4b> read_bgra(bool flush = false);
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
                    VkFormat color_format
                );
            };
            VkRenderPass _render_pass;
            VkExtent2D _size;
            std::vector<color_buffer_t> _color_buffers;
            std::vector<VkDescriptorImageInfo> _textures;
            std::vector<slot_t> _slots;
            size_t _write_slot{0};
            size_t _num_slots_filled{0};
        };    
    }
}
