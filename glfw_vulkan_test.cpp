#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "my_vulkan/utils.hpp"
#include "my_vulkan/instance.hpp"
#include "my_vulkan/surface.hpp"
#include "my_vulkan/device.hpp"
#include "my_vulkan/swap_chain.hpp"
#include "my_vulkan/image.hpp"
#include "my_vulkan/buffer.hpp"
#include "my_vulkan/command_pool.hpp"
#include "my_vulkan/command_buffer.hpp"
#include "my_vulkan/descriptor_pool.hpp"
#include "my_vulkan/descriptor_set.hpp"

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
// see also: 
// https://stackoverflow.com/questions/47098748/draw-multiple-objects-with-their-textures-in-vulkan?rq=1

using my_vulkan::vk_require;

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
    , instance{
        "test",
        getRequiredExtensions(!validationLayers.empty()),
        validationLayers
    }
    , callback{
        validationLayers.empty() ?
            0 :
            setupDebugCallback(instance.get())
    }
    , surface{instance.get(), window}
    , physical_device{pickPhysicalDevice(
        instance.get(),
        surface.get(),
        deviceExtensions
    )}
    , queue_indices{my_vulkan::findQueueFamilies(
        physical_device,
        surface.get()
    )}
    , logical_device{
        physical_device,
        queue_indices,
        validationLayers,
        deviceExtensions
    }
    , commandPool{
        logical_device.get(),
        *queue_indices.graphics
    }
    , vertex_buffer{createVertexBuffer(
        logical_device,
        commandPool
    )}
    , index_buffer{createIndexBuffer(
        logical_device,
        commandPool
    )}
    , swap_chain{
        physical_device,
        logical_device.get(),
        surface.get(),
        queue_indices,
        window_extent()
    }
    , descriptor_pool{
        logical_device.get(),
        swap_chain.images().size()
    }
    , depth_format{
        findDepthFormat(physical_device)
    }
    , render_pass{
        logical_device.get(),
        swap_chain.format(),
        depth_format
    }
    {        
        depth_image = createDepthImage(
            physical_device,
            logical_device,
            commandPool,
            depth_format,
            swap_chain.extent()
        );
        depth_view = depth_image->view(VK_IMAGE_ASPECT_DEPTH_BIT);
        swapChainImageViews = createImageViews(
            swap_chain.images()
        );
        swapChainFramebuffers = createFramebuffers(
            logical_device.get(),
            swapChainImageViews,
            depth_view,
            render_pass.get(),
            swap_chain.extent()
        );

        textureSampler = createTextureSampler(
            logical_device.get()
        );
        descriptorSetLayout = createDescriptorSetLayout(
            logical_device.get()
        );
        uniform_buffers = createUniformBuffers(
            logical_device,
            swap_chain.images().size()
        );
        texture_image = createTextureImage(
            physical_device,
            logical_device,
            commandPool
        );
        textureImageView = texture_image->view(VK_IMAGE_ASPECT_COLOR_BIT);
        descriptorSets = createDescriptorSets(
            descriptor_pool,
            uniform_buffers,
            textureImageView.get(),
            textureSampler,
            descriptorSetLayout,
            static_cast<uint32_t>(swap_chain.images().size())
        );
        graphicsPipeline = createGraphicsPipeline(
            logical_device.get(),
            swap_chain.extent(),
            render_pass.get(),
            descriptorSetLayout
        );
        commandBuffers = createCommandBuffers(
            commandPool,
            render_pass.get(),
            swapChainFramebuffers,
            swap_chain.extent(),
            vertex_buffer.get(),
            graphicsPipeline,
            index_buffer.get(),
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

    my_vulkan::instance_t instance;
    VkDebugUtilsMessengerEXT callback;
    my_vulkan::surface_t surface;

    VkPhysicalDevice physical_device;
    my_vulkan::queue_family_indices_t queue_indices;
    my_vulkan::device_t logical_device;

    my_vulkan::swap_chain_t swap_chain;

    VkFormat depth_format;

    std::vector<my_vulkan::image_view_t> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    render_pass_t render_pass;
    VkDescriptorSetLayout descriptorSetLayout;
    pipeline_t graphicsPipeline;

    my_vulkan::command_pool_t commandPool;

    std::unique_ptr<my_vulkan::image_t> depth_image;
    my_vulkan::image_view_t depth_view;

    std::unique_ptr<my_vulkan::image_t> texture_image;
    my_vulkan::image_view_t textureImageView;
    VkSampler textureSampler;

    my_vulkan::buffer_t vertex_buffer;
    my_vulkan::buffer_t index_buffer;

    std::vector<my_vulkan::buffer_t> uniform_buffers;

    my_vulkan::descriptor_pool_t descriptor_pool;
    std::vector<my_vulkan::descriptor_set_t> descriptorSets;

    std::vector<my_vulkan::command_buffer_t> commandBuffers;

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
        auto app = reinterpret_cast<HelloTriangleApplication*>(
            glfwGetWindowUserPointer(
                window
            )
        );
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

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            draw_frame();
        }
        logical_device.wait_idle();
    }

    void cleanupSwapChain(VkDevice device)
    {
        for (auto framebuffer : swapChainFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        vkDestroyPipeline(device, graphicsPipeline.pipeline, nullptr);
        vkDestroyPipelineLayout(device, graphicsPipeline.layout, nullptr);
    }

    void cleanup()
    {
        cleanupSwapChain(logical_device.get());
        vkDestroySampler(logical_device.get(), textureSampler, nullptr);
        vkDestroyDescriptorSetLayout(logical_device.get(), descriptorSetLayout, nullptr);
        if (callback)
            DestroyDebugUtilsMessengerEXT(instance.get(), callback, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void recreateSwapChain(my_vulkan::device_t& logical_device)
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
        swap_chain = my_vulkan::swap_chain_t{
            physical_device,
            logical_device.get(),
            surface.get(),
            queue_indices,
            window_extent()
        };
        swapChainImageViews = createImageViews(
            swap_chain.images()
        );
        render_pass = render_pass_t{
            logical_device.get(),
            swap_chain.format(),
            depth_format
        };
        graphicsPipeline = createGraphicsPipeline(
            logical_device.get(),
            swap_chain.extent(),
            render_pass.get(),
            descriptorSetLayout
        );
        depth_image = createDepthImage(
            physical_device,
            logical_device,
            commandPool,
            depth_format,
            swap_chain.extent()
        );
        depth_view = depth_image->view(VK_IMAGE_ASPECT_DEPTH_BIT);
        swapChainFramebuffers = createFramebuffers(
            logical_device.get(),
            swapChainImageViews,
            depth_view,
            render_pass.get(),
            swap_chain.extent()
        );
        commandBuffers = createCommandBuffers(
            commandPool,
            render_pass.get(),
            swapChainFramebuffers,
            swap_chain.extent(),
            vertex_buffer.get(),
            graphicsPipeline,
            index_buffer.get(),
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
        vk_require(
            CreateDebugUtilsMessengerEXT(
                instance,
                &createInfo, 
                nullptr,
                &callback
            ),
            "cerating debug messenger"
        );
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

    static std::vector<my_vulkan::image_view_t> createImageViews(
        const std::vector<my_vulkan::image_t>& images
    )
    {
        std::vector<my_vulkan::image_view_t> result;
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
        const std::vector<my_vulkan::image_view_t>& image_views,
        my_vulkan::image_view_t& depth_view,
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

    static std::unique_ptr<my_vulkan::image_t> createDepthImage(
        VkPhysicalDevice physical_device,
        my_vulkan::device_t& logical_device,
        my_vulkan::command_pool_t& commandPool,
        VkFormat format,
        VkExtent2D extent
    )
    {
        std::unique_ptr<my_vulkan::image_t> result{new my_vulkan::image_t{
            logical_device,
            {extent.width, extent.height, 1},
            format,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        }};
        transitionImageLayout(
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

    static std::unique_ptr<my_vulkan::image_t> createTextureImage(
        VkPhysicalDevice physical_device,
        my_vulkan::device_t& logical_device,
        my_vulkan::command_pool_t& commandPool
    )
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("../texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        my_vulkan::buffer_t staging_buffer{
            logical_device,
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        void* data;
        vkMapMemory(logical_device.get(), staging_buffer.memory()->get(), 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logical_device.get(), staging_buffer.memory()->get());

        stbi_image_free(pixels);


        std::unique_ptr<my_vulkan::image_t> result{new my_vulkan::image_t{
            logical_device,
            {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        }};

        transitionImageLayout(
            commandPool,
            *result,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        copyBufferToImage(
            logical_device,
            commandPool,
            staging_buffer.get(),
            result->get(),
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight)
        );
        transitionImageLayout(
            commandPool,
            *result,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
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
        my_vulkan::command_pool_t& command_pool,
        my_vulkan::image_t& image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    ) 
    {
        auto command_buffer = command_pool.make_buffer();
        auto command_scope = command_buffer.begin(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        );

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.get();

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(image.format())) {
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
            0,
            {},
            {},
            {barrier}
        );
        command_pool.queue().submit(command_buffer.get());
        command_pool.queue().wait_idle();
    }

    static void copyBufferToImage(
        my_vulkan::device_t& logical_device,
        my_vulkan::command_pool_t& command_pool,
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height
    ) 
    {
        auto command_buffer = command_pool.make_buffer();
        auto command_scope = command_buffer.begin(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        );

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
        command_scope.copy(
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            {region}
        );
        command_pool.queue().submit(command_buffer.get());
        command_pool.queue().wait_idle();
    }

    static my_vulkan::buffer_t createVertexBuffer(
        my_vulkan::device_t& logical_device,
        my_vulkan::command_pool_t& command_pool
    )
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        my_vulkan::buffer_t staging_buffer{
            logical_device,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        void* data;
        vkMapMemory(logical_device.get(), staging_buffer.memory()->get(), 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(logical_device.get(), staging_buffer.memory()->get());

        my_vulkan::buffer_t result{
            logical_device,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };

        copyBuffer(command_pool, staging_buffer.get(), result.get(), bufferSize);
        return result;
    }

    static my_vulkan::buffer_t createIndexBuffer(
        my_vulkan::device_t& logical_device,
        my_vulkan::command_pool_t& command_pool
    )
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        my_vulkan::buffer_t staging_buffer{
            logical_device,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        void* data;
        vkMapMemory(logical_device.get(), staging_buffer.memory()->get(), 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(logical_device.get(), staging_buffer.memory()->get());

        my_vulkan::buffer_t result{
            logical_device,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };

        copyBuffer(command_pool, staging_buffer.get(), result.get(), bufferSize);
        return result;
    }

    static std::vector<my_vulkan::buffer_t> createUniformBuffers(
        my_vulkan::device_t& device,
        size_t n
    )
    {
        const VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        std::vector<my_vulkan::buffer_t> result;
        for (size_t i = 0; i < n; i++) {
            result.emplace_back(
                device,
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
        return result;
    }

    static std::vector<my_vulkan::descriptor_set_t> createDescriptorSets(
        my_vulkan::descriptor_pool_t& descriptor_pool,
        std::vector<my_vulkan::buffer_t>& uniform_buffers,
        VkImageView textureImageView,
        VkSampler textureSampler,
        const VkDescriptorSetLayout& layout,
        uint32_t size
    )
    {
        std::vector<my_vulkan::descriptor_set_t> result;
        for (size_t i = 0; i < size; i++)
        {
            auto descriptor_set = descriptor_pool.make_descriptor_set(layout);
            descriptor_set.update_buffer_write(
                0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                {{uniform_buffers[i].get(), 0, sizeof(UniformBufferObject)}}
            );
            descriptor_set.update_combined_image_sampler_write(
                1,
                {{
                    textureSampler,
                    textureImageView,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }}
            );
            result.push_back(std::move(descriptor_set));
        }
        return result;
    }

    static void copyBuffer(
        my_vulkan::command_pool_t& command_pool,
        VkBuffer srcBuffer,
        VkBuffer dstBuffer,
        VkDeviceSize size
    ) 
    {
        auto command_buffer = command_pool.make_buffer();
        command_buffer.begin(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        ).copy(srcBuffer, dstBuffer, {{0, 0, size}});
        command_pool.queue().submit(command_buffer.get());
        command_pool.queue().wait_idle();
    }

    static std::vector<my_vulkan::command_buffer_t> createCommandBuffers(
        my_vulkan::command_pool_t& command_pool,
        VkRenderPass renderPass,
        std::vector<VkFramebuffer>& swapChainFramebuffers,
        VkExtent2D extent, // swap_chain.extent
        VkBuffer vertex_buffer, // vertex_buffer.buffer
        pipeline_t& graphicsPipeline,
        VkBuffer index_buffer, // index_buffer.buffer
        std::vector<my_vulkan::descriptor_set_t>& descriptorSets,
        size_t size
    )
    {
        std::vector<my_vulkan::command_buffer_t> command_buffers;
        for (size_t i = 0; i < size; ++i)
        {
            auto command_buffer = command_pool.make_buffer();
            auto command_scope = command_buffer.begin(
                VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
            );
            command_scope.begin_render_pass(
                renderPass, 
                swapChainFramebuffers[i],
                {{0, 0}, extent},
                {
                    {.color = {0.0f, 0.0f, 0.0f, 1.0f}},
                    {.depthStencil = {1.0f, 0}},
                },
                VK_SUBPASS_CONTENTS_INLINE
            );
            command_scope.bind_pipeline(
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline.pipeline
            );
            command_scope.bind_vertex_buffers(
                {{vertex_buffer, 0}}
            );
            command_scope.bind_index_buffer(
                index_buffer,
                VK_INDEX_TYPE_UINT16
            );
            command_scope.bind_descriptor_set(
                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                graphicsPipeline.layout,
                {descriptorSets[i].get()}
            );
            command_scope.draw_indexed(
                {0, static_cast<uint32_t>(indices.size())}
            );
            command_scope.end();
            command_buffers.push_back(std::move(command_buffer));
        }
        return command_buffers;
    }

    void updateUniformBuffer(VkDevice device, uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(
            currentTime - startTime
        ).count();

        UniformBufferObject ubo = {};
        ubo.model = glm::rotate(
            glm::mat4(1.0f),
            time * glm::radians(90.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        ubo.view = glm::lookAt(
            glm::vec3(2.0f, 2.0f, 2.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        ubo.proj = glm::perspective(
            glm::radians(45.0f),
            swap_chain.extent().width / (float) swap_chain.extent().height,
            0.1f,
            10.0f
        );
        ubo.proj[1][1] *= -1;

        void* data;
        auto& buffer = uniform_buffers[currentImage];
        vkMapMemory(device, buffer.memory()->get(), 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, buffer.memory()->get());
    }

    void draw_frame()
    {
        auto acquisition_failure = try_draw_frame();
        if (
            acquisition_failure == my_vulkan::acquisition_failure_t::out_of_date ||
            acquisition_failure == my_vulkan::acquisition_failure_t::suboptimal
        )
        {
            recreateSwapChain(logical_device);
            try_draw_frame();       
        }
    }

    boost::optional<my_vulkan::acquisition_failure_t> try_draw_frame()
    {
        auto& sync = frame_sync_points[currentFrame];
        sync.inFlight.wait();
        auto acquisition_result = swap_chain.acquire_next_image(
            sync.imageAvailable.get()
        );
        if (acquisition_result.failure)
            return acquisition_result.failure;
        uint32_t image_index = *acquisition_result.image_index;
        sync.inFlight.reset();
        updateUniformBuffer(logical_device.get(), image_index);
        logical_device.graphics_queue().submit(
            commandBuffers[image_index].get(),
            {{
                sync.imageAvailable.get(),
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            }},
            {sync.renderFinished.get()},
            sync.inFlight.get()
        );
        if (auto presentation_failure = logical_device.present_queue().present(
            {swap_chain.get(), image_index},
            {sync.renderFinished.get()}
        ))
            return presentation_failure;
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        return boost::none;
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

    static bool isDeviceSuitable(
        VkPhysicalDevice device,
        VkSurfaceKHR surface,
        std::vector<const char*> deviceExtensions
    )
    {
        auto indices = my_vulkan::findQueueFamilies(device, surface);
        if (!indices.isComplete())
            return false;
        if (checkDeviceExtensionSupport(device, deviceExtensions))
        {
            auto swapChainSupport = my_vulkan::query_swap_chain_support(device, surface);
            if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
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
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &extensionCount,
            availableExtensions.data()
        );

        std::set<std::string> requiredExtensions(
            deviceExtensions.begin(),
            deviceExtensions.end()
        );

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
