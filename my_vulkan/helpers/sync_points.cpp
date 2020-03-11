#include "sync_points.hpp"
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
}
