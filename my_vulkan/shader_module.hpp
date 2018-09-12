#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace my_vulkan
{
    VkShaderModule createShaderModule(
        VkDevice device,
        const std::vector<char>& code
    );
}