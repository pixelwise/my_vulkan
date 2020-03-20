#include "image.hpp"

#include "buffer.hpp"
#include "fence.hpp"
#include "utils.hpp"

#include <stdexcept>

namespace my_vulkan
{
    static device_memory_t::config_t find_image_memory_config(
        VkPhysicalDevice physical_device,
        VkDevice logical_device,
        VkImage image,
        VkMemoryPropertyFlags properties,
        PFN_vkGetMemoryFdKHR pfn_vkGetMemoryFdKHR,
        std::optional<VkExternalMemoryHandleTypeFlags> external_handle_types = std::nullopt
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
            type,
            external_handle_types,
            pfn_vkGetMemoryFdKHR
        };
    }

    static VkImage make_image(
        VkDevice device,
        VkExtent3D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageTiling tiling,
        VkImageLayout initial_layout,
        std::optional<VkExternalMemoryHandleTypeFlags> external_handle_types = std::nullopt
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
        VkExternalMemoryImageCreateInfo vkExternalMemImageCreateInfo = {};
        if (external_handle_types)
        {
            vkExternalMemImageCreateInfo.sType =
                VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
            vkExternalMemImageCreateInfo.pNext = NULL;
            vkExternalMemImageCreateInfo.handleTypes =
                external_handle_types.value();
            imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
            imageInfo.pNext = &vkExternalMemImageCreateInfo;
        }

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
        device_t& device,
        VkExtent3D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageLayout initial_layout,
        VkImageTiling tiling,
        VkMemoryPropertyFlags properties,
        std::optional<VkExternalMemoryHandleTypeFlags> external_handle_types
    )
    : image_t{
        device.get(),
        device.physical_device(),
        extent,
        format,
        usage,
        external_handle_types ? device.get_proc_record_if_needed<PFN_vkGetMemoryFdKHR>("vkGetMemoryFdKHR") : nullptr,
        initial_layout,
        tiling,
        properties,
        external_handle_types
    }
    {
    }

    image_t::image_t(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkExtent3D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        PFN_vkGetMemoryFdKHR pfn_vkGetMemoryFdKHR,
        VkImageLayout initial_layout,
        VkImageTiling tiling,
        VkMemoryPropertyFlags properties,
        std::optional<VkExternalMemoryHandleTypeFlags> external_handle_types
    )
    : _device{device}
    , _physical_device{physical_device}
    , _image{make_image(
        _device,
        extent,
        format,
        usage,
        tiling,
        initial_layout,
        external_handle_types
    )}
    , _format{format}
    , _extent{extent}
    , _layout{initial_layout}
    , _borrowed{false}
    , _memory{new device_memory_t{
        _device,
        find_image_memory_config(
            physical_device,
            _device,
            _image,
            properties,
            pfn_vkGetMemoryFdKHR,
            external_handle_types
        )
    }}
    {
        vkBindImageMemory(_device, _image, _memory->get(), 0);
    }

    image_t::image_t(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkImage image,
        VkFormat format,
        VkExtent2D extent,
        VkImageLayout initial_layout
    ) : image_t{
        device,
        physical_device,
        image,
        format,
        {extent.width, extent.height, 1},
        initial_layout
    }
    {
    }

    image_t::image_t(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkImage image,
        VkFormat format,
        VkExtent3D extent,
        VkImageLayout initial_layout
    )
    : _device{device}
    , _physical_device{physical_device}
    , _image{image}
    , _format{format}
    , _extent{extent}
    , _layout{initial_layout}
    , _borrowed{true}
    {
    }

    image_t::image_t(image_t&& other) noexcept
    : _device{nullptr}
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
        _physical_device = other._physical_device;
        _layout = other._layout;
        std::swap(_device, other._device);
        return *this;
    }

    image_t::~image_t()
    {
        cleanup();
    }

    image_view_t image_t::view(int aspect_flags) const
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

    device_memory_t* image_t::memory() const
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

    VkSubresourceLayout image_t::memory_layout(
        int aspect_flags,
        uint32_t mipLevel,
        uint32_t arrayLayer
    ) const
    {
        VkImageSubresource sub_resource{
            VkImageAspectFlagBits(aspect_flags),
            mipLevel,
            arrayLayer  
        };
        VkSubresourceLayout result;
        vkGetImageSubresourceLayout(
            _device,
            _image,
            &sub_resource,
            &result
        );
        return result;
    }

    void image_t::copy_from(
        VkBuffer buffer,
        command_pool_t& command_pool,
        std::optional<VkExtent3D> extent
    )
    {
        auto command_buffer = command_pool.make_buffer();
        auto command_scope = command_buffer.begin(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        );
        copy_from(buffer, command_scope, 0, extent);
        command_scope.end();
        fence_t fence{_device};
        command_pool.queue().submit(command_buffer.get(), fence.get());
        fence.wait();
    }

    void image_t::copy_from(
        VkBuffer buffer,
        command_buffer_t::scope_t& command_scope,
        uint32_t pitch,
        std::optional<VkExtent3D> in_extent
    )
    {
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = pitch;
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

    void image_t::copy_from(
        VkImage image,
        command_buffer_t::scope_t& command_scope,
        std::optional<VkExtent3D> in_extent,
        int src_aspects,
        int dst_aspects
    )
    {
        VkImageCopy region = {};
        region.srcSubresource.layerCount = 1;
        region.srcSubresource.aspectMask = src_aspects;
        region.dstSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = dst_aspects;
        region.extent = in_extent.value_or(extent());
        command_scope.copy(
            image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            _image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            {region}
        );        
    }

    void image_t::blit_from(
        image_t& image,
        command_buffer_t::scope_t& command_scope,
        int src_aspects,
        int dst_aspects
    )
    {
        auto src_extent = image.extent();
        auto dst_extent = extent();
        VkImageBlit region = {};
        region.srcSubresource.layerCount = 1;
        region.srcSubresource.aspectMask = src_aspects;
        region.dstSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = dst_aspects;
        region.srcOffsets[0] = {0, 0, 0};
        region.srcOffsets[1] = {
            int32_t(src_extent.width),
            int32_t(src_extent.height),
            int32_t(src_extent.depth)
        };
        region.dstOffsets[0] = {0, 0, 0};
        region.dstOffsets[1] = {
            int32_t(dst_extent.width),
            int32_t(dst_extent.height),
            int32_t(dst_extent.depth)
        };
        command_scope.blit(
            image.get(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
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
            newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        ) 
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (
            oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            newLayout == VK_IMAGE_LAYOUT_GENERAL
        ) 
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
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

        _layout = newLayout;

        command_scope.pipeline_barrier(
            sourceStage,
            destinationStage,
            {barrier}
        );        
    }

    void image_t::load_pixels(
        command_pool_t& command_pool,
        const void* pixels,
        std::optional<size_t> pitch
    )
    {
        auto oneshot_scope = command_pool.begin_oneshot();
        size_t image_size =
            _extent.width *
            pitch.value_or(_extent.height * bytes_per_pixel(format()));
        buffer_t staging_buffer{
            _device,
            _physical_device,
            image_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            nullptr
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
