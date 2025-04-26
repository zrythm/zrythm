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

#ifndef RUBBERBAND_FIXED_VECTOR_H
#define RUBBERBAND_FIXED_VECTOR_H

#include "Allocators.h"
#include <vector>

namespace RubberBand
{

template <typename T>
class FixedVector : public std::vector<T, StlAllocator<T>>
{
public:
    using std::vector<T, StlAllocator<T>>::vector;
    
private:
    FixedVector(const FixedVector &) =delete;
    FixedVector(FixedVector &&) =delete;
    FixedVector &operator=(const FixedVector &) =delete;
    FixedVector &operator=(FixedVector &&) =delete;
};

}

#endif
