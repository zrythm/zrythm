/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/

#ifndef __DROWAUDIO_CONSTANTS_H__
#define __DROWAUDIO_CONSTANTS_H__

//==============================================================================
/** @file 
 
	This file contains some useful constants for calculations such as reciprocals
	to avoid using expensive divides in programs.
 */

static const double oneOver60 = 1.0 / 60.0;
static const double oneOver60Squared = 1.0 / (60.0 * 60.0);
static const double oneOver180 = 1.0 / 180.0;
static const double oneOverPi = 1.0 / double_Pi;
static const double twoTimesPi = 2.0 * double_Pi;
static const double fourTimesPi = 4.0 * double_Pi;
static const double sixTimesPi = 6.0 * double_Pi;
static const double root2 = sqrt (2.0);
static const double oneOverRoot2 = 1.0 / sqrt (2.0);
static const double root3 = sqrt (3.0);
static const double oneOverRoot3 = 1.0 / sqrt (3.0);

template <typename Type>
inline Type squareNumber (Type input)
{
    return input * input;
}

template <typename Type>
inline Type cubeNumber (Type input)
{
    return input * input * input;
}

#if JUCE_WINDOWS
template <class Type>
inline Type log2 (Type input)
{
    return log (input) / log ((Type) 2);
}
#endif

#endif //__DROWAUDIO_CONSTANTS_H__
