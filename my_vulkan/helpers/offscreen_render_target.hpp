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
                slot_t(
                    VkDevice device,
                    VkRenderPass render_pass,
                    queue_reference_t& queue,
                    VkExtent2D size,
                    VkFormat color_format,
                    VkFormat depth_format,
                    bool need_readback,
                    VkPhysicalDevice physical_device
                );
                phase_context_t begin(size_t index, VkRect2D rect);
                void finish(
                    std::vector<queue_reference_t::wait_semaphore_info_t> waits,
                    std::vector<VkSemaphore> signals
                );
                cv::Mat4b read_bgra();
                VkDescriptorImageInfo texture();
            private:
                queue_reference_t* _queue;
                VkRenderPass _render_pass;
                image_t _color_image;
                image_view_t _color_view;
                texture_sampler_t _sampler;
                image_t _depth_image;
                image_view_t _depth_view;
                framebuffer_t _framebuffer;
                std::unique_ptr<image_t> _readback_image;
                fence_t _fence;
                command_pool_t _command_pool;
                command_buffer_t _command_buffer;
                boost::optional<command_buffer_t::scope_t> _commands;
                boost::optional<device_memory_t::mapping_t> _mapping;
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
            render_target_t render_target();
            size_t depth() const;
            VkRenderPass render_pass();
            phase_context_t begin_phase(boost::optional<VkRect2D> rect = boost::none);
            void finish_phase(
                std::vector<queue_reference_t::wait_semaphore_info_t> waits = {},
                std::vector<VkSemaphore> signals = {}
            );
            boost::optional<cv::Mat4b> read_bgra(bool flush = false);
            VkDescriptorImageInfo texture(size_t phase);
            VkExtent2D size();
        private:
            VkRenderPass _render_pass;
            VkExtent2D _size;
            std::vector<slot_t> _slots;
            size_t _write_slot{0};
            size_t _num_slots_filled{0};
        };    
    }
}
