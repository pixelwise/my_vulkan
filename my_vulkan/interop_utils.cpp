#include <iostream>
#include "interop_utils.hpp"
#include <algorithm>

std::vector<const char*> my_vulkan::get_required_vk_device_extensions_for_cuda(bool offscreen)
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

std::vector<const char*> my_vulkan::get_vk_device_extensions_for_cuda(VkPhysicalDevice device, bool offscreen, bool throw_if_not_support_cuda)
{
    auto ret = get_required_vk_device_extensions_for_cuda(offscreen);
    auto num_required_cuda_exts = ret.size();
    auto supported_exts = get_device_exts(device);

    for (auto it = ret.begin(); it != ret.end(); )
    {
        auto it_found = std::find_if(supported_exts.begin(), supported_exts.end(), [&it](auto x){return strcmp(*it, x.extensionName) == 0;});
        if (it_found == supported_exts.end())
        {
            std::cerr << "device does not support required cuda extension: " << *it << "\n";
            it = ret.erase(it);
        }
        else
        {
            ++it;
            supported_exts.erase(it_found);
        }
    }
    if (num_required_cuda_exts==ret.size())
    {
        return ret;
    }
    {
        auto msg = "This device does not support cuda-vulkan interoperation.";
        if (throw_if_not_support_cuda)
        {
            throw std::runtime_error(msg);
        }
        else
        {
            std::cerr << msg << "\n";
        }

        return std::vector<const char *>{};
    }
}

std::vector<const char *> my_vulkan::get_required_vk_instance_extensions_for_cuda()
{
    return {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
    };
}

std::vector<const char *> my_vulkan::get_vk_instance_extensions_for_cuda(bool throw_if_not_support_cuda)
{
    auto ret = get_required_vk_instance_extensions_for_cuda();
    auto num_required_cuda_exts = ret.size();
    auto supported_exts = get_instance_exts();

    for (auto it = ret.begin(); it != ret.end(); )
    {
        auto it_found = std::find_if(supported_exts.begin(), supported_exts.end(), [&it](auto x){return strcmp(*it, x.extensionName) == 0;});
        if (it_found == supported_exts.end())
        {
            std::cerr << "device does not support required cuda extension: " << *it << "\n";
            it = ret.erase(it);
        }
        else
        {
            ++it;
            supported_exts.erase(it_found);
        }
    }
    if (num_required_cuda_exts==ret.size())
    {
        return ret;
    }
    {
        auto msg = "This device does not support cuda-vulkan interoperation.";
        if (throw_if_not_support_cuda)
        {
            throw std::runtime_error(msg);
        }
        else
        {
            std::cerr << msg << "\n";
        }

        return std::vector<const char *>{};
    }
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
    auto ext_info = vkmem.external_info(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
    if (!ext_info)
        throw std::runtime_error("cannot get external handle from vulkan.\n");
    return ext_info->fd;
}
