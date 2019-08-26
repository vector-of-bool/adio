#ifndef ADIO_DRIVERS_SQLITE3_SQLITE3_HPP_INCLUDED
#define ADIO_DRIVERS_SQLITE3_SQLITE3_HPP_INCLUDED

#include <adio/bind_handler.hpp>
#include <adio/config.hpp>
#include <adio/connection.hpp>
#include <adio/out.hpp>
#include <adio/sql/program.hpp>
#include <adio/sql/raw.hpp>

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

ADIO_NAMESPACE_BEGIN

class sqlite3;

template <typename T>
struct sqlite3_binding_traits;

namespace detail {

class sqlite3_value {
    using var_type
        = std::variant<std::int64_t, double, std::string, std::vector<char>, decltype(nullptr)>;
    var_type _var;

public:
    template <typename What,
              typename = std::enable_if_t<std::is_constructible<var_type, What>::value>>
    explicit sqlite3_value(What&& w)
        : _var(ADIO_FWD(w)) {}

    explicit sqlite3_value(std::int64_t i)
        : _var(i) {}

    sqlite3_value(const sqlite3_value&) = default;
    sqlite3_value(sqlite3_value&&)      = default;
    sqlite3_value& operator=(const sqlite3_value&) = default;
    sqlite3_value& operator=(sqlite3_value&&) = default;

    template <typename T>
    decltype(auto) as() {
        using std::get;
        return std::get<T>(_var);
    }
};

class sqlite3_row {
    std::vector<sqlite3_value> _values;

public:
    sqlite3_row(std::vector<sqlite3_value> vals)
        : _values(std::move(vals)) {}

    sqlite3_value&       operator[](std::size_t i) noexcept { return _values[i]; }
    const sqlite3_value& operator[](std::size_t i) const noexcept { return _values[i]; }

    std::size_t size() const noexcept { return _values.size(); }
};

class sqlite3_statement {
    void* _stmt_void = nullptr;

    template <typename... Args>
    void _ignore(Args&&...) {}

    void _bind_i64(std::size_t idx, std::int64_t);
    void _bind_double(std::size_t idx, double);
    void _bind_string(std::size_t idx, std::string_view);
    void _bind_null(std::size_t idx);

    template <typename Arg>
    int _bind_one(std::size_t idx, const Arg& arg) {
        if constexpr (std::is_arithmetic<Arg>::value) {
            if constexpr (std::is_integral<Arg>::value) {
                _bind_i64(idx, static_cast<std::int64_t>(arg));
            } else {
                _bind_double(idx, static_cast<double>(arg));
            }
        } else if constexpr (std::is_convertible<Arg, std::string_view>::value) {
            _bind_string(idx, static_cast<std::string_view>(arg));
        } else if constexpr (std::is_same<Arg, decltype(nullptr)>::value) {
            _bind_null(idx);
        } else {
            _bind_one(idx, sqlite3_binding_traits<Arg>::convert(arg));
        }
        return 0;
    }

    template <typename... Args, std::size_t... Is>
    void _bind(std::tuple<Args...> tup, std::index_sequence<Is...>) {
        _ignore(_bind_one(Is, std::get<Is>(tup))...);
    }

public:
    sqlite3_statement() = default;
    explicit sqlite3_statement(void*&& st)
        : _stmt_void(st) {
        st = nullptr;
    }
    ~sqlite3_statement();
    sqlite3_statement(sqlite3_statement&& o) { _stmt_void = std::exchange(o._stmt_void, nullptr); }
    sqlite3_statement& operator=(sqlite3_statement&& o) {
        std::swap(_stmt_void, o._stmt_void);
        return *this;
    }

    void*       get_void_ptr() noexcept { return _stmt_void; }
    const void* get_void_ptr() const noexcept { return _stmt_void; }

    template <typename... Args>
    sqlite3_statement& bind(Args&&... args) & {
        _bind(std::forward_as_tuple(ADIO_FWD(args)...),
              std::make_index_sequence<sizeof...(Args)>());
        return *this;
    }

    template <typename... Args>
    sqlite3_statement bind(Args&&... args) && {
        _bind(std::forward_as_tuple(ADIO_FWD(args)...),
              std::make_index_sequence<sizeof...(Args)>());
        return std::move(*this);
    }
};

class sqlite3_conn {
    void*  _database_void = nullptr;
    void*& _database_void_ref() { return _database_void; }

    bool        _step(sqlite3_statement& st, error_code& ec);
    sqlite3_row _get_row(sqlite3_statement& st);

