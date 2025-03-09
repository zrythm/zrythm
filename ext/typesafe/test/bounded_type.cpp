// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/bounded_type.hpp>

#include <catch.hpp>

#include <type_safe/integer.hpp>

using namespace type_safe;

TEST_CASE("constraints::less")
{
    constraints::less<int> p(42);
    REQUIRE(p.get_bound() == 42);
    REQUIRE(p(0));
    REQUIRE(p(40));
    REQUIRE(!p(42));
    REQUIRE(!p(50));
    REQUIRE(!p(100));
}

TEST_CASE("constraints::less_equal")
{
    constraints::less_equal<int> p(42);
    REQUIRE(p.get_bound() == 42);
    REQUIRE(p(0));
    REQUIRE(p(40));
    REQUIRE(p(42));
    REQUIRE(!p(50));
    REQUIRE(!p(100));
}

TEST_CASE("constraints::greater")
{
    constraints::greater<int> p(42);
    REQUIRE(p.get_bound() == 42);
    REQUIRE(!p(0));
    REQUIRE(!p(40));
    REQUIRE(!p(42));
    REQUIRE(p(50));
    REQUIRE(p(100));
}

TEST_CASE("constraints::greater_equal")
{
    constraints::greater_equal<int> p(42);
    REQUIRE(p.get_bound() == 42);
    REQUIRE(!p(0));
    REQUIRE(!p(40));
    REQUIRE(p(42));
    REQUIRE(p(50));
    REQUIRE(p(100));
}

TEST_CASE("constraints::bounded")
{
    SECTION("closed, closed")
    {
        constraints::bounded<int, true, true> p(0, 42);
        static_assert(std::is_same<decltype(p), constraints::closed_interval<int>>::value, "");
        REQUIRE(p.get_lower_bound() == 0);
        REQUIRE(p.get_upper_bound() == 42);

        REQUIRE(p(30));
        REQUIRE(p(41));
        REQUIRE(p(1));

        REQUIRE(p(0));
        REQUIRE(p(42));

        REQUIRE(!p(-5));
        REQUIRE(!p(100));
    }
    SECTION("open, closed")
    {
        constraints::bounded<int, false, true> p(0, 42);
        REQUIRE(p.get_lower_bound() == 0);
        REQUIRE(p.get_upper_bound() == 42);

        REQUIRE(p(30));
        REQUIRE(p(41));
        REQUIRE(p(1));

        REQUIRE(!p(0));
        REQUIRE(p(42));

        REQUIRE(!p(-5));
        REQUIRE(!p(100));
    }
    SECTION("closed, open")
    {
        constraints::bounded<int, true, false> p(0, 42);
        REQUIRE(p.get_lower_bound() == 0);
        REQUIRE(p.get_upper_bound() == 42);

        REQUIRE(p(30));
        REQUIRE(p(41));
        REQUIRE(p(1));

        REQUIRE(p(0));
        REQUIRE(!p(42));

        REQUIRE(!p(-5));
        REQUIRE(!p(100));
    }
    SECTION("open, open")
    {
        constraints::bounded<int, false, false> p(0, 42);
        static_assert(std::is_same<decltype(p), constraints::open_interval<int>>::value, "");
        REQUIRE(p.get_lower_bound() == 0);
        REQUIRE(p.get_upper_bound() == 42);

        REQUIRE(p(30));
        REQUIRE(p(41));
        REQUIRE(p(1));

        REQUIRE(!p(0));
        REQUIRE(!p(42));

        REQUIRE(!p(-5));
        REQUIRE(!p(100));
    }
}

