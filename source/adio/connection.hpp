#ifndef ADIO_DATABASE_CONNECTION_HPP_INCLUDED
#define ADIO_DATABASE_CONNECTION_HPP_INCLUDED

#include "config.hpp"

#include "connection_fwd.hpp"

namespace adio
{

template <typename Driver>
class basic_connection : public asio::basic_io_object<typename Driver::service>
{
public:
    using self_type = basic_connection;
    using statement = typename Driver::statement;
    using service = typename Driver::service;
    using super_type = asio::basic_io_object<service>;

    explicit basic_connection(asio::io_service& ios)
        : super_type{ios}
    {
    }

#define ADIO_CON_DECL_FN(name)                                                 \
    template <typename... Args>                                                \
    auto name(Args&&... args)                                                  \
        ->decltype(this->get_service().name(this->get_implementation(),        \
                                            std::forward<Args>(args)...))      \
    {                                                                          \
        return this->get_service().name(this->get_implementation(),            \
                                        std::forward<Args>(args)...);          \
    }                                                                          \
    template <typename... Args>                                                \
    auto async_##name(Args&&... args)                                          \
        ->decltype(                                                            \
            this->get_service().async_##name(this->get_implementation(),       \
                                             std::forward<Args>(args)...))     \
    {                                                                          \
        return this->get_service().async_##name(this->get_implementation(),    \
                                                std::forward<Args>(args)...);  \
    }                                                                          \
    static_assert(true, "")

    ADIO_CON_DECL_FN(open);
    ADIO_CON_DECL_FN(prepare);
    ADIO_CON_DECL_FN(execute);
    ADIO_CON_DECL_FN(close);
#undef ADIO_CON_DECL_FN
};

} /* adio */

#endif  // ADIO_DATABASE_CONNECTION_HPP_INCLUDED
