#ifndef ADIO_CONFIG_HPP_INCLUDED
#define ADIO_CONFIG_HPP_INCLUDED

#include <adio/docs.hpp>

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

using error_code = ADIO_DOC_UNSPEC(std::error_code);
using error_condition = ADIO_DOC_UNSPEC(std::error_condition);
using sys_errc = ADIO_DOC_UNSPEC(std::errc);
using system_error = ADIO_DOC_UNSPEC(std::system_error);
using asio_error_category = ADIO_DOC_UNSPEC(std::error_category);
using std::make_error_code;
using std::make_error_condition;

#else  // Use Boost.Asio:

namespace asio = ADIO_DOC_UNSPEC(boost::asio);
/// Either ``std::error_code`` or ``boost::system::error_code``
using error_code = ADIO_DOC_UNSPEC(boost::system::error_code);
/// Either ``std::error_condition`` or ``boost::system::error_condition``
using error_condition = ADIO_DOC_UNSPEC(boost::system::error_condition);
/// Either ``std::errc`` or ``boost::system::errc::errc_t``
using sys_errc = ADIO_DOC_UNSPEC(boost::system::errc::errc_t);
/// Either ``std::system_error`` or ``boost::system::system_error``
using system_error = ADIO_DOC_UNSPEC(boost::system::system_error);
/// Either ``std::error_category`` or ``boost::system::error_category``
using asio_error_category = ADIO_DOC_UNSPEC(boost::system::error_category);
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
