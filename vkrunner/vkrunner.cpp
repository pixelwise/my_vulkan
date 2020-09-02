#include <my_vulkan/shader_module.hpp>
#include <my_vulkan/utils.hpp>
#include <my_vulkan/instance.hpp>
#include <my_vulkan/device.hpp>
#include <my_vulkan/render_pass.hpp>
#include <my_vulkan/graphics_pipeline.hpp>
#include <my_vulkan/helpers/offscreen_render_target.hpp>

#include <opencv2/imgcodecs.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/io.hpp>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/process/child.hpp>
#include <boost/process/search_path.hpp>

#include <vector>
#include <string>
#include <istream>
#include <fstream>
#include <iostream>
#include <optional>

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

struct vkrunner_test_content_t
{
    struct section_t
    {
        std::string name;
        std::vector<std::string> lines;
    };
    std::vector<section_t> sections;
};

vkrunner_test_content_t parse_test(std::istream& input)
{
    vkrunner_test_content_t result;
    std::optional<vkrunner_test_content_t::section_t> current_section;
    for (std::string line; std::getline(input, line); )
    {
        trim(line);
        if (line.size() >= 2 && line.front() == '[' && line.back() == ']')
        {
            auto section_name = line.substr(1, line.size() - 2);
            if (current_section)
                result.sections.push_back(std::move(*current_section));
            current_section = vkrunner_test_content_t::section_t{section_name};
        }
        else if (current_section && !line.empty())
        {
            current_section->lines.push_back(line);
        }
    }
    if (current_section)
        result.sections.push_back(std::move(*current_section));
    return result;
}

static std::vector<uint32_t> vertex_shader_passthrough{
    0x07230203, 0x00010000, 0x00070000, 0x0000000c,
    0x00000000, 0x00020011, 0x00000001, 0x0003000e,
    0x00000000, 0x00000001, 0x0007000f, 0x00000000,
    0x00000001, 0x6e69616d, 0x00000000, 0x00000002,
    0x00000003, 0x00040047, 0x00000002, 0x0000001e,
    0x00000000, 0x00040047, 0x00000003, 0x0000000b,
    0x00000000, 0x00020013, 0x00000004, 0x00030021,
    0x00000005, 0x00000004, 0x00030016, 0x00000006,
    0x00000020, 0x00040017, 0x00000007, 0x00000006,
    0x00000004, 0x00040020, 0x00000008, 0x00000001,
    0x00000007, 0x00040020, 0x00000009, 0x00000003,
    0x00000007, 0x0004003b, 0x00000008, 0x00000002,
    0x00000001, 0x0004003b, 0x00000009, 0x00000003,
    0x00000003, 0x00050036, 0x00000004, 0x00000001,
    0x00000000, 0x00000005, 0x000200f8, 0x0000000a,
    0x0004003d, 0x00000007, 0x0000000b, 0x00000002,
    0x0003003e, 0x00000003, 0x0000000b, 0x000100fd,
    0x00010038
};

static std::vector<uint8_t> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("failed to open " + filename);
    size_t fileSize = (size_t) file.tellg();
    std::vector<uint8_t> buffer(fileSize);
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    return buffer;
}

my_vulkan::shader_module_t load_shader_source(
    VkDevice device,
    const std::vector<std::string>& lines,
    std::string extension,
    uint32_t version
)
{
    boost::filesystem::path temp_in = boost::filesystem::unique_path(
        "shader_source_%%%%-%%%%-%%%%-%%%%" + extension
    );
    boost::filesystem::path temp_out = boost::filesystem::unique_path(
        "shader_binary_%%%%-%%%%-%%%%-%%%%.spv"
    );
    std::ofstream source_file{temp_in.string()};
    for (auto& line : lines)
        source_file << line << std::endl;
    source_file.close();
    auto compiler_command =str(
        boost::format{"%s -V --target-env vulkan%s.%s -o %s %s"}
        % boost::process::search_path("glslangValidator").string()
        % VK_VERSION_MAJOR(version)
        % VK_VERSION_MINOR(version)
        % temp_out
        % temp_in
    );
    std::cout << compiler_command << std::endl;
    boost::process::child compiler{compiler_command};
    compiler.wait();
    if (compiler.exit_code() != 0)
        throw std::runtime_error{
            str(boost::format{"glsl compiler returned %s"} % compiler.exit_code())
        };
    auto spirv_data = readFile(temp_out.string());
    boost::filesystem::remove(temp_in);
    boost::filesystem::remove(temp_out);
    return my_vulkan::shader_module_t{device, spirv_data};
}

