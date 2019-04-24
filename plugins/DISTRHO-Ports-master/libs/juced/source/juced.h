/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#ifndef __JUCED_JUCEHEADER__
#define __JUCED_JUCEHEADER__

#include "../../juce/source/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../juce/source/modules/juce_audio_devices/juce_audio_devices.h"
#include "../../juce/source/modules/juce_audio_formats/juce_audio_formats.h"
#include "../../juce/source/modules/juce_audio_processors/juce_audio_processors.h"
#include "../../juce/source/modules/juce_audio_utils/juce_audio_utils.h"
#include "../../juce/source/modules/juce_core/juce_core.h"
#include "../../juce/source/modules/juce_data_structures/juce_data_structures.h"
#include "../../juce/source/modules/juce_events/juce_events.h"
#include "../../juce/source/modules/juce_graphics/juce_graphics.h"
#include "../../juce/source/modules/juce_gui_basics/juce_gui_basics.h"
#include "../../juce/source/modules/juce_gui_extra/juce_gui_extra.h"

#define BEGIN_JUCE_NAMESPACE namespace juce {
#define END_JUCE_NAMESPACE }

//=============================================================================
/** Config: JUCE_LASH
	Enables LASH support on Linux.
	Not enabled by default.
 
	@see LashManager
*/
#ifndef JUCE_LASH
 #define JUCE_LASH 0
#endif

/** Config: JUCE_USE_GLX
	Enable this under Linux to use GLX for fast openGL rendering with alpha
	compositing support over a composite manager (compiz / xcompmgr).
	Not enabled by default.
*/
#ifndef JUCE_USE_GLX
 #define JUCE_USE_GLX 0
#endif

/** Config: JUCE_SUPPORT_SQLITE
	Setting this allows the build to use SQLITE libraries for access a self-contained,
	serverless, zero-configuration, transactional SQL database engine.
	Not enabled by default.
*/
#ifndef JUCE_SUPPORT_SQLITE
 #define JUCE_SUPPORT_SQLITE 0
#endif

/** Config: JUCE_SUPPORT_SCRIPTING
	Setting this allows the build to use Angelscript library for using scripting
	inside the juce library itself
	Not enabled by default.
*/
#ifndef JUCE_SUPPORT_SCRIPTING
 #define JUCE_SUPPORT_SCRIPTING 0
#endif

/** Config: JUCETICE_INCLUDE_ANGELSCRIPT_CODE
 Enables direct inclusion of the angelscript library.
 Enabled by default.
 
 @see angelscript
*/
#ifndef JUCETICE_INCLUDE_ANGELSCRIPT_CODE
 #define JUCETICE_INCLUDE_ANGELSCRIPT_CODE  1
#endif

/** Config: JUCETICE_INCLUDE_CURL_CODE
 Enables direct inclusion of curl.
 
 // Currently not available //
 
 @see cUrl
*/
#ifndef JUCETICE_INCLUDE_CURL_CODE
 #define JUCETICE_INCLUDE_CURL_CODE 1
#endif
//=============================================================================
//=============================================================================
BEGIN_JUCE_NAMESPACE

// START_AUTOINCLUDE

#ifndef __JUCETICE_STANDARDHEADER_HEADER__
 #include "base/jucetice_StandardHeader.h"
#endif
#ifndef __JUCETICE_AUDIOPARAMETER_HEADER__
 #include "base/jucetice_AudioParameter.h"
#endif
#ifndef __JUCETICE_AUDIOPLUGIN_HEADER__
 #include "base/jucetice_AudioPlugin.h"
#endif
#ifndef __JUCETICE_AUDIOPROCESSINGBUFFER_HEADER__
 #include "base/jucetice_AudioProcessingBuffer.h"
#endif
#ifndef __JUCETICE_AUDIOPROGRAM_HEADER__
 #include "base/jucetice_AudioProgram.h"
#endif
#ifndef __JUCETICE_EXTERNALTRANSPORT_HEADER__
 #include "base/jucetice_ExternalTransport.h"
#endif
#ifndef __JUCETICE_MATHCONSTANTS_HEADER__
 #include "base/jucetice_MathConstants.h"
#endif
//=============================================================================
#ifndef __JUCETICE_BEATDETECTOR_HEADER__
 #include "audio/beat/jucetice_BeatDetector.h"
#endif
#ifndef __JUCETICE_FFTWRAPPER_HEADER__
 #include "audio/fft/jucetice_FFTWrapper.h"
#endif

#ifndef __JUCETICE_LASHMANAGER_HEADER__
 #include "audio/lash/jucetice_LashManager.h"
#endif
#ifndef __JUCETICE_MIDI_LEARN_HEADER__
 #include "audio/midi/jucetice_MidiAutomatorManager.h"
#endif
#ifndef __JUCETICE_MIDI_FILTER_HEADER__
 #include "audio/midi/jucetice_MidiFilter.h"
#endif
#ifndef __JUCETICE_MIDI_MANIPULATOR_HEADER__
 #include "audio/midi/jucetice_MidiManipulator.h"
