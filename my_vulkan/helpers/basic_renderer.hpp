#pragma once

#include "../my_vulkan.hpp"

#include <memory>
#include <stdexcept>

namespace my_vulkan
{
    struct rendering_output_config_t
    {
        device_t* device;
        VkExtent2D extent;
        VkRenderPass render_pass;
        size_t depth;
    };

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
        typedef rendering_output_config_t output_config_t;
        basic_renderer_t(
            output_config_t output_config,
            std::vector<uint8_t> vertex_shader,
            std::vector<uint8_t> fragment_shader,
            render_settings_t render_settings = {},
            bool dynamic_viewport = false
        );
        class pipeline_buffer_t
        {
        public:
            class pinned_t
            {
            public:
                pipeline_buffer_t* operator->()
                {
                    return _target;
                }
                pinned_t(pipeline_buffer_t& target)
                {
                    if (target._pinned)
                        throw std::runtime_error{"double pinned pipeline buffer"};
                    target._pinned = true;
                    _target = &target;
                }
                pinned_t(const pinned_t&) = delete;
                pinned_t& operator=(const pinned_t&) = delete;
                pinned_t(pinned_t&& other)
                : _target{0}
                {
                    *this = std::move(other);
                }
                pinned_t& operator=(pinned_t&& other)
                {
                    reset();
                    std::swap(_target, other._target);
                    return *this;
                }
                ~pinned_t()
                {
                }
            private:
                void reset()
                {
                    if (_target)
                        _target->_pinned = false;
                    _target = 0;                    
                }
                pipeline_buffer_t* _target;
            };
            pipeline_buffer_t(
                device_t* device,
                VkDescriptorSetLayout layout
            );
            void update_vertices(
                const std::vector<vertex_t>& vertices
            );
            void update_indices(
                const std::vector<uint32_t>& indices
            );
            void update_uniforms(
                vertex_uniforms_t vertex_uniforms,
                fragment_uniforms_t fragment_uniforms
            );
            void update_texture(
                size_t index,
                VkDescriptorImageInfo texture
            );
            void begin_phase(size_t i)
            {
                if (_phase == i)
                    _phase.reset();
            }
            void bind(
                command_buffer_t::scope_t& command_scope,
                VkPipelineLayout layout,
                size_t phase
            );
            bool in_use() const;
            pinned_t pin() {return pinned_t{*this};}
        private:
            device_t* _device;
            buffer_t _vertex_uniforms;
            buffer_t _fragment_uniforms;
            descriptor_pool_t _descriptor_pool;
            descriptor_set_t _descriptor_set;
            boost::optional<buffer_t> _vertices;
            boost::optional<buffer_t> _indices;
            boost::optional<size_t> _phase;
            bool _pinned = false;
        };
        void begin_phase(size_t phase)
        {
            _current_phase = phase;
            _next_buffer_index = 0;
            for (auto& buffer_ptr : _pipeline_buffers)
                buffer_ptr->begin_phase(phase);
        }
        void execute_draw(
            pipeline_buffer_t& buffer,
            command_buffer_t::scope_t& command_scope,
            size_t num_vertices,
            boost::optional<VkRect2D> target_rect = boost::none
        );
        void execute_indexed_draw(
            pipeline_buffer_t& buffer,
            command_buffer_t::scope_t& command_scope,
            size_t num_indices,
            boost::optional<VkRect2D> target_rect = boost::none
        );
        void update_render_pipeline(
            VkExtent2D extent,
            VkRenderPass render_pass
        );
        pipeline_buffer_t& buffer();
        basic_renderer_t(basic_renderer_t&) = delete;
        basic_renderer_t(basic_renderer_t&&) = default;
        basic_renderer_t& operator=(const basic_renderer_t&) = delete;
        basic_renderer_t& operator=(basic_renderer_t&&) = default;
    private:
        void bind(
            pipeline_buffer_t& buffer,
            command_buffer_t::scope_t& command_scope,
            boost::optional<VkRect2D> target_rect
        );
        static VkVertexInputBindingDescription make_vertex_bindings_description();
        static std::vector<VkVertexInputAttributeDescription> make_attribute_descriptions();
        static vertex_layout_t make_vertex_layout();
        static std::vector<VkDescriptorSetLayoutBinding> make_uniform_layout();
        device_t* _device;
        std::vector<uint8_t> _vertex_shader;
        std::vector<uint8_t> _fragment_shader;
        std::vector<VkDescriptorSetLayoutBinding> _uniform_layout;
        graphics_pipeline_t _graphics_pipeline;
        std::vector<std::unique_ptr<pipeline_buffer_t>> _pipeline_buffers;
        const size_t _depth;
        size_t _current_phase{0};
        size_t _next_buffer_index{0};
    };
}
