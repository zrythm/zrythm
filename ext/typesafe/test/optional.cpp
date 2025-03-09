// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/optional.hpp>
#include <type_safe/visitor.hpp>

#include <catch.hpp>

#include "debugger_type.hpp"

using type_safe::optional;
using type_safe::optional_ref;
using type_safe::make_optional;
using type_safe::nullopt;

TEST_CASE("optional")
{
    SECTION("constructor - empty")
    {
        optional<int> a;
        REQUIRE_FALSE(a.has_value());

        optional<int> b(nullopt);
        REQUIRE_FALSE(b.has_value());
    }
    SECTION("constructor - value")
    {
        optional<debugger_type> a(debugger_type(3));
        REQUIRE(a.has_value());
        REQUIRE(a.value().id == 3);
        REQUIRE(a.value().move_ctor());

        debugger_type           dbg(2);
        optional<debugger_type> b(dbg);
        REQUIRE(b.has_value());
        REQUIRE(b.value().id == 2);
        REQUIRE(b.value().copy_ctor());

        optional<debugger_type> c(std::move(dbg));
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 2);
        REQUIRE(c.value().move_ctor());

        optional<debugger_type> d(0);
        REQUIRE(d.has_value());
        REQUIRE(d.value().id == 0);
        REQUIRE(d.value().ctor());
    }
    SECTION("constructor - move/copy")
    {
        optional<debugger_type> org_empty;
        optional<debugger_type> org_value(debugger_type(0));

        optional<debugger_type> a(org_empty);
        REQUIRE_FALSE(a.has_value());

        optional<debugger_type> b(std::move(org_empty));
        REQUIRE_FALSE(b.has_value());

        optional<debugger_type> c(org_value);
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 0);
        REQUIRE(c.value().copy_ctor());

        optional<debugger_type> d(std::move(org_value));
        REQUIRE(d.has_value());
        REQUIRE(d.value().id == 0);
        REQUIRE(d.value().move_ctor());
    }
    SECTION("assignment - nullopt_t")
    {
        optional<debugger_type> a;
        a = nullopt;
        REQUIRE_FALSE(a.has_value());

        optional<debugger_type> b(4);
        b = nullopt;
        REQUIRE_FALSE(b.has_value());
    }
    SECTION("assignment - value")
    {
        optional<debugger_type> a;
        a = debugger_type(0);
        REQUIRE(a.has_value());
        REQUIRE(a.value().id == 0);
        REQUIRE(a.value().move_ctor());
        REQUIRE(a.value().not_assigned());

        debugger_type           dbg(0);
        optional<debugger_type> b;
        b = dbg;
        REQUIRE(b.has_value());
        REQUIRE(b.value().id == 0);
        REQUIRE(b.value().copy_ctor());
        REQUIRE(b.value().not_assigned());

        optional<debugger_type> c;
        c = 0;
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 0);
        REQUIRE(c.value().ctor());
        REQUIRE(c.value().not_assigned());

        optional<debugger_type> d(0);
        d = debugger_type(1);
        REQUIRE(d.has_value());
        REQUIRE(d.value().id == 1);
        REQUIRE(d.value().ctor());
        REQUIRE(d.value().move_assigned());

        dbg.id = 1;
        optional<debugger_type> e(0);
        e = dbg;
        REQUIRE(e.has_value());
        REQUIRE(e.value().id == 1);
        REQUIRE(e.value().ctor());
        REQUIRE(e.value().copy_assigned());

        optional<debugger_type> f(0);
        f = 1; // assignment would use implicit conversion, so it destroys & recreates
        REQUIRE(f.has_value());
        REQUIRE(f.value().id == 1);
        REQUIRE(f.value().ctor());
        REQUIRE(f.value().not_assigned());
    }
    SECTION("assignment - move/copy")
    {
        optional<debugger_type> new_empty;
        optional<debugger_type> new_value(debugger_type(1));

        optional<debugger_type> a;
        a = new_empty;
        REQUIRE_FALSE(a.has_value());

        optional<debugger_type> b;
        b = new_value;
        REQUIRE(b.has_value());
        REQUIRE(b.value().id == 1);
        REQUIRE(b.value().copy_ctor());
        REQUIRE(b.value().not_assigned());

        optional<debugger_type> c;
        c = std::move(new_value);
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 1);
        REQUIRE(c.value().not_assigned());

        optional<debugger_type> d(0);
        d = new_empty;
        REQUIRE_FALSE(d.has_value());

        optional<debugger_type> e(0);
        e = new_value;
        REQUIRE(e.has_value());
        REQUIRE(e.value().id == 1);
        REQUIRE(e.value().ctor());
        REQUIRE(e.value().copy_assigned());

        optional<debugger_type> f(0);
        f = std::move(new_value);
        REQUIRE(f.has_value());
        REQUIRE(f.value().id == 1);
        REQUIRE(f.value().ctor());
    }
    SECTION("swap")
    {
        optional<debugger_type> empty1, empty2;
        optional<debugger_type> a(0);
        optional<debugger_type> b(1);

        SECTION("empty, empty")
        {
            swap(empty1, empty2);
            REQUIRE_FALSE(empty1.has_value());
            REQUIRE_FALSE(empty2.has_value());
        }
        SECTION("value, value")
        {
            swap(a, b);
            REQUIRE(a.has_value());
            REQUIRE(a.value().id == 1);
            REQUIRE(a.value().swapped);
            REQUIRE(b.has_value());
            REQUIRE(b.value().id == 0);
            REQUIRE(b.value().swapped);
        }
        SECTION("empty, value")
        {
            swap(empty1, a);
            REQUIRE_FALSE(a.has_value());
            REQUIRE(empty1.has_value());
            REQUIRE(empty1.value().id == 0);
            REQUIRE(empty1.value().not_assigned());
            REQUIRE_FALSE(empty1.value().swapped);
        }
        SECTION("value, empty")
        {
            swap(a, empty1);
            REQUIRE_FALSE(a.has_value());
            REQUIRE(empty1.has_value());
            REQUIRE(empty1.value().id == 0);
            REQUIRE(empty1.value().move_ctor());
            REQUIRE(empty1.value().not_assigned());
            REQUIRE_FALSE(empty1.value().swapped);
        }
    }
    SECTION("reset")
    {
        optional<debugger_type> a;
        a.reset();
        REQUIRE_FALSE(a.has_value());

        optional<debugger_type> b(0);
        b.reset();
        REQUIRE_FALSE(b.has_value());
    }
    SECTION("emplace")
    {
        debugger_type dbg(1);

        optional<debugger_type> a;
        a.emplace(1);
        REQUIRE(a.has_value());
        REQUIRE(a.value().id == 1);
        REQUIRE(a.value().ctor());
        REQUIRE(a.value().not_assigned());

        optional<debugger_type> b;
        b.emplace(dbg);
        REQUIRE(b.has_value());
        REQUIRE(b.value().id == 1);
        REQUIRE(b.value().copy_ctor());
        REQUIRE(b.value().not_assigned());

        optional<debugger_type> c;
        c.emplace(std::move(dbg));
        REQUIRE(c.has_value());
        REQUIRE(c.value().id == 1);
        REQUIRE(c.value().not_assigned());

        optional<debugger_type> d(0);
        d.emplace(1);
        REQUIRE(d.has_value());
        REQUIRE(d.value().id == 1);
        REQUIRE(d.value().ctor());
        REQUIRE(d.value().not_assigned());

        optional<debugger_type> e(0);
        e.emplace(dbg);
        REQUIRE(e.has_value());
        REQUIRE(e.value().id == 1);
        REQUIRE(e.value().ctor());
        REQUIRE(e.value().copy_assigned());

        optional<debugger_type> f(0);
        f.emplace(std::move(dbg));
        REQUIRE(f.has_value());
        REQUIRE(f.value().id == 1);
        REQUIRE(f.value().ctor());
        REQUIRE(f.value().move_assigned());
    }
    SECTION("operator bool")
    {
        optional<int> a;
        REQUIRE_FALSE(static_cast<bool>(a));

        optional<int> b(0);
        REQUIRE(static_cast<bool>(b));
    }
    SECTION("value")
    {
        // only test the return types
        optional<debugger_type> a(0);
        static_assert(std::is_same<decltype(a.value()), debugger_type&>::value, "");
        static_assert(std::is_same<decltype(std::move(a.value())), debugger_type&&>::value, "");

        const optional<debugger_type> b(0);
        static_assert(std::is_same<decltype(b.value()), const debugger_type&>::value, "");
        static_assert(std::is_same<decltype(std::move(b.value())), const debugger_type&&>::value,
                      "");
    }
    SECTION("value_or")
    {
        optional<debugger_type> a;
        auto                    a_res = a.value_or(1);
        REQUIRE(a_res.id == 1);

        optional<debugger_type> b(0);
        auto                    b_res = b.value_or(1);
        REQUIRE(b_res.id == 0);
    }
    SECTION("map")
    {
        auto func = [](int i) { return "abc"[i]; };

        optional<int>  a;
        optional<char> a_res = a.map(func);
        REQUIRE_FALSE(a_res.has_value());

        optional<int>  b(0);
        optional<char> b_res = b.map(func);
        REQUIRE(b_res.has_value());
        REQUIRE(b_res.value() == 'a');

        struct foo
        {
            int var = 42;

            int func(int i)
            {
                return 2 * i;
            }

            void func2(int i)
            {
                REQUIRE(i == 42);
            }
        };

        optional<foo> c(foo{});

        optional<int> c_res = c.map(&foo::func, 2);
        REQUIRE(c_res.has_value());
        REQUIRE(c_res.value() == 4);

        c.map(&foo::func2, 42);

        optional_ref<int> c_res2 = c.map(&foo::var);
        REQUIRE(c_res2.has_value());
        REQUIRE(c_res2.value() == 42);

#if TYPE_SAFE_USE_RETURN_TYPE_DEDUCTION
        // just compiler check, see https://github.com/foonathan/type_safe/issues/60
        struct bar
        {
            void non_const() {}
        };

        optional<bar> f = bar{};
        f.map([](auto&& b) {
            b.non_const();
            return 42;
        });
#endif
    }
    SECTION("with")
    {
        optional<int> a;
        with(a, [](int) { REQUIRE(false); });

        a = 0;
        with(a, [](int& i) {
            REQUIRE(i == 0);
            i = 1;
        });
        REQUIRE(a.has_value());
        REQUIRE(a.value() == 1);
    }
    SECTION("comparison")
    {
        optional<int> a;
        optional<int> b(1);
        optional<int> c(2);

        // ==
        REQUIRE(b == b);
        REQUIRE(!(b == c));
        REQUIRE(!(b == a));

        REQUIRE(a == nullopt);
        REQUIRE(nullopt == a);
        REQUIRE(!(b == nullopt));
        REQUIRE(!(nullopt == b));

        REQUIRE(b == 1);
        REQUIRE(!(a == 1));
        REQUIRE(!(1 == a));
        REQUIRE(!(c == 1));
        REQUIRE(!(1 == c));

        // !=
        REQUIRE(a != b);
        REQUIRE(b != c);
        REQUIRE(!(a != a));

        REQUIRE(b != nullopt);
        REQUIRE(nullopt != b);
        REQUIRE(!(a != nullopt));
        REQUIRE(!(nullopt != a));

        REQUIRE(b != 2);
        REQUIRE(2 != b);
        REQUIRE(a != 2);
        REQUIRE(2 != a);
        REQUIRE(!(c != 2));
        REQUIRE(!(2 != c));

        // <
        REQUIRE(a < b);
        REQUIRE(b < c);
        REQUIRE(!(c < b));
        REQUIRE(!(b < a));

        REQUIRE(!(a < nullopt));
        REQUIRE(!(nullopt < a));
        REQUIRE(!(b < nullopt));
        REQUIRE(nullopt < b);

        REQUIRE(a < 2);
        REQUIRE(!(2 < a));
        REQUIRE(!(c < 2));
        REQUIRE(!(2 < c));

        // <=
        REQUIRE(a <= b);
        REQUIRE(b <= c);
        REQUIRE(b <= b);
        REQUIRE(!(c <= b));

        REQUIRE(a <= nullopt);
        REQUIRE(nullopt <= a);
        REQUIRE(!(b <= nullopt));
        REQUIRE(nullopt <= b);

        REQUIRE(a <= 2);
        REQUIRE(!(2 <= a));
        REQUIRE(b <= 2);
        REQUIRE(!(2 <= b));
        REQUIRE(c <= 2);
        REQUIRE(2 <= c);

        // >
        REQUIRE(c > b);
        REQUIRE(b > a);
        REQUIRE(!(a > b));

        REQUIRE(b > nullopt);
        REQUIRE(!(nullopt > b));
        REQUIRE(!(a > nullopt));
        REQUIRE(!(nullopt > b));

        REQUIRE(c > 1);
        REQUIRE(!(1 > c));
        REQUIRE(!(b > 1));
        REQUIRE(!(1 > b));
        REQUIRE(!(a > 1));
        REQUIRE(1 > a);

        // >=
        REQUIRE(c >= b);
        REQUIRE(b >= a);
        REQUIRE(a >= a);
        REQUIRE(!(a >= b));

        REQUIRE(a >= nullopt);
        REQUIRE(nullopt >= a);
        REQUIRE(b >= nullopt);
        REQUIRE(!(nullopt >= b));

        REQUIRE(b >= 1);
        REQUIRE(1 >= b);
        REQUIRE(c >= 1);
        REQUIRE(!(1 >= c));
        REQUIRE(!(a >= 1));
        REQUIRE(1 >= a);
    }
    SECTION("make_optional")
    {
        optional<int> a = make_optional(5);
        REQUIRE(a.has_value());
        REQUIRE(a.value() == 5);

        optional<std::string> b = make_optional<std::string>(1u, 'a');
        REQUIRE(b.has_value());
        REQUIRE(b.value() == "a");
    }
}
