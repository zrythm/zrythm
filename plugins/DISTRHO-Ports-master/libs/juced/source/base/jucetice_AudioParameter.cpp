/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

BEGIN_JUCE_NAMESPACE

#include "jucetice_AudioPlugin.h"
#include "jucetice_AudioParameter.h"
#include "jucetice_MathConstants.h"

//==============================================================================
AudioParameter::AudioParameter ()
: index (0),
partNumber (0),
minValue (0.0f),
maxValue (1.0f),
baseValue (0.0f),
currentValue (0.0f),
#if AudioParameterUseMapping
transferFunction (AudioParameter::RangeLinear),
transferData (0.0f),
#endif
plugin (0)
{
    zeromem (paramName, AudioParameterMaxNameLength * sizeof (char));
    zeromem (paramLabel, AudioParameterMaxLabelLength * sizeof (char));

    getValueFunction = MakeDelegate (this, &AudioParameter::defaultGetFunction);
    setValueFunction = MakeDelegate (this, &AudioParameter::defaultSetFunction);
    getTextFunction = MakeDelegate (this, &AudioParameter::defaultGetTextFunction);
}

AudioParameter::~AudioParameter ()
{
    removeAllListeners ();
}

//==============================================================================
AudioParameter& AudioParameter::id (const int newIndex)
{
    index = newIndex;

    return *this;
}

AudioParameter& AudioParameter::part (const int newPartNumber)
{
    partNumber = newPartNumber;

    return *this;
}

AudioParameter& AudioParameter::name (const String& newName)
{
#if JUCE_WIN32
    _snprintf (paramName, AudioParameterMaxNameLength, "%s", newName.toRawUTF8());
#else
    snprintf (paramName, AudioParameterMaxNameLength, "%s", newName.toRawUTF8());
#endif

    return *this;
}

AudioParameter& AudioParameter::unit (const String& newLabel)
{
#if JUCE_WIN32
    _snprintf (paramLabel, AudioParameterMaxLabelLength, "%s", newLabel.toRawUTF8());
#else
    snprintf (paramLabel, AudioParameterMaxLabelLength, "%s", newLabel.toRawUTF8());
#endif

    return *this;
}

#if AudioParameterUseMapping
AudioParameter& AudioParameter::mapping (TransferFunction newFunction,
                                         const float additionalData)
{
    transferFunction = newFunction;
    transferData = additionalData;

    return *this;
}
#endif

AudioParameter& AudioParameter::range (const float min, const float max)
{
    minValue = min;
    maxValue = max;

    return *this;
}

AudioParameter& AudioParameter::base (const float newBaseValue)
{
    baseValue = newBaseValue;

    return *this;
}

AudioParameter& AudioParameter::set (SetValueDelegate newSetFunction)
{
    setValueFunction = newSetFunction;

    return *this;
}

AudioParameter& AudioParameter::get (GetValueDelegate newGetFunction)
{
    getValueFunction = newGetFunction;

    return *this;
}

AudioParameter& AudioParameter::text (GetTextDelegate newGetTextFunction)
{
    getTextFunction = newGetTextFunction;

    return *this;
}

//==============================================================================
float AudioParameter::getValue () const
{
#if AudioParameterUseMapping
    switch (transferFunction) {
        case RangeLinear:
        default:
            return (getValueFunction (partNumber) - minValue) / (maxValue - minValue);
        case RangeSquared:
            return sqrtf ((getValueFunction (partNumber) - minValue) / (maxValue - minValue));
        case RangeCubed:
            return powf ((getValueFunction (partNumber) - minValue) / (maxValue - minValue), 1.0f / 3.0f);
        case RangePow:
            return powf ((getValueFunction (partNumber) - minValue) / (maxValue - minValue), 1.0f / transferData);
        case RangeExp:
            return logf (1.0f - minValue + getValueFunction (partNumber)) / logf (1.0f - minValue + maxValue);
        case Stepped:
            return getValueFunction (partNumber) / (float) (transferData - 1);
        case Frequency:
            return (logf (getValueFunction (partNumber) / 20.0f) / logf (2.0f))
			/ 9.965784284662088765571752446703612804412841796875f;
    }
#else
    return (getValueFunction (partNumber) - minValue) / (maxValue - minValue);
#endif
}

