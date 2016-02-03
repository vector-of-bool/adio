#ifndef ADIO_UTILS_HPP_INCLUDED
#define ADIO_UTILS_HPP_INCLUDED

#include "config.hpp"

#include <type_traits>
#include <iterator>

namespace adio
{

namespace detail
{

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>{new T(std::forward<Args>(args)...)};
}

inline std::shared_ptr<io_service::work> make_work(io_service& ios)
{
    return std::make_shared<io_service::work>(ios);
}

inline void throw_if_error(const error_code& e, const string& what)
{
    if (e) throw system_error{e, what};
}

using std::begin;
using std::end;

} /* detail */

using std::begin;
using std::end;

} /* adio */

#endif  // ADIO_UTILS_HPP_INCLUDED
