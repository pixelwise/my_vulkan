#include "sync_points.hpp"
#include <sstream>
namespace my_vulkan
{

    void sync_point_refs_t::extend(sync_point_refs_t o)
    {
        auto other = std::move(o);
        if (!other.waits.empty())
            extend_waits(std::move(other.waits));
        if (!other.signals.empty())
            extend_signals(std::move(other.signals));
    }

    void sync_point_refs_t::extend_waits(sync_point_refs_t::waits_t o)
    {
        extend_vector(waits, std::move(o));
    }

    void sync_point_refs_t::extend_signals(sync_point_refs_t::signals_t o)
    {
        extend_vector(signals, std::move(o));
    }

    std::string sync_point_refs_t::to_string()
    {
        std::ostringstream ss;
        ss << "waits=[";
        for (auto &wait: waits)
        {
            ss << wait.semaphore << ", ";
        }
        ss << "]";
        ss << "signals=[";
        for (auto &signal: signals)
        {
            ss << signal << ", ";
        }
        ss << "]";
        return ss.str();
    }
}
