/*
 *  Parameters.h
 *
 *  Created by David Rowland on 02/02/2009.
 *  Copyright 2009 UWE. All rights reserved.
 *
 */

#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

#ifndef UNUSED_NOWARN

#if defined(JUCE_MAC)
// enable supression of unused variable is GCC
#define UNUSED_NOWARN __attribute__((unused))

#elif defined(JUCE_MSVC)

#define UNUSED_NOWARN

// disable unused variable warnings in MSVC (Windows)
#pragma warning( push )
#pragma warning( disable : 4705 )

#else

#define UNUSED_NOWARN

#endif

#endif //UNUSED_NOWARN

enum parameters
{
	RATE,
	DEPTH,
	FEEDBACK,
	MIX,
	noParams
};

static const char UNUSED_NOWARN *parameterNames[] = { 
	"Rate",					// 0
	"Intensity",					// 1
	"Feedback",						// 2
	"Wet/Dry Mix",						// 3
};

#endif //_PARAMETERS_H_
