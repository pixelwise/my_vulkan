#pragma once
#include <array>
 
 
//! Round up \p value to next multiple of \p roundedto.
constexpr static size_t roundedup(size_t value, size_t roundedto)
{ 
    return roundedto * ((value + roundedto - 1) / roundedto);
}
 
 
template<typename T>
class std140_traits
{
    template<typename C = T>
    constexpr static typename std::enable_if<std::is_arithmetic<C>::value, size_t>::type
    align_helper()
    {
        return sizeof(C);
    }
    template<typename C = T>
    constexpr static typename std::enable_if<!std::is_arithmetic<C>::value, size_t>::type
    align_helper() {
        return C::align;
    }
public:
    constexpr static size_t align = align_helper();
};
 
// declaration
template<typename Arg0, typename ...Args>
struct std140_struct_impl;
//!\internal
template<typename Arg0, typename ...Args>
class std140_struct_impl : public std140_struct_impl<Args...>
{
    typedef std140_struct_impl<Args...> Next;
public:
    template<int S>
    constexpr static size_t offset(size_t offs)
    {
        return S
            ? Next::template offset<S - 1>(roundedup(offs, std140_traits<Arg0>::align) + sizeof(Arg0))
            : roundedup(offs, std140_traits<Arg0>::align);
    }
    template<int S>
    static typename std::conditional<S, decltype(Next::template type<S - 1>()), Arg0>::type type();
};
//!\internal
template<typename Arg0>
struct std140_struct_impl<Arg0>
{
    template<int S>
    constexpr static size_t offset(size_t offs)
    {
        static_assert(S < 1, "There is no std140_struct member with that index.");
        return roundedup(offs, std140_traits<Arg0>::align) + (S ? sizeof(Arg0) : 0);
    }
    template<int> static Arg0 type();
};
 
//! std140 structure with \p Args arguments.
template<typename ...Args>
class std140_struct : private std140_struct_impl<Args...>
{
    typedef std140_struct_impl<Args...> Next;
public:
    //! Offset of \p S element of structure.
    template<int S>
    constexpr static size_t offset() { return Next::template offset<S>(0); }
    //! Size of std140 structure (with padding at the end).
    constexpr static size_t size = roundedup(Next::template offset<-1>(0), 16);
    //! Alignment of std140 structure
    constexpr static size_t align = 16;
 
    //! Get a reference to \p S element of structure.
    template<int S> auto &get()
    {
        return *reinterpret_cast<decltype(Next::template type<S>())*>(
            reinterpret_cast<char*>(this) + offset<S>());
    }
private:
    char reserved[size];
};
 
// scalar types
typedef int32_t std140_bool;    //!< std140 bool
typedef int32_t std140_int;        //!< std140 int
typedef uint32_t std140_uint;    //!< std140 unsigned int
typedef float std140_float;        //!< std140 float
typedef double std140_double;    //!< std140 double
 
//! std140 vector with \p T type elements and \p N size
template<typename T, size_t N>
struct std140_vector : public std::array<T, N>
{
    static_assert(N > 1 && N < 5, "Number of GLSL vector elements can be 2, 3 or 4");
    static_assert(std::is_same<T, std140_float>::value || std::is_same<T, std140_double>::value ||
                std::is_same<T, std140_int>::value || std::is_same<T, std140_uint>::value ||
                std::is_same<T, std140_bool>::value,
                "Elements of GLSL vectors must be bool, int, uint, float or double");
    //! Alignment of std140 vector
    constexpr static size_t align = sizeof(T) * (N == 3 ? 4 : N);
};
 
//! std140 array with \p T type elements and \p N size
template<typename T, size_t N>
struct std140_array
{
    //! Stride of std140 array elements
    constexpr static size_t stride = roundedup(sizeof(T), 16);
    //! Alignment of std140 array
    constexpr static size_t align = roundedup(std140_traits<T>::align, 16);
    //! Get a reference to \p idx element of array.
    T &operator[](size_t idx) { return *reinterpret_cast<T*>(reinterpret_cast<char*>(this) + idx * stride); }
private:
    char reserved[N * stride];
};

//! std140 matrix with \p T type elements, \p C columns, \p R rows and <tt>col_major = true</tt> if matrix is column major
template<typename T, size_t R, size_t C, bool col_major = true>
struct std140_matrix : public std140_array<std140_vector<T, col_major ? R : C>, col_major ? C : R>
{
    static_assert(R > 1 && R < 5 && C > 1 && C < 5, "Dimensions of GLSL matrices must be 2, 3 or 4");
    static_assert(std::is_same<T, std140_float>::value || std::is_same<T, std140_double>::value,
        "Elements of GLSL matrices must be float or double");
};
 
typedef std140_matrix<std140_float, 2, 2> std140_mat2;
typedef std140_matrix<std140_float, 3, 3> std140_mat3;
typedef std140_matrix<std140_float, 4, 4> std140_mat4;
typedef std140_matrix<std140_float, 4, 4> std140_mat4;

typedef std140_vector<std140_float, 2> std140_vec2;
typedef std140_vector<std140_float, 3> std140_vec3;
typedef std140_vector<std140_float, 4> std140_vec4;
 
 /*
int main()
{
    typedef std140_struct<
        std140_int,
        std140_array<
            std140_struct<
                std140_vector<std140_float, 3>,
                std140_float>, 4>,
        std140_int,
        std140_int
    > holy;
    holy f;
    cout << holy::size << "\n";
    cout << holy::offset<0>() << "\n";
    cout << holy::offset<1>() << "\n";
    cout << holy::offset<2>() << "\n";
    cout << holy::offset<3>() << "\n";
 
    f.get<1>()[3].get<0>()[2] = 15.001;
    cout << f.get<1>()[3].get<0>()[2] << "\n";
 
 
    return 0;
}
*/
