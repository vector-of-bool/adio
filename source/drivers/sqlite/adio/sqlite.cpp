#include <adio/sqlite.hpp>

#include <sqlite3.h>

#include <adio/sql/value.hpp>

using namespace adio;
using detail::sqlite_statement;
using detail::sqlite_service;

namespace adio
{

namespace detail
{

class sqlite_category : public asio_error_category
{
    const char* name() const noexcept override { return "adio::sqlite"; }
    string message(int e) const override { return ::sqlite3_errstr(e); }
};

struct sqlite_private
{
    ::sqlite3* db = nullptr;
    ~sqlite_private()
    {
        if (db) ::sqlite3_close(db);
    }
};

struct sqlite_statement_private
{
    ::sqlite3_stmt* st = nullptr;
    ~sqlite_statement_private()
    {
        if (st) ::sqlite3_finalize(st);
    }
};

} /* detail */

} /* adio */

const asio_error_category& adio::sqlite_category()
{
    static detail::sqlite_category cat;
    return cat;
}

sqlite::sqlite(service& service)
    : _parent_ios{service.get_io_service()}
    , _service{service}
    , _private{new detail::sqlite_private}
{
}

sqlite_statement::sqlite_statement() = default;
sqlite_statement::~sqlite_statement() = default;
sqlite_statement::sqlite_statement(
    std::shared_ptr<sqlite_statement_private>&& p)
    : _private{std::move(p)}
{
}

bool sqlite_statement::_advance()
{
    auto rc = ::sqlite3_step(_private->st);
    switch (rc)
    {
    case SQLITE_DONE:
        _done = true;
        return true;
    case SQLITE_ROW:
        return false;
    case SQLITE_MISUSE:
        std::terminate();
    default:
        throw system_error(make_error_code(static_cast<sqlite_errc>(rc)),
                           "Error advancing prepared statement");
    }
}

void sqlite_statement::execute(error_code& ec)
{
    while (1)
    {
        auto rc = ::sqlite3_step(_private->st);
        switch (rc)
        {
        case SQLITE_DONE:
            _done = true;
            return;
        case SQLITE_ROW:
            ec = make_error_code(static_cast<sqlite_errc>(SQLITE_OK));
            return;
        default:
            ec = make_error_code(static_cast<sqlite_errc>(rc));
            return;
        }
    }
}

sqlite::sqlite(sqlite&&) = default;
sqlite& sqlite::operator=(sqlite&&) = default;

sqlite::~sqlite() { close(); }

error_code sqlite::open(const string& path)
{
    close();
    const auto err = ::sqlite3_open(path.data(), &_private->db);
    if (err != SQLITE_OK) return make_error_code(static_cast<sqlite_errc>(err));
    return {};
}

std::shared_ptr<detail::sqlite_statement_private>
sqlite::_prepare(const string& str, error_code& e) const
{
    if (!_private->db)
    {
        e = make_error_code(adio::sys_errc::not_connected);
        return {};
    }
    auto p = detail::make_unique<detail::sqlite_statement_private>();
    auto err = ::sqlite3_prepare_v2(_private->db,
                                    str.data(),
                                    str.size(),
                                    &p->st,
                                    nullptr);
    if (err)
    {
        if (err != SQLITE_OK)
            e = make_error_code(static_cast<sqlite_errc>(err));
        return nullptr;
    }
    return {std::move(p)};
}

std::vector<sqlite::statement> sqlite::_multi_prepare(const string& source,
                                                      error_code& e) const
{
    if (!_private->db)
    {
        e = make_error_code(adio::sys_errc::not_connected);
        return {};
    }
    auto cur_ptr = source.data();
    auto remaining = source.size();
    std::vector<sqlite::statement> ret;
    while (cur_ptr)
    {
        auto next_ptr = cur_ptr;
        auto p = detail::make_unique<detail::sqlite_statement_private>();
        auto err = ::sqlite3_prepare_v2(_private->db,
                                        cur_ptr,
                                        remaining,
                                        &p->st,
                                        &next_ptr);
        if (err != SQLITE_OK)
        {
            e = make_error_code(static_cast<sqlite_errc>(err));
            return {};
        }
        ret.emplace_back(std::move(p));
        if (next_ptr == nullptr) break;
        // There's another statement to compile. Go around again
        remaining -= std::distance(cur_ptr, next_ptr);
        cur_ptr = next_ptr;
    }
    return ret;
}

