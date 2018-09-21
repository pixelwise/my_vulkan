#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "my_vulkan/my_vulkan.hpp"
#include "my_vulkan/helpers/standard_swap_chain.hpp"

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
    auto func =
        (PFN_vkCreateDebugUtilsMessengerEXT) 
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
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
    auto func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
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

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

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

    static my_vulkan::vertex_layout_t layout()
    {
        return {
            getBindingDescription(),
            getAttributeDescriptions()
        };
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

struct texture_sampler_t
{
private:
};

struct framebuffer_t
{
private:
};


struct frame_sync_points_t
{
    frame_sync_points_t(VkDevice device)
    : imageAvailable{device}
    , renderFinished{device}
    , inFlight{device}
    {}
    my_vulkan::semaphore_t imageAvailable;
    my_vulkan::semaphore_t renderFinished;
    my_vulkan::fence_t inFlight;
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
    , depth_format{
        findDepthFormat(physical_device)
    }
    , swap_chain{new my_vulkan::helpers::standard_swap_chain_t{
        logical_device,
        surface.get(),
        queue_indices,
        window_extent(),
        depth_format
    }}

    , vertex_buffer{createVertexBuffer(
        logical_device,
        swap_chain->command_pool()
    )}
    , index_buffer{createIndexBuffer(
        logical_device,
        swap_chain->command_pool()
    )}
    , texture_image{createTextureImage(
        logical_device,
        swap_chain->command_pool()
    )}
    , uniform_layout{
        {
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            0
        }, {
            1,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0
        }
    }
    , graphics_pipeline{
        logical_device.get(),
        swap_chain->extent(),
        swap_chain->render_pass(),
        uniform_layout,
        Vertex::layout(),
        readFile("shaders/26_shader_depth.vert.spv"),
        readFile("shaders/26_shader_depth.frag.spv")
    }
    , texture_view{texture_image.view(VK_IMAGE_ASPECT_COLOR_BIT)}
    , texture_sampler{createTextureSampler(
        logical_device.get()
    )}
    , uniform_buffers{createUniformBuffers(
        logical_device,
        swap_chain->depth()
    )}
    , descriptor_pool{
        logical_device.get(),
        swap_chain->depth()
    }
    , descriptor_sets{createDescriptorSets(
        descriptor_pool,
        uniform_buffers,
        texture_view.get(),
        texture_sampler,
        graphics_pipeline.uniform_layout()
    )}
    {
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
    
    VkFormat depth_format;
    std::unique_ptr<my_vulkan::helpers::standard_swap_chain_t> swap_chain;

    my_vulkan::buffer_t vertex_buffer;
    my_vulkan::buffer_t index_buffer;    
    my_vulkan::image_t texture_image;
    std::vector<VkDescriptorSetLayoutBinding> uniform_layout;
    my_vulkan::graphics_pipeline_t graphics_pipeline;
    my_vulkan::image_view_t texture_view;
    VkSampler texture_sampler;
    std::vector<my_vulkan::buffer_t> uniform_buffers;
    my_vulkan::descriptor_pool_t descriptor_pool;
    std::vector<my_vulkan::descriptor_set_t> descriptor_sets;

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

    void cleanup()
    {
        vkDestroySampler(logical_device.get(), texture_sampler, nullptr);
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
        logical_device.wait_idle();
        swap_chain.reset(new my_vulkan::helpers::standard_swap_chain_t{
            logical_device,
            surface.get(),
            queue_indices,
            window_extent(),
            depth_format
        });
        graphics_pipeline = my_vulkan::graphics_pipeline_t{
            logical_device.get(),
            swap_chain->extent(),
            swap_chain->render_pass(),
            uniform_layout,
            Vertex::layout(),
            readFile("shaders/26_shader_depth.vert.spv"),
            readFile("shaders/26_shader_depth.frag.spv")
        };
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
        my_vulkan::vk_require(
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

    static bool hasStencilComponent(VkFormat format)
    {
        return
            format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    static my_vulkan::image_t createTextureImage(
        my_vulkan::device_t& logical_device,
        my_vulkan::command_pool_t& command_pool
    )
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(
            "../texture.jpg",
            &texWidth,
            &texHeight,
            &texChannels,
            STBI_rgb_alpha
        );
        if (!pixels)
            throw std::runtime_error("failed to load texture image!");
        my_vulkan::image_t result{
            logical_device,
            {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
            VK_FORMAT_R8G8B8A8_UNORM
        };
        result.load_pixels(command_pool, pixels);
        stbi_image_free(pixels);
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

    static my_vulkan::buffer_t createVertexBuffer(
        my_vulkan::device_t& logical_device,
        my_vulkan::command_pool_t& command_pool
    )
    {
        my_vulkan::buffer_t result{
            logical_device,
            sizeof(vertices[0]) * vertices.size(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };
        result.load_data(command_pool, vertices.data());
        return result;
    }

    static my_vulkan::buffer_t createIndexBuffer(
        my_vulkan::device_t& logical_device,
        my_vulkan::command_pool_t& command_pool
    )
    {
        my_vulkan::buffer_t result{
            logical_device,
            sizeof(indices[0]) * indices.size(),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };
        result.load_data(command_pool, indices.data());
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
        VkImageView texture_view,
        VkSampler texture_sampler,
        const VkDescriptorSetLayout& layout
    )
    {
        std::vector<my_vulkan::descriptor_set_t> result;
        for (size_t i = 0; i < uniform_buffers.size(); i++)
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
                    texture_sampler,
                    texture_view,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }}
            );
            result.push_back(std::move(descriptor_set));
        }
        return result;
    }

    static void draw_commands(
        my_vulkan::command_buffer_t::scope_t& command_scope,
        VkBuffer vertex_buffer, // vertex_buffer.buffer
        my_vulkan::graphics_pipeline_t& graphics_pipeline,
        VkBuffer index_buffer, // index_buffer.buffer
        my_vulkan::descriptor_set_t& descriptor_set
    )
    {
        command_scope.bind_pipeline(
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphics_pipeline.get()
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
            graphics_pipeline.layout(),
            {descriptor_set.get()}
        );
        command_scope.draw_indexed(
            {0, static_cast<uint32_t>(indices.size())}
        );
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
            swap_chain->extent().width / (float) swap_chain->extent().height,
            0.1f,
            10.0f
        );
        ubo.proj[1][1] *= -1;
        uniform_buffers[currentImage].memory()->set_data(&ubo, sizeof(ubo));
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
        auto outcome = swap_chain->acquire(
            {{0, 0}, swap_chain->extent()},
            {
                {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                {.depthStencil = {1.0f, 0}},
            }
        );
        if (outcome.failure)
            return outcome.failure;
        auto& working_set = *outcome.working_set;
        updateUniformBuffer(logical_device.get(), working_set.phase());
        draw_commands(
            working_set.commands(),
            vertex_buffer.get(),
            graphics_pipeline,
            index_buffer.get(),
            descriptor_sets[working_set.phase()]
        );
        return working_set.finish();
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
