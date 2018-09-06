#pragma once

#include <vulkan/vulkan.h>
#include "utils.hpp"
#include "image.hpp"

namespace my_vulkan
{
    struct swap_chain_t
    {
        swap_chain_t(
            VkPhysicalDevice physical_device,
            VkDevice device,
            VkSurfaceKHR surface,
            queue_family_indices_t queue_indices,
            VkExtent2D actual_extent
        );
        VkSwapchainKHR swap_chain();
        const std::vector<image_t>& images();
        VkFormat format() const;
        VkExtent2D extent() const;
    private:
        VkDevice _device;
        VkSwapchainKHR _swap_chain;
        std::vector<image_t> _images;
        VkFormat _format;
        VkExtent2D _extent;
    };

    struct swap_chain_support_t
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    swap_chain_support_t query_swap_chain_support(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    );
}
