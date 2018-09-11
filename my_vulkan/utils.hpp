#pragma once

#include <vulkan/vulkan.h>
#include <boost/optional.hpp>
#include <vector>

namespace my_vulkan
{
    enum class acquisition_failure_t {
        not_ready,
        timeout,
        out_of_date,
        suboptimal
    };
    void vk_require(VkResult result, const char* description);
    struct queue_family_indices_t
    {
        boost::optional<uint32_t> graphics = -1;
        boost::optional<uint32_t> present = -1;
        bool isComplete() const;
        std::vector<uint32_t> unique_indices() const;
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
    struct index_range_t
    {
        uint32_t offset, count;
    };
}