    template <typename Handler>
    decltype(auto) _execute_into(sqlite3_statement& st, Handler&& res_handler, error_code& ec) {
        while (1) {
            bool is_done = _step(st, ec);
            if (is_done) {
                break;
            }
            auto value = _get_row(st);
            adio::sql::dispatch_row(res_handler, std::move(value));
        }
        return adio::sql::result(ADIO_FWD(res_handler));
    }

public:
    sqlite3_conn() = default;
    ~sqlite3_conn();
    sqlite3_conn(sqlite3_conn&& o)
        : _database_void{std::exchange(o._database_void, nullptr)} {}
    sqlite3_conn& operator=(sqlite3_conn&& o) {
        std::swap(_database_void, o._database_void);
        return *this;
    }
    void              connect(const std::string& str, error_code& ec);
    void              disconnect(error_code& ec);
    std::string       last_error() const noexcept;
    sqlite3_statement prepare(const std::string_view& q, error_code& ec);

    template <typename String>
    decltype(auto) _ensure_prepared(const adio::sql_string<String>& sql, error_code& ec) {
        return prepare(sql.string(), ec);
    }

    sqlite3_statement& _ensure_prepared(sqlite3_statement& st, error_code&) { return st; }

    template <typename Query, typename Handler>
    decltype(auto) execute(adio::sql::program<Query, Handler>& pr, error_code& ec) {
        auto&& stmt = _ensure_prepared(pr.query(), ec);
        if (ec) {
            return adio::sql::result(pr.result_handler());
        }
        return _execute_into(stmt, pr.result_handler(), ec);
    }

    template <typename Query, typename Handler>
    decltype(auto) execute(adio::sql::program<Query, Handler>&& pr, error_code& ec) {
        return execute(pr, ec);
    }

    template <typename String>
    decltype(auto) execute(const adio::sql_string<String>& sql, error_code& ec) {
        return execute(adio::sql::program(std::ref(sql), adio::sql::ignore_results), ec);
    }

    decltype(auto) execute(sqlite3_statement& st, error_code& ec) {
        return execute(adio::sql::program(std::ref(st), adio::sql::ignore_results), ec);
    }
};

}  // namespace detail

class sqlite3 {
    adio::thread_pool _pool;

    template <typename Task>
    void _post_task(Task&& t) {
        using adio::post;
        post(_pool, ADIO_FWD(t));
    }

public:
    using connection = basic_connection<sqlite3>;
    using row        = detail::sqlite3_row;

public:
    using implementation_type = detail::sqlite3_conn;

    template <typename Ctx>
    implementation_type create_implementation(Ctx&) {
        return implementation_type{};
    }

    void connect(implementation_type& conn, const std::string& str, error_code& ec) {
        conn.connect(str, ec);
    }

    using connect_signature = void(error_code);
    template <typename Alloc, typename Exec, typename Handler>
    void async_connect(implementation_type& conn,
                       const std::string&   str,
                       Alloc&&,
                       const Exec& ex,
                       Handler&&   h) {
        _post_task([&conn, str, work = make_work_guard(ex), ex, h = ADIO_FWD(h), this]() mutable {
            adio::error_code ec;
            connect(conn, str, ec);
            using adio::post;
            post(ex, adio::detail::bind_handler(std::move(h), ec));
        });
    }

    void disconnect(implementation_type& conn, error_code& ec) { conn.disconnect(ec); }

    using disconnect_signature = void(error_code);
    template <typename Alloc, typename Exec, typename Handler>
    void async_disconnect(implementation_type& conn, Alloc&&, const Exec& ex, Handler&& h) {
        _post_task([&conn, work = make_work_guard(ex), ex, h = ADIO_FWD(h), this]() mutable {
            adio::error_code ec;
            disconnect(conn, ec);
            using adio::post;
            post(ex, adio::detail::bind_handler(std::move(h), ec));
        });
    }

    using statement_type = detail::sqlite3_statement;

    statement_type prepare(implementation_type& conn, const std::string& query, error_code& ec) {
        return conn.prepare(query, ec);
    }

    template <typename>
    using prepare_signature_t = void(statement_type, error_code);

    template <typename Alloc, typename Exec, typename Handler>
    void async_prepare(implementation_type& conn,
                       const std::string&   query,
                       Alloc&&,
                       const Exec& ex,
                       Handler&&   h) {
        _post_task([&conn, query, work = make_work_guard(ex), ex, h = ADIO_FWD(h), this]() mutable {
            adio::error_code ec;
            statement_type   stmt = prepare(conn, query, ec);
            using adio::post;
            post(ex, adio::detail::bind_handler(std::move(h), std::move(stmt), ec));
        });
    }

    template <typename Query>
    decltype(auto) execute(implementation_type& conn, Query&& q, error_code& ec) {
        return conn.execute(ADIO_FWD(q), ec);
    }

    template <typename>
    using execute_signature_t = void(error_code);

    template <typename Exec, typename Handler>
    void async_execute(implementation_type& conn, statement_type&& st, Exec&& ex, Handler&& h) {
        _post_task([&conn,
                    st   = std::move(st),
                    work = make_work_guard(ex),
                    ex,
                    h = ADIO_FWD(h),
                    this]() mutable {
            adio::error_code ec;
            execute(conn, st, ec);
            using adio::post;
            post(ex, adio::detail::bind_handler(std::move(h), ec));
        });
    }

