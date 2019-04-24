/*
  ==============================================================================

    PluginParameter.h
    Created: 26 Mar 2012 10:50:00pm
    Author:  Adam Rogers

  ==============================================================================
*/

#ifndef __PLUGINPARAMETER_H_785CFC66__
#define __PLUGINPARAMETER_H_785CFC66__

#include "JuceHeader.h"
#include "JucePluginCharacteristics.h"

/**	Class to store parameter details within a plugin.

	This class can store parameter values, such as ID number,
	Name and Value. It can also smooth values to remove the
	'zipper effect' when changing parameters quickly.

	The class will store two parameter values:
	_value_ - this is the value that is used is most cases -
	eg. setting the value, reading the value (host/UI etc).
	_smoothedValue_ - This should be used in the audio processing -
	eg. for gain.. sample = sample * smoothedValue. **/

class PluginParameter
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    : public Component
#endif
{
public:
	PluginParameter();
	~PluginParameter();

/**	Set the value of the parameter.
	This would be the value set by the host/UI etc.
	@param float newValue The value to set. **/
	void	setValue (float newValue);

/**	Get the value of the parameter.
	@return float The unsmoothed value of the parameter. **/
	float	getValue ();

/**	Get the smoothed value of the parameter.
	@return float The smoothed value of the parameter. **/
	float	getSmoothedValue();

/** Smooths the value.
	This operates a low pass filter to smooth the value.
	This should be called periodically to constantly update the value.
	This period should be based on time not samples (to keep consistency
	over different sample rates). A good refresh time would be every
	8 samples at 44100KHz, with the default smoother speed. **/
	void	smooth();

/**	Smooths and returns the value.
	The same as running @PluginParameter::smooth() and @PluginParameter::getSmoothedValue()
	@return float The smoothed value. */
	float	smoothAndGetValue();

/**	Sets the ID number for this parameter.
	@param int newIDNumber The new ID number. */
	void	setIDNumber(int newIDNumber);

/**	Gets the ID number for this parameter.
	@return int The ID number */
	int	getIDNumber();

/**	Sets the name of this parameter.
	@param String newName The new name of the parameter.*/
	void	setParameterName(String newName);

/**	Sets the name of this parameter.
	@return String The parameter name. */
	String	getParameterName();

private:

	// Parameter Variables
	float	value;			// Current value
	float	smoothedValue;		// Current smoothed value
	int	idNumber;		// Parameter ID
	String	parameterName;		// Parameter Name

	// Smoother filter variables
	float smootherSpeed;	// Smoother Speed (defaults to 0.99)
	float coef;		// Used for smoothing
	float previousSample;	// Used for smoothing
};

#endif  // __PLUGINPARAMETER_H_785CFC66__
