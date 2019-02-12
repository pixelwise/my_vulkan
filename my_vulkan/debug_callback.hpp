#pragma once

#include <vulkan/vulkan.h>

namespace my_vulkan
{
    class debug_callback_t
    {
    public:
        debug_callback_t(
            VkInstance instance,
            PFN_vkDebugReportCallbackEXT callback
        );
        ~debug_callback_t();
        debug_callback_t(const debug_callback_t&) = delete;
    private:
        VkInstance _instance;
        VkDebugReportCallbackEXT _callback;
    };  
};
