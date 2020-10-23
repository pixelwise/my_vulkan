#pragma once

#include <vector>
#include <algorithm>
template <typename T>
void extend_vector(std::vector<T> & v1, const std::vector<T> & v2)
{
    v1.insert(v1.end(), v2.begin(), v2.end());
}

template <typename T>
void extend_vector(std::vector<T> & v1, std::vector<T>&& v2)
{
    v1.insert(v1.end(), std::make_move_iterator(v2.begin()), std::make_move_iterator(v2.end()));
}

/**
 * if size of T is large, this can be very slow
 * @tparam T
 * @param x
 * @return
 */
template <typename T>
std::vector<T> unique_vector(const std::vector<T> & x)
{
    std::vector<T> ret {x};
    std::sort(ret.begin(), ret.end());
    auto last = std::unique(ret.begin(), ret.end());
    ret.erase(last, ret.end());
    return ret;
}

/**
 * if size of T is large, this can be very slow
 * @tparam T
 * @param x
 */
template <typename T>
void unique_vector_inplace(std::vector<T> & x)
{
    std::sort(x.begin(), x.end());
    auto last = std::unique(x.begin(), x.end());
    x.erase(last, x.end());
}