#endif
#ifndef __JUCETICE_MIDI_TRANSFORM_HEADER__
 #include "audio/midi/jucetice_MidiTransform.h"
#endif
#ifndef __JUCETICE_TUNINGMAP_HEADER__
 #include "audio/midi/jucetice_TuningMap.h"
#endif
#ifndef __JUCETICE_OPENSOUNDBASE_HEADER__ 
 #include "audio/osc/jucetice_OpenSoundBase.h"
#endif
#ifndef __JUCETICE_OPENSOUNDBUNDLE_HEADER__
 #include "audio/osc/jucetice_OpenSoundBundle.h"
#endif
#ifndef __JUCETICE_OPENSOUNDCONTROLLER_HEADER__
 #include "audio/osc/jucetice_OpenSoundController.h"
#endif
#ifndef __JUCETICE_OPENSOUNDMESSAGE_HEADER__
 #include "audio/osc/jucetice_OpenSoundMessage.h"
#endif
#ifndef __JUCETICE_OPENSOUNDTIMETAG_HEADER__
 #include "audio/osc/jucetice_OpenSoundTimeTag.h"
#endif
#ifndef __JUCETICE_UDPSOCKET_HEADER__
 #include "audio/osc/jucetice_UDPSocket.h"
#endif
#ifndef __JUCETICE_AUDIOSOURCEPROCESSOR_HEADER__
 #include "audio/processors/jucetice_AudioSourceProcessor.h"
#endif
//=============================================================================
#ifndef __JUCETICE_CIRCULARBUFFER_JUCEHEADER__
 #include "containers/jucetice_CircularBuffer.h"
#endif
#ifndef __JUCETICE_HASH_JUCEHEADER__
 #include "containers/jucetice_Hash.h"
#endif
#ifndef __JUCETICE_LOCKFREEFIFO_HEADER__
 #include "containers/jucetice_LockFreeFifo.h"
#endif
#ifndef __JUCETICE_LOOKUPTABLE_HEADER__
 #include "containers/jucetice_LookupTable.h"
#endif
#ifndef __JUCETICE_OWNEDHASH_JUCEHEADER__
 #include "containers/jucetice_OwnedHash.h"
#endif
#ifndef __JUCETICE_SHAREDPOINTER_JUCEHEADER__
 #include "containers/jucetice_SharedPointer.h"
#endif
//=============================================================================
#ifndef __JUCETICE_AUDIOSCOPECOMPONENT_HEADER__
 #include "controls/jucetice_AudioScopeComponent.h"
#endif
#ifndef __JUCETICE_IMAGEKNOB_HEADER__
 #include "controls/jucetice_ImageKnob.h"
#endif
#ifndef __JUCETICE_IMAGESLIDER_HEADER__
 #include "controls/jucetice_ImageSlider.h"
#endif
#ifndef __JUCETICE_JOYSTICK_HEADER__
 #include "controls/jucetice_Joystick.h"
#endif
#ifndef __JUCETICE_PARAMETERCOMBOBOX_HEADER__
 #include "controls/jucetice_ParameterComboBox.h"
#endif
#ifndef __JUCETICE_PARAMETERJOYSTICK_HEADER__
 #include "controls/jucetice_ParameterJoystick.h"
#endif
#ifndef __JUCETICE_PARAMETERLEDBUTTON_HEADER__
 #include "controls/jucetice_ParameterLedButton.h"
#endif
#ifndef __JUCETICE_PARAMETERSLIDER_HEADER__
 #include "controls/jucetice_ParameterSlider.h"
#endif
#ifndef __JUCETICE_PARAMETERTOGGLEBUTTON_HEADER__
 #include "controls/jucetice_ParameterToggleButton.h"
#endif
#ifndef __JUCETICE_SPECTRUMANAYZER_HEADER__
 #include "controls/jucetice_SpectrumAnalyzer.h"
#endif

#ifndef __JUCETICE_COORDINATESYSTEM_HEADER__
 #include "controls/coordinate/jucetice_CoordinateSystem.h"
#endif
#ifndef __JUCETICE_COORDINATESYSREMDEMOCONTENTCOMPONENT_HEADER__
 #include "controls/coordinate/jucetice_CoordinateSystemDemoContentComponent.h"
#endif
#ifndef __JUCETICE_COORDINATESYSTEMRANGE_HEADER__
 #include "controls/coordinate/jucetice_CoordinateSystemRange.h"
#endif
#ifndef __JUCETICE_HELPERFUNCTIONS_JUCEHEADER__
 #include "controls/coordinate/jucetice_HelperFunctions.h"
#endif
#ifndef __JUCETICE_STRINGTOOLS_HEADER__
 #include "controls/coordinate/jucetice_StringTools.h"
#endif
#ifndef __JUCETICE_SYMBOLBUTTON_HEADER__
 #include "controls/coordinate/jucetice_SymbolButton.h"
#endif

