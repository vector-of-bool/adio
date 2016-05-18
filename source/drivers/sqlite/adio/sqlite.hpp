#ifndef ADIO_SQLITE_DRIVER_HPP_INCLUDED
#define ADIO_SQLITE_DRIVER_HPP_INCLUDED

#include <adio/connection_fwd.hpp>
#include <adio/service.hpp>
#include <adio/error.hpp>
#include <adio/sql/row.hpp>
#include <adio/utils.hpp>

#include <future>
#include <memory>
#include <string>

namespace adio
{

/// Enumeration of SQLite error codes. See the SQLite documentation for an
/// explanation of each
enum class sqlite_errc
{
    abort = 4,
    auth = 23,
    busy = 5,
    cant_open = 14,
    constraint = 19,
    corrupt = 11,
    done = 101,
    empty = 16,
    error = 1,
    format = 24,
    full = 13,
    internal = 2,
    interrupt = 9,
    ioerr = 10,
    locked = 6,
    mismatch = 20,
    misuse = 21,
    nolfs = 22,
    no_memory = 7,
    not_a_database = 26,
    not_found = 12,
    notice = 27,
    ok = 0,
    perm = 3,
    protocol = 15,
    range = 25,
    readonly = 8,
    row = 100,
    schema = 17,
    too_big = 18,
    warning = 28,

    abort_rollback = 516,
    busy_recovery = 261,
    busy_snapshot = 517,
    cant_open_convert_path = 1038,
    cant_open_full_path = 782,
    cant_open_is_directory = 526,
    cant_open_no_temp_directory = 270,
    constraint_check = 275,
    constraint_commit_hook = 531,
    constraint_foreign_key = 787,
    constraint_function = 1043,
    constraint_not_null = 1299,
    constraint_primary_key = 1555,
    constraint_rowid = 2579,
    constraint_trigger = 1811,
    constraint_unique = 2067,
    constraint_vtab = 2323,
    corrubt_vtab = 267,
    ioerr_access = 3338,
    ioerr_blocked = 2826,
    ioerr_check_reserved_lock = 3594,
    ioerr_close = 4106,
    ioerr_convert_path = 6666,
    ioerr_delete = 2570,
    ioerr_delete_noent = 5898,
    ioerr_dir_close = 4362,
    ioerr_dir_fsync = 1290,
    ioerr_fstat = 1802,
    ioerr_fsync = 1034,
    ioerr_gettemppath = 6410,
    ioerr_lock = 3850,
    ioerr_mmap = 6154,
    ioerr_nomem = 3082,
    ioerr_rdlock = 2314,
    ioerr_read = 266,
    ioerr_seek = 5642,
    ioerr_shmlock = 5130,
    ioerr_shmmap = 5386,
    ioerr_shmopen = 4618,
    ioerr_shmsize = 4874,
    ioerr_short_read = 522,
    ioerr_truncate = 1546,
    ioerr_unlock = 2058,
    ioerr_write = 778,
    locked_sharedcache = 262,
    notive_recover_rollback = 539,
    notice_recover_wal = 283,
    readonly_cantlock = 520,
    readonly_dbmoved = 1032,
    readonly_recovery = 264,
    readonly_rollback = 776,
    warning_autoindex = 284,
};

/// Obtain an instance of the SQLite error category.
extern const asio_error_category& sqlite_category();

inline error_code make_error_code(sqlite_errc e)
{
    return adio::error_code{static_cast<int>(e), sqlite_category()};
}

inline error_condition make_error_condition(sqlite_errc e)
{
    return {static_cast<int>(e), sqlite_category()};
}

namespace detail
{

struct sqlite_private;

class sqlite_statement_private;
class sqlite_statement;

class sqlite_statement
{
    std::shared_ptr<sqlite_statement_private> _private;
    friend class sqlite_row_iterator;

    bool _advance();

    bool _done = false;

public:
    sqlite_statement();
    explicit sqlite_statement(std::shared_ptr<sqlite_statement_private>&& p);
    ~sqlite_statement();
    sqlite_statement(sqlite_statement&&) = default;
    sqlite_statement(const sqlite_statement&) = delete;
    sqlite_statement& operator=(sqlite_statement&&) = default;
    sqlite_statement& operator=(const sqlite_statement&) = delete;

    using row = adio::row;

    row current_row() const;

    class iterator : public std::iterator<std::input_iterator_tag, row>
    {
        std::reference_wrapper<sqlite_statement> _st;
        bool _is_end = false;
        void _advance();

    public:
        iterator(sqlite_statement& st, bool end)
            : _st{st}
            , _is_end{end}
        {
            if (!end) ++*this;
        }

        iterator& operator++()
        {
            _is_end = _st.get()._advance();
            return *this;
        }

        bool operator!=(const iterator& other)
        {
            return other._is_end != _is_end;
        }

        row operator*() const { return _st.get().current_row(); }
    };

    iterator begin() { return {*this, false}; };
    iterator end() { return {*this, true}; }

    void execute()
    {
        error_code ec;
        execute(ec);
        detail::throw_if_error(ec, "Failed to execute prepared statement");
    }
    void execute(error_code& ec);

    void bind(int index, const value& value);
    void bind(const std::string& name, const value& value);

    bool done() const { return _done; }
};

class sqlite_service;

} /* detail */

class sqlite;

class sqlite : public std::enable_shared_from_this<sqlite>
{
public:
    using statement = detail::sqlite_statement;
    using row = statement::row;
    using connection = basic_connection<sqlite>;
    using service = detail::sqlite_service;

private:
    std::reference_wrapper<io_service> _parent_ios;
    std::reference_wrapper<service> _service;
    std::unique_ptr<detail::sqlite_private> _private;

    template <typename Task> void _push_task(Task&& task);

