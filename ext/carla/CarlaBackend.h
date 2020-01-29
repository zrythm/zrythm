/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BACKEND_H_INCLUDED
#define CARLA_BACKEND_H_INCLUDED

#include "CarlaDefines.h"

#ifdef CARLA_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#define STR_MAX 0xFF

#ifdef __cplusplus
# define CARLA_BACKEND_START_NAMESPACE namespace CarlaBackend {
# define CARLA_BACKEND_END_NAMESPACE }
# define CARLA_BACKEND_USE_NAMESPACE using namespace CarlaBackend;
# include <algorithm>
# include <cmath>
# include <limits>
/* Start namespace */
CARLA_BACKEND_START_NAMESPACE
#endif

/*!
 * @defgroup CarlaBackendAPI Carla Backend API
 *
 * The Carla Backend API.
 *
 * These are the base definitions for everything in the Carla backend code.
 * @{
 */

/* ------------------------------------------------------------------------------------------------------------
 * Carla Backend API (base definitions) */

/*!
 * Maximum default number of loadable plugins.
 */
static const uint MAX_DEFAULT_PLUGINS = 99;

/*!
 * Maximum number of loadable plugins in rack mode.
 */
static const uint MAX_RACK_PLUGINS = 16;

/*!
 * Maximum number of loadable plugins in patchbay mode.
 */
static const uint MAX_PATCHBAY_PLUGINS = 255;

/*!
 * Maximum default number of parameters allowed.
 * @see ENGINE_OPTION_MAX_PARAMETERS
 */
static const uint MAX_DEFAULT_PARAMETERS = 200;

/*!
 * The "plugin Id" for the global Carla instance.
 * Currently only used for audio peaks.
 */
static const uint MAIN_CARLA_PLUGIN_ID = 0xFFFF;

/* ------------------------------------------------------------------------------------------------------------
 * Engine Driver Device Hints */

/*!
 * @defgroup EngineDriverHints Engine Driver Device Hints
 *
 * Various engine driver device hints.
 * @see CarlaEngine::getHints(), CarlaEngine::getDriverDeviceInfo() and carla_get_engine_driver_device_info()
 * @{
 */

/*!
 * Engine driver device has custom control-panel.
 */
static const uint ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL = 0x1;

/*!
 * Engine driver device can use a triple-buffer (3 number of periods instead of the usual 2).
 * @see ENGINE_OPTION_AUDIO_NUM_PERIODS
 */
static const uint ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER = 0x2;

/*!
 * Engine driver device can change buffer-size on the fly.
 * @see ENGINE_OPTION_AUDIO_BUFFER_SIZE
 */
static const uint ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE = 0x4;

/*!
 * Engine driver device can change sample-rate on the fly.
 * @see ENGINE_OPTION_AUDIO_SAMPLE_RATE
 */
static const uint ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE = 0x8;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Hints */

/*!
 * @defgroup PluginHints Plugin Hints
 *
 * Various plugin hints.
 * @see CarlaPlugin::getHints() and carla_get_plugin_info()
 * @{
 */

/*!
 * Plugin is a bridge.
 * This hint is required because "bridge" itself is not a plugin type.
 */
static const uint PLUGIN_IS_BRIDGE = 0x001;

/*!
 * Plugin is hard real-time safe.
 */
static const uint PLUGIN_IS_RTSAFE = 0x002;

/*!
 * Plugin is a synth (produces sound).
 */
static const uint PLUGIN_IS_SYNTH = 0x004;

/*!
 * Plugin has its own custom UI.
 * @see CarlaPlugin::showCustomUI() and carla_show_custom_ui()
 */
static const uint PLUGIN_HAS_CUSTOM_UI = 0x008;

/*!
 * Plugin can use internal Dry/Wet control.
 */
static const uint PLUGIN_CAN_DRYWET = 0x010;

/*!
 * Plugin can use internal Volume control.
 */
static const uint PLUGIN_CAN_VOLUME = 0x020;

/*!
 * Plugin can use internal (Stereo) Balance controls.
 */
static const uint PLUGIN_CAN_BALANCE = 0x040;

/*!
 * Plugin can use internal (Mono) Panning control.
 */
static const uint PLUGIN_CAN_PANNING = 0x080;

/*!
 * Plugin needs a constant, fixed-size audio buffer.
 */
static const uint PLUGIN_NEEDS_FIXED_BUFFERS = 0x100;

/*!
 * Plugin needs to receive all UI events in the main thread.
 */
static const uint PLUGIN_NEEDS_UI_MAIN_THREAD = 0x200;

/*!
 * Plugin uses 1 program per MIDI channel.
 * @note: Only used in some internal plugins and sf2 files.
 */
static const uint PLUGIN_USES_MULTI_PROGS = 0x400;

/*!
 * Plugin can make use of inline display API.
 */
static const uint PLUGIN_HAS_INLINE_DISPLAY = 0x800;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Options */

/*!
 * @defgroup PluginOptions Plugin Options
 *
 * Various plugin options.
 * @note Do not modify these values, as they are saved as-is in project files.
 * @see CarlaPlugin::getOptionsAvailable(), CarlaPlugin::getOptionsEnabled(), carla_get_plugin_info() and carla_set_option()
 * @{
 */

/*!
 * Use constant/fixed-size audio buffers.
 */
static const uint PLUGIN_OPTION_FIXED_BUFFERS = 0x001;

/*!
 * Force mono plugin as stereo.
 */
static const uint PLUGIN_OPTION_FORCE_STEREO = 0x002;

/*!
 * Map MIDI programs to plugin programs.
 */
static const uint PLUGIN_OPTION_MAP_PROGRAM_CHANGES = 0x004;

/*!
 * Use chunks to save and restore data instead of parameter values.
 */
