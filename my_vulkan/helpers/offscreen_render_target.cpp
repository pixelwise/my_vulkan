#include "offscreen_render_target.hpp"

namespace my_vulkan
{
    namespace helpers
    {
        offscreen_render_target_t::offscreen_render_target_t(
            device_t& device,
            VkFormat color_format,
            VkExtent2D size,
            bool need_readback,
            size_t depth
        )
        : _size{size}
        {

            for (size_t i = 0; i < depth; ++i)
                _color_buffers.emplace_back(
                    device.get(),
                    device.physical_device(),
                    size,
                    color_format
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
                        image_t* readback_image
                    )
                    {
                        commands.pipeline_barrier(
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            {VkImageMemoryBarrier{
                                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_ACCESS_TRANSFER_READ_BIT,
                                VK_IMAGE_LAYOUT_UNDEFINED,//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_QUEUE_FAMILY_IGNORED,
                                VK_QUEUE_FAMILY_IGNORED,
                                image.get(),
                                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                            }}
                        );
                        commands.pipeline_barrier(
                            VK_PIPELINE_STAGE_HOST_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            {VkImageMemoryBarrier{
                                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr,
                                0,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_QUEUE_FAMILY_IGNORED,
                                VK_QUEUE_FAMILY_IGNORED,
                                readback_image->get(),
                                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                            }}
                        );
                        readback_image->copy_from(
                            image.get(),
                            commands
                        );
                        commands.pipeline_barrier(
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_HOST_BIT,
                            {VkImageMemoryBarrier{
                                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                nullptr,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_HOST_READ_BIT,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_GENERAL,
                                VK_QUEUE_FAMILY_IGNORED,
                                VK_QUEUE_FAMILY_IGNORED,
                                readback_image->get(),
                                VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                            }}
                        );
                    };
                }
                else
                {
                    end_callback =
                        [
                            &image = _color_buffers[i].image
                        ](
                            command_buffer_t::scope_t& commands,
                            image_t* //readback_image
                        ) {
                            image.transition_layout(
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                commands
                            );                        
                        };
                }
                _slots.emplace_back(
                    device.get(),
                    device.graphics_queue(),
                    size,
                    _color_buffers[i].view.get(),
                    need_readback,
                    device.physical_device(),
                    begin_callback,
                    end_callback
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
                    device.get(),
                    device.graphics_queue(),
                    size,
                    color_views[i],
                    false,
                    device.physical_device()
                );
            }            
        }

        offscreen_render_target_t::color_buffer_t::color_buffer_t(
            VkDevice device,
            VkPhysicalDevice physical_device,
            VkExtent2D size,
            VkFormat color_format
        )
        : image{
            device,
            physical_device,
            {size.width, size.height, 1},
            color_format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        }
        , view{image.view()}
        , sampler{device}
        {
        }

        render_target_t offscreen_render_target_t::render_target()
        {
            return {
                [&](VkRect2D rect){
                    auto scope = begin_phase(rect);
                    return render_scope_t{
                        scope.commands,
                        scope.index,
                        _color_buffers[scope.index].view.get(),
                        _size,
                    };
                },
                [&](auto waits, auto signals){
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
        offscreen_render_target_t::begin_phase(std::optional<VkRect2D> rect)
        {
            return _slots[_write_slot].begin(_write_slot, rect.value_or(VkRect2D{{0, 0}, size()}));
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
            VkDevice device,
            queue_reference_t& queue,
            VkExtent2D size,
            VkImageView color_view,
            bool need_readback,
            VkPhysicalDevice physical_device,
            begin_callback_t begin_callback,
            end_callback_t end_callback
        )
        : _queue{&queue}
        , _readback_image{
            need_readback ?
            new image_t{
                device,
                physical_device,
                {size.width, size.height, 1},
                VK_FORMAT_B8G8R8A8_UNORM,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_TILING_LINEAR,
                //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                //VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                VK_MEMORY_PROPERTY_HOST_CACHED_BIT
            } :
            nullptr
        }
        , _fence{device, VK_FENCE_CREATE_SIGNALED_BIT}
        , _command_pool{device, queue}
        , _command_buffer{_command_pool.make_buffer()}
        , _begin_callback{std::move(begin_callback)}
        , _end_callback{std::move(end_callback)}
        {
        }

        cv::Mat4b offscreen_render_target_t::slot_t::read_bgra()
        {
            if (!_readback_image)
                throw std::runtime_error{"no readback enabled"};
            _fence.wait();
            _mapping = _readback_image->memory()->map();
            _mapping->invalidate();
            auto size = _readback_image->extent();
            auto layout = _readback_image->memory_layout();
            auto data = ((const unsigned char*)_mapping->data()) + layout.offset;
            return cv::Mat4b{
                int(size.height),
                int(size.width),
                (cv::Vec4b*)data,
                layout.rowPitch
            };
        }

        offscreen_render_target_t::phase_context_t
        offscreen_render_target_t::slot_t::begin(size_t index, VkRect2D rect)
        {
            if (_commands)
                throw std::runtime_error{
                    "double begin in vulkan::offscreen_render_target_t"
                };
            _fence.wait();
            _fence.reset();
            _commands = _command_buffer.begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
            if (_begin_callback)
                _begin_callback(*_commands);
            return {
                index,
                &*_commands,
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
            _mapping.reset();
            if (_end_callback)
                _end_callback(*_commands, _readback_image.get());
            _commands.reset();
            _queue->submit(
                _command_buffer.get(),
                std::move(waits),
                std::move(signals),
                _fence.get()
            );               
        }
    }
}
