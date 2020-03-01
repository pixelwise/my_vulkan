#pragma once

#include <vector>
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
