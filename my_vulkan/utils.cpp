#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "utils.hpp"

#include <iostream>
#include <sstream>
#include <set>

#include <boost/stacktrace.hpp>
#include <boost/format.hpp>

namespace my_vulkan
{
    const char* to_string(acquisition_failure_t failure)
    {
        switch(failure)
        {
            case acquisition_failure_t::not_ready:
                return "not_ready";
            case acquisition_failure_t::timeout:
                return "timeout";
            case acquisition_failure_t::out_of_date:
                return "out_of_date";
            case acquisition_failure_t::suboptimal:
                return "suboptimal";
        }
        return "??";
    }

    static inline const char* string_VkResult(VkResult input_value)
    {
        switch ((VkResult)input_value)
        {
            case VK_ERROR_INITIALIZATION_FAILED:
                return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_NOT_PERMITTED_EXT:
                return "VK_ERROR_NOT_PERMITTED_EXT";
            case VK_ERROR_INVALID_EXTERNAL_HANDLE:
                return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
            case VK_NOT_READY:
                return "VK_NOT_READY";
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VK_TIMEOUT:
                return "VK_TIMEOUT";
            case VK_ERROR_FRAGMENTED_POOL:
                return "VK_ERROR_FRAGMENTED_POOL";
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_FRAGMENTATION_EXT:
                return "VK_ERROR_FRAGMENTATION_EXT";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
            case VK_SUCCESS:
                return "VK_SUCCESS";
            case VK_ERROR_INVALID_SHADER_NV:
                return "VK_ERROR_INVALID_SHADER_NV";
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VK_ERROR_SURFACE_LOST_KHR:
                return "VK_ERROR_SURFACE_LOST_KHR";
            case VK_ERROR_VALIDATION_FAILED_EXT:
                return "VK_ERROR_VALIDATION_FAILED_EXT";
            case VK_SUBOPTIMAL_KHR:
                return "VK_SUBOPTIMAL_KHR";
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "VK_ERROR_TOO_MANY_OBJECTS";
            case VK_EVENT_RESET:
                return "VK_EVENT_RESET";
            case VK_ERROR_OUT_OF_DATE_KHR:
                return "VK_ERROR_OUT_OF_DATE_KHR";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "VK_ERROR_MEMORY_MAP_FAILED";
            case VK_EVENT_SET:
                return "VK_EVENT_SET";
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_INCOMPLETE:
                return "VK_INCOMPLETE";
            case VK_ERROR_DEVICE_LOST:
                return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                return "VK_ERROR_OUT_OF_POOL_MEMORY";
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "VK_ERROR_OUT_OF_HOST_MEMORY";
            default:
                return "Unhandled VkResult";
        }
    }

    void vk_require(VkResult result, const char* description)
    {
        if (result != VK_SUCCESS)
        {
            std::stringstream os;
            os
                << description << ": "
                << string_VkResult(result) << " (" << result << ")"
                << std::endl;
            throw std::runtime_error{os.str()};
        }
    }

    bool queue_family_indices_t::isComplete() const
    {
        return graphics && present && transfer;
    }

    std::vector<uint32_t> queue_family_indices_t::unique_indices() const
    {
        std::set<uint32_t> index_set;
        if (graphics)
            index_set.insert(*graphics);
        if (present)
            index_set.insert(*present);
        if (transfer)
            index_set.insert(*transfer);
        return {index_set.begin(), index_set.end()};
    }

    bool queue_family_indices_t::isComplete_offscreen() const
    {
        return graphics && transfer;
    }

    queue_family_indices_t find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        queue_family_indices_t indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphics = i;

            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
                indices.transfer = i;
            if (surface)
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

                if (queueFamily.queueCount > 0 && presentSupport)
                    indices.present = i;

