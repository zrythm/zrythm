// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/visitor.hpp>

#include <catch.hpp>

using type_safe::nullopt_t;
using type_safe::nullvar_t;
using type_safe::optional;
using type_safe::variant;

TEST_CASE("visit optional")
{
    struct visitor
    {
        using incomplete_visitor = void;

        int value;

        void operator()(nullopt_t) const
        {
            REQUIRE(value == -1);
        }

        void operator()(int i) const
        {
            REQUIRE(value == i);
        }

        void operator()(int, nullopt_t) const
        {
            REQUIRE(value == -1);
        }

        void operator()(int, int b) const
        {
            REQUIRE(value == b);
        }
    };

    optional<int> a;
    visit(visitor{-1}, a);

    a = 42;
    visit(visitor{42}, a);

    optional<int> b;
    visit(visitor{-1}, a, b);

    b = 32;
    visit(visitor{32}, a, b);
}

TEST_CASE("visit variant")
{
    SECTION("optional variant")
    {
        struct visitor
        {
            int value;

            void operator()(nullvar_t) const
            {
                REQUIRE(value == -1);
            }

            void operator()(int i) const
            {
                REQUIRE(value == i);
            }

            void operator()(float f) const
            {
                REQUIRE(f == 3.14f);
            }

            void operator()(nullvar_t, nullvar_t) const
            {
                REQUIRE(false);
            }
            void operator()(nullvar_t, int) const
            {
                REQUIRE(false);
            }

            void operator()(int, nullvar_t) const
            {
                REQUIRE(value == -1);
            }
            void operator()(int, int b) const
            {
                REQUIRE(value == b);
            }

            void operator()(float, nullvar_t) const
            {
                REQUIRE(false);
            }
            void operator()(float a, int b) const
            {
                REQUIRE(value == b);
                REQUIRE(a == 3.14f);
            }
        };

        variant<nullvar_t, int, float> a;
        visit(visitor{-1}, a);

        a = 42;
        visit(visitor{42}, a);

        variant<nullvar_t, int> b;
        visit(visitor{-1}, a, b);

        b = 32;
        visit(visitor{32}, a, b);

        a = 3.14f;
        visit(visitor{-1}, a);
        visit(visitor{32}, a, b);
    }
    SECTION("returning reference")
    {
        struct visitor
        {
            int  value;
            int& operator()(int)
            {
                return value;
            }
        };

        visitor      v{1};
        variant<int> x{1};
        visit(v, x) = 2;

        REQUIRE(v.value == 2);
    }
    SECTION("rarely empty variant")
    {
        struct visitor
        {
            int value;

            void operator()(int a)
            {
                REQUIRE(a == value);
            }

            void operator()(float b)
            {
                REQUIRE(b == 3.14f);
                REQUIRE(value == -1);
            }

            void operator()(int, int)
            {
                REQUIRE(false);
            }

            void operator()(int, float)
            {
                REQUIRE(false);
            }

            void operator()(float a, int b)
            {
                REQUIRE(a == 3.14f);
                REQUIRE(b == value);
            }

            void operator()(float, float)
            {
                REQUIRE(false);
            }
        };

        variant<int, float> a(0);
        visit(visitor{0}, a);

        a = 3.14f;
        visit(visitor{-1}, a);

        variant<int, float> b(0);
        visit(visitor{0}, a, b);
    }
    SECTION("return type")
    {
        variant<int> a(1);
        auto         b = a;

        REQUIRE(visit([](int) { return 0; }, a) == 0);
        REQUIRE(visit([](int, int) { return 0; }, a, b) == 0);
    }
}
