#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace my_vulkan
{
    struct shader_module_t
    {
        shader_module_t(
            VkDevice device,
            const std::vector<uint8_t>& code
        );
        shader_module_t(
            VkDevice device,
            const char* code,
            size_t size
        );
        shader_module_t(const shader_module_t&) = delete;
        shader_module_t& operator=(const shader_module_t&) = delete;
        shader_module_t(shader_module_t&& other) noexcept;
        shader_module_t& operator=(shader_module_t&& other) noexcept;
        ~shader_module_t();
        VkShaderModule get();
    private:
        void cleanup();
        VkDevice _device;
        VkShaderModule _shader_module;
    };
}