static std::vector<const char *> get_required_instance_extensions(
    bool enableValidationLayers, const std::vector<const char *> &extra_instance_extensions
)
{
    std::vector<const char *> extensions {
        extra_instance_extensions.begin(),
        extra_instance_extensions.end()
    };
    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    std::sort(extensions.begin(), extensions.end());
    extensions.erase(std::unique(extensions.begin(), extensions.end()), extensions.end());
    return extensions;
}

class base_setup_t
{
public:
    base_setup_t(size_t device_index = 0)
    : device_extensions{}
    , validation_layers{
        "VK_LAYER_KHRONOS_validation"
    }
    , instance{
        "vkrunner",
        get_required_instance_extensions(
            !validation_layers.empty(),
            {}
        ),
        {}
    }
    , physical_device{
        my_vulkan::pick_physical_device(
            device_index,
            instance.get(),
            nullptr,
            device_extensions
        )
    }
    , queue_indices{
        .graphics = my_vulkan::find_graphics_queue(physical_device),
        .present = 0,
        .transfer = my_vulkan::find_transfer_queue(physical_device)
    }
    , logical_device{
        physical_device,
        queue_indices,
        {},
        device_extensions
    }
    {
    }
    std::vector<const char *> device_extensions;
    std::vector<const char *> validation_layers;
    my_vulkan::instance_t instance;
    VkPhysicalDevice physical_device;
    my_vulkan::queue_family_indices_t queue_indices;
    my_vulkan::device_t logical_device;    
};

