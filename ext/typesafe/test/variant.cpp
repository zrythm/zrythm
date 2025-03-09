// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/variant.hpp>

#include <catch.hpp>

#include "debugger_type.hpp"

using type_safe::fallback_variant;
using type_safe::nullvar;
using type_safe::nullvar_t;
using type_safe::tagged_union;
using type_safe::union_type;
using type_safe::variant;
using type_safe::variant_type;

// use optional variant to be able to test all functions
// test policies separately
using variant_t = variant<nullvar_t, int, double, debugger_type>;
using union_t   = tagged_union<int, double, debugger_type>;

template <class Variant>
void check_variant_empty(const Variant& var)
{
    // boolean queries
    REQUIRE(!var.has_value());
    REQUIRE(!var);

    // type queries
    REQUIRE(var.type() == Variant::invalid_type);
    REQUIRE(var.has_value(variant_type<nullvar_t>{}));

    // value query
    // (check that it compiles and does not abort)
    nullvar_t null = var.value(variant_type<nullvar_t>{});
    (void)null;

    // optional_value queries
    REQUIRE(var.optional_value(variant_type<nullvar_t>{}));
    REQUIRE(!var.optional_value(variant_type<int>{}));
}

template <class Variant, typename T>
void check_variant_value(const Variant& var, const T& val)
{
    // boolean queries
    REQUIRE(var.has_value());
    REQUIRE(var);

    // type queries
    REQUIRE(var.type() == typename Variant::type_id(variant_type<T>{}));
    REQUIRE(var.has_value(variant_type<T>{}));

    // value query
    REQUIRE(var.value(variant_type<T>{}) == val);

    // optional_value queries
    using is_int    = std::is_same<T, int>;
    using is_double = std::is_same<T, double>;
    using is_dbg    = std::is_same<T, debugger_type>;

    REQUIRE(!var.optional_value(variant_type<nullvar_t>{}));
    REQUIRE(bool(var.optional_value(variant_type<int>{})) == is_int::value);
    REQUIRE(bool(var.optional_value(variant_type<double>{})) == is_double::value);
    REQUIRE(bool(var.optional_value(variant_type<debugger_type>{})) == is_dbg::value);
}

