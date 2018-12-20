#include "device.hpp"
#include "utils.hpp"

#include <boost/range/algorithm/find.hpp>

namespace my_vulkan
{
    VkDevice make_device(
        VkPhysicalDevice physical_device,
        std::vector<queue_request_t> queue_requests,
        std::vector<const char*> validation_layers,
        std::vector<const char*> device_extensions        
    );

    device_t::device_t(
        VkPhysicalDevice physical_device,
        queue_family_indices_t queue_indices,
        std::vector<const char*> validation_layers,
        std::vector<const char*> device_extensions        
    )
    : _physical_device{physical_device}
    , _device{make_device(
        physical_device,
        queue_indices.request_one_each(),
        validation_layers,
        device_extensions
    )}
    , _queue_indices{queue_indices}
    {
        auto unique_queue_indices = _queue_indices.unique_indices();
        for (auto i : unique_queue_indices)
        {
            VkQueue queue;
            vkGetDeviceQueue(get(), i, 0, &queue);
            _queues.push_back(queue_reference_t{queue, i});
        }
        if (_queue_indices.graphics)
        {
            auto index = boost::find(unique_queue_indices, *_queue_indices.graphics);
            _graphics_queue = &_queues[index - unique_queue_indices.begin()];
        }
        if (_queue_indices.present)
        {
            auto index = boost::find(unique_queue_indices, *_queue_indices.present);
            _present_queue = &_queues[index - unique_queue_indices.begin()];
        }
        if (_queue_indices.transfer)
        {
            auto index = boost::find(unique_queue_indices, *_queue_indices.transfer);
            _transfer_queue = &_queues[index - unique_queue_indices.begin()];
        }
    }

    VkPhysicalDevice device_t::physical_device() const
    {
        return _physical_device;
    }

    VkDevice device_t::get() const
    {
        return _device;
    }

    queue_family_indices_t device_t::queue_indices()
    {
        return _queue_indices;
    }

    queue_reference_t& device_t::graphics_queue()
    {
        if (!_graphics_queue)
            throw std::runtime_error{"no graphics queue"};
        return *_graphics_queue;
    }

    queue_reference_t& device_t::present_queue()
    {
        if (!_present_queue)
            throw std::runtime_error{"no present queue"};
        return *_present_queue;
    }

    queue_reference_t& device_t::transfer_queue()
    {
        if (!_transfer_queue)
            throw std::runtime_error{"no transfer queue"};
        return *_transfer_queue;
    }

    VkDevice make_device(
        VkPhysicalDevice physical_device,
        std::vector<queue_request_t> queue_requests,
        std::vector<const char*> validation_layers,
        std::vector<const char*> device_extensions        
    )
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (auto queue_request : queue_requests)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queue_request.index;
            queueCreateInfo.queueCount = queue_request.count;
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

    device_t::~device_t()
    {
        if (auto device = get())
            vkDestroyDevice(device, 0);
    }

    void device_t::wait_idle()
    {
        vk_require(
            vkDeviceWaitIdle(get()),
            "waiting for device to be idle"
        );
    }
}
