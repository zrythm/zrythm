// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>
#include <string>

#include <type_safe/constrained_type.hpp>

namespace ts = type_safe;

// a string type that is never empty
// the constraint is just a predicate, you can easily write your own
using non_empty_string = ts::constrained_type<std::string, ts::constraints::non_empty>;

// this function must not get an empty string
// the return value is also not empty
non_empty_string func(non_empty_string str)
{
    std::cout << str.get_value() << '\n';
    // get_value() returns a const reference
    // if we want to modify it:
    {
        auto modifier = str.modify();
        modifier.get() += " suffix";
        // destructor of modifier verifies that str fulfills constraint again
    } // extra scope is important here
    return str;
}

int main()
{
    //    std::cout << func("foo") << '\n'; // error: "foo" is not a non_empty_string
    // we must be explicit
    std::string str = func(non_empty_string("foo")).release(); // release() moves the string out
    std::cout << str << '\n';

    non_empty_string str2("foobar");
    // an alternative to str2.modify():
    ts::with(str2, [&](std::string& s) { s += str; });
    std::cout << str2.get_value() << '\n';
}

// some constraints do not need to be checked
// (example taken from: https://www.youtube.com/watch?v=ojZbFIQSdl8)
struct sanitized
{};
struct unsanitized
{};

// ts::tagged_type is an alias for ts::constrained_type with the ts::null_verifier as third
// parameter it is a constrained type where the constraint isn't checked so the constraint does not
// need to be a predicate it is just there to create different, non convertible types
using sanitized_string   = ts::tagged_type<std::string, sanitized>;
using unsanitized_string = ts::tagged_type<std::string, unsanitized>;

unsanitized_string get_form_data();
sanitized_string   sanitize(const unsanitized_string& str);
void               execute_query(const sanitized_string& str);

// now impossible to accidentally use unsanitized strings
void do_stuff()
{
    //    execute_query(get_form_data());             // error!
    //    execute_query(get_form_data().get_value()); // error!
    //    execute_query(sanitize(get_form_data()));   // only this possible (but linker error,
    //    so...)
}
