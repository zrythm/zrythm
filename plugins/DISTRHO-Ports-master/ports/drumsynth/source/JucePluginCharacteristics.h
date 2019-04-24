/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#ifndef __JUCE_PLUGIN_CHARACTERISTICS_H__
#define __JUCE_PLUGIN_CHARACTERISTICS_H__

#define JucePlugin_Name                     "DrumSynth"
#define JucePlugin_Desc                     "Drum Synthesizer"
#define JucePlugin_Manufacturer             "kRAkEn/gORe"
#define JucePlugin_ManufacturerCode         'kNGr'
#define JucePlugin_PluginCode               'jds0'
#define JucePlugin_MaxNumInputChannels              0
#define JucePlugin_MaxNumOutputChannels             2
#define JucePlugin_PreferredChannelConfigurations   { 0, 2 }
#define JucePlugin_IsSynth                          1
#define JucePlugin_IsMidiEffect                     0
#define JucePlugin_WantsMidiInput                   1
#define JucePlugin_ProducesMidiOutput               0
#define JucePlugin_SilenceInProducesSilenceOut      0
#define JucePlugin_EditorRequiresKeyboardFocus      1

#define JucePlugin_VersionCode              0x00000103
#define JucePlugin_VersionString            "0.1.3"
#define JucePlugin_VSTUniqueID              JucePlugin_PluginCode
#define JucePlugin_VSTCategory              kPlugCategSynth
#define JucePlugin_AUMainType               kAudioUnitType_MusicDevice
#define JucePlugin_AUSubType                JucePlugin_PluginCode
#define JucePlugin_AUExportPrefix           DrumSynthAU
#define JucePlugin_AUExportPrefixQuoted     "DrumSynthAU"
#define JucePlugin_AUManufacturerCode       JucePlugin_ManufacturerCode
#define JucePlugin_CFBundleIdentifier       "com.juced.DrumSynth"
#define JucePlugin_RTASCategory             ePlugInCategory_SWGenerators
#define JucePlugin_RTASManufacturerCode     JucePlugin_ManufacturerCode
#define JucePlugin_RTASProductId            JucePlugin_PluginCode
#define JucePlugin_WinBag_path              "C:\\essentials\\PT_73_SDK\\WinBag"

#define JucePlugin_LV2URI                   "urn:juced:DrumSynth"
#define JucePlugin_LV2Category              "InstrumentPlugin"
#define JucePlugin_WantsLV2Latency          0
#define JucePlugin_WantsLV2State            1
#define JucePlugin_WantsLV2StateString      1
#define JucePlugin_WantsLV2Presets          0
#define JucePlugin_WantsLV2TimePos          0

//==============================================================================

#endif
