#include "buffer.hpp"

#include "utils.hpp"

namespace my_vulkan
{
    buffer_t::buffer_t(
        device_t& device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
    )
    : _device{device.get()}
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vk_require(
            vkCreateBuffer(_device, &bufferInfo, nullptr, &_buffer),
            "creating buffer"
        );
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(_device, _buffer, &memRequirements);
        _memory.reset(new device_memory_t{
            _device,
            {
                memRequirements.size,
                findMemoryType(
                    device.physical_device(),
                    memRequirements.memoryTypeBits,
                    properties
                )                
            }
        });
        vkBindBufferMemory(_device, _buffer, _memory->get(), 0);
    }

    device_memory_t* buffer_t::memory()
    {
        return _memory.get();
    }

    buffer_t::buffer_t(buffer_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    buffer_t& buffer_t::operator=(buffer_t&& other) noexcept
    {
        cleanup();
        _buffer = other._buffer;
        _memory = std::move(other._memory);
        std::swap(_device, other._device);
        return *this;
    }

    VkBuffer buffer_t::get()
    {
        return _buffer;
    }

    buffer_t::~buffer_t()
    {
        cleanup();
    }

    void buffer_t::cleanup()
    {
        if (_device)
        {
            vkDestroyBuffer(_device, _buffer, nullptr);
            _memory.reset();
            _device = 0;
        }
    }
}
