#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <array>
#include <set>

const int WIDTH = 800;
const int HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 3;

// so...
// need like 1 swap chain and stuff
// and then per renderer one graphics pipeline i guess
// and fill the command buffer with draw commands and stuff...
// so those are my main top-level primitives
// - swap chain
// - graphics pipeline
// -- command buffer

void vk_require(VkResult result, const char* description)
{
    if (result != VK_SUCCESS)
    {
        std::stringstream os;
        os << description << ": " << result;
        throw std::runtime_error{os.str()};
    }
}

std::vector<const char*> getRequiredExtensions(bool enableValidationLayers)
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(
        glfwExtensions,
        glfwExtensions + glfwExtensionCount
    );
    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return extensions;
}

bool checkValidationLayerSupport(std::vector<const char*> validationLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pCallback
)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT callback,
    const VkAllocationCallbacks* pAllocator
)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

VkExtent2D chooseSwapExtent(VkExtent2D actualExtent, const VkSurfaceCapabilitiesKHR& capabilities)
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

struct QueueFamilyIndices {
    int graphics = -1;
    int present = -1;
    bool isComplete() const
    {
        return graphics >= 0 && present >= 0;
    }
    std::vector<int> unique_indices() const
    {
        auto index_set = std::set<int>{graphics, present};
        return {index_set.begin(), index_set.end()};
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.present = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

VkImageView createImageView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags
)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

uint32_t findMemoryType(
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
            return i;
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

struct instance_t
{
    instance_t(
        const std::string& name = "vulkan",
        std::vector<const char*> validationLayers = {}
    )
    {
        if (!checkValidationLayerSupport(validationLayers))
            throw std::runtime_error("validation layers requested, but not available!");
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = name.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        auto extensions = getRequiredExtensions(!validationLayers.empty());
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
            throw std::runtime_error("failed to create instance!");
    }
    instance_t(const instance_t&) = delete;
    instance_t(instance_t&& other) noexcept
    {
        *this = std::move(other);
    }
    instance_t& operator=(const instance_t& other) = delete;
    instance_t& operator=(instance_t&& other) noexcept
    {
        cleanup();
        std::swap(_instance, other._instance);
        return *this;
    }
    VkInstance get()
    {
        return _instance;
    }
    ~instance_t()
    {
        cleanup();
    }
private:
    void cleanup()
    {
        if (_instance)
        {
            vkDestroyInstance(_instance, 0);
            _instance = 0;
        }
    }
    VkInstance _instance{0};
};

struct surface_t
{
    surface_t(VkInstance instance, GLFWwindow* window)
    : _instance{instance}
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &_surface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
    }
    ~surface_t()
    {
        vkDestroySurfaceKHR(_instance, _surface, 0);
    }
    surface_t(const surface_t& other) = delete;
    surface_t(surface_t&& other) = delete;
    surface_t& operator=(const surface_t& other) = delete;
    surface_t& operator=(surface_t&& other) = delete;
    VkSurfaceKHR get() {return _surface;}
private:
    VkInstance _instance;
    VkSurfaceKHR _surface;
};

struct device_t
{
    device_t(
        VkPhysicalDevice physical_device,
        QueueFamilyIndices queue_indices,
        std::vector<const char*> validation_layers,
        std::vector<const char*> device_extensions        
    )
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        for (int queueFamily : queue_indices.unique_indices())
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        createInfo.ppEnabledExtensionNames = device_extensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        createInfo.ppEnabledLayerNames = validation_layers.data();
        vk_require(
            vkCreateDevice(physical_device, &createInfo, nullptr, &_device),
            "create logical device"
        );
        vkGetDeviceQueue(_device, queue_indices.graphics, 0, &_graphicsQueue);
        vkGetDeviceQueue(_device, queue_indices.present, 0, &_presentQueue);      
    }
    device_t(const device_t&) = delete;
    ~device_t()
    {
        vkDestroyDevice(_device, 0);
    }
    VkDevice get() {return _device;}
    VkQueue graphics_queue() {return _graphicsQueue;}
    VkQueue present_queue() {return _presentQueue;}
private:
    VkDevice _device;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
};

struct texture_sampler_t
{
private:
};

struct descriptor_set_layout_t
{
private:
};

