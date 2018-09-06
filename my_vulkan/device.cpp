#include "device.hpp"

namespace my_vulkan
{
    device_t::device_t(
        VkPhysicalDevice physical_device,
        queue_family_indices_t queue_indices,
        std::vector<const char*> validation_layers,
        std::vector<const char*> device_extensions        
    )
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (int queueFamily : queue_indices.unique_indices())
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        createInfo.ppEnabledExtensionNames = device_extensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        createInfo.ppEnabledLayerNames = validation_layers.data();
        vk_require(
            vkCreateDevice(physical_device, &createInfo, nullptr, &_device),
            "create logical device"
        );
        vkGetDeviceQueue(_device, queue_indices.graphics, 0, &_graphicsQueue);
        vkGetDeviceQueue(_device, queue_indices.present, 0, &_presentQueue);      
    }
    device_t::~device_t()
    {
        vkDestroyDevice(_device, 0);
    }
    VkDevice device_t::get()
    {
        return _device;
    }
    VkQueue device_t::graphics_queue()
    {
        return _graphicsQueue;
    }
    VkQueue device_t::present_queue()
    {
        return _presentQueue;
    }
}
