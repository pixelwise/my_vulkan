#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "descriptor_set_layout.hpp"
#include "swap_chain.hpp"
#include "shader_module.hpp"

namespace my_vulkan
{
    struct vertex_layout_t
    {
        VkVertexInputBindingDescription binding;
        std::vector<VkVertexInputAttributeDescription> attributes;
    };
    enum class blending_t
    {
        none,
        alpha,
        inverse_alpha,
        add
    };
    struct render_settings_t
    {
        bool depth_test = false;
        blending_t blending = blending_t::none;
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // culling
    };
    struct graphics_pipeline_t
    {
        graphics_pipeline_t(
            VkDevice device,
            VkExtent2D extent,
            VkRenderPass render_pass,
            const std::vector<VkDescriptorSetLayoutBinding>& uniform_layout,
            vertex_layout_t vertex_layout,
            const shader_module_t& vertex_shader,
            const shader_module_t& fragment_shader,
            render_settings_t settings = {},
            bool dynamic_viewport = false
        );
        graphics_pipeline_t(
            VkDevice device,
            VkExtent2D extent,
            VkRenderPass render_pass,
            const std::vector<VkDescriptorSetLayoutBinding>& uniform_layout,
            vertex_layout_t vertex_layout,
            const std::vector<uint8_t>& vertex_shader,
            const std::vector<uint8_t>& fragment_shader,
            render_settings_t settings = {},
            bool dynamic_viewport = false
        );
        graphics_pipeline_t(const graphics_pipeline_t&) = delete;
        graphics_pipeline_t(graphics_pipeline_t&& other) noexcept;
        graphics_pipeline_t& operator=(const graphics_pipeline_t&) = delete;
        graphics_pipeline_t& operator=(graphics_pipeline_t&& other) noexcept;
        VkPipeline get();
        VkPipelineLayout layout();
        VkDevice device();
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
