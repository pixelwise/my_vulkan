#include "instance.hpp"
#include "utils.hpp"
#include <cstring>

namespace my_vulkan
{
    static bool checkValidationLayerSupport(std::vector<const char*> validationLayers)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    instance_t::instance_t(
        const std::string& name,
        std::vector<const char*> extensions,
        std::vector<const char*> validation_layers
    )
    {
        if (!checkValidationLayerSupport(validation_layers))
            throw std::runtime_error("validation layers requested, but not available!");
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = name.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        createInfo.ppEnabledLayerNames = validation_layers.data();
        vk_require(
            vkCreateInstance(&createInfo, nullptr, &_instance),
            "creating instance"
        );
    }

    instance_t::instance_t(instance_t&& other) noexcept
    {
        *this = std::move(other);
    }

    instance_t& instance_t::operator=(instance_t&& other) noexcept
    {
        cleanup();
        std::swap(_instance, other._instance);
        return *this;
    }

    std::vector<VkPhysicalDevice> instance_t::physical_devices()
    {
        uint32_t num_devices = 0;
        vk_require(
            vkEnumeratePhysicalDevices(
                _instance, &num_devices, 0
            ),
            "counting physical devices"
        );
        std::vector<VkPhysicalDevice> result(num_devices);
        vk_require(
            vkEnumeratePhysicalDevices(
                _instance, &num_devices, result.data()
            ),
            "getting physical devices"
        );
        return result;
    }

    VkInstance instance_t::get()
    {
        return _instance;
    }

    instance_t::~instance_t()
    {
        cleanup();
    }

    void instance_t::cleanup()
    {
        if (_instance)
        {
            _loaded_procs.clear();
            vkDestroyInstance(_instance, 0);
            _instance = 0;
        }
    }
}
