// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/downcast.hpp>

#include <catch.hpp>

using namespace type_safe;

TEST_CASE("downcast")
{
    struct base
    {
        virtual ~base() = default;
    };

    struct derived : base
    {
        ~derived() override = default;
    };

    base    b;
    derived d;

    SECTION("base -> base")
    {
        base& ref = b;

        base& res1 = downcast<base&>(ref);
        REQUIRE(&res1 == &ref);

        base& res2 = downcast(derived_type<base>{}, ref);
        REQUIRE(&res2 == &ref);
    }
    SECTION("const base -> const base")
    {
        const base& ref = b;

        const base& res1 = downcast<const base&>(ref);
        REQUIRE(&res1 == &ref);

        const base& res2 = downcast(derived_type<base>{}, ref);
        REQUIRE(&res2 == &ref);
    }
    SECTION("base -> derived")
    {
        base& ref = d;

        derived& res1 = downcast<derived&>(ref);
        REQUIRE(&res1 == &ref);

        derived& res2 = downcast(derived_type<derived>{}, ref);
        REQUIRE(&res2 == &ref);
    }
    SECTION("const base -> const derived")
    {
        const base& ref = d;

        const derived& res1 = downcast<const derived&>(ref);
        REQUIRE(&res1 == &ref);

        const derived& res2 = downcast(derived_type<derived>{}, ref);
        REQUIRE(&res2 == &ref);
    }
}
