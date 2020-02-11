#include "device.hpp"
#include "utils.hpp"

#include <boost/range/algorithm/find.hpp>
#include <iostream>
#include <boost/format.hpp>

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
        const instance_t& instance,
        queue_family_indices_t queue_indices,
        std::vector<const char*> validation_layers,
        std::vector<const char*> device_extensions
    )
    : device_t{
        physical_device,
        queue_indices,
        std::move(validation_layers),
        std::move(device_extensions)
    }
    {
        _fpGetPhysicalDeviceProperties2 = fetch_fpGetPhysicalDeviceProperties2(instance);
        fetch_physical_device_ID();
    }
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
    , _fpGetPhysicalDeviceProperties2{nullptr}
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

    void device_t::fetch_physical_device_ID()
    {
        if (_fpGetPhysicalDeviceProperties2 == nullptr)
        {
            std::cerr << "WARNING: this device does not support GetPhysicalDeviceProperties2.\n";
            return;
        }
        VkPhysicalDeviceIDProperties vkPhysicalDeviceIDProperties{};
        vkPhysicalDeviceIDProperties.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        vkPhysicalDeviceIDProperties.pNext = NULL;

        VkPhysicalDeviceProperties2 vkPhysicalDeviceProperties2 = {};
        vkPhysicalDeviceProperties2.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        vkPhysicalDeviceProperties2.pNext = &vkPhysicalDeviceIDProperties;

        _fpGetPhysicalDeviceProperties2(_physical_device,
            &vkPhysicalDeviceProperties2);
        _maybe_vkPhysicalDeviceIDProperties = vkPhysicalDeviceIDProperties;
    }

    std::optional<VkPhysicalDeviceIDProperties> device_t::physcial_device_id_properties()
    {
        return _maybe_vkPhysicalDeviceIDProperties;
    }

    std::optional<vk_uuid_t> device_t::physical_device_uuid()
    {
        if (_maybe_vkPhysicalDeviceIDProperties)
        {
            auto & device_id = (*_maybe_vkPhysicalDeviceIDProperties);
            vk_uuid_t ret;
            std::copy(
                std::begin(device_id.deviceUUID),
                std::end(device_id.deviceUUID),
                ret.begin()
            );
            return ret;
        }
        else
        {
            std::cerr << "WARNING: this device does not support GetPhysicalDeviceProperties2, or it is initialized without giving vulkan instance. Call physical_device_uuid() with an instance argument instead.\n";
        }
        return std::nullopt;
    }

}
