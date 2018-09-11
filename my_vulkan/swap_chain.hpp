#pragma once

#include <vulkan/vulkan.h>
#include "utils.hpp"
#include "image.hpp"
#include <boost/variant.hpp>
#include <boost/optional.hpp>

namespace my_vulkan
{
    struct swap_chain_t
    {
        struct acquisition_outcome_t
        {
            boost::optional<uint32_t> image_index;
            boost::optional<acquisition_failure_t> failure;
        };
        swap_chain_t(
            VkPhysicalDevice physical_device,
            VkDevice device,
            VkSurfaceKHR surface,
            queue_family_indices_t queue_indices,
            VkExtent2D actual_extent
        );
        const std::vector<image_t>& images();
        VkFormat format() const;
        VkExtent2D extent() const;
        acquisition_outcome_t acquire_next_image(
            VkSemaphore semaphore,
            boost::optional<uint64_t> timeout = boost::none
        );
        acquisition_outcome_t acquire_next_image(
            VkFence fence,
            boost::optional<uint64_t> timeout = boost::none
        );
        acquisition_outcome_t acquire_next_image(
            VkSemaphore semaphore,
            VkFence fence,
            boost::optional<uint64_t> timeout = boost::none
        );
        VkSwapchainKHR get();
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
