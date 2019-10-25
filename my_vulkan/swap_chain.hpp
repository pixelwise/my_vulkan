#pragma once

#include <vulkan/vulkan.h>
#include "utils.hpp"
#include "image.hpp"
#include <boost/variant.hpp>
#include <optional>

namespace my_vulkan
{
    struct swap_chain_t
    {
        struct acquisition_outcome_t
        {
            std::optional<uint32_t> image_index;
            std::optional<acquisition_failure_t> failure;
        };
        swap_chain_t(
            device_t& _device,
            VkSurfaceKHR surface,
            VkExtent2D actual_extent
        );
        swap_chain_t(const swap_chain_t&) = delete;
        swap_chain_t(swap_chain_t&& other) noexcept;
        swap_chain_t& operator=(const swap_chain_t&) = delete;
        swap_chain_t& operator=(swap_chain_t&& other) noexcept;
        const std::vector<image_t>& images();
        VkFormat format() const;
        VkExtent2D extent() const;
        acquisition_outcome_t acquire_next_image(
            VkSemaphore semaphore,
            std::optional<uint64_t> timeout = std::nullopt
        );
        acquisition_outcome_t acquire_next_image(
            VkFence fence,
            std::optional<uint64_t> timeout = std::nullopt
        );
        acquisition_outcome_t acquire_next_image(
            VkSemaphore semaphore,
            VkFence fence,
            std::optional<uint64_t> timeout = std::nullopt
        );
        VkSwapchainKHR get();
        ~swap_chain_t();
        VkDevice device();
    private:
        void cleanup();
        device_t* _device;
        VkSwapchainKHR _swap_chain;
        std::vector<image_t> _images;
        VkFormat _format;
        VkExtent2D _extent;
    };
}
