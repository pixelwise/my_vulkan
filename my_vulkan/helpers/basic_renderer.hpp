#pragma once

#include "../my_vulkan.hpp"

namespace my_vulkan
{
    template<
        typename in_vertex_uniforms_t,
        typename in_fragment_uniforms_t,
        // must be a fusion iterable type with glm vec members
        // or be a plain glm vec
        // hint: use BOOST_FUSION_DEFINE_STRUCT for convenience
        // std::tuple should also work
        typename in_vertex_t,
        size_t num_textures
    > class basic_renderer_t
    {
    public:
        using vertex_uniforms_t = in_vertex_uniforms_t;
        using fragment_uniforms_t = in_fragment_uniforms_t;
        using vertex_t = in_vertex_t;
        struct output_config_t
        {
            my_vulkan::device_reference_t device;
            VkExtent2D extent;
            VkRenderPass render_pass;
            size_t depth;
        };
        basic_renderer_t(
            output_config_t output_config,
            const std::vector<uint8_t>& vertex_shader,
            const std::vector<uint8_t>& fragment_shader                
        );
        void update_vertices(
            size_t phase,
            const std::vector<vertex_t>& vertices
        );
        void update_uniforms(
            size_t phase,
            vertex_uniforms_t vertex_uniforms,
            fragment_uniforms_t fragment_uniforms
        );
        void update_texture(
            size_t phase,
            size_t index,
            VkDescriptorImageInfo texture
        );
        void execute_draw(
            size_t phase,
            my_vulkan::command_buffer_t::scope_t& command_scope,
            size_t num_vertices
        );
        basic_renderer_t(basic_renderer_t&) = delete;
        basic_renderer_t(basic_renderer_t&&) = default;
        basic_renderer_t& operator=(const basic_renderer_t&) = delete;
        basic_renderer_t& operator=(basic_renderer_t&&) = default;
    private:
        static VkVertexInputBindingDescription make_vertex_bindings_description();
        static std::vector<VkVertexInputAttributeDescription> make_attribute_descriptions();
        static my_vulkan::vertex_layout_t make_vertex_layout();
        static std::vector<VkDescriptorSetLayoutBinding> make_uniform_layout();
        struct pipeline_buffer_t
        {
            my_vulkan::buffer_t vertex_uniforms;
            my_vulkan::buffer_t fragment_uniforms;
            my_vulkan::descriptor_set_t descriptor_set;
            boost::optional<my_vulkan::buffer_t> vertices;
        };
        my_vulkan::device_reference_t _device;
        std::vector<VkDescriptorSetLayoutBinding> _uniform_layout;
        my_vulkan::graphics_pipeline_t _graphics_pipeline;
        my_vulkan::descriptor_pool_t _descriptor_pool;
        std::vector<pipeline_buffer_t> _pipeline_buffers;
    };
}
