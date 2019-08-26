#ifndef ADIO_CONNETION_HPP_INCLUDED
#define ADIO_CONNETION_HPP_INCLUDED

#include <adio/config.hpp>
#include <adio/service.hpp>

#include <functional>
#include <utility>

ADIO_NAMESPACE_BEGIN

template <typename Driver, typename Executor = adio::executor>
class basic_connection {
public:
    using service_type        = basic_database_service<Driver>;
    using driver_type         = Driver;
    using implementation_type = typename Driver::implementation_type;
    using executor_type       = Executor;

private:
    std::reference_wrapper<service_type> _srv_ref;
    implementation_type                  _impl;
    executor_type                        _executor;

    service_type&       _srv() noexcept { return _srv_ref; }
    const service_type& _srv() const noexcept { return _srv_ref; }

    driver_type&       _driver() noexcept { return _srv().get_driver(); }
    const driver_type& _driver() const noexcept { return _srv().get_driver(); }

public:
    template <typename Ctx, typename ExecArg = typename Ctx::executor_type, typename... Args>
    explicit basic_connection(Ctx& ctx, Args&&... args)
        : _srv_ref(use_service<service_type>(static_cast<execution_context&>(ctx)))
        , _impl(_driver().create_implementation(ctx, std::forward<Args>(args)...))
        , _executor(ctx.get_executor()) {}

    /**
     * Obtain an executor associated with this connection
     */
    executor_type get_executor() const noexcept { return _executor; }

    /**
     * Connect the underlying database connection
     */
    template <typename Arg>
    decltype(auto) connect(Arg&& arg, error_code& ec) {
        return _driver().connect(_impl, ADIO_FWD(arg), ec);
    }
    template <typename Arg>
    decltype(auto) connect(Arg&& arg) {
        error_code     ec;
        decltype(auto) ret = adio::regularize_invoke([&] { return connect(ADIO_FWD(arg), ec); });
        throw_if_error(ec, "connect()");
        return ADIO_FWD_RET(ret);
    }
    template <typename Arg, typename Token>
    decltype(auto) async_connect(Arg&& arg, Token&& h) {
        async_completion<Token, typename Driver::connect_signature> comp(h);
        auto ex    = get_associated_executor(h, _executor);
        auto alloc = get_associated_allocator(h);
        _driver().async_connect(_impl,
                                ADIO_FWD(arg),
                                alloc,
                                ex,
                                std::move(comp.completion_handler));
        return comp.result.get();
    }

    decltype(auto) disconnect(error_code& ec) { return _driver().disconnect(_impl, ec); }
    decltype(auto) disconnect() {
        error_code     ec;
        decltype(auto) ret = adio::regularize_invoke([&] { return disconnect(ec); });
        throw_if_error(ec, "disconnect()");
        return ADIO_FWD_RET(ret);
    }
    template <typename Token>
    decltype(auto) async_disconnect(Token&& h) {
        async_completion<Token, typename Driver::disconnect_signature> comp(h);
        auto alloc = get_associated_allocator(h);
        auto ex    = get_associated_executor(h, _executor);
        _driver().async_disconnect(_impl, alloc, ex, std::move(comp.completion_handler));
        return comp.result.get();
    }

    template <typename Q>
    decltype(auto) prepare(Q&& query, error_code& ec) {
        return _driver().prepare(_impl, ADIO_FWD(query), ec);
    }
    template <typename Q>
    decltype(auto) prepare(Q&& query) {
        error_code     ec;
        decltype(auto) ret = adio::regularize_invoke([&] { return prepare(ADIO_FWD(query), ec); });
        throw_if_error(ec, "prepare()");
        return ADIO_FWD_RET(ret);
    }
    template <typename Q, typename Token>
    decltype(auto) async_prepare(Q&& query, Token&& tok) {
        using Signature = typename Driver::template prepare_signature_t<Q>;
        async_completion<Token, Signature> comp(tok);
        auto                               alloc = get_associated_allocator(tok);
        auto                               ex    = get_associated_executor(tok, _executor);
        _driver().async_prepare(_impl,
                                ADIO_FWD(query),
                                alloc,
                                ex,
                                std::move(comp.completion_handler));
        return comp.result.get();
    }

    template <typename Q>
    decltype(auto) execute(Q&& query, error_code& ec) {
        return _driver().execute(_impl, ADIO_FWD(query), ec);
    }
    template <typename Q>
    decltype(auto) execute(Q&& query) {
        error_code     ec;
        decltype(auto) ret = adio::regularize_invoke([&] { return execute(ADIO_FWD(query), ec); });
        throw_if_error(ec, "execute()");
        return ADIO_FWD_RET(ret);
    }
    template <typename Q, typename Token>
    decltype(auto) async_execute(Q&& query, Token&& tok) {
        using Signature = typename Driver::template execute_signature_t<Q>;
        async_completion<Token, Signature> comp(tok);
        _driver().async_execute(_impl,
                                ADIO_FWD(query),
                                _executor,
                                std::move(comp.completion_handler));
        return comp.result.get();
    }

    decltype(auto) last_error() const noexcept { return _driver().last_error(_impl); }
};

ADIO_NAMESPACE_END

#endif  // ADIO_CONNETION_HPP_INCLUDED
