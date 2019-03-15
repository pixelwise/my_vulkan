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
    const char* to_string(acquisition_failure_t failure);
    void vk_require(VkResult result, const char* description);
    struct queue_request_t
    {
        uint32_t index;
        uint32_t count;
    };
    struct queue_family_indices_t
    {
        boost::optional<uint32_t> graphics;
        boost::optional<uint32_t> present;
        boost::optional<uint32_t> transfer;
        bool isComplete() const;
        std::vector<uint32_t> unique_indices() const;
        std::vector<queue_request_t> request_one_each()
        {
            std::vector<queue_request_t> result;
            for (auto i : unique_indices())
                result.push_back({i, 1});
            return result;
        }
    };
    queue_family_indices_t find_queue_families(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    );
    boost::optional<uint32_t> find_graphics_queue(
        VkPhysicalDevice device
    );
    boost::optional<uint32_t> find_transfer_queue(
        VkPhysicalDevice device
    );
    boost::optional<uint32_t> find_queue(
        VkPhysicalDevice device,
        VkQueueFlags queue_type
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
    VkFormat uchar_format_with_components(size_t n);
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
    VkBool32 error_throw_callback(
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objectType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     messageCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMessage,
        void*                                       pUserData);
}

