#include "shader_module.hpp"
#include "utils.hpp"

namespace my_vulkan
{
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        vk_require(
            vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
            "creating shader module"
        );

        return shaderModule;
    }    
}

