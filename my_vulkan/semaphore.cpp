#include <iostream>
#include "semaphore.hpp"

#include "utils.hpp"
#include "device.hpp"

namespace my_vulkan
{
    semaphore_t::semaphore_t(
        VkDevice device,
        VkSemaphoreCreateFlags flags,
        std::optional<VkExternalSemaphoreHandleTypeFlags> external_handle_types
    )
    : _device{device}
    {
        VkSemaphoreCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.flags = flags;
        if (external_handle_types)
        {
            VkExportSemaphoreCreateInfoKHR vulkanExportSemaphoreCreateInfo = {};
            vulkanExportSemaphoreCreateInfo.sType =
                VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;

            vulkanExportSemaphoreCreateInfo.pNext = NULL;
            vulkanExportSemaphoreCreateInfo.handleTypes =
                *external_handle_types;

            info.pNext = &vulkanExportSemaphoreCreateInfo;
            _fpGetSemaphoreFdKHR = device_t::get_proc<PFN_vkGetSemaphoreFdKHR>(_device, "vkGetSemaphoreFdKHR");
        }
        vk_require(
            vkCreateSemaphore(device, &info, nullptr, &_semaphore), 
            "create semaphore"
        );
        for (auto const &type: vk_ext_semaphore_handle_types_from_vkflag(external_handle_types))
        {
            record_external_handle(type);
        }
    }

    semaphore_t::semaphore_t(semaphore_t&& other) noexcept
    : _device{nullptr}
    {
        *this = std::move(other);
    }

    semaphore_t& semaphore_t::operator=(semaphore_t&& other) noexcept
    {
        cleanup();
        _semaphore = other._semaphore;
        std::swap(_device, other._device);
        _fpGetSemaphoreFdKHR = other._fpGetSemaphoreFdKHR;
        std::swap(_external_handles, other._external_handles);
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
            for( auto & [key, val] : _external_handles)
            {
                close(val);
            }
            _external_handles.clear();
            vkDestroySemaphore(_device, _semaphore, 0);
            _device = nullptr;
            _fpGetSemaphoreFdKHR = nullptr;
            _semaphore = nullptr;
        }
    }

    void semaphore_t::record_external_handle(VkExternalSemaphoreHandleTypeFlagBits externalHandleType)
    {
        auto search = _external_handles.find(externalHandleType);
        if (search != _external_handles.end())
        {
            close(search->second);
        }

        auto maybe_fd = create_ext_fd(externalHandleType);
        if (!maybe_fd)
        {
            return;
        }
        _external_handles[externalHandleType] = {
            maybe_fd.value()
        };
    }

    std::optional<int> semaphore_t::create_ext_fd(VkExternalSemaphoreHandleTypeFlagBits externalHandleType) const
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

    std::optional<int> semaphore_t::get_external_handle(VkExternalSemaphoreHandleTypeFlagBits externalHandleType) const
    {
        auto search = _external_handles.find(externalHandleType);
        if (search == _external_handles.end())
        {
            return std::nullopt;
        }
        else
        {
            return search->second;
        }
    }
}
