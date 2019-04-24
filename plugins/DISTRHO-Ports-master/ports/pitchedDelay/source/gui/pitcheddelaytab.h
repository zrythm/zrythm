/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  13 Feb 2013 7:22:10pm

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

#ifndef __JUCER_HEADER_PITCHEDDELAYTAB_PITCHEDDELAYTAB_1C1B489C__
#define __JUCER_HEADER_PITCHEDDELAYTAB_PITCHEDDELAYTAB_1C1B489C__

//[Headers]	 -- You can add your own extra header files here --
#include "JuceHeader.h"
#include "../PluginProcessor.h"
//[/Headers]



//==============================================================================
/**
																	//[Comments]
	An auto-generated component, created by the Jucer.

	Describe your class and how it works here!
																	//[/Comments]
*/
class PitchedDelayTab  : public Component,
                         public ActionBroadcaster,
                         public Timer,
                         public SliderListener,
                         public ComboBoxListener,
                         public ButtonListener
{
public:
	//==============================================================================
	PitchedDelayTab (PitchedDelayAudioProcessor* processor, int delayindex);
	~PitchedDelayTab() override;

	//==============================================================================
	//[UserMethods]	 -- You can add your own custom methods in this section.

	void setDelayRange(bool sendMessage = true);
	void setPitchRange();

	double getDelaySeconds();
	void setDelaySeconds(double seconds, bool sendMessage=false);

	double getPreDelaySeconds();
	void setPreDelaySeconds(double seconds, bool sendMessage=false);

	void timerCallback() override;

	void setParam(int index, double value);

	//MonoStereoSwitcher* getSwitcher() { return monoStereoSwitcher; }

	PitchedDelayAudioProcessor* getProcessor() { return filter; }
	//[/UserMethods]

	void paint (Graphics& g) override;
	void resized() override;
	void sliderValueChanged (Slider* sliderThatWasMoved) override;
	void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
	void buttonClicked (Button* buttonThatWasClicked) override;

private:
	//[UserVariables]   -- You can add your own custom variables in this section.
	double getSliderDelaySeconds();
	void updateBPM();

	double quantizeDelay(double seconds);
	double unQuantizeDelay(double sliderValue);

	PitchedDelayAudioProcessor* filter;
	const int delayIndex;

	double currentValues[DelayTabDsp::kNumParameters];
	double currentBPM;
	friend class DelayGraph;
	//[/UserVariables]

	//==============================================================================
	Slider* sDelay;
	Label* label;
	ComboBox* cbSync;
	Slider* sPitch;
	ToggleButton* tbPostPitch;
	Slider* sFeedback;
	Label* label3;
	Slider* sFreq;
	Label* label4;
	ToggleButton* tbSemitones;
	Slider* sQfactor;
	Label* label5;
	ComboBox* cbFilter;
	Label* label6;
	Slider* sGain;
	Label* label7;
	ToggleButton* tbEnable;
	Slider* sVolume;
	Label* label8;
	ComboBox* cbPitch;
	ToggleButton* tbMono;
	ToggleButton* tbStereo;
	ToggleButton* tbPingpong;
	Label* lPan;
	Slider* sPan;
	Slider* sPreDelay;
	Label* label2;
	Slider* sPreVolume;
	Path internalPath1;


	//==============================================================================
	// (prevent copy constructor and operator= being generated..)
	PitchedDelayTab (const PitchedDelayTab&);
	const PitchedDelayTab& operator= (const PitchedDelayTab&);
};


#endif   // __JUCER_HEADER_PITCHEDDELAYTAB_PITCHEDDELAYTAB_1C1B489C__
