#pragma once

#include <string>
#include <vector>
#include <boost/format.hpp>
#include <vulkan/vulkan.h>

#include "queue.hpp"
#include "utils.hpp"
#include "instance.hpp"
#include <map>

namespace my_vulkan
{
    struct device_t
    {
        device_t(
            VkPhysicalDevice physical_device,
            queue_family_indices_t queue_indices,
            std::vector<const char*> validation_layers,
            std::vector<const char*> device_extensions        
        );
        device_t(
            VkPhysicalDevice physical_device,
            const instance_t & instance,
            queue_family_indices_t queue_indices,
            std::vector<const char*> validation_layers,
            std::vector<const char*> device_extensions
        );
        device_t(const device_t&) = delete;
        ~device_t();
        void wait_idle();
        VkPhysicalDevice physical_device() const;
        queue_reference_t& graphics_queue();
        queue_reference_t& present_queue();
        queue_reference_t& transfer_queue();
        queue_family_indices_t queue_indices();
        VkDevice get() const;
        std::optional<VkPhysicalDeviceIDProperties> physcial_device_id_properties() const;
        std::optional<vk_uuid_t> physical_device_uuid() const;
        static PFN_vkVoidFunction get_proc_voidp(VkDevice device, const std::string & proc_name);
        template <typename T>
        static T get_proc(VkDevice device, const std::string & proc_name)
        {
            return (T)get_proc_voidp(device, proc_name);
        }
        template <typename T>
        T get_proc(const std::string & proc_name) const
        {
            return (T)_loaded_procs.at(proc_name);
        }

        PFN_vkVoidFunction record_proc(const std::string & proc_name);

        template <typename T>
        T get_proc_record_if_needed(const std::string & proc_name)
        {
            auto search = _loaded_procs.find(proc_name);
            if (search == _loaded_procs.end())
            {
                return (T)record_proc(proc_name);
            }
            else
            {
                return (T)(search->second);
            }
        }

        std::optional<vk_uuid_t> physical_device_uuid(const instance_t& instance);
    private:
        VkPhysicalDevice _physical_device;
        PFN_vkGetPhysicalDeviceProperties2 _fpGetPhysicalDeviceProperties2 {nullptr};
        std::optional<VkPhysicalDeviceIDProperties> _maybe_vkPhysicalDeviceIDProperties{std::nullopt};
        VkDevice _device;
        queue_family_indices_t _queue_indices;
        std::vector<queue_reference_t> _queues;
        queue_reference_t* _graphics_queue{0};
        queue_reference_t* _present_queue{0};
        queue_reference_t* _transfer_queue{0};
        void fetch_physical_device_ID();
        static PFN_vkGetPhysicalDeviceProperties2 fetch_fpGetPhysicalDeviceProperties2(const instance_t& instance)
        {
            return instance.get_proc<PFN_vkGetPhysicalDeviceProperties2>("vkGetPhysicalDeviceProperties2");
        }
        std::map<std::string, PFN_vkVoidFunction> _loaded_procs;
    };
}
