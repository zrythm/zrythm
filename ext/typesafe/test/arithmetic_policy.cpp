// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/arithmetic_policy.hpp>

#include <catch.hpp>

using namespace type_safe;

TEST_CASE("over/underflow")
{
    SECTION("unsigned")
    {
        auto max = std::numeric_limits<unsigned>::max();

        REQUIRE(detail::will_addition_error(detail::unsigned_integer_tag{}, max, 1u));
        REQUIRE(detail::will_addition_error(detail::unsigned_integer_tag{}, max - 5u, 10u));
        REQUIRE(detail::will_addition_error(detail::unsigned_integer_tag{}, max - 1u, max));
        REQUIRE(!detail::will_addition_error(detail::unsigned_integer_tag{}, 0u, max));
        REQUIRE(!detail::will_addition_error(detail::unsigned_integer_tag{}, 1u, max - 1u));

        REQUIRE(detail::will_subtraction_error(detail::unsigned_integer_tag{}, 0u, 1u));
        REQUIRE(detail::will_subtraction_error(detail::unsigned_integer_tag{}, 5u, 6u));
        REQUIRE(detail::will_subtraction_error(detail::unsigned_integer_tag{}, 0u, max));
        REQUIRE(!detail::will_subtraction_error(detail::unsigned_integer_tag{}, 5u, 5u));

        REQUIRE(detail::will_multiplication_error(detail::unsigned_integer_tag{}, max, max));
        REQUIRE(detail::will_multiplication_error(detail::unsigned_integer_tag{}, max, 2u));
        REQUIRE(detail::will_multiplication_error(detail::unsigned_integer_tag{}, max / 2u, 3u));
        REQUIRE(!detail::will_multiplication_error(detail::unsigned_integer_tag{}, max / 3u, 3u));

        REQUIRE(detail::will_division_error(detail::unsigned_integer_tag{}, 1u, 0u));
        REQUIRE(!detail::will_division_error(detail::unsigned_integer_tag{}, 1u, 1u));

        REQUIRE(detail::will_modulo_error(detail::unsigned_integer_tag{}, 1u, 0u));
        REQUIRE(!detail::will_modulo_error(detail::unsigned_integer_tag{}, 1u, 1u));
    }
    SECTION("signed")
    {
        auto max = std::numeric_limits<int>::max();
        auto min = std::numeric_limits<int>::min();

        REQUIRE(detail::will_addition_error(detail::signed_integer_tag{}, max, 1));
        REQUIRE(detail::will_addition_error(detail::signed_integer_tag{}, max - 5, 10));
        REQUIRE(detail::will_addition_error(detail::signed_integer_tag{}, max - 1, max));
        REQUIRE(!detail::will_addition_error(detail::signed_integer_tag{}, 0, max));
        REQUIRE(!detail::will_addition_error(detail::signed_integer_tag{}, 1, max - 1));
        REQUIRE(detail::will_addition_error(detail::signed_integer_tag{}, min, -1));
        REQUIRE(detail::will_addition_error(detail::signed_integer_tag{}, min + 5, -10));
        REQUIRE(!detail::will_addition_error(detail::signed_integer_tag{}, 0, min));
        REQUIRE(!detail::will_addition_error(detail::signed_integer_tag{}, -1, min + 1));

        REQUIRE(detail::will_subtraction_error(detail::signed_integer_tag{}, min, 1));
        REQUIRE(detail::will_subtraction_error(detail::signed_integer_tag{}, min + 5, 6));
        REQUIRE(detail::will_subtraction_error(detail::signed_integer_tag{}, min, max));
        REQUIRE(!detail::will_subtraction_error(detail::signed_integer_tag{}, 5, 5));
        REQUIRE(detail::will_subtraction_error(detail::signed_integer_tag{}, max, -1));
        REQUIRE(detail::will_subtraction_error(detail::signed_integer_tag{}, max - 5, -6));
        REQUIRE(!detail::will_subtraction_error(detail::signed_integer_tag{}, 5, -5));

        REQUIRE(detail::will_multiplication_error(detail::signed_integer_tag{}, max, max));
        REQUIRE(detail::will_multiplication_error(detail::signed_integer_tag{}, max, 2));
        REQUIRE(detail::will_multiplication_error(detail::signed_integer_tag{}, max / 2, 3));
        REQUIRE(!detail::will_multiplication_error(detail::signed_integer_tag{}, max / 3, 3));
        REQUIRE(detail::will_multiplication_error(detail::signed_integer_tag{}, max, min));
        REQUIRE(detail::will_multiplication_error(detail::signed_integer_tag{}, max, -2));
        REQUIRE(detail::will_multiplication_error(detail::signed_integer_tag{}, max / 2, -3));
        REQUIRE(!detail::will_multiplication_error(detail::signed_integer_tag{}, max / 3, -3));

        REQUIRE(detail::will_division_error(detail::signed_integer_tag{}, 1, 0));
        REQUIRE(detail::will_division_error(detail::signed_integer_tag{}, min, -1));
        REQUIRE(!detail::will_division_error(detail::signed_integer_tag{}, 1, 1));

        REQUIRE(detail::will_modulo_error(detail::signed_integer_tag{}, 1, 0));
        REQUIRE(!detail::will_modulo_error(detail::signed_integer_tag{}, 1, 1));
    }
}
