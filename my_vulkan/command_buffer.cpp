#include "command_buffer.hpp"
#include "utils.hpp"
#include <utility>

namespace my_vulkan
{
    command_buffer_t::command_buffer_t(
        VkDevice device,
        VkCommandPool command_pool,
        VkCommandBufferLevel level
    )
    : _device{device}
    , _command_pool{command_pool}
    {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = level;
        allocInfo.commandPool = command_pool;
        allocInfo.commandBufferCount = 1;
        vk_require(
            vkAllocateCommandBuffers(device, &allocInfo, &_command_buffer),
            "allocating command buffer"
        );
    }

    command_buffer_t::command_buffer_t(command_buffer_t&& other) noexcept
    : _device{0}
    {
        *this = std::move(other);
    }

    command_buffer_t& command_buffer_t::operator=(command_buffer_t&& other) noexcept
    {
        cleanup();
        _command_buffer = other._command_buffer;
        _command_pool = other._command_pool;
        std::swap(_device, other._device);
        return *this;
    }

    VkCommandBuffer command_buffer_t::get()
    {
        return _command_buffer;
    }

    VkDevice command_buffer_t::device()
    {
        return _device;
    }

    command_buffer_t::scope_t command_buffer_t::begin(VkCommandBufferUsageFlags flags)
    {
        return scope_t{_command_buffer, flags};
    }

    command_buffer_t::~command_buffer_t()
    {
        cleanup();
    }

    void command_buffer_t::cleanup()
    {
        if (_device)
        {
            vkFreeCommandBuffers(_device, _command_pool, 1, &_command_buffer);
            _device = 0;
        }
    }

