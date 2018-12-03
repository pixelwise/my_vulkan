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
            device_reference_t device;
            VkExtent2D extent;
            VkRenderPass render_pass;
            size_t depth;
        };
        basic_renderer_t(
            output_config_t output_config,
            std::vector<uint8_t> vertex_shader,
            std::vector<uint8_t> fragment_shader,
            render_settings_t render_settings = {}
        );
        class pipeline_buffer_t
        {
        public:
            pipeline_buffer_t(
                device_reference_t device,
                VkDescriptorPool descriptor_pool,
                VkDescriptorSetLayout layout
            );
            void update_vertices(
                const std::vector<vertex_t>& vertices
            );
            void update_uniforms(
                vertex_uniforms_t vertex_uniforms,
                fragment_uniforms_t fragment_uniforms
            );
            void update_texture(
                size_t index,
                VkDescriptorImageInfo texture
            );
            void bind(
                command_buffer_t::scope_t& command_scope,
                VkPipelineLayout layout
            );
        private:
            device_reference_t _device;
            buffer_t _vertex_uniforms;
            buffer_t _fragment_uniforms;
            descriptor_set_t _descriptor_set;
            boost::optional<buffer_t> _vertices;
        };
        void execute_draw(
            pipeline_buffer_t& buffer,
            command_buffer_t::scope_t& command_scope,
            size_t num_vertices
        );
        void update_render_pipeline(
            VkExtent2D extent,
            VkRenderPass render_pass
        );
        pipeline_buffer_t& buffer(size_t phase);
        basic_renderer_t(basic_renderer_t&) = delete;
        basic_renderer_t(basic_renderer_t&&) = default;
        basic_renderer_t& operator=(const basic_renderer_t&) = delete;
        basic_renderer_t& operator=(basic_renderer_t&&) = default;
    private:
        static VkVertexInputBindingDescription make_vertex_bindings_description();
        static std::vector<VkVertexInputAttributeDescription> make_attribute_descriptions();
        static vertex_layout_t make_vertex_layout();
        static std::vector<VkDescriptorSetLayoutBinding> make_uniform_layout();
        device_reference_t _device;
        std::vector<uint8_t> _vertex_shader;
        std::vector<uint8_t> _fragment_shader;
        std::vector<VkDescriptorSetLayoutBinding> _uniform_layout;
        graphics_pipeline_t _graphics_pipeline;
        std::vector<descriptor_pool_t> _descriptor_pools;
        std::vector<std::vector<pipeline_buffer_t>> _pipeline_buffers;
        const size_t _depth;
        size_t _current_phase{0};
        size_t _next_buffer_index{0};
    };
}
