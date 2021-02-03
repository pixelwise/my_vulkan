#include "standard_swap_chain.hpp"
#include <iostream>

namespace my_vulkan
{
    namespace helpers
    {
        standard_swap_chain_t::standard_swap_chain_t(
            device_t& device,
            VkSurfaceKHR surface,
            VkExtent2D desired_extent
        )
        : _device{&device}
        , _surface{surface}
        , _swap_chain{new swap_chain_t{
            device,
            surface,
            desired_extent
        }}
        , _graphics_queue{&device.graphics_queue()}
        , _present_queue{&device.present_queue()}
        , _command_pool{device.get(), device.graphics_queue()}
        {
            for (auto&& image : _swap_chain->images())
            {
                _pipeline_resources.push_back(
                    pipeline_resources_t{
                        image.view(VK_IMAGE_ASPECT_COLOR_BIT),
                        _command_pool.make_buffer()
                    }
                );
                _frame_sync_points.push_back(
                    frame_sync_points_t{
                        semaphore_t{device},
                        semaphore_t{device},
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
            // otherwise get weird crashes at least on ios
            _device->wait_idle();
            //wait_for_idle();
            _swap_chain.reset(new swap_chain_t{
                *_device,
                _surface,
                new_extent
            });
            auto& images = _swap_chain->images();
            for (size_t i = 0; i < images.size(); ++i)
            {
                _pipeline_resources[i].command_buffer.reset();
                _pipeline_resources[i].image_view = images[i].view(
                    VK_IMAGE_ASPECT_COLOR_BIT
                );
            }
            std::cout << "updated swap chain with size "
                << new_extent.width << "x" << new_extent.height
                << std::endl;
        }

        standard_swap_chain_t::acquisition_outcome_t standard_swap_chain_t::acquire()
        {
            acquisition_outcome_t outcome;
            auto& sync_points = _frame_sync_points[_current_frame];
            _current_frame = (_current_frame + 1) % _frame_sync_points.size();
            auto parent_outcome = _swap_chain->acquire_next_image(sync_points.image_available.get());
            outcome.failure = parent_outcome.failure;
            if (auto i = parent_outcome.image_index)
            {
                sync_points.in_flight.reset();
                outcome.working_set = working_set_t{
                    *this,
                    *i,
                    sync_points
                };
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

        std::optional<acquisition_failure_t> standard_swap_chain_t::working_set_t::finish(
            std::vector<queue_reference_t::wait_semaphore_info_t> wait_semaphores,
            std::vector<VkSemaphore> signal_semaphores
        )
        {
            auto command_buffer = commands().end();
            wait_semaphores.push_back({
                _sync->image_available.get(),
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            });
            signal_semaphores.push_back(_sync->render_finished.get());
            _parent->_graphics_queue->submit(
                command_buffer,
                std::move(wait_semaphores),
                std::move(signal_semaphores),
                _sync->in_flight.get()
            );
            if (auto presentation_failure = _parent->_present_queue->present(
                {_parent->_swap_chain->get(), phase()},
                {_sync->render_finished.get()}
            ))
                return presentation_failure;
            return std::nullopt;
        }

        size_t standard_swap_chain_t::depth() const
        {
            return _pipeline_resources.size();
        }

        command_pool_t& standard_swap_chain_t::command_pool()
        {
            return _command_pool;
        }

        render_target_t standard_swap_chain_t::render_target(VkExternalMemoryHandleTypeFlagBits external_mem_type)
        {
            std::shared_ptr<std::optional<working_set_t>> working_set{
                new std::optional<working_set_t>{}
            };
            return {
                [external_mem_type, working_set, this](VkRect2D rect){
                    auto outcome = acquire();
                    if (outcome.failure)
                    {
                        throw std::runtime_error{
                            std::string{"swap chaing acquisition failed: "} +
                            to_string(*outcome.failure)
                        };
                    }
                    *working_set = std::move(*outcome.working_set);
                    uint32_t phase = (*working_set)->phase();
                    return render_scope_t{
                        &(*working_set)->commands(),
                        phase,
                        _pipeline_resources[phase].image_view.get(),
                        extent(),
                        rect,
                        _swap_chain->images()[phase].memory() ?
                            _swap_chain->images()[phase].memory()->external_info(external_mem_type) :
                            std::nullopt
                    };
                },
                [working_set](auto waits, auto signals){
                    if (auto failure = (*working_set)->finish(std::move(waits), std::move(signals)))
                        std::cout << "presentation failure: " << to_string(*failure) << std::endl;
                },
                {
                    extent().width,
                    extent().height
                },
                depth()
            };
        }
        
        VkFormat standard_swap_chain_t::color_format() const
        {
            return _swap_chain->format();
        }

        VkExtent2D standard_swap_chain_t::extent() const
        {
            return _swap_chain->extent();
        }

        const std::vector<standard_swap_chain_t::pipeline_resources_t> &standard_swap_chain_t::pipeline_resources() const
        {
            return _pipeline_resources;
        }
    }
}
