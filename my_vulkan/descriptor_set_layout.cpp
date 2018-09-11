#include "descriptor_set_layout.hpp"

#include <utility>
#include "utils.hpp"

namespace my_vulkan
{
    descriptor_set_layout_t::descriptor_set_layout_t(
        VkDevice device,
        std::vector<VkDescriptorSetLayoutBinding> bindings
    )
    : _device{device}
    {
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        vk_require(
            vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_layout),
            "creating descriptor set layout"
        );
    }

    descriptor_set_layout_t::descriptor_set_layout_t(
        descriptor_set_layout_t&& other
    ) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    descriptor_set_layout_t& descriptor_set_layout_t::operator=(
        descriptor_set_layout_t&& other
    ) noexcept
    {
        cleanup();
        _layout = other._layout;
        std::swap(_device, other._device);
        return *this;
    }

    VkDescriptorSetLayout descriptor_set_layout_t::get()
    {
        return _layout;
    }

    descriptor_set_layout_t::~descriptor_set_layout_t()
    {
        cleanup();
    }

    void descriptor_set_layout_t::cleanup()
    {
        if (_device)
        {
            vkDestroyDescriptorSetLayout(_device, _layout, 0);
            _device = 0;
        }
    }
}
