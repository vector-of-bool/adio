#ifndef ADIO_ERROR_HPP_INCLUDED
#define ADIO_ERROR_HPP_INCLUDED

#include "config.hpp"

namespace adio
{

enum class errc
{
};

extern const asio_error_category& error_category();

} /* adio */

#endif  // ADIO_ERROR_HPP_INCLUDED
