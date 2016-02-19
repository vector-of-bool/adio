#include <adio/value.hpp>

#include <catch/catch.hpp>

using adio::value;

TEST_CASE("Null values")
{
    value v;
    auto b = v == adio::null;
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

struct MyString
{
    std::string str;
};

struct NullableString
{
    MyString data;
    bool null;
};

namespace adio
{

template<> struct value_adaptor<MyString>
{
    using base_type = std::string;
    enum { nullable = false };
    static MyString convert(const base_type& t)
    {
        return MyString{t};
    }

    static base_type convert(const MyString& t)
    {
        return t.str;
    }
};

template<> struct value_adaptor<NullableString>
{
    enum { nullable = true };
    using base_type = MyString;
    static NullableString null() { return NullableString{"", true}; }
    static bool is_null(const NullableString& ns) { return ns.null; }
    static NullableString convert(const base_type& t) { return NullableString{t, false}; }
    static base_type convert(const NullableString& ns) { return ns.data; }
};

} /* adio */


TEST_CASE("Custom types")
{
    value v{ "Cats" };
    CHECK(v.get<MyString>().str == "Cats");
    value other{ MyString{ "Dogs" } };
    CHECK(other == "Dogs");
    CHECK(other != 12);
}

TEST_CASE("Custom nullable types")
{
    {
        value v{"Dogs"};
        CHECK_FALSE(v.get<NullableString>().null);
        CHECK(v.get<NullableString>().data.str == "Dogs");
    }

    {
        value v;
        CHECK(v.get<NullableString>().null);
    }

    {
        NullableString str{ "Cats", false };
        CHECK(value{str} == "Cats");
    }

    {
        NullableString nullstr{ "", true };
        CHECK(value{nullstr} == adio::null);
    }
}
