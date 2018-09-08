#include "utils.hpp"

#include <iostream>
#include <sstream>
#include <set>

namespace my_vulkan
{
    void vk_require(VkResult result, const char* description)
    {
        if (result != VK_SUCCESS)
        {
            std::stringstream os;
            os << description << ": " << result;
            throw std::runtime_error{os.str()};
        }
        //std::cout << description << ": OK" << std::endl;
    }

    bool queue_family_indices_t::isComplete() const
    {
        return graphics && present;
    }

    std::vector<uint32_t> queue_family_indices_t::unique_indices() const
    {
        auto index_set = std::set<uint32_t>{*graphics, *present};
        return {index_set.begin(), index_set.end()};
    }

    queue_family_indices_t findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        queue_family_indices_t indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (queueFamily.queueCount > 0 && presentSupport) {
                indices.present = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    uint32_t findMemoryType(
        VkPhysicalDevice physical_device,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties
    )
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if (
                (typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties
            )
                return i;
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }
}
