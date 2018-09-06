#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

namespace my_vulkan
{
    struct instance_t
    {
        instance_t(
            const std::string& name = "vulkan",
            std::vector<const char*> extensions = {},
            std::vector<const char*> validation_layers = {}
        );
        instance_t(const instance_t&) = delete;
        instance_t(instance_t&& other) noexcept;
        instance_t& operator=(const instance_t& other) = delete;
        instance_t& operator=(instance_t&& other) noexcept;
        VkInstance get();
        ~instance_t();
    private:
        void cleanup();
        VkInstance _instance{0};
    };
}