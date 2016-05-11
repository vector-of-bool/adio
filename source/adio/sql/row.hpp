#ifndef ADIO_ROW_HPP_INCLUDED
#define ADIO_ROW_HPP_INCLUDED

#include "value.hpp"

#include <cinttypes>

namespace adio
{

template <typename T> struct row_adaptor;

class row
{
    std::vector<value> _data;

public:
    explicit row(std::vector<value> d)
        : _data{std::move(d)}
    {
    }

    std::size_t size() const { return _data.size(); }

    const value& operator[](std::size_t n) const { return _data[n]; }

    template <typename T> T as() const { return row_adaptor<T>::read(*this); }

    template <typename T> explicit operator T() const { return as<T>(); }
};

template <std::size_t N> const value& get(const row& r) { return r[N]; }


namespace detail
{

inline void check_singular(const row& r)
{
    if (r.size() != 1)
        throw std::invalid_argument{
            "Cannot read single value from row of width != 1"};
}

template <typename T>
struct int_row_adaptor
{
    static T read(const row& r)
    {
        detail::check_singular(r);
        return T(r[0].get<value::integer>());
    }
};

} /* detail */

#define DECL_INT_ADAPTOR(t)                                                    \
    template <> struct row_adaptor<t> : detail::int_row_adaptor<t>             \
    {                                                                          \
    }
DECL_INT_ADAPTOR(std::uint8_t);
DECL_INT_ADAPTOR(std::int8_t);
DECL_INT_ADAPTOR(std::uint16_t);
DECL_INT_ADAPTOR(std::int16_t);
DECL_INT_ADAPTOR(std::uint32_t);
DECL_INT_ADAPTOR(std::int32_t);
DECL_INT_ADAPTOR(std::uint64_t);
DECL_INT_ADAPTOR(std::int64_t);
#undef DECL_INT_ADAPTOR

template <typename CharT, typename Traits, typename Allocator>
struct row_adaptor<std::basic_string<CharT, Traits, Allocator>>
{
    using value_type = std::basic_string<CharT, Traits, Allocator>;
    static value_type read(const row& r)
    {
        detail::check_singular(r);
        auto str = r[0].get<value::text>();
        return value_type(begin(str), end(str));
    }
};

} /* adio */

#endif  // ADIO_ROW_HPP_INCLUDED

