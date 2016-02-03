#include <catch/catch.hpp>

#include <adio/connection.hpp>
#include <adio/empty.hpp>

#include <boost/asio/spawn.hpp>

TEST_CASE("Create empty driver")
{
    adio::asio::io_service ios;
    adio::empty_driver::connection db{ios};
    db.async_open("foo", []{});
    db.execute("SELECT * FROM stuff");

    bool did_run = true;
    db.async_execute("foo", [&] { std::cout << "We did a thing!\n"; });

    bool did_run2 = true;
    adio::asio::spawn(ios, [&](adio::asio::yield_context yc) {
        db.async_execute("SELECT foo FROM bars", yc);
        did_run2 = true;
        std::cout << "Coroutine returned\n";
    });

    ios.run();
    CHECK(did_run);
    CHECK(did_run2);
}
