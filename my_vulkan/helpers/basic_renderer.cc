#include "basic_renderer.hpp"

#include "to_std140.hpp"

#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/counting_range.hpp>

#include <glm/glm.hpp>

inline VkFormat vertex_format_with_components(float, size_t num_components)
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
inline VkFormat vertex_format_for_attribute(attribute_t)
{
    return vertex_format_with_components(
        typename attribute_t::value_type{},
        attribute_t{}.length()
    );
}

template<typename vertex_t>
inline std::vector<VkVertexInputAttributeDescription>
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

static size_t round_to_multiple_of_16(size_t n)
{
    return 16 * ((n + 15) / 16);
}

namespace my_vulkan
{
    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    > basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::basic_renderer_t(
        output_config_t output_config,
        const basic_renderer_shader_modules_t& shaders,
        render_settings_t render_settings
    )
    : _device{output_config.device}
    , _uniform_layout{make_uniform_layout()}
    , _render_settings{render_settings}
    , _dynamic_viewport{output_config.dynamic_viewport}
    , _graphics_pipeline{
        output_config.device->get(),
        output_config.extent,
        output_config.render_pass,
        output_config.subpass,
        _uniform_layout,
        make_vertex_layout(),
        shaders.vertex_shader,
        shaders.fragment_shader,
        _render_settings,
        _dynamic_viewport
    }
    {
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    std::vector<VkDescriptorSetLayoutBinding>
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::make_uniform_layout()
    {
        std::vector<VkDescriptorSetLayoutBinding> result;
        uint32_t texture_location = 0;
        static_assert(
            boost::fusion::result_of::empty<vertex_uniforms_t>::value ==
            boost::fusion::result_of::empty<fragment_uniforms_t>::value,
            "none or both of vertex and fragment uniforms must empty"
        );
        if (!boost::fusion::result_of::empty<vertex_uniforms_t>::value)
        {
            result.push_back({
                texture_location++,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                1,
                VK_SHADER_STAGE_VERTEX_BIT,
                0
            });
        }
        if (!boost::fusion::result_of::empty<fragment_uniforms_t>::value)
        {
            result.push_back({
                texture_location++,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                1,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0
            });
        }
        for (auto i : boost::counting_range<uint32_t>(0, num_textures))
        {
            result.push_back({
                texture_location + i,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                1,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0
            });
        }
        for (auto i : boost::counting_range<uint32_t>(0, num_input_attachments))
        {
            result.push_back({
                uint32_t(texture_location + num_textures + i),
                VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
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
        size_t num_textures,
        size_t num_input_attachments
    >
    vertex_layout_t
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
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
        size_t num_textures,
        size_t num_input_attachments
    >
    VkVertexInputBindingDescription
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
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
        size_t num_textures,
        size_t num_input_attachments
    >
    std::vector<VkVertexInputAttributeDescription>
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::make_attribute_descriptions()
    {
        return make_vertex_attribute_descriptions(vertex_t{});
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::pipeline_buffer_t::pipeline_buffer_t(
        device_t* device,
        VkDescriptorSetLayout layout
    )
    : _device{device}
    , _vertex_uniforms{
        *_device,
        round_to_multiple_of_16(to_std140(vertex_uniforms_t{}).data.size()),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  
    }
    , _fragment_uniforms{
        *_device,
        round_to_multiple_of_16(to_std140(fragment_uniforms_t{}).data.size()),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  
    }
    , _descriptor_pool{
        _device->get(),
        std::vector<VkDescriptorPoolSize>{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(2)},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(num_textures)},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, uint32_t(num_input_attachments)},
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
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::pipeline_buffer_t::update_vertices(
        std::shared_ptr<buffer_t> vertices
    )
    {
        _vertices = vertices;
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::pipeline_buffer_t::update_vertices(
        const std::vector<vertex_t>& vertices
    )
    {
        size_t data_size = sizeof(vertex_t) * vertices.size();
        if (!_vertices || _vertices->size() < data_size)
        {
            _vertices = std::make_shared<buffer_t>(
                *_device,
                data_size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
        _vertices->memory()->set_data(
            vertices.data(),
            data_size
        );
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    std::shared_ptr<buffer_t>
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::upload_vertices(
        const std::vector<vertex_t>& vertices
    )
    {
        size_t data_size = sizeof(vertex_t) * vertices.size();
        auto vertex_buffer = std::make_shared<buffer_t>(
            *_device,
            data_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        vertex_buffer->memory()->set_data(
            vertices.data(),
            data_size
        );
        return vertex_buffer;
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
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
        size_t num_textures,
        size_t num_input_attachments
    >
    bool
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::pipeline_buffer_t::in_use() const
    {
        return !!_phase || _pinned;
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
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
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::pipeline_buffer_t::update_texture(
        size_t index,
        VkDescriptorImageInfo texture
    )
    {
        if (index >= num_textures)
            throw std::runtime_error{"invalid texture index"};
        _descriptor_set.update_combined_image_sampler_write(
            uint32_t(texture_location_offset() + index),
            {texture}
        );
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::pipeline_buffer_t::update_input_attachment(
        size_t index,
        descriptor_set_t::image_info_t image
    )
    {
        if (index >= num_input_attachments)
            throw std::runtime_error{"invalid input attachment index"};
        _descriptor_set.update_image_write(
            uint32_t(texture_location_offset() + num_textures + index),
            VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            {image}
        );
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    size_t
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::pipeline_buffer_t::texture_location_offset()
    {
        size_t offset = 0;
        if (!boost::fusion::result_of::empty<vertex_uniforms_t>::value)
            ++offset;
        if (!boost::fusion::result_of::empty<fragment_uniforms_t>::value)
            ++offset;
        return offset;        
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::pipeline_buffer_t::bind(
        command_buffer_t::scope_t& command_scope,
        VkPipelineLayout layout,
        phase_t phase
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
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::execute_draw(
        pipeline_buffer_t& buffer,
        command_buffer_t::scope_t& command_scope,
        index_range_t range,
        std::optional<VkRect2D> target_rect
    )
    {
        bind(buffer, command_scope, target_rect);
        command_scope.draw(range);
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::execute_indexed_draw(
        pipeline_buffer_t& buffer,
        command_buffer_t::scope_t& command_scope,
        index_range_t range,
        std::optional<VkRect2D> target_rect
    )
    {
        bind(buffer, command_scope, target_rect);
        command_scope.draw_indexed(range);
    }

    template<
        typename vertex_uniforms_t,
        typename fragment_uniforms_t,
        typename vertex_t,
        size_t num_textures,
        size_t num_input_attachments
    >
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::bind(
        pipeline_buffer_t& buffer,
        command_buffer_t::scope_t& command_scope,
        std::optional<VkRect2D> target_rect
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
        size_t num_textures,
        size_t num_input_attachments
    >
    auto 
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures,
        num_input_attachments
    >::buffer() -> pipeline_buffer_t&
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
