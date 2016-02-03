#include "error.hpp"


using namespace adio;

namespace adio
{

namespace detail
{

class error_category : public adio::asio_error_category
{
    const char* name() const noexcept override;
    std::string message(int) const override;
};

} /* detail */

} /* adio */


const char* detail::error_category::name() const noexcept
{
    return "adio::base";
}

std::string detail::error_category::message(int) const
{
    return "Unkown error";
}

const asio_error_category& adio::error_category()
{
    static detail::error_category cat;
    return cat;
}
