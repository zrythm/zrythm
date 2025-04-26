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
#include "../common/Allocators.h"

#include <stdexcept>
#include <vector>

using namespace RubberBand;

BOOST_AUTO_TEST_SUITE(TestAllocators)

#define COMPARE_ARRAY(a, b)						\
    for (int cmp_i = 0; cmp_i < (int)(sizeof(a)/sizeof(a[0])); ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i] - b[cmp_i], 1e-14);			\
    }

#define COMPARE_N(a, b, n)						\
    for (int cmp_i = 0; cmp_i < n; ++cmp_i) { \
        BOOST_CHECK_SMALL(a[cmp_i] - b[cmp_i], 1e-14);			\
    }

BOOST_AUTO_TEST_CASE(alloc_dealloc)
{
    double *v = allocate<double>(4);
    v[0] = 0.1;
    v[1] = 2.0;
    v[2] = -0.3;
    v[3] = 4.0;
    double *e = allocate<double>(4);
    e[0] = -0.3;
    e[1] = 4.0;
    e[2] = 0.1;
    e[3] = 2.0;
    v_fftshift(v, 4);
    COMPARE_N(v, e, 4);
    deallocate(v);
    deallocate(e);
}

BOOST_AUTO_TEST_CASE(alloc_zero)
{
    double *v = allocate_and_zero<double>(4);
    BOOST_CHECK_EQUAL(v[0], 0.f);
    BOOST_CHECK_EQUAL(v[1], 0.f);
    BOOST_CHECK_EQUAL(v[2], 0.f);
    BOOST_CHECK_EQUAL(v[3], 0.f);
    deallocate(v);
}

BOOST_AUTO_TEST_CASE(alloc_dealloc_channels)
{
    double **v = allocate_channels<double>(2, 4);
    v[0][0] = 0.1;
    v[0][1] = 2.0;
    v[0][2] = -0.3;
    v[0][3] = 4.0;
    v[1][0] = -0.3;
    v[1][1] = 4.0;
    v[1][2] = 0.1;
    v[1][3] = 2.0;
    v_fftshift(v[0], 4);
    COMPARE_N(v[0], v[1], 4);
    deallocate_channels(v, 2);
}

BOOST_AUTO_TEST_CASE(stl)
{
    std::vector<double, StlAllocator<double> > v;
    v.push_back(0.1);
    v.push_back(2.0);
    v.push_back(-0.3);
    v.push_back(4.0);
    double e[] = { -0.3, 4.0, 0.1, 2.0 };
    v_fftshift(v.data(), 4);
    COMPARE_N(v.data(), e, 4);
}

BOOST_AUTO_TEST_SUITE_END()

