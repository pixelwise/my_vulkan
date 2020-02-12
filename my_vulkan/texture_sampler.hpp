#pragma once

#include <vulkan/vulkan.h>

namespace my_vulkan
{
    class texture_sampler_t
    {
    public:
        enum class filter_mode_t{linear, nearest, cubic};
        explicit texture_sampler_t(
            VkDevice device,
            filter_mode_t filter_mode = filter_mode_t::linear
        );
        texture_sampler_t(const texture_sampler_t&) = delete;
        texture_sampler_t(texture_sampler_t&& other) noexcept;
        texture_sampler_t& operator=(texture_sampler_t&& other) noexcept;
        texture_sampler_t& operator=(const texture_sampler_t&) = delete;
        ~texture_sampler_t();
        VkSampler get();
    private:
        void cleanup();
        VkDevice _device;
        VkSampler _sampler;
    };
}
