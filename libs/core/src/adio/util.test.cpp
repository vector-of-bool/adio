#include <adio/util.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Throw for errors") {
    adio::error_code ec;
    CHECK_NOTHROW(adio::throw_if_error(ec, "test"));
    ec = make_error_code(adio::errc::address_in_use);
    CHECK_THROWS_AS(adio::throw_if_error(ec, "test"), adio::system_error);
}

TEST_CASE("Regularize") {
    // This lambda returns `void`
    auto void_fn = [] {};
    // This won't compile, as `retval` would be `void`:
    /// auto retval = void_fn();
    // We wrap it with a detail type to represent the 'void'-ness
    auto retval = adio::regularize_invoke(void_fn);
    // `unregularize` will convert the retval back to `void`, and should be
    // used directly as a return statementn expression:
    auto revoid_fn = [&] { return adio::unregularize(retval); };
    static_assert(std::is_void_v<decltype(revoid_fn())>);
    // Won't compile, again:
    /// auto retval2 = revoid_fn();

    int            i = 0;
    decltype(auto) v = ADIO_FWD_RET(i);
    static_assert(std::is_same_v<decltype(v), int>);

    int&           i2 = i;
    decltype(auto) v2 = ADIO_FWD_RET(i2);
    static_assert(std::is_same_v<decltype(v2), int&>);

    int&&          i3 = std::move(i);
    decltype(auto) v3 = ADIO_FWD_RET(i3);
    static_assert(std::is_same_v<decltype(v3), int&&>);

    // auto t = ADIO_FWD_RET(retval);
}
