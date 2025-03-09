// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/flag.hpp>

#include <catch.hpp>

using namespace type_safe;

TEST_CASE("flag")
{
    SECTION("constructor")
    {
        flag a(true);
        REQUIRE(a == true);

        flag b(false);
        REQUIRE(b == false);
    }
    SECTION("toggle")
    {
        flag a(true);
        REQUIRE(a.toggle());
        REQUIRE(a == false);

        flag b(false);
        REQUIRE(!b.toggle());
        REQUIRE(b == true);
    }
    SECTION("change")
    {
        flag a(true);
        a.change(false);
        REQUIRE(a == false);

        flag b(false);
        b.change(true);
        REQUIRE(b == true);
    }
    SECTION("set")
    {
        flag a(true);
        a.set();
        REQUIRE(a == true);

        flag b(false);
        b.set();
        REQUIRE(b == true);
    }
    SECTION("try_set")
    {
        flag a(true);
        REQUIRE(!a.try_set());
        REQUIRE(a == true);

        flag b(false);
        REQUIRE(b.try_set());
        REQUIRE(b == true);
    }
    SECTION("reset")
    {
        flag a(true);
        a.reset();
        REQUIRE(a == false);

        flag b(false);
        b.reset();
        REQUIRE(b == false);
    }
    SECTION("try_reset")
    {
        flag a(true);
        REQUIRE(a.try_reset());
        REQUIRE(a == false);

        flag b(false);
        REQUIRE(!b.try_reset());
        REQUIRE(b == false);
    }
}
