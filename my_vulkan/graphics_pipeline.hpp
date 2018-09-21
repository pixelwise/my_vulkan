#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "descriptor_set_layout.hpp"
#include "swap_chain.hpp"

namespace my_vulkan
{
    struct vertex_layout_t
    {
        VkVertexInputBindingDescription binding;
        std::vector<VkVertexInputAttributeDescription> attributes;
    };
    struct graphics_pipeline_t
    {
        graphics_pipeline_t(
            VkDevice device,
            VkExtent2D extent,
            VkRenderPass render_pass,
            const std::vector<VkDescriptorSetLayoutBinding>& uniform_layout,
            vertex_layout_t vertex_layout,
            const std::vector<char>& vertex_shader,
            const std::vector<char>& fragment_shader        
        );
        graphics_pipeline_t(const graphics_pipeline_t&) = delete;
        graphics_pipeline_t(graphics_pipeline_t&& other) noexcept;
        graphics_pipeline_t& operator=(const graphics_pipeline_t&) = delete;
        graphics_pipeline_t& operator=(graphics_pipeline_t&& other) noexcept;
        VkPipeline get();
        VkPipelineLayout layout();
        VkDescriptorSetLayout uniform_layout();
        ~graphics_pipeline_t();
    private:
        void cleanup();
        VkDevice _device;
        descriptor_set_layout_t _uniform_layout;
        VkPipeline _pipeline;
        VkPipelineLayout _layout;
    };    
}
