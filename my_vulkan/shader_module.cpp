#include "shader_module.hpp"
#include "utils.hpp"

namespace my_vulkan
{
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

    shader_module_t::shader_module_t(
        VkDevice device,
        const std::vector<uint8_t>& code
    )
    : shader_module_t{
        device,
        reinterpret_cast<const char*>(code.data()),
        code.size()
    }
    {
    }

    shader_module_t::shader_module_t(
        VkDevice device,
        const std::string& code
    )
    : shader_module_t{device, code.data(), code.size()}
    {
    }

     shader_module_t::shader_module_t(
        VkDevice device,
        const char* code,
        size_t size
    )
    : _device{device}
    , _shader_module{createShaderModule(device, code, size)}
    {
    }

    shader_module_t::shader_module_t(shader_module_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    shader_module_t& shader_module_t::operator=(shader_module_t&& other) noexcept
    {
        cleanup();
        _shader_module = other._shader_module;
        std::swap(_device, other._device);
        return *this;
    }

    VkShaderModule shader_module_t::get() const
    {
        return _shader_module;
    }

    shader_module_t::~shader_module_t()
    {
        cleanup();
    }

    void shader_module_t::cleanup()
    {
        if (_device)
        {
            vkDestroyShaderModule(_device, _shader_module, 0);
            _device = 0;
        }
    }
 }

