#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace my_vulkan
{
    struct descriptor_set_layout_t
    {
        descriptor_set_layout_t(
            VkDevice device,
            const std::vector<VkDescriptorSetLayoutBinding>& bindings
        );
        descriptor_set_layout_t(const descriptor_set_layout_t&) = delete;
        descriptor_set_layout_t(descriptor_set_layout_t&& other) noexcept;
        descriptor_set_layout_t& operator=(const descriptor_set_layout_t&) = delete;
        descriptor_set_layout_t& operator=(descriptor_set_layout_t&& other) noexcept;
        ~descriptor_set_layout_t();
        VkDescriptorSetLayout get();
    private:
        void cleanup();
        VkDevice _device;
        VkDescriptorSetLayout _layout;
    };
};
