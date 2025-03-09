// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/optional_ref.hpp>

#include <catch.hpp>

#include "debugger_type.hpp"

using type_safe::optional;
using type_safe::object_ref;
using type_safe::optional_ref;
using type_safe::optional_xvalue_ref;
using type_safe::opt_ref;
using type_safe::opt_cref;
using type_safe::opt_xref;
using type_safe::ref;

template <typename A, typename B, typename Value>
void test_optional_ref_conversion(Value)
{
    static_assert(std::is_constructible<optional_ref<A>, B>::value == Value::value, "");
    static_assert(std::is_assignable<optional_ref<A>, B>::value == Value::value, "");
}

TEST_CASE("optional_ref")
{
    // only test stuff special for optional_ref
    struct base
    {};
    struct derived : base
    {};

    test_optional_ref_conversion<int, int&&>(std::false_type{});
    test_optional_ref_conversion<int, const int&>(std::false_type{});
    test_optional_ref_conversion<int, optional_ref<const int>>(std::false_type{});
    test_optional_ref_conversion<const int, object_ref<int>>(std::true_type{});
    test_optional_ref_conversion<const int, optional_ref<int>>(std::true_type{});
    test_optional_ref_conversion<base, object_ref<derived>>(std::true_type{});
    test_optional_ref_conversion<base, optional_ref<derived>>(std::true_type{});

    int value = 0;
    SECTION("constructor")
    {
        optional_ref<int> a(nullptr);
        REQUIRE_FALSE(a.has_value());

        optional_ref<int> b(value);
        REQUIRE(b.has_value());
        REQUIRE(&b.value() == &value);

        optional_ref<int> c(ref(value));
        REQUIRE(c.has_value());
        REQUIRE(&c.value() == &value);
    }
    SECTION("assignment")
    {
        optional_ref<int> a;
        a = nullptr;
        REQUIRE_FALSE(a.has_value());

        optional_ref<int> b;
        b = ref(value);
        REQUIRE(b.has_value());
        REQUIRE(&b.value() == &value);
    }
    SECTION("value_or")
    {
        int v1 = 0, v2 = 0;

        optional_ref<int> a;
        a.value_or(v2) = 1;
        REQUIRE(v2 == 1);
        REQUIRE(v1 == 0);
        v2 = 0;

        optional_ref<int> b(v1);
        b.value_or(v2) = 1;
        REQUIRE(v1 == 1);
        REQUIRE(v2 == 0);
    }
    SECTION("ref")
    {
        optional_ref<int> a = opt_ref(static_cast<int*>(nullptr));
        REQUIRE_FALSE(a.has_value());

        optional_ref<int> b = opt_ref(&value);
        REQUIRE(b.has_value());
        REQUIRE(&b.value() == &value);

        optional_ref<int> c = opt_ref(ref(value));
        REQUIRE(c.has_value());
        REQUIRE(&c.value() == &value);

        optional<int> opt_a, opt_b(42);

        optional_ref<int> d = opt_ref(opt_a);
        REQUIRE_FALSE(d.has_value());

        optional_ref<int> e = opt_ref(opt_b);
        REQUIRE(e.has_value());
        REQUIRE(&e.value() == &opt_b.value());

        optional_ref<int> f = opt_ref(value);
        REQUIRE(f.has_value());
        REQUIRE(&f.value() == &value);
    }
    SECTION("cref")
    {
        optional_ref<const int> a = opt_cref(static_cast<const int*>(nullptr));
        REQUIRE_FALSE(a.has_value());

        optional_ref<const int> b = opt_cref(&value);
        REQUIRE(b.has_value());
        REQUIRE(&b.value() == &value);

        optional_ref<const int> c = opt_cref(ref(value));
        REQUIRE(c.has_value());
        REQUIRE(&c.value() == &value);

        optional<int> opt_a, opt_b(42);

        optional_ref<const int> d = opt_cref(opt_a);
        REQUIRE_FALSE(d.has_value());

        optional_ref<const int> e = opt_cref(opt_b);
        REQUIRE(e.has_value());
        REQUIRE(&e.value() == &opt_b.value());

        optional_ref<const int> f = opt_cref(value);
        REQUIRE(f.has_value());
        REQUIRE(&f.value() == &value);
    }
    SECTION("copy")
    {
        debugger_type dbg(0);

        optional_ref<debugger_type> a;
        optional<debugger_type>     a_res = copy(a);
        REQUIRE_FALSE(a_res.has_value());

        optional_ref<debugger_type> b(dbg);
        optional<debugger_type>     b_res = copy(b);
        REQUIRE(b_res.has_value());
        REQUIRE(b_res.value().id == 0);
    }
    SECTION("value comparison")
    {
        int value2 = 0;

        optional_ref<int> a;
        optional_ref<int> b(value);
        optional_ref<int> c(value2);

        REQUIRE_FALSE(a == value);
        REQUIRE_FALSE(value == a);
        REQUIRE_FALSE(a == value2);
        REQUIRE_FALSE(value2 == a);

        REQUIRE(b == value);
        REQUIRE(value == b);
        REQUIRE_FALSE(b == value2);
        REQUIRE_FALSE(value2 == b);

        REQUIRE_FALSE(c == value);
        REQUIRE_FALSE(value == c);
        REQUIRE(c == value2);
        REQUIRE(value2 == c);
    }
}

TEST_CASE("optional_xvalue_ref")
{
    int value = 0;

    SECTION("constructor")
    {
        optional_xvalue_ref<int> a(nullptr);
        REQUIRE_FALSE(a.has_value());

        optional_xvalue_ref<int> b(value);
        REQUIRE(b.has_value());
        REQUIRE(b.value() == value);
    }
    SECTION("assignment")
    {
        optional_xvalue_ref<int> a;
        a = nullptr;
        REQUIRE_FALSE(a.has_value());

        optional_xvalue_ref<int> b;
        b = ref(value);
        REQUIRE(b.has_value());
        REQUIRE(b.value() == value);
    }
    SECTION("value_or")
    {
        int v1 = 1, v2 = 0;

        optional_xvalue_ref<int> a;
        REQUIRE(a.value_or(v2) == v2);

        optional_xvalue_ref<int> b(v1);
        REQUIRE(b.value_or(v2) == v1);
    }
    SECTION("xref")
    {
        optional_xvalue_ref<int> a = opt_xref(static_cast<int*>(nullptr));
        REQUIRE_FALSE(a.has_value());

        optional_xvalue_ref<int> b = opt_xref(&value);
        REQUIRE(b.has_value());
        REQUIRE(b.value() == value);
    }
    SECTION("move")
    {
        debugger_type dbg(0);

        optional_xvalue_ref<debugger_type> a;
        optional<debugger_type>            a_res = move(a);
        REQUIRE_FALSE(a_res.has_value());

        optional_xvalue_ref<debugger_type> b(dbg);
        optional<debugger_type>            b_res = move(b);
        REQUIRE(b_res.has_value());
        REQUIRE(b_res.value().id == 0);
#ifndef _MSC_VER
        REQUIRE(b_res.value().move_ctor());
#endif
    }
}
