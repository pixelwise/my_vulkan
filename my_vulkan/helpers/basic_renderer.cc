#include "basic_renderer.hpp"

#include "to_std140.hpp"

#include "utils/range_utils.hpp"

#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/counting_range.hpp>

#include <glm/glm.hpp>

static VkFormat vertex_format_with_components(float, size_t num_components)
{
    switch(num_components)
    {
        case 1:
            return VK_FORMAT_R32_SFLOAT;
        case 2:
            return VK_FORMAT_R32G32_SFLOAT;
        case 3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case 4:
            return VK_FORMAT_R32G32B32_SFLOAT;
    }
    return VK_FORMAT_UNDEFINED;
}

template<typename attribute_t>
static VkFormat vertex_format_for_attribute(attribute_t)
{
    return vertex_format_with_components(
        typename attribute_t::value_type{},
        attribute_t{}.length()
    );
}

template<typename vertex_t>
static std::vector<VkVertexInputAttributeDescription>
make_vertex_attribute_descriptions(vertex_t prototype)
{
    std::vector<VkVertexInputAttributeDescription> result;
    uint32_t i = 0;
    boost::fusion::for_each(
        prototype,
        [&](const auto& attribute)
        {
            result.push_back(VkVertexInputAttributeDescription{
                i++, // assign to locations, not bindings
                0,
                vertex_format_for_attribute(attribute),
                uint32_t(
                    reinterpret_cast<const char*>(std::addressof(attribute)) -
                    reinterpret_cast<const char*>(std::addressof(prototype))
                )
            });
        }
    );
    return result;
}

namespace glm
{
    template<typename component_t>
    static std::vector<VkVertexInputAttributeDescription>
    make_vertex_attribute_descriptions(tvec1<component_t> attribute)
    {
        return {{
            0, 0,
            vertex_format_for_attribute(attribute),
            0
        }};
    }

    template<typename component_t>
    static std::vector<VkVertexInputAttributeDescription>
    make_vertex_attribute_descriptions(tvec2<component_t> attribute)
    {
        return {{
            0, 0,
            vertex_format_for_attribute(attribute),
            0
        }};
    }

    template<typename component_t>
    static std::vector<VkVertexInputAttributeDescription>
    make_vertex_attribute_descriptions(tvec3<component_t> attribute)
    {
        return {{
            0, 0,
            vertex_format_for_attribute(attribute),
            0
        }};
    }

    template<typename component_t>
    static std::vector<VkVertexInputAttributeDescription>
    make_vertex_attribute_descriptions(tvec4<component_t> attribute)
    {
        return {{
            0, 0,
            vertex_format_for_attribute(attribute),
            0
        }};
    }
}