TEST_CASE("basic_variant")
{
    variant_t empty;
    variant_t non_empty1(5);
    variant_t non_empty2(debugger_type(42));

    SECTION("constructor - empty")
    {
        variant_t a;
        check_variant_empty(a);

        variant_t b(nullvar);
        check_variant_empty(b);
    }
    SECTION("constructor - value")
    {
        variant_t a(5);
        check_variant_value(a, 5);

        variant_t b(3.0);
        check_variant_value(b, 3.0);

        variant_t c(debugger_type(42));
        check_variant_value(c, debugger_type(42));
        REQUIRE(c.value(variant_type<debugger_type>{}).move_ctor());
    }
    SECTION("constructor - args")
    {
        variant_t a(variant_type<int>{}, 5);
        check_variant_value(a, 5);

        variant_t b(variant_type<double>{}, 3.0);
        check_variant_value(b, 3.0);

        variant_t c(variant_type<debugger_type>{}, 42);
        check_variant_value(c, debugger_type(42));
        REQUIRE(c.value(variant_type<debugger_type>{}).ctor());
    }
    SECTION("constructor - union copy")
    {
        union_t u;

        variant_t a(u);
        check_variant_empty(a);

        u.emplace(union_type<int>{}, 5);

        variant_t b(u);
        check_variant_value(b, 5);

        u.destroy(union_type<int>{});
        u.emplace(union_type<debugger_type>{}, debugger_type(42));

        variant_t c(u);
        check_variant_value(c, debugger_type(42));
        REQUIRE(c.value(variant_type<debugger_type>{}).copy_ctor());
    }
    SECTION("constructor - union move")
    {
        union_t u;

        variant_t a(std::move(u));
        check_variant_empty(a);

        u.emplace(union_type<int>{}, 5);

        variant_t b(std::move(u));
        check_variant_value(b, 5);

        u.destroy(union_type<int>{});
        u.emplace(union_type<debugger_type>{}, debugger_type(42));

        variant_t c(std::move(u));
        check_variant_value(c, debugger_type(42));
        REQUIRE(c.value(variant_type<debugger_type>{}).move_ctor());
    }
    SECTION("constructor - copy")
    {
        variant_t a(empty);
        check_variant_empty(a);

        variant_t b(non_empty1);
        check_variant_value(b, 5);

        variant_t c(non_empty2);
        check_variant_value(c, debugger_type(42));
        REQUIRE(c.value(variant_type<debugger_type>{}).copy_ctor());
    }
    SECTION("constructor - move")
    {
        variant_t a(std::move(empty));
        check_variant_empty(a);

        variant_t b(std::move(non_empty1));
        check_variant_value(b, 5);

        variant_t c(std::move(non_empty2));
        check_variant_value(c, debugger_type(42));
        REQUIRE(c.value(variant_type<debugger_type>{}).move_ctor());
    }
    SECTION("copy assignment")
    {
        variant_t a;
        a = empty;
        check_variant_empty(a);
        a = non_empty1;
        check_variant_value(a, 5);
        a = non_empty2;
        check_variant_value(a, debugger_type(42));
        REQUIRE(a.value(variant_type<debugger_type>{}).copy_ctor());

        variant_t b(5);
        b = non_empty1;
        check_variant_value(b, 5);
        b = non_empty2;
        check_variant_value(b, debugger_type(42));
        REQUIRE(b.value(variant_type<debugger_type>{}).copy_ctor());
        b = empty;
        check_variant_empty(b);

        variant_t c(debugger_type(42));
        c = non_empty2;
        check_variant_value(c, debugger_type(42));
        REQUIRE(c.value(variant_type<debugger_type>{}).copy_assigned());
        c = non_empty1;
        check_variant_value(c, 5);
        c = empty;
        check_variant_empty(c);
    }
    SECTION("move assignment")
    {
        variant_t a;
        a = std::move(empty);
        check_variant_empty(a);
        a = std::move(non_empty1);
        check_variant_value(a, 5);
        a = std::move(non_empty2);
        check_variant_value(a, debugger_type(42));
        REQUIRE(a.value(variant_type<debugger_type>{}).move_ctor());

        variant_t b(5);
        b = std::move(non_empty1);
        check_variant_value(b, 5);
        b = std::move(non_empty2);
        check_variant_value(b, debugger_type(42));
        REQUIRE(b.value(variant_type<debugger_type>{}).move_ctor());
        b = std::move(empty);
        check_variant_empty(b);

        variant_t c(debugger_type(42));
        c = std::move(non_empty2);
        check_variant_value(c, debugger_type(42));
        REQUIRE(c.value(variant_type<debugger_type>{}).move_assigned());
        c = std::move(non_empty1);
        check_variant_value(c, 5);
        c = std::move(empty);
        check_variant_empty(c);
    }
    SECTION("swap")
    {
        SECTION("empty, empty")
        {
            swap(empty, empty);
            check_variant_empty(empty);

            variant_t empty2(empty);
            swap(empty, empty2);
            check_variant_empty(empty);
            check_variant_empty(empty2);
        }
        SECTION("empty, non_empty1")
        {
            swap(empty, non_empty1);
            check_variant_value(empty, 5);
            check_variant_empty(non_empty1);

            swap(empty, non_empty1);
            check_variant_value(non_empty1, 5);
            check_variant_empty(empty);
        }
        SECTION("empty, non_empty2")
        {
            swap(empty, non_empty2);
            check_variant_value(empty, debugger_type(42));
            check_variant_empty(non_empty2);
            REQUIRE(empty.value(variant_type<debugger_type>{}).ctor());

            swap(empty, non_empty2);
            check_variant_value(non_empty2, debugger_type(42));
            check_variant_empty(empty);
            REQUIRE(non_empty2.value(variant_type<debugger_type>{}).ctor());
        }
        SECTION("non-empty, different types")
        {
            swap(non_empty1, non_empty2);
            check_variant_value(non_empty1, debugger_type(42));
            check_variant_value(non_empty2, 5);
            REQUIRE(non_empty1.value(variant_type<debugger_type>{}).move_ctor());

            swap(non_empty1, non_empty2);
            check_variant_value(non_empty2, debugger_type(42));
            check_variant_value(non_empty1, 5);
            REQUIRE(non_empty2.value(variant_type<debugger_type>{}).move_ctor());
        }
        SECTION("non-empty, same types")
        {
            swap(non_empty2, non_empty2);
            check_variant_value(non_empty2, debugger_type(42));
            REQUIRE(non_empty2.value(variant_type<debugger_type>{}).swapped);

            variant_t other(debugger_type(43));
            swap(non_empty2, other);
            check_variant_value(non_empty2, debugger_type(43));
            check_variant_value(other, debugger_type(42));
            REQUIRE(non_empty2.value(variant_type<debugger_type>{}).swapped);
            REQUIRE(other.value(variant_type<debugger_type>{}).swapped);

            swap(non_empty2, other);
            check_variant_value(non_empty2, debugger_type(42));
            check_variant_value(other, debugger_type(43));
            REQUIRE(non_empty2.value(variant_type<debugger_type>{}).swapped);
            REQUIRE(other.value(variant_type<debugger_type>{}).swapped);
        }
    }
    SECTION("reset")
    {
        empty.reset();
        check_variant_empty(empty);

        non_empty1.reset();
        check_variant_empty(non_empty1);

        non_empty2.reset();
        check_variant_empty(non_empty2);
    }
    SECTION("reset assignment")
    {
        empty = nullvar;
        check_variant_empty(empty);

        non_empty1 = nullvar;
        check_variant_empty(non_empty1);

        non_empty2 = nullvar;
        check_variant_empty(non_empty2);
    }
    SECTION("emplace single arg")
    {
        empty.emplace(variant_type<debugger_type>{}, debugger_type(43));
        check_variant_value(empty, debugger_type(43));
        REQUIRE(empty.value(variant_type<debugger_type>{}).move_ctor());

        non_empty1.emplace(variant_type<debugger_type>{}, debugger_type(43));
        check_variant_value(non_empty1, debugger_type(43));
        REQUIRE(non_empty1.value(variant_type<debugger_type>{}).move_ctor());

        non_empty2.emplace(variant_type<debugger_type>{}, debugger_type(43));
        check_variant_value(non_empty1, debugger_type(43));
        REQUIRE(non_empty2.value(variant_type<debugger_type>{}).move_assigned());
    }
    SECTION("emplace assignment")
    {
        empty = debugger_type(43);
        check_variant_value(empty, debugger_type(43));
        REQUIRE(empty.value(variant_type<debugger_type>{}).move_ctor());

        non_empty1 = debugger_type(43);
        check_variant_value(non_empty1, debugger_type(43));
        REQUIRE(non_empty1.value(variant_type<debugger_type>{}).move_ctor());

        non_empty2 = debugger_type(43);
        check_variant_value(non_empty1, debugger_type(43));
        REQUIRE(non_empty2.value(variant_type<debugger_type>{}).move_assigned());
    }
    SECTION("emplace multiple args")
    {
        empty.emplace(variant_type<debugger_type>{}, 43, 5.0, 'a');
        check_variant_value(empty, debugger_type(43));
        REQUIRE(empty.value(variant_type<debugger_type>{}).ctor());

        non_empty1.emplace(variant_type<debugger_type>{}, 43, 5.0, 'a');
        check_variant_value(non_empty1, debugger_type(43));
        REQUIRE(non_empty1.value(variant_type<debugger_type>{}).ctor());

        non_empty2.emplace(variant_type<debugger_type>{}, 43, 5.0, 'a');
        check_variant_value(non_empty1, debugger_type(43));
        REQUIRE(non_empty2.value(variant_type<debugger_type>{}).ctor());
    }
    SECTION("value_or")
    {
        REQUIRE(empty.value_or(variant_type<int>{}, 3) == 3);
        REQUIRE(non_empty1.value_or(variant_type<int>{}, 3) == 5);
        REQUIRE(non_empty2.value_or(variant_type<int>{}, 3) == 3);

        REQUIRE(non_empty2.value_or(variant_type<double>{}, 3.14) == 3.14);
    }
    SECTION("map")
    {
        struct functor_t
        {
            bool expect_call = false;

            int operator()(int i, int j)
            {
                REQUIRE(expect_call);
                REQUIRE(i == 5);
                REQUIRE(j == 0);
                return 12;
            }

            int operator()(const debugger_type& dbg, int j)
            {
                REQUIRE(expect_call);
                REQUIRE(dbg.id == 42);
                REQUIRE(j == 0);
                return 42;
            }

            int operator()(double, int) = delete;
        } functor;

        functor.expect_call = false;
        auto a              = empty.map(functor, 0);
        check_variant_empty(a);

        functor.expect_call = true;
        auto b              = non_empty1.map(functor, 0);
        check_variant_value(b, 12);

        functor.expect_call = true;
        auto c              = non_empty2.map(functor, 0);
        check_variant_value(c, 42);

        functor.expect_call = false;
        auto d              = variant_t(3.0).map(functor, 0);
        check_variant_value(d, 3.0);
    }
    SECTION("compare null")
    {
        REQUIRE(empty == nullvar);
        REQUIRE_FALSE(empty != nullvar);
        REQUIRE_FALSE(empty < nullvar);
        REQUIRE_FALSE(nullvar < empty);
        REQUIRE(empty <= nullvar);
        REQUIRE(nullvar <= empty);
        REQUIRE_FALSE(empty > nullvar);
        REQUIRE_FALSE(nullvar > empty);
        REQUIRE(empty >= nullvar);
        REQUIRE(nullvar >= empty);

        REQUIRE_FALSE(non_empty1 == nullvar);
        REQUIRE(non_empty1 != nullvar);
        REQUIRE_FALSE(non_empty1 < nullvar);
        REQUIRE(nullvar < non_empty1);
        REQUIRE_FALSE(non_empty1 <= nullvar);
        REQUIRE(nullvar <= non_empty1);
        REQUIRE(non_empty1 > nullvar);
        REQUIRE_FALSE(nullvar > non_empty1);
        REQUIRE(non_empty1 >= nullvar);
        REQUIRE_FALSE(nullvar >= non_empty1);
    }
    SECTION("compare value")
    {
        REQUIRE_FALSE(empty == 4);
        REQUIRE(empty != 4);
        REQUIRE(empty < 4);
        REQUIRE_FALSE(4 < empty);
        REQUIRE(empty <= 4);
        REQUIRE_FALSE(4 <= empty);
        REQUIRE_FALSE(empty > 4);
        REQUIRE(4 > empty);
        REQUIRE_FALSE(empty >= 4);
        REQUIRE(4 >= empty);

        REQUIRE_FALSE(non_empty1 == 4);
        REQUIRE(non_empty1 != 4);
        REQUIRE_FALSE(non_empty1 < 4);
        REQUIRE(4 < non_empty1);
        REQUIRE_FALSE(non_empty1 <= 4);
        REQUIRE(4 <= non_empty1);
        REQUIRE(non_empty1 > 4);
        REQUIRE_FALSE(4 > non_empty1);
        REQUIRE(non_empty1 >= 4);
        REQUIRE_FALSE(4 >= non_empty1);

        REQUIRE(non_empty1 == 5);
        REQUIRE_FALSE(non_empty1 != 5);
        REQUIRE_FALSE(non_empty1 < 5);
        REQUIRE_FALSE(5 < non_empty1);
        REQUIRE(non_empty1 <= 5);
        REQUIRE(5 <= non_empty1);
        REQUIRE_FALSE(non_empty1 > 5);
        REQUIRE_FALSE(5 > non_empty1);
        REQUIRE(non_empty1 >= 5);
        REQUIRE(5 >= non_empty1);

        REQUIRE_FALSE(non_empty2 == 4);
        REQUIRE(non_empty2 != 4);
        REQUIRE_FALSE(non_empty2 < 4);
        REQUIRE(4 < non_empty2);
        REQUIRE_FALSE(non_empty2 <= 4);
        REQUIRE(4 <= non_empty2);
        REQUIRE(non_empty2 > 4);
        REQUIRE_FALSE(4 > non_empty2);
        REQUIRE(non_empty2 >= 4);
        REQUIRE_FALSE(4 >= non_empty2);
    }
    SECTION("compare variant")
    {
        REQUIRE(empty == variant_t());
        REQUIRE_FALSE(empty != variant_t());
        REQUIRE_FALSE(empty < variant_t());
        REQUIRE_FALSE(variant_t() < empty);
        REQUIRE(empty <= variant_t());
        REQUIRE(variant_t() <= empty);
        REQUIRE_FALSE(empty > variant_t());
        REQUIRE_FALSE(variant_t() > empty);
        REQUIRE(empty >= variant_t());
        REQUIRE(variant_t() >= empty);

        REQUIRE_FALSE(non_empty1 == variant_t());
        REQUIRE(non_empty1 != variant_t());
        REQUIRE_FALSE(non_empty1 < variant_t());
        REQUIRE(variant_t() < non_empty1);
        REQUIRE_FALSE(non_empty1 <= variant_t());
        REQUIRE(variant_t() <= non_empty1);
        REQUIRE(non_empty1 > variant_t());
        REQUIRE_FALSE(variant_t() > non_empty1);
        REQUIRE(non_empty1 >= variant_t());
        REQUIRE_FALSE(variant_t() >= non_empty1);

        REQUIRE_FALSE(empty == variant_t(4));
        REQUIRE(empty != variant_t(4));
        REQUIRE(empty < variant_t(4));
        REQUIRE_FALSE(variant_t(4) < empty);
        REQUIRE(empty <= variant_t(4));
        REQUIRE_FALSE(variant_t(4) <= empty);
        REQUIRE_FALSE(empty > variant_t(4));
        REQUIRE(variant_t(4) > empty);
        REQUIRE_FALSE(empty >= variant_t(4));
        REQUIRE(variant_t(4) >= empty);

        REQUIRE_FALSE(non_empty1 == variant_t(4));
        REQUIRE(non_empty1 != variant_t(4));
        REQUIRE_FALSE(non_empty1 < variant_t(4));
        REQUIRE(variant_t(4) < non_empty1);
        REQUIRE_FALSE(non_empty1 <= variant_t(4));
        REQUIRE(variant_t(4) <= non_empty1);
        REQUIRE(non_empty1 > variant_t(4));
        REQUIRE_FALSE(variant_t(4) > non_empty1);
        REQUIRE(non_empty1 >= variant_t(4));
        REQUIRE_FALSE(variant_t(4) >= non_empty1);

        REQUIRE(non_empty1 == variant_t(5));
        REQUIRE_FALSE(non_empty1 != variant_t(5));
        REQUIRE_FALSE(non_empty1 < variant_t(5));
        REQUIRE_FALSE(variant_t(5) < non_empty1);
        REQUIRE(non_empty1 <= variant_t(5));
        REQUIRE(variant_t(5) <= non_empty1);
        REQUIRE_FALSE(non_empty1 > variant_t(5));
        REQUIRE_FALSE(variant_t(5) > non_empty1);
        REQUIRE(non_empty1 >= variant_t(5));
        REQUIRE(variant_t(5) >= non_empty1);

        REQUIRE_FALSE(non_empty2 == variant_t(4));
        REQUIRE(non_empty2 != variant_t(4));
        REQUIRE_FALSE(non_empty2 < variant_t(4));
        REQUIRE(variant_t(4) < non_empty2);
        REQUIRE_FALSE(non_empty2 <= variant_t(4));
        REQUIRE(variant_t(4) <= non_empty2);
        REQUIRE(non_empty2 > variant_t(4));
        REQUIRE_FALSE(variant_t(4) > non_empty2);
        REQUIRE(non_empty2 >= variant_t(4));
        REQUIRE_FALSE(variant_t(4) >= non_empty2);
    }
    SECTION("with")
    {
        struct visitor
        {
            int i;

            void operator()(int val) const
            {
                REQUIRE(i == 1);
                REQUIRE(val == 5);
            }

            void operator()(double f) const
            {
                REQUIRE(i == 2);
                REQUIRE(f == 3.14);
            }
        } v;

        variant_t a;
        v.i = 0;
        with(a, v);

        variant_t b(5);
        v.i = 1;
        with(b, v);

        variant_t c(3.14);
        v.i = 2;
        with(c, v);

        variant_t d(5);
        with(d, [](int& i) { ++i; });
        check_variant_value(d, 6);
    }
}

