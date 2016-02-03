#include <adio/value.hpp>

#include <catch/catch.hpp>

using adio::value;

TEST_CASE("Null values")
{
    value v;
    auto b = v == nullptr;
    CHECK(b);
    auto b2 = v.get_type() == adio::type::null_t;
    CHECK(b2);
    CHECK(v == v);
    CHECK_FALSE(v != v);
    CHECK_FALSE(v < v);
    CHECK(v <= v);
    CHECK(v >= v);
    CHECK_FALSE(v > v);
}

TEST_CASE("Integer values")
{
    value v{value::integer{12}};
    CHECK(v == 12);
    CHECK(v < 14);
    CHECK(v != 13);
    CHECK(v > 9);
    CHECK(v != "Dogs");
}

TEST_CASE("String values")
{
    value v = "Hello!";
    CHECK(v == "Hello!");
    CHECK(v != "World");
    CHECK(v < "World");
    CHECK(v > "Apple");
}

TEST_CASE("Conversion")
{
    value v = 128;
    CHECK_THROWS_AS(v.get<value::text>(), adio::invalid_access);
    CHECK(v.get<value::integer>() == 128);
    CHECK(v.get<short>() == 128);
    CHECK(value::integer(v) == 128);
    // Check bool conversion
    CHECK(v);
}

TEST_CASE("String conversion")
{
    value v = "Hello, world!";
    CHECK_THROWS_AS(v.get<int>(), adio::invalid_access);
    char arr[] = "Hello, world!";
    CHECK(v.get<std::basic_string<unsigned char>>()
          == std::basic_string<unsigned char>(std::begin(arr), std::end(arr)-1));
}
