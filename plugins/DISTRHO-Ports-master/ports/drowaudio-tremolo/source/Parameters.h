/*
  ==============================================================================

    Common.h
    Created: 10 Jun 2012 7:30:05pm
    Author:  David Rowland

  ==============================================================================
*/

#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

#include "includes.h"

/* Our list of parameters and the names that will be displayed for them.
 */
namespace Parameters
{
    enum Parameters
    {
        rate = 0,
        depth,
        shape,
        phase,
        numParameters
    };

    static const char* UNUSED_NOWARN names[numParameters] =
    {
        "Rate",
        "Depth",
        "Shape",
        "Phase"
    };

    static const ParameterUnit UNUSED_NOWARN units[numParameters] =
    {
        UnitHertz,
        UnitPercent,
        UnitGeneric,
        UnitDegrees
    };

    static const char* UNUSED_NOWARN descriptions[numParameters] =
    {
        "The rate of the effect",
        "The depth of the effect",
        "The shape of the tremolo effect",
        "The level of offset of the second channel"
    };

    static const double UNUSED_NOWARN mins[numParameters] =
    {
        0.0,
        0.0,
        0.2,
        -180.0
    };

    static const double UNUSED_NOWARN maxs[numParameters] =
    {
        20.0,
        100.0,
        10.0,
        180.0
    };

    static const double UNUSED_NOWARN defaults[numParameters] =
    {
        5.0,
        100.0,
        1.0,
        0.0,
    };
}


#endif  // _PARAMETERS_H_