struct render_pass_t
{
    render_pass_t(
        VkDevice device,
        VkFormat image_format,
        VkFormat depth_format
    )
    {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = image_format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = depth_format;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &_render_pass) != VK_SUCCESS)
            throw std::runtime_error("failed to create render pass!");
    }
    render_pass_t(render_pass_t&& other)
    : _device{0}
    {
        *this = std::move(other);
    }
    render_pass_t(const render_pass_t&) = delete;
    render_pass_t& operator=(render_pass_t&& other)
    {
        cleanup();
        _render_pass = other._render_pass;
        std::swap(_device, other._device);
        return *this;
    }
    render_pass_t& operator=(const render_pass_t&) = delete;
    VkRenderPass get()
    {
        return _render_pass;
    }
private:
    void cleanup()
    {
        if (_device)
        {
            vkDestroyRenderPass(_device, _render_pass, 0);
            _device = 0;
        }
    }
    VkDevice _device;
    VkRenderPass _render_pass;
};

struct image_view_t
{
    image_view_t() {}
    image_view_t(VkDevice device, VkImageView view)
    : _device{device}
    , _view{view}
    {
    }
    image_view_t(image_view_t&& other) noexcept
    {
        *this = std::move(other);
    }
    image_view_t(const image_view_t&) = delete;
    image_view_t& operator=(image_view_t&& other) noexcept
    {
        cleanup();
        _view = other._view;
        std::swap(_device, other._device);
        return *this;
    }
    image_view_t& operator=(const image_view_t&) = delete;
    VkImageView get() const {return _view;}
private:
    void cleanup()
    {
        if (_device)
        {
            vkDestroyImageView(_device, _view, 0);
            _device = 0;
        }
    }
    VkDevice _device{0};
    VkImageView _view{0};
};

struct device_reference_t
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
};

struct image_t
{
    image_t(
        device_reference_t device,
        VkExtent3D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    ) 
    : _device{device.logical_device}
    , image{0}
    , memory{0}
    , format{format}
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
        if (vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            cleanup();
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(_device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            device.physical_device,
            memRequirements.memoryTypeBits,
            properties
        );
        if (vkAllocateMemory(_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
        {
            cleanup();
            throw std::runtime_error("failed to allocate image memory!");
        }
        vkBindImageMemory(_device, image, memory, 0);
    }
    image_t(const image_t&) = delete;
    image_t(image_t&& other) noexcept
    {
        *this = std::move(other);
    }
    image_t& operator=(const image_t&) = delete;
    image_t& operator=(image_t&& other) noexcept
    {
        cleanup();
        image = other.image;
        memory = other.memory;
        format = other.format;
        std::swap(_device, other._device);
        return *this;
    }
    ~image_t()
    {
        cleanup();
    }
    image_view_t view(VkImageAspectFlagBits flags) const
    {
        return image_view_t{
            _device,
            createImageView(
                _device,
                image,
                format,
                flags
            )
        };
    }
    image_t(VkDevice device, VkImage image, VkFormat format)
    : _device{device}
    , image{image}
    , format{format}
    , _borrowed{true}
    {

    }
    VkImage image;
    VkDeviceMemory memory{0};
    VkFormat format;
private:
    bool _borrowed;
    void cleanup()
    {
        if (_device && !_borrowed)
        {
            vkDestroyImage(_device, image, nullptr);
            vkFreeMemory(_device, memory, nullptr);
        }
        _device = 0;
    }
    VkDevice _device{0};
};

struct buffer_t
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void destroy(VkDevice device)
    {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }
};

struct swap_chain_t
{
    swap_chain_t(
        VkPhysicalDevice physical_device,
        VkDevice device,
        VkSurfaceKHR surface,
        VkExtent2D actual_extent
    )
    : _device{device}
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physical_device, surface);
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        _extent = chooseSwapExtent(actual_extent, swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = _extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physical_device, surface);
        uint32_t queueFamilyIndices[] = {
            (uint32_t) indices.graphics,
            (uint32_t) indices.present
        };
        if (indices.graphics != indices.present) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &_swap_chain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }
        std::vector<VkImage> images;
        vkGetSwapchainImagesKHR(device, _swap_chain, &imageCount, nullptr);
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(device, _swap_chain, &imageCount, images.data());
        _format = surfaceFormat.format;
        for (auto image : images)
            _images.push_back(image_t{_device, image, _format});
    }
    VkSwapchainKHR swap_chain() {return _swap_chain;}
    const std::vector<image_t>& images() {return _images;}
    VkFormat format() const {return _format;}
    VkExtent2D extent() const {return _extent;}
