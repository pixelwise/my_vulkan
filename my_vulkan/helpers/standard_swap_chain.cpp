#include "standard_swap_chain.hpp"
#include <iostream>

namespace my_vulkan
{
    namespace helpers
    {
        static image_t create_depth_image(
            device_t& logical_device,
            VkFormat format,
            VkExtent2D extent
        )
        {
            my_vulkan::image_t result{
                logical_device,
                {extent.width, extent.height, 1},
                format,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            };
            return result;
        }

        standard_swap_chain_t::standard_swap_chain_t(
            device_t& device,
            VkSurfaceKHR surface,
            queue_family_indices_t queue_indices,
            VkExtent2D desired_extent
        )
        : _device{&device}
        , _surface{surface}
        , _queue_indices{queue_indices}
        , _swap_chain{new swap_chain_t{
            device,
            surface,
            queue_indices,
            desired_extent
        }}
        , _graphics_queue{&device.graphics_queue()}
        , _present_queue{&device.present_queue()}
        , _depth_image{create_depth_image(
            device,
            find_depth_format(device.physical_device()),
            _swap_chain->extent()
        )}
        , _depth_view{
            _depth_image.view(VK_IMAGE_ASPECT_DEPTH_BIT)
        }
        , _render_pass{
            device.get(),
            color_format(),
            _depth_image.format()
        }
        , _command_pool{device.get(), device.graphics_queue()}
        {
            for (auto&& image : _swap_chain->images())
            {
                auto image_view = image.view(VK_IMAGE_ASPECT_COLOR_BIT);
                auto image_view_handle = image_view.get();
                _pipeline_resources.push_back(
                    pipeline_resources_t{
                        std::move(image_view),
                        framebuffer_t{
                            device.get(),
                            _render_pass.get(),
                            std::vector<VkImageView>{
                                image_view_handle,
                                _depth_view.get()
                            },
                            extent()                        
                        },
                        _command_pool.make_buffer()
                    }
                );
                _frame_sync_points.push_back(
                    frame_sync_points_t{
                        semaphore_t{device.get()},
                        semaphore_t{device.get()},
                        fence_t{
                            device.get(),
                            VK_FENCE_CREATE_SIGNALED_BIT
                        }
                    }
                );
            }
        }

        void standard_swap_chain_t::update(VkExtent2D new_extent)
        {
            wait_for_idle();
            _swap_chain.reset(new swap_chain_t{
                *_device,
                _surface,
                _queue_indices,
                new_extent
            });
            _depth_image = create_depth_image(
                *_device,
                find_depth_format(_device->physical_device()),
                _swap_chain->extent()
            );
            _depth_view = _depth_image.view(VK_IMAGE_ASPECT_DEPTH_BIT);
            auto& images = _swap_chain->images();
            for (size_t i = 0; i < images.size(); ++i)
            {
                _pipeline_resources[i].command_buffer.reset();
                _pipeline_resources[i].image_view = images[i].view(
                    VK_IMAGE_ASPECT_COLOR_BIT
                );
                _pipeline_resources[i].framebuffer = framebuffer_t{
                    _device->get(),
                    _render_pass.get(),
                    std::vector<VkImageView>{
                        _pipeline_resources[i].image_view.get(),
                        _depth_view.get()
                    },
                    new_extent
                };
            }
            std::cout << "updated swap chain with size "
                << new_extent.width << "x" << new_extent.height
                << std::endl;
        }

        standard_swap_chain_t::acquisition_outcome_t standard_swap_chain_t::acquire(
            VkRect2D rect,
            std::vector<VkClearValue> clear_values
        )
        {
            acquisition_outcome_t outcome;
            auto& sync_points = _frame_sync_points[_current_frame];
            _current_frame = (_current_frame + 1) % _frame_sync_points.size();
            sync_points.in_flight.wait();
            auto parent_outcome = _swap_chain-> acquire_next_image(sync_points.image_available.get());
            outcome.failure = parent_outcome.failure;
            if (auto i = parent_outcome.image_index)
            {
                sync_points.in_flight.reset();
                outcome.working_set = working_set_t{
                    *this,
                    *i,
                    sync_points
                };
                outcome.working_set->commands().begin_render_pass(
                    render_pass(),
                    _pipeline_resources[*i].framebuffer.get(),
                    rect,
                    clear_values
                );
            }
            return outcome;
        }

