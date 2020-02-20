#include "semaphore.hpp"

#include "utils.hpp"
#include "device.hpp"

namespace my_vulkan
{
    semaphore_t::semaphore_t(
        VkDevice device,
        VkSemaphoreCreateFlags flags,
        std::optional<VkExternalSemaphoreHandleTypeFlags> external_handle_type
    )
    : _device{device}
    {
        VkSemaphoreCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.flags = flags;
        if (external_handle_type)
        {
            VkExportSemaphoreCreateInfoKHR vulkanExportSemaphoreCreateInfo = {};
            vulkanExportSemaphoreCreateInfo.sType =
                VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;

            vulkanExportSemaphoreCreateInfo.pNext = NULL;
            vulkanExportSemaphoreCreateInfo.handleTypes =
                *external_handle_type;

            info.pNext = &vulkanExportSemaphoreCreateInfo;
            _fpGetSemaphoreFdKHR = device_t::get_proc<PFN_vkGetSemaphoreFdKHR>(_device, "vkGetSemaphoreFdKHR");
        }
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
        _fpGetSemaphoreFdKHR = other._fpGetSemaphoreFdKHR;
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
            _fpGetSemaphoreFdKHR = nullptr;
            _semaphore = nullptr;
        }
    }

    std::optional<int> semaphore_t::get_external_handle(VkExternalSemaphoreHandleTypeFlagBits externalHandleType) const
    {
        if (! _fpGetSemaphoreFdKHR)
            return std::nullopt;
        int fd;
        VkSemaphoreGetFdInfoKHR vulkanSemaphoreGetFdInfoKHR = {};
        vulkanSemaphoreGetFdInfoKHR.sType =
            VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
        vulkanSemaphoreGetFdInfoKHR.pNext = NULL;
        vulkanSemaphoreGetFdInfoKHR.semaphore = _semaphore;
        vulkanSemaphoreGetFdInfoKHR.handleType = externalHandleType;

        _fpGetSemaphoreFdKHR(_device, &vulkanSemaphoreGetFdInfoKHR, &fd);

        return fd;
    }
}
