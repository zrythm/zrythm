/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2024 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>

#include "../common/StretchCalculator.h"

#include <iostream>

using namespace RubberBand;

using std::cerr;
using std::endl;

using std::vector;

namespace tt = boost::test_tools;

static Log cerrLog(
    [](const char *message) {
        cerr << message << "\n";
    },
    [](const char *message, double arg) {
        cerr << message << ": " << arg << "\n";
    },
    [](const char *message, double arg0, double arg1) {
        cerr << message << ": (" << arg0 << ", " << arg1 << ")\n";
    }
    );

BOOST_AUTO_TEST_SUITE(TestStretchCalculator)

BOOST_AUTO_TEST_CASE(offline_linear_hp)
{
    StretchCalculator sc(44100, 512, true, cerrLog);

    vector<int> out, expected;

    out = sc.calculate(1.0, 5120, { 1.0, 1.0, 1.0, 1.0, 1.0,
                                    1.0, 1.0, 1.0, 1.0, 1.0 }); 
    expected = { 512, 512, 512, 512, 512,
                 512, 512, 512, 512, 512 };
    BOOST_TEST(out == expected, tt::per_element());

    out = sc.calculate(0.5, 5120, { 1.0, 1.0, 1.0, 1.0, 1.0,
                                    1.0, 1.0, 1.0, 1.0, 1.0 }); 
    expected = { 256, 256, 256, 256, 256,
                 256, 256, 256, 256, 256 };
    BOOST_TEST(out == expected, tt::per_element());
}

BOOST_AUTO_TEST_CASE(offline_linear_nohp)
{
    StretchCalculator sc(44100, 512, false, cerrLog);

    vector<int> out, expected;

    out = sc.calculate(1.0, 5120, { 1.0, 1.0, 1.0, 1.0, 1.0,
                                    1.0, 1.0, 1.0, 1.0, 1.0 }); 
    expected = { 512, 512, 512, 512, 512,
                 512, 512, 512, 512, 512 };
    BOOST_TEST(out == expected, tt::per_element());

    out = sc.calculate(0.5, 5120, { 1.0, 1.0, 1.0, 1.0, 1.0,
                                    1.0, 1.0, 1.0, 1.0, 1.0 }); 
    expected = { 256, 256, 256, 256, 256,
                 256, 256, 256, 256, 256 };
    BOOST_TEST(out == expected, tt::per_element());
}

BOOST_AUTO_TEST_CASE(offline_onep_hp)
{
    StretchCalculator sc(44100, 512, true, cerrLog);
    
    vector<int> out, expected;

    out = sc.calculate(1.0, 5120, { 1.0, 1.0, 1.0, 1.0, 2.0,
                                    1.0, 1.0, 1.0, 1.0, 1.0 }); 
    expected = { 512, 512, 512, 512, -512,
                 512, 512, 512, 512, 512 };
    BOOST_TEST(out == expected, tt::per_element());

    out = sc.calculate(0.5, 5120, { 1.0, 1.0, 1.0, 1.0, 2.0,
                                    1.0, 1.0, 1.0, 1.0, 1.0 }); 
    expected = { 256, 256, 256, 256, -512,
                 205, 205, 204, 205, 205 };
    BOOST_TEST(out == expected, tt::per_element());
}

BOOST_AUTO_TEST_CASE(offline_onep_nohp)
{
    StretchCalculator sc(44100, 512, false, cerrLog);
    
    vector<int> out, expected;

    out = sc.calculate(1.0, 5120, { 1.0, 1.0, 1.0, 1.0, 2.0,
                                    1.0, 1.0, 1.0, 1.0, 1.0 }); 
    expected = { 512, 512, 512, 512, 512,
                 512, 512, 512, 512, 512 };
    BOOST_TEST(out == expected, tt::per_element());

    out = sc.calculate(0.5, 5120, { 1.0, 1.0, 1.0, 1.0, 2.0,
                                    1.0, 1.0, 1.0, 1.0, 1.0 }); 
    expected = { 256, 256, 256, 256, 256,
                 256, 256, 256, 256, 256 };
    BOOST_TEST(out == expected, tt::per_element());
}

BOOST_AUTO_TEST_SUITE_END()

