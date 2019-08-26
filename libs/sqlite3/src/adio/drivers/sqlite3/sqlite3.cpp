#include "sqlite3.hpp"

#include "./third/sqlite3.h"

using namespace adio;

namespace {
class sqlite_category : public error_category {
    const char* name() const noexcept override { return "adio::sqlite"; }
    std::string message(int e) const override { return ::sqlite3_errstr(e); }
};

error_code to_error_code(int err) { return make_error_code(static_cast<adio::sqlite3::errc>(err)); }
void       set_error_code(error_code& ec, int err) {
    ec = to_error_code(err);
    if (ec == adio::sqlite3::errc::no_memory) {
        throw std::bad_alloc();
    }
}
}  // namespace

const error_category& adio::sqlite3::error_category() {
    static class ::sqlite_category cat;
    return cat;
}

#define MY_DB_PTR static_cast<::sqlite3*>(this->_database_void)

detail::sqlite3_conn::~sqlite3_conn() {
    error_code ec;
    disconnect(ec);
}

void detail::sqlite3_conn::disconnect(error_code& ec) {
    set_error_code(ec, ::sqlite3_close(MY_DB_PTR));
}

void detail::sqlite3_conn::connect(const std::string& str, error_code& ec) {
    disconnect(ec);
    ec                = {};
    ::sqlite3* new_db = nullptr;
    set_error_code(ec, ::sqlite3_open(str.data(), &new_db));
    _database_void = new_db;
}

std::string detail::sqlite3_conn::last_error() const noexcept {
    return ::sqlite3_errmsg(MY_DB_PTR);
}

detail::sqlite3_statement detail::sqlite3_conn::prepare(const std::string_view& query,
                                                        error_code&             ec) {
    ec = {};
    ::sqlite3_stmt* ret;
    const char*     str_tail = nullptr;
    set_error_code(ec,
                   ::sqlite3_prepare_v2(MY_DB_PTR, query.data(), query.size(), &ret, &str_tail));
    return detail::sqlite3_statement(std::move(ret));
}

bool detail::sqlite3_conn::_step(detail::sqlite3_statement& st, error_code& ec) {
    ec          = {};
    auto st_ptr = static_cast<::sqlite3_stmt*>(st.get_void_ptr());
    while (1) {
        auto rc = ::sqlite3_step(st_ptr);
        if (rc == SQLITE_DONE) {
            set_error_code(ec, SQLITE_OK);
            goto stop;
        } else if (rc == SQLITE_ROW) {
            // Another row!
            return false;
        } else {
            ec = make_error_code(static_cast<adio::sqlite3::errc>(rc));
            if (ec) {
                goto stop;
            }
        }
    }
stop:
    ::sqlite3_reset(st_ptr);
    return true;
}

detail::sqlite3_row detail::sqlite3_conn::_get_row(detail::sqlite3_statement& st) {
    const auto                 pst         = static_cast<::sqlite3_stmt*>(st.get_void_ptr());
    const auto                 num_columns = ::sqlite3_column_count(pst);
    std::vector<sqlite3_value> values;
    values.reserve(num_columns);

    for (auto i = 0; i < num_columns; ++i) {
        const auto sql_type = ::sqlite3_column_type(pst, i);

        switch (sql_type) {
        case SQLITE_INTEGER:
            values.emplace_back(::sqlite3_column_int64(pst, i));
            break;
        case SQLITE_FLOAT:
            values.emplace_back(::sqlite3_column_double(pst, i));
            break;
        case SQLITE_TEXT: {
            const auto  ptr = ::sqlite3_column_text(pst, i);
            const auto  len = ::sqlite3_column_bytes(pst, i);
            std::string str{ptr, ptr + len};
            values.emplace_back(std::move(str));
            break;
        }
        case SQLITE_BLOB: {
            const auto        ptr = reinterpret_cast<const char*>(::sqlite3_column_blob(pst, i));
            const auto        len = ::sqlite3_column_bytes(pst, i);
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

    return sqlite3_row{std::move(values)};
}

#define MY_STMT static_cast<::sqlite3_stmt*>(_stmt_void)

detail::sqlite3_statement::~sqlite3_statement() { ::sqlite3_finalize(MY_STMT); }

void detail::sqlite3_statement::_bind_i64(std::size_t idx, std::int64_t val) {
    assert(_stmt_void);
    error_code ec;
    set_error_code(ec, ::sqlite3_bind_int64(MY_STMT, idx + 1, val));
    throw_if_error(ec, "sqlite3_bind_int64()");
}

void detail::sqlite3_statement::_bind_double(std::size_t idx, double val) {
    error_code ec;
    set_error_code(ec, ::sqlite3_bind_double(MY_STMT, idx + 1, val));
    throw_if_error(ec, "sqlite3_bind_double()");
}

void detail::sqlite3_statement::_bind_string(std::size_t idx, std::string_view str) {
    error_code ec;
    set_error_code(ec,
                   ::sqlite3_bind_text(MY_STMT, idx + 1, str.data(), str.size(), SQLITE_TRANSIENT));
    throw_if_error(ec, "sqlite3_bind_text()");
}
