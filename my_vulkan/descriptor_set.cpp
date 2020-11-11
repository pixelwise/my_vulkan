#include "descriptor_set.hpp"
#include "utils.hpp"

#include <stdexcept>

namespace my_vulkan
{
    descriptor_set_t::descriptor_set_t(
        VkDevice device,
        VkDescriptorPool pool,
        VkDescriptorSetLayout layout
    )
    : _device{device}
    , _descriptor_pool{pool}
    {
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;
        vk_require(
            vkAllocateDescriptorSets(_device, &allocInfo, &_descriptor_set),
            "allocating descriptor set"
        );
    }

    void descriptor_set_t::update_sampler_write(
        uint32_t binding,
        std::vector<VkSampler> samplers,
        uint32_t array_offset
    )
    {
        std::vector<VkDescriptorImageInfo> infos(samplers.size());
        for (size_t i = 0; i < samplers.size(); ++i)
            infos[i] = VkDescriptorImageInfo{samplers[i], 0, {}};
        VkWriteDescriptorSet info = {};
        info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        info.dstSet = _descriptor_set;
        info.dstBinding = binding;
        info.dstArrayElement = array_offset;
        info.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        info.descriptorCount = uint32_t(infos.size());
        info.pImageInfo = infos.data();
        vkUpdateDescriptorSets(
            _device,
            1,
            &info,
            0,
            0
        );
    }

    void descriptor_set_t::update_image_write(
        uint32_t binding,
        VkDescriptorType type,
        std::vector<image_info_t> image_infos,
        uint32_t array_offset
    )
    {
        std::vector<VkDescriptorImageInfo> infos(image_infos.size());
        for (size_t i = 0; i < image_infos.size(); ++i)
            infos[i] = VkDescriptorImageInfo{
                0, 
                image_infos[i].view, 
                image_infos[i].layout
            };
        VkWriteDescriptorSet info = {};
        info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        info.dstSet = _descriptor_set;
        info.dstBinding = binding;
        info.dstArrayElement = array_offset;
        info.descriptorType = type;
        info.descriptorCount = uint32_t(infos.size());
        info.pImageInfo = infos.data();
        vkUpdateDescriptorSets(
            _device,
            1,
            &info,
            0,
            0
        );
    }

    void descriptor_set_t::update_combined_image_sampler_write(
        uint32_t binding,
        std::vector<VkDescriptorImageInfo> image_infos,
        uint32_t array_offset
    )
    {
        VkWriteDescriptorSet info = {};
        info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        info.dstSet = _descriptor_set;
        info.dstBinding = binding;
        info.dstArrayElement = array_offset;
        info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        info.descriptorCount = uint32_t(image_infos.size());
        info.pImageInfo = image_infos.data();
        vkUpdateDescriptorSets(
            _device,
            1,
            &info,
            0,
            0
        );
    }

    void descriptor_set_t::update_buffer_write(
        uint32_t binding,
        VkDescriptorType type,
        std::vector<VkDescriptorBufferInfo> buffer_infos,
        uint32_t array_offset
    )
    {
        VkWriteDescriptorSet info = {};
        info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        info.dstSet = _descriptor_set;
        info.dstBinding = binding;
        info.dstArrayElement = array_offset;
        info.descriptorType = type;
        info.descriptorCount = uint32_t(buffer_infos.size());
        info.pBufferInfo = buffer_infos.data();
        vkUpdateDescriptorSets(
            _device,
            1,
            &info,
            0,
            0
        );
    }

    void descriptor_set_t::update_texel_write(
        uint32_t binding,
        VkDescriptorType type,
        std::vector<VkBufferView> buffer_views,
        uint32_t array_offset
    )    
    {
        VkWriteDescriptorSet info = {};
        info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        info.dstSet = _descriptor_set;
        info.dstBinding = binding;
        info.dstArrayElement = array_offset;
        info.descriptorType = type;
        info.descriptorCount = uint32_t(buffer_views.size());
        info.pTexelBufferView = buffer_views.data();
        vkUpdateDescriptorSets(
            _device,
            1,
            &info,
            0,
            0
        );
    }

    void descriptor_set_t::update_uniform_block_write(
        uint32_t binding,
        uint32_t offset,
        uint32_t size
    )
    {
#if 1
        throw std::runtime_error{
            "VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT not available"
        };
#else
        VkWriteDescriptorSet info = {};
        info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        info.dstSet = _descriptor_set;
        info.dstBinding = binding;
        info.dstArrayElement = offset;
        info.descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT;
        info.descriptorCount = size;
        vkUpdateDescriptorSets(
            _device,
            1,
            &info,
            0,
            0
        );
#endif
    }

    void descriptor_set_t::update_copy_to(
        VkDescriptorSet target,
        uint32_t source_binding,
        uint32_t source_array_offset,
        uint32_t target_binding,
        uint32_t target_array_offset,
        uint32_t count
    )
    {
        VkCopyDescriptorSet info = {};
        info.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
        info.srcSet = _descriptor_set;
        info.dstSet = target;
        info.srcBinding = source_binding;
        info.dstBinding = target_binding;
        info.srcArrayElement = source_array_offset;
        info.dstArrayElement = target_array_offset;
        info.descriptorCount = count;
        vkUpdateDescriptorSets(
            _device,
            0,
            0,
            1,
            &info
        );
    }

    descriptor_set_t::descriptor_set_t(descriptor_set_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    descriptor_set_t& descriptor_set_t::operator=(descriptor_set_t&& other) noexcept
    {
        cleanup();
        _descriptor_pool = other._descriptor_pool;
        _descriptor_set = other._descriptor_set;
        std::swap(_device, other._device);
        return *this;
    }

    VkDescriptorSet descriptor_set_t::get()
    {
        return _descriptor_set;
    }

    descriptor_set_t::~descriptor_set_t()
    {
        cleanup();
    }

    void descriptor_set_t::cleanup()
    {
        if (_device)
        {
            vkFreeDescriptorSets(
                _device,
                _descriptor_pool,
                1,
                &_descriptor_set
            );
            _descriptor_set = 0;
            _device = 0;
        }
    }
}
