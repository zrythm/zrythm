// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <type_safe/output_parameter.hpp>

#include <catch.hpp>
#include <string>

using namespace type_safe;

TEST_CASE("output_parameter")
{
    SECTION("reference")
    {
        std::string output;

        output_parameter<std::string> out(output);
        SECTION("operator=")
        {
            std::string& res = (out = "abc");
            REQUIRE(&res == &output);
            REQUIRE(res == "abc");
        }
        SECTION("assign")
        {
            std::string& res = out.assign(3u, 'c');
            REQUIRE(&res == &output);
            REQUIRE(res == "ccc");
        }
        SECTION("multi")
        {
            std::string& res_a = (out = "abc");
            REQUIRE(&res_a == &output);
            REQUIRE(res_a == "abc");

            std::string& res_b = (out = "def");
            REQUIRE(&res_b == &output);
            REQUIRE(res_b == "def");

            std::string& res_c = out.assign(3u, 'c');
            REQUIRE(&res_c == &output);
            REQUIRE(res_c == "ccc");
        }
    }
    SECTION("deferred_construction")
    {
        deferred_construction<std::string> output;

        output_parameter<std::string> out(output);
        SECTION("operator=")
        {
            std::string& res = (out = "abc");
            REQUIRE(output.has_value());
            REQUIRE(&res == &output.value());
            REQUIRE(res == "abc");
        }
        SECTION("assign")
        {
            std::string& res = out.assign(3u, 'c');
            REQUIRE(output.has_value());
            REQUIRE(&res == &output.value());
            REQUIRE(res == "ccc");
        }
        SECTION("multi")
        {
            std::string& res_a = (out = "abc");
            REQUIRE(output.has_value());
            REQUIRE(&res_a == &output.value());
            REQUIRE(res_a == "abc");

            std::string& res_b = (out = "def");
            REQUIRE(&res_b == &output.value());
            REQUIRE(res_b == "def");

            std::string& res_c = out.assign(3u, 'c');
            REQUIRE(&res_c == &output.value());
            REQUIRE(res_c == "ccc");
        }
    }
}