        standard_swap_chain_t::working_set_t::working_set_t(
            standard_swap_chain_t& parent,
            uint32_t phase,
            frame_sync_points_t& sync
        )
        : _parent{&parent}
        , _sync{&sync}
        , _phase{phase}
        , _commands{parent._pipeline_resources[phase].command_buffer.begin(
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
        )}
        {
        }

        uint32_t standard_swap_chain_t::working_set_t::phase() const
        {
            return _phase;
        }

        command_buffer_t::scope_t& standard_swap_chain_t::working_set_t::commands()
        {
            return _commands;
        }

        boost::optional<acquisition_failure_t> standard_swap_chain_t::working_set_t::finish(
            std::vector<queue_reference_t::wait_semaphore_info_t> wait_semaphores,
            std::vector<VkSemaphore> signal_semaphores
        )
        {
            commands().end_render_pass();
            commands().end();
            auto& resources = _parent->_pipeline_resources[phase()];
            wait_semaphores.push_back({
                _sync->image_available.get(),
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            });
            signal_semaphores.push_back(_sync->render_finished.get());
            _parent->_graphics_queue->submit(
                resources.command_buffer.get(),
                std::move(wait_semaphores),
                std::move(signal_semaphores),
                _sync->in_flight.get()
            );
            if (auto presentation_failure = _parent->_present_queue->present(
                {_parent->_swap_chain->get(), phase()},
                {_sync->render_finished.get()}
            ))
                return presentation_failure;
            return boost::none;
        }

        size_t standard_swap_chain_t::depth() const
        {
            return _pipeline_resources.size();
        }

        VkRenderPass standard_swap_chain_t::render_pass()
        {
            return _render_pass.get();
        }

        command_pool_t& standard_swap_chain_t::command_pool()
        {
            return _command_pool;
        }

        std::vector<VkFramebuffer> standard_swap_chain_t::framebuffers()
        {
            std::vector<VkFramebuffer> result(depth());
            for (size_t i = 0; i < depth(); ++i)
                result[i] = _pipeline_resources[i].framebuffer.get();
            return result;
        }

        void standard_swap_chain_t::wait_for_idle()
        {
            for (auto& sync : _frame_sync_points)
                sync.in_flight.wait();
        }

        render_target_t standard_swap_chain_t::render_target()
        {
            std::shared_ptr<boost::optional<working_set_t>> working_set{
                new boost::optional<working_set_t>{}
            };
            return {
                [working_set, this](VkRect2D rect){
                    auto outcome = acquire(
                        rect,
                        {
                            {.color = {{0.0f, 0.0f, 0.0f, 0.0f}}},
                            {.depthStencil = {1.0f, 0}},
                        }
                    );
                    if (outcome.failure)
                        throw std::runtime_error{
                            std::string{"swap chaing acquisition failed: "} +
                            to_string(*outcome.failure)
                        };
                    *working_set = std::move(*outcome.working_set);
                    return render_scope_t{
                        &(*working_set)->commands(),
                        (*working_set)->phase()
                    };
                },
                [working_set](auto waits, auto signals){
                    (*working_set)->finish(std::move(waits), std::move(signals));
                },
                {
                    extent().width,
                    extent().height
                },
                depth(),
                false
            };
        }
        
        VkFormat standard_swap_chain_t::color_format() const
        {
            return _swap_chain->format();
        }

        VkFormat standard_swap_chain_t::depth_format() const
        {
            return _depth_image.format();
        }

        VkExtent2D standard_swap_chain_t::extent() const
        {
            return _swap_chain->extent();
        }
    }
}