static const uint PLUGIN_OPTION_USE_CHUNKS = 0x008;

/*!
 * Send MIDI control change events.
 */
static const uint PLUGIN_OPTION_SEND_CONTROL_CHANGES = 0x010;

/*!
 * Send MIDI channel pressure events.
 */
static const uint PLUGIN_OPTION_SEND_CHANNEL_PRESSURE = 0x020;

/*!
 * Send MIDI note after-touch events.
 */
static const uint PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH = 0x040;

/*!
 * Send MIDI pitch-bend events.
 */
static const uint PLUGIN_OPTION_SEND_PITCHBEND = 0x080;

/*!
 * Send MIDI all-sounds/notes-off events, single note-offs otherwise.
 */
static const uint PLUGIN_OPTION_SEND_ALL_SOUND_OFF = 0x100;

/*!
 * Send MIDI bank/program changes.
 * @note: This option conflicts with PLUGIN_OPTION_MAP_PROGRAM_CHANGES and cannot be used at the same time.
 */
static const uint PLUGIN_OPTION_SEND_PROGRAM_CHANGES = 0x200;

/*!
 * Special flag to indicate that plugin options are not yet set.
 * This flag exists because 0x0 as an option value is a valid one, so we need something else to indicate "null-ness".
 */
static const uint PLUGIN_OPTIONS_NULL = 0x10000;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Parameter Hints */

/*!
 * @defgroup ParameterHints Parameter Hints
 *
 * Various parameter hints.
 * @see CarlaPlugin::getParameterData() and carla_get_parameter_data()
 * @{
 */

/*!
 * Parameter value is boolean.
 * It's always at either minimum or maximum value.
 */
static const uint PARAMETER_IS_BOOLEAN = 0x001;

/*!
 * Parameter value is integer.
 */
static const uint PARAMETER_IS_INTEGER = 0x002;

/*!
 * Parameter value is logarithmic.
 */
static const uint PARAMETER_IS_LOGARITHMIC = 0x004;

/*!
 * Parameter is enabled.
 * It can be viewed, changed and stored.
 */
static const uint PARAMETER_IS_ENABLED = 0x010;

/*!
 * Parameter is automable (real-time safe).
 */
static const uint PARAMETER_IS_AUTOMABLE = 0x020;

/*!
 * Parameter is read-only.
 * It cannot be changed.
 */
static const uint PARAMETER_IS_READ_ONLY = 0x040;

/*!
 * Parameter needs sample rate to work.
 * Value and ranges are multiplied by sample rate on usage and divided by sample rate on save.
 */
static const uint PARAMETER_USES_SAMPLERATE = 0x100;

/*!
 * Parameter uses scale points to define internal values in a meaningful way.
 */
static const uint PARAMETER_USES_SCALEPOINTS = 0x200;

/*!
 * Parameter uses custom text for displaying its value.
 * @see CarlaPlugin::getParameterText() and carla_get_parameter_text()
 */
static const uint PARAMETER_USES_CUSTOM_TEXT = 0x400;

/*!
 * Parameter can be turned into a CV control.
 */
static const uint PARAMETER_CAN_BE_CV_CONTROLLED = 0x800;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Patchbay Port Hints */

/*!
 * @defgroup PatchbayPortHints Patchbay Port Hints
 *
 * Various patchbay port hints.
 * @{
 */

/*!
 * Patchbay port is input.
 * When this hint is not set, port is assumed to be output.
 */
static const uint PATCHBAY_PORT_IS_INPUT = 0x01;

/*!
 * Patchbay port is of Audio type.
 */
static const uint PATCHBAY_PORT_TYPE_AUDIO = 0x02;

/*!
 * Patchbay port is of CV type (Control Voltage).
 */
static const uint PATCHBAY_PORT_TYPE_CV = 0x04;

/*!
 * Patchbay port is of MIDI type.
 */
static const uint PATCHBAY_PORT_TYPE_MIDI = 0x08;

/*!
 * Patchbay port is of OSC type.
 */
static const uint PATCHBAY_PORT_TYPE_OSC = 0x10;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Patchbay Port Group Hints */

/*!
 * @defgroup PatchbayPortGroupHints Patchbay Port Group Hints
 *
 * Various patchbay port group hints.
 * @{
 */

/*!
 * Indicates that this group should be considered the "main" input.
 */
static const uint PATCHBAY_PORT_GROUP_MAIN_INPUT = 0x01;

/*!
 * Indicates that this group should be considered the "main" output.
 */
static const uint PATCHBAY_PORT_GROUP_MAIN_OUTPUT = 0x02;

/*!
 * A stereo port group, where the 1st port is left and the 2nd is right.
 */
static const uint PATCHBAY_PORT_GROUP_STEREO = 0x04;

/*!
 * A mid-side stereo group, where the 1st port is center and the 2nd is side.
 */
static const uint PATCHBAY_PORT_GROUP_MID_SIDE = 0x08;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Custom Data Types */

/*!
 * @defgroup CustomDataTypes Custom Data Types
 *
 * These types define how the value in the CustomData struct is stored.
 * @see CustomData::type
 * @{
 */

/*!
 * Boolean string type URI.
 * Only "true" and "false" are valid values.
 */
static const char* const CUSTOM_DATA_TYPE_BOOLEAN = "http://kxstudio.sf.net/ns/carla/boolean";

/*!
 * Chunk type URI.
 */
static const char* const CUSTOM_DATA_TYPE_CHUNK = "http://kxstudio.sf.net/ns/carla/chunk";

/*!
 * Property type URI.
 */
static const char* const CUSTOM_DATA_TYPE_PROPERTY = "http://kxstudio.sf.net/ns/carla/property";

