#pragma once

#include <vulkan/vulkan.h>

template<typename ostream_t>
ostream_t& operator<<(ostream_t& os, VkOffset2D offset)
{
    os << "(" << offset.x << "," << offset.y << ")";
    return os;
}

template<typename ostream_t>
ostream_t& operator<<(ostream_t& os, VkExtent2D extent)
{
    os << extent.width << "x" << extent.height;
    return os;
}

template<typename ostream_t>
ostream_t& operator<<(ostream_t& os, VkRect2D rect)
{
    os << "@" << rect.offset << " " << rect.extent;
    return os;
}
