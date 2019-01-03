#include "buffer.hpp"

#include "utils.hpp"

namespace my_vulkan
{
    buffer_t::buffer_t(
        device_t* device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
    )
    : _device{device}
    , _size{size}
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vk_require(
            vkCreateBuffer(_device->get(), &bufferInfo, nullptr, &_buffer),
            "creating buffer"
        );
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(_device->get(), _buffer, &memRequirements);
        _memory.reset(new device_memory_t{
            _device->get(),
            {
                memRequirements.size,
                findMemoryType(
                    device->physical_device(),
                    memRequirements.memoryTypeBits,
                    properties
                )                
            }
        });
        vkBindBufferMemory(_device->get(), _buffer, _memory->get(), 0);
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

    void buffer_t::load_data(command_pool_t& command_pool, const void* data)
    {
        buffer_t staging_buffer{
            _device,
            _size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
        staging_buffer.memory()->set_data(data, _size);
        my_vulkan::buffer_t result{
            _device,
            _size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };
        auto oneshot_scope = command_pool.begin_oneshot();
        oneshot_scope.commands().copy(staging_buffer.get(), get(), {{0, 0, _size}});
        oneshot_scope.execute_and_wait();
    }

    VkDeviceSize buffer_t::size() const
    {
        return _size;
    }

    buffer_t::~buffer_t()
    {
        cleanup();
    }

    void buffer_t::cleanup()
    {
        if (_device)
        {
            vkDestroyBuffer(_device->get(), _buffer, nullptr);
            _memory.reset();
            _device = 0;
        }
    }
}
