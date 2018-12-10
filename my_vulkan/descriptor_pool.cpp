#include "descriptor_pool.hpp"

#include "utils.hpp"

#include <boost/range/algorithm/remove_if.hpp>

namespace my_vulkan
{
    descriptor_pool_t::descriptor_pool_t(
        VkDevice device,
        std::vector<VkDescriptorPoolSize> pool_sizes,
        size_t max_num_sets
    )
    : _device{device}
    {
        auto i_end = boost::remove_if(
            pool_sizes,
            [](VkDescriptorPoolSize entry){return entry.descriptorCount == 0;}
        );
        pool_sizes.erase(i_end, pool_sizes.end());
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = uint32_t(pool_sizes.size());
        poolInfo.pPoolSizes = pool_sizes.data();
        poolInfo.maxSets = max_num_sets;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
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

    descriptor_set_t descriptor_pool_t::make_descriptor_set(VkDescriptorSetLayout layout)
    {
        return {_device, _descriptor_pool, layout};
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
        if (_device)
        {
            vkDestroyDescriptorPool(
                _device,
                _descriptor_pool,
                0
            );
            _descriptor_pool = 0;
            _device = 0;            
        }
    }
}
