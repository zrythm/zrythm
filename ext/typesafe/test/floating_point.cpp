// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/floating_point.hpp>

#include <catch.hpp>

#include <sstream>

using namespace type_safe;

#ifndef TYPE_SAFE_TEST_NO_STATIC_ASSERT
static_assert(std::is_trivially_copyable<floating_point<float>>::value, "");
static_assert(std::is_standard_layout<floating_point<float>>::value, "");
// conversion checks
static_assert(std::is_constructible<floating_point<float>, float>::value, "");
static_assert(!std::is_constructible<floating_point<float>, double>::value, "");
static_assert(std::is_constructible<floating_point<double>, double>::value, "");
static_assert(std::is_constructible<floating_point<double>, double>::value, "");
static_assert(!std::is_constructible<floating_point<double>, long double>::value, "");
static_assert(std::is_assignable<floating_point<float>, float>::value, "");
static_assert(!std::is_assignable<floating_point<float>, double>::value, "");
static_assert(std::is_assignable<floating_point<double>, double>::value, "");
static_assert(std::is_assignable<floating_point<double>, double>::value, "");
static_assert(!std::is_assignable<floating_point<double>, long double>::value, "");
#endif

TEST_CASE("floating_point")
{
    using float_t = floating_point<double>;

    SECTION("constructor")
    {
        float_t a(0.);
        REQUIRE(static_cast<double>(a) == 0.);
        float_t b(3.14);
        REQUIRE(static_cast<double>(b) == 3.14);
        float_t c(-42.5);
        REQUIRE(static_cast<double>(c) == -42.5);
    }
    SECTION("assignment")
    {
        float_t a(0.);
        a = 3.14;
        REQUIRE(static_cast<double>(a) == 3.14);
        a = -42.5;
        REQUIRE(static_cast<double>(a) == -42.5);
    }
    SECTION("unary")
    {
        float_t a(13.255);
        REQUIRE(static_cast<double>(+a) == static_cast<double>(a));
        REQUIRE(static_cast<double>(-a) == -static_cast<double>(a));
    }
    SECTION("addition")
    {
        float_t wrapper(0.);
        double  normal(0.);
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper += 4.5;
        normal += 4.5;
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = wrapper + (-2.3);
        normal  = normal + (-2.3);
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = 1.1 + wrapper;
        normal  = 1.1 + normal;
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = float_t(-11.0) + wrapper;
        normal  = -11.0 + normal;
        REQUIRE(static_cast<double>(wrapper) == normal);
    }
    SECTION("subtraction")
    {
        float_t wrapper(0.);
        double  normal(0.);
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper -= 4.5;
        normal -= 4.5;
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = wrapper - (-2.3);
        normal  = normal - (-2.3);
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = 1.1 - wrapper;
        normal  = 1.1 - normal;
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = float_t(-11.0) - wrapper;
        normal  = -11.0 - normal;
        REQUIRE(static_cast<double>(wrapper) == normal);
    }
    SECTION("multiplication")
    {
        float_t wrapper(1.);
        double  normal(1.);
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper *= 4.5;
        normal *= 4.5;
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = wrapper * (-2.3);
        normal  = normal * (-2.3);
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = 1.1 * wrapper;
        normal  = 1.1 * normal;
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = float_t(-11.0) * wrapper;
        normal  = -11.0 * normal;
        REQUIRE(static_cast<double>(wrapper) == normal);
    }
    SECTION("division")
    {
        float_t wrapper(10.);
        double  normal(10.);
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper /= 4.5;
        normal /= 4.5;
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = wrapper / (-2.3);
        normal  = normal / (-2.3);
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = 1.1 / wrapper;
        normal  = 1.1 / normal;
        REQUIRE(static_cast<double>(wrapper) == normal);

        wrapper = float_t(-11.0) / wrapper;
        normal  = -11.0 / normal;
        REQUIRE(static_cast<double>(wrapper) == normal);
    }
    SECTION("comparison")
    {
        // <
        REQUIRE(bool(float_t(4.) < float_t(5.)));
        REQUIRE(!(float_t(5.) < float_t(4.)));
        REQUIRE(!(float_t(4.) < float_t(4.)));

        REQUIRE(bool(4. < float_t(5.)));
        REQUIRE(!(5. < float_t(4.)));
        REQUIRE(!(4. < float_t(4.)));

        REQUIRE(bool(float_t(4.) < 5.));
        REQUIRE(!(float_t(5.) < 4.));
        REQUIRE(!(float_t(4.) < 4.));

        // <=
        REQUIRE(bool(float_t(4.) <= float_t(5.)));
        REQUIRE(!(float_t(5.) <= float_t(4.)));
        REQUIRE(bool(float_t(4.) <= float_t(4.)));

        REQUIRE(bool(4. <= float_t(5.)));
        REQUIRE(!(5. <= float_t(4.)));
        REQUIRE(bool(4. <= float_t(4.)));

        REQUIRE(bool(float_t(4.) <= 5.));
        REQUIRE(!(float_t(5.) <= 4.));
        REQUIRE(bool(float_t(4.) <= 4.));

        // >
        REQUIRE(bool(float_t(5.) > float_t(4.)));
        REQUIRE(!(float_t(4.) > float_t(5.)));
        REQUIRE(!(float_t(5.) > float_t(5.)));

        REQUIRE(bool(5. > float_t(4.)));
        REQUIRE(!(4. > float_t(5.)));
        REQUIRE(!(5. > float_t(5.)));

        REQUIRE(bool(float_t(5.) > 4.));
        REQUIRE(!(float_t(4.) > 5.));
        REQUIRE(!(float_t(5.) > 5.));

        // >=
        REQUIRE(bool(float_t(5.) >= float_t(4.)));
        REQUIRE(!(float_t(4.) >= float_t(5.)));
        REQUIRE(bool(float_t(5.) >= float_t(5.)));

        REQUIRE(bool(5. >= float_t(4.)));
        REQUIRE(!(4. >= float_t(5.)));
        REQUIRE(bool(5. >= float_t(5.)));

        REQUIRE(bool(float_t(5.) >= 4.));
        REQUIRE(!(float_t(4.) >= 5.));
        REQUIRE(bool(float_t(5.) >= 5.));
    }
    SECTION("i/o")
    {
        std::ostringstream out;
        std::istringstream in("1.0");

        float_t f(0.0);
        out << f;
        REQUIRE(out.str() == "0");

        in >> f;
        REQUIRE(static_cast<double>(f) == 1.0);
    }
}
