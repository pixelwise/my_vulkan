#include "standard_swap_chain.hpp"
namespace my_vulkan
{
    namespace helpers
    {
        static image_t create_depth_image(
            device_t& logical_device,
            VkFormat format,
            VkExtent2D extent
        )
        {
            my_vulkan::image_t result{
                logical_device,
                {extent.width, extent.height, 1},
                format,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            };
            return result;
        }

        standard_swap_chain_t::standard_swap_chain_t(
            device_t& logical_device,
            VkSurfaceKHR surface,
            queue_family_indices_t queue_indices,
            VkExtent2D desired_extent,
            VkFormat depth_format
        )
        : _swap_chain{
            logical_device.physical_device(),
            logical_device.get(),
            surface,
            queue_indices,
            desired_extent
        }
        , _depth_image{create_depth_image(
            logical_device,
            depth_format,
            _swap_chain.extent()
        )}
        , _depth_view{
            _depth_image.view(VK_IMAGE_ASPECT_DEPTH_BIT)
        }
        , _render_pass{
            logical_device.get(),
            _swap_chain.format(),
            depth_format
        }
        {
            for (auto&& image : _swap_chain.images())
            {
                _image_views.emplace_back(image.view(VK_IMAGE_ASPECT_COLOR_BIT));
                _framebuffers.emplace_back(
                    logical_device.get(),
                    _render_pass.get(),
                    std::vector<VkImageView>{
                        _image_views.back().get(),
                        _depth_view.get()
                    },
                    _swap_chain.extent()
                );
            }
        }
    }
}