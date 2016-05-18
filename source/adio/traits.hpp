#ifndef ADIO_TRAITS_HPP_INCLUDED
#define ADIO_TRAITS_HPP_INCLUDED

#include <type_traits>

#include <adio/config.hpp>

namespace adio
{

namespace detail
{

template <typename T> struct void_t_helper
{
    using type = void;
};

} /* detail */

template <typename T> using void_t = typename detail::void_t_helper<T>::type;

#define ADIO_DECLARE_MEMBER_DETECTOR(name, member)                             \
    template <typename, typename = void> struct name : public std::false_type  \
    {                                                                          \
    };                                                                         \
    template <typename T>                                                      \
    struct name<T, ::adio::void_t<typename T::member>> : public std::true_type \
    {                                                                          \
    }

ADIO_DECLARE_MEMBER_DETECTOR(has_implementation_type, implementation_type);

#define ADIO_MEMBER_TYPE_OR(name, member)                                      \
    namespace detail                                                           \
    {                                                                          \
    template <typename T, typename Fallback, typename = void>                  \
    struct name##_helper                                                       \
    {                                                                          \
        using type = Fallback;                                                 \
    };                                                                         \
    template <typename T, typename Fallback>                                   \
    struct name##_helper<T, Fallback, void_t<typename T::member>>              \
    {                                                                          \
        using type = typename T::member;                                       \
    };                                                                         \
    } /* detail */                                                             \
    template <typename T, typename Fallback>                                   \
    using name = typename detail::name##_helper<T, Fallback>::type

ADIO_MEMBER_TYPE_OR(implementation_type_or, implementation_type);

template <typename Handler>
using handler_decay = typename std::remove_reference<
    typename std::remove_cv<Handler>::type>::type;

template <typename Signature, typename Handler> struct handler_helper
{
    using RealHandlerType =
        typename asio::handler_type<handler_decay<Handler>, Signature>::type;

    RealHandlerType handler;
    asio::async_result<RealHandlerType> result;

    handler_helper(const Handler& h)
        : handler(h)
        , result(handler)
    {
    }
};

namespace detail
{

// This speedy impelementation brought to you by Xeo,
// https://stackoverflow.com/questions/13072359/c11-compile-time-array-with-logarithmic-evaluation-depth/13073076#13073076
template <class T> using Invoke = typename T::type;

template <std::size_t...> struct seq
{
    using type = seq;
};

template <class S1, class S2> struct concat;

template <std::size_t... I1, std::size_t... I2>
struct concat<seq<I1...>, seq<I2...>> : seq<I1..., (sizeof...(I1) + I2)...>
{
};

template <class S1, class S2> using Concat = Invoke<concat<S1, S2>>;

template <std::size_t N> struct gen_seq;
template <std::size_t N> using GenSeq = Invoke<gen_seq<N>>;

template <std::size_t N>
struct gen_seq : Concat<GenSeq<N / 2>, GenSeq<N - N / 2>>
{
};

template <> struct gen_seq<0> : seq<>
{
};
template <> struct gen_seq<1> : seq<0>
{
};

template <typename... Ts> struct tag
{
};

template <class T, std::size_t I> struct type_index_pair
{
    friend T my_declval(type_index_pair, std::integral_constant<std::size_t, I>)
    {
    }
};

template <class, class> struct pop_back_helper;

template <class... TT, std::size_t... Is>
struct pop_back_helper<tag<TT...>, seq<Is...>>
{
    struct base : type_index_pair<TT, Is>...
    {
    };

    template <std::size_t... Is2>
    using join = tag<decltype(
        my_declval(base{}, std::integral_constant<std::size_t, Is2>{}))...>;
};

template <class... TT, std::size_t... Is, std::size_t... Is2>
auto deduce(seq<Is...>, seq<Is2...>) ->
    typename pop_back_helper<tag<TT...>, seq<Is...>>::template join<Is2...>;

template <class... TT>
using pop_back = decltype(
    deduce<TT...>(gen_seq<sizeof...(TT)>{}, gen_seq<sizeof...(TT)-1>{}));


} /* detail */

template <typename, typename, typename = void>
struct handler_matches : std::false_type
{
};

template <typename Handler, typename Ret, typename... Args>
struct handler_matches<Handler,
                       Ret(Args...),
                       void_t<decltype(std::declval<Handler&>()(
                           std::declval<Args&&>()...))>>
    : std::integral_constant<bool,
                             std::is_same<decltype(std::declval<Handler&>()(
                                              std::declval<Args&&>()...)),
                                          Ret>::value>
{
};

template <typename Handler, typename Sig, typename Ret = void>
using require_handler_matches =
    typename std::enable_if<handler_matches<Handler, Sig>::value, Ret>::type;

} /* adio */

#endif  // ADIO_TRAITS_HPP_INCLUDED
