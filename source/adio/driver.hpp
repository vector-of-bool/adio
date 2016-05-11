#ifndef ADIO_DATABASE_DRIVER_HPP_INCLUDED
#define ADIO_DATABASE_DRIVER_HPP_INCLUDED

#include "config.hpp"
#include "traits.hpp"
#include "utils.hpp"

#include <memory>

namespace adio
{

namespace detail
{

template <typename Derived,
          typename Implementation = typename Derived::connection_type>
class base_driver_service : public asio::io_service::service
{
public:
    static asio::io_service::id id;

    using statement = typename Implementation::statement;

    using self_type = base_driver_service;
    using super_type = asio::io_service::service;

    using implementation_type = std::shared_ptr<Implementation>;

    base_driver_service(asio::io_service& ios)
        : super_type(ios)
    {
    }

    void construct(implementation_type& impl)
    {
        impl.reset(new Implementation(static_cast<Derived&>(*this)));
    }
    void destroy(implementation_type& impl) { impl.reset(); }

#define ADIO_SERVICE_DECL_FN(name, ...)                                        \
public:                                                                        \
    template <typename... Args>                                                \
    auto name(implementation_type& impl, Args&&... args)                       \
        ->decltype(impl->name(std::forward<Args>(args)...))                    \
    {                                                                          \
        return impl->name(std::forward<Args>(args)...);                        \
    }                                                                          \
                                                                               \
private:                                                                       \
    template <typename... Args> struct invoker_for_##name                      \
    {                                                                          \
        template <typename Handler>                                            \
        void                                                                   \
        _invoke(implementation_type& impl, Args&&... args, Handler&& handler)  \
        {                                                                      \
            static_assert(                                                     \
                adio::handler_matches<Handler,                                 \
                                      typename Implementation::                \
                                          name##_handler_signature>::value,    \
                "Invalid async handler passed to async_" #name                 \
                " for this database driver");                                  \
            impl->async_##name(std::forward<Args>(args)..., handler);          \
        }                                                                      \
        template <typename Handler>                                            \
        auto operator()(implementation_type& impl,                             \
                        Args&&... args,                                        \
                        Handler&& handler)                                     \
            -> decltype(std::declval<handler_helper<                           \
                            typename Implementation::name##_handler_signature, \
                            handler_decay<Handler>>&>()                        \
                            .result.get())                                     \
        {                                                                      \
            handler_helper<typename Implementation::name##_handler_signature,  \
                           handler_decay<Handler>>                             \
                init{std::forward<Handler>(handler)};                          \
            _invoke(impl, std::forward<Args>(args)..., init.handler);          \
            return init.result.get();                                          \
        }                                                                      \
    };                                                                         \
    template <typename... TagArgs, typename... Args>                           \
    auto _async_##name(detail::tag<TagArgs...>,                                \
                       implementation_type& impl,                              \
                       Args&&... args)                                         \
        ->decltype(                                                            \
            invoker_for_##name<TagArgs...>{}(impl,                             \
                                             std::forward<Args>(args)...))     \
    {                                                                          \
        return invoker_for_##name<TagArgs...>{}(impl,                          \
                                                std::forward<Args>(args)...);  \
    }                                                                          \
                                                                               \
public:                                                                        \
    template <typename... Args>                                                \
    auto async_##name(implementation_type& impl, Args&&... args)               \
        ->decltype(this->_async_##name(detail::pop_back<Args...>{},            \
                                       impl,                                   \
                                       std::forward<Args>(args)...))           \
    {                                                                          \
        return _async_##name(detail::pop_back<Args...>{},                      \
                             impl,                                             \
                             std::forward<Args>(args)...);                     \
    }                                                                          \
    static_assert(true, "")

    ADIO_SERVICE_DECL_FN(open);
    ADIO_SERVICE_DECL_FN(prepare);
    ADIO_SERVICE_DECL_FN(execute);
    ADIO_SERVICE_DECL_FN(step);
    ADIO_SERVICE_DECL_FN(close);

private:
    void shutdown_service() override{};
};

template <typename Derived, typename Implementation>
io_service::id base_driver_service<Derived, Implementation>::id;

} /* detail */

} /* adio */

#endif  // ADIO_DATABASE_DRIVER_HPP_INCLUDED
