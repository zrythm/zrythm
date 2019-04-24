/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2007 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

#ifndef __JUCETICE_EQINOX_PLUGIN_CHARACTERISTICS_H__
#define __JUCETICE_EQINOX_PLUGIN_CHARACTERISTICS_H__

#define JucePlugin_Name                     "EQinox"
#define JucePlugin_Desc                     "6band EQualizer"
#define JucePlugin_Manufacturer             "kRAkEn/gORe"
#define JucePlugin_ManufacturerCode         'kNGr'
#define JucePlugin_PluginCode               'jeq0'
#define JucePlugin_MaxNumInputChannels              2
#define JucePlugin_MaxNumOutputChannels             2
#define JucePlugin_PreferredChannelConfigurations   { 1, 1 }, { 2, 2 }
#define JucePlugin_IsSynth                          0
#define JucePlugin_IsMidiEffect                     0
#define JucePlugin_WantsMidiInput                   1
#define JucePlugin_ProducesMidiOutput               0
#define JucePlugin_SilenceInProducesSilenceOut      1
#define JucePlugin_EditorRequiresKeyboardFocus      1

#define JucePlugin_VersionCode              0x00000205
#define JucePlugin_VersionString            "0.2.5"
#define JucePlugin_VSTUniqueID              JucePlugin_PluginCode
#define JucePlugin_VSTCategory              kPlugCategEffect
#define JucePlugin_AUMainType               kAudioUnitType_Effect
#define JucePlugin_AUSubType                JucePlugin_PluginCode
#define JucePlugin_AUExportPrefix           JuceDemoAU
#define JucePlugin_AUExportPrefixQuoted     "JuceDemoAU"
#define JucePlugin_AUManufacturerCode       JucePlugin_ManufacturerCode
#define JucePlugin_RTASCategory             ePlugInCategory_None
#define JucePlugin_RTASManufacturerCode     JucePlugin_ManufacturerCode
#define JucePlugin_RTASProductId            JucePlugin_PluginCode

#define JucePlugin_LV2URI                   "urn:juced:EQinox"
#define JucePlugin_LV2Category              "EQPlugin"
#define JucePlugin_WantsLV2Latency          0
#define JucePlugin_WantsLV2Presets          0
#define JucePlugin_WantsLV2State            0
#define JucePlugin_WantsLV2TimePos          0

//==============================================================================

#endif
