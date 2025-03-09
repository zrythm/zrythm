// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>
#include <sstream>
#include <string>

#include <type_safe/output_parameter.hpp>

namespace ts = type_safe;

// task: read strings from stream until EOF
// concatenate all strings
// return whether any strings read

// using a reference as output parameter - BAD:
// * not obvious from the caller that the string will be modified  (in the general case)
// * requires default constructor
// * implicit precondition that the output is empty
bool read_str_naive(std::istream& in, std::string& out)
{
    for (std::string tmp; in >> tmp;)
        out += tmp;
    return !out.empty();
}

// does not have these problems
bool read_str_better(std::istream& in, ts::output_parameter<std::string> out)
{
    std::string result; // no way to access the string directly
    // so need to create new one

    for (std::string tmp; in >> tmp;)
        result += tmp;

    // granted, that's verbose
    auto empty = result.empty();    // we need to query here, because after move it might be empty
    out        = std::move(result); // set output parameter
    return !empty;

    // so use this one:
    // assignment op returns the value that was assigned
    return (out = std::move(result)).empty();
}

int main()
{
    {
        std::istringstream in("hello world");
        std::string        str;
        auto               res = read_str_naive(in, str);
        std::cout << res << ' ' << str << '\n';
    }
    {
        std::istringstream in("hello world");
        std::string        str;
        // use ts::out() to create an output_parameter easily
        auto res = read_str_better(in, ts::out(str));
        std::cout << res << ' ' << str << '\n';
    }
    // what if std::string had no default constructor?
    {
        // use this one:
        ts::deferred_construction<std::string> str;
        // str is not initialized yet,
        // so it does not require a constructor
        // once you give it a value, it will never be empty again

        std::istringstream in("hello world");
        auto               res = read_str_better(in, ts::out(str));
        // if the function assigned a value to the output parameter,
        // it will call the constructor and directly initializes it with the correct value
        std::cout << res << ' ' << str.has_value() << ' ' << str.value() << '\n';
    }
}
