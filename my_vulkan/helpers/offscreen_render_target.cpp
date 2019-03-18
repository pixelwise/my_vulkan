#include "offscreen_render_target.hpp"

namespace my_vulkan
{
    namespace helpers
    {
        offscreen_render_target_t::offscreen_render_target_t(
            device_t& device,
            VkRenderPass render_pass,
            VkFormat color_format,
            VkFormat depth_format,
            VkExtent2D size,
            bool need_readback,
            size_t depth
        )
        : _render_pass{render_pass}
        , _size{size}
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
                slot_t::finish_callback_t callback =
                    [&image = _color_buffers[i].image](
                        command_buffer_t::scope_t& commands,
                        image_t& //readback_image
                    ) {
                        image.transition_layout(
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            commands
                        );                        
                    };
                if (need_readback) 
                    callback = [callback, &image = _color_buffers[i].image](
                        command_buffer_t::scope_t& commands,
                        image_t& readback_image
                    ) {
                        callback(commands, readback_image);
                        readback_image.transition_layout(
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            commands
                        );
                        readback_image.blit_from(
                            image,
                            commands
                        );
                        readback_image.transition_layout(
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_GENERAL,
                            commands
                        );                        
                    };
                _slots.emplace_back(
                    device.get(),
                    _render_pass,
                    device.graphics_queue(),
                    size,
                    _color_buffers[i].view.get(),
                    depth_format,
                    need_readback,
                    device.physical_device(),
                    callback
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
            VkRenderPass render_pass,
            std::vector<VkImageView> color_views,
            VkFormat depth_format,
            VkExtent2D size
        )
        : _render_pass{render_pass}
        , _size{size}
        {
            for (size_t i = 0; i < color_views.size(); ++i)
            {
                _slots.emplace_back(
                    device.get(),
                    _render_pass,
                    device.graphics_queue(),
                    size,
                    color_views[i],
                    depth_format,
                    false,
                    device.physical_device(),
                    slot_t::finish_callback_t{0}
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
                        scope.index
                    };
                },
                [&](auto waits, auto signals){
                    finish_phase(std::move(waits), std::move(signals));
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

        VkRenderPass offscreen_render_target_t::render_pass()
        {
            return _render_pass;
        }

        size_t offscreen_render_target_t::depth() const
        {
            return _slots.size();
        }

        offscreen_render_target_t::phase_context_t offscreen_render_target_t::begin_phase(boost::optional<VkRect2D> rect)
        {
            return _slots[_write_slot].begin(_write_slot, rect.value_or(VkRect2D{{0, 0}, size()}));
        }

        void offscreen_render_target_t::finish_phase(
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

        boost::optional<cv::Mat4b> offscreen_render_target_t::read_bgra(bool flush)
        {
            if (auto read_slot = consume_read_slot(flush))
                return _slots[*read_slot].read_bgra();
            else
                return boost::none;
        }

        boost::optional<size_t> offscreen_render_target_t::consume_read_slot(bool flush)
        {
            size_t num_slots = _slots.size();
            size_t slots_required = flush ? 1 : num_slots;
            if (_num_slots_filled < slots_required)
                return boost::none;
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
            VkRenderPass render_pass,
            queue_reference_t& queue,
            VkExtent2D size,
            VkImageView color_view,
            VkFormat depth_format,
            bool need_readback,
            VkPhysicalDevice physical_device,
            finish_callback_t finish_callback
        )
        : _queue{&queue}
        , _render_pass{render_pass}
        , _depth_image{
            device,
            physical_device,
            {size.width, size.height, 1},
            depth_format,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        }
        , _depth_view{_depth_image.view(VK_IMAGE_ASPECT_DEPTH_BIT)}
        , _framebuffer{
            device,
            render_pass,
            {color_view, _depth_view.get()},
            size                
        }
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
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT                    
            } :
            nullptr
        }
        , _fence{device, VK_FENCE_CREATE_SIGNALED_BIT}
        , _command_pool{device, queue}
        , _command_buffer{_command_pool.make_buffer()}
        , _finish_callback{std::move(finish_callback)}
        {
        }

        cv::Mat4b offscreen_render_target_t::slot_t::read_bgra()
        {
            if (!_readback_image)
                throw std::runtime_error{"no readback enabled"};
            _fence.wait();
            _mapping = _readback_image->memory()->map();
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
            _commands->begin_render_pass(
                _render_pass,
                _framebuffer.get(),
                rect
            );
            return {
                index,
                _framebuffer.get(),
                &*_commands
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
            _commands->end_render_pass();
            _mapping.reset();
            if (_finish_callback)
                _finish_callback(*_commands, *_readback_image);
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