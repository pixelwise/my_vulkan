#include <iostream>
#include "interop_utils.hpp"
#include <algorithm>
namespace my_vulkan
{
    std::vector<const char *> get_required_vk_device_extensions_for_cuda(bool offscreen)
    {
        std::vector<const char *> ret{
            VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
        };
        if (!offscreen)
            ret.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        return ret;
    }

    static void filter_extensions(std::vector<const char *>& extensions, std::vector<VkExtensionProperties> supported_extensions)
    {
        for (auto it = extensions.begin(); it != extensions.end();)
        {
            auto it_found = std::find_if(
                supported_extensions.begin(), supported_extensions.end(),
                [&it](auto x) { return strcmp(*it, x.extensionName) == 0; }
            );
            if (it_found == supported_extensions.end())
            {
                std::cerr << "device does not support required cuda extension: " << *it << "\n";
                it = extensions.erase(it);
            }
            else
            {
                ++it;
                supported_extensions.erase(it_found);
            }
        }
    }
    std::vector<const char *> get_vk_device_extensions_for_cuda(
        VkPhysicalDevice device, bool offscreen, bool throw_if_not_support_cuda
    )
    {
        auto ret = get_required_vk_device_extensions_for_cuda(offscreen);
        auto num_required_cuda_exts = ret.size();
        auto supported_exts = get_device_exts(device);
        filter_extensions(ret, supported_exts);
        if (num_required_cuda_exts == ret.size())
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

    std::vector<const char *> get_required_vk_instance_extensions_for_cuda()
    {
        return {
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
        };
    }

    std::vector<const char *> get_vk_instance_extensions_for_cuda(bool throw_if_not_support_cuda)
    {
        auto ret = get_required_vk_instance_extensions_for_cuda();
        auto num_required_cuda_exts = ret.size();
        auto supported_exts = get_instance_exts();

        filter_extensions(ret, supported_exts);
        if (num_required_cuda_exts == ret.size())
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

    int get_vk_semaphore_fd(const semaphore_t &vksem)
    {
        auto fd = vksem.get_external_handle(VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
        if (!fd)
            throw std::runtime_error("cannot get external handle from vulkan.\n");
        return fd.value();
    }

    int get_vk_memory_fd(const device_memory_t &vkmem)
    {
        auto ext_info = vkmem.external_info(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
        if (!ext_info)
            throw std::runtime_error("cannot get external handle from vulkan.\n");
        return ext_info->fd;
    }
}
