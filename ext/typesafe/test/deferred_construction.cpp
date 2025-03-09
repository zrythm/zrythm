// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/deferred_construction.hpp>

#include <catch.hpp>
#include <string>

using namespace type_safe;

TEST_CASE("deferred_construction")
{
    SECTION("constructor - empty")
    {
        deferred_construction<int> a;
        REQUIRE(!a.has_value());
    }
    SECTION("constructor - copy/move")
    {
        deferred_construction<int> a;
        a = 0;

        deferred_construction<int> b(a);
        REQUIRE(b.has_value());
        REQUIRE(b.value() == 0);

        deferred_construction<int> c(std::move(a));
        REQUIRE(c.has_value());
        REQUIRE(c.value() == 0);

        deferred_construction<int> d;

        deferred_construction<int> e(d);
        REQUIRE(!e.has_value());

        deferred_construction<int> f(std::move(d));
        REQUIRE(!d.has_value());
    }
    SECTION("assignment")
    {
        deferred_construction<int> a;
        REQUIRE(!a.has_value());

        a = 42;
        REQUIRE(a.has_value());
        REQUIRE(a.value() == 42);
    }
    SECTION("emplace")
    {
        deferred_construction<std::string> a;
        REQUIRE(!a.has_value());

        a.emplace(3u, 'c');
        REQUIRE(a.has_value());
        REQUIRE(a.value() == "ccc");
    }
    SECTION("operator bool")
    {
        deferred_construction<int> a;
        REQUIRE(!a);
        a = 42;
        REQUIRE(a);
    }
}
