// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/flag_set.hpp>

#include <catch.hpp>

// no using namespace to test operator namespace

enum class test_flags
{
    a,
    b,
    c
};

namespace type_safe
{
template <>
struct flag_set_traits<test_flags> : std::true_type
{
    static constexpr std::size_t size()
    {
        return 3;
    }
};
} // namespace type_safe

void check_set(const type_safe::flag_set<test_flags>& set, bool a, bool b, bool c)
{
    REQUIRE(set.is_set(test_flags::a) == a);
    REQUIRE(set.as_flag(test_flags::a) == a);
    if (a)
        REQUIRE((set & test_flags::a));

    REQUIRE(set.is_set(test_flags::b) == b);
    REQUIRE(set.as_flag(test_flags::b) == b);
    if (b)
        REQUIRE((set & test_flags::b));

    REQUIRE(set.is_set(test_flags::c) == c);
    REQUIRE(set.as_flag(test_flags::c) == c);
    if (c)
        REQUIRE((set & test_flags::c));

    if (a || b || c)
    {
        REQUIRE(set.any());
        REQUIRE_FALSE(set.none());
        REQUIRE(set != type_safe::noflag);
        REQUIRE(type_safe::noflag != set);
    }
    else
    {
        REQUIRE_FALSE(set.any());
        REQUIRE(set.none());
        REQUIRE(set == type_safe::noflag);
        REQUIRE(type_safe::noflag == set);
    }

    auto number = (a ? 4 : 0) + (b ? 2 : 0) + (c ? 1 : 0);
    switch (number)
    {
    case 0:
        REQUIRE(set == type_safe::flag_set<test_flags>());
        break;
    case 1:
        REQUIRE(set == test_flags::c);
        REQUIRE(set == type_safe::combo(~test_flags::a & ~test_flags::b));
        REQUIRE((set & test_flags::c));
        break;
    case 2:
        REQUIRE(set == test_flags::b);
        REQUIRE(set == type_safe::combo(~test_flags::a & ~test_flags::c));
        REQUIRE((set & test_flags::b));
        break;
    case 3:
        REQUIRE(set == (test_flags::b | test_flags::c));
        REQUIRE(set == type_safe::combo(~test_flags::a));
        REQUIRE((set & (test_flags::b | test_flags::c)));
        break;
    case 4:
        REQUIRE(set == test_flags::a);
        REQUIRE(set == type_safe::combo(~test_flags::b & ~test_flags::c));
        REQUIRE((set & test_flags::a));
        break;
    case 5:
        REQUIRE(set == (test_flags::a | test_flags::c));
        REQUIRE(set == type_safe::combo(~test_flags::b));
        REQUIRE((set & (test_flags::a | test_flags::c)));
        break;
    case 6:
        REQUIRE(set == (test_flags::a | test_flags::b));
        REQUIRE(set == type_safe::combo(~test_flags::c));
        REQUIRE((set & (test_flags::a | test_flags::b)));
        break;
    case 7:
        REQUIRE(set.all());
        REQUIRE(set == (test_flags::a | test_flags::b | test_flags::c));
        REQUIRE((set & (test_flags::a | test_flags::b | test_flags::c)));
        break;

    default:
        REQUIRE(false);
        break;
    }
}

TEST_CASE("flag_set_traits")
{
    using namespace type_safe;

    enum class a
    {
        e1,
        e2,
        e3
    };
    static_assert(!flag_set_traits<a>::value, "a is not a flag set enum");

    enum class b
    {
        e1,
        e2,
        e3,
        _flag_set_size,
    };
    static_assert(flag_set_traits<b>::value, "b is a flag set enum");
    static_assert(flag_set_traits<b>::size() == 3u, "size of b is 3");
}

TEST_CASE("flag_set")
{
    using namespace type_safe;

    using set = flag_set<test_flags>;

    set s;
    check_set(s, false, false, false);

    SECTION("constructor/assignment")
    {
        set a;
        check_set(a, false, false, false);

        set b(test_flags::a);
        check_set(b, true, false, false);

        a = test_flags::b;
        check_set(a, false, true, false);

        b = test_flags::c;
        check_set(b, false, false, true);
    }
    SECTION("from_int")
    {
        set a = set::from_int<std::uint8_t>(0b000);
        check_set(a, false, false, false);

        a = set::from_int<std::uint8_t>(0b001);
        check_set(a, true, false, false);

        a = set::from_int<std::uint8_t>(0b010);
        check_set(a, false, true, false);

        a = set::from_int<std::uint8_t>(0b100);
        check_set(a, false, false, true);

        a = set::from_int<std::uint8_t>(0b111);
        check_set(a, true, true, true);
    }
    SECTION("set")
    {
        s.set(test_flags::a);
        check_set(s, true, false, false);

        s.set(test_flags::b, true);
        check_set(s, true, true, false);

        s.set(test_flags::a, false);
        check_set(s, false, true, false);

        s.set(test_flags::a, flag(true));
        check_set(s, true, true, false);
    }
    SECTION("reset")
    {
        s.set(test_flags::a);
        check_set(s, true, false, false);

        s.reset(test_flags::b);
        check_set(s, true, false, false);

        s.reset(test_flags::a);
        check_set(s, false, false, false);
    }
    SECTION("toggle")
    {
        s.toggle(test_flags::a);
        check_set(s, true, false, false);

        s.toggle(test_flags::b);
        check_set(s, true, true, false);

        s.toggle(test_flags::a);
        check_set(s, false, true, false);
    }
    SECTION("set_all/reset_all/toggle_all")
    {
        s.set_all();
        check_set(s, true, true, true);

        s.reset_all();
        check_set(s, false, false, false);

        s.set(test_flags::c);
        check_set(s, false, false, true);

        s.toggle_all();
        check_set(s, true, true, false);
    }
    SECTION("binary op")
    {
        s |= test_flags::a;
        check_set(s, true, false, false);

        s |= test_flags::b | test_flags::c;
        check_set(s, true, true, true);

        s &= ~test_flags::c;
        check_set(s, true, true, false);

        s = ~s;
        check_set(s, false, false, true);

        s ^= test_flags::a | test_flags::c;
        check_set(s, true, false, false);
    }
}