struct evil_variant_test_type
{
    bool move_throw = false;

    evil_variant_test_type(int) noexcept {}

    evil_variant_test_type(const char*)
    {
        throw "buh!";
    }

    evil_variant_test_type(double)
    {
        move_throw = true;
    }

    evil_variant_test_type(evil_variant_test_type&& other)
    {
        if (other.move_throw)
            throw "haha!";
    }
};

inline bool operator==(const evil_variant_test_type&, const evil_variant_test_type&)
{
    return true;
}

TEST_CASE("fallback_variant")
{
    fallback_variant<int, evil_variant_test_type> var(3);

    SECTION("harmless ctor")
    {
        var.emplace(variant_type<evil_variant_test_type>{}, 42);
        check_variant_value(var, evil_variant_test_type(42));
    }
    SECTION("move ctor")
    {
        var = evil_variant_test_type(42);
        check_variant_value(var, evil_variant_test_type(42));
    }
    SECTION("throwing move")
    {
        evil_variant_test_type test(42);
        test.move_throw = true;
        try
        {
            var = std::move(test);
        }
        catch (...)
        {}
        check_variant_value(var, 0);
    }
}

TEST_CASE("optional_variant")
{
    variant<nullvar_t, int, evil_variant_test_type> var(3);

    SECTION("harmless ctor")
    {
        var.emplace(variant_type<evil_variant_test_type>{}, 42);
        check_variant_value(var, evil_variant_test_type(42));
    }
    SECTION("move ctor")
    {
        var = evil_variant_test_type(42);
        check_variant_value(var, evil_variant_test_type(42));
    }
    SECTION("throwing move")
    {
        evil_variant_test_type test(42);
        test.move_throw = true;
        try
        {
            var = std::move(test);
        }
        catch (...)
        {}
        check_variant_empty(var);
    }
}

TEST_CASE("rarely_empty_variant")
{
    variant<int, evil_variant_test_type> var(3);

    SECTION("harmless ctor")
    {
        var.emplace(variant_type<evil_variant_test_type>{}, 42);
        check_variant_value(var, evil_variant_test_type(42));
    }
    SECTION("move ctor")
    {
        var = evil_variant_test_type(42);
        check_variant_value(var, evil_variant_test_type(42));
    }
    SECTION("throwing ctor")
    {
        try
        {
            var.emplace(variant_type<evil_variant_test_type>{}, "I will throw");
        }
        catch (...)
        {}
        check_variant_value(var, 3);
    }
    SECTION("delayed throwing move")
    {
        try
        {
            var.emplace(variant_type<evil_variant_test_type>{}, 3.14);
        }
        catch (...)
        {}
        check_variant_empty(var);
    }
    SECTION("throwing move")
    {
        evil_variant_test_type test(42);
        test.move_throw = true;
        try
        {
            var = std::move(test);
        }
        catch (...)
        {}
        check_variant_empty(var);
    }
}