struct bits_t
{
    std::optional<my_vulkan::shader_module_t> vertex_shader;
    std::optional<my_vulkan::shader_module_t> fragment_shader;
    std::vector<std::string> test_script;
    // todo: parse/generate these
    VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent2D extent{800,800};
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkVertexInputBindingDescription vertex_binding = {
        .binding = 0,
        .stride = sizeof(glm::vec3),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    std::vector<VkVertexInputAttributeDescription> attributes{{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0,
    }};
};

struct rect_t
{
    glm::vec2 origin;
    glm::vec2 size;
};

my_vulkan::buffer_t draw_rect(
    base_setup_t& setup,
    bits_t& bits,
    my_vulkan::render_pass_t& render_pass,
    my_vulkan::command_buffer_t::scope_t& command_scope,
    rect_t rect
)
{
    my_vulkan::buffer_t vertex_buffer{
        setup.logical_device,
        4 * sizeof(glm::vec3),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    std::vector<glm::vec3> vertices{
        glm::vec3{rect.origin.x, rect.origin.y, 0},
        glm::vec3{rect.origin.x + rect.size.x, rect.origin.y, 0},
        glm::vec3{rect.origin.x, rect.origin.y + rect.size.y, 0},
        glm::vec3{rect.origin.x, rect.origin.y + rect.size.y, 0},
        glm::vec3{rect.origin.x + rect.size.x, rect.origin.y, 0},
        glm::vec3{rect.origin.x + rect.size.x, rect.origin.y + rect.size.y, 0},
    };
    vertex_buffer.memory()->set_data(
        vertices.data(),
        vertices.size() * sizeof(glm::vec3)
    );
    command_scope.bind_vertex_buffers(
        {{vertex_buffer.get(), 0}}
    );
    command_scope.draw({0, 6});
    return vertex_buffer;
}

std::vector<std::string> tokenize_script_command(const std::string& s)
{
    std::vector<std::string> result;
    size_t token_start = 0;
    size_t bracket_depth = 0;
    std::cout << "tokenizing " << s << std::endl;
    for (size_t i = 0; i < s.size(); ++i)
    {
        auto c = s[i];
        if (bracket_depth == 0 && std::isspace(c))
        {
            if (i != token_start)
            {
                result.push_back(s.substr(token_start, i - token_start));
                std::cout << "- '" << result.back() << "'" << std::endl;
            }
            token_start = i + 1;
        }
        else if (c == '(')
            ++bracket_depth;
        else if (c == ')')
            --bracket_depth;
    }
    if (bracket_depth == 0 && token_start < s.size())
    {
        result.push_back(s.substr(token_start));
        std::cout << "- '" << result.back() << "'" << std::endl;
    }
    return result;
}

std::vector<std::string> tokenize_bracketet_vector(const std::string& s)
{
    std::optional<size_t> number_start;
    std::vector<std::string> result;
    auto is_digit = [](char c){return c >= '0' && c <= '9';};
    if (!s.empty())
    {
        for (size_t i = 1; i < s.size() - 1; ++i)
        {
            auto c = s[i];
            bool is_number_part = is_digit(c) || c == '.';
            if (!number_start && is_number_part)
            {
                number_start = i;
            }
            else if (number_start && !is_number_part)
            {
                result.push_back(s.substr(*number_start, i - *number_start));
                number_start.reset();
            }
        } 
    } 
    if (number_start)
    {
        result.push_back(s.substr(*number_start, s.size() - 1 - *number_start));
    }
    return result;
}

glm::vec4 tokenize_bracketet_vec4(const std::string& s)
{
    auto parts = tokenize_bracketet_vector(s);
    if (parts.size() == 4)
        return glm::vec4{
            boost::lexical_cast<float>(parts[0]),
            boost::lexical_cast<float>(parts[1]),
            boost::lexical_cast<float>(parts[2]),
            boost::lexical_cast<float>(parts[3]),
        };
    else
        throw std::runtime_error{"no vec4"};
}

bool probe_rect(const cv::Mat4b& bgra, rect_t rect, glm::vec4 color, const std::string& colorspace)
{
    auto region = bgra(
        cv::Range{int(rect.origin.y), int(rect.origin.y + rect.size.y)},
        cv::Range{int(rect.origin.x), int(rect.origin.x + rect.size.x)}
    );
    cv::Vec4b bgra_color{
        uint8_t(color.x * 255),
        uint8_t(color.y * 255),
        uint8_t(color.z * 255),
        uint8_t(color.w * 255),
    };
    std::cout << " bgra color " << bgra_color << std::endl;
    for (auto pixel : region)
    {
        if (pixel != bgra_color)
        {
            std::cout << " pixel " << pixel << " != " << bgra_color << std::endl;
            return false;
        }
    }
    return true;
}

int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cout << "usage " << argv[0] << " <test file>" << std::endl;
        return -1;
    }
    std::ifstream input{argv[1]};
    auto test = parse_test(input);
    base_setup_t setup;
    bits_t bits;
    auto version = VK_MAKE_VERSION(1, 0, 2);
    for (auto&& section : test.sections)
    {
        std::cout << "section '" << section.name << "' with " << section.lines.size() << " lines " << std::endl;
        for (auto& line : section.lines)
        {
            std::cout << " " << line << std::endl;
        }
        if (section.name == "fragment shader")
            bits.fragment_shader = load_shader_source(
                setup.logical_device.get(),
                section.lines,
                ".frag",
                version
            );
        else if (section.name == "vertex shader")
            bits.vertex_shader = load_shader_source(
                setup.logical_device.get(),
                section.lines,
                ".vert",
                version
            );
        else if (section.name == "vertex shader passthrough")
            bits.vertex_shader = my_vulkan::shader_module_t{
                setup.logical_device.get(),
                reinterpret_cast<const char*>(vertex_shader_passthrough.data()),
                vertex_shader_passthrough.size() * 4
            };
        else if (section.name == "test")
            for (auto& line : section.lines)
                if (!line.empty() && line[0] != '#')
                {
                    std::cout << "test line " << line << std::endl;
                    bits.test_script.push_back(line);
                }
    }
    bool success = true;
    if (bits.vertex_shader && bits.fragment_shader)
    {
        std::cout << "beginning draw test" << std::endl;
        my_vulkan::helpers::offscreen_render_target_t target{
            setup.logical_device,
            bits.color_format,
            bits.extent,
            true, // need readback
            1 // depth
        };
        my_vulkan::render_pass_t render_pass{
            setup.logical_device.get(),
            bits.color_format,
            VK_FORMAT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ATTACHMENT_LOAD_OP_CLEAR
        };
        std::vector<VkDescriptorSetLayoutBinding> uniform_layout{
            {
                0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                1,
                VK_SHADER_STAGE_VERTEX_BIT,
                0
            },
            {
                0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                1,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0
            },
        };
        my_vulkan::graphics_pipeline_t graphics_pipeline{
            setup.logical_device.get(),
            bits.extent,
            render_pass.get(),
            0,
            uniform_layout,
            my_vulkan::vertex_layout_t{
                bits.vertex_binding,
                bits.attributes,
            },
            *bits.vertex_shader,
            *bits.fragment_shader,
            my_vulkan::render_settings_t{
                .topology = bits.topology
            },
            true
        };
        my_vulkan::framebuffer_t framebuffer{
            setup.logical_device.get(),
            render_pass.get(),
            {target.texture(0).imageView},
            bits.extent
        };
        std::vector<my_vulkan::buffer_t> buffers;
        size_t num_image = 0;
        std::optional<my_vulkan::helpers::offscreen_render_target_t::phase_context_t> current_scope;
        std::optional<cv::Mat4b> current_image;
        auto begin = [&]{
            current_image.reset();
            auto scope = target.begin_phase();
            VkRect2D target_rect{
                .offset = {0, 0},
                .extent = bits.extent
            };
            scope.commands->begin_render_pass(
                render_pass.get(),
                framebuffer.get(),
                target_rect,
                {{{{0.0f, 0.0f, 0.0f, 1.0f}}},}
            );
            scope.commands->bind_pipeline(
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphics_pipeline.get(),
                target_rect
            );
            current_scope = scope;
        };
        auto end = [&]{
            if (!current_scope)
                throw std::runtime_error{"no scope"};
            current_scope->commands->end_render_pass();
            current_scope.reset();
            target.end_phase();
            current_image = target.read_bgra(true/*flush*/);
            cv::imwrite(str(boost::format{"test_frame%07d.png"} % num_image++), *current_image);
            buffers.clear();
            if (!current_image)
                throw std::runtime_error{"readback failed"};
        };
        auto notify_draw = [&]{
            if (!current_scope)
                begin();
        };
        auto notify_probe = [&]{
            if (!current_image)
                end();
            if (!current_image)
                throw std::runtime_error{"no image to probe"};
        };
        for (auto& line : bits.test_script)
        {
            auto tokens = tokenize_script_command(line);
            if (tokens.size() == 6 && tokens[0] == "draw" && tokens[1] == "rect")
            {
                notify_draw();
                rect_t rect = {
                    glm::vec2{
                        boost::lexical_cast<float>(tokens[2]),
                        boost::lexical_cast<float>(tokens[3]),
                    },
                    glm::vec2{
                        boost::lexical_cast<float>(tokens[4]),
                        boost::lexical_cast<float>(tokens[5]),
                    }
                };
                std::cout << "draw rect " << rect.origin << " " << rect.size << std::endl;
                buffers.push_back(draw_rect(
                    setup,
                    bits,
                    render_pass,
                    *current_scope->commands,
                    rect
                ));
            }
            if (tokens.size() == 5 && tokens[0] == "probe" && tokens[1] == "rect")
            {
                notify_probe();
                auto colorspace = tokens[2];
                auto rect = tokenize_bracketet_vec4(tokens[3]);
                auto color = tokenize_bracketet_vec4(tokens[4]);
                std::cout << "probe rect " << colorspace << " " << rect << " " << color << std::endl;
                auto result = probe_rect(
                    *current_image,
                    rect_t{
                        glm::vec2{rect.x, rect.y},
                        glm::vec2{rect.z, rect.w},
                    },
                    color,
                    colorspace
                );
                if (result)
                    std::cout << "-> success" << std::endl;
                else
                    std::cout << "-> failure" << std::endl;
                success &= result;
            }
        }
    }
    if (success)
        return 0;
    else
        return -1;
}