#ifndef ADIO_VALUE_HPP_INCLUDED
#define ADIO_VALUE_HPP_INCLUDED

#include <chrono>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/operators.hpp>
#include <boost/variant.hpp>

#include "traits.hpp"

namespace adio
{

using boost::variant;

/// Represents the type of some database value
enum class type
{
    null_t,
    integer,
    real,
    text,
    blob,
    datetime,
};

/// Special class used to represent NULL database values. @see adio::null
class null_t final : public boost::operators<null_t> {
    bool operator==(null_t) const { return true; }
    bool operator<(null_t) const { return false; }
};
/// An instance of ``null_t``
extern null_t null;

/// Exception throw when accessing a ``value`` object using the incorrect type.
class invalid_access : public std::runtime_error
{
public:
    invalid_access(std::string s)
        : std::runtime_error{s}
    {
    }
};

using boost::get;
using std::get;

/// Type used to adapt to and from ``value`` (See @ref custom_adaptors "Custom adaptors")
template <typename T> struct value_adaptor;
template <typename T> struct is_basic_type;

template <typename T, typename = void>
struct has_value_adaptor : std::false_type
{
};

template <typename T>
struct has_value_adaptor<T, void_t<typename value_adaptor<T>::base_type>>
    : std::true_type
{
};

class value : boost::operators<value>
{
public:
    using null_t = adio::null_t;
    using integer = int64_t;
    using real = double;
    using text = std::string;
    using blob = std::vector<char>;
    using datetime = std::chrono::high_resolution_clock::time_point;
    using variant_type = variant<null_t, integer, real, text, blob, datetime>;

private:
    variant_type _data;

#define DECL_GETTER(tn)                                                        \
    tn _get(detail::tag<tn>) const                                             \
    {                                                                          \
        return get_type() == type::tn                                      \
            ? boost::get<tn>(_data)                                            \
            : throw invalid_access{"Cannot get a " #tn                         \
                                   " value from a non-" #tn                    \
                                   " value object (Type is "                   \
                                   + std::string{type_name()}                  \
                                   + ")"};                                     \
    }

    DECL_GETTER(null_t);
    DECL_GETTER(integer);
    DECL_GETTER(real);
    DECL_GETTER(text);
    DECL_GETTER(blob);
    DECL_GETTER(datetime);

    template <typename T> T _get_maybe_nullable(std::true_type) const
    {
        using adaptor = value_adaptor<T>;
        if (get_type() == type::null_t) return adaptor::null();
        auto value = get<typename adaptor::base_type>();
        return adaptor::convert(value);
    }

    template <typename T> T _get_maybe_nullable(std::false_type) const
    {
        using adaptor = value_adaptor<T>;
        auto value = get<typename adaptor::base_type>();
        return adaptor::convert(value);
    }

    template <typename T,
              typename Adaptor = value_adaptor<typename std::decay<T>::type>>
    static value _from_maybe_nullable(T&& other, std::true_type)
    {
        return Adaptor::is_null(other)
            ? value{null}
            : value{Adaptor::convert(std::forward<T>(other))};
    }

    template <typename T,
              typename Adaptor = value_adaptor<typename std::decay<T>::type>>
    static value _from_maybe_nullable(T&& other, std::false_type)
    {
        return value{Adaptor::convert(std::forward<T>(other))};
    }

public:
    value(null_t = null)
        : _data{null}
    {
    }
    value(integer i)
        : _data{i}
    {
    }
    value(real r)
        : _data{r}
    {
    }
    template <ADIO_DOCS_LIE(typename Other)(
        typename Other,
        typename Decayed = typename std::decay<Other>::type,
        typename
        = typename std::enable_if<has_value_adaptor<Decayed>::value
                                  && !is_basic_type<Decayed>::value>::type,
        typename Adaptor = value_adaptor<Decayed>)>
    value(Other&& other)
        : value(
              _from_maybe_nullable(std::forward<Other>(other),
                                   std::integral_constant<bool,
                                                          Adaptor::nullable>{}))
    {
    }
    value(text t)
        : _data{std::move(t)}
    {
    }
    value(blob b)
        : _data{std::move(b)}
    {
    }
    value(datetime d)
        : _data{d}
    {
    }
    template <std::size_t N>
    value(const char (&arr)[N])
        : _data{text(arr)}
    {
    }

    template <typename T>
    typename std::enable_if<is_basic_type<T>::value, T>::type get() const
    {
        return _get(detail::tag<T>{});
    }

    template <typename T, typename = void>
    typename std::enable_if<!is_basic_type<T>::value, T>::type get() const
    {
        using adaptor = value_adaptor<T>;
        static_assert(has_value_adaptor<T>::value,
                      "Cannot convert an adio::value object to the given type: "
                      "No adaptor has been provided");
        return _get_maybe_nullable<T>(
            std::integral_constant<bool, adaptor::nullable>{});
    }

