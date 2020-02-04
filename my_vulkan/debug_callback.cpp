#include "debug_callback.hpp"
#include "utils.hpp"
#include <cassert>
namespace my_vulkan
{
    debug_callback_t::debug_callback_t(
        VkInstance instance,
        PFN_vkDebugReportCallbackEXT callback
    )
    : _instance{instance}
    {
        VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {};
        debugReportCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debugReportCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT;
        debugReportCreateInfo.pfnCallback = 
            (PFN_vkDebugReportCallbackEXT)callback;

        // We have to explicitly load this function.
        PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
            reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
                vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT")
            );
        assert(vkCreateDebugReportCallbackEXT);
        VkDebugReportCallbackEXT debugReportCallback;
        vk_require(
            vkCreateDebugReportCallbackEXT(
                instance,
                &debugReportCreateInfo,
                nullptr,
                &debugReportCallback
            ),
            "installing debug callback"
        );
        _callback = debugReportCallback;
    }
    
    debug_callback_t::~debug_callback_t()
    {
        PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
            reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
                vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT")
            );
        assert(vkDestroyDebugReportCallbackEXT);
        vkDestroyDebugReportCallbackEXT(
            _instance,
            _callback,
            0
        );
    }
}
