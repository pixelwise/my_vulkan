#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "utils.hpp"

namespace my_vulkan
{
    struct command_buffer_t
    {
        struct scope_t
        {
            scope_t(
                VkCommandBuffer command_buffer,
                VkCommandBufferUsageFlags flags
            );
            scope_t(const scope_t&) = delete;
            scope_t(scope_t&& other) noexcept;
            scope_t& operator=(const scope_t&) = delete;
            scope_t& operator=(scope_t&& other) noexcept;

            struct buffer_binding_t
            {
                VkBuffer buffer;
                VkDeviceSize offset;
            };

            void begin_render_pass(
                VkRenderPass renderPass,
                VkFramebuffer framebuffer,
                VkRect2D render_area,
                std::vector<VkClearValue> clear_values,
                VkSubpassContents contents
            );
            void bind_pipeline(
                VkPipelineBindPoint bind_point,
                VkPipeline pipeline
            );
            void bind_vertex_buffers(
                std::vector<buffer_binding_t> bindings,
                uint32_t offset = 0
            );
            void bind_index_buffer(
                VkBuffer buffer,
                VkIndexType = VK_INDEX_TYPE_UINT16,
                size_t offset = 0
            );
            void bind_descriptor_set(
                VkPipelineBindPoint bind_point,
                VkPipelineLayout    layout,
                std::vector<VkDescriptorSet> descriptors,
                uint32_t offset = 0,
                std::vector<uint32_t> dynamic_offset = {}
            );
            void draw_indexed(
                index_range_t index_range,
                uint32_t vertex_offset = 0,
                index_range_t instance_range = {0, 1}
            );
            void end_render_pass();

            void pipeline_barrier(
                VkPipelineStageFlags src_stage_mask,
                VkPipelineStageFlags dst_stage_mask,
                VkDependencyFlags dependency_flags,
                std::vector<VkMemoryBarrier> memory_barriers,
                std::vector<VkBufferMemoryBarrier> buffer_barriers,
                std::vector<VkImageMemoryBarrier> image_barriers
            );

            void copy(
                VkBuffer src,
                VkBuffer dst,
                std::vector<VkBufferCopy> operations
            );
            void copy(
                VkBuffer src,
                VkImage dst,
                VkImageLayout dst_layout,
                std::vector<VkBufferImageCopy> operations
            );

            void end();
            ~scope_t();
        private:
            VkCommandBuffer _command_buffer{0};
        };
 
        command_buffer_t(
            VkDevice device,
            VkCommandPool command_pool,
            VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
        );
        command_buffer_t(const command_buffer_t&) = delete;
        command_buffer_t(command_buffer_t&& other) noexcept;
        command_buffer_t& operator=(const command_buffer_t&) = delete;
        command_buffer_t& operator=(command_buffer_t&& other) noexcept;
        ~command_buffer_t();
        VkCommandBuffer get();
        scope_t begin(VkCommandBufferUsageFlags flags);

    private:
        void cleanup();
        VkDevice _device;
        VkCommandPool _command_pool;
        VkCommandBuffer _command_buffer;
    };
}