TEST_CASE("bounded literal")
{
    SECTION("unsigned")
    {
        constraints::less<integer<unsigned>, lit_detail::integer_bound<unsigned long long, 42>> p(
            42_boundu);
        static_assert(std::is_same<decltype(p.get_bound()), const unsigned long long&>::value,
                      "ups");
        REQUIRE(p.get_bound() == 42);
    }
    SECTION("signed")
    {
        constraints::less<integer<int>, lit_detail::integer_bound<long long, 42>> p(42_bound);
        static_assert(std::is_same<decltype(p.get_bound()), const long long&>::value, "ups");
        REQUIRE(p.get_bound() == 42);
    }
    SECTION("signed negative")
    {
        constraints::less<integer<int>, lit_detail::integer_bound<long long, -42>> p(-42_bound);
        static_assert(std::is_same<decltype(p.get_bound()), const long long&>::value, "ups");
        REQUIRE(p.get_bound() == -42);
    }

    bounded_type<integer<int>, true, true, lit_detail::integer_bound<long long, 0>,
                 lit_detail::integer_bound<long long, 100>>
        bounded = make_bounded(integer<int>(50), 0_bound, 100_bound);
    REQUIRE(bounded.get_constraint().get_lower_bound() == 0);
    REQUIRE(bounded.get_constraint().get_upper_bound() == 100);
}

TEST_CASE("bounded_type")
{
    constrained_type<int, constraints::closed_interval<int>> dynamic_closed
        = make_bounded(10, 0, 42);
    static_assert(std::is_same<decltype(dynamic_closed), bounded_type<int, true, true>>::value, "");
    REQUIRE(dynamic_closed.get_value() == 10);
    REQUIRE(dynamic_closed.get_constraint().get_lower_bound() == 0);
    REQUIRE(dynamic_closed.get_constraint().get_upper_bound() == 42);

    constrained_type<int, constraints::open_interval<int>> dynamic_open
        = make_bounded_exclusive(10, 0, 42);
    static_assert(std::is_same<decltype(dynamic_open), bounded_type<int, false, false>>::value, "");
    REQUIRE(dynamic_open.get_value() == 10);
    REQUIRE(dynamic_open.get_constraint().get_lower_bound() == 0);
    REQUIRE(dynamic_open.get_constraint().get_upper_bound() == 42);

    constrained_type<int, constraints::closed_interval<int, std::integral_constant<int, 0>,
                                                       std::integral_constant<int, 42>>>
        static_closed
        = make_bounded(10, std::integral_constant<int, 0>{}, std::integral_constant<int, 42>{});
    static_assert(std::is_same<decltype(static_closed),
                               bounded_type<int, true, true, std::integral_constant<int, 0>,
                                            std::integral_constant<int, 42>>>::value,
                  "");
    REQUIRE(static_closed.get_value() == 10);
    REQUIRE(static_closed.get_constraint().get_lower_bound() == 0);
    REQUIRE(static_closed.get_constraint().get_upper_bound() == 42);

    decltype(static_closed) static_closed_default(10);
    REQUIRE(static_closed_default.get_value() == 10);
    REQUIRE(static_closed_default.get_constraint().get_lower_bound() == 0);
    REQUIRE(static_closed_default.get_constraint().get_upper_bound() == 42);

    constrained_type<int, constraints::open_interval<int, std::integral_constant<int, 0>,
                                                     std::integral_constant<int, 42>>>
        static_open = make_bounded_exclusive(10, std::integral_constant<int, 0>{},
                                             std::integral_constant<int, 42>{});
    static_assert(std::is_same<decltype(static_open),
                               bounded_type<int, false, false, std::integral_constant<int, 0>,
                                            std::integral_constant<int, 42>>>::value,
                  "");
    REQUIRE(static_open.get_value() == 10);
    REQUIRE(static_open.get_constraint().get_lower_bound() == 0);
    REQUIRE(static_open.get_constraint().get_upper_bound() == 42);

    decltype(static_open) static_open_default(10);
    REQUIRE(static_open_default.get_value() == 10);
    REQUIRE(static_open_default.get_constraint().get_lower_bound() == 0);
    REQUIRE(static_open_default.get_constraint().get_upper_bound() == 42);

    constrained_type<int, constraints::closed_interval<int, constraints::dynamic_bound,
                                                       std::integral_constant<int, 42>>>
        mixed_closed = make_bounded(10, 0, std::integral_constant<int, 42>{});
    static_assert(std::is_same<decltype(mixed_closed),
                               bounded_type<int, true, true, constraints::dynamic_bound,
                                            std::integral_constant<int, 42>>>::value,
                  "");
    REQUIRE(mixed_closed.get_value() == 10);
    REQUIRE(mixed_closed.get_constraint().get_lower_bound() == 0);
    REQUIRE(mixed_closed.get_constraint().get_upper_bound() == 42);

    constrained_type<int, constraints::open_interval<int, constraints::dynamic_bound,
                                                     std::integral_constant<int, 42>>>
        mixed_open = make_bounded_exclusive(10, 0, std::integral_constant<int, 42>{});
    static_assert(std::is_same<decltype(mixed_open),
                               bounded_type<int, false, false, constraints::dynamic_bound,
                                            std::integral_constant<int, 42>>>::value,
                  "");
    REQUIRE(mixed_open.get_value() == 10);
    REQUIRE(mixed_open.get_constraint().get_lower_bound() == 0);
    REQUIRE(mixed_open.get_constraint().get_upper_bound() == 42);
}