    /**
     * HELLO!
     *
     * If you called this deleted function, it means you tried passing an l-value
     * reference to a statement to async_execute. This is very dangerous, as the
     * statement must outlive the asynchronous operation.
     *
     * You must do one of two things:
     *
     * - `std::move()` the statement obect into async_execute()
     * - Call `std::ref()` to explicitly call out the statement object as a
     *   reference
     */
    template <typename Exec, typename Handler>
    void async_execute(implementation_type&, statement_type& st, Exec&&, Handler&&) = delete;

    template <typename Alloc, typename Exec, typename Handler>
    void async_execute(implementation_type&                   conn,
                       std::reference_wrapper<statement_type> st,
                       Alloc&&,
                       Exec&&    ex,
                       Handler&& h) {
        _post_task([&conn, st, work = make_work_guard(ex), ex, h = ADIO_FWD(h), this]() mutable {
            adio::error_code ec;
            execute(conn, st, ec);
            using adio::post;
            post(ex, adio::detail::bind_handler(h, ec));
        });
    }

    std::string last_error(const implementation_type& conn) const noexcept {
        return conn.last_error();
    }

    /// Enumeration of SQLite error codes. See the SQLite documentation for an
    /// explanation of each
    enum class errc {
        abort          = 4,
        auth           = 23,
        busy           = 5,
        cant_open      = 14,
        constraint     = 19,
        corrupt        = 11,
        done           = 101,
        empty          = 16,
        error          = 1,
        format         = 24,
        full           = 13,
        internal       = 2,
        interrupt      = 9,
        ioerr          = 10,
        locked         = 6,
        mismatch       = 20,
        misuse         = 21,
        nolfs          = 22,
        no_memory      = 7,
        not_a_database = 26,
        not_found      = 12,
        notice         = 27,
        ok             = 0,
        perm           = 3,
        protocol       = 15,
        range          = 25,
        readonly       = 8,
        row            = 100,
        schema         = 17,
        too_big        = 18,
        warning        = 28,

        abort_rollback              = 516,
        busy_recovery               = 261,
        busy_snapshot               = 517,
        cant_open_convert_path      = 1038,
        cant_open_full_path         = 782,
        cant_open_is_directory      = 526,
        cant_open_no_temp_directory = 270,
        constraint_check            = 275,
        constraint_commit_hook      = 531,
        constraint_foreign_key      = 787,
        constraint_function         = 1043,
        constraint_not_null         = 1299,
        constraint_primary_key      = 1555,
        constraint_rowid            = 2579,
        constraint_trigger          = 1811,
        constraint_unique           = 2067,
        constraint_vtab             = 2323,
        corrubt_vtab                = 267,
        ioerr_access                = 3338,
        ioerr_blocked               = 2826,
        ioerr_check_reserved_lock   = 3594,
        ioerr_close                 = 4106,
        ioerr_convert_path          = 6666,
        ioerr_delete                = 2570,
        ioerr_delete_noent          = 5898,
        ioerr_dir_close             = 4362,
        ioerr_dir_fsync             = 1290,
        ioerr_fstat                 = 1802,
        ioerr_fsync                 = 1034,
        ioerr_gettemppath           = 6410,
        ioerr_lock                  = 3850,
        ioerr_mmap                  = 6154,
        ioerr_nomem                 = 3082,
        ioerr_rdlock                = 2314,
        ioerr_read                  = 266,
        ioerr_seek                  = 5642,
        ioerr_shmlock               = 5130,
        ioerr_shmmap                = 5386,
        ioerr_shmopen               = 4618,
        ioerr_shmsize               = 4874,
        ioerr_short_read            = 522,
        ioerr_truncate              = 1546,
        ioerr_unlock                = 2058,
        ioerr_write                 = 778,
        locked_sharedcache          = 262,
        notive_recover_rollback     = 539,
        notice_recover_wal          = 283,
        readonly_cantlock           = 520,
        readonly_dbmoved            = 1032,
        readonly_recovery           = 264,
        readonly_rollback           = 776,
        warning_autoindex           = 284,
    };

    /// Obtain an instance of the SQLite error category.
    static const adio::error_category& error_category();
};

inline error_code make_error_code(sqlite3::errc e) {
    return adio::error_code{static_cast<int>(e), sqlite3::error_category()};
}

inline error_condition make_error_condition(sqlite3::errc e) {
    return {static_cast<int>(e), sqlite3::error_category()};
}

ADIO_NAMESPACE_END

ADIO_DECLARE_ERRC_ENUM(ADIO_NAMESPACE::sqlite3::errc);

#endif  // ADIO_DRIVERS_SQLITE3_SQLITE3_HPP_INCLUDED
