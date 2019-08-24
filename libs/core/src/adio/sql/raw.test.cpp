#include "./raw.hpp"

#include <catch2/catch.hpp>

using namespace std::literals;

using namespace adio::literals;

TEST_CASE("sql_string basics") {
    // If given a string literal, should produce a sql_string of a std::string,
    // not a character array.
    adio::sql_string<std::string>      s  = adio::sql_string("This is a string literal");
    adio::sql_string<std::string_view> s2 = adio::sql_string("A string view"sv);
    adio::sql_string<std::string_view> s3 = "I am a SQL literal"_sql;
}