void AudioParameter::setValue (const float newValue)
{
#if AudioParameterUseMapping
    switch (transferFunction) {
        case RangeLinear:
        default:
            setValueFunction (partNumber, minValue + newValue * (maxValue - minValue));
        case RangeSquared:
            setValueFunction (partNumber, minValue + (newValue * newValue) * (maxValue - minValue));
        case RangeCubed:
            setValueFunction (partNumber, minValue + (newValue * newValue * newValue) * (maxValue - minValue));
        case RangePow:
            setValueFunction (partNumber, minValue + powf (newValue, transferData) * (maxValue - minValue));
        case RangeExp:
            setValueFunction (partNumber, minValue + expf (logf (maxValue - minValue + 1.0f) * newValue) - 1.0f);
        case Stepped:
            setValueFunction (partNumber, (float) roundFloatToInt (newValue * (transferData - 0.01f)));
        case Frequency:
            setValueFunction (partNumber, 20.0f * powf (2.0f, newValue * 9.965784284662088765571752446703612804412841796875f));
    }
#else
    setValueFunction (partNumber, minValue + newValue * (maxValue - minValue));
#endif
}


//==============================================================================
void AudioParameter::addListener (AudioParameterListener* const listener)
{
    listeners.addIfNotAlreadyThere (listener);

    if (plugin)
    {
        const ScopedLock sl (plugin->getParameterLock());
        listener->attachedToParameter (this, index);
    }
    else
    {
        listener->attachedToParameter (this, index);
    }
}

void AudioParameter::removeListener (AudioParameterListener* const listener)
{
    if (plugin)
    {
        const ScopedLock sl (plugin->getParameterLock());
        listener->detachedFromParameter (this, index);
    }
    else
    {
        listener->detachedFromParameter (this, index);
    }

    int index = listeners.indexOf (listener);
    if (index >= 0)
        listeners.remove (index);
}

void AudioParameter::removeAllListeners ()
{
    if (plugin)
    {
        const ScopedLock sl (plugin->getParameterLock());

        for (int i = listeners.size (); --i >= 0;)
            listeners.getUnchecked (i)->detachedFromParameter (this, index);
    }
    else
    {
        for (int i = listeners.size (); --i >= 0;)
            listeners.getUnchecked (i)->detachedFromParameter (this, index);
    }

    listeners.clear ();
}

//==============================================================================
void AudioParameter::setAudioPlugin (AudioPlugin* newPlugin, const int newIndex)
{
    plugin = newPlugin;
    index = newIndex;
}

//==============================================================================
void AudioParameter::handleAsyncUpdate ()
{
    const ScopedLock sl (plugin->getParameterLock());

    for (int i = listeners.size (); --i >= 0;)
        listeners.getUnchecked (i)->parameterChanged (this, index);
}

//==============================================================================
bool AudioParameter::handleMidiMessage (const MidiMessage& message)
{
    // TODO - handle parameters changes by midi notes and not only cc.

    plugin->setParameter (index, message.getControllerValue () * float_MidiScaler);

    return true;
}

//==============================================================================
float AudioParameter::defaultGetFunction (int partNumber)
{
    return currentValue;
}

void AudioParameter::defaultSetFunction (int partNumber, float value)
{
    currentValue = value;
}

const String AudioParameter::defaultGetTextFunction (int partNumber, float value)
{
    int decimalNumber = 0;
    float absValue = fabsf (value);

    if (absValue < 10) decimalNumber = 4;
    else if (absValue < 100) decimalNumber = 3;
    else if (absValue < 1000) decimalNumber = 2;
    else if (absValue < 10000) decimalNumber = 1;
    else if (absValue < 100000) decimalNumber = 0;

    return String (value, jmin (jmax (decimalNumber, 0), 4))
	+ " "
	+ (String)paramLabel;
}



//==============================================================================
juce_ImplementSingleton (AudioParameterThread);

AudioParameterThread::AudioParameterThread ()
: Thread ("AudioParameterThread"),
parameterChanges (8192),
changeChecksPerSecond (50)
{
    startThread (6);
}

AudioParameterThread::~AudioParameterThread ()
{
    stopThread (5000);
}

void AudioParameterThread::sendParameterChange (AudioParameter* parameter)
{
    parameterChanges.put (parameter);

    notify ();
}

int AudioParameterThread::getParametersChangeChecksPerSecond () const
{
    return changeChecksPerSecond;
}

void AudioParameterThread::setParametersChangeChecksPerSecond (const int howManyCheckPerSecond)
{
    changeChecksPerSecond = jmax (1, jmin (100, howManyCheckPerSecond));
}

void AudioParameterThread::run()
{
    AudioParameter* parameter;

    while (! threadShouldExit ())
    {
        int32 currentWaitTime = Time::getMillisecondCounter ();

        while (! parameterChanges.isEmpty ())
        {
            parameter = (AudioParameter*) parameterChanges.get ();
            if (parameter)
                parameter->triggerAsyncUpdate ();
        }

        currentWaitTime = 1000 / changeChecksPerSecond
		- (Time::getMillisecondCounter () - currentWaitTime);

		//        wait (jmax (1, jmin (1000, currentWaitTime)));

        Thread::sleep (jmax (1, jmin (1000, currentWaitTime)));
    }
}


END_JUCE_NAMESPACE

