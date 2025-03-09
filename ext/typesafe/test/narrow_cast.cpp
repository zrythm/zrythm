// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/narrow_cast.hpp>

#include <catch.hpp>

using namespace type_safe;

TEST_CASE("narrow_cast<integer>")
{
    integer<int> a(4);

    integer<short> b = narrow_cast<short>(a);
    REQUIRE(static_cast<short>(b) == 4);

    integer<short> c = narrow_cast<integer<short>>(a);
    REQUIRE(static_cast<short>(c) == 4);

    integer<short> d = narrow_cast<integer<short>>(42);
    REQUIRE(static_cast<short>(d) == 42);
}

TEST_CASE("narrow_cast<floating_point>")
{
    floating_point<double> a(1.);

    floating_point<float> b = narrow_cast<float>(a);
    REQUIRE(static_cast<float>(b) == 1.);

    floating_point<float> c = narrow_cast<floating_point<float>>(a);
    REQUIRE(static_cast<float>(c) == 1.);
}
