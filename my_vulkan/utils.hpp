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
        boost::optional<uint32_t> graphics;
        boost::optional<uint32_t> present;
        bool isComplete() const;
        std::vector<uint32_t> unique_indices() const;
    };
    queue_family_indices_t find_queue_families(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    );
    boost::optional<uint32_t> find_graphics_queue(
        VkPhysicalDevice device
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
    bool has_stencil_component(VkFormat format);
    size_t bytes_per_pixel(VkFormat format);
    VkFormat find_depth_format(VkPhysicalDevice physical_device);
    VkFormat find_supported_format(
        VkPhysicalDevice physical_device,
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    );
    struct swap_chain_support_t
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    swap_chain_support_t query_swap_chain_support(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    );
    VkPhysicalDevice pick_physical_device(
        VkInstance instance,
        VkSurfaceKHR surface,
        std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME}
    );

}

