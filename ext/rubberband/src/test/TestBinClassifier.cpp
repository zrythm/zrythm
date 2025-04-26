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

// This test suite (shallowly) tests both BinClassifier and BinSegmenter
#include "../finer/BinClassifier.h"
#include "../finer/BinSegmenter.h"

#include "../common/sysutils.h"

using namespace RubberBand;

using std::vector;
using std::string;

namespace tt = boost::test_tools;

// We use the symbols H, X, and _ for harmonic, percussive, and
// residual respectively, because they are easier to distinguish than
// H, P, R

static constexpr auto H = BinClassifier::Classification::Harmonic;
static constexpr auto X = BinClassifier::Classification::Percussive;
static constexpr auto _ = BinClassifier::Classification::Residual;

vector<string> classes_to_strings(const vector<BinClassifier::Classification> &v)
{
    vector<std::string> sv(v.size(), "*");
    for (auto i = 0; i < int(v.size()); ++i) {
        switch (v[i]) {
        case H: sv[i] = "H"; break;
        case X: sv[i] = "X"; break;
        case _: sv[i] = "_"; break;
        }
    }
    return sv;
}

BOOST_AUTO_TEST_SUITE(TestBinClassifier)

BOOST_AUTO_TEST_CASE(classify_bins)
{
    vector<vector<process_t>> magColumns {
        { 0, 8, 1, 1, 0, 1 },
        { 0, 8, 0, 0, 0, 0 },
        { 8, 8, 8, 8, 8, 0 },
        { 0, 7, 0, 1, 0, 0 },
        { 0, 6, 0, 0, 0, 0 },
        { 0, 8, 0, 9, 9, 9 },
        { 0, 7, 0, 0, 1, 0 }
    };

    vector<vector<BinClassifier::Classification>> classifications(7, { 6, _ });

    BinClassifier::Parameters params(6, 3, /* lag */ 1, 3, 2.0, 2.0);
    BinClassifier classifier(params);

    for (int i = 0; i < 7; ++i) {
        classifier.classify(magColumns[i].data(), classifications[i].data());
    }

    /*
       The lag of 1 specified for the horizontal filter means that the
       results are delayed by a column (here row) but the vertical
       filter outputs are aligned with the middle of the 3-bin
       horizontal filters rather than the end.
    
       So the horizontal filter outputs (filtering vertically as
       presented here) are
           0 8 1 1 0 1 <- This is the "lag" column that is not meaningful
           0 8 0 0 0 0 <- This is the actual median for the first col (row)
           0 8 1 1 0 0 
           0 8 0 1 0 0 
           0 7 0 1 0 0 
           0 7 0 1 0 0 
           0 7 0 0 1 0 

       And the vertical ones (lagged by one column to match the
       horizontal filter outputs) are
           0 0 0 0 0 0 <- The "lag" column (here row)
           0 1 1 1 1 0 <- The effective first column (row)
           0 0 0 0 0 0 
           8 8 8 8 8 0 
           0 0 1 0 0 0 
           0 0 0 0 0 0 
           0 0 8 9 9 9

       We have harmonic, percussive, and residual bins. (Initially we
       detected silent bins too, but of course if done naively that
       doesn't align with the lagged filter output, and silent bins
       didn't appear relevant enough to take extra trouble over.) In
       our case, wherever both horizontal and vertical filter outputs
       are the same-ish (0, 1, or one of 7/8/9) we expect to see a
       residual classification. Otherwise we expect harmonic if the
       horizontal output is greater, percussive otherwise.
    */
    
    vector<vector<BinClassifier::Classification>> expected {
        // These results are lagged by one relative to the input
        { _, H, H, H, _, H },
        { _, H, X, X, X, _ },
        { _, H, H, H, _, _ },
        { X, _, X, X, X, _ },
        { _, H, X, H, _, _ },
        { _, H, _, H, _, _ },
        { _, H, X, X, X, X }
    };

    for (int i = 0; i < 7; ++i) {
        BOOST_TEST(classes_to_strings(classifications[i]) ==
                   classes_to_strings(expected[i]),
                   tt::per_element());
    }
}

BOOST_AUTO_TEST_CASE(segment_classification)
{
    vector<vector<BinClassifier::Classification>> classification {
        { _, H, X, X, X, _ },
        { _, H, H, H, _, _ },
        { X, _, X, X, X, _ },
        { _, H, X, H, _, _ },
        { X, X, _, H, _, _ },
        { _, H, X, X, X, X },
        { _, H, _, _, _, _ }
    };

    BinSegmenter::Parameters params(16, 6, 48000, 3);
    BinSegmenter segmenter(params);

    vector<BinSegmenter::Segmentation> segmented;
    for (int i = 0; i < 7; ++i) {
        segmented.push_back(segmenter.segment(classification[i].data()));
    }

    /* 
       Modal filter length 3 was specified, with the ordering for
       resolving equal counts as H, X, _. So the filtered
       classifications will be:

           H H X X X X
           H H H H _ _
           X X X X X X
           H H H H _ _
           X X H _ _ _
           H H X X X X
           H _ _ _ _ _
    */
       
    vector<BinSegmenter::Segmentation> expected {
        {    0.0,  3000.0, 15000.0 },
        {    0.0,  9000.0,  9000.0 }, // Though any equal values would do!
        {    0.0,     0.0, 15000.0 },
        {    0.0,  9000.0,  9000.0 },
        { 6000.0,  6000.0,  6000.0 }, // Similarly
        {    0.0,  3000.0, 15000.0 },
        {    0.0, 24000.0, 24000.0 }
    };

    for (int i = 0; i < 7; ++i) {
        BOOST_TEST(segmented[i].percussiveBelow == expected[i].percussiveBelow);
        BOOST_TEST(segmented[i].percussiveAbove == expected[i].percussiveAbove);
        BOOST_TEST(segmented[i].residualAbove == expected[i].residualAbove);
    }
}

BOOST_AUTO_TEST_SUITE_END()


