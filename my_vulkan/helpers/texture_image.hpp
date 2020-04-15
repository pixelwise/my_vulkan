#pragma once

#include "../image.hpp"
#include "../device.hpp"
#include "../buffer.hpp"
#include "../texture_sampler.hpp"

namespace my_vulkan::helpers
{
    class texture_image_t
    {
    public:
        texture_image_t(
            device_t& device,
            VkExtent2D size,
            size_t num_components,
            size_t pitch,
            std::optional<VkExternalMemoryHandleTypeFlagBits> external_handle_types = std::nullopt
        );
        VkDescriptorImageInfo descriptor();
        void upload(
            my_vulkan::command_pool_t& command_pool,
            const void* pixels,
            bool keep_buffers = true
        );
        void prepare_for_transfer(my_vulkan::command_pool_t& command_pool);
        void prepare_for_shader(my_vulkan::command_pool_t& command_pool);
        void prepare_for_transfer(command_buffer_t::scope_t& commands);
        void prepare_for_shader(command_buffer_t::scope_t& commands);
        VkExtent3D extent() const;
        VkFormat format() const;
        std::optional<device_memory_t::external_memory_info_t> external_memory_info();
    private:
        buffer_t& staging_buffer();
        device_t* _device;
        size_t _pitch;
        size_t _transfer_byte_size;
        std::unique_ptr<buffer_t> _staging_buffer;
        image_t _image;
        image_view_t _view;
        texture_sampler_t _sampler;
    };    
}
