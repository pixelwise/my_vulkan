#include "basic_renderer.hpp"

#include <boost/fusion/include/for_each.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/counting_range.hpp>

#include <glm/glm.hpp>

#include <memory>
#include <stdexcept>

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
        attribute_t::length()
    );
}

template<typename vertex_t>
static std::vector<VkVertexInputAttributeDescription>
make_vertex_attribute_descriptions(vertex_t prototype)
{
    std::vector<VkVertexInputAttributeDescription> result;
    size_t i = 0;
    boost::fusion::for_each(
        prototype,
        [&](const auto& attribute)
        {
            result.emplace_back(
                0,
                i++,
                vertex_format_for_attribute(attribute),
                static_cast<size_t>(std::addressof(attribute)) -
                static_cast<size_t>(std::addressof(prototype))
            );
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
        std::vector<uint8_t> fragment_shader                
    )
    : _device{output_config.device}
    , _vertex_shader{std::move(vertex_shader)}
    , _fragment_shader{std::move(fragment_shader)}
    , _uniform_layout{make_uniform_layout()}
    , _graphics_pipeline{
        output_config.device.get(),
        output_config.extent,
        output_config.render_pass,
        _uniform_layout,
        make_vertex_layout(),
        _vertex_shader,
        _fragment_shader,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
    }
    , _descriptor_pool{
        output_config.device.get(),
        output_config.depth
    }
    , _pipeline_buffers{
        materialize(boost::counting_range<size_t>(0, output_config.depth) |
        boost::adaptors::transformed([&](auto i){
            return pipeline_buffer_t{
                {
                    output_config.device,
                    sizeof(vertex_uniforms_t),
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  
                },
                {
                    output_config.device,
                    sizeof(fragment_uniforms_t),
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  
                },
                _descriptor_pool.make_descriptor_set(_graphics_pipeline.uniform_layout())
            };
        }))
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
    >::update_render_pipeline(
        VkExtent2D extent,
        VkRenderPass render_pass
    )
    {
        _graphics_pipeline = {
            _device.get(),
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
        result.push_back({
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            0
        });
        result.push_back({
            1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0
        });
        for (auto i : boost::counting_range<uint32_t>(0, num_textures))
        {
            result.push_back({
                i + 2,
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
    my_vulkan::vertex_layout_t
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
    void
    basic_renderer_t<
        vertex_uniforms_t,
        fragment_uniforms_t,
        vertex_t,
        num_textures
    >::update_vertices(
        size_t phase,
        const std::vector<vertex_t>& vertices
    )
    {
        auto& buffer = _pipeline_buffers[phase];
        size_t data_size = sizeof(vertex_t) * vertices.size();
        my_vulkan::buffer_t vertex_buffer{
            _device,
            data_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
        vertex_buffer.memory()->set_data(
            vertices.data(),
            data_size
        );
        buffer.vertices = std::move(vertex_buffer);
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
    >::update_uniforms(
        size_t phase,
        vertex_uniforms_t vertex_uniforms,
        fragment_uniforms_t fragment_uniforms
    )
    {
        auto& buffer = _pipeline_buffers[phase];
        buffer.vertex_uniforms.memory()->set_data(
            &vertex_uniforms,
            sizeof(vertex_uniforms)
        );
        buffer.descriptor_set.update_buffer_write(
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            {{buffer.vertex_uniforms.get(), 0, sizeof(vertex_uniforms_t)}}
        );
        buffer.fragment_uniforms.memory()->set_data(
            &fragment_uniforms,
            sizeof(fragment_uniforms)
        );
        buffer.descriptor_set.update_buffer_write(
            1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            {{buffer.fragment_uniforms.get(), 0, sizeof(fragment_uniforms_t)}}
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
    >::update_texture(
        size_t phase,
        size_t index,
        VkDescriptorImageInfo texture
    )
    {
        if (index >= num_textures)
            throw std::runtime_error{"invalid texture index"};
        auto& buffer = _pipeline_buffers[phase];
        buffer.descriptor_set.update_combined_image_sampler_write(
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
    >::execute_draw(
        size_t phase,
        my_vulkan::command_buffer_t::scope_t& command_scope,
        size_t num_vertices
    )
    {
        auto& buffer = _pipeline_buffers[phase];
        command_scope.bind_pipeline(
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _graphics_pipeline.get()
        );
        command_scope.bind_vertex_buffers(
            {{buffer.vertices->get(), 0}}
        );            
        command_scope.bind_descriptor_set(
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            _graphics_pipeline.layout(),
            {buffer.descriptor_set.get()}
        );
        command_scope.draw({0, uint32_t(num_vertices)});            
    }
}
