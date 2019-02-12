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
            while (_slots.size() < depth)
            {
                _slots.emplace_back(
                    device.get(),
                    _render_pass,
                    device.graphics_queue(),
                    size,
                    color_format,
                    depth_format,
                    need_readback,
                    device.physical_device()
                );
            }
        }

        render_target_t offscreen_render_target_t::render_target()
        {
            return {
                [&]{
                    auto scope = begin_phase();
                    return render_scope_t{
                        scope.commands,
                        scope.index
                    };
                },
                [&](auto waits, auto signals){
                    finish_phase(std::move(waits), std::move(signals));
                },
                _size,
                depth()
            };
        }

        VkDescriptorImageInfo offscreen_render_target_t::texture(size_t phase)
        {
            return _slots.at(phase).texture();
        }

        VkRenderPass offscreen_render_target_t::render_pass()
        {
            return _render_pass;
        }

        size_t offscreen_render_target_t::depth() const
        {
            return _slots.size();
        }

        offscreen_render_target_t::phase_context_t offscreen_render_target_t::begin_phase()
        {
            return _slots[_write_slot].begin(_write_slot);
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
            size_t num_slots = _slots.size();
            size_t slots_required = flush ? 1 : num_slots;
            if (_num_slots_filled < slots_required)
                return boost::none;
            size_t read_slot = (_write_slot + num_slots - _num_slots_filled) % num_slots;
            --_num_slots_filled;
            return _slots[read_slot].read_bgra();
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
            VkFormat color_format,
            VkFormat depth_format,
            bool need_readback,
            VkPhysicalDevice physical_device
        )
        : _queue{&queue}
        , _render_pass{render_pass}
        , _size{size}
        , _color_image{
            device,
            physical_device,
            {size.width, size.height, 1},
            color_format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT
        }
        , _color_view{_color_image.view()}
        , _sampler{device}
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
            {_color_view.get(), _depth_view.get()},
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
        {
        }

        VkDescriptorImageInfo offscreen_render_target_t::slot_t::texture()
        {
            return {
                _sampler.get(),
                _color_view.get(),
                _color_image.layout()
            };
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
        offscreen_render_target_t::slot_t::begin(size_t index)
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
                {{0, 0}, _size}
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
            if (_readback_image)
            {
                _mapping.reset();
                _readback_image->transition_layout(
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    *_commands
                );
                _readback_image->blit_from(
                    _color_image,
                    *_commands
                );
                _readback_image->transition_layout(
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_GENERAL,
                    *_commands
                );
            }
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