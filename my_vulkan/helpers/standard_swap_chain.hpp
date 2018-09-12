#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "../device.hpp"
#include "../utils.hpp"
#include "../command_pool.hpp"
#include "../swap_chain.hpp"
#include "../image_view.hpp"
#include "../framebuffer.hpp"
#include "../render_pass.hpp"

namespace my_vulkan
{
    namespace helpers
    {
        struct standard_swap_chain_t
        {
            standard_swap_chain_t(
                device_t& logical_device,
                VkSurfaceKHR surface,
                queue_family_indices_t queue_indices,
                VkExtent2D desired_extent,
                VkFormat depth_format
            );
        private:
            swap_chain_t _swap_chain;
            std::vector<image_view_t> _image_views;
            image_t _depth_image;
            image_view_t _depth_view;
            render_pass_t _render_pass;
            std::vector<framebuffer_t> _framebuffers;
        };
    }
}