    command_buffer_t::scope_t::scope_t(
        VkCommandBuffer command_buffer,
        VkCommandBufferUsageFlags flags
    )
    : _command_buffer{command_buffer}
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;
        vk_require(
            vkBeginCommandBuffer(_command_buffer, &beginInfo),
            "begining command buffer recording"
        );
    }

    command_buffer_t::scope_t::scope_t(scope_t&& other) noexcept
    : _command_buffer{0}
    {
        *this = std::move(other);
    }
 
    command_buffer_t::scope_t& command_buffer_t::scope_t::operator=(
        scope_t&& other
    ) noexcept
    {
        end();
        std::swap(_command_buffer, other._command_buffer);
        return *this;
    }

    void command_buffer_t::scope_t::begin_render_pass(
        VkRenderPass renderPass,
        VkFramebuffer framebuffer,
        VkRect2D render_area,
        std::vector<VkClearValue> clear_values,
        VkSubpassContents contents
    )
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = renderPass;
        info.framebuffer = framebuffer;
        info.renderArea = render_area;
        info.clearValueCount = uint32_t(clear_values.size());
        info.pClearValues = clear_values.data();
        vkCmdBeginRenderPass(_command_buffer, &info, contents);
    }

    void command_buffer_t::scope_t::set_viewport(
        std::vector<VkViewport> viewports,
        uint32_t first
    )
    {
        vkCmdSetViewport(
            _command_buffer,
            first,
            uint32_t(viewports.size()),
            viewports.data()
        );
    }

    void command_buffer_t::scope_t::set_scissor(
        std::vector<VkRect2D> scissors,
        uint32_t first
    )
    {
        vkCmdSetScissor(
            _command_buffer,
            first,
            uint32_t(scissors.size()),
            scissors.data()
        );
    }

    void command_buffer_t::scope_t::clear(
        std::vector<VkClearAttachment> attachements,
        std::vector<VkClearRect> rects
    )
    {
        vkCmdClearAttachments(
            _command_buffer,
            uint32_t(attachements.size()),
            attachements.data(),
            uint32_t(rects.size()),
            rects.data()
        );
    }

    void command_buffer_t::scope_t::bind_pipeline(
        VkPipelineBindPoint bind_point,
        VkPipeline pipeline,
        boost::optional<VkRect2D> target_rect
    )
    {
        vkCmdBindPipeline(
            _command_buffer, 
            bind_point, 
            pipeline
        );
        if (target_rect)
        {
            VkViewport viewport{
                (float)target_rect->offset.x,
                (float)target_rect->offset.y,
                (float)target_rect->extent.width,
                (float)target_rect->extent.height,
                0,
                1
            };
            vkCmdSetViewport(
                _command_buffer,
                0,
                1,
                &viewport
            );
            vkCmdSetScissor(
                _command_buffer,
                0,
                1,
                &*target_rect
            );
        }
    }

    void command_buffer_t::scope_t::bind_vertex_buffers(
        std::vector<buffer_binding_t> bindings,
        uint32_t offset
    )
    {
        // transpose AoS->SoA
        std::vector<VkBuffer> buffers;
        std::vector<VkDeviceSize> offsets;
        for (size_t i = 0; i < bindings.size(); ++i)
        {
            buffers.push_back(bindings[i].buffer);
            offsets.push_back(bindings[i].offset);
        }
        vkCmdBindVertexBuffers(
            _command_buffer,
            offset,
            uint32_t(bindings.size()),
            buffers.data(),
            offsets.data()
        );
    }

    void command_buffer_t::scope_t::bind_index_buffer(
        VkBuffer buffer,
        VkIndexType type,
        size_t offset
    )
    {
        vkCmdBindIndexBuffer(_command_buffer, buffer, offset, type);        
    }

    void command_buffer_t::scope_t::bind_descriptor_set(
        VkPipelineBindPoint bind_point,
        VkPipelineLayout layout,
        std::vector<VkDescriptorSet> descriptors,
        uint32_t offset,
        std::vector<uint32_t> dynamic_offset
    )
    {
        vkCmdBindDescriptorSets(
            _command_buffer,
            bind_point,
            layout,
            0, 
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data(),
            static_cast<uint32_t>(dynamic_offset.size()),
            dynamic_offset.data()
        );
    }

    void command_buffer_t::scope_t::draw(
        index_range_t index_range,
        index_range_t instance_range
    )
    {
        vkCmdDraw(
            _command_buffer,
            index_range.count,
            instance_range.count,
            index_range.offset,
            instance_range.offset
        );
    }

    void command_buffer_t::scope_t::draw_indexed(
        index_range_t index_range,
        uint32_t vertex_offset,
        index_range_t instance_range
    )
    {
        vkCmdDrawIndexed(
            _command_buffer,
            index_range.count,
            instance_range.count,
            index_range.offset,
            vertex_offset,
            instance_range.offset
        );
    }

    void command_buffer_t::scope_t::end_render_pass()
    {
        vkCmdEndRenderPass(_command_buffer);
    }

    void command_buffer_t::scope_t::pipeline_barrier(
        VkPipelineStageFlags src_stage_mask,
        VkPipelineStageFlags dst_stage_mask,
        std::vector<VkMemoryBarrier> barriers,
        VkDependencyFlags dependency_flags
    )
    {
        pipeline_barrier(
            src_stage_mask,
            dst_stage_mask,
            std::move(barriers),
            {},
            {},
            dependency_flags
        );   
    }

    void command_buffer_t::scope_t::pipeline_barrier(
        VkPipelineStageFlags src_stage_mask,
        VkPipelineStageFlags dst_stage_mask,
        std::vector<VkBufferMemoryBarrier> barriers,
        VkDependencyFlags dependency_flags
    )
    {
        pipeline_barrier(
            src_stage_mask,
            dst_stage_mask,
            {},
            std::move(barriers),
            {},
            dependency_flags
        );   
    }

    void command_buffer_t::scope_t::pipeline_barrier(
        VkPipelineStageFlags src_stage_mask,
        VkPipelineStageFlags dst_stage_mask,
        std::vector<VkImageMemoryBarrier> barriers,
        VkDependencyFlags dependency_flags
    )
    {
        pipeline_barrier(
            src_stage_mask,
            dst_stage_mask,
            {},
            {},
            std::move(barriers),
            dependency_flags
        );   
    }

    void command_buffer_t::scope_t::pipeline_barrier(
        VkPipelineStageFlags src_stage_mask,
        VkPipelineStageFlags dst_stage_mask,
        std::vector<VkMemoryBarrier> memory_barriers,
        std::vector<VkBufferMemoryBarrier> buffer_barriers,
        std::vector<VkImageMemoryBarrier> image_barriers,
        VkDependencyFlags dependency_flags
    )
    {
        vkCmdPipelineBarrier(
            _command_buffer,
            src_stage_mask,
            dst_stage_mask,
            dependency_flags,
            uint32_t(memory_barriers.size()),
            memory_barriers.data(),
            uint32_t(buffer_barriers.size()),
            buffer_barriers.data(),
            uint32_t(image_barriers.size()),
            image_barriers.data()
        );
    }

    void command_buffer_t::scope_t::copy(
        VkBuffer src,
        VkBuffer dst,
        std::vector<VkBufferCopy> operations
    )
    {
        vkCmdCopyBuffer(
            _command_buffer,
            src,
            dst,
            uint32_t(operations.size()),
            operations.data()
        );
    }

    void command_buffer_t::scope_t::copy(
        VkBuffer src,
        VkImage dst,
        VkImageLayout dst_layout,
        std::vector<VkBufferImageCopy> operations
    )
    {
        vkCmdCopyBufferToImage(
            _command_buffer,
            src,
            dst,
            dst_layout,
            uint32_t(operations.size()),
            operations.data()
        );
    }

    void command_buffer_t::scope_t::copy(
        VkImage src,
        VkImageLayout src_layout,
        VkImage dst,
        VkImageLayout dst_layout,
        std::vector<VkImageCopy> operations
    )
    {
        vkCmdCopyImage(
            _command_buffer,
            src,
            src_layout,
            dst,
            dst_layout,
            uint32_t(operations.size()),
            operations.data()
        );
    }

    void command_buffer_t::scope_t::blit(
        VkImage src,
        VkImageLayout src_layout,
        VkImage dst,
        VkImageLayout dst_layout,
        std::vector<VkImageBlit> operations,
        VkFilter filter
    )
    {
        vkCmdBlitImage(
            _command_buffer,
            src,
            src_layout,
            dst,
            dst_layout,
            uint32_t(operations.size()),
            operations.data(),
            filter
        );
    }

    command_buffer_t::scope_t::~scope_t()
    {
        end();
    }

    void command_buffer_t::scope_t::end()
    {
        if (_command_buffer)
        {
            vkEndCommandBuffer(_command_buffer);
            _command_buffer = 0;
        }
    }

    void command_buffer_t::reset()
    {
        vk_require(
            vkResetCommandBuffer(get(), 0),
            "reetting command buffer"
        );
    }
}
