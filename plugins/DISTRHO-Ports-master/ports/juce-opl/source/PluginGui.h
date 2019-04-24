/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.0.2

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include <array>
#include "JuceHeader.h"
#include "PluginProcessor.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
	This is a GUI for the OPL2 VST plugin, created in Juce.
                                                                    //[/Comments]
*/
class PluginGui  : public AudioProcessorEditor,
                   public FileDragAndDropTarget,
                   public DragAndDropContainer,
                   public Timer,
                   public ComboBoxListener,
                   public SliderListener,
                   public ButtonListener
{
public:
    //==============================================================================
    PluginGui (AdlibBlasterAudioProcessor* ownerFilter);
    ~PluginGui();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
	void updateFromParameters();
    bool isInterestedInFileDrag (const StringArray& files);
    void fileDragEnter (const StringArray& files, int x, int y);
    void fileDragMove (const StringArray& files, int x, int y);
    void fileDragExit (const StringArray& files);
    void filesDropped (const StringArray& files, int x, int y);
	void timerCallback();
	void setRecordButtonState(bool recording);
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void buttonClicked (Button* buttonThatWasClicked) override;

    // Binary resources:
    static const char* full_sine_png;
    static const int full_sine_pngSize;
    static const char* half_sine_png;
    static const int half_sine_pngSize;
    static const char* abs_sine_png;
    static const int abs_sine_pngSize;
    static const char* quarter_sine_png;
    static const int quarter_sine_pngSize;
    static const char* camel_sine_png;
    static const int camel_sine_pngSize;
    static const char* alternating_sine_png;
    static const int alternating_sine_pngSize;
    static const char* square_png;
    static const int square_pngSize;
    static const char* logarithmic_saw_png;
    static const int logarithmic_saw_pngSize;
    static const char* channeloff_png;
    static const int channeloff_pngSize;
    static const char* channelon_png;
    static const int channelon_pngSize;
    static const char* toggle_off_sq_png;
    static const int toggle_off_sq_pngSize;
    static const char* toggle_on_sq_png;
    static const int toggle_on_sq_pngSize;
    static const char* line_border_horiz_png;
    static const int line_border_horiz_pngSize;
    static const char* line_border_vert_png;
    static const int line_border_vert_pngSize;
    static const char* algo_switch_off_png;
    static const int algo_switch_off_pngSize;
    static const char* algo_switch_on_png;
    static const int algo_switch_on_pngSize;
    static const char* algo_switch_on2_png;
    static const int algo_switch_on2_pngSize;
    static const char* algo_switch_on3_png;
    static const int algo_switch_on3_pngSize;
    static const char* twoopAm_png;
    static const int twoopAm_pngSize;
    static const char* twoopFm_png;
    static const int twoopFm_pngSize;
    static const char* bassdrum_png;
    static const int bassdrum_pngSize;
    static const char* snare_png;
    static const int snare_pngSize;
    static const char* disabled_png;
    static const int disabled_pngSize;
    static const char* tom_png;
    static const int tom_pngSize;
    static const char* hihat_png;
    static const int hihat_pngSize;
    static const char* cymbal_png;
    static const int cymbal_pngSize;
    static const char* adlib_png;
    static const int adlib_pngSize;


private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	static const uint32 COLOUR_MID = 0xff007f00;
	static const uint32 COLOUR_RECORDING = 0xffff0000;
	AdlibBlasterAudioProcessor* processor;
	std::array<ScopedPointer<TextButton>, Hiopl::CHANNELS> channels;
	TooltipWindow tooltipWindow;
	File instrumentLoadDirectory;
	File instrumentSaveDirectory;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<GroupComponent> groupComponent2;
    ScopedPointer<GroupComponent> groupComponent4;
    ScopedPointer<GroupComponent> groupComponent11;
    ScopedPointer<GroupComponent> groupComponent10;
    ScopedPointer<GroupComponent> groupComponent9;
    ScopedPointer<GroupComponent> groupComponent;
    ScopedPointer<ComboBox> frequencyComboBox;
    ScopedPointer<Label> frequencyLabel;
    ScopedPointer<Slider> aSlider;
    ScopedPointer<Label> aLabel;
    ScopedPointer<Slider> dSlider;
    ScopedPointer<Label> dLabel;
    ScopedPointer<Slider> sSlider;
    ScopedPointer<Label> dLabel2;
    ScopedPointer<Slider> rSlider;
    ScopedPointer<Label> rLabel;
    ScopedPointer<Slider> attenuationSlider;
    ScopedPointer<Label> attenuationLabel;
    ScopedPointer<Label> dbLabel;
    ScopedPointer<ImageButton> sineImageButton;
    ScopedPointer<ImageButton> halfsineImageButton;
    ScopedPointer<ImageButton> abssineImageButton;
    ScopedPointer<ImageButton> quartersineImageButton;
    ScopedPointer<Label> waveLabel;
    ScopedPointer<ToggleButton> tremoloButton;
    ScopedPointer<ToggleButton> vibratoButton;
    ScopedPointer<ToggleButton> sustainButton;
    ScopedPointer<ToggleButton> keyscaleEnvButton;
    ScopedPointer<Label> dbLabel2;
    ScopedPointer<ComboBox> frequencyComboBox2;
    ScopedPointer<Label> frequencyLabel3;
    ScopedPointer<Slider> aSlider2;
    ScopedPointer<Label> aLabel2;
    ScopedPointer<Slider> dSlider2;
    ScopedPointer<Label> dLabel3;
    ScopedPointer<Slider> sSlider2;
    ScopedPointer<Label> dLabel4;
    ScopedPointer<Slider> rSlider2;
    ScopedPointer<Label> rLabel2;
    ScopedPointer<Slider> attenuationSlider2;
    ScopedPointer<Label> attenuationLabel2;
    ScopedPointer<Label> dbLabel3;
    ScopedPointer<ImageButton> sineImageButton2;
    ScopedPointer<ImageButton> halfsineImageButton2;
    ScopedPointer<ImageButton> abssineImageButton2;
    ScopedPointer<ImageButton> quartersineImageButton2;
    ScopedPointer<Label> waveLabel2;
    ScopedPointer<ToggleButton> tremoloButton2;
    ScopedPointer<ToggleButton> vibratoButton2;
    ScopedPointer<ToggleButton> sustainButton2;
    ScopedPointer<ToggleButton> keyscaleEnvButton2;
    ScopedPointer<Label> frequencyLabel4;
    ScopedPointer<GroupComponent> groupComponent3;
    ScopedPointer<Slider> tremoloSlider;
    ScopedPointer<Label> frequencyLabel5;
    ScopedPointer<Label> dbLabel5;
    ScopedPointer<Slider> vibratoSlider;
    ScopedPointer<Label> frequencyLabel6;
    ScopedPointer<Label> dbLabel6;
    ScopedPointer<Slider> feedbackSlider;
    ScopedPointer<Label> frequencyLabel7;
    ScopedPointer<ComboBox> velocityComboBox;
    ScopedPointer<ComboBox> velocityComboBox2;
    ScopedPointer<Label> attenuationLabel4;
    ScopedPointer<ImageButton> alternatingsineImageButton;
    ScopedPointer<ImageButton> camelsineImageButton;
    ScopedPointer<ImageButton> squareImageButton;
    ScopedPointer<ImageButton> logsawImageButton;
    ScopedPointer<ImageButton> alternatingsineImageButton2;
    ScopedPointer<ImageButton> camelsineImageButton2;
    ScopedPointer<ImageButton> squareImageButton2;
    ScopedPointer<ImageButton> logsawImageButton2;
    ScopedPointer<Label> dbLabel4;
    ScopedPointer<ComboBox> keyscaleAttenuationComboBox2;
    ScopedPointer<ComboBox> keyscaleAttenuationComboBox;
    ScopedPointer<GroupComponent> groupComponent5;
    ScopedPointer<Slider> emulatorSlider;
    ScopedPointer<Label> emulatorLabel;
    ScopedPointer<Label> emulatorLabel2;
    ScopedPointer<ToggleButton> recordButton;
    ScopedPointer<TextButton> exportButton;
    ScopedPointer<TextButton> loadButton;
    ScopedPointer<Label> versionLabel;
    ScopedPointer<ImageButton> ToggleButtonOffExample;
    ScopedPointer<ImageButton> ToggleButtonOnExample;
    ScopedPointer<Label> label;
    ScopedPointer<Label> label2;
    ScopedPointer<ImageButton> LineBorderButton1C;
    ScopedPointer<ImageButton> LineBorderButton1A;
    ScopedPointer<ImageButton> LineBorderButton1B;
    ScopedPointer<Label> label3;
    ScopedPointer<ImageButton> LineBorderButton1C2;
    ScopedPointer<ImageButton> LineBorderButton1A2;
    ScopedPointer<ImageButton> LineBorderButton1B2;
    ScopedPointer<ImageButton> LineBorderButton1C3;
    ScopedPointer<ImageButton> LineBorderButton1B3;
    ScopedPointer<ImageButton> algoSwitchButtonOffEx1;
    ScopedPointer<ImageButton> algoSwitchButtonOffEx2;
    ScopedPointer<ImageButton> algoSwitchButtonOnEx1;
    ScopedPointer<ImageButton> algoSwitchButtonOnEx2;
    ScopedPointer<Label> label4;
    ScopedPointer<Label> label5;
    ScopedPointer<Label> label6;
    ScopedPointer<Label> label7;
    ScopedPointer<Label> label8;
    ScopedPointer<ImageButton> algoSwitchButtonOn2Ex1;
    ScopedPointer<ImageButton> algoSwitchButtonOn2Ex2;
    ScopedPointer<Label> label9;
    ScopedPointer<Label> label10;
    ScopedPointer<ImageButton> algoSwitchButtonOn3Ex1;
    ScopedPointer<ImageButton> algoSwitchButtonOn3Ex2;
    ScopedPointer<Label> label11;
    ScopedPointer<Label> label12;
    ScopedPointer<ImageButton> TwoOpAMButton;
    ScopedPointer<ImageButton> TwoOpFMButton;
    ScopedPointer<Label> label13;
    ScopedPointer<Label> label14;
    ScopedPointer<Label> label15;
    ScopedPointer<Label> label16;
    ScopedPointer<Label> label17;
    ScopedPointer<GroupComponent> groupComponent6;
    ScopedPointer<ImageButton> algoSwitchButtonOnEx3;
    ScopedPointer<Label> label18;
    ScopedPointer<ImageButton> algoSwitchButtonOffEx3;
    ScopedPointer<Label> label19;
    ScopedPointer<ImageButton> TwoOpAMButton2;
    ScopedPointer<Label> label20;
    ScopedPointer<Label> label21;
    ScopedPointer<Label> label22;
    ScopedPointer<ImageButton> algoSwitchButtonOffEx4;
    ScopedPointer<Label> label23;
    ScopedPointer<ImageButton> algoSwitchButtonOn3Ex3;
    ScopedPointer<Label> label24;
    ScopedPointer<ImageButton> TwoOpFMButton2;
    ScopedPointer<Label> label25;
    ScopedPointer<Label> label26;
    ScopedPointer<GroupComponent> groupComponent7;
    ScopedPointer<ImageButton> algoSwitchButtonOffEx5;
    ScopedPointer<Label> label27;
    ScopedPointer<ImageButton> algoSwitchButtonOn3Ex4;
    ScopedPointer<Label> label28;
    ScopedPointer<GroupComponent> groupComponent8;
    ScopedPointer<Label> frequencyLabel9;
    ScopedPointer<Label> label29;
    ScopedPointer<Label> label30;
    ScopedPointer<Label> frequencyLabel10;
    ScopedPointer<Label> attenuationLabel5;
    ScopedPointer<ImageButton> fmButton;
    ScopedPointer<ImageButton> additiveButton;
    ScopedPointer<ImageButton> bassDrumButton;
    ScopedPointer<ImageButton> snareDrumButton;
    ScopedPointer<ImageButton> disablePercussionButton;
    ScopedPointer<ImageButton> tomTomButton;
    ScopedPointer<ImageButton> cymbalButton;
    ScopedPointer<ImageButton> hiHatButton;
    ScopedPointer<Label> dbLabel7;
    ScopedPointer<Label> dbLabel8;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginGui)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
