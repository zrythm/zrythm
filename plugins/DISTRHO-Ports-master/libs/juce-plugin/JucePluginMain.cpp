/*
  ==============================================================================

   Main symbol/entry for juce plugins

  ==============================================================================
*/

#include "JucePluginMain.h"

#if JucePlugin_Build_AU
 #include "modules/juce_audio_plugin_client/AU/juce_AU_Wrapper.mm"
#elif JucePlugin_Build_LV2
 #include "modules/juce_audio_plugin_client/LV2/juce_LV2_Wrapper.cpp"
#elif JucePlugin_Build_RTAS
 #include "modules/juce_audio_plugin_client/RTAS/juce_RTAS_Wrapper.cpp"
#elif JucePlugin_Build_VST
 // we need to include 'juce_VSTMidiEventList' before 'juce_VST_Wrapper'
//  #ifndef _MSC_VER
//   #define __cdecl
//  #endif
//  #include "pluginterfaces/vst2.x/aeffectx.h"
//  namespace juce {
//  #include "modules/juce_audio_processors/format_types/juce_VSTMidiEventList.h"
//  }
 #ifdef JUCE_MAC
  #include "modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.mm"
 #else
  #include "modules/juce_audio_plugin_client/VST/juce_VST_Wrapper.cpp"
 #endif
#elif JucePlugin_Build_Standalone
 #include "juce_StandaloneFilterApplication.cpp"
#else
 #error Invalid configuration
#endif

#if ! JucePlugin_Build_Standalone
 #include "modules/juce_audio_plugin_client/utility/juce_PluginUtilities.cpp"
#endif
