#include "image.hpp"

#include <stdexcept>
#include "buffer.hpp"
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
        VkImageTiling tiling,
        VkImageLayout initial_layout
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
        imageInfo.initialLayout = initial_layout;
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
        device_reference_t device,
        VkExtent3D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageLayout initial_layout,
        VkImageTiling tiling,
        VkMemoryPropertyFlags properties
    )
    : _device{device}
    , _image{make_image(
        _device.get(),
        extent,
        format,
        usage,
        tiling,
        initial_layout
    )}
    , _format{format}
    , _extent{extent}
    , _layout{initial_layout}
    , _borrowed{false}
    , _memory{new device_memory_t{
        _device.get(),
        find_image_memory_config(
            _device.physical_device(),
            _device.get(),
            _image,
            properties
        )
    }}
    {
        vkBindImageMemory(_device.get(), _image, _memory->get(), 0);
    }

    image_t::image_t(
        VkDevice device,
        VkImage image,
        VkFormat format,
        VkExtent2D extent,
        VkImageLayout initial_layout
    ) : image_t{device, image, format, {extent.width, extent.height, 1}, initial_layout}
    {
    }

    image_t::image_t(
        VkDevice device,
        VkImage image,
        VkFormat format,
        VkExtent3D extent,
        VkImageLayout initial_layout
    )
    : _device{0, device}
    , _image{image}
    , _format{format}
    , _extent{extent}
    , _layout{initial_layout}
    , _borrowed{true}
    {
    }

    image_t::image_t(image_t&& other) noexcept
    : _device{0, 0}
    , _borrowed{false}
    {
        *this = std::move(other);
    }

    image_t& image_t::operator=(image_t&& other) noexcept
    {
        cleanup();
        _image = other._image;
        _memory = std::move(other._memory);
        _format = other._format;
        _extent = other._extent;
        _borrowed = other._borrowed;
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
        viewInfo.pNext = 0;
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
            vkCreateImageView(_device.get(), &viewInfo, nullptr, &image_view),
            "creating image view"
        );
        return image_view_t{
            _device.get(),
            image_view
        };
    }

    void image_t::cleanup()
    {
        if (_device.get() && !_borrowed)
        {
            if (_image)
                vkDestroyImage(_device.get(), _image, nullptr);
            _memory.reset();
        }
        _device.clear();
    }

    VkImage image_t::get()
    {
        return _image;
    }

    device_memory_t* image_t::memory()
    {
        return _memory.get();
    }

    VkFormat image_t::format() const
    {
        return _format;
    }

    VkExtent3D image_t::extent() const
    {
        return _extent;
    }

    VkImageLayout image_t::layout() const
    {
        return _layout;
    }

    void image_t::copy_from(
        VkBuffer buffer,
        command_pool_t& command_pool,
        boost::optional<VkExtent3D> extent
    )
    {
        auto command_buffer = command_pool.make_buffer();
        auto command_scope = command_buffer.begin(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        );
        copy_from(buffer, command_scope, extent);
        command_scope.end();
        command_pool.queue().submit(command_buffer.get());
        command_pool.queue().wait_idle();   
    }

    void image_t::copy_from(
        VkBuffer buffer,
        command_buffer_t::scope_t& command_scope,
        boost::optional<VkExtent3D> in_extent
    )
    {
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = in_extent.value_or(extent());
        command_scope.copy(
            buffer,
            _image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            {region}
        );
    }

    void image_t::transition_layout(
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        command_buffer_t::scope_t& command_scope
    )
    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = get();

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (has_stencil_component(format()))
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (
            oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        )
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (
            oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        ) 
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (
            oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        )
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask =
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        command_scope.pipeline_barrier(
            sourceStage,
            destinationStage,
            {barrier}
        );        
    }

    void image_t::load_pixels(command_pool_t& command_pool, const void* pixels)
    {
        auto oneshot_scope = command_pool.begin_oneshot();
        size_t image_size = _extent.width * _extent.height * bytes_per_pixel(format());
        buffer_t staging_buffer{
            _device,
            image_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
        staging_buffer.memory()->set_data(pixels, image_size);
        transition_layout(
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            oneshot_scope.commands()
        );
        copy_from(
            staging_buffer.get(),
            oneshot_scope.commands()
        );
        transition_layout(
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            oneshot_scope.commands()
        );
        oneshot_scope.execute_and_wait();
    }
}