void sqlite::close()
{
    if (_private->db)
    {
        ::sqlite3_close(_private->db);
        _private->db = nullptr;
    }
}

sqlite_service::sqlite_service(io_service& ios)
    : super_type{ios}
    , _my_ios{std::thread::hardware_concurrency() * 2}
    , _workptr{new io_service::work{_my_ios}}
{
}

sqlite_service::~sqlite_service()
{
    _workptr.reset();
    _my_ios.stop();
    for (auto& t : _threads) t.join();
}

void sqlite_service::_ensure_threads_started()
{
    std::call_once(_start_threads_flag, [this] {
        _threads.resize(std::thread::hardware_concurrency());
        for (auto& t : _threads) t = std::thread{[this] { _my_ios.run(); }};
    });
}

void sqlite_service::_start_task(std::function<void()> task)
{
    _ensure_threads_started();
    _my_ios.post(std::move(task));
}

row sqlite_statement::current_row() const
{
    const auto pst = _private->st;
    const auto num_columns = ::sqlite3_column_count(pst);
    std::vector<adio::value> values;
    values.reserve(num_columns);

    for (auto i = 0; i < num_columns; ++i)
    {
        const auto sql_type = ::sqlite3_column_type(pst, i);

        switch (sql_type)
        {
        case SQLITE_INTEGER:
            values.emplace_back(value::integer{::sqlite3_column_int64(pst, i)});
            break;
        case SQLITE_FLOAT:
            values.emplace_back(::sqlite3_column_double(pst, i));
            break;
        case SQLITE_TEXT:
        {
            const auto ptr = ::sqlite3_column_text(pst, i);
            const auto len = ::sqlite3_column_bytes(pst, i);
            std::string str{ptr, ptr + len};
            values.emplace_back(std::move(str));
            break;
        }
        case SQLITE_BLOB:
        {
            const auto ptr
                = reinterpret_cast<const char*>(::sqlite3_column_blob(pst, i));
            const auto len = ::sqlite3_column_bytes(pst, i);
            std::vector<char> data{ptr, ptr + len};
            values.emplace_back(std::move(data));
            break;
        }
        case SQLITE_NULL:
            values.emplace_back(nullptr);
            break;
        default:
            assert(0);
        }
    }

    return row{std::move(values)};
}

void sqlite_statement::bind(int index, const value& value)
{
    const auto pst = _private->st;
    int rc = SQLITE_OK;
    switch (value.get_type())
    {
    case type::null_t:
        rc = ::sqlite3_bind_null(pst, index);
        break;
    case type::integer:
        rc = ::sqlite3_bind_int64(pst, index, value.get<value::integer>());
        break;
    case type::real:
        rc = ::sqlite3_bind_double(pst, index, value.get<value::real>());
        break;
    case type::text:
    {
        const auto& str = value.get<value::text>();
        rc = ::sqlite3_bind_text(pst,
                                 index,
                                 str.data(),
                                 str.size(),
                                 SQLITE_TRANSIENT);
        break;
    }
    case type::blob:
    {
        const auto& bytes = value.get<value::blob>();
        rc = ::sqlite3_bind_blob(pst,
                                 index,
                                 bytes.data(),
                                 bytes.size(),
                                 SQLITE_TRANSIENT);
        break;
    }
    case type::datetime:
    {
        const value::integer time
            = value.get<value::datetime>().time_since_epoch().count();
        rc = ::sqlite3_bind_int64(pst, index, time);
        break;
    }
    default:
        assert(0);
        std::terminate();
    }
    switch (rc)
    {
    case SQLITE_NOMEM:
        throw std::bad_alloc{};
    case SQLITE_MISUSE:
        std::terminate();
    case SQLITE_RANGE:
        throw std::out_of_range{"Parameter index out of range"};
    case SQLITE_TOOBIG:
        throw std::domain_error{"Parameter is too big"};
    case SQLITE_OK:
        return;
    default:
        throw system_error(make_error_code(static_cast<sqlite_errc>(rc)),
                           "Failed to bind parameter");
    }
}

void sqlite_statement::bind(const std::string& name, const value& value)
{
    auto nparam = ::sqlite3_bind_parameter_count(_private->st);
    assert(nparam > 0);
    const auto index
        = ::sqlite3_bind_parameter_index(_private->st, name.data());
    if (index == 0) throw std::domain_error{"No such parameter: " + name};
    bind(index, value);
}
