#pragma once
#include <vulkan/vulkan.hpp>
#include "my_vulkan.hpp"

namespace my_vulkan
{
    std::vector<const char*> get_vk_device_extensions_for_cuda(bool offscreen = false);

    std::vector<const char*> get_vk_instance_extensions_for_cuda();

    int get_vk_semaphore_fd(const semaphore_t & vksem);

    int get_vk_memory_fd(const device_memory_t & vkmem);

}

