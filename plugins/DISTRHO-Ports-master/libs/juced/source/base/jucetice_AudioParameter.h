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

#ifndef __JUCETICE_AUDIOPARAMETER_HEADER__
#define __JUCETICE_AUDIOPARAMETER_HEADER__

#include "../utils/jucetice_FastDelegate.h"
#include "../containers/jucetice_LockFreeFifo.h"
#include "../audio/midi/jucetice_MidiAutomatorManager.h"

#define AudioParameterUseMapping     0
#define AudioParameterMaxNameLength  32
#define AudioParameterMaxLabelLength 16

class AudioPlugin;
class AudioParameter;
class AudioParameterThread;
class AudioProgram;


//==============================================================================
/**
    A class for receiving callbacks from the Parameters.

    To be told when a slider's value changes, you can register a
    ParameterManagerListener object using Parameter::addListener()

    @see 
 */
class AudioParameterListener
{
public:

    virtual ~AudioParameterListener() {}

    //==============================================================================
    /**
        Any listener should override this method to be notified when a new
        parameter changes. The index is the current Parameter index as stored
        in the ParameterManager.

        @param index              current Parameter index in the ParameterManager
        @param parameter          the real parameter object that is changing
     */
    virtual void parameterChanged (AudioParameter* parameter,
                                   const int parameterIndex) = 0;

    //==============================================================================
    /**
        Any listener should override this method if they need to be notified
        when they are attached to a new parameter

        @param index              current Parameter index in the ParameterManager
        @param parameter          the real parameter object we are attaching to
     */
    virtual void attachedToParameter (AudioParameter* parameter,
                                      const int parameterIndex) { }

    //==============================================================================
    /**
        Any listener should override this method if they need to be notified
        when they are detached to a new parameter

        @param index              current Parameter index in the ParameterManager
        @param parameter          the real parameter object we are attaching to
     */
    virtual void detachedFromParameter (AudioParameter* parameter,
                                        const int parameterIndex) { }

protected:

    AudioParameterListener () {}
};


//==============================================================================
/**
    The parameter updating thread

    It is a shared global static class that starts a global thread and updates
    asyncronously the parameters and their attached listeners 

*/
class AudioParameterThread : public Thread,
                             public DeletedAtShutdown
{
public:

    //==============================================================================
    /** Constructor */
    AudioParameterThread ();

    /** Destructor */
    ~AudioParameterThread ();

    //==============================================================================
    /**
        Set the current number of parameter update check per second
     */
    void sendParameterChange (AudioParameter* parameter);

    //==============================================================================
    /**
        Set the current number of parameter update check per second
     */
    int getParametersChangeChecksPerSecond () const;

    /**
        Set the current number of parameter update check per second
     */
    void setParametersChangeChecksPerSecond (const int howManyCheckPerSecond);

    //==============================================================================
    /**
        The thread callback. Here we check for changes to parameters and trigger
        async updates to  true if the specified output channel is part of a stereo pair

        @see Thread
    */
    void run();

    //==============================================================================
    juce_DeclareSingleton (AudioParameterThread, true)
    
private:

    LockFreeFifo<AudioParameter*> parameterChanges;
    int changeChecksPerSecond;    
};


