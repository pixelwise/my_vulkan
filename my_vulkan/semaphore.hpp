#pragma once

#include <vulkan/vulkan.h>
#include <optional>

namespace my_vulkan
{
    struct semaphore_t
    {
        explicit semaphore_t(
            VkDevice device,
            VkSemaphoreCreateFlags flags = 0,
            std::optional<VkExternalSemaphoreHandleTypeFlags> external_handle_type = std::nullopt
        );
        semaphore_t(const semaphore_t&) = delete;
        semaphore_t& operator=(const semaphore_t&) = delete;
        semaphore_t(semaphore_t&& other) noexcept;
        semaphore_t& operator=(semaphore_t&& other) noexcept;
        VkSemaphore get();
        ~semaphore_t();
        std::optional<int> get_external_handle(VkExternalSemaphoreHandleTypeFlagBits externalHandleType) const;
    private:
        void cleanup();
        VkDevice _device;
        PFN_vkGetSemaphoreFdKHR _fpGetSemaphoreFdKHR;
        VkSemaphore _semaphore;
    };
}
