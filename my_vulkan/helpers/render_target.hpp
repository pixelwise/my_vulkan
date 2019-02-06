#pragma once

#include "../command_buffer.hpp"
#include <functional>

namespace my_vulkan
{
    namespace helpers
    {
        struct render_scope_t
        {
            command_buffer_t::scope_t* commands;
            size_t phase;
        };
        struct render_target_t
        {
            std::function<render_scope_t()> begin;
            std::function<void()> end;
        };
    }
}