//==============================================================================
/**
    This is a parameter mangler, which holds the real mangler and a set of
    functions to manipulate the real parameter.
*/
class AudioParameter : public AsyncUpdater,
                       public MidiAutomatable
{
public:

    //==============================================================================
#if AudioParameterUseMapping
    enum TransferFunction
    {
        RangeLinear = 0,
        RangeSquared,
        RangeCubed,
        RangePow,
        RangeExp,
        Stepped,
        RangeInt,
        Frequency
    };
#endif

    //==============================================================================
    /** Get and Set delegates */
    typedef FastDelegate1<int, float> GetValueDelegate;
    typedef FastDelegate2<int, float, void> SetValueDelegate;
    typedef FastDelegate2<int, float, const String> GetTextDelegate;

    //==============================================================================
    /** Constructor */
    AudioParameter ();

    /** Destructor */   
    ~AudioParameter ();

    //==============================================================================
    /**
        Set the internal identifier for the mangler

        This should follow the global index position where the parameter is stored
        inside the ParameterManager.
    */
    AudioParameter& id (const int newIndex);

    /** Set the internal part number */
    AudioParameter& part (const int newPartNumber);

    /** Set the parameter name */
    AudioParameter& name (const String& newName);

    /** Set the internal unit label */
    AudioParameter& unit (const String& newLabel);

#if AudioParameterUseMapping
    /** Set the internal unit label */
    AudioParameter& mapping (TransferFunction newMapping, 
                             const float additionalData = 0.0f);
#endif

    /** Set the internal range for the parameter */
    AudioParameter& range (const float min, const float max);

    /** Set the default value for the parameter (in real min..max) */
    AudioParameter& base (const float newBaseValue);
    
    /** Set the fast delegate to be used as setter callback */
    AudioParameter& set (SetValueDelegate newSetFunction);

    /** Set the fast delegate to be used as getter callback */
    AudioParameter& get (GetValueDelegate newGetFunction);

    /** Set the fast delegate to be used as get text callback */
    AudioParameter& text (GetTextDelegate newGetTextFunction);

    //==============================================================================
    /** */
    int getIndex () const                           { return index; }

    //==============================================================================
    /** */
    const char* getName () const                    { return &paramName [0]; }

    //==============================================================================
    /** */
    const char* getLabel () const                   { return &paramLabel [0]; }

    //==============================================================================
    /** Get the default parameter value in 0..1 */
    float getBaseValue () const
    {
        return (baseValue - minValue) / (maxValue - minValue);
    }

    /** Get the default parameter value mapped to min..max */
    float getBaseValueReal () const
    {
        return baseValue;
    }

    //==============================================================================
    /** Get parameter value in 0..1 */
    float getValue () const;

    /** Get parameter value in 0..1 integer format */
    int getIntValue () const
    {
        return roundFloatToInt (getValue ());
    }

    /** Get mapped parameter value in min..max */
    float getValueMapped () const
    {
        return getValueFunction (partNumber);
    }

    /** Get mapped parameter value in min..max integer format */
    int getIntValueMapped () const
    {
        return roundFloatToInt (getValueMapped ());
    }

    //==============================================================================
    /** Set parameter value in 0..1 */
    void setValue (const float newValue);

    /** Set parameter value mapped to min..max */
    void setValueMapped (const float newValue)
    {
        setValueFunction (partNumber, jmax (minValue, jmin (maxValue, newValue)));
    }

    /** Set normalised parameter value in min..max */
    void setValueMapped (const int newValue)
    {
        setValueFunction (partNumber, jmax (minValue, jmin (maxValue, (float) newValue)));
    }

    //==============================================================================
    /**
        Return the current value representation 0..1 in text
    */
    String getValueAsString ()
    {
        return getTextFunction (partNumber, getValue ());
    }

    /**
        Return the current value representation mapped to min..max in text
    */
    String getValueMappedAsString ()
    {
        return getTextFunction (partNumber, getValueMapped ());
    }

    //==============================================================================
    /**
        Register a listener with this parameter manager.
     */
    void addListener (AudioParameterListener* const listener);

    /**
        Remove a previously added listener
    */
    void removeListener (AudioParameterListener* const listener);

    /**
        Remove all previously added listeners
    */
    void removeAllListeners ();

    //==============================================================================
    /** @internal */
    void setAudioPlugin (AudioPlugin* newPlugin, const int newIndex);
    /** @internal */
    void handleAsyncUpdate ();
    /** @internal */
    bool handleMidiMessage (const MidiMessage& message);

protected:

    //==============================================================================
    /** @internal */
    float defaultGetFunction (int partNumber);
    /** @internal */
    void defaultSetFunction (int partNumber, float value);
    /** @internal */
    const String defaultGetTextFunction (int partNumber, float value);

    int index;
    char paramName[AudioParameterMaxNameLength];
    char paramLabel[AudioParameterMaxLabelLength];
    int partNumber;
    float minValue, maxValue;
    float baseValue;
    float currentValue;
#if AudioParameterUseMapping
    TransferFunction transferFunction;
    float transferData;
#endif

    GetValueDelegate getValueFunction;
    SetValueDelegate setValueFunction;
    GetTextDelegate getTextFunction;

    AudioPlugin* plugin;
    Array<AudioParameterListener*> listeners;
};

#endif
