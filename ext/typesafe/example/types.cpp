// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>

#include <type_safe/narrow_cast.hpp> // narrow_cast<>
#include <type_safe/types.hpp>       // boolean<>, integer<>, floating_point<>, aliases

namespace ts = type_safe;

// ts::unsigned_t is alias for ts::integer<unsigned>
// function is type safe - only unsigned integer types smaller than unsigned can be passed to the
// function
void only_unsigned(ts::unsigned_t val)
{
    std::cout << "only_unsigned() got: " << val << '\n';
}

int main()
{
    using namespace ts::literals; // for the literal operators

    //=== type safe boolean ===//
    // ts::bool_t is alias for ts::boolean
    ts::bool_t b1(true);
    //    ts::bool_t b2;    // error: no default constructor
    //    ts::bool_t b3(0); // error: 0 is not a bool
    std::cout << "boolean value: " << b1 << '\n';

    ts::bool_t b4 = !b1;
    if (b4)
        std::cout << "b4 is true\n";
    else
        std::cout << "b4 is false\n";

    //    b4 + 0; // error: arithmetic not possible
    //    ++b4;   // error: arithmetic not possible

    //=== type safe integer ===//
    // ts::int_t is alias for ts::integer<int>
    // ts::unsigned_t is alias for ts::integer<unsigned>
    ts::int_t i1(0);
    //    ts::int_t i2;        // error: no default constructor
    //    ts::int_t i3(100ll); // error: long long -> int would be narrowing

    ts::unsigned_t u1(0u);
    //    ts::unsigned_t u2;    // error: no default constructor
    //    ts::unsigned_t u3(0); // error: 0 is signed

    std::cout << "integer value: " << i1 << ' ' << u1 << '\n';

    i1 += 4;
    ts::integer<long long> i4 = i1 + 100ll; // 100ll is long long, so result is long long
    std::cout << i4 << '\n';

    //    --u1; // runtime error: unsigned underflow
    //    -u1;  // error: no unary minus for unsigned

    ts::uint8_t u4(255_u8); // instead of std::uint8_t(255)
    //    u4 += 2;  // error: 2 is signed
    //    u4 += 2_u8; // runtime error: unsigned overflow

    //    only_unsigned(-4); // error: -4 is not unsigned
    //    only_unsigned(10); // error: 10 is not unsigned
    //    only_unsigned(i1); // error: i1 is not unsigned
    only_unsigned(u4);

    ts::unsigned_t u5 = ts::make_unsigned(i1);
    //    ts::unsigned_t u6 = ts::make_unsigned(-i1); // runtime error: value does not fit
    ts::unsigned_t u7 = ts::abs(-i1); // but this works and returns an the unsigned integer
    std::cout << u5 << ' ' << u7 << '\n';

    //    ts::int8_t i5 = ts::make_signed(u4); // runtime error: value does not fit
    ts::int8_t i6 = ts::make_signed(u4 - 200_u8);
    std::cout << i6 << '\n';

    //=== type safe floating points ===//
    // ts::float_t is alias for ts::floating_point<std::float_t> (std::float_t is float)
    // ts::double_t is alias for ts::floating_point<std::double_t> (std::double_t is double)
    ts::float_t f1(0.f);
    //    ts::float_t f2;     // error: no default constructor
    //    ts::float_t f3(0.); // error: 0. is double literal

    ts::double_t d1(0.);
    //    ts::double_t d2;      // error: no default constructor
    ts::double_t d3(0.f); // okay: float -> double is allowed

    std::cout << "floating point value: " << f1 << ' ' << d1 << ' ' << d3 << '\n';

    //    f1++;      // error: no in/decrement for floats
    //    f1 == 0.f; // error: no equality comparison for floats

    //=== narrow cast ===//
    ts::uint32_t uint32_val(325_u32);
    //    ts::uint8_t  uint8_val1 =
    //        ts::narrow_cast<ts::uint8_t>(uint32_val); // runtime error: value does not fit
    uint32_val             = 100_u32;
    ts::uint8_t uint8_val2 = ts::narrow_cast<ts::uint8_t>(uint32_val); // works
    std::cout << uint32_val << ' ' << uint8_val2 << '\n';

    ts::double_t double_val(0.1);
    //    ts::float_t  float_val1 =
    //        ts::narrow_cast<ts::float_t>(double_val); // runtime error: 0.1 is not exact
    double_val             = 0.5;
    ts::float_t float_val2 = ts::narrow_cast<ts::float_t>(double_val); // works, 0.5 is exact
    std::cout << double_val << ' ' << float_val2 << '\n';
}