#ifndef __JUCETICE_GRAPHCONNECTORCOMPONENT_HEADER__
 #include "controls/graph/jucetice_GraphConnectorComponent.h"
#endif
#ifndef __JUCETICE_GRAPHLINKCOMPONENT_HEADER__
 #include "controls/graph/jucetice_GraphLinkComponent.h"
#endif
#ifndef __JUCETICE_GRAPHNODECOMPONENT_HEADER__
 #include "controls/graph/jucetice_GraphNodeComponent.h"
#endif
#ifndef __JUCETICE_GRAPHNODELISTENER_HEADER__
 #include "controls/graph/jucetice_GraphNodeListener.h"
#endif

#ifndef __JUCETICE_PIANOGRID_HEADER__
 #include "controls/grid/jucetice_PianoGrid.h"
#endif
#ifndef __JUCETICE_PIANOGRIDHEADER_HEADER__
 #include "controls/grid/jucetice_PianoGridHeader.h"
#endif
#ifndef __JUCETICE_PIANOGRIDINDICATOR_HEADER__
 #include "controls/grid/jucetice_PianoGridIndicator.h"
#endif
#ifndef __JUCETICE_PIANOGRIDKEYBOARD_HEADER__
 #include "controls/grid/jucetice_PianoGridKeyboard.h"
#endif
#ifndef __JUCETICE_PIANOGRIDNOTE_HEADER__
 #include "controls/grid/jucetice_PianoGridNote.h"
#endif

#ifndef __JUCETICE_COMPONENTLAYOUTMANAGER_HEADER__
 #include "controls/layout/jucetice_ComponentLayoutEditor.h"
#endif
#ifndef __JUCETICE_DOCK_HEADER__
 #include "controls/layout/jucetice_Dock.h"
#endif
#ifndef __JUCETICE_VIEWPORT_HEADER__
 #include "controls/layout/jucetice_Viewport.h"
#endif
#ifndef __JUCETICE_VIEWPORTNAVIGATOR_HEADER__
 #include "controls/layout/jucetice_ViewportNavigator.h"
#endif

#ifndef __JUCETICE_DECIBELSCALE_HEADER__
 #include "controls/meter/jucetice_DecibelScale.h"
#endif
#ifndef __JUCETICE_HIGHQUALITYMETER_HEADER__
 #include "controls/meter/jucetice_HighQualityMeter.h"
#endif
#ifndef __JUCETICE_METERCOMPONENT_HEADER__
 #include "controls/meter/jucetice_MeterComponent.h"
#endif

#ifndef __JUCETICE_DRAWABLEPAD_HEADER__
 #include "controls/pads/jucetice_DrawablePad.h"
#endif
#ifndef __JUCETICE_MIDIPAD_HEADER__
 #include "controls/pads/jucetice_MidiPad.h"
#endif

#ifndef __JUCETICE_PRESETSELECTORCOMPONENT_HEADER__
 #include "controls/selector/jucetice_PresetSelectorComponent.h"
#endif
//=============================================================================
#ifndef __JUCETICE_SQLITE_HEADER__
 #include "database/jucetice_Sqlite.h"
#endif
//=============================================================================
#ifndef __JUCETICE_JUCETICELOOKANDFEEL_HEADER__
 #include "lookandfeel/jucetice_JuceticeLookAndFeel.h"
#endif
//=============================================================================
#ifndef __JUCETICE_NET_HEADER__
 #include "network/jucetice_Net.h"
#endif
//=============================================================================
#ifndef __JUCETICE_SCRIPTABLEENGINECORE_HEADER__
 #include "scripting/bindings/jucetice_ScriptableEngineCore.h"
#endif
#ifndef __JUCETICE_SCRIPTABLEENGINESTRING_HEADER__
 #include "scripting/bindings/jucetice_ScriptableEngineString.h"
#endif
#ifndef __JUCETICE_SCRIPTABLEENGINE_HEADER__
 #include "scripting/jucetice_ScriptableEngine.h"
#endif
//=============================================================================
#ifndef __JUCETICE_TESTINGFRAMEWORK_HEADER__
 #include "testing/jucetice_TestingFramework.h"
#endif
//=============================================================================
#ifndef __JUCETICE_COMMANDLINETOKENIZER_HEADER__
 #include "utils/jucetice_CommandLineTokenizer.h"
#endif
#ifndef __JUCETICE_FASTDELEGATES_HEADER__
 #include "utils/jucetice_FastDelegate.h"
#endif
#ifndef __JUCETICE_GNUPLOTINTERFACE_HEADER__
 #include "utils/jucetice_GnuplotInterface.h"
#endif
#ifndef __JUCETICE_SERIALIZABLE_HEADER__
 #include "utils/jucetice_Serializable.h"
#endif
#ifndef __JUCETICE_TESTAPPLICATION_HEADER__
 #include "utils/jucetice_TestApplication.h"
#endif

// END_AUTOINCLUDE

END_JUCE_NAMESPACE

#endif   // __JUCED_JUCEHEADER__
