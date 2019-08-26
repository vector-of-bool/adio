#include <catch2/catch.hpp>

#include <adio/sqlite3.hpp>

#include <adio/sql/program.hpp>

using namespace adio::literals;

class sqlite_conn_fixture {
public:
    adio::io_context          ioc;
    adio::error_code          ec;
    adio::sqlite3::connection conn{ioc};

    void connect_mem() { conn.connect(":memory:", ec); }

    void make_simple_table_1() {
        conn.execute("CREATE TABLE my_table (val INTEGER, str TEXT)"_sql);
    }

    ~sqlite_conn_fixture() { conn.disconnect(); }
};

TEST_CASE_METHOD(sqlite_conn_fixture, "Create empty connection") {}

TEST_CASE_METHOD(sqlite_conn_fixture, "Connect to DB in memory") {
    connect_mem();
    CHECK_FALSE(ec);
}

TEST_CASE_METHOD(sqlite_conn_fixture, "Connect to bad DB file") {
    conn.connect("/foo/bar/bad-path/not-real.db", ec);
    CHECK(ec == adio::sqlite3::errc::cant_open);
}

TEST_CASE_METHOD(sqlite_conn_fixture, "Failed to run invalid SQL") {
    connect_mem();
    CHECK(conn.last_error() == "not an error");
    conn.execute("This is invalid sql"_sql, ec);
    CHECK(ec == adio::sqlite3::errc::error);
    CHECK(conn.last_error().find("syntax") != std::string::npos);
}

TEST_CASE_METHOD(sqlite_conn_fixture, "Run valid sql") {
    connect_mem();
    conn.execute("CREATE TABLE my_table (val INTEGER)"_sql);
    // Trying to create the same table again is an error:
    CHECK_THROWS_AS(conn.execute("CREATE TABLE my_table (val INTEGER)"_sql), adio::system_error);
    // IF NOT EXISTS:
    conn.execute("CREATE TABLE IF NOT EXISTS my_table (val INTEGER)"_sql);
}

TEST_CASE_METHOD(sqlite_conn_fixture, "Insert values") {
    connect_mem();
    make_simple_table_1();
    auto stmt = conn.prepare("INSERT INTO my_table VALUES (?, ?)");
    conn.execute(stmt.bind(12, "I am a string"));
    conn.execute(stmt.bind(4, "I am a different string"));
}

TEST_CASE_METHOD(sqlite_conn_fixture, "Retrieve values") {
    connect_mem();
    make_simple_table_1();
    auto stmt = conn.prepare("INSERT INTO my_table VALUES (?, ?)");
    conn.execute(stmt.bind(12, "I am a string"));
    conn.execute(stmt.bind(4, "I am a different string"));

    stmt = conn.prepare("SELECT * FROM my_table");
    std::vector<adio::sqlite3::row> rows;
    conn.execute(adio::sql::program(std::ref(stmt), adio::sql::into_container(rows)));
    CHECK(rows.size() == 2);
    CHECK(rows[0][0].as<std::int64_t>() == 12);
    CHECK(rows[1][0].as<std::int64_t>() == 4);
}

TEST_CASE_METHOD(sqlite_conn_fixture, "Retrieve values into std::tuples") {
    connect_mem();
    make_simple_table_1();
    auto stmt = conn.prepare("INSERT INTO my_table VALUES (?, ?)");
    conn.execute(stmt.bind(12, "I am a string"));
    conn.execute(stmt.bind(4, "I am a different string"));

    stmt = conn.prepare("SELECT * FROM my_table");
    std::vector<std::tuple<std::int64_t, std::string>> rows;
    conn.execute(adio::sql::program(std::ref(stmt), adio::sql::into_container(rows)));
    REQUIRE(rows.size() == 2);
    {
        auto [int_, str] = rows[0];
        CHECK(int_ == 12);
        CHECK(str == "I am a string");
    }
    {
        auto [int_, str] = rows[1];
        CHECK(int_ == 4);
        CHECK(str == "I am a different string");
    }
}

TEST_CASE_METHOD(sqlite_conn_fixture, "Create a vector of tuples") {
    connect_mem();
    make_simple_table_1();
    auto stmt = conn.prepare("INSERT INTO my_table VALUES (?, ?)");
    conn.execute(stmt.bind(12, "I am a string"));
    conn.execute(stmt.bind(4, "I am a different string"));

    stmt = conn.prepare("SELECT * FROM my_table");
    const auto rows
        = conn.execute(adio::sql::program(std::ref(stmt),
                                          adio::sql::create_container<
                                              std::vector<std::tuple<std::int64_t, std::string>>>));
    REQUIRE(rows.size() == 2);
    auto [int1, str1] = rows[0];
    auto [int2, str2] = rows[1];
    CHECK(int1 == 12);
    CHECK(str1 == "I am a string");
    CHECK(int2 == 4);
    CHECK(str2 == "I am a different string");
}

TEST_CASE_METHOD(sqlite_conn_fixture, "Inline SQL in program") {
    connect_mem();
    make_simple_table_1();
    conn.execute("INSERT INTO my_table VALUES (12, 'I am a string')"_sql);

    auto rows
        = conn.execute(adio::sql::program("SELECT * FROM my_table"_sql,
                                          adio::sql::create_container<
                                              std::vector<std::tuple<std::int64_t, std::string>>>));
    CHECK(rows.size() == 1);
    auto [int1, str1] = rows[0];
    CHECK(int1 == 12);
    CHECK(str1 == "I am a string");
}

TEST_CASE_METHOD(sqlite_conn_fixture, "Get last row ID") {
    connect_mem();
    make_simple_table_1();
    conn.execute("INSERT INTO my_table VALUES (1, 'lolhi')"_sql);
    auto rid = conn.ext().last_insert_rowid();
    CHECK(rid == 1);
}
