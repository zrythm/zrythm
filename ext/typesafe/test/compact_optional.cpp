// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/compact_optional.hpp>

#include <catch.hpp>

using namespace type_safe;

namespace type_safe
{
template class basic_optional<compact_optional_storage<compact_bool_policy<bool>>>;
template class basic_optional<compact_optional_storage<compact_integer_policy<int, -1>>>;
template class basic_optional<compact_optional_storage<compact_floating_point_policy<float>>>;
} // namespace type_safe

TEST_CASE("compact_bool")
{
    using storage = compact_optional_storage<compact_bool_policy<bool>>;

    storage s;
    REQUIRE(!s.has_value());

    s.create_value(true);
    REQUIRE(s.has_value());
    REQUIRE(s.get_value() == true);

    s.destroy_value();
    REQUIRE(!s.has_value());

    s.create_value(false);
    REQUIRE(s.has_value());
    REQUIRE(s.get_value() == false);
}

TEST_CASE("compact_integer")
{
    using storage = compact_optional_storage<compact_integer_policy<int, -1>>;

    storage s;
    REQUIRE(!s.has_value());

    s.create_value(0);
    REQUIRE(s.has_value());
    REQUIRE(s.get_value() == 0);

    s.destroy_value();
    REQUIRE(!s.has_value());

    s.create_value(1);
    REQUIRE(s.has_value());
    REQUIRE(s.get_value() == 1);
}

TEST_CASE("compact_floating_point")
{
    using storage = compact_optional_storage<compact_floating_point_policy<float>>;

    storage s;
    REQUIRE(!s.has_value());

    s.create_value(0.1);
    REQUIRE(s.has_value());
    REQUIRE(s.get_value() == 0.1f);

    s.destroy_value();
    REQUIRE(!s.has_value());

    s.create_value(1.0);
    REQUIRE(s.has_value());
    REQUIRE(s.get_value() == 1.0);
}

enum class test_compact_enum
{
    a,
    b,
};

template class type_safe::basic_optional<
    compact_optional_storage<compact_enum_policy<test_compact_enum, 2>>>;

TEST_CASE("compact_enum")
{
    using storage = compact_optional_storage<compact_enum_policy<test_compact_enum, 2>>;

    storage s;
    REQUIRE(!s.has_value());

    s.create_value(test_compact_enum::a);
    REQUIRE(s.has_value());
    REQUIRE(s.get_value() == test_compact_enum::a);

    s.destroy_value();
    REQUIRE(!s.has_value());

    s.create_value(test_compact_enum::b);
    REQUIRE(s.has_value());
    REQUIRE(s.get_value() == test_compact_enum::b);
}

struct test_compact_container
{
    bool empty_ = true;

    test_compact_container() {}

    test_compact_container(int) : empty_(false) {}

    bool empty() const
    {
        return empty_;
    }
};
template class type_safe::basic_optional<
    compact_optional_storage<compact_container_policy<test_compact_container>>>;

TEST_CASE("compact_container")
{
    using storage = compact_optional_storage<compact_container_policy<test_compact_container>>;

    storage s;
    REQUIRE(!s.has_value());

    s.create_value(0);
    REQUIRE(s.has_value());
    REQUIRE(!s.get_value().empty());

    s.destroy_value();
    REQUIRE(!s.has_value());
}
