#include "descriptor_pool.hpp"

#include "utils.hpp"

namespace my_vulkan
{
    descriptor_pool_t::descriptor_pool_t(VkDevice device, size_t size)
    : _device{device}
    {
        VkDescriptorPoolSize poolSizes[2] = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = size;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = size;
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes;
        poolInfo.maxSets = size;
        vk_require(
            vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptor_pool),
            "creating dexcriptor pool"
        );
    }

    descriptor_pool_t::descriptor_pool_t(descriptor_pool_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    descriptor_pool_t& descriptor_pool_t::operator=(descriptor_pool_t&& other) noexcept
    {
        cleanup();
        _descriptor_pool = other._descriptor_pool;
        std::swap(_device, other._device);
        return *this;
    }

    VkDescriptorPool descriptor_pool_t::get()
    {
        return _descriptor_pool;
    }

    descriptor_pool_t::~descriptor_pool_t()
    {
        cleanup();
    }
    
    void descriptor_pool_t::cleanup()
    {
        vkDestroyDescriptorPool(
            _device,
            _descriptor_pool,
            0
        );
    }
}
