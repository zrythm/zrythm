/*
 * Carla Plugin Host
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_PLUGIN_HPP_INCLUDED
#define CARLA_PLUGIN_HPP_INCLUDED

#include "CarlaBackend.h"

// -----------------------------------------------------------------------
// Avoid including extra libs here

typedef void* lo_message;
typedef struct _NativePluginDescriptor NativePluginDescriptor;
struct LADSPA_RDF_Descriptor;

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} /* Fix editor indentation */
#endif

// -----------------------------------------------------------------------

/*!
 * @defgroup CarlaPluginAPI Carla Plugin API
 *
 * The Carla Plugin API.
 * @{
 */

class CarlaEngineAudioPort;
class CarlaEngineCVPort;
class CarlaEngineEventPort;
class CarlaEngineCVSourcePorts;
class CarlaEngineBridge;
struct CarlaStateSave;

// -----------------------------------------------------------------------

/*!
 * Carla Backend base plugin class
 *
 * This is the base class for all available plugin types available in Carla Backend.
 * All virtual calls are implemented in this class as fallback (except reload and process),
 * so it's safe to only override needed calls.
 *
 * @see PluginType
 */
class CARLA_API CarlaPlugin
{
protected:
    /*!
     * This is the constructor of the base plugin class.
     *
     * @param engine The engine which this plugin belongs to, must not be null
     * @param id     The 'id' of this plugin, must be between 0 and CarlaEngine::maxPluginNumber()
     */
    CarlaPlugin(CarlaEngine* engine, uint id);

public:
    /*!
     * This is the destructor of the base plugin class.
     */
    virtual ~CarlaPlugin();

    // -------------------------------------------------------------------
    // Information (base)

    /*!
     * Get the plugin's type (a subclass of CarlaPlugin).
     *
     * @note Plugin bridges will return their respective plugin type, there is no plugin type such as "bridge".
     *       To check if a plugin is a bridge use:
     * @code
     * if (getHints() & PLUGIN_IS_BRIDGE)
     *     ...
     * @endcode
     */
    virtual PluginType getType() const noexcept = 0;

    /*!
     * Get the plugin's id (as passed in the constructor).
     *
     * @see setId()
     */
    uint getId() const noexcept;

    /*!
     * Get the plugin's hints.
     *
     * @see PluginHints
     */
    uint getHints() const noexcept;

    /*!
     * Get the plugin's options (currently in use).
     *
     * @see PluginOptions, getOptionsAvailable() and setOption()
     */
    uint getOptionsEnabled() const noexcept;

    /*!
     * Check if the plugin is enabled.
     * When a plugin is disabled, it will never be processed or managed in any way.
     *
     * @see setEnabled()
     */
    bool isEnabled() const noexcept;

    /*!
     * Get the plugin's internal name.
     * This name is unique within all plugins in an engine.
     *
     * @see getRealName() and setName()
     */
    const char* getName() const noexcept;

    /*!
     * Get the currently loaded DLL filename for this plugin.
     * (Sound kits return their exact filename).
     */
    const char* getFilename() const noexcept;

    /*!
     * Get the plugins's icon name.
     */
    const char* getIconName() const noexcept;

    /*!
     * Get the plugin's category (delay, filter, synth, etc).
     */
    virtual CarlaPluginCategory getCategory() const noexcept;

    /*!
     * Get the plugin's native unique Id.
     * May return 0 on plugin types that don't support Ids.
     */
    virtual int64_t getUniqueId() const noexcept;

    /*!
     * Get the plugin's latency, in sample frames.
     */
    virtual uint32_t getLatencyInFrames() const noexcept;

    // -------------------------------------------------------------------
    // Information (count)

    /*!
     * Get the number of audio inputs.
     */
    uint32_t getAudioInCount() const noexcept;

    /*!
     * Get the number of audio outputs.
     */
    uint32_t getAudioOutCount() const noexcept;

