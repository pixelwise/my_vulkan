#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace my_vulkan
{
    void vk_require(VkResult result, const char* description);
    struct queue_family_indices_t
    {
        int graphics = -1;
        int present = -1;
        bool isComplete() const;
        std::vector<int> unique_indices() const;
    };
    queue_family_indices_t findQueueFamilies(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    );
    uint32_t findMemoryType(
        VkPhysicalDevice physical_device,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties
    );
}
