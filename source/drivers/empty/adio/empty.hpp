#ifndef ADIO_EMPTY_DRIVER_HPP_INCLUDED
#define ADIO_EMPTY_DRIVER_HPP_INCLUDED

#include <adio/driver.hpp>
#include <adio/connection_fwd.hpp>

#include <iostream>

namespace adio
{

class empty_service;

class empty_driver
{
    std::reference_wrapper<asio::io_service> _parent_ios;
    template <typename Handler> void _post(Handler&& h)
    {
        _parent_ios.get().post(std::forward<Handler>(h));
    }

public:
    using self_type = empty_driver;

    using statement = std::nullptr_t;
    using service = empty_service;
    using connection = basic_connection<empty_driver>;

    empty_driver(empty_service& ios);

    using execute_handler_signature = void();
    using open_handler_signature = void();
    using step_handler_signatuve = void();

    void open(const std::string& path)
    {
        std::cerr << "Opened fake database: " << path << '\n';
    }

    template <typename Handler>
    void async_open(const std::string& path, Handler&& h)
    {
        std::cerr << "Async opened fake database: " << path << '\n';
        _post(std::forward<Handler>(h));
    }

    void execute(const std::string& str)
    {
        std::cerr << "Executing query: " << str << '\n';
    }

    template <typename Handler>
    void async_execute(const std::string& str, Handler&& h)
    {
        std::cerr << "Executing async query: " << str << '\n';
        _post(std::forward<Handler>(h));
    }

    int step(std::string) { return 12; }

    int prepare(const std::string& str) const { return 42; }
    void close() {}
};

class empty_service : public detail::base_driver_service<empty_service, empty_driver>
{
public:
    using base_driver_service<empty_service, empty_driver>::base_driver_service;
};

empty_driver::empty_driver(empty_service& service)
    : _parent_ios{service.get_io_service()}
{
}

} /* adio */

#endif  // ADIO_EMPTY_DRIVER_HPP_INCLUDED