    /*!
     * Get the number of CV inputs.
     */
    uint32_t getCVInCount() const noexcept;

    /*!
     * Get the number of CV outputs.
     */
    uint32_t getCVOutCount() const noexcept;

    /*!
     * Get the number of MIDI inputs.
     */
    virtual uint32_t getMidiInCount() const noexcept;

    /*!
     * Get the number of MIDI outputs.
     */
    virtual uint32_t getMidiOutCount() const noexcept;

    /*!
     * Get the number of parameters.
     * To know the number of parameter inputs and outputs separately use getParameterCountInfo() instead.
     */
    uint32_t getParameterCount() const noexcept;

    /*!
     * Get the number of scalepoints for parameter @a parameterId.
     */
    virtual uint32_t getParameterScalePointCount(uint32_t parameterId) const noexcept;

    /*!
     * Get the number of programs.
     */
    uint32_t getProgramCount() const noexcept;

    /*!
     * Get the number of MIDI programs.
     */
    uint32_t getMidiProgramCount() const noexcept;

    /*!
     * Get the number of custom data sets.
     */
    uint32_t getCustomDataCount() const noexcept;

    // -------------------------------------------------------------------
    // Information (current data)

    /*!
     * Get the current program number (-1 if unset).
     *
     * @see setProgram()
     */
    int32_t getCurrentProgram() const noexcept;

    /*!
     * Get the current MIDI program number (-1 if unset).
     *
     * @see setMidiProgram()
     * @see setMidiProgramById()
     */
    int32_t getCurrentMidiProgram() const noexcept;

    /*!
     * Get the parameter data of @a parameterId.
     */
    const ParameterData& getParameterData(uint32_t parameterId) const noexcept;

    /*!
     * Get the parameter ranges of @a parameterId.
     */
    const ParameterRanges& getParameterRanges(uint32_t parameterId) const noexcept;

    /*!
     * Check if parameter @a parameterId is of output type.
     */
    bool isParameterOutput(uint32_t parameterId) const noexcept;

    /*!
     * Get the MIDI program at @a index.
     *
     * @see getMidiProgramName()
     */
    const MidiProgramData& getMidiProgramData(uint32_t index) const noexcept;

    /*!
     * Get the custom data set at @a index.
     *
     * @see getCustomDataCount() and setCustomData()
     */
    const CustomData& getCustomData(uint32_t index) const noexcept;

    /*!
     * Get the complete plugin chunk data into @a dataPtr.
     *
     * @note Make sure to verify the plugin supports chunks before calling this function!
     * @return The size of the chunk or 0 if invalid.
     *
     * @see setChunkData()
     */
    virtual std::size_t getChunkData(void** dataPtr) noexcept;

    // -------------------------------------------------------------------
    // Information (per-plugin data)

    /*!
     * Get the plugin available options.
     *
     * @see PluginOptions, getOptions() and setOption()
     */
    virtual uint getOptionsAvailable() const noexcept;

    /*!
     * Get the current parameter value of @a parameterId.
     */
    virtual float getParameterValue(uint32_t parameterId) const noexcept;

    /*!
     * Get the scalepoint @a scalePointId value of the parameter @a parameterId.
     */
    virtual float getParameterScalePointValue(uint32_t parameterId, uint32_t scalePointId) const noexcept;

    /*!
     * Get the plugin's label (URI for LV2 plugins).
     */
    __attribute__((warn_unused_result))
    virtual bool getLabel(char* strBuf) const noexcept;

    /*!
     * Get the plugin's maker.
     */
    __attribute__((warn_unused_result))
    virtual bool getMaker(char* strBuf) const noexcept;

    /*!
     * Get the plugin's copyright/license.
     */
    __attribute__((warn_unused_result))
    virtual bool getCopyright(char* strBuf) const noexcept;

    /*!
     * Get the plugin's (real) name.
     *
     * @see getName() and setName()
     */
    __attribute__((warn_unused_result))
    virtual bool getRealName(char* strBuf) const noexcept;

