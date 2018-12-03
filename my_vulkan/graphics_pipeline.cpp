#include "graphics_pipeline.hpp"

#include "utils.hpp"
#include "shader_module.hpp"

#include <iostream>

namespace my_vulkan
{
    graphics_pipeline_t::graphics_pipeline_t(
        VkDevice device,
        VkExtent2D extent,
        VkRenderPass render_pass,
        const std::vector<VkDescriptorSetLayoutBinding>& uniform_layout,
        vertex_layout_t vertex_layout,
        const std::vector<uint8_t>& vertex_shader,
        const std::vector<uint8_t>& fragment_shader,
        VkPrimitiveTopology topology,
        render_settings_t settings
    )
    : _device{device}
    , _uniform_layout{_device, uniform_layout}
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        auto uniform_layout_handle = _uniform_layout.get();
        pipelineLayoutInfo.pSetLayouts = &uniform_layout_handle;

        vk_require(
            vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_layout),
            "creating pipeline layout"
        );

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
        depthStencil.depthTestEnable = settings.depth_test ? VK_TRUE : VK_FALSE;
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
        switch(settings.blending)
        {
        case blending_t::add:
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            break;
        case blending_t::alpha:
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            break;
        case blending_t::none:
            colorBlendAttachment.blendEnable = VK_FALSE;
            break;
        }

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

        VkShaderModule vertShaderModule = createShaderModule(device, vertex_shader);
        VkShaderModule fragShaderModule = createShaderModule(device, fragment_shader);

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

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(vertex_layout.attributes.size());
        vertexInputInfo.pVertexBindingDescriptions = &vertex_layout.binding;
        vertexInputInfo.pVertexAttributeDescriptions =
            vertex_layout.attributes.data();


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
        pipelineInfo.layout = _layout;
        pipelineInfo.renderPass = render_pass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        vk_require(
            vkCreateGraphicsPipelines(
                device,
                VK_NULL_HANDLE,
                1,
                &pipelineInfo,
                nullptr,
                &_pipeline
            ),
            "creating pipeline"
        );
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    graphics_pipeline_t::graphics_pipeline_t(graphics_pipeline_t&& other) noexcept
    : _device{other._device}
    , _uniform_layout{std::move(other._uniform_layout)}
    {
        _pipeline = other._pipeline;
        _layout = other._layout;
        other._device = 0;        
    }

    graphics_pipeline_t& graphics_pipeline_t::operator=(
        graphics_pipeline_t&& other
    ) noexcept
    {
        cleanup();
        _pipeline = other._pipeline;
        _layout = other._layout;
        std::swap(_device, other._device);
        return *this;
    }

    graphics_pipeline_t::~graphics_pipeline_t()
    {
        cleanup();
    }

    void graphics_pipeline_t::cleanup()
    {
        if (_device)
        {
            vkDestroyPipeline(_device, _pipeline, 0);
            vkDestroyPipelineLayout(_device, _layout, 0);
            _device = 0;
        }
    }

    VkPipeline graphics_pipeline_t::get()
    {
        return _pipeline;
    }

    VkPipelineLayout graphics_pipeline_t::layout()
    {
        return _layout;
    }

    VkDescriptorSetLayout graphics_pipeline_t::uniform_layout()
    {
        return _uniform_layout.get();
    }

    VkDevice graphics_pipeline_t::device()
    {
        return _device;
    }
}
