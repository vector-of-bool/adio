#ifndef ADIO_SERVICE_HPP_INCLUDED
#define ADIO_SERVICE_HPP_INCLUDED

#include "config.hpp"
#include "util.hpp"

#include <utility>

ADIO_NAMESPACE_BEGIN

template <typename Driver>
class basic_database_service : public execution_context::service {
private:
    void shutdown() noexcept override {}

    Driver _driver;

public:
    using driver_type = Driver;
    using key_type    = basic_database_service;

    using implementation_type = typename Driver::implementation_type;

    explicit basic_database_service(execution_context& ctx)
        : service(ctx) {}

    driver_type& get_driver() noexcept { return _driver;  }
    const driver_type& get_driver() const noexcept { return _driver; }
};

ADIO_NAMESPACE_END

#endif  // ADIO_SERVICE_HPP_INCLUDED