private:
    VkDevice _device;
    VkSwapchainKHR _swap_chain;
    std::vector<image_t> _images;
    VkFormat _format;
    VkExtent2D _extent;    
};

struct render_pipeline_t
{
    render_pipeline_t(
        VkPhysicalDevice physical_device,
        VkDevice logical_device,
        VkFormat image_format,
        VkFormat depth_format
    )
    : _render_pass{logical_device, image_format, depth_format}
    {

    }
private:
    render_pass_t _render_pass;
};

struct pipeline_t
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct fence_t
{
    fence_t(
        VkDevice device,
        VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT
    )
    : _device{device}
    {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = flags;
        vk_require(
            vkCreateFence(device, &fenceInfo, nullptr, &_fence), 
            "create fence"
        );
    }
    fence_t(const fence_t&) = delete;
    fence_t& operator=(const fence_t&) = delete;
    fence_t(fence_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }
    fence_t& operator=(fence_t&& other) noexcept
    {
        cleanup();
        _fence = other._fence;
        std::swap(_device, other._device);
        return *this;
    }
    void reset()
    {
        vkResetFences(_device, 1, &_fence);        
    }
    void wait(
        uint64_t timeout = std::numeric_limits<uint64_t>::max()
    )
    {
        vkWaitForFences(_device, 1, &_fence, VK_TRUE, timeout);
    }
    VkFence get() {return _fence;}
private:
    void cleanup()
    {
        if (_device)
        {
            vkDestroyFence(_device, _fence, 0);
            _device = 0;
        }
    }
    VkDevice _device;
    VkFence _fence;
};

struct semaphore_t
{
    semaphore_t(
        VkDevice device,
        VkSemaphoreCreateFlags flags = 0
    )
    : _device{device}
    {
        VkSemaphoreCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.flags = flags;
        vk_require(
            vkCreateSemaphore(device, &info, nullptr, &_semaphore), 
            "create semaphore"
        );
    }
    semaphore_t(const semaphore_t&) = delete;
    semaphore_t& operator=(const semaphore_t&) = delete;
    semaphore_t(semaphore_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }
    semaphore_t& operator=(semaphore_t&& other) noexcept
    {
        cleanup();
        _semaphore = other._semaphore;
        std::swap(_device, other._device);
        return *this;
    }
    VkSemaphore get() {return _semaphore;}
private:
    void cleanup()
    {
        if (_device)
        {
            vkDestroySemaphore(_device, _semaphore, 0);
            _device = 0;
        }
    }
    VkDevice _device;
    VkSemaphore _semaphore;    
};

struct frame_sync_points_t
{
    frame_sync_points_t(VkDevice device)
    : imageAvailable{device}
    , renderFinished{device}
    , inFlight{device}
    {}
    semaphore_t imageAvailable;
    semaphore_t renderFinished;
    fence_t inFlight;
};