    /*!
     * Get the name of the parameter @a parameterId.
     */
    __attribute__((warn_unused_result))
    virtual bool getParameterName(uint32_t parameterId, char* strBuf) const noexcept;

    /*!
     * Get the symbol of the parameter @a parameterId.
     */
    __attribute__((warn_unused_result))
    virtual bool getParameterSymbol(uint32_t parameterId, char* strBuf) const noexcept;

    /*!
     * Get the custom text of the parameter @a parameterId.
     * @see PARAMETER_USES_CUSTOM_TEXT
     */
    __attribute__((warn_unused_result))
    virtual bool getParameterText(uint32_t parameterId, char* strBuf) noexcept;

    /*!
     * Get the unit of the parameter @a parameterId.
     */
    __attribute__((warn_unused_result))
    virtual bool getParameterUnit(uint32_t parameterId, char* strBuf) const noexcept;

    /*!
     * Get the comment (documentation) of the parameter @a parameterId.
     */
    __attribute__((warn_unused_result))
    virtual bool getParameterComment(uint32_t parameterId, char* strBuf) const noexcept;

    /*!
     * Get the group name of the parameter @a parameterId.
     * @note The group name is prefixed by a unique symbol and ":".
     */
    __attribute__((warn_unused_result))
    virtual bool getParameterGroupName(uint32_t parameterId, char* strBuf) const noexcept;

    /*!
     * Get the scalepoint @a scalePointId label of the parameter @a parameterId.
     */
    __attribute__((warn_unused_result))
    virtual bool getParameterScalePointLabel(uint32_t parameterId, uint32_t scalePointId, char* strBuf) const noexcept;

    /*!
     * Get the current parameter value of @a parameterId.
     * @a parameterId can be negative to allow internal parameters.
     * @see InternalParametersIndex
     */
    float getInternalParameterValue(int32_t parameterId) const noexcept;

    /*!
     * Get the name of the program at @a index.
     */
    __attribute__((warn_unused_result))
    bool getProgramName(uint32_t index, char* strBuf) const noexcept;

    /*!
     * Get the name of the MIDI program at @a index.
     *
     * @see getMidiProgramInfo()
     */
    __attribute__((warn_unused_result))
    bool getMidiProgramName(uint32_t index, char* strBuf) const noexcept;

    /*!
     * Get information about the plugin's parameter count.
     * This is used to check how many input, output and total parameters are available.
     *
     * @note Some parameters might not be input or output (ie, invalid).
     *
     * @see getParameterCount()
     */
    void getParameterCountInfo(uint32_t& ins, uint32_t& outs) const noexcept;

    // -------------------------------------------------------------------
    // Set data (state)

    /*!
     * Tell the plugin to prepare for save.
     */
    virtual void prepareForSave();

    /*!
     * Reset all possible parameters.
     */
    virtual void resetParameters() noexcept;

    /*!
     * Randomize all possible parameters.
     */
    virtual void randomizeParameters() noexcept;

    /*!
     * Get the plugin's save state.
     * The plugin will automatically call prepareForSave() if requested.
     *
     * @see loadStateSave()
     */
    const CarlaStateSave& getStateSave(bool callPrepareForSave = true);

    /*!
     * Get the plugin's save state.
     *
     * @see getStateSave()
     */
    void loadStateSave(const CarlaStateSave& stateSave);

    /*!
     * Save the current plugin state to @a filename.
     *
     * @see loadStateFromFile()
     */
    bool saveStateToFile(const char* filename);

    /*!
     * Save the plugin state from @a filename.
     *
     * @see saveStateToFile()
     */
    bool loadStateFromFile(const char* filename);

    /*!
     * Export this plugin as LV2.
     */
    bool exportAsLV2(const char* lv2path);

    // -------------------------------------------------------------------
    // Set data (internal stuff)

    /*!
     * Set the plugin's id to @a newId.
     *
     * @see getId()
     * @note RT call
     */
    virtual void setId(uint newId) noexcept;