/*!
 * String type URI.
 */
static const char* const CUSTOM_DATA_TYPE_STRING = "http://kxstudio.sf.net/ns/carla/string";

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Custom Data Keys */

/*!
 * @defgroup CustomDataKeys Custom Data Keys
 *
 * Pre-defined keys used internally in Carla.
 * @see CustomData::key
 * @{
 */

/*!
 * UI position key.
 */
static const char* const CUSTOM_DATA_KEY_UI_POSITION = "CarlaUiPosition";

/*!
 * UI size key.
 */
static const char* const CUSTOM_DATA_KEY_UI_SIZE = "CarlaUiSize";

/*!
 * UI visible key.
 */
static const char* const CUSTOM_DATA_KEY_UI_VISIBLE = "CarlaUiVisible";

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * Binary Type */

/*!
 * The binary type of a plugin.
 */
typedef enum {
    /*!
     * Null binary type.
     */
    BINARY_NONE = 0,

    /*!
     * POSIX 32bit binary.
     */
    BINARY_POSIX32 = 1,

    /*!
     * POSIX 64bit binary.
     */
    BINARY_POSIX64 = 2,

    /*!
     * Windows 32bit binary.
     */
    BINARY_WIN32 = 3,

    /*!
     * Windows 64bit binary.
     */
    BINARY_WIN64 = 4,

    /*!
     * Other binary type.
     */
    BINARY_OTHER = 5

} BinaryType;

/* ------------------------------------------------------------------------------------------------------------
 * File Type */

/*!
 * File type.
 */
typedef enum {
    /*!
     * Null file type.
     */
    CARLA_FILE_NONE = 0,

    /*!
     * Audio file.
     */
    CARLA_FILE_AUDIO = 1,

    /*!
     * MIDI file.
     */
    CARLA_FILE_MIDI = 2

} CarlaFileType;

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Type */

/*!
 * Plugin type.
 * Some files are handled as if they were plugins.
 */
typedef enum {
    /*!
     * Null plugin type.
     */
    PLUGIN_NONE = 0,

    /*!
     * Internal plugin.
     */
    PLUGIN_INTERNAL = 1,

    /*!
     * LADSPA plugin.
     */
    PLUGIN_LADSPA = 2,

    /*!
     * DSSI plugin.
     */
    PLUGIN_DSSI = 3,

    /*!
     * LV2 plugin.
     */
    PLUGIN_LV2 = 4,

    /*!
     * VST2 plugin.
     */
    PLUGIN_VST2 = 5,

    /*!
     * VST3 plugin.
     * @note Windows and MacOS only
     */
    PLUGIN_VST3 = 6,

    /*!
     * AU plugin.
     * @note MacOS only
     */
    PLUGIN_AU = 7,

    /*!
     * DLS file.
     */
    PLUGIN_DLS = 8,

    /*!
     * GIG file.
     */
    PLUGIN_GIG = 9,

    /*!
     * SF2/3 file (SoundFont).
     */
    PLUGIN_SF2 = 10,

    /*!
     * SFZ file.
     */
    PLUGIN_SFZ = 11,

    /*!
     * JACK application.
     */
    PLUGIN_JACK = 12

} PluginType;

/* ------------------------------------------------------------------------------------------------------------
 * Plugin Category */

/*!
 * Plugin category, which describes the functionality of a plugin.
 */
typedef enum {
    /*!
     * Null plugin category.
     */
    CARLA_PLUGIN_CATEGORY_NONE = 0,

    /*!
     * A synthesizer or generator.
     */
    CARLA_PLUGIN_CATEGORY_SYNTH = 1,

    /*!
     * A delay or reverb.
     */
    CARLA_PLUGIN_CATEGORY_DELAY = 2,

    /*!
     * An equalizer.
     */
    CARLA_PLUGIN_CATEGORY_EQ = 3,

    /*!
     * A filter.
     */
    CARLA_PLUGIN_CATEGORY_FILTER = 4,

    /*!
     * A distortion plugin.
     */
    CARLA_PLUGIN_CATEGORY_DISTORTION = 5,

    /*!
     * A 'dynamic' plugin (amplifier, compressor, gate, etc).
     */
    CARLA_PLUGIN_CATEGORY_DYNAMICS = 6,

    /*!
     * A 'modulator' plugin (chorus, flanger, phaser, etc).
     */
    CARLA_PLUGIN_CATEGORY_MODULATOR = 7,

    /*!
     * An 'utility' plugin (analyzer, converter, mixer, etc).
     */
    CARLA_PLUGIN_CATEGORY_UTILITY = 8,

    /*!
     * Miscellaneous plugin (used to check if the plugin has a category).
     */
    CARLA_PLUGIN_CATEGORY_OTHER = 9

} CarlaPluginCategory;

/* ------------------------------------------------------------------------------------------------------------
 * Parameter Type */

/*!
 * Plugin parameter type.
 */
typedef enum {
    /*!
     * Null parameter type.
     */
    PARAMETER_UNKNOWN = 0,

    /*!
     * Input parameter.
     */
    PARAMETER_INPUT = 1,

    /*!
     * Output parameter.
     */
    PARAMETER_OUTPUT = 2

} ParameterType;

/* ------------------------------------------------------------------------------------------------------------
 * Internal Parameter Index */

/*!
 * Special parameters used internally in Carla.
 * Plugins do not know about their existence.
 */