class HelloTriangleApplication {
public:
    HelloTriangleApplication()
    : window{initWindow(this)}
    , validationLayers{
        //"VK_LAYER_LUNARG_standard_validation"        
    }
    , deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    }
    , instance{"test", validationLayers}
    , surface{instance.get(), window}
    , physical_device{pickPhysicalDevice(
        instance.get(),
        surface.get(),
        deviceExtensions
    )}
    , queue_indices{findQueueFamilies(
        physical_device,
        surface.get()
    )}
    , logical_device{
        physical_device,
        queue_indices,
        validationLayers,
        deviceExtensions
    }
    {
        auto depth_format = findDepthFormat(physical_device);
        if (!validationLayers.empty())
            callback = setupDebugCallback(instance.get());
        commandPool = createCommandPool(
            logical_device.get(),
            queue_indices
        );
        textureSampler = createTextureSampler(
            logical_device.get()
        );
        descriptorSetLayout = createDescriptorSetLayout(
            logical_device.get()
        );
        swap_chain.reset(new swap_chain_t{
            physical_device,
            logical_device.get(),
            surface.get(),
            window_extent()
        });
        swapChainImageViews = createImageViews(
            logical_device.get(),
            swap_chain->images()
        );
        depth_image = createDepthImage(
            physical_device,
            logical_device,
            commandPool,
            depth_format,
            swap_chain->extent()
        );
        renderPass.reset(new render_pass_t{
            logical_device.get(),
            swap_chain->format(),
            depth_format
        });
        depth_view = depth_image->view(VK_IMAGE_ASPECT_DEPTH_BIT);
        swapChainFramebuffers = createFramebuffers(
            logical_device.get(),
            swapChainImageViews,
            depth_view,
            renderPass->get(),
            swap_chain->extent()
        );
        uniform_buffers = createUniformBuffers(
            physical_device,
            logical_device.get(),
            swap_chain->images().size()
        );
        descriptorPool = createDescriptorPool(
            logical_device.get(),
            static_cast<uint32_t>(swap_chain->images().size())
        );
        texture_image = createTextureImage(
            physical_device,
            logical_device,
            commandPool
        );
        textureImageView = texture_image->view(VK_IMAGE_ASPECT_COLOR_BIT);
        descriptorSets = createDescriptorSets(
            logical_device.get(),
            descriptorPool,
            uniform_buffers,
            textureImageView.get(),
            textureSampler,
            descriptorSetLayout,
            static_cast<uint32_t>(swap_chain->images().size())
        );
        graphicsPipeline = createGraphicsPipeline(
            logical_device.get(),
            swap_chain->extent(),
            renderPass->get(),
            descriptorSetLayout
        );
        vertex_buffer = createVertexBuffer(
            physical_device,
            logical_device,
            commandPool
        );
        index_buffer = createIndexBuffer(
            physical_device,
            logical_device,
            commandPool
        );
        commandBuffers = createCommandBuffers(
            logical_device.get(),
            commandPool,
            renderPass->get(),
            swapChainFramebuffers,
            swap_chain->extent(),
            vertex_buffer.buffer,
            graphicsPipeline,
            index_buffer.buffer,
            descriptorSets,
            swapChainFramebuffers.size()
        );

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            frame_sync_points.emplace_back(logical_device.get());
    }
    
    void run()
    {
        mainLoop();
    }

    ~HelloTriangleApplication()
    {
        cleanup();        
    }