namespace my_vulkan
{
    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    > basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::basic_renderer_t(
        output_config_t output_config,
        std::vector<uint8_t> vertex_shader,
        std::vector<uint8_t> fragment_shader,
        render_settings_t render_settings,
        bool dynamic_viewport
    )
    : _device{output_config.device}
    , _vertex_shader{std::move(vertex_shader)}
    , _fragment_shader{std::move(fragment_shader)}
    , _uniform_layout{make_uniform_layout()}
    , _graphics_pipeline{
        output_config.device->get(),
        output_config.extent,
        output_config.render_pass,
        _uniform_layout,
        make_vertex_layout(),
        _vertex_shader,
        _fragment_shader,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        render_settings,
        dynamic_viewport
    }
    , _depth{output_config.depth}
    {
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::update_render_pipeline(
        VkExtent2D extent,
        VkRenderPass render_pass
    )
    {
        _graphics_pipeline = {
            _device->get(),
            extent,
            render_pass,
            _uniform_layout,
            make_vertex_layout(),
            _vertex_shader,
            _fragment_shader,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
        };
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    std::vector<VkDescriptorSetLayoutBinding> basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::make_uniform_layout()
    {
        std::vector<VkDescriptorSetLayoutBinding> result;
        uint32_t next_location = 0;
        static_assert(
            boost::fusion::result_of::empty<vertex_uniforms_t>::value ==
            boost::fusion::result_of::empty<fragment_uniforms_t>::value,
            "none or both of vertex and fragment uniforms must empty"
        );
        if (!boost::fusion::result_of::empty<vertex_uniforms_t>::value)
        {
            result.push_back({
                next_location++,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                1,
                VK_SHADER_STAGE_VERTEX_BIT,
                0
            });
        }
        if (!boost::fusion::result_of::empty<fragment_uniforms_t>::value)
        {
            result.push_back({
                next_location++,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                1,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0
            });
        }
        for (auto i : boost::counting_range<uint32_t>(0, num_textures))
        {
            result.push_back({
                next_location++,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                1,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0
            });
        }
        return result;
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    vertex_layout_t
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::make_vertex_layout()
    {
        return {
            make_vertex_bindings_description(),
            make_attribute_descriptions()
        };
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    VkVertexInputBindingDescription
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::make_vertex_bindings_description()
    {
        VkVertexInputBindingDescription result = {};
        result.binding = 0;
        result.stride = sizeof(vertex_t);
        result.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return result;
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    std::vector<VkVertexInputAttributeDescription>
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::make_attribute_descriptions()
    {
        return make_vertex_attribute_descriptions(vertex_t{});
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::pipeline_buffer_t::pipeline_buffer_t(
        device_t* device,
        VkDescriptorSetLayout layout
    )
    : _device{device}
    , _vertex_uniforms{
        *_device,
        to_std140(vertex_uniforms_t{}).data.size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  
    }
    , _fragment_uniforms{
        *_device,
        to_std140(fragment_uniforms_t{}).data.size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  
    }
    , _descriptor_pool{
        _device->get(),
        std::vector<VkDescriptorPoolSize>{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(2)},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(num_textures)}
        },
        1        
    }
    , _descriptor_set{
        _device->get(),
        _descriptor_pool.get(),
        layout
    }
    {   
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::pipeline_buffer_t::update_vertices(
        const std::vector<vertex_t>& vertices
    )
    {
        size_t data_size = sizeof(vertex_t) * vertices.size();
        buffer_t vertex_buffer{
            *_device,
            data_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
        vertex_buffer.memory()->set_data(
            vertices.data(),
            data_size
        );
        _vertices = std::move(vertex_buffer);
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::pipeline_buffer_t::update_indices(
        const std::vector<uint32_t> &indices
    )
    {
        size_t data_size = sizeof(uint32_t) * indices.size();
        buffer_t index_buffer{
            *_device,
            data_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
        index_buffer.memory()->set_data(
            indices.data(),
            data_size
        );
        _indices = std::move(index_buffer);
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    bool
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::pipeline_buffer_t::in_use() const
    {
        return !!_phase || _pinned;
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::pipeline_buffer_t::update_uniforms(
        vertex_uniforms_t vertex_uniforms,
        fragment_uniforms_t fragment_uniforms
    )
    {
        auto vertex_data = to_std140(vertex_uniforms).data;
        uint32_t next_location = 0;
        if (!vertex_data.empty())
        {
            _vertex_uniforms.memory()->set_data(
                vertex_data.data(),
                vertex_data.size()
            );
            _descriptor_set.update_buffer_write(
                next_location++,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                {{_vertex_uniforms.get(), 0, vertex_data.size()}}
            );            
        }
        auto fragment_data = to_std140(fragment_uniforms).data;
        if (!fragment_data.empty())
        {
            _fragment_uniforms.memory()->set_data(
                fragment_data.data(),
                fragment_data.size()
            );
            _descriptor_set.update_buffer_write(
                next_location++,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                {{_fragment_uniforms.get(), 0, fragment_data.size()}}
            );
        }
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::pipeline_buffer_t::update_texture(
        size_t index,
        VkDescriptorImageInfo texture
    )
    {
        if (index >= num_textures)
            throw std::runtime_error{"invalid texture index"};
        _descriptor_set.update_combined_image_sampler_write(
            2 + index,
            {texture}
        );
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::pipeline_buffer_t::bind(
        command_buffer_t::scope_t& command_scope,
        VkPipelineLayout layout,
        size_t phase
    )
    {
        _phase = phase;
        command_scope.bind_vertex_buffers(
            {{_vertices->get(), 0}}
        );
        if (_indices)
            command_scope.bind_index_buffer(
                _indices->get(),
                VK_INDEX_TYPE_UINT32
            );
        command_scope.bind_descriptor_set(
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            layout,
            {_descriptor_set.get()}
        );        
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::execute_draw(
        pipeline_buffer_t& buffer,
        command_buffer_t::scope_t& command_scope,
        size_t num_vertices,
        boost::optional<VkRect2D> target_rect
    )
    {
        bind(buffer, command_scope, target_rect);
        command_scope.draw({0, uint32_t(num_vertices)});            
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::execute_indexed_draw(
        pipeline_buffer_t& buffer,
        command_buffer_t::scope_t& command_scope,
        size_t num_indices,
        boost::optional<VkRect2D> target_rect
    )
    {
        bind(buffer, command_scope, target_rect);
        command_scope.draw_indexed(
            {0, uint32_t(num_indices)}
        );
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::bind(
        pipeline_buffer_t& buffer,
        command_buffer_t::scope_t& command_scope,
        boost::optional<VkRect2D> target_rect
    )
    {
        command_scope.bind_pipeline(
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _graphics_pipeline.get(),
            target_rect
        );
        buffer.bind(
            command_scope,
            _graphics_pipeline.layout(),
            _current_phase
        );
    }

    // have a buffer pool
    // hand out buffers as needed
    // when executing draw, mark buffer as "used in phase i"
    // when beginning a phase mark all buffers "in use" in that phase as not in use

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures
    >
    typename basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::pipeline_buffer_t&
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::buffer()
    {
        for (auto& buffer_ptr : _pipeline_buffers)
            if (!buffer_ptr->in_use())
                return *buffer_ptr;
         _pipeline_buffers.push_back(
            std::make_unique<pipeline_buffer_t>(
                _device,
                _graphics_pipeline.uniform_layout()
            )
        );
        return *_pipeline_buffers.back();
    }
}
