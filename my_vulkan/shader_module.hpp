#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace my_vulkan
{
    VkShaderModule createShaderModule(
        VkDevice device,
        const std::vector<char>& code
    );

    VkShaderModule createShaderModule(
        VkDevice device,
        const std::vector<uint8_t>& code
    );

    VkShaderModule createShaderModule(
        VkDevice device,
        const char* code,
        size_t size
    );
}