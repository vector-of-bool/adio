#ifndef ADIO_CONFIG_HPP_INCLUDED
#define ADIO_CONFIG_HPP_INCLUDED

// clang-format off
// Adio is built with either using boost or vanilla by default, use can
// override:
#ifdef ADIO_USE_BOOST_ASIO
    // Use requested Boost.Asio
#   include <boost/asio.hpp>
#   define ADIO_DETAIL_BOOST_ASIO
#elif defined(ADIO_USE_VANILLA_ASIO)
    // Use requests vanilla Asio
#   include <asio.hpp>
#   include <asio/spawn.hpp>
#   define ADIO_DETAIL_VANILLA_ASIO
#else
    // Use has not overriden the default:
#   ifdef ADIO_DETAIL_USE_BOOST_ASIO_DEFAULT
        // Built against boost::asio:
#       include <boost/asio.hpp>
#       define ASIO_DETAIL_BOOST_ASIO
#   elif defined(ADIO_DETAIL_USE_VANILLA_ASIO_DEFAULT)
        // Built against vanilla Asio:
#       include <asio.hpp>
#       include <asio/spawn.hpp>
#       define ASIO_DETAIL_VANILLA_ASIO
#   else
        // Unable to determine which asio to use!
#       error "Define either ADIO_USE_BOOST_ASIO or ADIO_USE_VANILLA_ASIO"
#   endif
#endif
// clang-format on

#ifdef ASIO_DETAIL_VANILLA_ASIO
// Vanilla asio uses the stdlib:
#include <system_error>
#endif

namespace adio
{

#ifdef ASIO_DETAIL_VANILLA_ASIO

namespace asio = asio;

using error_code = std::error_code;
using error_condition = std::error_condition;
using sys_errc = std::errc;
using system_error = std::system_error;
using asio_error_category = std::error_category;
using std::make_error_code;
using std::make_error_condition;

#else  // Use Boost.Asio:

namespace asio = boost::asio;
using error_code = boost::system::error_code;
using error_condition = boost::system::error_condition;
using sys_errc = boost::system::errc::errc_t;
using system_error = boost::system::system_error;
using asio_error_category = boost::system::error_category;
using boost::system::errc::make_error_code;
using boost::system::errc::make_error_condition;

#endif

using io_service = asio::io_service;

using std::string;

} /* adio */

#ifdef ADIO_DETAIL_VANILLA_ASIO
#define ADIO_DECLARE_ERRC_ENUM(type)                                           \
    namespace std                                                              \
    {                                                                          \
    template <> struct is_error_code_enum<type> : public std::true_type        \
    {                                                                          \
    };                                                                         \
    } /* stc */                                                                \
    static_assert(true, "")
#else
#define ADIO_DECLARE_ERRC_ENUM(type)                                           \
    namespace boost                                                            \
    {                                                                          \
    namespace system                                                           \
    {                                                                          \
                                                                               \
    template <> struct is_error_code_enum<type>                                \
    {                                                                          \
        enum                                                                   \
        {                                                                      \
            value = true                                                       \
        };                                                                     \
    };                                                                         \
    } /* system */                                                             \
    } /* boost */ static_assert(true, "")
#endif // ADIO_DETAIL_VANILLA_ASIO

#endif  // ADIO_CONFIG_HPP_INCLUDED
