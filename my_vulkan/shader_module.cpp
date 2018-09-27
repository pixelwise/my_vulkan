#include "shader_module.hpp"
#include "utils.hpp"

namespace my_vulkan
{
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
    {
        return createShaderModule(device, code.data(), code.size());
    }

    VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t>& code)
    {
        return createShaderModule(device, (const char*)code.data(), code.size());
    }

    VkShaderModule createShaderModule(
        VkDevice device,
        const char* code,
        size_t size
    )
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = size;
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code);

        VkShaderModule shaderModule;
        vk_require(
            vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
            "creating shader module"
        );

        return shaderModule;
    }    
}