typedef enum {
    /*!
     * Null parameter.
     */
    PARAMETER_NULL = -1,

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    /*!
     * Active parameter, boolean type.
     * Default is 'false'.
     */
    PARAMETER_ACTIVE = -2,

    /*!
     * Dry/Wet parameter.
     * Range 0.0...1.0; default is 1.0.
     */
    PARAMETER_DRYWET = -3,

    /*!
     * Volume parameter.
     * Range 0.0...1.27; default is 1.0.
     */
    PARAMETER_VOLUME = -4,

    /*!
     * Stereo Balance-Left parameter.
     * Range -1.0...1.0; default is -1.0.
     */
    PARAMETER_BALANCE_LEFT = -5,

    /*!
     * Stereo Balance-Right parameter.
     * Range -1.0...1.0; default is 1.0.
     */
    PARAMETER_BALANCE_RIGHT = -6,

    /*!
     * Mono Panning parameter.
     * Range -1.0...1.0; default is 0.0.
     */
    PARAMETER_PANNING = -7,

    /*!
     * MIDI Control channel, integer type.
     * Range -1...15 (-1 = off).
     */
    PARAMETER_CTRL_CHANNEL = -8,
#endif

    /*!
     * Max value, defined only for convenience.
     */
    PARAMETER_MAX = -9

} InternalParameterIndex;

/* ------------------------------------------------------------------------------------------------------------
 * Special Mapped Control Index */

/*!
 * Specially designated mapped control indexes.
 * Values between 0 and 119 (0x77) are reserved for MIDI CC, which uses direct values.
 * @see ParameterData::mappedControlIndex
 */
typedef enum {
    /*!
     * Unused control index, meaning no mapping is enabled.
     */
    CONTROL_INDEX_NONE = -1,

    /*!
     * CV control index, meaning the parameter is exposed as CV port.
     */
    CONTROL_INDEX_CV = 130,

    /*!
     * Special value to indicate MIDI pitchbend.
     */
    CONTROL_INDEX_MIDI_PITCHBEND = 131,

    /*!
     * Highest index allowed for mappings.
     */
    CONTROL_INDEX_MAX_ALLOWED = CONTROL_INDEX_MIDI_PITCHBEND

} SpecialMappedControlIndex;

/* ------------------------------------------------------------------------------------------------------------
 * Engine Callback Opcode */

/*!
 * Engine callback opcodes.
 * Front-ends must never block indefinitely during a callback.
 * @see EngineCallbackFunc, CarlaEngine::setCallback() and carla_set_engine_callback()
 */
