//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef ADIO_BIND_HANDLER_HPP_INCLUDED
#define ADIO_BIND_HANDLER_HPP_INCLUDED

#include <adio/config.hpp>

#include <functional>
#include <utility>

ADIO_NAMESPACE_BEGIN

namespace detail {

/**
 * `Handler` that calls the wrapped handler with bound arguments.
 *
 * Provides the same executor and allocator as the wrapped handler
 */
template <class Handler, class... Args>
class bound_handler {
    // Can't friend partial specializations,
    // so we just friend the whole thing.
    template <class T, class Executor>
    friend struct ASIO_NAMESPACE::associated_executor;

    using args_type = std::tuple<std::decay_t<Args>...>;

    Handler   h_;
    args_type args_;

    template <class Arg, class Vals>
    static std::enable_if_t<std::is_placeholder<std::decay_t<Arg>>::value == 0, Arg&&>
    extract(Arg&& arg, Vals&) {
        return std::forward<Arg>(arg);
    }

    template <class Arg, class Vals>
    static std::enable_if_t<
        std::is_placeholder<std::decay_t<Arg>>::value != 0,
        std::tuple_element_t<std::is_placeholder<std::decay_t<Arg>>::value - 1, Vals>>&&
    extract(Arg&&, Vals&& vals) {
        return std::get<std::is_placeholder<std::decay_t<Arg>>::value - 1>(
            std::forward<Vals>(vals));
    }

    template <class ArgsTuple, std::size_t... S>
    static void invoke(Handler& h, ArgsTuple& args, std::tuple<>&&, std::index_sequence<S...>) {
        h(std::get<S>(std::move(args))...);
    }

    template <class ArgsTuple, class ValsTuple, std::size_t... S>
    static void invoke(Handler& h, ArgsTuple& args, ValsTuple&& vals, std::index_sequence<S...>) {
        h(extract(std::get<S>(std::move(args)), std::forward<ValsTuple>(vals))...);
    }

public:
    using result_type = void;

    using allocator_type = ASIO_NAMESPACE::associated_allocator_t<Handler>;

    bound_handler(bound_handler&&)      = default;
    bound_handler(bound_handler const&) = delete;

    template <class DeducedHandler>
    explicit bound_handler(DeducedHandler&& handler, Args&&... args)
        : h_(std::forward<DeducedHandler>(handler))
        , args_(std::forward<Args>(args)...) {}

    allocator_type get_allocator() const noexcept {
        return (ASIO_NAMESPACE::get_associated_allocator)(h_);
    }

    friend bool asio_handler_is_continuation(bound_handler* h) {
        using ASIO_NAMESPACE::asio_handler_is_continuation;
        return asio_handler_is_continuation(std::addressof(h->h_));
    }

    template <class Function>
    friend void asio_handler_invoke(Function&& f, bound_handler* h) {
        using ASIO_NAMESPACE::asio_handler_invoke;
        asio_handler_invoke(f, std::addressof(h->h_));
    }

    template <class... Values>
    void operator()(Values&&... values) {
        invoke(h_,
               args_,
               std::forward_as_tuple(std::forward<Values>(values)...),
               std::index_sequence_for<Args...>());
    }

    template <class... Values>
    void operator()(Values&&... values) const {
        invoke(h_,
               args_,
               std::forward_as_tuple(std::forward<Values>(values)...),
               std::index_sequence_for<Args...>());
    }
};

template <class Handler, class... Args>
detail::bound_handler<std::decay_t<Handler>, Args...> bind_handler(Handler&& handler,
                                                                   Args&&... args) {
    return detail::bound_handler<std::decay_t<Handler>, Args...>(std::forward<Handler>(handler),
                                                                 std::forward<Args>(args)...);
}

}  // namespace detail

ADIO_NAMESPACE_END

ASIO_NAMESPACE_BEGIN

template <class Handler, class... Args, class Executor>
struct associated_executor<adio::detail::bound_handler<Handler, Args...>, Executor> {
    using type = associated_executor_t<Handler, Executor>;

    static type get(adio::detail::bound_handler<Handler, Args...> const& h,
                    Executor const&                                      ex = Executor()) noexcept {
        return associated_executor<Handler, Executor>::get(h.h_, ex);
    }
};

ASIO_NAMESPACE_END

namespace std {
template <class Handler, class... Args>
void bind(adio::detail::bound_handler<Handler, Args...>, ...) = delete;
}  // namespace std

#endif  // ADIO_BIND_HANDLER_HPP_INCLUDED
