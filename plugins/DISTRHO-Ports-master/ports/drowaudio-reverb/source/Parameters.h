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
	PREDELAY,
	ROOMSHAPE,
	EARLYDECAY,
	EARLYLATEMIX,
	FBCOEFF,
	DELTIME,
	FILTERCF,
	DIFFUSION,
	SPREAD,
	LOWEQ,
	HIGHEQ,
	WETDRYMIX,
	noParams
};

static const char UNUSED_NOWARN *parameterNames[] = { 
	"Pre Delay",						
	"Room Shape",
	"Early Decay Time",
	"Early/Late Mix",
	"Rev. Time",						
	"Room Size",						
	"Absorption",						
	"Diffusion",						
	"Stereo Spread",						
	"Low EQ",						
	"High EQ",						
	"Wet Mix"						
};

#endif //_PARAMETERS_H_