typedef enum {
    /*!
     * Debug.
     * This opcode is undefined and used only for testing purposes.
     */
    ENGINE_CALLBACK_DEBUG = 0,

    /*!
     * A plugin has been added.
     * @a pluginId Plugin Id
     * @a valueStr Plugin name
     */
    ENGINE_CALLBACK_PLUGIN_ADDED = 1,

    /*!
     * A plugin has been removed.
     * @a pluginId Plugin Id
     */
    ENGINE_CALLBACK_PLUGIN_REMOVED = 2,

    /*!
     * A plugin has been renamed.
     * @a pluginId Plugin Id
     * @a valueStr New plugin name
     */
    ENGINE_CALLBACK_PLUGIN_RENAMED = 3,

    /*!
     * A plugin has become unavailable.
     * @a pluginId Plugin Id
     * @a valueStr Related error string
     */
    ENGINE_CALLBACK_PLUGIN_UNAVAILABLE = 4,

    /*!
     * A parameter value has changed.
     * @a pluginId Plugin Id
     * @a value1   Parameter index
     * @a valuef   New parameter value
     */
    ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED = 5,

    /*!
     * A parameter default has changed.
     * @a pluginId Plugin Id
     * @a value1   Parameter index
     * @a valuef   New default value
     */
    ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED = 6,

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    /*!
     * A parameter's mapped control index has changed.
     * @a pluginId Plugin Id
     * @a value1   Parameter index
     * @a value2   New control index
     */
    ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED = 7,

    /*!
     * A parameter's MIDI channel has changed.
     * @a pluginId Plugin Id
     * @a value1   Parameter index
     * @a value2   New MIDI channel
     */
    ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 8,

    /*!
     * A plugin option has changed.
     * @a pluginId Plugin Id
     * @a value1   Option
     * @a value2   New on/off state (1 for on, 0 for off)
     * @see PluginOptions
     */
    ENGINE_CALLBACK_OPTION_CHANGED = 9,
#endif

    /*!
     * The current program of a plugin has changed.
     * @a pluginId Plugin Id
     * @a value1   New program index
     */
    ENGINE_CALLBACK_PROGRAM_CHANGED = 10,

    /*!
     * The current MIDI program of a plugin has changed.
     * @a pluginId Plugin Id
     * @a value1   New MIDI program index
     */
    ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED = 11,

    /*!
     * A plugin's custom UI state has changed.
     * @a pluginId Plugin Id
     * @a value1   New state, as follows:
     *                  0: UI is now hidden
     *                  1: UI is now visible
     *                 -1: UI has crashed and should not be shown again
     */
    ENGINE_CALLBACK_UI_STATE_CHANGED = 12,

    /*!
     * A note has been pressed.
     * @a pluginId Plugin Id
     * @a value1   Channel
     * @a value2   Note
     * @a value3   Velocity
     */
    ENGINE_CALLBACK_NOTE_ON = 13,

    /*!
     * A note has been released.
     * @a pluginId Plugin Id
     * @a value1   Channel
     * @a value2   Note
     */
    ENGINE_CALLBACK_NOTE_OFF = 14,

    /*!
     * A plugin needs update.
     * @a pluginId Plugin Id
     */
    ENGINE_CALLBACK_UPDATE = 15,

    /*!
     * A plugin's data/information has changed.
     * @a pluginId Plugin Id
     */
    ENGINE_CALLBACK_RELOAD_INFO = 16,

    /*!
     * A plugin's parameters have changed.
     * @a pluginId Plugin Id
     */
    ENGINE_CALLBACK_RELOAD_PARAMETERS = 17,

    /*!
     * A plugin's programs have changed.
     * @a pluginId Plugin Id
     */
    ENGINE_CALLBACK_RELOAD_PROGRAMS = 18,

    /*!
     * A plugin state has changed.
     * @a pluginId Plugin Id
     */
    ENGINE_CALLBACK_RELOAD_ALL = 19,

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    /*!
     * A patchbay client has been added.
     * @a pluginId Client Id
     * @a value1   Client icon
     * @a value2   Plugin Id (-1 if not a plugin)
     * @a valueStr Client name
     * @see PatchbayIcon
     */
    ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED = 20,

    /*!
     * A patchbay client has been removed.
     * @a pluginId Client Id
     */
    ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED = 21,

    /*!
     * A patchbay client has been renamed.
     * @a pluginId Client Id
     * @a valueStr New client name
     */
    ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED = 22,

    /*!
     * A patchbay client data has changed.
     * @a pluginId Client Id
     * @a value1   New icon
     * @a value2   New plugin Id (-1 if not a plugin)
     * @see PatchbayIcon
     */
    ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED = 23,

    /*!
     * A patchbay port has been added.
     * @a pluginId Client Id
     * @a value1   Port Id
     * @a value2   Port hints
     * @a value3   Port group Id (0 for none)
     * @a valueStr Port name
     * @see PatchbayPortHints
     */
    ENGINE_CALLBACK_PATCHBAY_PORT_ADDED = 24,

    /*!
     * A patchbay port has been removed.
     * @a pluginId Client Id
     * @a value1   Port Id
     */
    ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED = 25,

    /*!
     * A patchbay port has changed (like the name or group Id).
     * @a pluginId Client Id
     * @a value1   Port Id
     * @a value2   Port hints
     * @a value3   Port group Id (0 for none)
     * @a valueStr Port name
     */
    ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED = 26,

    /*!
     * A patchbay connection has been added.
     * @a pluginId Connection Id
     * @a valueStr Out group and port plus in group and port, in "og:op:ig:ip" syntax.
     */
    ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED = 27,

    /*!
     * A patchbay connection has been removed.
     * @a pluginId Connection Id
     */
    ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED = 28,
#endif

    /*!
     * Engine started.
     * @a pluginId How many plugins are known to be running
     * @a value1   Process mode
     * @a value2   Transport mode
     * @a value3   Buffer size
     * @a valuef   Sample rate
     * @a valuestr Engine driver
     * @see EngineProcessMode
     * @see EngineTransportMode
     */
    ENGINE_CALLBACK_ENGINE_STARTED = 29,

    /*!
     * Engine stopped.
     */
    ENGINE_CALLBACK_ENGINE_STOPPED = 30,

    /*!
     * Engine process mode has changed.
     * @a value1 New process mode
     * @see EngineProcessMode
     */
    ENGINE_CALLBACK_PROCESS_MODE_CHANGED = 31,

    /*!
     * Engine transport mode has changed.
     * @a value1   New transport mode
     * @a valueStr New transport features enabled
     * @see EngineTransportMode
     */
    ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED = 32,

    /*!
     * Engine buffer-size changed.
     * @a value1 New buffer size
     */
    ENGINE_CALLBACK_BUFFER_SIZE_CHANGED = 33,

    /*!
     * Engine sample-rate changed.
     * @a valuef New sample rate
     */
    ENGINE_CALLBACK_SAMPLE_RATE_CHANGED = 34,

    /*!
     * A cancelable action has been started or stopped.
     * @a pluginId Plugin Id the action relates to, -1 for none
     * @a value1   1 for action started, 0 for stopped
     * @a valueStr Action name
     */
    ENGINE_CALLBACK_CANCELABLE_ACTION = 35,

    /*!
     * Project has finished loading.
     */
    ENGINE_CALLBACK_PROJECT_LOAD_FINISHED = 36,

    /*!
     * NSM callback, to be handled by a frontend.
     * Frontend must call carla_nsm_ready() with opcode as parameter as a response
     * @a value1   NSM opcode
     * @a value2   Integer value
     * @a valueStr String value
     * @see NsmCallbackOpcode
     */
    ENGINE_CALLBACK_NSM = 37,

    /*!
     * Idle frontend.
     * This is used by the engine during long operations that might block the frontend,
     * giving it the possibility to idle while the operation is still in place.
     */
    ENGINE_CALLBACK_IDLE = 38,

    /*!
     * Show a message as information.
     * @a valueStr The message
     */
    ENGINE_CALLBACK_INFO = 39,

    /*!
     * Show a message as an error.
     * @a valueStr The message
     */
    ENGINE_CALLBACK_ERROR = 40,

    /*!
     * The engine has crashed or malfunctioned and will no longer work.
     */
    ENGINE_CALLBACK_QUIT = 41,

    /*!
     * A plugin requested for its inline display to be redrawn.
     * @a pluginId Plugin Id to redraw
     */
    ENGINE_CALLBACK_INLINE_DISPLAY_REDRAW = 42,

    /*!
     * A patchbay port group has been added.
     * @a pluginId Client Id
     * @a value1   Group Id (unique value within this client)
     * @a value2   Group hints
     * @a valueStr Group name
     * @see PatchbayPortGroupHints
     */
    ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_ADDED = 43,

    /*!
     * A patchbay port group has been removed.
     * @a pluginId Client Id
     * @a value1   Group Id
     */
    ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_REMOVED = 44,

    /*!
     * A patchbay port group has changed.
     * @a pluginId Client Id
     * @a value1   Group Id
     * @a value2   Group hints
     * @a valueStr Group name
     * @see PatchbayPortGroupHints
     */
    ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_CHANGED = 45,

    /*!
     * A parameter's mapped range has changed.
     * @a pluginId Plugin Id
     * @a value1   Parameter index
     * @a valueStr New mapped range as "%f:%f" syntax
     */
    ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED = 46

} EngineCallbackOpcode;

