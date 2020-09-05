#include "physical_device_utils.hpp"
#include <iostream>

namespace my_vulkan
{
    std::optional<VkPhysicalDeviceIDProperties> fetch_physical_device_IDProp(
        VkPhysicalDevice device,
        PFN_vkGetPhysicalDeviceProperties2 fpGetPhysicalDeviceProperties2,
        std::optional<uint32_t> maybe_api_version
    )
    {
        if (not maybe_api_version)
        {
            VkPhysicalDeviceProperties prop;
            vkGetPhysicalDeviceProperties(device, &prop);
            maybe_api_version = prop.apiVersion;
        }
        if (fpGetPhysicalDeviceProperties2 == nullptr || *maybe_api_version < VK_MAKE_VERSION(1, 1, 0))
        {
            std::cerr << "WARNING: this device does not support GetPhysicalDeviceProperties2.\n";
            return std::nullopt;
        }
        VkPhysicalDeviceIDProperties vkPhysicalDeviceIDProperties{};
        vkPhysicalDeviceIDProperties.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        vkPhysicalDeviceIDProperties.pNext = nullptr;

        VkPhysicalDeviceProperties2 vkPhysicalDeviceProperties2 = {};
        vkPhysicalDeviceProperties2.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        vkPhysicalDeviceProperties2.pNext = &vkPhysicalDeviceIDProperties;

        fpGetPhysicalDeviceProperties2(device,
            &vkPhysicalDeviceProperties2);
        return vkPhysicalDeviceIDProperties;
    }

    vk_physical_device_info_t fetch_physical_device_properties(
        VkPhysicalDevice device, PFN_vkGetPhysicalDeviceProperties2 fpGetPhysicalDeviceProperties2
    )
    {
        vk_physical_device_info_t ret;
        vkGetPhysicalDeviceProperties(device, &ret.prop);
        if (fpGetPhysicalDeviceProperties2 != nullptr && ret.prop.apiVersion >= VK_MAKE_VERSION(1, 1, 0))
        {
            VkPhysicalDeviceIDProperties vkPhysicalDeviceIDProperties{};
            vkPhysicalDeviceIDProperties.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
            vkPhysicalDeviceIDProperties.pNext = nullptr;

            VkPhysicalDeviceProperties2 vkPhysicalDeviceProperties2 = {};
            vkPhysicalDeviceProperties2.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            vkPhysicalDeviceProperties2.pNext = &vkPhysicalDeviceIDProperties;

            fpGetPhysicalDeviceProperties2(device,
                &vkPhysicalDeviceProperties2);
            ret.id = vkPhysicalDeviceIDProperties;
            ret.prop = vkPhysicalDeviceProperties2.properties; // maybe not necessary, does the prop2 change the VkPhysicalDeviceProperties at all?
        }
        return ret;
    }

    std::vector<my_vulkan::vk_physical_device_info_t> physical_devices_info(instance_t &instance)
    {
        auto devices = instance.physical_devices();
        auto fpGetPhysicalDeviceProperties2 = instance.fetch_fpGetPhysicalDeviceProperties2();
        std::vector<my_vulkan::vk_physical_device_info_t> ret;
        for (const auto& device : devices)
        {
            ret.push_back(
                my_vulkan::fetch_physical_device_properties(device, fpGetPhysicalDeviceProperties2)
            );
        }
        return ret;
    }
}
