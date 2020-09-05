#pragma once
#include <vulkan/vulkan.h>
#include "instance.hpp"
#include "utils.hpp"

namespace my_vulkan
{
    struct vk_physical_device_info_t{
        VkPhysicalDeviceProperties prop;
        std::optional<VkPhysicalDeviceIDProperties> id = std::nullopt;
    };


    vk_physical_device_info_t fetch_physical_device_properties(
        VkPhysicalDevice device,
        PFN_vkGetPhysicalDeviceProperties2 fpGetPhysicalDeviceProperties2
    );

    std::optional<VkPhysicalDeviceIDProperties> fetch_physical_device_IDProp(
        VkPhysicalDevice device,
        PFN_vkGetPhysicalDeviceProperties2 fpGetPhysicalDeviceProperties2,
        std::optional<uint32_t> maybe_api_version
    );
    std::vector<my_vulkan::vk_physical_device_info_t> physical_devices_info(my_vulkan::instance_t & instance);
}
