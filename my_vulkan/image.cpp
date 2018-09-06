#include "image.hpp"

#include <stdexcept>
#include "utils.hpp"

namespace my_vulkan
{
    static device_memory_t::config_t find_image_memory_config(
        VkPhysicalDevice physical_device,
        VkDevice logical_device,
        VkImage image,
        VkMemoryPropertyFlags properties
    )
    {
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(logical_device, image, &requirements);
        auto type = findMemoryType(
            physical_device,
            requirements.memoryTypeBits,
            properties
        );
        return {
            requirements.size,
            type
        };
    }

    static VkImage make_image(
        VkDevice device,
        VkExtent3D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageTiling tiling
    )
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = extent;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkImage result;
        vk_require(
            vkCreateImage(device, &imageInfo, nullptr, &result),
            "creating image"
        );
        return result;
    }

    image_t::image_t(
        VkPhysicalDevice physical_device,
        VkDevice logical_device,
        VkExtent3D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageTiling tiling,
        VkMemoryPropertyFlags properties
    )
    : _device{logical_device}
    , _format{format}
    , _image{make_image(
        logical_device,
        extent,
        format,
        usage,
        tiling
    )}
    , _memory{new device_memory_t{
        logical_device,
        find_image_memory_config(
            physical_device,
            logical_device,
            _image,
            properties
        )
    }}
    {
        vkBindImageMemory(_device, _image, _memory->get(), 0);
    }

    image_t::image_t(VkDevice device, VkImage image, VkFormat format)
    : _device{device}
    , _image{image}
    , _format{format}
    , _borrowed{true}
    {
    }

    image_t::image_t(image_t&& other) noexcept
    {
        *this = std::move(other);
    }

    image_t& image_t::operator=(image_t&& other) noexcept
    {
        cleanup();
        _image = other._image;
        _memory = std::move(other._memory);
        _format = other._format;
        std::swap(_device, other._device);
        return *this;
    }

    image_t::~image_t()
    {
        cleanup();
    }

    image_view_t image_t::view(VkImageAspectFlagBits aspect_flags) const
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = _format;
        viewInfo.subresourceRange.aspectMask = aspect_flags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        VkImageView image_view;
        vk_require(
            vkCreateImageView(_device, &viewInfo, nullptr, &image_view),
            "creating image view"
        );
        return image_view_t{
            _device,
            image_view
        };
    }

    void image_t::cleanup()
    {
        if (_device && !_borrowed)
        {
            if (_image)
                vkDestroyImage(_device, _image, nullptr);
            _memory.reset();
        }
        _device = 0;
    }

    VkImage image_t::get()
    {
        return _image;
    }

    device_memory_t* image_t::memory()
    {
        return _memory.get();
    }

    VkFormat image_t::format()
    {
        return _format;
    }
}