#include "offscreen_render_target.hpp"

namespace my_vulkan
{
    namespace helpers
    {
        offscreen_render_target_t::offscreen_render_target_t(
            device_t& device,
            VkExtent2D size,
            VkFormat color_format,
            VkFormat depth_format,
            bool need_readback
        )
        : _render_pass{
            device.get(),
            color_format,
            depth_format,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL                
        }
        {
            while (_slots.size() < 2)
            {
                _slots.emplace_back(
                    device,
                    _render_pass.get(),
                    size,
                    color_format,
                    depth_format,
                    need_readback
                );
            }
        }

        VkRenderPass offscreen_render_target_t::render_pass()
        {
            return _render_pass.get();
        }

        size_t offscreen_render_target_t::depth() const
        {
            return _slots.size();
        }

        offscreen_render_target_t::phase_context_t offscreen_render_target_t::begin_phase()
        {
            return _slots[_write_slot].begin(_write_slot);
        }

        void offscreen_render_target_t::finish_phase()
        {
            _slots[_write_slot].finish();
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

        offscreen_render_target_t::slot_t::slot_t(
            device_t& device,
            VkRenderPass render_pass,
            VkExtent2D size,
            VkFormat color_format,
            VkFormat depth_format,
            bool need_readback
        )
        : _device{&device}
        , _color_image{
            device,
            {size.width, size.height, 1},
            color_format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        }
        , _color_view{_color_image.view()}
        , _depth_image{
            device,
            {size.width, size.height, 1},
            depth_format,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        }
        , _depth_view{_depth_image.view(VK_IMAGE_ASPECT_DEPTH_BIT)}
        , _framebuffer{
            device.get(),
            render_pass,
            {_color_view.get(), _depth_view.get()},
            size                
        }
        , _readback_image{
            need_readback ?
            new image_t{
                device,
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
        , _fence{device.get(), VK_FENCE_CREATE_SIGNALED_BIT}
        , _command_pool{device.get(), device.graphics_queue()}
        , _command_buffer{_command_pool.make_buffer()}
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
        offscreen_render_target_t::slot_t::begin(size_t index)
        {
            if (_commands)
                throw std::runtime_error{
                    "double begin in vulkan::offscreen_render_target_t"
                };
            _fence.wait();
            _fence.reset();
            _commands = _command_buffer.begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
            return {
                index,
                _framebuffer.get(),
                &*_commands
            };
        }

        void offscreen_render_target_t::slot_t::finish()
        {
            if (!_commands)
                throw std::runtime_error{
                    "finish before begin in vulkan::offscreen_render_target_t"
                };
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
            _device->graphics_queue().submit(
                _command_buffer.get(),
                _fence.get()
            );               
        }
    }
}