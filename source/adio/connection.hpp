#ifndef ADIO_DATABASE_CONNECTION_HPP_INCLUDED
#define ADIO_DATABASE_CONNECTION_HPP_INCLUDED

#include "config.hpp"

#include "connection_fwd.hpp"

namespace adio
{

/** Represents a database connection, through which a user make execute queries.
 *
 * @param Driver The backend of the database connection, such as SQLite,
 * PostgreSQL, etc.
 *
 * This class is an Asio IO object, so it should be used in conjuction with
 * ``asio::io_service``, similar to using an Asio socket object.
 *
 * This class exposes four primary operations: ``open``, ``close``, ``prepare``,
 * and ``execute``. There are four synchronous methods and four asynchronous
 * methods corresponding to these operations. The return type and argument types
 * for each is not specified within the ``basic_connection`` class template, but
 * is defined by the backend ``Driver`` type. To see the precise semantics of
 * these methods, refer to the driver classes themselves. For example, see ``adio::sqlite``.
 */
template <typename Driver>
class basic_connection : public asio::basic_io_object<typename Driver::service>
{
public:
    /// Our own type
    using self_type = basic_connection;
    /// The type of statement used by the backend
    using statement = ADIO_DOC_IMPLDEF(typename Driver::statement);
    using service = ADIO_DOC_IMPLDEF(typename Driver::service);
    /// @internal
    using super_type = asio::basic_io_object<service>;

    /** Constructs a new connection.
     *
     * @param ios An Asio ``io_service`` instance.
     */
    explicit basic_connection(asio::io_service& ios)
        : super_type{ios}
    {
    }

#define ADIO_CON_DECL_FN(name)                                                 \
    template <typename... Args>                                                \
    ADIO_DOC_IMPLDEF(auto)                                                     \
    name(Args&&... args) ADIO_DOC_UNSPEC(                                      \
            ->decltype(this->get_service().name(this->get_implementation(),    \
                                                std::forward<Args>(args)...))) \
    {                                                                          \
        return this->get_service().name(this->get_implementation(),            \
                                        std::forward<Args>(args)...);          \
    }                                                                          \
    template <typename... Args>                                                \
    ADIO_DOC_IMPLDEF(auto)                                                     \
    async_##name(Args&&... args)                                               \
        ADIO_DOC_UNSPEC(->decltype(                                            \
                            this->get_service()                                \
                                .async_##name(this->get_implementation(),      \
                                              std::forward<Args>(args)...)))   \
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
