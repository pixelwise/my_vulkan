#pragma once

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <array>
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
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        std::optional<uint32_t> transfer;
        bool isComplete() const;
        bool isComplete_offscreen() const;
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
    std::optional<uint32_t> find_graphics_queue(
        VkPhysicalDevice device
    );
    std::optional<uint32_t> find_transfer_queue(
        VkPhysicalDevice device
    );
    std::optional<uint32_t> find_queue(
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
        const std::vector<const char*>& deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME}
    );

    VkPhysicalDevice pick_physical_device(
        uint32_t device_index,
        VkInstance instance,
        VkSurfaceKHR surface,
        const std::vector<const char*>& deviceExtensions
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

    typedef std::array<uint8_t, VK_UUID_SIZE> vk_uuid_t;

    template <typename T>
    inline std::optional<VkFlags> to_vkflag(const std::vector<T>& enums)
    {
        if (enums.empty())
            return std::nullopt;
        VkExternalMemoryHandleTypeFlags ret{0};
        for (auto & type : enums)
        {
            ret |= type;
        }
        return ret;
    }

    template <typename T>
    inline std::vector<T> from_vkflag(std::optional<VkFlags> flag, const std::vector<T> &supported_bits)
    {
        std::vector<T> ret;
        if(flag)
        {
            for (auto const & type: supported_bits)
            {
                if(type & *flag)
                {
                    ret.push_back(type);
                }
            }
        }
        return ret;
    }

    std::vector<VkExternalMemoryHandleTypeFlagBits> vk_ext_mem_handle_types_from_vkflag(std::optional<VkExternalMemoryHandleTypeFlags> flag);

    std::vector<VkExternalSemaphoreHandleTypeFlagBits> vk_ext_semaphore_handle_types_from_vkflag(std::optional<VkExternalSemaphoreHandleTypeFlags> flag);
}

