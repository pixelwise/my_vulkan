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
    , surface{instance.get(), window}
    , physical_device{my_vulkan::pick_physical_device(
        instance.get(),
        surface.get(),
        deviceExtensions
    )}
    , queue_indices{my_vulkan::find_queue_families(
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
        my_vulkan::find_depth_format(physical_device)
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
    , texture_sampler{
        logical_device.get()
    }
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
        texture_sampler.get(),
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
    my_vulkan::texture_sampler_t texture_sampler;
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