    /*!
     * Set the plugin's name to @a newName.
     *
     * @see getName() and getRealName()
     */
    virtual void setName(const char* newName);

    /*!
     * Set a plugin's option.
     *
     * @see getOptions() and getOptionsAvailable()
     */
    virtual void setOption(uint option, bool yesNo, bool sendCallback);

    /*!
     * Enable or disable the plugin according to @a yesNo.
     * When a plugin is disabled, it will never be processed or managed in any way.
     *
     * @see isEnabled()
     */
    void setEnabled(bool yesNo) noexcept;

    /*!
     * Set plugin as active according to @a active.
     *
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     */
    void setActive(bool active, bool sendOsc, bool sendCallback) noexcept;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    /*!
     * Set the plugin's dry/wet signal value to @a value.
     * @a value must be between 0.0 and 1.0.
     *
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     */
    void setDryWet(float value, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Set the plugin's output volume to @a value.
     * @a value must be between 0.0 and 1.27.
     *
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     */
    void setVolume(float value, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Set the plugin's output left balance value to @a value.
     * @a value must be between -1.0 and 1.0.
     *
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     *
     * @note Pure-Stereo plugins only!
     */
    void setBalanceLeft(float value, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Set the plugin's output right balance value to @a value.
     * @a value must be between -1.0 and 1.0.
     *
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     *
     * @note Pure-Stereo plugins only!
     */
    void setBalanceRight(float value, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Set the plugin's output panning value to @a value.
     * @a value must be between -1.0 and 1.0.
     *
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     *
     * @note Force-Stereo plugins only!
     */
    void setPanning(float value, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Overloaded functions, to be called from within RT context only.
     */
    void setDryWetRT(float value, bool sendCallbackLater) noexcept;
    void setVolumeRT(float value, bool sendCallbackLater) noexcept;
    void setBalanceLeftRT(float value, bool sendCallbackLater) noexcept;
    void setBalanceRightRT(float value, bool sendCallbackLater) noexcept;
    void setPanningRT(float value, bool sendCallbackLater) noexcept;
#endif // ! BUILD_BRIDGE_ALTERNATIVE_ARCH

    /*!
     * Set the plugin's midi control channel.
     *
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     */
    virtual void setCtrlChannel(int8_t channel, bool sendOsc, bool sendCallback) noexcept;

    // -------------------------------------------------------------------
    // Set data (plugin-specific stuff)

    /*!
     * Set a plugin's parameter value.
     *
     * @param parameterId The parameter to change
     * @param value The new parameter value, must be within the parameter's range
     * @param sendGui Send message change to plugin's custom GUI, if any
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     *
     * @see getParameterValue()
     */
    virtual void setParameterValue(uint32_t parameterId, float value, bool sendGui, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Overloaded function, to be called from within RT context only.
     */
    virtual void setParameterValueRT(uint32_t parameterId, float value, bool sendCallbackLater) noexcept;

    /*!
     * Set a plugin's parameter value, including internal parameters.
     * @a rindex can be negative to allow internal parameters change (as defined in InternalParametersIndex).
     *
     * @see setParameterValue()
     * @see setActive()
     * @see setDryWet()
     * @see setVolume()
     * @see setBalanceLeft()
     * @see setBalanceRight()
     */
    void setParameterValueByRealIndex(int32_t rindex, float value, bool sendGui, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Set parameter's @a parameterId MIDI channel to @a channel.
     * @a channel must be between 0 and 15.
     */
    virtual void setParameterMidiChannel(uint32_t parameterId, uint8_t channel, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Set parameter's @a parameterId mapped control index to @a index.
     * @see ParameterData::mappedControlIndex
     */
    virtual void setParameterMappedControlIndex(uint32_t parameterId, int16_t index, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Set parameter's @a parameterId mapped range to @a minimum and @a maximum.
     */
    virtual void setParameterMappedRange(uint32_t parameterId, float minimum, float maximum, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Add a custom data set.
     * If @a key already exists, its current value will be swapped with @a value.
     *
     * @param type Type of data used in @a value.
     * @param key A key identifying this data set.
     * @param value The value of the data set, of type @a type.
     * @param sendGui Send message change to plugin's custom GUI, if any
     *
     * @see getCustomDataCount() and getCustomData()
     */
    virtual void setCustomData(const char* type, const char* key, const char* value, bool sendGui);

    /*!
     * Set the complete chunk data as @a data.
     *
     * @see getChunkData()
     *
     * @note Make sure to verify the plugin supports chunks before calling this function
     */
    virtual void setChunkData(const void* data, std::size_t dataSize);

    /*!
     * Change the current plugin program to @a index.
     *
     * If @a index is negative the plugin's program will be considered unset.
     * The plugin's default parameter values will be updated when this function is called.
     *
     * @param index New program index to use
     * @param sendGui Send message change to plugin's custom GUI, if any
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     */
    virtual void setProgram(int32_t index, bool sendGui, bool sendOsc, bool sendCallback, bool doingInit = false) noexcept;

    /*!
     * Change the current MIDI plugin program to @a index.
     *
     * If @a index is negative the plugin's program will be considered unset.
     * The plugin's default parameter values will be updated when this function is called.
     *
     * @param index New program index to use
     * @param sendGui Send message change to plugin's custom GUI, if any
     * @param sendOsc Send message change over OSC
     * @param sendCallback Send message change to registered callback
     */
    virtual void setMidiProgram(int32_t index, bool sendGui, bool sendOsc, bool sendCallback, bool doingInit = false) noexcept;

    /*!
     * This is an overloaded call to setMidiProgram().
     * It changes the current MIDI program using @a bank and @a program values instead of index.
     */
    void setMidiProgramById(uint32_t bank, uint32_t program, bool sendGui, bool sendOsc, bool sendCallback) noexcept;

    /*!
     * Overloaded functions, to be called from within RT context only.
     */
    virtual void setProgramRT(uint32_t index, bool sendCallbackLater) noexcept;
    virtual void setMidiProgramRT(uint32_t index, bool sendCallbackLater) noexcept;

    // -------------------------------------------------------------------
    // Plugin state

    /*!
     * Reload the plugin's entire state (including programs).
     * The plugin will be disabled during this call.
     */
    virtual void reload() = 0;

    /*!
     * Reload the plugin's programs state.
     */
    virtual void reloadPrograms(bool doInit);

    // -------------------------------------------------------------------
    // Plugin processing

protected:
    /*!
     * Plugin activate call.
     */
    virtual void activate() noexcept;

    /*!
     * Plugin activate call.
     */
    virtual void deactivate() noexcept;

public:
    /*!
     * Plugin process call.
     */
    virtual void process(const float* const* audioIn, float** audioOut,
                         const float* const* cvIn, float** cvOut, uint32_t frames) = 0;

    /*!
     * Tell the plugin the current buffer size changed.
     */
    virtual void bufferSizeChanged(uint32_t newBufferSize);

    /*!
     * Tell the plugin the current sample rate changed.
     */
    virtual void sampleRateChanged(double newSampleRate);

    /*!
     * Tell the plugin the current offline mode changed.
     */
    virtual void offlineModeChanged(bool isOffline);

    // -------------------------------------------------------------------
    // Misc

    /*!
     * Idle function (non-UI), called at regular intervals.
     * @note: This function is NOT called from the main thread.
     */
    virtual void idle();

    /*!
     * Try to lock the plugin's master mutex.
     * @param forcedOffline When true, always locks and returns true
     */
    bool tryLock(bool forcedOffline) noexcept;

    /*!
     * Unlock the plugin's master mutex.
     */
    void unlock() noexcept;

    // -------------------------------------------------------------------
    // Plugin buffers

    /*!
     * Initialize all RT buffers of the plugin.
     */
    virtual void initBuffers() const noexcept;

    /*!
     * Delete and clear all RT buffers.
     */
    virtual void clearBuffers() noexcept;

    // -------------------------------------------------------------------
    // OSC stuff

    /*!
     * Handle an OSC message.
     * FIXME
     */
    virtual void handleOscMessage(const char* method,
                                  int argc,
                                  const void* argv,
                                  const char* types,
                                  lo_message msg);

    // -------------------------------------------------------------------
    // MIDI events

    /*!
     * Send a single midi note to be processed in the next audio callback.
     * A note with 0 velocity means note-off.
     * @note Non-RT call
     */
    void sendMidiSingleNote(uint8_t channel, uint8_t note, uint8_t velo,
                            bool sendGui, bool sendOsc, bool sendCallback);

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    /*!
     * Send all midi notes off to the host callback.
     * This doesn't send the actual MIDI All-Notes-Off event, but 128 note-offs instead (IFF ctrlChannel is valid).
     * @note RT call
     */
    void postponeRtAllNotesOff();
#endif

    // -------------------------------------------------------------------
    // UI Stuff

    /*!
     * Show (or hide) the plugin's custom UI according to @a yesNo.
     * This function is always called from the main thread.
     */
    virtual void showCustomUI(bool yesNo);

    /*!
     * UI idle function, called at regular intervals.
     * This function is only called from the main thread if PLUGIN_NEEDS_UI_MAIN_THREAD is set.
     * @note This function may sometimes be called even if the UI is not visible yet.
     */
    virtual void uiIdle();

    /*!
     * Tell the UI a parameter has changed.
     * @see uiIdle
     */
    virtual void uiParameterChange(uint32_t index, float value) noexcept;

    /*!
     * Tell the UI the current program has changed.
     * @see uiIdle
     */
    virtual void uiProgramChange(uint32_t index) noexcept;

    /*!
     * Tell the UI the current midi program has changed.
     * @see uiIdle
     */
    virtual void uiMidiProgramChange(uint32_t index) noexcept;

    /*!
     * Tell the UI a note has been pressed.
     * @see uiIdle
     */
    virtual void uiNoteOn(uint8_t channel, uint8_t note, uint8_t velo) noexcept;

    /*!
     * Tell the UI a note has been released.
     * @see uiIdle
     */
    virtual void uiNoteOff(uint8_t channel, uint8_t note) noexcept;

    // -------------------------------------------------------------------
    // Helper functions

    /*!
     * Get the plugin's engine, as passed in the constructor.
     */
    CarlaEngine* getEngine() const noexcept;

    /*!
     * Get the plugin's engine client.
     */
    CarlaEngineClient* getEngineClient() const noexcept;

    /*!
     * Get a plugin's audio input port.
     */
    CarlaEngineAudioPort* getAudioInPort(uint32_t index) const noexcept;

    /*!
     * Get a plugin's audio output port.
     */
    CarlaEngineAudioPort* getAudioOutPort(uint32_t index) const noexcept;

    /*!
     * Get a plugin's CV input port.
     */
    CarlaEngineCVPort* getCVInPort(uint32_t index) const noexcept;

    /*!
     * Get a plugin's CV output port.
     */
    CarlaEngineCVPort* getCVOutPort(uint32_t index) const noexcept;

    /*!
     * Get the plugin's default event input port.
     */
    CarlaEngineEventPort* getDefaultEventInPort() const noexcept;

    /*!
     * Get the plugin's default event output port.
     */
    CarlaEngineEventPort* getDefaultEventOutPort() const noexcept;

    /*!
     * Get the plugin's type native handle.
     * This will be LADSPA_Handle, LV2_Handle, etc.
     */
    virtual void* getNativeHandle() const noexcept;

    /*!
     * Get the plugin's type native descriptor.
     * This will be LADSPA_Descriptor, DSSI_Descriptor, LV2_Descriptor, AEffect, etc.
     */
    virtual const void* getNativeDescriptor() const noexcept;

    /*!
     * Get the plugin UI bridge process Id.
     */
    virtual uintptr_t getUiBridgeProcessId() const noexcept;

    // -------------------------------------------------------------------

    /*!
     * Get the plugin's patchbay nodeId.
     * @see setPatchbayNodeId()
     */
    uint32_t getPatchbayNodeId() const noexcept;

    /*!
     * Set the plugin's patchbay nodeId.
     * @see getPatchbayNodeId()
     */
    void setPatchbayNodeId(uint32_t nodeId) noexcept;

    // -------------------------------------------------------------------
    // Plugin initializers

    /*!
     * Get a plugin's binary type.
     * This is always BINARY_NATIVE unless the plugin is a bridge.
     */
    virtual BinaryType getBinaryType() const noexcept
    {
        return BINARY_NATIVE;
    }

    /*!
     * Handy function required by CarlaEngine::clonePlugin().
     */
    virtual const void* getExtraStuff() const noexcept
    {
        return nullptr;
    }

#ifndef DOXYGEN
    struct Initializer {
        CarlaEngine* const engine;
        const uint id;
        const char* const filename;
        const char* const name;
        const char* const label;
        const int64_t uniqueId;
        const uint options; // see PluginOptions
    };

    static CarlaPlugin* newNative(const Initializer& init);
    static CarlaPlugin* newBridge(const Initializer& init, BinaryType btype, PluginType ptype, const char* bridgeBinary);

    static CarlaPlugin* newLADSPA(const Initializer& init, const LADSPA_RDF_Descriptor* rdfDescriptor);
    static CarlaPlugin* newDSSI(const Initializer& init);
    static CarlaPlugin* newLV2(const Initializer& init);
    static CarlaPlugin* newVST2(const Initializer& init);
    static CarlaPlugin* newVST3(const Initializer& init);
    static CarlaPlugin* newAU(const Initializer& init);

    static CarlaPlugin* newJuce(const Initializer& init, const char* format);
    static CarlaPlugin* newFluidSynth(const Initializer& init, bool use16Outs);
    static CarlaPlugin* newSFZero(const Initializer& init);

    static CarlaPlugin* newJackApp(const Initializer& init);
#endif

    // -------------------------------------------------------------------

protected:
    /*!
     * Internal data, for CarlaPlugin subclasses only.
     */
    struct ProtectedData;
    ProtectedData* const pData;

    // -------------------------------------------------------------------
    // Internal helper functions

public:
    // FIXME: remove public exception on 2.1 release
    /*!
     * Call LV2 restore.
     */
    virtual void restoreLV2State() noexcept;

protected:
    /*!
     * Give plugin bridges a change to update their custom data sets.
     */
    virtual void waitForBridgeSaveSignal() noexcept;

    // -------------------------------------------------------------------
    // Helper classes

    /*!
     * Fully disable plugin in scope and also its engine client.
     * May wait-block on constructor for plugin process to end.
     */
    class ScopedDisabler
    {
    public:
        ScopedDisabler(CarlaPlugin* plugin) noexcept;
        ~ScopedDisabler() noexcept;

    private:
        CarlaPlugin* const fPlugin;
        bool fWasEnabled;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(ScopedDisabler)
    };

    /*!
     * Lock the plugin's own run/process call.
     * Plugin will still work as normal, but output only silence.
     * On destructor needsReset flag might be set if the plugin might have missed some events.
     */
    class ScopedSingleProcessLocker
    {
    public:
        ScopedSingleProcessLocker(CarlaPlugin* plugin, bool block) noexcept;
        ~ScopedSingleProcessLocker() noexcept;

    private:
        CarlaPlugin* const fPlugin;
        const bool fBlock;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(ScopedSingleProcessLocker)
    };

    friend class CarlaEngineBridge;
    CARLA_DECLARE_NON_COPY_CLASS(CarlaPlugin)
};

/**@}*/

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_PLUGIN_HPP_INCLUDED
