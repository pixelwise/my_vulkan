#include "device.hpp"
#include "utils.hpp"

namespace my_vulkan
{
    device_reference_t::device_reference_t(
        VkPhysicalDevice physical_device,
        VkDevice device
    )
    : _physical_device{physical_device}
    , _device{device}
    {}

    VkPhysicalDevice device_reference_t::physical_device() const
    {
        return _physical_device;
    }

    VkDevice device_reference_t::get() const
    {
        return _device;
    }

    void device_reference_t::clear()
    {
        _device = 0;
    }

    VkDevice make_device(
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
        VkDevice result;
        vk_require(
            vkCreateDevice(physical_device, &createInfo, nullptr, &result),
            "create logical device"
        );
        return result;
    }

    device_t::device_t(
        VkPhysicalDevice physical_device,
        queue_family_indices_t queue_indices,
        std::vector<const char*> validation_layers,
        std::vector<const char*> device_extensions        
    )
    : device_reference_t{
        physical_device,
        make_device(physical_device, queue_indices, validation_layers, device_extensions)
    }
    {
        if (queue_indices.graphics)
            vkGetDeviceQueue(get(), *queue_indices.graphics, 0, &_graphicsQueue);
        if (queue_indices.present)
            vkGetDeviceQueue(get(), *queue_indices.present, 0, &_presentQueue);      
    }

    device_t::~device_t()
    {
        if (auto device = get())
            vkDestroyDevice(device, 0);
    }

    queue_reference_t device_t::graphics_queue()
    {
        return _graphicsQueue;
    }

    queue_reference_t device_t::present_queue()
    {
        return _presentQueue;
    }

    void device_t::wait_idle()
    {
        vk_require(
            vkDeviceWaitIdle(get()),
            "waiting for device to be idle"
        );
    }
}
