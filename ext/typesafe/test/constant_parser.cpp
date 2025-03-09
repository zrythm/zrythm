// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/detail/constant_parser.hpp>

#include <catch.hpp>

using namespace type_safe;

TEST_CASE("detail::parse")
{
    REQUIRE((detail::parse<int, '0'>() == 0));
    REQUIRE((detail::parse<int, '1', '0'>() == 10));
    REQUIRE((detail::parse<int, '4', '2', '3'>() == 423));
    REQUIRE((detail::parse<int, '2', '3', '\'', '9', '0', '0'>() == 23900));

    REQUIRE((detail::parse<int, '0', '1', '0'>() == 8));
    REQUIRE((detail::parse<int, '0', 'x', 'A'>() == 10));
    REQUIRE((detail::parse<int, '0', 'b', '1'>() == 1));
}
