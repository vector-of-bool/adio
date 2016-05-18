#include <catch/catch.hpp>

#include <adio/postgresql.hpp>
#include <adio/connection.hpp>


TEST_CASE("Open a connection")
{
    adio::io_service ios;
    adio::pg::connection conn{ ios };
}