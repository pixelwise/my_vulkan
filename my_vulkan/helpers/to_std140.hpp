#pragma once
#include "std140.hpp"

#include <glm/glm.hpp>

#include <boost/fusion/include/for_each.hpp>
#include <boost/range/counting_range.hpp>

#include <vector>

struct std140_data
{
    std::vector<char> data;
    size_t align;
};

inline std140_data to_std140(bool v)
{
    std140_bool sv = v;
    auto* p = reinterpret_cast<char*>(&sv);
    return {std::vector<char>(p, p + sizeof(sv)), sizeof(sv)};
}

inline std140_data to_std140(int v)
{
    std140_int sv = v;
    auto* p = reinterpret_cast<char*>(&sv);
    return {std::vector<char>(p, p + sizeof(sv)), sizeof(sv)};
}

inline std140_data to_std140(float v)
{
    std140_float sv = v;
    auto* p = reinterpret_cast<char*>(&sv);
    return {std::vector<char>(p, p + sizeof(sv)), sizeof(sv)};
}

namespace glm
{
    inline std140_data to_std140(vec2 v)
    {
        std140_vector<float, 2> sv;
        for (auto i : boost::counting_range(0, v.length()))
            sv[i] = v[i];
        auto* p = reinterpret_cast<char*>(&sv);
        return {std::vector<char>(p, p + sizeof(sv)), sv.align};
    }

    inline std140_data to_std140(vec3 v)
    {
        std140_vector<float, 3> sv;
        for (auto i : boost::counting_range(0, v.length()))
            sv[i] = v[i];
        auto* p = reinterpret_cast<char*>(&sv);
        return {std::vector<char>(p, p + sizeof(sv)), sv.align};
    }

    inline std140_data to_std140(vec4 v)
    {
        std140_vector<float, 4> sv;
        for (auto i : boost::counting_range(0, v.length()))
            sv[i] = v[i];
        auto* p = reinterpret_cast<char*>(&sv);
        return {std::vector<char>(p, p + sizeof(sv)), sv.align};
    }

    inline std140_data to_std140(mat3 m)
    {
        std140_matrix<float, 3, 3> sm;
        for (auto i : boost::counting_range(0, m.length()))
            for (auto j : boost::counting_range(0, m[0].length()))
                sm[i][j] = m[i][j];
        auto* p = reinterpret_cast<char*>(&sm);
        return {std::vector<char>(p, p + sizeof(sm)), sm.align};
    }

    inline std140_data to_std140(mat4 m)
    {
        std140_matrix<float, 4, 4> sm;
        for (auto i : boost::counting_range(0, m.length()))
            for (auto j : boost::counting_range(0, m[0].length()))
                sm[i][j] = m[i][j];
        auto* p = reinterpret_cast<char*>(&sm);
        return {std::vector<char>(p, p + sizeof(sm)), sm.align};
    }
}

namespace std
{
    template<typename value_t, size_t n> std140_data to_std140(const std::array<value_t, n> v)
    {
        std140_array<value_t, n> sv;
        for (auto i : boost::counting_range<size_t>(0, v.size()))
            sv[i] = v[i];
        auto* p = reinterpret_cast<char*>(&sv);
        return {std::vector<char>(p, p + sizeof(sv)), sv.align};
    }
}

template<typename struct_t>
inline std140_data to_std140(struct_t the_struct)
{
    std::vector<char> result;
    boost::fusion::for_each(
        the_struct,
        [&](auto v){
            auto data = to_std140(v);
            size_t aligned_size = roundedup(result.size(), data.align);
            if (aligned_size > result.size())
                result.resize(aligned_size);
            result.insert(result.end(), data.data.begin(), data.data.end());
        }
    );
    return {result, 16};
}
