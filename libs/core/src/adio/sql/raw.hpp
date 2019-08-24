#ifndef ADIO_SQL_RAW_HPP_INCLUDED
#define ADIO_SQL_RAW_HPP_INCLUDED

#include <adio/config.hpp>

#include <string>
#include <string_view>

ADIO_NAMESPACE_BEGIN

/**
 * Wraps a string type as a string containing SQL code.
 */
template <typename String>
class sql_string {
public:
    using string_type = String;
    using value_type  = typename string_type::value_type;

private:
    // The inner string
    string_type _str;

public:
    // Explicit conversion from the string_type:
    explicit sql_string(const string_type& str)
        : _str(str) {}
    explicit sql_string(string_type&& str)
        : _str(std::move(str)) {}

    /**
     * Get the string contained within
     */
    const string_type& string() const& noexcept { return _str; }
    string_type        string() const&& { return std::move(_str); }
};

// Deduce for std::basic_string types
template <typename String>
sql_string(String)->sql_string<String>;

// Character pointers should be their basic_string type
template <typename Char>
sql_string(const Char*)->sql_string<std::basic_string<Char>>;

// template <typename Char, typename Traits, typename Alloc>
// Char* data(std::basic_string<Char, Traits, Alloc>& str) {
//     return const_cast<Char*>(str.data());
// }

// template <typename Char, typename Traits, typename Alloc>
// const Char* data(const std::basic_string<Char, Traits, Alloc>& str) {
//     return str.data();
// }

// template <typename Char>
// Char* data(Char* str) {
//     return str;
// }

namespace literals {

// Create a sql string from a string literal
inline sql_string<std::string_view> operator""_sql(const char* sql, std::size_t) {
    return sql_string(std::string_view(sql));
}

}  // namespace literals

ADIO_NAMESPACE_END

#endif  // ADIO_SQL_RAW_HPP_INCLUDED