TEST_CASE("clamping_verifier")
{
    SECTION("less_equal")
    {
        constraints::less_equal<int> p(42);

        int a = 0;
        a     = clamping_verifier::verify(a, p);
        REQUIRE(a == 0);

        int b = 30;
        b     = clamping_verifier::verify(b, p);
        REQUIRE(b == 30);

        int c = 42;
        c     = clamping_verifier::verify(c, p);
        REQUIRE(c == 42);

        int d = 50;
        d     = clamping_verifier::verify(d, p);
        REQUIRE(d == 42);
    }
    SECTION("greater_equal")
    {
        constraints::greater_equal<int> p(42);

        int a = 0;
        a     = clamping_verifier::verify(a, p);
        REQUIRE(a == 42);

        int b = 30;
        b     = clamping_verifier::verify(b, p);
        REQUIRE(b == 42);

        int c = 42;
        c     = clamping_verifier::verify(c, p);
        REQUIRE(c == 42);

        int d = 50;
        d     = clamping_verifier::verify(d, p);
        REQUIRE(d == 50);
    }
    SECTION("closed_interval")
    {
        constraints::closed_interval<int> p(0, 42);

        int a = 30;
        a     = clamping_verifier::verify(a, p);
        REQUIRE(a == 30);

        int b = 10;
        b     = clamping_verifier::verify(b, p);
        REQUIRE(b == 10);

        int c = 0;
        c     = clamping_verifier::verify(c, p);
        REQUIRE(c == 0);

        int d = 42;
        d     = clamping_verifier::verify(d, p);
        REQUIRE(d == 42);

        int e = 50;
        e     = clamping_verifier::verify(e, p);
        REQUIRE(e == 42);

        int f = -20;
        f     = clamping_verifier::verify(f, p);
        REQUIRE(f == 0);
    }
}

TEST_CASE("clamped_type")
{
    int value;
    SECTION("no_clamping")
    {
        value = 10;
    }
    SECTION("clamping_lower")
    {
        value = -42;
    }
    SECTION("clamping_upper")
    {
        value = 100;
    }

    clamped_type<int> dynamic = make_clamped(value, 0, 42);

    auto clamped_val = value;
    clamped_val      = clamp(dynamic.get_constraint(), clamped_val);
    REQUIRE(dynamic.get_value() == clamped_val);

    REQUIRE(dynamic.get_constraint().get_lower_bound() == 0);
    REQUIRE(dynamic.get_constraint().get_upper_bound() == 42);

    clamped_type<int, std::integral_constant<int, 0>, std::integral_constant<int, 42>> static_
        = make_clamped(value, std::integral_constant<int, 0>{}, std::integral_constant<int, 42>{});

    clamped_val = value;
    clamped_val = clamp(static_.get_constraint(), clamped_val);
    REQUIRE(static_.get_value() == clamped_val);

    REQUIRE(static_.get_constraint().get_lower_bound() == 0);
    REQUIRE(static_.get_constraint().get_upper_bound() == 42);

    clamped_type<int, std::integral_constant<int, 0>> mixed
        = make_clamped(value, std::integral_constant<int, 0>{}, 42);

    clamped_val = value;
    clamped_val = clamp(mixed.get_constraint(), clamped_val);
    REQUIRE(mixed.get_value() == clamped_val);

    REQUIRE(mixed.get_constraint().get_lower_bound() == 0);
    REQUIRE(mixed.get_constraint().get_upper_bound() == 42);
}
