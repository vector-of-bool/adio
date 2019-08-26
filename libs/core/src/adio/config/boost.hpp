#ifndef ADIO_CONFIG_BOOST_HPP_INCLUDED
#define ADIO_CONFIG_BOOST_HPP_INCLUDED

#define ADIO_BACKEND_NAMESPACE boost

#include <adio/config/common.hpp>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#define ASIO_NAMESPACE_BEGIN                                                                       \
    namespace boost {                                                                              \
    namespace asio {

#define ASIO_NAMESPACE_END                                                                         \
    }                                                                                              \
    }

#define ASIO_NAMESPACE boost::asio

ADIO_NAMESPACE_BEGIN

using boost::system::error_category;
using boost::system::error_code;
using boost::system::error_condition;
using boost::system::system_error;
using boost::system::errc::make_error_code;
using boost::system::errc::make_error_condition;

using errc = boost::system::errc::errc_t;

using boost::asio::asio_handler_invoke;
using boost::asio::associated_allocator_t;
using boost::asio::associated_executor_t;
using boost::asio::async_completion;
using boost::asio::dispatch;
using boost::asio::execution_context;
using boost::asio::executor;
using boost::asio::executor_work_guard;
using boost::asio::get_associated_allocator;
using boost::asio::get_associated_executor;
using boost::asio::io_context;
using boost::asio::make_work_guard;
using boost::asio::post;
using boost::asio::thread_pool;
using boost::asio::use_service;

namespace posix = boost::asio::posix;

#define ADIO_DECLARE_ERRC_ENUM(type)                                                               \
    namespace boost {                                                                              \
    namespace system {                                                                             \
                                                                                                   \
    template <>                                                                                    \
    struct is_error_code_enum<type> {                                                              \
        enum { value = true };                                                                     \
    };                                                                                             \
    } /* system */                                                                                 \
    } /* boost */                                                                                  \
    static_assert(true, "")

ADIO_NAMESPACE_END

#endif  // ADIO_CONFIG_BOOST_HPP_INCLUDED
