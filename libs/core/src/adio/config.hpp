#ifndef ADIO_CONFIG_HPP_INCLUDED
#define ADIO_CONFIG_HPP_INCLUDED

#define ADIO_BACKEND_Boost 1
#define ADIO_BACKEND_Asio 2
#define ADIO_BACKEND_NetworkTS 3

#define ADIO_PASTE_1(a, b) a##b
#define ADIO_PASTE(a, b) ADIO_PASTE_1(a, b)

#define ADIO_STR(a) #a

#define ADIO_BACKEND_EXPAND(what) ADIO_PASTE(ADIO_BACKEND_, what)

#define ADIO_ABI_VERSION v1

#define ADIO_FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

#if ADIO_BACKEND_EXPAND(ADIO_BACKEND) == ADIO_BACKEND_Boost
#include <adio/config/boost.hpp>
#elif ADIO_BACKEND_EXPAND(ADIO_BACKEND) == ADIO_BACKEND_Asio
#include <adio/config/asio.hpp>
#elif ADIO_BACKEND_EXPAND(ADIO_BACKEND) == ADIO_BACKEND_NetworkTS
#include <adio/config/network_ts.hpp>
#else
#error "Define ADIO_BACKEND to `Boost', `Asio', or `NetworkTS'"
#endif

#endif  // ADIO_CONFIG_HPP_INCLUDED
