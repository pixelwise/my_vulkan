#include "texture_image.hpp"

namespace my_vulkan::helpers
{
    static VkFormat image_format_with_components(size_t num_components)
    {
        switch(num_components)
        {
            case 1:
                return VK_FORMAT_R8_UNORM;
            case 2:
                return VK_FORMAT_R8G8_UNORM;
            case 3:
                return VK_FORMAT_R8G8B8_UNORM;
            case 4:
                return VK_FORMAT_B8G8R8A8_UNORM;
        }
        return VK_FORMAT_UNDEFINED;
    }

    texture_image_t::texture_image_t(
        device_t& device,
        VkExtent2D size,
        uint32_t num_components,
        uint32_t pitch,
        std::optional<VkExternalMemoryHandleTypeFlags> external_handle_types
    )
    : _device{&device}
    , _num_components{num_components}
    , _pitch{pitch / num_components}
    , _transfer_byte_size{pitch * size.height}
    , _image{
        device,
        VkExtent3D{size.width, size.height, 1},
        image_format_with_components(num_components),
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_TILING_OPTIMAL,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        external_handle_types
    }
    , _view{_image.view()}
    , _sampler{device.get()}
    {
        assert(_image.memory());
    }

    buffer_t& texture_image_t::staging_buffer()
    {
        if (!_staging_buffer)
            _staging_buffer.reset(new buffer_t{
                *_device,
                _transfer_byte_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            });
        return *_staging_buffer;
    }

    void texture_image_t::upload(
        my_vulkan::command_pool_t& command_pool,
        const void* pixels,
        bool keep_buffers,
        std::optional<uint32_t> pitch
    )
    {
        auto oneshot_scope = command_pool.begin_oneshot();
        upload(oneshot_scope.commands(), pixels, pitch);
        oneshot_scope.execute_and_wait();
        if (!keep_buffers)
            _staging_buffer.reset();
    }

    void texture_image_t::upload(
        command_buffer_t::scope_t& commands,
        const void* pixels,
        std::optional<uint32_t> pitch
    )
    {
        staging_buffer().memory()->set_data(pixels, _transfer_byte_size);
        prepare_for_transfer(commands);
        auto out_pitch = _pitch;
        if (pitch)
            out_pitch = *pitch / _num_components;
        _image.copy_from(
            staging_buffer().get(),
            commands,
            out_pitch
        );
        prepare_for_shader(commands);
    }

    void texture_image_t::prepare_for_transfer(my_vulkan::command_pool_t& command_pool)
    {
        auto oneshot_scope = command_pool.begin_oneshot();
        prepare_for_transfer(oneshot_scope.commands());
        oneshot_scope.execute_and_wait();
    }

    void texture_image_t::prepare_for_shader(my_vulkan::command_pool_t& command_pool)
    {
        auto oneshot_scope = command_pool.begin_oneshot();
        prepare_for_shader(oneshot_scope.commands());
        oneshot_scope.execute_and_wait();
    }

    void texture_image_t::prepare_for_transfer(command_buffer_t::scope_t& commands)
    {
        _image.transition_layout(
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            commands
        );        
    }

    void texture_image_t::prepare_for_shader(command_buffer_t::scope_t& commands)
    {
        _image.transition_layout(
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            commands
        );
    }

    VkDescriptorImageInfo texture_image_t::descriptor()
    {
        return {
            _sampler.get(),
            _view.get(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
    }    

    VkExtent3D texture_image_t::extent() const
    {
        return _image.extent();
    }

    VkFormat texture_image_t::format() const
    {
        return _image.format();
    }

    std::optional<device_memory_t::external_memory_info_t> texture_image_t::external_memory_info(
        VkExternalMemoryHandleTypeFlagBits externalHandleType
    )
    {
        return _image.external_memory_info(externalHandleType);
    }
}
