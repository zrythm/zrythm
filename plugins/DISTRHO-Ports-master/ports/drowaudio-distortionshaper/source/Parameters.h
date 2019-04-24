/*
 *  Parameters.h
 *
 *  Created by David Rowland on 02/02/2009.
 *  Copyright 2009 UWE. All rights reserved.
 *
 */

#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

enum parameters
{
	INGAIN,
	OUTGAIN,
	PREFILTER,
	POSTFILTER,
	X1,
	Y1,
	X2,
	Y2,
	noParams
};

static const char UNUSED_NOWARN *parameterNames[] = { 
	"Input Gain",						
	"Output",
	"Pre Filter",
	"Post Filter",
	"x1",
	"y1",
	"x2",						
	"y2"						
};

#endif //_PARAMETERS_H_