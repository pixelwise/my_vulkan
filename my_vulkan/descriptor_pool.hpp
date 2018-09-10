#pragma once

#include "vulkan/vulkan.h"

#include "descriptor_set.hpp"

namespace my_vulkan
{
    struct descriptor_pool_t
    {
        descriptor_pool_t(VkDevice device, size_t size);
        descriptor_pool_t(const descriptor_pool_t&) = delete;
        descriptor_pool_t(descriptor_pool_t&& other) noexcept;
        descriptor_pool_t& operator=(const descriptor_pool_t&) = delete;
        descriptor_pool_t& operator=(descriptor_pool_t&& other) noexcept;
        ~descriptor_pool_t();
        VkDescriptorPool get();
        descriptor_set_t make_descriptor_set(VkDescriptorSetLayout layout);
    private:
        void cleanup();
        VkDevice _device;
        VkDescriptorPool _descriptor_pool;
    };
}
