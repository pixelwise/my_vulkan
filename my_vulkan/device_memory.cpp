#include "device_memory.hpp"

#include <utility>
#include "utils.hpp"

namespace my_vulkan
{
    device_memory_t::device_memory_t(
        VkDevice device,
        config_t config
    )
    : _device{device}
    {
        VkMemoryAllocateInfo info{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            0,
            config.size,
            config.type_index
        };
        vk_require(
            vkAllocateMemory(
                device,
                &info,
                0,
                &_memory
            ),
            "allocating device memory"
        );
    }

    device_memory_t::device_memory_t(device_memory_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    device_memory_t& device_memory_t::operator=(
        device_memory_t&& other
    ) noexcept
    {
        cleanup();
        _memory = other._memory;
        std::swap(_device, other._device);
        return *this;
    }

    size_t device_memory_t::size() const
    {
        return _size;
    }

    void device_memory_t::set_data(const void* data, size_t size)
    {
        auto mapping = map();
        memcpy(mapping.data(), data, size);
    }

    device_memory_t::mapping_t device_memory_t::map(
        boost::optional<region_t> region,
        VkMemoryMapFlags flags
    )
    {
        return {*this, region, flags};
    }

    VkDeviceMemory device_memory_t::get()
    {
        return _memory;
    }

    device_memory_t::~device_memory_t()
    {
        cleanup();
    }

    void device_memory_t::cleanup()
    {
        if (_device)
        {
            vkFreeMemory(_device, _memory, 0);
            _device = 0;
        }
    }

    device_memory_t::mapping_t::mapping_t(
        device_memory_t& memory,
        boost::optional<region_t> optional_region,
        VkMemoryMapFlags flags
    )
    : _memory(&memory)
    {
        auto region = optional_region.value_or(region_t{0, memory.size()});
        vk_require(
            vkMapMemory(
                memory._device,
                memory._memory, 
                region.offset,
                region.size, 
                flags, 
                &_data
            ),
            "mapping memory"
        );
    }

    void* device_memory_t::mapping_t::data()
    {
        return _data;
    }

    device_memory_t::mapping_t::mapping_t(mapping_t&& other) noexcept
    {
        *this = std::move(other);
    }

    device_memory_t::mapping_t& device_memory_t::mapping_t::operator=(
        mapping_t&& other
    ) noexcept
    {
        cleanup();
        _data = other._data;
        std::swap(_memory, other._memory);
        return *this;
    }
    
    void device_memory_t::mapping_t::unmap()
    {
        cleanup();
    }

    device_memory_t::mapping_t::~mapping_t()
    {
        cleanup();
    }

    void device_memory_t::mapping_t::cleanup()
    {
        if (_memory)
        {
            vkUnmapMemory(_memory->_device, _memory->_memory);
            _memory = 0;   
        }
    }
}
