#pragma once

#include <string>
#include <vector>
#include <boost/format.hpp>
#include <vulkan/vulkan.h>

namespace my_vulkan
{
    struct instance_t
    {
        explicit instance_t(
            const std::string& name = "vulkan",
            std::vector<const char*> extensions = {},
            std::vector<const char*> validation_layers = {}
        );
        instance_t(const instance_t&) = delete;
        instance_t(instance_t&& other) noexcept;
        instance_t& operator=(const instance_t& other) = delete;
        instance_t& operator=(instance_t&& other) noexcept;
        VkInstance get();
        std::vector<VkPhysicalDevice> physical_devices();
        ~instance_t();
        template <typename T>
        T get_proc(const std::string & proc_name) const
        {
            auto ret =(T)vkGetInstanceProcAddr(_instance, proc_name.c_str());
            if (ret == nullptr) {
                auto msg = boost::format(
                    "Vulkan instance(%p): Proc address for \"%s\" not "
                    "found.\n"
                )%_instance%proc_name;
                throw std::runtime_error(msg.str());
            }
            return ret;
        }
    private:
        void cleanup();
        VkInstance _instance{0};
    };
}