private:
    GLFWwindow* window;
    std::vector<const char*> validationLayers;
    std::vector<const char*> deviceExtensions;

    instance_t instance;
    VkDebugUtilsMessengerEXT callback;
    surface_t surface;

    VkPhysicalDevice physical_device;
    QueueFamilyIndices queue_indices;
    device_t logical_device;

    std::unique_ptr<swap_chain_t> swap_chain;

    std::vector<image_view_t> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    std::unique_ptr<render_pass_t> renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    pipeline_t graphicsPipeline;

    VkCommandPool commandPool;

    std::unique_ptr<image_t> depth_image;
    image_view_t depth_view;

    std::unique_ptr<image_t> texture_image;
    image_view_t textureImageView;
    VkSampler textureSampler;

    buffer_t vertex_buffer;
    buffer_t index_buffer;

    std::vector<buffer_t> uniform_buffers;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<frame_sync_points_t> frame_sync_points;
    size_t currentFrame = 0;

    bool framebufferResized = false;

    static GLFWwindow* initWindow(void* userdata)
    {
        GLFWwindow* window;
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, userdata);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        return window;
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    VkExtent2D window_extent() const
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        return VkExtent2D{
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };        
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame(logical_device);
        }

        vkDeviceWaitIdle(logical_device.get());
    }

    void cleanupSwapChain(VkDevice device)
    {
        for (auto framebuffer : swapChainFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        vkFreeCommandBuffers(
            device,
            commandPool,
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data()
        );
        vkDestroyPipeline(device, graphicsPipeline.pipeline, nullptr);
        vkDestroyPipelineLayout(device, graphicsPipeline.layout, nullptr);
    }

    void cleanup()
    {
        cleanupSwapChain(logical_device.get());
        vkDestroySampler(logical_device.get(), textureSampler, nullptr);
        vkDestroyDescriptorPool(logical_device.get(), descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(logical_device.get(), descriptorSetLayout, nullptr);
        for (auto&& buffer : uniform_buffers)
            buffer.destroy(logical_device.get());
        index_buffer.destroy(logical_device.get());
        vertex_buffer.destroy(logical_device.get());
        vkDestroyCommandPool(logical_device.get(), commandPool, nullptr);
        vkDestroyDevice(logical_device.get(), nullptr);
        if (callback)
            DestroyDebugUtilsMessengerEXT(instance.get(), callback, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void recreateSwapChain(device_t& logical_device)
    {
        std::cout << "recreating swap chain" << std::endl;
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(logical_device.get());

        cleanupSwapChain(logical_device.get());

        auto depth_format = findDepthFormat(physical_device);
        swap_chain.reset(new swap_chain_t{
            physical_device,
            logical_device.get(),
            surface.get(),
            window_extent()
        });
        swapChainImageViews = createImageViews(
            logical_device.get(),
            swap_chain->images()
        );
        renderPass.reset(new render_pass_t{
            logical_device.get(),
            swap_chain->format(),
            depth_format
        });
        graphicsPipeline = createGraphicsPipeline(
            logical_device.get(),
            swap_chain->extent(),
            renderPass->get(),
            descriptorSetLayout
        );
        depth_image = createDepthImage(
            physical_device,
            logical_device,
            commandPool,
            depth_format,
            swap_chain->extent()
        );
        depth_view = depth_image->view(VK_IMAGE_ASPECT_DEPTH_BIT);
        swapChainFramebuffers = createFramebuffers(
            logical_device.get(),
            swapChainImageViews,
            depth_view,
            renderPass->get(),
            swap_chain->extent()
        );
        commandBuffers = createCommandBuffers(
            logical_device.get(),
            commandPool,
            renderPass->get(),
            swapChainFramebuffers,
            swap_chain->extent(),
            vertex_buffer.buffer,
            graphicsPipeline,
            index_buffer.buffer,
            descriptorSets,
            swapChainFramebuffers.size()
        );
    }

    static VkDebugUtilsMessengerEXT setupDebugCallback(VkInstance instance)
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        VkDebugUtilsMessengerEXT callback;
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
            throw std::runtime_error("failed to set up debug callback!");
        return callback;
    }

    static VkPhysicalDevice pickPhysicalDevice(
        VkInstance instance,
        VkSurfaceKHR surface,
        std::vector<const char*> deviceExtensions
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

    static std::vector<image_view_t> createImageViews(VkDevice device, const std::vector<image_t>& images)
    {
        std::vector<image_view_t> result;
        for (uint32_t i = 0; i < images.size(); i++)
            result.push_back(images[i].view(VK_IMAGE_ASPECT_COLOR_BIT));
        return result;
    }

    static VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        VkDescriptorSetLayout descriptorSetLayout;
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
        return descriptorSetLayout;
    }

    static pipeline_t createGraphicsPipeline(
        VkDevice device,
        VkExtent2D extent,
        VkRenderPass render_pass,
        VkDescriptorSetLayout descriptor_set_layout
    )
    {
        auto vertShaderCode = readFile("shaders/26_shader_depth.vert.spv");
        auto fragShaderCode = readFile("shaders/26_shader_depth.frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {
            vertShaderStageInfo,
            fragShaderStageInfo
        };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) extent.width;
        viewport.height = (float) extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = extent;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptor_set_layout;

        pipeline_t result;
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &result.layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = result.layout;
        pipelineInfo.renderPass = render_pass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &result.pipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        return result;
    }

    static std::vector<VkFramebuffer> createFramebuffers(
        VkDevice device,
        const std::vector<image_view_t>& image_views,
        image_view_t& depth_view,
        VkRenderPass renderPass,
        VkExtent2D extent
    )
    {
        std::vector<VkFramebuffer> result;
        result.resize(image_views.size());

        for (size_t i = 0; i < image_views.size(); i++)
        {
            std::array<VkImageView, 2> attachments = {
                image_views[i].get(),
                depth_view.get()
            };
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &result[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create framebuffer!");
        }
        return result;
    }

    static VkCommandPool createCommandPool(
        VkDevice device,
        QueueFamilyIndices queueFamilyIndices
    )
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
        VkCommandPool result;
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &result) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics command pool!");
        return result;
    }

    static std::unique_ptr<image_t> createDepthImage(
        VkPhysicalDevice physical_device,
        device_t& logical_device,
        VkCommandPool commandPool,
        VkFormat format,
        VkExtent2D extent
    )
    {
        std::unique_ptr<image_t> result{new image_t{
            {physical_device, logical_device.get()},
            {extent.width, extent.height, 1},
            format,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        }};
        transitionImageLayout(
            logical_device,
            commandPool,
            *result,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        );
        return result;
    }

    static VkFormat findDepthFormat(VkPhysicalDevice physical_device)
    {
        return findSupportedFormat(
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

    static VkFormat findSupportedFormat(
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
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    static bool hasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    static std::unique_ptr<image_t> createTextureImage(
        VkPhysicalDevice physical_device,
        device_t& logical_device,
        VkCommandPool commandPool
    )
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("../texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        auto staging_buffer = createBuffer(
            physical_device,
            logical_device.get(),
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        void* data;
        vkMapMemory(logical_device.get(), staging_buffer.memory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logical_device.get(), staging_buffer.memory);

        stbi_image_free(pixels);


        std::unique_ptr<image_t> result{new image_t{
            {physical_device, logical_device.get()},            
            {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        }};

        transitionImageLayout(
            logical_device,
            commandPool,
            *result,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        copyBufferToImage(
            logical_device,
            commandPool,
            staging_buffer.buffer,
            result->image,
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight)
        );
        transitionImageLayout(
            logical_device,
            commandPool,
            *result,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        vkDestroyBuffer(logical_device.get(), staging_buffer.buffer, nullptr);
        vkFreeMemory(logical_device.get(), staging_buffer.memory, nullptr);
        return result;
    }

    static VkSampler createTextureSampler(VkDevice device)
    {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        VkSampler result;
        if (vkCreateSampler(device, &samplerInfo, nullptr, &result) != VK_SUCCESS)
            throw std::runtime_error("failed to create texture sampler!");
        return result;
    }

    static void transitionImageLayout(
        device_t& logical_device,
        VkCommandPool commandPool,
        image_t& image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    ) 
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(logical_device.get(), commandPool);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.image;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(image.format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(logical_device, commandPool, commandBuffer);
    }

    static void copyBufferToImage(
        device_t& logical_device,
        VkCommandPool commandPool,
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height
    ) 
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(logical_device.get(), commandPool);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(logical_device, commandPool, commandBuffer);
    }

    static buffer_t createVertexBuffer(
        VkPhysicalDevice physical_device,
        device_t& logical_device,
        VkCommandPool commandPool
    )
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        auto staging_buffer = createBuffer(
            physical_device,
            logical_device.get(),
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        void* data;
        vkMapMemory(logical_device.get(), staging_buffer.memory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(logical_device.get(), staging_buffer.memory);

        auto result = createBuffer(
            physical_device,
            logical_device.get(),
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        copyBuffer(logical_device, commandPool, staging_buffer.buffer, result.buffer, bufferSize);

        vkDestroyBuffer(logical_device.get(), staging_buffer.buffer, nullptr);
        vkFreeMemory(logical_device.get(), staging_buffer.memory, nullptr);
        return result;
    }

    static buffer_t createIndexBuffer(
        VkPhysicalDevice physical_device,
        device_t& logical_device,
        VkCommandPool commandPool
    )
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        auto staging_buffer = createBuffer(
            physical_device,
            logical_device.get(),
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        void* data;
        vkMapMemory(logical_device.get(), staging_buffer.memory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(logical_device.get(), staging_buffer.memory);

        auto result = createBuffer(
            physical_device,
            logical_device.get(),
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        copyBuffer(logical_device, commandPool, staging_buffer.buffer, result.buffer, bufferSize);

        vkDestroyBuffer(logical_device.get(), staging_buffer.buffer, nullptr);
        vkFreeMemory(logical_device.get(), staging_buffer.memory, nullptr);
        return result;
    }

    static std::vector<buffer_t> createUniformBuffers(
        VkPhysicalDevice physical_device,
        VkDevice device,
        size_t n
    )
    {
        const VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        std::vector<buffer_t> result;
        for (size_t i = 0; i < n; i++) {
            result.push_back(createBuffer(
                physical_device,
                device,
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ));
        }
        return result;
    }

    static VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t size)
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = size;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = size;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = size;
        VkDescriptorPool result;
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &result) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
        return result;
    }

    static std::vector<VkDescriptorSet> createDescriptorSets(
        VkDevice device,
        VkDescriptorPool descriptorPool,
        std::vector<buffer_t>& uniform_buffers,
        VkImageView textureImageView,
        VkSampler textureSampler,
        const VkDescriptorSetLayout& descriptorSetLayout,
        uint32_t size
    )
    {
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkDescriptorSetLayout> layouts(size, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = size;
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(size);
        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[0]) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");

        for (size_t i = 0; i < size; i++)
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = uniform_buffers[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
        return descriptorSets;
    }

    static buffer_t createBuffer(
        VkPhysicalDevice physical_device,
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
    ) 
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        buffer_t result;
        if (vkCreateBuffer(device, &bufferInfo, nullptr, &result.buffer) != VK_SUCCESS)
            throw std::runtime_error("failed to create buffer!");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, result.buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            physical_device,
            memRequirements.memoryTypeBits,
            properties
        );

        if (vkAllocateMemory(device, &allocInfo, nullptr, &result.memory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate buffer memory!");
        vkBindBufferMemory(device, result.buffer, result.memory, 0);
        return result;
    }

    static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
    {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    static void endSingleTimeCommands(
        device_t& logical_device,
        VkCommandPool commandPool,
        VkCommandBuffer commandBuffer
    )
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(logical_device.graphics_queue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(logical_device.graphics_queue());
        vkFreeCommandBuffers(logical_device.get(), commandPool, 1, &commandBuffer);
    }

    static void copyBuffer(
        device_t& logical_device,
        VkCommandPool commandPool,
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize size
    ) 
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands(logical_device.get(), commandPool);

        VkBufferCopy copyRegion = {};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(logical_device, commandPool, commandBuffer);
    }

    static std::vector<VkCommandBuffer> createCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        VkRenderPass renderPass,
        std::vector<VkFramebuffer>& swapChainFramebuffers,
        VkExtent2D extent, // swap_chain.extent
        VkBuffer vertex_buffer, // vertex_buffer.buffer
        pipeline_t& graphicsPipeline,
        VkBuffer index_buffer, // index_buffer.buffer
        std::vector<VkDescriptorSet>& descriptorSets,
        size_t size
    )
    {
        std::vector<VkCommandBuffer> commandBuffers(size);

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = extent;

            std::array<VkClearValue, 2> clearValues = {};
            clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);

            VkBuffer vertexBuffers[] = {vertex_buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(
                commandBuffers[i],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline.layout,
                0, 1,
                &descriptorSets[i],
                0, nullptr
            );

            vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffers[i]);

            vk_require(vkEndCommandBuffer(commandBuffers[i]), "record command buffer");
        }
        return commandBuffers;
    }

    void updateUniformBuffer(VkDevice device, uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo = {};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swap_chain->extent().width / (float) swap_chain->extent().height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        void* data;
        auto& buffer = uniform_buffers[currentImage];
        vkMapMemory(device, buffer.memory, 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, buffer.memory);
    }

    void drawFrame(device_t& logical_device)
    {
        auto& sync = frame_sync_points[currentFrame];
        sync.inFlight.wait();

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
            logical_device.get(),
            swap_chain->swap_chain(),
            std::numeric_limits<uint64_t>::max(),
            sync.imageAvailable.get(),
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain(logical_device);
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {sync.imageAvailable.get()};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {sync.renderFinished.get()};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        sync.inFlight.reset();

        updateUniformBuffer(logical_device.get(), imageIndex);

        vk_require(
            vkQueueSubmit(
                logical_device.graphics_queue(),
                1,
                &submitInfo,
                sync.inFlight.get()
            ),
            "submit draw command"
        );

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swap_chain->swap_chain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(logical_device.present_queue(), &presentInfo);
        if (
            result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR ||
            framebufferResized
        )
        {
            framebufferResized = false;
            recreateSwapChain(logical_device);
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<const char*> deviceExtensions)
    {
        QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensions);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    static bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> deviceExtensions)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
        void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
