#ifndef ADIO_CONFIG_ASIO_HPP_INCLUDED
#define ADIO_CONFIG_ASIO_HPP_INCLUDED

#define ADIO_BACKEND_NAMESPACE asio

#include <adio/config/common.hpp>

#include <asio/asio.hpp>
#include <system_error>

ADIO_NAMESPACE_BEGIN

using std::error_category;
using std::error_code;
using std::error_condition;
using std::system_error;
using std::errc::make_error_code;
using std::errc::make_error_condition;

using asio::io_service;

ADIO_NAMESPACE_END

#endif  // ADIO_CONFIG_ASIO_HPP_INCLUDED
