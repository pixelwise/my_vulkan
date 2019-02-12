#include "semaphore.hpp"

#include "utils.hpp"

namespace my_vulkan
{
    semaphore_t::semaphore_t(
        VkDevice device,
        VkSemaphoreCreateFlags flags
    )
    : _device{device}
    {
        VkSemaphoreCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.flags = flags;
        vk_require(
            vkCreateSemaphore(device, &info, nullptr, &_semaphore), 
            "create semaphore"
        );
    }

    semaphore_t::semaphore_t(semaphore_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    semaphore_t& semaphore_t::operator=(semaphore_t&& other) noexcept
    {
        cleanup();
        _semaphore = other._semaphore;
        std::swap(_device, other._device);
        return *this;
    }

    VkSemaphore semaphore_t::get()
    {
        return _semaphore;
    }

    semaphore_t::~semaphore_t()
    {
        cleanup();
    }

    void semaphore_t::cleanup()
    {
        if (_device)
        {
            vkDestroySemaphore(_device, _semaphore, 0);
            _device = 0;
        }
    }
}