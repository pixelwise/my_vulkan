#include <iostream>
#include "interop_utils.hpp"

std::vector<const char *> my_vulkan::get_vk_device_extensions_for_cuda(bool offscreen)
{
    std::vector<const char *> ret {
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
    };
    if (!offscreen)
        ret.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    return ret;
}

std::vector<const char *> my_vulkan::get_vk_instance_extensions_for_cuda()
{
    return {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
    };
}


int my_vulkan::get_vk_semaphore_fd(const my_vulkan::semaphore_t &vksem)
{
    auto fd = vksem.get_external_handle(VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
    if (!fd)
        throw std::runtime_error("cannot get external handle from vulkan.\n");
    return fd.value();
}

int my_vulkan::get_vk_memory_fd(const my_vulkan::device_memory_t &vkmem)
{
    auto fd = vkmem.get_external_handle(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
    if (!fd)
        throw std::runtime_error("cannot get external handle from vulkan.\n");
    return fd.value();
}
