#include "device_memory.hpp"

#include <utility>
#include <cstring>

#include "utils.hpp"

namespace my_vulkan
{
    device_memory_t::device_memory_t(
        VkDevice device,
        config_t config
    )
    : _device{device}
    , _size{config.size}
    {
        VkMemoryAllocateInfo info{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            0,
            config.size,
            config.type_index
        };
        if (config.external_handle_type)
        {
            VkExportMemoryAllocateInfoKHR vulkanExportMemoryAllocateInfoKHR = {};
            vulkanExportMemoryAllocateInfoKHR.sType =
                VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;

            vulkanExportMemoryAllocateInfoKHR.pNext = NULL;

            vulkanExportMemoryAllocateInfoKHR.handleTypes =
                *(config.external_handle_type);
            info.pNext = &vulkanExportMemoryAllocateInfoKHR;
        }

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
        _size = other._size;
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
        std::memcpy(mapping.data(), data, size);
    }

    device_memory_t::mapping_t device_memory_t::map(
        std::optional<region_t> region,
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
        std::optional<region_t> optional_region,
        VkMemoryMapFlags flags
    )
    : _memory(memory._memory)
    , _device{memory._device}
    , _region{optional_region.value_or(region_t{0, memory.size()})}
    {
        vk_require(
            vkMapMemory(
                _device,
                _memory,
                _region.offset,
                _region.size, 
                flags, 
                &_data
            ),
            "mapping memory"
        );
    }

    void device_memory_t::mapping_t::flush()
    {
        VkMappedMemoryRange range{
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            nullptr,
            _memory,
            _region.offset,
            _region.size
        };
        vk_require(
            vkFlushMappedMemoryRanges(
                _device,
                1,
                &range
            ),
            "flushing mapped memory"
        );
    }

    void device_memory_t::mapping_t::invalidate()
    {
        VkMappedMemoryRange range{
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            0,
            _memory,
            _region.offset,
            _region.size
        };
        vk_require(
            vkInvalidateMappedMemoryRanges(
                _device,
                1,
                &range
            ),
            "flushing mapped memory"
        );
    }

    void* device_memory_t::mapping_t::data()
    {
        return _data;
    }

    device_memory_t::mapping_t::mapping_t(mapping_t&& other) noexcept
    : _memory{0}
    {
        *this = std::move(other);
    }

    device_memory_t::mapping_t& device_memory_t::mapping_t::operator=(
        mapping_t&& other
    ) noexcept
    {
        cleanup();
        _data = other._data;
        _device = other._device;
        _region = other._region;
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
            vkUnmapMemory(_device, _memory);
            _memory = 0;   
        }
    }
}
