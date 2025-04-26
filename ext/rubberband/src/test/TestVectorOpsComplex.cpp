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

#include "../common/VectorOpsComplex.h"
#include "../common/VectorOps.h"

#include <stdexcept>
#include <vector>
#include <iostream>

using namespace RubberBand;

BOOST_AUTO_TEST_SUITE(TestVectorOpsComplex)

#ifdef USE_APPROXIMATE_ATAN2
static const double eps = 5.0e-3;
static const double eps_approx = eps;
#else
static const double eps = 1.0e-14;
static const double eps_approx = 1.0e-8;
#endif
    
#define COMPARE_N(a, b, n) \
    for (int cmp_i = 0; cmp_i < n; ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i] - b[cmp_i], eps); \
    }

BOOST_AUTO_TEST_CASE(cartesian_to_magnitudes)
{
    double re[] = { 1.0, 3.0 };
    double im[] = { 2.0, -4.0 };
    double o[2];
    double expected[] = { sqrt(5.0), 5.0 };
    v_cartesian_to_magnitudes(o, re, im, 2);
    COMPARE_N(o, expected, 2);
}

BOOST_AUTO_TEST_CASE(cartesian_interleaved_to_magnitudes)
{
    double a[] = { 1.0, 2.0, 3.0, -4.0 };
    double o[2];
    double expected[] = { sqrt(5.0), 5.0 };
    v_cartesian_interleaved_to_magnitudes(o, a, 2);
    COMPARE_N(o, expected, 2);
}

BOOST_AUTO_TEST_CASE(cartesian_to_polar)
{
    double re[] = { 0.0, 1.0, 0.0 };
    double im[] = { 0.0, 1.0, -1.0 };
    double mo[3], po[3];
    double me[] = { 0.0, sqrt(2.0), 1.0 };
    double pe[] = { 0.0, M_PI / 4.0, -M_PI * 0.5 };
    v_cartesian_to_polar(mo, po, re, im, 3);
    COMPARE_N(mo, me, 3);
    COMPARE_N(po, pe, 3);
}

BOOST_AUTO_TEST_CASE(cartesian_to_polar_interleaved_inplace)
{
    double a[] = { 0.0, 0.0, 1.0, 1.0, 0.0, -1.0 };
    double e[] = { 0.0, 0.0, sqrt(2.0), M_PI / 4.0, 1.0, -M_PI * 0.5 };
    v_cartesian_to_polar_interleaved_inplace(a, 3);
    COMPARE_N(a, e, 6);
}

BOOST_AUTO_TEST_CASE(cartesian_interleaved_to_polar)
{
    double a[] = { 0.0, 0.0, 1.0, 1.0, 0.0, -1.0 };
    double mo[3], po[3];
    double me[] = { 0.0, sqrt(2.0), 1.0 };
    double pe[] = { 0.0, M_PI / 4.0, -M_PI * 0.5 };
    v_cartesian_interleaved_to_polar(mo, po, a, 3);
    COMPARE_N(mo, me, 3);
    COMPARE_N(po, pe, 3);
}

BOOST_AUTO_TEST_CASE(polar_to_cartesian)
{
    double m[] = { 0.0, sqrt(2.0), 1.0 };
    double p[] = { 0.0, M_PI / 4.0, -M_PI * 0.5 };
    double ro[3], io[3];
    double re[] = { 0.0, 1.0, 0.0 };
    double ie[] = { 0.0, 1.0, -1.0 };
    v_polar_to_cartesian(ro, io, m, p, 3);
    COMPARE_N(ro, re, 3);
    COMPARE_N(io, ie, 3);
}

BOOST_AUTO_TEST_CASE(polar_to_cartesian_interleaved_inplace)
{
    double a[] = { 0.0, 0.0, sqrt(2.0), M_PI / 4.0, 1.0, -M_PI * 0.5 };
    double e[] = { 0.0, 0.0, 1.0, 1.0, 0.0, -1.0 };
    v_polar_interleaved_to_cartesian_inplace(a, 3);
    COMPARE_N(a, e, 6);
}

BOOST_AUTO_TEST_CASE(polar_to_cartesian_interleaved)
{
    double m[] = { 0.0, sqrt(2.0), 1.0 };
    double p[] = { 0.0, M_PI / 4.0, -M_PI * 0.5 };
    double o[6];
    double e[] = { 0.0, 0.0, 1.0, 1.0, 0.0, -1.0 };
    v_polar_to_cartesian_interleaved(o, m, p, 3);
    COMPARE_N(o, e, 6);
}

BOOST_AUTO_TEST_SUITE_END()

