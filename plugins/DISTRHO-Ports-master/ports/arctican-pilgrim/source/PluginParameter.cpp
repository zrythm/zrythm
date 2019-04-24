/*
  ==============================================================================

    PluginParameter.cpp
    Created: 26 Mar 2012 10:50:00pm
    Author:  Adam Rogers
 http://www.musicdsp.org/showone.php?id=257
  ==============================================================================
*/

#include "PluginParameter.h"


// out of range checks!
// match smoothed and value when close enough
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ------------------------------------
PluginParameter::PluginParameter()
{
	smootherSpeed = 0.99;
	coef = 1.0 - smootherSpeed;
	previousSample = 0;
}

PluginParameter::~PluginParameter(){}
// ------------------------------------

void	PluginParameter::setValue (float newValue)			{value = newValue;}
float	PluginParameter::getValue ()						{return value;}
float	PluginParameter::getSmoothedValue()					{return smoothedValue;}
void	PluginParameter::setIDNumber(int newIDNumber)		{idNumber = newIDNumber;}
int		PluginParameter::getIDNumber()						{return idNumber;}
void	PluginParameter::setParameterName(String newName)	{parameterName = newName;}
String	PluginParameter::getParameterName()					{return parameterName;}

void	PluginParameter::smooth()
{
	// Do some smoothing using simple low pass filter
	previousSample = (getValue() * coef) + (previousSample * smootherSpeed);

	// Replace to match value
	if (previousSample > 0.99999990) {
		previousSample = 1;
	}
	if (previousSample < 0.00000001) {
		previousSample = 0;
	}

	// set smoothed value to output from filter
	smoothedValue = previousSample;
}

float	PluginParameter::smoothAndGetValue(void)
{
	smooth();
	return	smoothedValue;
}
