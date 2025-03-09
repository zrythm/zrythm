// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/boolean.hpp>

#include <catch.hpp>

#include <sstream>

using namespace type_safe;

#ifndef TYPE_SAFE_TEST_NO_STATIC_ASSERT
static_assert(std::is_standard_layout<boolean>::value, "");
static_assert(std::is_trivially_copyable<boolean>::value, "");
// conversion checks
static_assert(std::is_constructible<boolean, bool>::value, "");
static_assert(!std::is_constructible<boolean, int>::value, "");
static_assert(std::is_assignable<boolean, bool>::value, "");
static_assert(!std::is_assignable<boolean, int>::value, "");
#endif

TEST_CASE("boolean")
{
    SECTION("constructor")
    {
        boolean b1(true);
        REQUIRE(static_cast<bool>(b1));

        boolean b2(false);
        REQUIRE(!static_cast<bool>(b2));
    }
    SECTION("assignment")
    {
        boolean b1(true);
        b1 = false;
        REQUIRE(!static_cast<bool>(b1));
        b1 = true;
        REQUIRE(static_cast<bool>(b1));
    }
    SECTION("negate")
    {
        boolean b1(true);
        REQUIRE(!b1 == false);

        boolean b2(false);
        REQUIRE(!b2 == true);
    }
    SECTION("comparison")
    {
        boolean b1(true);
        REQUIRE(b1 == true);
        REQUIRE(true == b1);
        REQUIRE(b1 != false);
        REQUIRE(false != b1);
        REQUIRE(b1 == boolean(true));
        REQUIRE(b1 != boolean(false));

        boolean b2(false);
        REQUIRE(b2 == false);
        REQUIRE(false == b2);
        REQUIRE(b2 != true);
        REQUIRE(true != b2);
        REQUIRE(b2 == boolean(false));
        REQUIRE(b2 != boolean(true));
    }
    SECTION("i/o")
    {
        std::ostringstream out;
        std::istringstream in("0");

        boolean b(true);
        out << b;
        REQUIRE(out.str() == "1");

        in >> b;
        REQUIRE(!static_cast<bool>(b));
    }
}
