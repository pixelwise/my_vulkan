#include <iostream>
#include "offscreen_render_target.hpp"
#include "../vector_helpers.hpp"
namespace my_vulkan
{
    namespace helpers
    {
        offscreen_render_target_t::offscreen_render_target_t(
            device_t& device,
            VkFormat color_format,
            VkExtent2D size,
            bool need_readback,
            size_t depth,
            std::optional<VkExternalMemoryHandleTypeFlagBits> external_handle_types,
            std::vector<sync_points_t> sync_points_list
        )
        : _size{size}
        , _external_mem_handle_types{external_handle_types}
        {
            std::cerr << "offscreen_render_target_t()" << this << " with device_t:" << &device << "\n";
            bool has_sync_points = !sync_points_list.empty();
            if (has_sync_points && sync_points_list.size() != depth)
                throw std::runtime_error("Number of input sync points must equal to depth.");
            auto in_sync_points_list = std::move(sync_points_list);

            for (size_t i = 0; i < depth; ++i)
                _color_buffers.emplace_back(
                    device,
                    size,
                    color_format,
                    _external_mem_handle_types
                );
            for (size_t i = 0; i < depth; ++i)
            {
                slot_t::begin_callback_t begin_callback;
                slot_t::end_callback_t end_callback;
                if (need_readback)
                {
                    begin_callback = [
                        &image = _color_buffers[i].image
                    ](
                        command_buffer_t::scope_t &commands
                    )
                    {
                        commands.pipeline_barrier(
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            {VkImageMemoryBarrier{
                                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr,
                                0,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_QUEUE_FAMILY_IGNORED,
                                VK_QUEUE_FAMILY_IGNORED,
                                image.get(),
                                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                            }}
                        );
                    };
                    end_callback = [
                        &image = _color_buffers[i].image
                        //size = size
                    ](
                        command_buffer_t::scope_t &commands,
                        buffer_t* readback_buffer
                    )
                    {
                        commands.pipeline_barrier(
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            {VkImageMemoryBarrier{
                                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                .dstAccessMask = (VK_ACCESS_TRANSFER_READ_BIT |
                                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                                  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT),
                                .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .image = image.get(),
                                .subresourceRange = {
                                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                        .baseMipLevel = 0,
                                        .levelCount = 1,
                                        .baseArrayLayer = 0,
                                        .layerCount = 1
                                }
                            }}
                        );
                        image.copy_to(
                            readback_buffer->get(),
                            commands
                        );
                        commands.pipeline_barrier(
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            {VkImageMemoryBarrier{
                                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                .srcAccessMask = 0,
                                .dstAccessMask = 0,
                                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                .image = image.get(),
                                .subresourceRange = {
                                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                        .baseMipLevel = 0,
                                        .levelCount = 1,
                                        .baseArrayLayer = 0,
                                        .layerCount = 1
                                }
                            }}
                        );
                        commands.pipeline_barrier(
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_HOST_BIT,
                            {VkBufferMemoryBarrier{
                                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                                    .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
                                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .buffer = readback_buffer->get(),
                                    .offset = 0,
                                    .size = VK_WHOLE_SIZE
                            }}
                        );
                    };
                }
                else
                {
                    end_callback =
                        [
                            &image = _color_buffers[i].image,
                            do_transition = bool{!external_handle_types}
                        ](
                            command_buffer_t::scope_t& commands,
                            buffer_t* //readback buffer
                        ) {
                            if (do_transition)
                                image.transition_layout(
                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    commands
                                );
                        };
                }
                _slots.emplace_back(
                    device,
                    device.graphics_queue(),
                    size,
                    _color_buffers[i].view.get(),
                    need_readback,
                    begin_callback,
                    end_callback,
                    has_sync_points ?
                        std::move(in_sync_points_list[i])
                        : sync_points_t{{}, {}}
                );
                _textures.push_back({
                    _color_buffers[i].sampler.get(),
                    _color_buffers[i].view.get(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                });
            }
        }

        offscreen_render_target_t::offscreen_render_target_t(
            device_t& device,
            std::vector<VkImageView> color_views,
            VkExtent2D size
        )
        : _size{size}
        {
            for (size_t i = 0; i < color_views.size(); ++i)
            {
                _slots.emplace_back(
                    device,
                    device.graphics_queue(),
                    size,
                    color_views[i],
                    false
                );
            }            
        }

        offscreen_render_target_t::color_buffer_t::color_buffer_t(
            device_t& device,
            VkExtent2D size,
            VkFormat color_format,
            std::optional<VkExternalMemoryHandleTypeFlagBits> external_handle_types
        )
        : image{
            device,
            {size.width, size.height, 1},
            color_format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_TILING_OPTIMAL,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            external_handle_types
        }
        , view{image.view()}
        , sampler{device.get()}
        {
        }

        render_target_t offscreen_render_target_t::render_target()
        {
            return render_target(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
        }

        render_target_t offscreen_render_target_t::render_target(VkExternalMemoryHandleTypeFlagBits external_mem_type)
        {
            return {
                [external_mem_type, this](VkRect2D rect){
                    auto scope = begin_phase(rect);
                    return render_scope_t{
                        scope.commands,
                        scope.index,
                        scope.color_view,
                        _size,
                        rect,
                        _external_mem_handle_types ? _color_buffers[scope.index].image.memory()->external_info(external_mem_type) : std::nullopt
                    };
                },
                [&](auto waits, auto signals) {
                    end_phase(std::move(waits), std::move(signals));
                },
                _size,
                depth(),
                false
            };
        }

        VkDescriptorImageInfo offscreen_render_target_t::texture(size_t phase)
        {
            return _textures.at(phase);
        }

        size_t offscreen_render_target_t::depth() const
        {
            return _slots.size();
        }

        offscreen_render_target_t::phase_context_t
        offscreen_render_target_t::begin_phase(std::optional<VkRect2D> rect, VkCommandBufferUsageFlags flags)
        {
            return _slots[_write_slot].begin(_write_slot, rect.value_or(VkRect2D{{0, 0}, size()}), flags);
        }

        void offscreen_render_target_t::end_phase(
            std::vector<queue_reference_t::wait_semaphore_info_t> waits,
            std::vector<VkSemaphore> signals
        )
        {
            _slots[_write_slot].finish(std::move(waits), std::move(signals));
            ++_write_slot;
            _num_slots_filled = std::min(depth(), _num_slots_filled + 1);
            if (_write_slot == _slots.size())
                _write_slot = 0;
        }

        std::optional<cv::Mat4b> offscreen_render_target_t::read_bgra(bool flush)
        {
            if (auto read_slot = consume_read_slot(flush))
                return _slots[*read_slot].read_bgra();
            else
                return std::nullopt;
        }

        std::optional<size_t> offscreen_render_target_t::consume_read_slot(bool flush)
        {
            size_t num_slots = _slots.size();
            size_t slots_required = flush ? 1 : num_slots;
            if (_num_slots_filled < slots_required)
                return std::nullopt;
            size_t read_slot = (_write_slot + num_slots - _num_slots_filled) % num_slots;
            --_num_slots_filled;
            return read_slot;
        }

        VkExtent2D offscreen_render_target_t::size()
        {
            return _size;
        }

        offscreen_render_target_t::slot_t::slot_t(
            device_t& device,
            queue_reference_t& queue,
            VkExtent2D extent,
            VkImageView color_view,
            bool need_readback,
            begin_callback_t begin_callback,
            end_callback_t end_callback,
            sync_points_t sync_points
        )
        : _queue{&queue}
        , _extent{extent}
        , _readback_buffer{
            need_readback ?
            new buffer_t{
                device,
                4 * extent.width * extent.height,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            } :
            nullptr
        }
        , _need_invalidate{
            _readback_buffer &&
            (
                _readback_buffer->memory_properties() &
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ) == 0
        }
        , _mapping{
            _readback_buffer ?
            new device_memory_t::mapping_t{
                _readback_buffer->memory()->map()
            } :
            nullptr
        }
        , _fence{device.get(), VK_FENCE_CREATE_SIGNALED_BIT}
        , _command_pool{device.get(), queue}
        , _command_buffer{_command_pool.make_buffer()}
        , _begin_callback{std::move(begin_callback)}
        , _end_callback{std::move(end_callback)}
        , _sync_points{std::move(sync_points)}
        , _color_view{color_view}
        {
        }

        cv::Mat4b offscreen_render_target_t::slot_t::read_bgra()
        {
            if (!_readback_buffer)
                throw std::runtime_error{"no readback enabled"};
            _fence.wait();
            if (_need_invalidate)
                _mapping->invalidate();
            auto data = ((const unsigned char*)_mapping->data());
            return cv::Mat4b{
                int(_extent.height),
                int(_extent.width),
                (cv::Vec4b*)data,
                4 * _extent.width
            };
        }

        offscreen_render_target_t::phase_context_t
        offscreen_render_target_t::slot_t::begin(size_t index, VkRect2D rect, VkCommandBufferUsageFlags flags)
        {
            if (_commands)
                throw std::runtime_error{
                    "double begin in vulkan::offscreen_render_target_t"
                };
            _fence.wait();
            _fence.reset();
            _commands = _command_buffer.begin(flags);
            if (_begin_callback)
                _begin_callback(*_commands);
            return {
                index,
                &*_commands,
                _color_view,
            };
        }

        void offscreen_render_target_t::slot_t::finish(
            std::vector<queue_reference_t::wait_semaphore_info_t> waits,
            std::vector<VkSemaphore> signals
        )
        {
            if (!_commands)
                throw std::runtime_error{
                    "finish before begin in vulkan::offscreen_render_target_t"
                };
            if (_end_callback)
                _end_callback(*_commands, _readback_buffer.get());
            _commands.reset();
            auto in_waits = std::move(waits);
            auto in_signals = std::move(signals);
            extend_vector(in_waits, _sync_points.waits);
            extend_vector(in_signals, _sync_points.signals);
            _queue->submit(
                _command_buffer.get(),
                std::move(in_waits),
                std::move(in_signals),
                _fence.get()
            );
        }
    }
}
