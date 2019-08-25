#ifndef ADIO_SQL_PROGRAM_HPP_INCLUDED
#define ADIO_SQL_PROGRAM_HPP_INCLUDED

#include <adio/config.hpp>
#include <adio/util.hpp>

#include <tuple>

ADIO_NAMESPACE_BEGIN

namespace sql {

namespace detail {

template <typename, typename = void>
struct pick_result_type {
    using type = void;
};

template <typename Handler>
struct pick_result_type<Handler, std::void_t<typename Handler::result_type>> {
    using type = typename Handler::result_type;
};

template <typename, typename = void>
struct pick_row_type {
    using type = void;
};

template <typename Handler>
struct pick_row_type<Handler, std::void_t<typename Handler::row_type>> {
    using type = typename Handler::row_type;
};

}  // namespace detail

template <typename T>
struct row_converter;

template <typename OutRow, typename InRow, typename = void>
struct has_row_converter : std::false_type {};

template <typename Out, typename In>
struct has_row_converter<Out,
                         In,
                         std::void_t<decltype(row_converter<Out>::convert(std::declval<In>()))>>
    : std::true_type {};

template <typename Handler>
struct row_handler_traits {
    using result_type = typename detail::pick_result_type<Handler>::type;
    using row_type    = typename detail::pick_row_type<Handler>::type;

    template <typename Row>
    static row_type convert_row(Row&& row) {
        static_assert(
            has_row_converter<row_type, Row>::value,
            "There is no row converter that works with the given row input and output types");
        return row_converter<row_type>::convert(ADIO_FWD(row));
    }

    template <typename Arg>
    static result_type result(Arg&& a) {
        if constexpr (std::is_same<result_type, void>::value) {
            return;
        } else {
            return ADIO_FWD(a).result();
        }
    }
};

template <typename Handler, typename Row>
void dispatch_row(Handler&& h, Row&& r) {
    using Traits = row_handler_traits<std::decay_t<Handler>>;
    if constexpr (std::is_same<typename Traits::row_type, void>()
                  || std::is_convertible<Row, typename Traits::row_type>::value) {
        h(ADIO_FWD(r));
    } else {
        auto new_row = Traits::convert_row(ADIO_FWD(r));
        h(std::move(new_row));
    }
}

template <typename Handler>
decltype(auto) result(Handler&& h) {
    return row_handler_traits<std::decay_t<Handler>>::result(ADIO_FWD(h));
}

template <typename Query, typename ResultHandler>
class program {
public:
    using query_type          = std::remove_reference_t<adio::unwrap_reference_t<Query>>;
    using result_handler_type = std::remove_reference_t<adio::unwrap_reference_t<ResultHandler>>;

private:
    Query         _query;
    ResultHandler _handler;

public:
    query_type&          query() noexcept { return _query; }
    result_handler_type& result_handler() noexcept { return _handler; }

    template <typename Q, typename H>
    program(Q&& q, H&& h)
        : _query(ADIO_FWD(q))
        , _handler(ADIO_FWD(h)) {}
};

// template <typename Query, typename ResultHandler>
// program(Query &q, ResultHandler &)->program<Query&, ResultHandler&>;

template <typename Query, typename ResultHandler>
program(Query&& q, ResultHandler &&)->program<std::decay_t<Query>, std::decay_t<ResultHandler>>;

struct ignore_results_t {
    template <typename What>
    void operator()(What&&) {}
};

inline ignore_results_t ignore_results;

template <typename Iter>
class into_iterator {
    Iter _iter;

public:
    into_iterator(Iter i)
        : _iter(i) {}

    template <typename What>
    void operator()(What&& w) {
        *_iter++ = ADIO_FWD(w);
    }
};

template <typename Container>
class into_container {
    Container& _container;

public:
    into_container(Container& c)
        : _container(c) {}

    template <typename What>
    void operator()(What&& w) {
        using std::end;
        auto end_iter = end(_container);
        _container.insert(end_iter, ADIO_FWD(w));
    }

    using row_type = std::decay_t<decltype(*std::declval<Container&>().begin())>;
};

namespace detail {

template <typename Container>
class create_container_impl {
    Container c;

public:
    using row_type = typename into_container<Container>::row_type;

    template <typename What>
    void operator()(What&& w) {
        auto into = adio::sql::into_container(c);
        into(ADIO_FWD(w));
    }

    using result_type = Container;
    result_type result() & { return c; }
    result_type result() && { return std::move(c); }
};

}  // namespace detail

template <typename Container>
inline detail::create_container_impl<Container> create_container;

template <typename... Types>
struct row_converter<std::tuple<Types...>> {
private:
    using tuple_type = std::tuple<Types...>;

    template <typename Row, std::size_t... Is>
    static tuple_type _convert(Row&& r, std::index_sequence<Is...>) {
        return tuple_type(r[Is].template as<Types>()...);
    }

public:
    template <typename Row>
    static tuple_type convert(Row&& row) {
        return _convert(ADIO_FWD(row), std::index_sequence_for<Types...>());
    }
};

}  // namespace sql

ADIO_NAMESPACE_END

#endif  // ADIO_SQL_PROGRAM_HPP_INCLUDED