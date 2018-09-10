#pragma once

#include <vulkan/vulkan.h>
#include <vector>
namespace my_vulkan
{
    struct descriptor_set_t
    {
        descriptor_set_t(
            VkDevice device,
            VkDescriptorPool pool,
            VkDescriptorSetLayout layout
        );
        descriptor_set_t(const descriptor_set_t&) = delete;
        descriptor_set_t(descriptor_set_t&& other) noexcept;
        descriptor_set_t& operator=(const descriptor_set_t&) = delete;
        descriptor_set_t& operator=(descriptor_set_t&& other) noexcept;
        ~descriptor_set_t();
        struct image_info_t{
            VkImageView view;
            VkImageLayout layout;
        };
        void update_sampler_write(
            uint32_t binding,
            std::vector<VkSampler> samplers,
            uint32_t array_offset = 0
        );
        void update_combined_image_sampler_write(
            uint32_t binding,
            std::vector<VkDescriptorImageInfo> infos,
            uint32_t array_offset = 0
        );
        /*
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
            VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
        */
        void update_image_write(
            uint32_t binding,
            VkDescriptorType type,
            std::vector<image_info_t> image_infos,
            uint32_t array_offset = 0
        );
        /*
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
        */
        void update_buffer_write(
            uint32_t binding,
            VkDescriptorType type,
            std::vector<VkDescriptorBufferInfo> buffer_infos,
            uint32_t array_offset = 0
        );
        /*
            VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
            VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
        */
        void update_texel_write(
            uint32_t binding,
            VkDescriptorType type,
            std::vector<VkBufferView> buffer_views,
            uint32_t array_offset = 0
        );
        void update_uniform_block_write(
            uint32_t binding,
            uint32_t offset,
            uint32_t size
        );
        void update_copy_to(
            VkDescriptorSet target,
            uint32_t source_binding,
            uint32_t source_array_offset,
            uint32_t target_binding,
            uint32_t target_array_offset,
            uint32_t count
        );
        VkDescriptorSet get();
    private:
        void cleanup();
        VkDevice _device;
        VkDescriptorPool _descriptor_pool;
        VkDescriptorSet _descriptor_set;
    };
}
