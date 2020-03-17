#pragma once

#include <vulkan/vulkan.h>
#include <optional>
#include <map>
#include "device.hpp"
namespace my_vulkan
{
    struct semaphore_t
    {
        explicit semaphore_t(
            device_t& device,
            VkSemaphoreCreateFlags flags = 0,
            std::optional<VkExternalSemaphoreHandleTypeFlags> external_handle_types = std::nullopt
        );

        explicit semaphore_t(
            VkDevice device,
            VkSemaphoreCreateFlags flags = 0,
            std::optional<VkExternalSemaphoreHandleTypeFlags> external_handle_types = std::nullopt,
            PFN_vkGetSemaphoreFdKHR fpGetSemaphoreFdKHR = nullptr
        );
        semaphore_t(const semaphore_t&) = delete;
        semaphore_t& operator=(const semaphore_t&) = delete;
        semaphore_t(semaphore_t&& other) noexcept;
        semaphore_t& operator=(semaphore_t&& other) noexcept;
        VkSemaphore get();
        ~semaphore_t();
        std::optional<int> get_external_handle(VkExternalSemaphoreHandleTypeFlagBits externalHandleType) const;
        void record_external_handle(VkExternalSemaphoreHandleTypeFlagBits externalHandleType);
    private:
        void cleanup();
        VkDevice _device;
        PFN_vkGetSemaphoreFdKHR _fpGetSemaphoreFdKHR;
        VkSemaphore _semaphore;
        std::map<VkExternalSemaphoreHandleTypeFlagBits, int> _external_handles;
        std::optional<int> create_ext_fd(VkExternalSemaphoreHandleTypeFlagBits externalHandleType) const;
    };
}
