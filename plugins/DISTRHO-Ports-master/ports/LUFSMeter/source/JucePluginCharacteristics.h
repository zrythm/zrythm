/*

    IMPORTANT! This file is auto-generated each time you save your
    project - if you alter its contents, your changes may be overwritten!

    There's a section below where you can add your own custom code safely, and the
    Introjucer will preserve the contents of that block, but the best way to change
    any of these definitions is by using the Introjucer's project settings.

    Any commented-out settings will assume their default values.

*/

#ifndef __JUCE_APPCONFIG_I07HNS__
#define __JUCE_APPCONFIG_I07HNS__

//==============================================================================
// Audio plugin settings..

#ifdef LUFS_MULTI
 #define JucePlugin_Name                   "LUFS Meter (Multichannel)"
 #define JucePlugin_PluginCode             'LufM'
 #define JucePlugin_MaxNumInputChannels    24
 #define JucePlugin_MaxNumOutputChannels   24
 #define JucePlugin_PreferredChannelConfigurations  {24, 24}
 #define JucePlugin_AUExportPrefix         LUFSMeterMultichannelAU
 #define JucePlugin_AUExportPrefixQuoted   "LUFSMeterMultichannelAU"
 #define JucePlugin_CFBundleIdentifier     com.klangfreund.lufsmetermultichannel
 #define JucePlugin_AAXIdentifier          com.klangfreund.lufsmetermultichannel
 #define JucePlugin_LV2URI                 "http://www.klangfreund.com/lufsmetermultichannel"
#else
 #define JucePlugin_Name                   "LUFS Meter"
 #define JucePlugin_PluginCode             'Lufs'
 #define JucePlugin_MaxNumInputChannels    2
 #define JucePlugin_MaxNumOutputChannels   2
 #define JucePlugin_PreferredChannelConfigurations  {2, 2}
 #define JucePlugin_AUExportPrefix         LUFSMeterAU
 #define JucePlugin_AUExportPrefixQuoted   "LUFSMeterAU"
 #define JucePlugin_CFBundleIdentifier     com.klangfreund.lufsmeter
 #define JucePlugin_AAXIdentifier          com.klangfreund.lufsmeter
 #define JucePlugin_LV2URI                 "http://www.klangfreund.com/lufsmeter"
#endif
#define JucePlugin_Desc                   "Measures loudness according to ITU 1770-3 and EBU R128."
#define JucePlugin_Manufacturer           "Klangfreund"
#define JucePlugin_ManufacturerWebsite    "http://www.klangfreund.com"
#define JucePlugin_ManufacturerEmail      "support@yourcompany.com"
#define JucePlugin_ManufacturerCode       'Klan'
#define JucePlugin_IsSynth                0
#define JucePlugin_IsMidiEffect           0
#define JucePlugin_WantsMidiInput         0
#define JucePlugin_ProducesMidiOutput     0
#define JucePlugin_SilenceInProducesSilenceOut  1
#define JucePlugin_EditorRequiresKeyboardFocus  0
#define JucePlugin_Version                0.6.0
#define JucePlugin_VersionCode            0x600
#define JucePlugin_VersionString          "0.6.0"
#define JucePlugin_VSTUniqueID            JucePlugin_PluginCode
#define JucePlugin_VSTCategory            kPlugCategEffect
#define JucePlugin_AUMainType             kAudioUnitType_Effect
#define JucePlugin_AUSubType              JucePlugin_PluginCode
#define JucePlugin_AUManufacturerCode     JucePlugin_ManufacturerCode
#define JucePlugin_RTASCategory           ePlugInCategory_None
#define JucePlugin_RTASManufacturerCode   JucePlugin_ManufacturerCode
#define JucePlugin_RTASProductId          JucePlugin_PluginCode
#define JucePlugin_RTASDisableBypass      0
#define JucePlugin_RTASDisableMultiMono   0
#define JucePlugin_AAXManufacturerCode    JucePlugin_ManufacturerCode
#define JucePlugin_AAXProductId           JucePlugin_PluginCode
#define JucePlugin_AAXCategory            AAX_ePlugInCategory_Dynamics
#define JucePlugin_AAXDisableBypass       0
#define JucePlugin_AAXDisableMultiMono    0

#define JucePlugin_WantsLV2Latency          0
#define JucePlugin_WantsLV2State            1
#define JucePlugin_WantsLV2TimePos          1
#define JucePlugin_WantsLV2Presets          0

#endif  // __JUCE_APPCONFIG_I07HNS__
