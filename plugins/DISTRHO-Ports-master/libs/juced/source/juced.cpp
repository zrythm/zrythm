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

#if defined (__JUCED_JUCEHEADER__) && ! defined (JUCE_AMALGAMATED_INCLUDE)
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

// Your project must contain an AppConfig.h file with your project-specific settings in it,
// and your header search path must make it accessible to the module's files.
#include "AppConfig.h"

#include <stdint.h>

#if LINUX
#include <unistd.h>
#endif

#include "../../juce/source/modules/juce_core/native/juce_BasicNativeHeaders.h"
#include "juced.h"

// START_AUTOINCLUDE
#include "base/jucetice_AudioParameter.cpp"

#include "audio/beat/jucetice_BeatDetector.cpp"
#include "audio/fft/jucetice_FFTWrapper.cpp"

#include "audio/lash/jucetice_LashManager.cpp"
#include "audio/midi/jucetice_MidiAutomatorManager.cpp"
#include "audio/midi/jucetice_MidiFilter.cpp"
#include "audio/midi/jucetice_MidiManipulator.cpp"
#include "audio/midi/jucetice_MidiTransform.cpp"
#include "audio/midi/jucetice_TuningMap.cpp"

#include "audio/osc/jucetice_OpenSoundBundle.cpp"
#include "audio/osc/jucetice_OpenSoundController.cpp"
#include "audio/osc/jucetice_OpenSoundMessage.cpp"
#include "audio/osc/jucetice_OpenSoundTimeTag.cpp"
#include "audio/osc/jucetice_UDPSocket.cpp"

#include "audio/processors/jucetice_AudioSourceProcessor.cpp"

#include "controls/jucetice_ImageKnob.cpp"
#include "controls/jucetice_ImageSlider.cpp"
#include "controls/jucetice_Joystick.cpp"
#include "controls/jucetice_SpectrumAnalyzer.cpp"
#include "controls/coordinate/jucetice_CoordinateSystem.cpp"
#include "controls/coordinate/jucetice_CoordinateSystemDemoContentComponent.cpp"
#include "controls/coordinate/jucetice_CoordinateSystemRange.cpp"
#include "controls/coordinate/jucetice_HelperFunctions.cpp"
#include "controls/coordinate/jucetice_StringTools.cpp"
#include "controls/coordinate/jucetice_SymbolButton.cpp"
#include "controls/graph/jucetice_GraphConnectorComponent.cpp"
#include "controls/graph/jucetice_GraphLinkComponent.cpp"
#include "controls/graph/jucetice_GraphNodeComponent.cpp"
#include "controls/grid/jucetice_PianoGrid.cpp"
#include "controls/grid/jucetice_PianoGridHeader.cpp"
#include "controls/grid/jucetice_PianoGridIndicator.cpp"
#include "controls/grid/jucetice_PianoGridKeyboard.cpp"
#include "controls/grid/jucetice_PianoGridNote.cpp"
#include "controls/layout/jucetice_ComponentLayoutEditor.cpp"
#include "controls/layout/jucetice_Dock.cpp"
#include "controls/layout/jucetice_Viewport.cpp"
#include "controls/layout/jucetice_ViewportNavigator.cpp"
#include "controls/meter/jucetice_DecibelScale.cpp"
#include "controls/meter/jucetice_HighQualityMeter.cpp"
#include "controls/meter/jucetice_MeterComponent.cpp"
#include "controls/pads/jucetice_DrawablePad.cpp"
#include "controls/pads/jucetice_MidiPad.cpp"

#include "database/jucetice_Sqlite.cpp"

#include "lookandfeel/jucetice_JuceticeLookAndFeel.cpp"

#include "network/jucetice_Net.cpp"

#include "scripting/bindings/jucetice_ScriptableEngineCore.cpp"
#include "scripting/bindings/jucetice_ScriptableEngineString.cpp"
#include "scripting/jucetice_ScriptableEngine.cpp"

#include "testing/jucetice_TestingFramework.cpp"

#include "utils/jucetice_CommandLineTokenizer.cpp"
#include "utils/jucetice_GnuplotInterface.cpp"
#include "utils/jucetice_Serializable.cpp"

// END_AUTOINCLUDE