    enum type get_type() const
    {
        switch (_data.which())
        {
        case 0:
            return type::null_t;
        case 1:
            return type::integer;
        case 2:
            return type::real;
        case 3:
            return type::text;
        case 4:
            return type::blob;
        case 5:
            return type::datetime;
        default:
            assert(0);
            std::terminate();
        }
    }

    const char* type_name() const
    {
        switch (get_type())
        {
        case type::null_t:
            return "NULL";
        case type::integer:
            return "integral";
        case type::real:
            return "real";
        case type::text:
            return "text";
        case type::blob:
            return "blob";
        case type::datetime:
            return "datetime";
        default:
            assert(0);
            std::terminate();
        }
    }

    template <typename T> explicit operator T() const { return get<T>(); }

    inline bool operator<(const value& other) const;
    inline bool operator==(const value& other) const;
};

template <typename T> struct is_basic_type : std::false_type
{
};
template <> struct is_basic_type<value::null_t> : std::true_type
{
};
template <> struct is_basic_type<value::integer> : std::true_type
{
};
template <> struct is_basic_type<value::real> : std::true_type
{
};
template <> struct is_basic_type<value::text> : std::true_type
{
};
template <> struct is_basic_type<value::blob> : std::true_type
{
};
template <> struct is_basic_type<value::datetime> : std::true_type
{
};

template <typename T> T get(const value& val) { return val.get<T>(); }

inline bool value::operator<(const value& other) const
{
    const auto other_t = other.get_type();
    if (get_type() != other_t) return get_type() < other_t;
    switch (other_t)
    {
    case type::null_t:
        return false;
    case type::integer:
        return get<integer>() < other.get<integer>();
    case type::real:
        return get<real>() < other.get<real>();
    case type::text:
        return get<text>() < other.get<text>();
    case type::blob:
        return get<blob>() < other.get<blob>();
    case type::datetime:
        return get<datetime>() < other.get<datetime>();
    default:
        assert(0);
        std::terminate();
    }
}

inline bool value::operator==(const value& other) const
{
    const auto other_t = other.get_type();
    if (get_type() != other_t) return false;
    switch (other_t)
    {
    case type::null_t:
        return true;
    case type::integer:
        return get<integer>() == other.get<integer>();
    case type::real:
        return get<real>() == other.get<real>();
    case type::text:
        return get<text>() == other.get<text>();
    case type::blob:
        return get<blob>() == other.get<blob>();
    case type::datetime:
        return get<datetime>() == other.get<datetime>();
    default:
        assert(0);
        std::terminate();
    }
}

inline std::ostream& operator<<(std::ostream& o, const value& v)
{
    switch (v.get_type())
    {
    case type::null_t:
        return o << "NULL";
    case type::integer:
        return o << get<value::integer>(v);
    case type::real:
        return o << get<value::real>(v);
    case type::text:
        return o << get<value::text>(v);
    case type::blob:
        return o << "{" << get<value::blob>(v).size() << " bytes of data}";
    case type::datetime:
        return o << get<value::datetime>(v);
    default:
        assert(0);
        std::terminate();
    }
}

namespace detail
{

template <typename Int> struct integer_adaptor
{
    enum
    {
        nullable = false
    };
    using base_type = value::integer;
    static Int convert(base_type v) { return static_cast<Int>(v); }
    static base_type convert(Int v) { return static_cast<base_type>(v); }
};

} /* detail */

#define DECL_INT_ADAPTOR(t)                                                    \
    template <> struct value_adaptor<t> : detail::integer_adaptor<t>           \
    {                                                                          \
    }
DECL_INT_ADAPTOR(std::int8_t);
DECL_INT_ADAPTOR(std::uint8_t);
DECL_INT_ADAPTOR(std::int16_t);
DECL_INT_ADAPTOR(std::uint16_t);
DECL_INT_ADAPTOR(std::int32_t);
DECL_INT_ADAPTOR(std::uint32_t);
DECL_INT_ADAPTOR(std::uint64_t);
DECL_INT_ADAPTOR(bool);
#undef DECL_INT_ADAPTOR

template <typename CharT, typename Traits, typename Allocator>
struct value_adaptor<std::basic_string<CharT, Traits, Allocator>>
{
    enum
    {
        nullable = false
    };
    using base_type = std::string;
    using value_type = std::basic_string<CharT, Traits, Allocator>;
    static value_type convert(const base_type& str)
    {
        return value_type{std::begin(str), std::end(str)};
    }
    static base_type convert(const value_type& str)
    {
        return base_type{begin(str), end(str)};
    }
};

} /* adio */

#endif  // ADIO_VALUE_HPP_INCLUDED