                if (indices.isComplete())
                    break;
            }
            else
            {
                indices.isComplete_offscreen();
            }
            i++;
        }

        return indices;
    }

    memory_type_info_t findMemoryType(
        VkPhysicalDevice physical_device,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties
    )
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if (
                (typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties
            )
                return {
                    i,
                    memProperties.memoryTypes[i].propertyFlags
                };
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    bool has_stencil_component(VkFormat format)
    {
        return
            format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT;        
    }

    VkFormat uchar_format_with_components(size_t n)
    {
        switch(n)
        {
            case 1: return VK_FORMAT_R8_UNORM;
            case 2: return VK_FORMAT_R8G8_UNORM;
            case 3: return VK_FORMAT_R8G8B8_UNORM;
            case 4: return VK_FORMAT_R8G8B8A8_UNORM;
            default:
                std::cerr << "Does not support depth =" << n << "\n";
                break;

        }
        return VK_FORMAT_UNDEFINED;
    }

    size_t bytes_per_pixel(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_UNDEFINED:
                return 0;
            case VK_FORMAT_R4G4_UNORM_PACK8:
                return 1;
            case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            case VK_FORMAT_R5G6B5_UNORM_PACK16:
            case VK_FORMAT_B5G6R5_UNORM_PACK16:
            case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
                return 2;
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_USCALED:
            case VK_FORMAT_R8_SSCALED:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_SINT:
            case VK_FORMAT_R8_SRGB:
                return 1;
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_USCALED:
            case VK_FORMAT_R8G8_SSCALED:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SINT:
            case VK_FORMAT_R8G8_SRGB:
                return 2;
            case VK_FORMAT_R8G8B8_UNORM:
            case VK_FORMAT_R8G8B8_SNORM:
            case VK_FORMAT_R8G8B8_USCALED:
            case VK_FORMAT_R8G8B8_SSCALED:
            case VK_FORMAT_R8G8B8_UINT:
            case VK_FORMAT_R8G8B8_SINT:
            case VK_FORMAT_R8G8B8_SRGB:
            case VK_FORMAT_B8G8R8_UNORM:
            case VK_FORMAT_B8G8R8_SNORM:
            case VK_FORMAT_B8G8R8_USCALED:
            case VK_FORMAT_B8G8R8_SSCALED:
            case VK_FORMAT_B8G8R8_UINT:
            case VK_FORMAT_B8G8R8_SINT:
            case VK_FORMAT_B8G8R8_SRGB:
                return 3;
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_USCALED:
            case VK_FORMAT_R8G8B8A8_SSCALED:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SNORM:
            case VK_FORMAT_B8G8R8A8_USCALED:
            case VK_FORMAT_B8G8R8A8_SSCALED:
            case VK_FORMAT_B8G8R8A8_UINT:
            case VK_FORMAT_B8G8R8A8_SINT:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
            case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
            case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
            case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
            case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
            case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            case VK_FORMAT_A2R10G10B10_SINT_PACK32:
            case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
            case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
            case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
            case VK_FORMAT_A2B10G10R10_UINT_PACK32:
            case VK_FORMAT_A2B10G10R10_SINT_PACK32:
                return 4;
            case VK_FORMAT_R16_UNORM:
            case VK_FORMAT_R16_SNORM:
            case VK_FORMAT_R16_USCALED:
            case VK_FORMAT_R16_SSCALED:
            case VK_FORMAT_R16_UINT:
            case VK_FORMAT_R16_SINT:
            case VK_FORMAT_R16_SFLOAT:
                return 2;
            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_SNORM:
            case VK_FORMAT_R16G16_USCALED:
            case VK_FORMAT_R16G16_SSCALED:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_R16G16_SINT:
            case VK_FORMAT_R16G16_SFLOAT:
                return 4;
            case VK_FORMAT_R16G16B16_UNORM:
            case VK_FORMAT_R16G16B16_SNORM:
            case VK_FORMAT_R16G16B16_USCALED:
            case VK_FORMAT_R16G16B16_SSCALED:
            case VK_FORMAT_R16G16B16_UINT:
            case VK_FORMAT_R16G16B16_SINT:
            case VK_FORMAT_R16G16B16_SFLOAT:
                return 6;
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_SNORM:
            case VK_FORMAT_R16G16B16A16_USCALED:
            case VK_FORMAT_R16G16B16A16_SSCALED:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return 8;
            case VK_FORMAT_R32_UINT:
            case VK_FORMAT_R32_SINT:
            case VK_FORMAT_R32_SFLOAT:
                return 4;
            case VK_FORMAT_R32G32_UINT:
            case VK_FORMAT_R32G32_SINT:
            case VK_FORMAT_R32G32_SFLOAT:
                return 8;
            case VK_FORMAT_R32G32B32_UINT:
            case VK_FORMAT_R32G32B32_SINT:
            case VK_FORMAT_R32G32B32_SFLOAT:
                return 12;
            case VK_FORMAT_R32G32B32A32_UINT:
            case VK_FORMAT_R32G32B32A32_SINT:
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return 16;
            case VK_FORMAT_R64_UINT:
            case VK_FORMAT_R64_SINT:
            case VK_FORMAT_R64_SFLOAT:
                return 8;
            case VK_FORMAT_R64G64_UINT:
            case VK_FORMAT_R64G64_SINT:
            case VK_FORMAT_R64G64_SFLOAT:
                return 16;
            case VK_FORMAT_R64G64B64_UINT:
            case VK_FORMAT_R64G64B64_SINT:
            case VK_FORMAT_R64G64B64_SFLOAT:
                return 24;
            case VK_FORMAT_R64G64B64A64_UINT:
            case VK_FORMAT_R64G64B64A64_SINT:
            case VK_FORMAT_R64G64B64A64_SFLOAT:
                return 32;
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
                return 4;
            case VK_FORMAT_D16_UNORM:
                return 2;
            case VK_FORMAT_X8_D24_UNORM_PACK32:
            case VK_FORMAT_D32_SFLOAT:
                return 4;
            case VK_FORMAT_S8_UINT:
                return 1;
            case VK_FORMAT_D16_UNORM_S8_UINT:
                return 2;
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return 3;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return 4;
            case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
            case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            case VK_FORMAT_BC2_UNORM_BLOCK:
            case VK_FORMAT_BC2_SRGB_BLOCK:
            case VK_FORMAT_BC3_UNORM_BLOCK:
            case VK_FORMAT_BC3_SRGB_BLOCK:
            case VK_FORMAT_BC4_UNORM_BLOCK:
            case VK_FORMAT_BC4_SNORM_BLOCK:
            case VK_FORMAT_BC5_UNORM_BLOCK:
            case VK_FORMAT_BC5_SNORM_BLOCK:
            case VK_FORMAT_BC6H_UFLOAT_BLOCK:
            case VK_FORMAT_BC6H_SFLOAT_BLOCK:
            case VK_FORMAT_BC7_UNORM_BLOCK:
            case VK_FORMAT_BC7_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
            case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
            case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
            case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
            case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
            case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
            case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
            case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
            case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            case VK_FORMAT_G8B8G8R8_422_UNORM:
            case VK_FORMAT_B8G8R8G8_422_UNORM:
            case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
            case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
            case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
            case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
            case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
            case VK_FORMAT_R10X6_UNORM_PACK16:
            case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
            case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
            case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
            case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
            case VK_FORMAT_R12X4_UNORM_PACK16:
            case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
            case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
            case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
            case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            case VK_FORMAT_G16B16G16R16_422_UNORM:
            case VK_FORMAT_B16G16R16G16_422_UNORM:
            case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
            case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
            case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
            case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
            case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
            case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
            case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
            case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
            case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
            case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
            case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
            case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
            case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
                return 0;
            default:
                break;
        }
        return 0;
    }

    VkFormat find_depth_format(VkPhysicalDevice physical_device)
    {
        return find_supported_format(
            physical_device,
            {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT
            },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    VkFormat find_supported_format(
        VkPhysicalDevice physical_device,
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    )
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
            if (
                tiling == VK_IMAGE_TILING_LINEAR &&
                (props.linearTilingFeatures & features) == features
            ) 
            {
                return format;
            }
            else if (
                tiling == VK_IMAGE_TILING_OPTIMAL &&
                (props.optimalTilingFeatures & features) == features
            )
            {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    static bool isDeviceSuitable(
        VkPhysicalDevice device,
        VkSurfaceKHR surface,
        std::vector<const char*> deviceExtensions
    );

    static bool checkDeviceExtensionSupport(
        VkPhysicalDevice device,
        std::vector<const char*> deviceExtensions
    );

    VkPhysicalDevice pick_physical_device(
        VkInstance instance,
        VkSurfaceKHR surface,
        const std::vector<const char*>& deviceExtensions
    )
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices)
        {
            if (isDeviceSuitable(device, surface, deviceExtensions))
            {
                return device;
            }
        }
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    std::optional<uint32_t> find_graphics_queue(
        VkPhysicalDevice device
    )
    {
        return find_queue(device, VK_QUEUE_GRAPHICS_BIT);
    }

    std::optional<uint32_t> find_transfer_queue(
        VkPhysicalDevice device
    )
    {
        return find_queue(device, VK_QUEUE_TRANSFER_BIT);
    }

    std::optional<uint32_t> find_queue(
        VkPhysicalDevice device,
        VkQueueFlags queue_type
    )
    {
        uint32_t num_queues = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queues, 0);
        std::vector<VkQueueFamilyProperties> properties(num_queues);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queues, properties.data());
        for (uint32_t i = 0; i < num_queues; ++i)
            if (properties[i].queueFlags & queue_type)
                 return i;
        return std::nullopt;       
    }

    static bool isDeviceSuitable(
        VkPhysicalDevice device,
        VkSurfaceKHR surface,
        std::vector<const char*> deviceExtensions
    )
    {
        auto indices = my_vulkan::find_queue_families(device, surface);
        if (surface)
        {
            if (!indices.isComplete())
                return false;
        }
        else
        {
            if (!indices.isComplete_offscreen())
                return false;
        }

        if (checkDeviceExtensionSupport(device, deviceExtensions))
        {
            if (surface)
            {
                auto swapChainSupport = query_swap_chain_support(device, surface);
                if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
                    return false;
            }
        }
        else
        {
            return false;
        }
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        return supportedFeatures.samplerAnisotropy;
    }

    static bool checkDeviceExtensionSupport(
        VkPhysicalDevice device,
        std::vector<const char*> deviceExtensions
    )
    {
        auto availableExtensions = get_device_exts(device);

        std::set<std::string> requiredExtensions(
            deviceExtensions.begin(),
            deviceExtensions.end()
        );

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

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

    VkBool32 error_throw_callback(
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objectType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     messageCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMessage,
        void*                                       pUserData)

    {
        auto message = "[" + std::string{pLayerPrefix} + "]: " + std::string{pMessage};
        std::cerr << message << std::endl;
        std::cerr << boost::stacktrace::stacktrace() << std::endl;
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
            throw std::runtime_error{message};
        return true;
    }

    VkPhysicalDevice pick_physical_device(
        uint32_t device_index,
        VkInstance instance,
        VkSurfaceKHR surface,
        const std::vector<const char *>& deviceExtensions
    )
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        if (isDeviceSuitable(devices[device_index], surface, deviceExtensions))
        {
            return devices[device_index];
        }
        throw std::runtime_error(str(boost::format("device %d is NOT a suitable GPU!")%device_index));
    }

    std::vector<VkExternalMemoryHandleTypeFlagBits> vk_ext_mem_handle_types_from_vkflag(std::optional<VkExternalMemoryHandleTypeFlags> flag)
    {
        static const std::vector<VkExternalMemoryHandleTypeFlagBits> supported {
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT,
        };
        return from_vkflag(flag, supported);
    }

    std::vector<VkExternalSemaphoreHandleTypeFlagBits>
    vk_ext_semaphore_handle_types_from_vkflag(std::optional<VkExternalSemaphoreHandleTypeFlags> flag)
    {
        static const std::vector<VkExternalSemaphoreHandleTypeFlagBits> supported {
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT,
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT,
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT,
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT
        };
        return from_vkflag(flag, supported);
    }

    std::vector<VkExtensionProperties> get_device_exts(VkPhysicalDevice device)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> exts(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, exts.data());
        return exts;
    }

    std::vector<VkExtensionProperties> get_instance_exts()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> exts(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, exts.data());
        return exts;
    }


}
