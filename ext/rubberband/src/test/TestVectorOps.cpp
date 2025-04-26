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

#include "../common/VectorOps.h"

#include <stdexcept>
#include <vector>

using namespace RubberBand;

BOOST_AUTO_TEST_SUITE(TestVectorOps)

#define COMPARE_ARRAY(a, b)						\
    for (int cmp_i = 0; cmp_i < (int)(sizeof(a)/sizeof(a[0])); ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i] - b[cmp_i], 1e-14);			\
    }

#define COMPARE_N(a, b, n)						\
    for (int cmp_i = 0; cmp_i < n; ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i] - b[cmp_i], 1e-14);			\
    }

BOOST_AUTO_TEST_CASE(add)
{
    double a[] = { 1.0, 2.0, 3.0 };
    double b[] = { -1.0, 3.0, -4.5 };
    double expected[] = { 0.0, 5.0, -1.5 };
    v_add(a, b, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(add_with_gain)
{
    double a[] = { 1.0, 2.0, 3.0 };
    double b[] = { -1.0, 3.0, -4.5 };
    double expected[] = { -0.5, 6.5, -3.75 };
    v_add_with_gain(a, b, 1.5, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(subtract)
{
    double a[] = { 1.0, 2.0, 3.0 };
    double b[] = { -1.0, 3.0, -4.5 };
    double expected[] = { 2.0, -1.0, 7.5 };
    v_subtract(a, b, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(scale)
{
    double a[] = { -1.0, 3.0, -4.5 };
    double scale = -0.5;
    double expected[] = { 0.5, -1.5, 2.25 };
    v_scale(a, scale, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(multiply)
{
    double a[] = { 1.0, 2.0, 3.0 };
    double b[] = { -1.0, 3.0, -4.5 };
    double expected[] = { -1.0, 6.0, -13.5 };
    v_multiply(a, b, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(multiply_and_add)
{
    double a[] = { 1.0, 2.0, 3.0 };
    double b[] = { -1.0, 3.0, -4.5 };
    double c[] = { 3.0, -1.0, 4.0 };
    double expected[] = { 2.0, 5.0, -9.5 };
    v_multiply_and_add(c, a, b, 3);
    COMPARE_N(c, expected, 3);
}

BOOST_AUTO_TEST_CASE(divide)
{
    double a[] = { 1.0, 2.0, 3.0 };
    double b[] = { -1.0, 3.0, -4.5 };
    double expected[] = { -1.0, 2.0/3.0, 3.0/-4.5 };
    v_divide(a, b, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(sum)
{
    double a[] = { 1.0, 2.0, -3.5 };
    double s = v_sum(a, 3);
    BOOST_CHECK_EQUAL(s, -0.5);
}

BOOST_AUTO_TEST_CASE(multiply_and_sum)
{
    double a[] = { 2.0, 0.0, -1.5 };
    double b[] = { 3.0, 4.0, 5.0 };
    double s = v_multiply_and_sum(a, b, 3);
    BOOST_CHECK_EQUAL(s, -1.5);
}

BOOST_AUTO_TEST_CASE(log)
{
    double a[] = { 1.0, 1.0 / M_E, M_E };
    double expected[] = { 0.0, -1.0, 1.0 };
    v_log(a, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(exp)
{
    double a[] = { 0.0, -1.0, 2.0 };
    double expected[] = { 1.0, 1.0 / M_E, M_E * M_E };
    v_exp(a, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(sqrt)
{
    double a[] = { 0.0, 1.0, 4.0 };
    double expected[] = { 0.0, 1.0, 2.0 };
    v_sqrt(a, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(square)
{
    double a[] = { 0.0, 1.5, -2.0 };
    double expected[] = { 0.0, 2.25, 4.0 };
    v_square(a, 3);
    COMPARE_N(a, expected, 3);
}

BOOST_AUTO_TEST_CASE(abs)
{
    double a[] = { -1.9, 0.0, 0.01, -0.0 };
    double expected[] = { 1.9, 0.0, 0.01, 0.0 };
    v_abs(a, 4);
    COMPARE_N(a, expected, 4);
}

BOOST_AUTO_TEST_CASE(mean)
{
    double a[] = { -1.0, 1.6, 3.0 };
    double s = v_mean(a, 3);
    BOOST_CHECK_EQUAL(s, 1.2);
}

BOOST_AUTO_TEST_CASE(interleave_1)
{
    double a[] = { 1.0, 2.0, 3.0 };
    double *ch[] = { a };
    double o[3];
    double expected[] = { 1.0, 2.0, 3.0 };
    v_interleave(o, ch, 1, 3);
    COMPARE_N(o, expected, 3);
}

BOOST_AUTO_TEST_CASE(interleave_2)
{
    double a[] = { 1.0, 2.0, 3.0 };
    double b[] = { 4.0, 5.0, 6.0 };
    double *ch[] = { a, b };
    double o[6];
    double expected[] = { 1.0, 4.0, 2.0, 5.0, 3.0, 6.0 };
    v_interleave(o, ch, 2, 3);
    COMPARE_N(o, expected, 6);
}

BOOST_AUTO_TEST_CASE(interleave_3)
{
    double a[] = { 1.0, 2.0 };
    double b[] = { 3.0, 4.0 };
    double c[] = { 5.0, 6.0 };
    double *ch[] = { a, b, c };
    double o[6];
    double expected[] = { 1.0, 3.0, 5.0, 2.0, 4.0, 6.0 };
    v_interleave(o, ch, 3, 2);
    COMPARE_N(o, expected, 6);
}

BOOST_AUTO_TEST_CASE(deinterleave_1)
{
    double a[] = { 1.0, 2.0, 3.0 };
    double o[3];
    double *oo[] = { o };
    double *expected[] = { a };
    v_deinterleave(oo, a, 1, 3);
    COMPARE_N(oo[0], expected[0], 3);
}

BOOST_AUTO_TEST_CASE(deinterleave_2)
{
    double a[] = { 1.0, 4.0, 2.0, 5.0, 3.0, 6.0 };
    double o1[3], o2[3];
    double *oo[] = { o1, o2 };
    double e1[] = { 1.0, 2.0, 3.0 }, e2[] = { 4.0, 5.0, 6.0 };
    double *expected[] = { e1, e2 };
    v_deinterleave(oo, a, 2, 3);
    COMPARE_N(oo[0], expected[0], 3);
    COMPARE_N(oo[1], expected[1], 3);
}

BOOST_AUTO_TEST_CASE(deinterleave_3)
{
    double a[] = { 1.0, 3.0, 5.0, 2.0, 4.0, 6.0 };
    double o1[2], o2[2], o3[2];
    double *oo[] = { o1, o2, o3 };
    double e1[] = { 1.0, 2.0 }, e2[] = { 3.0, 4.0 }, e3[] = { 5.0, 6.0 };
    double *expected[] = { e1, e2, e3 };
    v_deinterleave(oo, a, 3, 2);
    COMPARE_N(oo[0], expected[0], 2);
    COMPARE_N(oo[1], expected[1], 2);
    COMPARE_N(oo[2], expected[2], 2);
}

BOOST_AUTO_TEST_CASE(fftshift)
{
    double a[] = { 0.1, 2.0, -0.3, 4.0 };
    double e[] = { -0.3, 4.0, 0.1, 2.0 };
    v_fftshift(a, 4);
    COMPARE_N(a, e, 4);
}

BOOST_AUTO_TEST_SUITE_END()