/* ------------------------------------------------------------------------------------------------------------
 * NSM Callback Opcode */

/*!
 * NSM callback opcodes.
 * @see ENGINE_CALLBACK_NSM
 */
typedef enum {
    /*!
     * NSM is available and initialized.
     */
    NSM_CALLBACK_INIT = 0,

    /*!
     * Error from NSM side.
     * @a valueInt Error code
     * @a valueStr Error string
     */
    NSM_CALLBACK_ERROR = 1,

    /*!
     * Announce message.
     * @a valueInt SM Flags (WIP, to be defined)
     * @a valueStr SM Name
     */
    NSM_CALLBACK_ANNOUNCE = 2,

    /*!
     * Open message.
     * @a valueStr Project filename
     */
    NSM_CALLBACK_OPEN = 3,

    /*!
     * Save message.
     */
    NSM_CALLBACK_SAVE = 4,

    /*!
     * Session-is-loaded message.
     */
    NSM_CALLBACK_SESSION_IS_LOADED = 5,

    /*!
     * Show-optional-gui message.
     */
    NSM_CALLBACK_SHOW_OPTIONAL_GUI = 6,

    /*!
     * Hide-optional-gui message.
     */
    NSM_CALLBACK_HIDE_OPTIONAL_GUI = 7

} NsmCallbackOpcode;

/* ------------------------------------------------------------------------------------------------------------
 * Engine Option */

/*!
 * Engine options.
 * @see CarlaEngine::getOptions(), CarlaEngine::setOption() and carla_set_engine_option()
 */
typedef enum {
    /*!
     * Debug.
     * This option is undefined and used only for testing purposes.
     */
    ENGINE_OPTION_DEBUG = 0,

    /*!
     * Set the engine processing mode.
     * Default is ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS on Linux and ENGINE_PROCESS_MODE_PATCHBAY for all other OSes.
     * @see EngineProcessMode
     */
    ENGINE_OPTION_PROCESS_MODE = 1,

    /*!
     * Set the engine transport mode.
     * Default is ENGINE_TRANSPORT_MODE_JACK on Linux and ENGINE_TRANSPORT_MODE_INTERNAL for all other OSes.
     * @see EngineTransportMode
     */
    ENGINE_OPTION_TRANSPORT_MODE = 2,

    /*!
     * Force mono plugins as stereo, by running 2 instances at the same time.
     * Default is false, but always true when process mode is ENGINE_PROCESS_MODE_CONTINUOUS_RACK.
     * @note Not supported by all plugins
     * @see PLUGIN_OPTION_FORCE_STEREO
     */
    ENGINE_OPTION_FORCE_STEREO = 3,

    /*!
     * Use plugin bridges whenever possible.
     * Default is no, EXPERIMENTAL.
     */
    ENGINE_OPTION_PREFER_PLUGIN_BRIDGES = 4,

    /*!
     * Use UI bridges whenever possible, otherwise UIs will be directly handled in the main backend thread.
     * Default is yes.
     */
    ENGINE_OPTION_PREFER_UI_BRIDGES = 5,

    /*!
     * Make custom plugin UIs always-on-top.
     * Default is yes.
     */
    ENGINE_OPTION_UIS_ALWAYS_ON_TOP = 6,

    /*!
     * Maximum number of parameters allowed.
     * Default is MAX_DEFAULT_PARAMETERS.
     */
    ENGINE_OPTION_MAX_PARAMETERS = 7,

    /*!
     * Reset Xrun counter after project load.
     */
    ENGINE_OPTION_RESET_XRUNS = 8,

    /*!
     * Timeout value for how much to wait for UI bridges to respond, in milliseconds.
     * Default is 4000 (4 seconds).
     */
    ENGINE_OPTION_UI_BRIDGES_TIMEOUT = 9,

    /*!
     * Audio buffer size.
     * Default is 512.
     */
    ENGINE_OPTION_AUDIO_BUFFER_SIZE = 10,

    /*!
     * Audio sample rate.
     * Default is 44100.
     */
    ENGINE_OPTION_AUDIO_SAMPLE_RATE = 11,

    /*!
     * Wherever to use 3 audio periods instead of the default 2.
     * Default is false.
     */
    ENGINE_OPTION_AUDIO_TRIPLE_BUFFER = 12,

    /*!
     * Audio driver.
     * Default depends on platform.
     */
    ENGINE_OPTION_AUDIO_DRIVER = 13,

    /*!
     * Audio device (within a driver).
     * Default unset.
     */
    ENGINE_OPTION_AUDIO_DEVICE = 14,

    /*!
     * Wherever to enable OSC support in the engine.
     */
    ENGINE_OPTION_OSC_ENABLED = 15,

    /*!
     * The network TCP port to use for OSC.
     * A value of 0 means use a random port.
     * A value of < 0 means to not enable the TCP port for OSC.
     * @note Valid ports begin at 1024 and end at 32767 (inclusive)
     */
    ENGINE_OPTION_OSC_PORT_TCP = 16,

    /*!
     * The network UDP port to use for OSC.
     * A value of 0 means use a random port.
     * A value of < 0 means to not enable the UDP port for OSC.
     * @note Disabling this option prevents DSSI UIs from working!
     * @note Valid ports begin at 1024 and end at 32767 (inclusive)
     */
    ENGINE_OPTION_OSC_PORT_UDP = 17,

    /*!
     * Set path used for a specific file type.
     * Uses value as the file format, valueStr as actual path.
     */
    ENGINE_OPTION_FILE_PATH = 18,

    /*!
     * Set path used for a specific plugin type.
     * Uses value as the plugin format, valueStr as actual path.
     * @see PluginType
     */
    ENGINE_OPTION_PLUGIN_PATH = 19,

    /*!
     * Set path to the binary files.
     * Default unset.
     * @note Must be set for plugin and UI bridges to work
     */
    ENGINE_OPTION_PATH_BINARIES = 20,

    /*!
     * Set path to the resource files.
     * Default unset.
     * @note Must be set for some internal plugins to work
     */
    ENGINE_OPTION_PATH_RESOURCES = 21,

    /*!
     * Prevent bad plugin and UI behaviour.
     * @note: Linux only
     */
    ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR = 22,

    /*!
     * Set UI scaling used in frontend, so backend can do the same for plugin UIs.
     */
    ENGINE_OPTION_FRONTEND_UI_SCALE = 23,

    /*!
     * Set frontend winId, used to define as parent window for plugin UIs.
     */
    ENGINE_OPTION_FRONTEND_WIN_ID = 24,

#if !defined(BUILD_BRIDGE_ALTERNATIVE_ARCH) && !defined(CARLA_OS_WIN)
    /*!
     * Set path to wine executable.
     */
    ENGINE_OPTION_WINE_EXECUTABLE = 25,

    /*!
     * Enable automatic wineprefix detection.
     */
    ENGINE_OPTION_WINE_AUTO_PREFIX = 26,

    /*!
     * Fallback wineprefix to use if automatic detection fails or is disabled, and WINEPREFIX is not set.
     */
    ENGINE_OPTION_WINE_FALLBACK_PREFIX = 27,

    /*!
     * Enable realtime priority for Wine application and server threads.
     */
    ENGINE_OPTION_WINE_RT_PRIO_ENABLED = 28,

    /*!
     * Base realtime priority for Wine threads.
     */
    ENGINE_OPTION_WINE_BASE_RT_PRIO = 29,

    /*!
     * Wine server realtime priority.
     */
    ENGINE_OPTION_WINE_SERVER_RT_PRIO = 30,
#endif

    /*!
     * Capture console output into debug callbacks.
     */
    ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT = 31

} EngineOption;

