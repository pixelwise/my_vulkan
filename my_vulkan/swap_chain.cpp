#include "swap_chain.hpp"

namespace my_vulkan
{
    swap_chain_support_t query_swap_chain_support(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    )
    {
        swap_chain_support_t details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device,
                surface,
                &formatCount,
                details.formats.data()
            );
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device,
            surface,
            &presentModeCount,
            nullptr
        );

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device,
                surface,
                &presentModeCount,
                details.presentModes.data()
            );
        }
        return details;
    }

    static VkSurfaceFormatKHR choose_surface_format(
        const std::vector<VkSurfaceFormatKHR>& availableFormats
    )
    {
        if (
            availableFormats.size() == 1 &&
            availableFormats[0].format == VK_FORMAT_UNDEFINED
        )
            return{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        for (const auto& availableFormat : availableFormats)
            if (
                availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            )
                return availableFormat;
        return availableFormats[0];
    }

    VkPresentModeKHR choose_present_mode(
        const std::vector<VkPresentModeKHR>& availablePresentModes
    )
    {
        VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;
            else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                bestMode = availablePresentMode;
        }

        return bestMode;
    }

    static VkExtent2D choose_extent(
        VkExtent2D actualExtent,
        const VkSurfaceCapabilitiesKHR& capabilities
    )
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    swap_chain_t::swap_chain_t(
        VkPhysicalDevice physical_device,
        VkDevice device,
        VkSurfaceKHR surface,
        queue_family_indices_t queue_indices,
        VkExtent2D actual_extent
    )
    : _device{device}
    {
        auto support = query_swap_chain_support(physical_device, surface);
        VkSurfaceFormatKHR surfaceFormat = choose_surface_format(support.formats);
        VkPresentModeKHR presentMode = choose_present_mode(support.presentModes);
        _extent = choose_extent(actual_extent, support.capabilities);

        uint32_t imageCount = support.capabilities.minImageCount + 1;
        if (
            support.capabilities.maxImageCount > 0 &&
            imageCount > support.capabilities.maxImageCount
        )
            imageCount = support.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = _extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = {
            *queue_indices.graphics,
            *queue_indices.present
        };
        if (queue_indices.graphics != queue_indices.present) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = support.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        vk_require(vkCreateSwapchainKHR(device, &createInfo, nullptr, &_swap_chain),
            "creating swap chain"
        );
        std::vector<VkImage> images;
        vkGetSwapchainImagesKHR(device, _swap_chain, &imageCount, nullptr);
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(device, _swap_chain, &imageCount, images.data());
        _format = surfaceFormat.format;
        for (auto image : images)
            _images.push_back(image_t{_device, image, _format});
    }
    
    swap_chain_t::acquisition_outcome_t swap_chain_t::acquire_next_image(
        VkSemaphore semaphore,
        boost::optional<uint64_t> timeout
    )
    {
        return acquire_next_image(semaphore, 0, timeout);
    }

    swap_chain_t::acquisition_outcome_t swap_chain_t::acquire_next_image(
        VkFence fence,
        boost::optional<uint64_t> timeout
    )
    {
        return acquire_next_image(0, fence, timeout);
    }

    swap_chain_t::acquisition_outcome_t swap_chain_t::acquire_next_image(
        VkSemaphore semaphore,
        VkFence fence,
        boost::optional<uint64_t> timeout
    )
    {
        swap_chain_t::acquisition_outcome_t result;
        uint32_t image_index;
        VkResult result_code = vkAcquireNextImageKHR(
            _device,
            _swap_chain,
            timeout.value_or(std::numeric_limits<uint64_t>::max()),
            semaphore,
            fence,
            &image_index
        );
        switch(result_code)
        {
            case VK_SUCCESS:
                result.image_index = image_index;
                break;
            case VK_SUBOPTIMAL_KHR:
                result.image_index = image_index;
                result.failure = acquisition_failure_t::suboptimal;
                break;
            case VK_NOT_READY:
                result.failure = acquisition_failure_t::not_ready;
                break;
            case VK_TIMEOUT:
                result.failure = acquisition_failure_t::timeout;
                break;
            case VK_ERROR_OUT_OF_DATE_KHR:
                result.failure = acquisition_failure_t::out_of_date;
                break;
            default:
                vk_require(result_code, "acquiring image from swap chain");
        }
        return result;
    }

    VkSwapchainKHR swap_chain_t::get()
    {
        return _swap_chain;
    }
    
    const std::vector<image_t>& swap_chain_t::images()
    {
        return _images;
    }

    VkFormat swap_chain_t::format() const
    {
        return _format;
    }

    VkExtent2D swap_chain_t::extent() const
    {
        return _extent;
    }
}
