#ifndef ADIO_POSTGRESQL_HPP_INCLUDED
#define ADIO_POSTGRESQL_HPP_INCLUDED

#include <adio/connection_fwd.hpp>
#include <adio/service.hpp>

#include <boost/optional.hpp>

namespace adio
{

class pg;

namespace detail
{

class pg_service;
}

class pg
{
public:
    using service = detail::pg_service;
    using connection = basic_connection<pg>;

    using statement = int;

    pg(service&){}

    void open(boost::optional<std::string> where = boost::none);
    void prepare();
    void execute();
    void step();
    void close();
};

namespace detail
{

class pg_service : public db_service_base<pg_service, pg>
{
public:
    using db_service_base::db_service_base;
};
}
}

#endif  // ADIO_POSTGRESQL_HPP_INCLUDED