/* ------------------------------------------------------------------------------------------------------------
 * Engine Process Mode */

/*!
 * Engine process mode.
 * @see ENGINE_OPTION_PROCESS_MODE
 */
typedef enum {
    /*!
     * Single client mode.
     * Inputs and outputs are added dynamically as needed by plugins.
     */
    ENGINE_PROCESS_MODE_SINGLE_CLIENT = 0,

    /*!
     * Multiple client mode.
     * It has 1 master client + 1 client per plugin.
     */
    ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS = 1,

    /*!
     * Single client, 'rack' mode.
     * Processes plugins in order of Id, with forced stereo always on.
     */
    ENGINE_PROCESS_MODE_CONTINUOUS_RACK = 2,

    /*!
     * Single client, 'patchbay' mode.
     */
    ENGINE_PROCESS_MODE_PATCHBAY = 3,

    /*!
     * Special mode, used in plugin-bridges only.
     */
    ENGINE_PROCESS_MODE_BRIDGE = 4

} EngineProcessMode;

/* ------------------------------------------------------------------------------------------------------------
 * Engine Transport Mode */

/*!
 * Engine transport mode.
 * @see ENGINE_OPTION_TRANSPORT_MODE
 */
typedef enum {
    /*!
     * No transport.
     */
    ENGINE_TRANSPORT_MODE_DISABLED = 0,

    /*!
     * Internal transport mode.
     */
    ENGINE_TRANSPORT_MODE_INTERNAL = 1,

    /*!
     * Transport from JACK.
     * Only available if driver name is "JACK".
     */
    ENGINE_TRANSPORT_MODE_JACK = 2,

    /*!
     * Transport from host, used when Carla is a plugin.
     */
    ENGINE_TRANSPORT_MODE_PLUGIN = 3,

    /*!
     * Special mode, used in plugin-bridges only.
     */
    ENGINE_TRANSPORT_MODE_BRIDGE = 4

} EngineTransportMode;

/* ------------------------------------------------------------------------------------------------------------
 * File Callback Opcode */

/*!
 * File callback opcodes.
 * Front-ends must always block-wait for user input.
 * @see FileCallbackFunc, CarlaEngine::setFileCallback() and carla_set_file_callback()
 */
typedef enum {
    /*!
     * Debug.
     * This opcode is undefined and used only for testing purposes.
     */
    FILE_CALLBACK_DEBUG = 0,

    /*!
     * Open file or folder.
     */
    FILE_CALLBACK_OPEN = 1,

    /*!
     * Save file or folder.
     */
    FILE_CALLBACK_SAVE = 2

} FileCallbackOpcode;

/* ------------------------------------------------------------------------------------------------------------
 * Patchbay Icon */

/*!
 * The icon of a patchbay client/group.
 */
enum PatchbayIcon {
    /*!
     * Generic application icon.
     * Used for all non-plugin clients that don't have a specific icon.
     */
    PATCHBAY_ICON_APPLICATION = 0,

    /*!
     * Plugin icon.
     * Used for all plugin clients that don't have a specific icon.
     */
    PATCHBAY_ICON_PLUGIN = 1,

    /*!
     * Hardware icon.
     * Used for hardware (audio or MIDI) clients.
     */
    PATCHBAY_ICON_HARDWARE = 2,

    /*!
     * Carla icon.
     * Used for the main app.
     */
    PATCHBAY_ICON_CARLA = 3,

    /*!
     * DISTRHO icon.
     * Used for DISTRHO based plugins.
     */
    PATCHBAY_ICON_DISTRHO = 4,

