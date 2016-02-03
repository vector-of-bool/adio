#include <catch/catch.hpp>

#include <adio/connection.hpp>
#include <adio/sqlite.hpp>

#include <boost/asio/spawn.hpp>

#define DECL_CON                                                               \
    adio::io_service ios;                                                      \
    adio::sqlite::connection con { ios }
#define DECL_OPEN                                                              \
    DECL_CON;                                                                  \
    con.open("foo.db")

TEST_CASE("Open SQLite database")
{
    DECL_CON;
    auto ec = con.open("test.db");
    CHECK_FALSE(ec);
}

TEST_CASE("Async open SQLite database")
{
    DECL_CON;
    bool worked = false;
    con.async_open("test.db", [&](adio::error_code) { worked = true; });
    ios.run();
    CHECK(worked);
}

TEST_CASE("Spawn open SQLite database")
{
    DECL_CON;
    bool worked = false;
    adio::asio::spawn(ios, [&](adio::asio::yield_context yc) {
        auto ec = make_error_code(adio::sqlite_errc::ioerr_read);
        con.async_open("test.db", yc[ec]);
        CHECK_FALSE(ec);
        worked = true;
    });
    ios.run();
    CHECK(worked);
}

TEST_CASE("Bad open sqlite database")
{
    DECL_CON;
    auto ec = con.open("nonexistent-directory/foo.db");
    CHECK(ec);
}

TEST_CASE("Bad async open sqlite database")
{
    DECL_CON;
    adio::error_code ec;
    CHECK_FALSE(ec);
    con.async_open("nonexistent-dir/foo.db",
                   [&](adio::error_code err) { ec = err; });
    ios.run();
    CHECK(ec);
}

TEST_CASE("Bad spawn open sqlite database")
{
    DECL_CON;
    adio::error_code ec;
    adio::asio::spawn(ios, [&](adio::asio::yield_context yc) {
        con.async_open("nonexistent-dir/foo.db", yc[ec]);
    });
    CHECK_FALSE(ec);
    ios.run();
    CHECK(ec);
}

TEST_CASE("Bad spawn open with exception")
{
    DECL_CON;
    adio::asio::spawn(ios, [&](adio::asio::yield_context yc) {
        try
        {
            con.async_open("nonexistent-dir/foo.db", yc);
            CHECK(false);
        }
        catch (adio::system_error& e)
        {
            CHECK(e.code() == adio::sqlite_errc::cant_open);
        }
    });
    ios.run();
}

TEST_CASE("SQLite prepare")
{
    DECL_OPEN;
    con.prepare(
        "CREATE TABLE IF NOT EXISTS myTable (id INTEGER PRIMARY KEY, name "
        "VARCHAR(1024))");
}


TEST_CASE("SQLite async prepare")
{
    DECL_OPEN;
    bool did_call = false;
    con.async_prepare(
        "CREATE TABLE IF NOT EXISTS myTable (id INTEGER PRIMARY KEY, name "
        "VARCHAR(1024))",
        [&](adio::sqlite::connection::statement, adio::error_code) {
            did_call = true;
        });
    ios.run();
    CHECK(did_call);
}


TEST_CASE("Bad SQLite prepare")
{
    DECL_OPEN;
    try
    {
        con.prepare(
            "CREATE WHERE TABLE IF NOT EXISTS myTable (id INTEGER PRIMARY KEY, "
            "name VARCHAR(1024))");
        CHECK(false);
    }
    catch (adio::system_error& e)
    {
        CHECK(e.code() == adio::sqlite_errc::error);
    }
}


TEST_CASE("Bad SQLite async prepare")
{
    DECL_OPEN;
    bool did_call = false;
    con.async_prepare(
        "CREATE WHERE TABLE IF NOT EXISTS myTable (id INTEGER PRIMARY KEY, "
        "name VARCHAR(1024))",
        [&](adio::sqlite::connection::statement, adio::error_code e) {
            did_call = true;
            CHECK(e == adio::sqlite_errc::error);
        });
    ios.run();
    CHECK(did_call);
}


TEST_CASE("SQLite execute")
{
    DECL_OPEN;
    con.execute(
        "CREATE TABLE IF NOT EXISTS myTable (id INTEGER PRIMARY KEY, name "
        "VARCHAR(1024))");
}


TEST_CASE("Bad SQLite execute")
{
    adio::io_service ios;
    adio::sqlite::connection con{ios};
    adio::error_code ec;
    con.execute("CREATE FOO BLARGH", ec);
    CHECK(ec);
}


TEST_CASE("Iterate over data")
{
    DECL_OPEN;
    con.prepare("DROP TABLE IF EXISTS myTable").execute();
    con.prepare(
           "CREATE TABLE IF NOT EXISTS myTable(id INTEGER PRIMARY KEY, name "
           "VARCHAR(1024))")
        .execute();
    con.prepare("INSERT INTO myTable (name) VALUES ('Hats')").execute();
    con.prepare("INSERT INTO myTable (name) VALUES ('Hats')").execute();
    con.prepare("INSERT INTO myTable (name) VALUES ('Hats')").execute();
    auto st = con.prepare("SELECT * FROM myTable");
    int count = 0;
    for (const auto& item : st)
    {
        count++;
        CHECK(item[0] == count);
        CHECK(item[1] == "Hats");
        CHECK(item.size() == 2);
    }
    CHECK(count == 3);
}


TEST_CASE("Bind indexed parameters")
{
    DECL_OPEN;
    con.execute("DROP TABLE IF EXISTS hats");
    con.execute(R"(
        CREATE TABLE IF NOT EXISTS hats (
                id INTEGER PRIMARY KEY NOT NULL,
                name TEXT NOT NULL,
                color VARCHAR(16) NOT NULL
                ))");
    con.execute("INSERT INTO hats (name, color) VALUES ('top', 'black')");
    auto st = con.prepare("SELECT name FROM hats WHERE color = ?");
    st.bind(1, adio::value{"black"});
    std::vector<adio::sqlite::row> rows{ begin(st), end(st) };
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].size() == 1);
    CHECK(rows[0][0] == "top");
}


TEST_CASE("Bind named parameters")
{
    DECL_OPEN;
    con.execute("DROP TABLE IF EXISTS hats");
    con.execute(R"(
        CREATE TABLE IF NOT EXISTS hats (
                id INTEGER PRIMARY KEY NOT NULL,
                type TEXT NOT NULL,
                color VARCHAR(16) NOT NULL
                ))");
    con.execute("INSERT INTO hats (type, color) VALUES ('top', 'black')");
    con.execute("INSERT INTO hats (type, color) VALUES ('top', 'green')");
    auto st = con.prepare("SELECT type FROM hats WHERE color = :color");
    st.bind(":color", adio::value{ "black" });
    std::vector<std::string> rows{ begin(st), end(st) };
    REQUIRE(rows.size() == 1);
    CHECK(rows[0] == "top");
}
