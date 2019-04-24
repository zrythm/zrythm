/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_341B599F75F1BD78__
#define __JUCE_HEADER_341B599F75F1BD78__

//[Headers]     -- You can add your own extra header files here --
#include "JuceHeader.h"
#include "PluginProcessor.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PreferencesLayout  : public Component,
                           public ComboBoxListener,
                           public SliderListener
{
public:
    //==============================================================================
    PreferencesLayout (AdmvAudioProcessor* plugin);
    ~PreferencesLayout();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);
    void sliderValueChanged (Slider* sliderThatWasMoved);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	tomatl::dsp::SingleBound<double> mSpectroReleaseBounds;
	tomatl::dsp::LinearScale mSpectroReleaseScale;
	tomatl::dsp::SingleBound<double> mGonioScaleReleaseBounds;
	tomatl::dsp::LinearScale mGonioScaleReleaseScale;
	AdmvAudioProcessor* mParentProcessor;

	void setSpectroReleaseLabelText(double value)
	{
		if (value == std::numeric_limits<double>::infinity() ||
				(int)value == (int)mSpectroReleaseScale.unscale(mSpectroReleaseSlider->getMaximum(), mSpectroReleaseBounds, mSpectroReleaseSlider->getMaximum()))
		{
			mSpectroReleaseLabel->setText(L"Never", juce::dontSendNotification);
		}
		else
		{
			mSpectroReleaseLabel->setText(juce::String((int)value), juce::dontSendNotification);
		}
	}
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<GroupComponent> mSpectroGroup;
    ScopedPointer<GroupComponent> mGenericGroup;
    ScopedPointer<GroupComponent> mGonioGroup;
    ScopedPointer<ComboBox> mGoniometerScaleModeBox;
    ScopedPointer<Slider> mSpectroReleaseSlider;
    ScopedPointer<Label> mSpectroReleaseLabel;
    ScopedPointer<ComboBox> mSpectrumFillModeBox;
    ScopedPointer<ComboBox> mOutputModeBox;
    ScopedPointer<Slider> mGonioScaleReleaseSlider;
    ScopedPointer<Label> mGonioScaleReleaseLabel;
    ScopedPointer<Label> mGonioOptionLabel1;
    ScopedPointer<Label> mGonioOptionLabel2;
    ScopedPointer<Label> mSpectroOptionLabel1;
    ScopedPointer<Label> mSpectroOptionLabel2;
    ScopedPointer<Label> mGenericOptionLabel1;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesLayout)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_341B599F75F1BD78__