    /*!
     * File icon.
     * Used for file type plugins (like SF2 snd SFZ).
     */
    PATCHBAY_ICON_FILE = 5
};

/* ------------------------------------------------------------------------------------------------------------
 * Carla Backend API (C stuff) */

/*!
 * Engine callback function.
 * Front-ends must never block indefinitely during a callback.
 * @see EngineCallbackOpcode, CarlaEngine::setCallback() and carla_set_engine_callback()
 */
typedef void (*EngineCallbackFunc)(void* ptr, EngineCallbackOpcode action, uint pluginId,
                                   int value1, int value2, int value3,
                                   float valuef, const char* valueStr);

/*!
 * File callback function.
 * @see FileCallbackOpcode
 */
typedef const char* (*FileCallbackFunc)(void* ptr, FileCallbackOpcode action, bool isDir, const char* title, const char* filter);

/*!
 * Parameter data.
 */
typedef struct {
    /*!
     * This parameter type.
     */
    ParameterType type;

    /*!
     * This parameter hints.
     * @see ParameterHints
     */
    uint hints;

    /*!
     * Index as seen by Carla.
     */
    int32_t index;

    /*!
     * Real index as seen by plugins.
     */
    int32_t rindex;

    /*!
     * Currently mapped MIDI channel.
     * Counts from 0 to 15.
     */
    uint8_t midiChannel;

    /*!
     * Currently mapped index.
     * @see SpecialMappedControlIndex
     */
    int16_t mappedControlIndex;

    /*!
     * Minimum value that this parameter maps to.
     */
    float mappedMinimum;

    /*!
     * Maximum value that this parameter maps to.
     */
    float mappedMaximum;

} ParameterData;

/*!
 * Parameter ranges.
 */
typedef struct _ParameterRanges {
    /*!
     * Default value.
     */
    float def;

    /*!
     * Minimum value.
     */
    float min;

    /*!
     * Maximum value.
     */
    float max;

    /*!
     * Regular, single step value.
     */
    float step;

    /*!
     * Small step value.
     */
    float stepSmall;

    /*!
     * Large step value.
     */
    float stepLarge;

#ifdef __cplusplus
    /*!
     * Fix the default value within range.
     */
    void fixDefault() noexcept
    {
        fixValue(def);
    }

    /*!
     * Fix a value within range.
     */
    void fixValue(float& value) const noexcept
    {
        if (value < min)
            value = min;
        else if (value > max)
            value = max;
    }

    /*!
     * Get a fixed value within range.
     */
    const float& getFixedValue(const float& value) const noexcept
    {
        if (value <= min)
            return min;
        if (value >= max)
            return max;
        return value;
    }

    /*!
     * Get a value normalized to 0.0<->1.0.
     */
    float getNormalizedValue(const float& value) const noexcept
    {
        const float normValue((value - min) / (max - min));

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;
        return normValue;
    }

    /*!
     * Get a value normalized to 0.0<->1.0, fixed within range.
     */
    float getFixedAndNormalizedValue(const float& value) const noexcept
    {
        if (value <= min)
            return 0.0f;
        if (value >= max)
            return 1.0f;

        const float normValue((value - min) / (max - min));

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;

        return normValue;
    }

    /*!
     * Get a proper value previously normalized to 0.0<->1.0.
     */
    float getUnnormalizedValue(const float& value) const noexcept
    {
        if (value <= 0.0f)
            return min;
        if (value >= 1.0f)
            return max;

        return value * (max - min) + min;
    }

    /*!
     * Get a logarithmic value previously normalized to 0.0<->1.0.
     */
    float getUnnormalizedLogValue(const float& value) const noexcept
    {
        if (value <= 0.0f)
            return min;
        if (value >= 1.0f)
            return max;

        float rmin = min;

        if (std::abs(min) < std::numeric_limits<float>::epsilon())
            rmin = 0.00001f;

        return rmin * std::pow(max/rmin, value);
    }
#endif /* __cplusplus */

} ParameterRanges;

/*!
 * MIDI Program data.
 */
typedef struct {
    /*!
     * MIDI bank.
     */
    uint32_t bank;

    /*!
     * MIDI program.
     */
    uint32_t program;

    /*!
     * MIDI program name.
     */
    const char* name;

} MidiProgramData;

/*!
 * Custom data, used for saving key:value 'dictionaries'.
 */
typedef struct {
    /*!
     * Value type, in URI form.
     * @see CustomDataTypes
     */
    const char* type;

    /*!
     * Key.
     * @see CustomDataKeys
     */
    const char* key;

    /*!
     * Value.
     */
    const char* value;

#ifdef __cplusplus
    /*!
     * Check if valid.
     */
    bool isValid() const noexcept
    {
        if (type  == nullptr || type[0] == '\0') return false;
        if (key   == nullptr || key [0] == '\0') return false;
        if (value == nullptr)                    return false;
        return true;
    }
#endif /* __cplusplus */

} CustomData;

/*!
 * Engine driver device information.
 */
typedef struct {
    /*!
     * This driver device hints.
     * @see EngineDriverHints
     */
    uint hints;

    /*!
     * Available buffer sizes.
     * Terminated with 0.
     */
    const uint32_t* bufferSizes;

    /*!
     * Available sample rates.
     * Terminated with 0.0.
     */
    const double* sampleRates;

} EngineDriverDeviceInfo;

/** @} */

#ifdef __cplusplus
/* Forward declarations of commonly used Carla classes */
class CarlaEngine;
class CarlaEngineClient;
class CarlaEngineCVSourcePorts;
class CarlaPlugin;
/* End namespace */
CARLA_BACKEND_END_NAMESPACE
#endif

#endif /* CARLA_BACKEND_H_INCLUDED */
