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

#ifndef __JUCETICE_AUDIOPLUGIN_HEADER__
#define __JUCETICE_AUDIOPLUGIN_HEADER__

#include "jucetice_AudioParameter.h"
#include "jucetice_AudioProgram.h"
#include "../containers/jucetice_LockFreeFifo.h"

//==============================================================================
/**
    The default jucetice plugin base.

    It holds a set of parameters and let you register and attach listeners to
    some of them separately or register a listener to the whole set in a glance.

    You can't instantiate one of this directly, instead you have to derive
    your own plugin class and register at least a couple of parameters.

*/
class AudioPlugin : public AudioProcessor,
                    public ChangeBroadcaster
{
public:

    //==============================================================================
    /** Constructor */
    AudioPlugin()
        : currentProgram (0)
    {
        parameterThread = AudioParameterThread::getInstance ();
    }

    /** Destructor */
    ~AudioPlugin() override
    {
        removeAllParameters ();

        parameterThread = 0;
    }

    //==============================================================================
    const String getName() const override
    {
        return "AudioPlugin";
    }

    bool acceptsMidi() const override
    {
        return false;
    }

    bool producesMidi() const override
    {
        return false;
    }

    //==============================================================================
    /**
        Sets the current size of the parameters

        This has to be done at least once before registering parameters in the
        audio plugin. It will allocate space for every parameter pointer

        @see AudioProcessor
    */
    void setNumParameters (int numParameters)
    {
        parameters.insertMultiple (0, 0, numParameters);
    }

    /**
        Add a new parameter to the list of the parameter

        You should call setNumParameters at least once before you start adding
        parameters to the plugin. We do this in order to make it possible to
        register only some parameters, while keeping the total amount of
        parameters fixed (and so you don't have problem restoring presets, unless
        you change the total number of parameters and their order at the same time)

        @see removeParameter
    */
    void registerParameter (const int index, AudioParameter* parameter)
    {
        // You must call setNumParameters before registering a parameter !
        jassert (parameters.size () != 0);

        if (parameter)
        {
            parameter->setAudioPlugin (this, index);

            midiAutomatorManager.registerMidiAutomatable (parameter);
        }

        parameters.set (index, parameter);
    }

    /**
        Remove parameter at the specified index

        @see registerParameter
    */
    void removeParameter (const int index, const bool deleteObject = false)
    {
        AudioParameter* parameter = parameters.getUnchecked (index);

        if (parameter)
        {
            midiAutomatorManager.removeMidiAutomatable (parameter);

            if (deleteObject)
                delete parameter;
        }

        parameters.set (index, 0);
    }

    /**
        Remove a parameter pointer if it is present in our list

        @see registerParameter
    */
    void removeParameter (AudioParameter* parameter, const bool deleteObject = false)
    {
        const int index = parameters.indexOf (parameter);

        if (parameter)
        {
            midiAutomatorManager.removeMidiAutomatable (parameter);

            if (deleteObject)
                delete parameter;
        }

        parameters.set (index, 0);
    }

    /**
        Remove a parameter pointer if it is present in our list

        @see registerParameter
    */
    void removeAllParameters (const bool deleteObjects = false)
    {
        for (int i = 0; i < parameters.size (); i++)
        {
            AudioParameter* parameter = parameters.getUnchecked (i);
            if (parameter)
            {
                midiAutomatorManager.removeMidiAutomatable (parameter);

                if (deleteObjects)
                    delete parameter;
            }
        }

        parameters.clear ();
    }

    /**
        Returns a parameter object

        @see registerParameter
    */
    AudioParameter* getParameterObject (const int index)
    {
        return parameters.getUnchecked (index);
    }

    //==============================================================================
    /**
        Returns the actual number of parameters.

        This is used internally to fill up the real plugin, that in some case
        they require to know beforehand the total number of parameter.

        @see AudioProcessor
    */
    int getNumParameters() override
    {
        return parameters.size ();
    }

    /**
        Returns the actual parameter value, in normalized 0..1 format

        @param parameterIndex      the actual parameter index

        @see AudioProcessor
    */
    float getParameter (int index) override
    {
        AudioParameter* parameter = parameters [index];

        return parameter ? parameter->getValue ()
                         : 0.0f;
    }

    /**
        Returns the actual parameter value, in normalized 0..1 format

        @param parameterIndex      the actual parameter index
    */
    float getParameterMapped (int index)
    {
        AudioParameter* parameter = parameters [index];

        return parameter ? parameter->getValueMapped ()
                         : 0.0f;
    }

    /**
        Returns the actual parameter value, in integer format

        @param parameterIndex      the actual parameter index
    */
    int getIntParameterMapped (int index)
    {
        AudioParameter* parameter = parameters [index];

        return parameter ? parameter->getIntValueMapped ()
                         : 0;
    }

    /**
        Change a parameter internal value in 0..1 format

        @param parameterIndex      the actual parameter index
        @param newValue            new parameter value in 0..1 format

        @see AudioProcessor
    */
    void setParameter (int index, float newValue) override
    {
        AudioParameter* parameter = parameters [index];

        if (parameter)
        {
            parameter->setValue (newValue);
            parameterThread->sendParameterChange (parameter);
        }
    }

    /**
        Change a parameter internal value in 0..1 format

        @param parameterIndex      the actual parameter index
        @param newValue            new parameter value in 0..1 format
    */
    void setParameterMapped (int index, float newValue)
    {
        AudioParameter* parameter = parameters [index];

        if (parameter)
        {
            parameter->setValueMapped (newValue);
            parameterThread->sendParameterChange (parameter);
        }
    }

    /**
        Change a parameter internal value in real format

        This will notify the host of the changes (accordingly scaling parameter
        to the host 0..1 language)

        @param index        the actual parameter index
        @param newValue     new parameter value in min..max format
    */
    void setParameterMappedNotifyingHost (int index, float newValue)
    {
        setParameterMapped (index, newValue);
        sendParamChangeMessageToListeners (index, getParameter (index));
    }

    //==============================================================================
    /**
        Get the parameter name

        @param index        the actual parameter index

        @see AudioProcessor
    */
    const String getParameterName (int index) override
    {
        AudioParameter* parameter = parameters [index];

        return parameter ? String (parameter->getName ())
                         : String();
    }

    /**
        This returns the parameter value representation in text format

        @param index        the actual parameter index

        @see AudioProcessor
    */
    const String getParameterText (int index) override
    {
        AudioParameter* parameter = parameters [index];

        return parameter ? parameter->getValueMappedAsString ()
                         : String();
    }

    //==============================================================================
    /**
        This returns the parameter midi CC number

        @param index        the actual parameter index

        @see AudioParameter, MidiAutomatable
    */
    int getParameterControllerNumber (int index) const
    {
        AudioParameter* parameter = parameters [index];

        return parameter ? parameter->getControllerNumber ()
                         : -1;
    }

    /**
        This returns the parameter midi CC number

        @param index        the actual parameter index

        @see AudioParameter, MidiAutomatable
    */
    void setParameterControllerNumber (int index, int newControlChange)
    {
        AudioParameter* parameter = parameters [index];

        if (parameter)
            parameter->setControllerNumber (newControlChange);
    }


    //==============================================================================
    /**
        Returns the current lock on the parameters

        This is usually and often called in GUIs, locking the parameter manager
        then u need to add or remove listeners to them.

        @see addListenerToParameter, removeListenerToParameter
    */
    const CriticalSection& getParameterLock()
    {
        return parameterLock;
    }

    /**
        Add a new ParameterListener listening to a parameter change

        @param index        index of the parameter
        @param listener     ParameterListener to register

        @see ParameterManager
    */
    void addListenerToParameter (const int index,
                                 AudioParameterListener* const listener)
    {
        AudioParameter* parameter = parameters [index];

        if (parameter)
            parameter->addListener (listener);
    }

    /**
        Add a new ParameterListener listening to all parameters

        @param listener     ParameterListener that will listen to all parameter

        @see AudioParameter, AudioParameterListener
    */
    void addListenerToParameters (AudioParameterListener* const listener)
    {
        for (int i = parameters.size (); --i >= 0;)
        {
            addListenerToParameter (i, listener);
        }
    }

    /**
        Remove a ParameterListener listening to a parameter change

        @param index        index of the parameter
        @param listener     ParameterListener to unregister

        @see AudioParameter, AudioParameterListener
    */
    void removeListenerToParameter (const int index,
                                    AudioParameterListener* const listener)
    {
        AudioParameter* parameter = parameters [index];

        if (parameter)
            parameter->removeListener (listener);
    }

    /**
        Remove a ParameterListener listening to all parameters

        @param listener     ParameterListener to unregister

        @see AudioParameter, AudioParameterListener
    */
    void removeListenerToParameters (AudioParameterListener* listener)
    {
        for (int i = parameters.size (); --i >= 0;)
        {
            removeListenerToParameter (i, listener);
        }
    }

    /**
        Removes every instance of AudioParameterListener registered with
        every parameter, thus clearing all listsners.

        @see AudioParameter, AudioParameterListener
    */
    void removeAllListenersToParameters ()
    {
        for (int i = parameters.size (); --i >= 0;)
        {
            AudioParameter* parameter = parameters.getUnchecked (i);

            if (parameter)
                parameter->removeAllListeners ();
        }
    }

    //==============================================================================
    /**
        Set the current number of parameter update check per second
     */
    int getParametersChangeChecksPerSecond () const
    {
        return parameterThread->getParametersChangeChecksPerSecond ();
    }

    /**
        Set the current number of parameter update check per second
     */
    void setParametersChangeChecksPerSecond (const int howManyChecksPerSecond)
    {
        parameterThread->setParametersChangeChecksPerSecond (howManyChecksPerSecond);
    }

    //==============================================================================
    /**
        Write the current parameters state to an XML element

        This should take care about mapping and additional stuff, but actually it
        should not take care of the actual value.

        TODO - We have to fix this as soon as possible.

        @see readParametersFromXmlElement
    */
    void writeParametersToXmlElement (XmlElement* xml)
    {
        for (int i = 0; i < parameters.size(); i++)
        {
            AudioParameter* parameter = parameters.getUnchecked (i);

            if (parameter)
            {
                XmlElement* e = new XmlElement ("parameter");
                e->setAttribute ("key", i);
                //e->setAttribute (T("id"), parameter->getIndex ());
                e->setAttribute ("cc", parameter->getControllerNumber ());
                e->setAttribute ("value", parameter->getValueMapped ());
#ifdef JUCE_DEBUG
                e->setAttribute ("text", parameter->getValueMappedAsString ());
#endif
                xml->addChildElement (e);
            }
        }
    }

    /**
        Read the parameters state from an XML element

        This should take care about mapping and additional stuff, but actually it
        should not take care of the actual value.

        TODO - We have to fix this as soon as possible.

        @see writeParametersToXmlElement
     */
    void readParametersFromXmlElement (XmlElement* xml,
                                       const bool notifyParameterChange = false)
    {
        forEachXmlChildElement (*xml, e)
        {
            if (e->hasTagName ("parameter"))
            {
                //  Use the hash to determine if is the correct parameter
                int newKey = e->getIntAttribute ("key", -1);
                //int newID = e->getIntAttribute ("id", 0);
                int newMidiCC = e->getIntAttribute ("cc", -1);
                float newValue = e->getDoubleAttribute ("value", 0.0);

                if (newKey >= 0 && newKey < parameters.size())
                {
                    AudioParameter* parameter = parameters.getUnchecked (newKey);

                    if (parameter)
                    {
                        parameter->setValueMapped (newValue);
                        parameter->setControllerNumber (newMidiCC);

                        if (notifyParameterChange)
                            parameterThread->sendParameterChange (parameter);
                    }
                }
            }
        }
    }

    //==============================================================================
    /**
        Returns the number of actual presets

        @see AudioProcessor
    */
    int getNumPrograms() override
    {
        return programs.size ();
    }

    /**
        Returns the number of the current selected preset

        @see AudioProcessor
    */
    int getCurrentProgram() override
    {
        return currentProgram;
    }

    /**
        Sets the current selected preset

        @param index    the index of the new preset

        @see AudioProcessor
    */
    void setCurrentProgram (int index) override
    {
        if (currentProgram != index
            && index >= 0 && index < programs.size())
        {
            currentProgram = index;

            AudioProgram* thisProgram = programs [currentProgram];
            if (thisProgram)
            {
                restoreProgramFromFile (thisProgram->getFile(), false);
            }
        }
    }

    /**
        Returns the name of a preset

        @param index      the index of the preset we want to know the name

        @see AudioProcessor
    */
    const String getProgramName (int index) override
    {
        AudioProgram* thisProgram = programs[index];

        return thisProgram ? thisProgram->getName () : String();
    }

    /**
        Just changes the name of a preset

        @param index      the index of the preset name we want to change
        @param newName    name we want to give it

        @see AudioProcessor
    */
    void changeProgramName (int index, const String& newName) override
    {
        AudioProgram* thisProgram = programs[index];

        if (thisProgram)
            thisProgram->setName (newName);
    }

    //==============================================================================
    /**
        Saves the state of the plugin into a block of memory

        If you don't subclass this function in your plugin, then we let the preset
        manager do the job instead of you.

        @see AudioProcessor
    */
    void getStateInformation (MemoryBlock& destData) override
    {
    }

    /**
        Restores the state of the plugin into a block of memory

        If you don't subclass this function in your plugin, then we let the preset
        manager do the job instead of you. But actually it doesn't take care of a
        lot of situations, especially when the preset restored have different number
        of parameters: in fact if you changed the number of parameters between builds
        of the plugins, restoring them will probably mistake each parameter index.

        @see AudioProcessor
    */
    void setStateInformation (const void* data, int sizeInBytes) override
    {
    }

    //==============================================================================
    /**
       This function lets you add a new preset based on the current state of
       the passed parameter manager.

       It's easy to setup your filter in a well known state for its internal
       parameters, then just call this to save that setup.

       @return the preset index that was added
    */
    bool restoreProgramFromFile (const File& file, const bool addProgram = true)
    {
        if (! file.exists())
            return false;

        XmlDocument xmlDoc (file.loadFileAsString ());
        XmlElement* xml = xmlDoc.getDocumentElement();

        if (xml == 0 || ! xml->hasTagName ("preset"))
        {
            String errString = xmlDoc.getLastParseError();
			DBG("Error parsing preset XML: " + errString)
        }
        else
        {
#if 0
            int pluginUniqueID = xml->getIntAttribute ("uniqueid", 0);
            if (owner->getFilterID () == pluginUniqueID)
#endif
            {
                // current preset
                XmlElement* chunk = xml->getChildByName ("chunk");
                if (chunk)
                {
                    // presets are chunk based
                    forEachXmlChildElement (*chunk, dataElement)
                    {
                        if (dataElement->hasTagName ("data"))
                        {
                            MemoryBlock mb;
                            mb.fromBase64Encoding (dataElement->getAllSubText ());

                            setStateInformation (mb.getData (), mb.getSize ());

                            if (addProgram)
                                programs.add (new AudioProgram (file.getFileNameWithoutExtension (),
                                                                file));
                            break;
                        }

                        if (dataElement->hasTagName ("params"))
                        {
                            readParametersFromXmlElement (dataElement, false);
                        }
                    }

                    return true;
                }
            }
        }

        return false;
    }

    /**
       This function lets you add a new preset based on the current state of
       the passed parameter manager.

       It's easy to setup your filter in a well known state for its internal
       parameters, then just call this to save that setup.

       @return the preset index that was added
    */
    bool storeProgramToFile (const File& file, const bool addProgram = true)
    {
        MemoryBlock mb;
        File fileToSave = file;

        getStateInformation (mb);

        if (mb.getSize () > 0)
        {
            AudioProgram* programToSave = 0;

            if (! fileToSave.exists())
            {
                programToSave = programs [currentProgram];
                if (programToSave)
                    fileToSave = programToSave->getFile();
            }

            XmlElement xml ("preset");
            xml.setAttribute ("name", getName ());
            xml.setAttribute ("author", "");
            xml.setAttribute ("category", "");
            xml.setAttribute ("comments", "");

            XmlElement* chunk = new XmlElement ("chunk");
            {
                XmlElement* dataElement = new XmlElement ("data");
                dataElement->addTextElement (mb.toBase64Encoding ());
                chunk->addChildElement (dataElement);

                XmlElement* paramElement = new XmlElement ("params");
                writeParametersToXmlElement (paramElement);
                chunk->addChildElement (paramElement);
            }
            xml.addChildElement (chunk);

            if (fileToSave.replaceWithText (xml.createDocument (String())))
            {
                if (addProgram && programToSave == 0)
                    programs.add (new AudioProgram (fileToSave.getFileNameWithoutExtension (),
                                                    fileToSave));

                return true;
            }
        }

        return false;
    }

    /**
        Scan directory and search for programs

        The programs found will be added
    */
    void scanDirectoryForPrograms (const File& programsDirectory,
                                   const String& wildCard = "*.xml",
                                   const bool createDirectory = false)
    {
        File dirToScan = programsDirectory;

        if (dirToScan.existsAsFile ())
            dirToScan = dirToScan.getParentDirectory();

        if (! dirToScan.exists() && createDirectory)
            dirToScan.createDirectory ();

        if (dirToScan.isDirectory ())
        {
            DirectoryIterator iter (dirToScan, true, wildCard);
            while (iter.next())
            {
                File file (iter.getFile());

                programs.add (new AudioProgram (file.getFileNameWithoutExtension (),
                                                file));
            }
        }
    }

    /**
        Removes all added presets
    */
    void removeAllPrograms ()
    {
        for (int i = programs.size(); --i >= 0;)
            delete programs.getUnchecked (i);

        programs.clear ();
    }

    //==============================================================================
    /**
        Returns the name of the specified input channel

        @param channelIndex     index of the channel to query

        @see AudioProcessor
    */
    const String getInputChannelName (const int channelIndex) const override
    {
        return String (channelIndex + 1);
    }

    /**
        Returns the name of the specified output channel

        @param channelIndex     index of the channel to query

        @see AudioProcessor
    */
    const String getOutputChannelName (const int channelIndex) const override
    {
        return String (channelIndex + 1);
    }

    /**
        Returns true if the specified input channel is part of a stereo pair

        @param index            index of the channel to query

        @see AudioProcessor
    */
    bool isInputChannelStereoPair (int index) const override
    {
        return true;
    }

    /**
        Returns true if the specified output channel is part of a stereo pair

        @param index            index of the channel to query

        @see AudioProcessor
    */
    bool isOutputChannelStereoPair (int index) const override
    {
        return true;
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int blockSize) override
    {
    }

    void releaseResources () override
    {
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override
    {
    }

    //==============================================================================
    /**
        Returns a reference to the internal keyboard state
    */
    MidiKeyboardState& getKeyboardState ()             { return keyboardState; }

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    //==============================================================================
    /**
        Create the default editor for the processor

        If you don't override this, no editor will be created, and you should
        rely on the host default plugin editor (if it's capable of doing it)

        TODO - create a default plugin editor "AudioPluginDefaultEditor"
    */
    virtual AudioProcessorEditor* createEditor() override
    {
        return 0;
    }
#endif

    //==============================================================================
    juce_UseDebuggingNewOperator

protected:

    CriticalSection parameterLock;
    Array<AudioParameter*> parameters;
    AudioParameterThread* parameterThread;

    Array<AudioProgram*> programs;
    int currentProgram;

    MidiKeyboardState keyboardState;
    MidiAutomatorManager midiAutomatorManager;
};

#endif
