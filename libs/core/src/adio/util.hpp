#ifndef ADIO_UTIL_HPP_INCLUDED
#define ADIO_UTIL_HPP_INCLUDED

#include <adio/config.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

ADIO_NAMESPACE_BEGIN

/**
 * Throw a `system_error` if the given error code represents an errant state,
 * optionally using `what` as the exception message.
 */
inline void throw_if_error(const adio::error_code& e, std::string_view what) {
    if (e) {
        throw system_error{e, std::string(what)};
    }
}

/**
 * General-purpose variadic tag type.
 */
template <typename...>
struct tag {};

/**
 * Invoke the given callable and always return a value, even if `void_return`
 */
template <typename Callable>
decltype(auto) regularize_invoke(Callable&&);

namespace detail {

struct void_return {};

// Map void-returning functions to return `void_return`
template <typename Callable>
void_return regularize_invoke(Callable&& c, tag<void>) {
    std::invoke(ADIO_FWD(c));
    return void_return();
}

// Map non-void to its regular return type
template <typename Callable, typename Ret>
decltype(auto) regularize_invoke(Callable&& c, tag<Ret>) {
    return std::invoke(ADIO_FWD(c));
}

}  // namespace detail

// impl
template <typename Callable>
decltype(auto) regularize_invoke(Callable&& c) {
    using ret_type = std::result_of_t<Callable()>;
    return detail::regularize_invoke(ADIO_FWD(c), tag<ret_type>{});
}

/**
 * Given the return value from `regularize_invoke`, map it back to the return
 * type or `void`. This should be used immediately in a `return` statement with
 * return type deduction.
 */
template <typename Value>
decltype(auto) unregularize(Value&& v) {
    return ADIO_FWD(v);
}

// For `void_return`, we return void
inline void unregularize(detail::void_return) {}

template <typename T>
T forward_return(std::remove_reference_t<T>& t) {
    return static_cast<T&&>(t);
}

template <typename T>
T forward_return(std::remove_reference_t<T>&& t) {
    return static_cast<T&&>(t);
}

#define ADIO_FORWARD_RET(expr) ::adio::forward_return<decltype(expr)>(expr)

/**
 * Forward an argument as if using the reference category of a possibly
 * unrelated type
 */
template <typename Other, typename Arg>
decltype(auto) forward_like(Arg&& what) {
    using ArgNoRef = std::remove_reference_t<Arg>;
    using Ref = std::conditional_t<std::is_rvalue_reference<Other&&>::value, ArgNoRef&&, ArgNoRef&>;
    return static_cast<Ref>(what);
}

/**
 * Given a type, remove any std::reference_wrapper to get the underlying type
 */
template <typename T>
struct unwrap_reference {
    using type = T;
};

template <typename T>
struct unwrap_reference<std::reference_wrapper<T>> {
    using type = T;
};

/**
 * Convenience alias
 */
template <typename T>
using unwrap_reference_t = typename unwrap_reference<T>::type;

ADIO_NAMESPACE_END

#endif  // ADIO_UTIL_HPP_INCLUDED