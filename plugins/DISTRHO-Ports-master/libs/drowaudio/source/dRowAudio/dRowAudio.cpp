/*
  ==============================================================================
  
  This file is part of the dRowAudio JUCE module
  Copyright 2004-12 by dRowAudio.
  
  ------------------------------------------------------------------------------
 
  dRowAudio can be redistributed and/or modified under the terms of the GNU General
  Public License (Version 2), as published by the Free Software Foundation.
  A copy of the license is included in the module distribution, or can be found
  online at www.gnu.org/licenses.
  
  dRowAudio is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  
  ==============================================================================
*/


#ifdef __DROWAUDIO_JUCEHEADER__
    /*  When you add this cpp file to your project, you mustn't include it in a file where you've
        already included any other headers - just put it inside a file on its own, possibly with your config
        flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
        header files that the compiler may be using.
     */
#error "Incorrect use of DROWAUDIO cpp file"
#endif

// Your project must contain an AppConfig.h file with your project-specific settings in it,
// and your header search path must make it accessible to the module's files.
#include "AppConfig.h"

//#include "../juce_core/native/juce_BasicNativeHeaders.h"

#if JUCE_MAC || JUCE_IOS
    #import <Foundation/Foundation.h>
    #import <AudioToolbox/AudioToolbox.h>
#endif

#if JUCE_IOS
    #import <AVFoundation/AVFoundation.h>
    #import <MediaPlayer/MediaPlayer.h>
#endif

#include "dRowAudio.h"

#include "audio/soundtouch/SoundTouch_Source.cpp"

namespace drow {

// Audio
#include "audio/dRowAudio_AudioFilePlayer.cpp"
#include "audio/dRowAudio_AudioFilePlayerExt.cpp"
#include "audio/dRowAudio_AudioSampleBufferAudioFormat.cpp"

#include "audio/dRowAudio_SoundTouchProcessor.cpp"
#include "audio/dRowAudio_SoundTouchAudioSource.cpp"

#include "audio/dRowAudio_FilteringAudioSource.cpp"
#include "audio/dRowAudio_ReversibleAudioSource.cpp"
#include "audio/dRowAudio_LoopingAudioSource.cpp"

#include "audio/dRowAudio_PitchDetector.cpp"

#include "audio/dRowAudio_AudioUtilityUnitTests.cpp"

#include "audio/dRowAudio_EnvelopeFollower.cpp"
#include "audio/dRowAudio_SampleRateConverter.cpp"

#include "audio/filters/dRowAudio_BiquadFilter.cpp"
#include "audio/filters/dRowAudio_OnePoleFilter.cpp"

#include "audio/fft/dRowAudio_Window.cpp"
#include "audio/fft/dRowAudio_FFT.cpp"
#include "audio/fft/dRowAudio_LTAS.cpp"

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
// Gui
#include "gui/dRowAudio_AudioFileDropTarget.cpp"
#include "gui/dRowAudio_GraphicalComponent.cpp"
#include "gui/dRowAudio_AudioOscilloscope.cpp"
#include "gui/dRowAudio_AudioTransportCursor.cpp"
#include "gui/dRowAudio_SegmentedMeter.cpp"
#include "gui/dRowAudio_Sonogram.cpp"
#include "gui/dRowAudio_Spectrograph.cpp"
#include "gui/dRowAudio_Spectroscope.cpp"
#include "gui/dRowAudio_TriggeredScope.cpp"
#include "gui/dRowAudio_CpuMeter.cpp"
#include "gui/dRowAudio_Clock.cpp"
//#include "gui/dRowAudio_CentreAlignViewport.cpp"
#include "gui/dRowAudio_MusicLibraryTable.cpp"
#include "gui/filebrowser/dRowAudio_BasicFileBrowser.cpp"
#include "gui/filebrowser/dRowAudio_ColumnFileBrowser.cpp"
#include "gui/audiothumbnail/dRowAudio_AudioThumbnailImage.cpp"
#include "gui/audiothumbnail/dRowAudio_ColouredAudioThumbnail.cpp"
#include "gui/audiothumbnail/dRowAudio_PositionableWaveDisplay.cpp"
#include "gui/audiothumbnail/dRowAudio_DraggableWaveDisplay.cpp"
#endif

// maths
#include "maths/dRowAudio_MathsUnitTests.cpp"

// native
#if JUCE_IOS
 #include "native/dRowAudio_AudioPicker.mm"
 #include "native/dRowAudio_AVAssetAudioFormat.mm"
 #include "native/dRowAudio_IOSAudioConverter.mm"
#endif
    
// network
#include "network/dRowAudio_CURLManager.cpp"
#include "network/dRowAudio_CURLEasySession.cpp"

// streams
#include "streams/dRowAudio_MemoryInputSource.cpp"

// Utility
#include "utility/dRowAudio_EncryptedString.cpp"
#include "utility/dRowAudio_ITunesLibrary.cpp"
#include "utility/dRowAudio_ITunesLibraryParser.cpp"
#include "utility/dRowAudio_UnityBuilder.cpp"
#include "utility/dRowAudio_UnityProjectBuilder.cpp"
#include "parameters/dRowAudio_PluginParameter.cpp"

}