    std::shared_ptr<detail::sqlite_statement_private>
    _prepare(const string&, error_code&) const;

    std::vector<statement> _multi_prepare(const string&, error_code&) const;

public:
    sqlite(service& service);
    sqlite(sqlite&&);
    sqlite& operator=(sqlite&&);
    ~sqlite();

    void close();

    using open_handler_signature = void(error_code);
    error_code open(const string&);
    template <typename Handler>
    void async_open(const string& path, Handler&& handler)
    {
        auto this_pin = shared_from_this();
        _push_task([
            this_pin,
            work_pin = detail::make_work(_parent_ios),
            this,
            path,
            handler = std::forward<Handler>(handler)
        ] {
            auto ec = open(path);
            _parent_ios.get().post(std::bind(handler, ec));
        });
    }

    using prepare_handler_signature = void(statement, error_code);
    statement prepare(const string& str)
    {
        error_code ec;
        auto st = prepare(str, ec);
        detail::throw_if_error(ec,
                               "Failed to prepare statement: \"" + str + "\"");
        return st;
    }
    statement prepare(const string& query, error_code& ec)
    {
        return statement{_prepare(query, ec)};
    }
    using prepare_multiple_handler_signature
        = void(std::vector<statement>, error_code);
    std::vector<statement> prepare_multiple(const string& source)
    {
        error_code ec;
        auto sts = prepare_multiple(source, ec);
        detail::throw_if_error(ec,
                               "Failed to prepare SQL source: \"" + source
                                   + "\"");
        return sts;
    }

    std::vector<statement> prepare_multiple(const string& source,
                                            error_code& ec)
    {
        return _multi_prepare(source, ec);
    }

    template <typename Handler>
    void async_prepare(const string& query, Handler&& handler)
    {
        _push_task([
            this_pin = shared_from_this(),
            work_pin = detail::make_work(_parent_ios),
            this,
            query,
            handler = std::forward<Handler>(handler)
        ] {
            error_code err;
            auto st = _prepare(query, err);
            _parent_ios.get().post([st, err, handler]() mutable {
                handler(statement{std::move(st)}, err);
            });
        });
    }

    using execute_handler_signature = void(error_code);
    void execute(statement&& st) { execute(st); }
    void execute(statement& st)
    {
        error_code ec;
        execute(st, ec);
        detail::throw_if_error(ec, "Failed to execute prepared statement");
    }
    void execute(statement&& st, error_code& ec) { execute(st, ec); }
    void execute(statement& st, error_code& ec) { st.execute(ec); }
    template <typename Handler>
    void async_execute(statement& st, Handler&& handler)
    {
        _push_task([
            this_pin = shared_from_this(),
            work_pin = detail::make_work(_parent_ios),
            this,
            st_ref = std::ref(st),
            handler = std::forward<Handler>(handler)
        ]() mutable {
            error_code ec;
            execute(st_ref, ec);
            _parent_ios.get().post(std::bind(handler, ec));
        });
    }

    void execute(const string& query)
    {
        error_code ec;
        execute(query, ec);
        detail::throw_if_error(ec, "Failed to execute query: " + query);
    }
    void execute(const string& query, error_code& ec)
    {
        ec = {};
        auto st = prepare(query, ec);
        if (ec) return;
        execute(st, ec);
    }
    template <typename Handler>
    void async_execute(const string& query, Handler&& handler)
    {
        async_prepare(query, [
            this_pin = shared_from_this(),
            work_pin = detail::make_work(_parent_ios),
            this,
            handler = std::forward<Handler>(handler)
        ](statement st, error_code ec) {
            if (ec)
            {
                _parent_ios.get().post(std::bind(handler, ec));
                return;
            }
            async_execute([
                this_pin,
                work_pin,
                this,
                handler = std::forward<Handler>(handler)
            ](error_code ec) {
                _parent_ios.get().post(std::bind(handler, ec));
            });
        });
    }

    using step_handler_signature = void(row, error_code);
    row step(statement& st)
    {
        error_code ec;
        const auto result = step(st, ec);
        detail::throw_if_error(ec, "Failed to step query");
        return result;
    }
    row step(statement& st, error_code& ec)
    {
        ec = {};
        st.execute(ec);
        if (ec || st.done()) return row({});
        return st.current_row();
    }
    template <typename Handler> void async_step(statement& st, Handler&& h)
    {
        _push_task([
            this_pin = shared_from_this(),
            work_pin = detail::make_work(_parent_ios),
            this,
            st_ref = std::ref(st),
            handler = std::forward<Handler>(h)
        ]() mutable {
            error_code ec;
            step(st_ref, ec);
            _parent_ios.get().post(std::bind(handler, ec));
        });
    }
};

namespace detail
{

class sqlite_service : public db_service_base<sqlite_service, sqlite>
{
public:
    using self_type = sqlite_service;
    using super_type = db_service_base<sqlite_service, sqlite>;

private:
    friend class adio::sqlite;

    io_service _my_ios;
    std::unique_ptr<io_service::work> _workptr;
    std::once_flag _start_threads_flag;
    std::vector<std::thread> _threads;

    void _ensure_threads_started();
    void _start_task(std::function<void()>);

    template <typename Task> void _push_task(Task&& task)
    {
        std::function<void()> pt{std::forward<Task>(task)};
        _start_task(std::move(pt));
    }

public:
    sqlite_service(io_service&);
    ~sqlite_service();

    using connection_type = sqlite;
};

} /* detail */

template <typename Task> void sqlite::_push_task(Task&& task)
{
    _service.get()._push_task(std::forward<Task>(task));
}

} /* adio */

ADIO_DECLARE_ERRC_ENUM(adio::sqlite_errc);

#endif  // ADIO_SQLITE_DRIVER_HPP_INCLUDED
