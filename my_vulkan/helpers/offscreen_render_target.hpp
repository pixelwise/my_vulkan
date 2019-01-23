#pragma once

#include "../my_vulkan.hpp"

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
                phase_context_t begin(size_t index);
                void finish();
                cv::Mat4b read_bgra();
            private:
                queue_reference_t* _queue;
                image_t _color_image;
                image_view_t _color_view;
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
                render_pass_t& render_pass,
                VkPhysicalDevice physical_device,
                queue_reference_t& queue,
                VkExtent2D size,
                bool need_readback = false
            );
            size_t depth() const;
            VkRenderPass render_pass();
            phase_context_t begin_phase();
            void finish_phase();
            boost::optional<cv::Mat4b> read_bgra(bool flush = false);
        private:
            VkRenderPass _render_pass;
            std::vector<slot_t> _slots;
            size_t _write_slot{0};
            size_t _num_slots_filled{0};
        };    
    }
}
