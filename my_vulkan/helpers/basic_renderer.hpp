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
        uint32_t subpass = 0;
        bool dynamic_viewport = true;
    };

    struct basic_renderer_shader_modules_t
    {
        shader_module_t vertex_shader;
        shader_module_t fragment_shader;
    };

    struct basic_renderer_phase_t
    {
        size_t context_id;
        size_t phase;
        basic_renderer_phase_t(size_t context_id, size_t phase)
        : context_id{context_id}
        , phase{phase}
        {}
        explicit basic_renderer_phase_t(size_t phase)
        : context_id{0}
        , phase{phase}
        {}
        bool operator==(basic_renderer_phase_t y) const
        {
            return 
                context_id == y.context_id &&
                phase == y.phase;
        }
    };

    template<
        typename in_vertex_uniforms_t,
        typename in_fragment_uniforms_t,
        // must be a fusion iterable type with glm vec members
        // or be a plain glm vec
        // hint: use BOOST_FUSION_DEFINE_STRUCT for convenience
        // std::tuple should also work
        typename in_vertex_t,
        size_t num_textures,
        size_t num_input_attachments = 0
    > class basic_renderer_t
    {
    public:
        using vertex_uniforms_t = in_vertex_uniforms_t;
        using fragment_uniforms_t = in_fragment_uniforms_t;
        using vertex_t = in_vertex_t;
        using phase_t = basic_renderer_phase_t;

        typedef rendering_output_config_t output_config_t;
        basic_renderer_t(
            output_config_t output_config,
            std::vector<uint8_t> vertex_shader,
            std::vector<uint8_t> fragment_shader,
            render_settings_t render_settings = {}
        )
        : basic_renderer_t{
            output_config,
            {
                shader_module_t{output_config.device->get(), vertex_shader},
                shader_module_t{output_config.device->get(), fragment_shader},
            },
            render_settings
        }
        {}
        basic_renderer_t(
            output_config_t output_config,
            const basic_renderer_shader_modules_t& shaders,
            render_settings_t render_settings = {}
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
                explicit pinned_t(pipeline_buffer_t& target)
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
                pinned_t& operator=(pinned_t&& other) noexcept
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
                std::shared_ptr<buffer_t> vertices
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
            void update_input_attachment(
                size_t index,
                descriptor_set_t::image_info_t image
            );
            void begin_phase(phase_t i)
            {
                if (_phase == i)
                    _phase.reset();
            }
            void bind(
                command_buffer_t::scope_t& command_scope,
                VkPipelineLayout layout,
                phase_t phase
            );
            bool in_use() const;
            pinned_t pin() {return pinned_t{*this};}
        private:
            static size_t texture_location_offset();
            device_t* _device;
            buffer_t _vertex_uniforms;
            buffer_t _fragment_uniforms;
            descriptor_pool_t _descriptor_pool;
            descriptor_set_t _descriptor_set;
            std::shared_ptr<buffer_t> _vertices;
            std::optional<buffer_t> _indices;
            std::optional<phase_t> _phase;
            bool _pinned = false;
        };
        std::shared_ptr<buffer_t> upload_vertices(
            const std::vector<vertex_t>& vertices
        );
        void begin_phase(phase_t phase)
        {
            _current_phase = phase;
            for (auto& buffer_ptr : _pipeline_buffers)
                buffer_ptr->begin_phase(phase);
        }
        void execute_draw(
            pipeline_buffer_t& buffer,
            command_buffer_t::scope_t& command_scope,
            index_range_t range,
            std::optional<VkRect2D> target_rect = std::nullopt
        );
        void execute_indexed_draw(
            pipeline_buffer_t& buffer,
            command_buffer_t::scope_t& command_scope,
            index_range_t range,
            std::optional<VkRect2D> target_rect = std::nullopt
        );
        pipeline_buffer_t& buffer();
        basic_renderer_t(const basic_renderer_t&) = delete;
        basic_renderer_t(basic_renderer_t&&) noexcept = default;
        basic_renderer_t& operator=(const basic_renderer_t&) = delete;
        basic_renderer_t& operator=(basic_renderer_t&&) noexcept = default;
    private:
        void bind(
            pipeline_buffer_t& buffer,
            command_buffer_t::scope_t& command_scope,
            std::optional<VkRect2D> target_rect
        );
        static VkVertexInputBindingDescription make_vertex_bindings_description();
        static std::vector<VkVertexInputAttributeDescription> make_attribute_descriptions();
        static vertex_layout_t make_vertex_layout();
        static std::vector<VkDescriptorSetLayoutBinding> make_uniform_layout();
        device_t* _device{nullptr};
        std::vector<VkDescriptorSetLayoutBinding> _uniform_layout;
        render_settings_t _render_settings;
        bool _dynamic_viewport{false};
        graphics_pipeline_t _graphics_pipeline;
        std::vector<std::unique_ptr<pipeline_buffer_t>> _pipeline_buffers;
        phase_t _current_phase{0, 0};
        size_t _next_buffer_index{0};
    };
}
