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

#ifndef __JUCE_HEADER_4057432C47622D16__
#define __JUCE_HEADER_4057432C47622D16__

//[Headers]     -- You can add your own extra header files here --
#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "TomatlLookAndFeel.h"
#include "AboutLayout.h"
#include "PreferencesLayout.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MainLayout  : public Component,
                    public SliderListener,
                    public ButtonListener
{
public:
    //==============================================================================
    MainLayout (AdmvAudioProcessor* plugin);
    ~MainLayout();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void updateFromState(const AdmvPluginState& state)
	{
		mGonioScaleValue.get()->setValue(TOMATL_TO_DB(state.mManualGoniometerScaleValue), dontSendNotification);
		mGonioScaleValue.get()->setEnabled(state.mManualGoniometerScale);
		mSpectroMagnitudeScale.get()->setMinValue(state.mSpectrometerMagnitudeScale.first, dontSendNotification);
		mSpectroMagnitudeScale.get()->setMaxValue(state.mSpectrometerMagnitudeScale.second, dontSendNotification);
		mSpectroFreqScale.get()->setMinAndMaxValues(state.mSpectrometerFrequencyScale.first, state.mSpectrometerFrequencyScale.second, dontSendNotification);
	}

	juce::Rectangle<int> getGonioRectangle()
	{
		return mGonioPlaceholder.get()->getBoundsInParent();
	}

	juce::Rectangle<int> getSpectroRectangle()
	{
		return mSpectroPlaceholder.get()->getBoundsInParent();
	}

	void updateGonioScale(double scale)
	{
		mGonioScaleValue.get()->setValue(TOMATL_TO_DB(1. / scale), dontSendNotification);
	}

	void showDefaultDialog(Component* contents, std::wstring caption)
	{
		DialogWindow::LaunchOptions options;

		options.content.setOwned(contents);

		options.content->setSize(contents->getWidth(), contents->getHeight());

		options.dialogTitle = caption.c_str();
		options.dialogBackgroundColour = LookAndFeel::getDefaultLookAndFeel().findColour(TomatlLookAndFeel::defaultBackground);
		options.escapeKeyTriggersCloseButton = true;
		options.useNativeTitleBar = false;
		options.resizable = false;

		const RectanglePlacement placement(RectanglePlacement::xRight + RectanglePlacement::yBottom + RectanglePlacement::doNotResize);

		DialogWindow* dw = options.launchAsync();

		dw->centreAroundComponent(this, dw->getWidth(), dw->getHeight());
	}

	void showAboutDialog()
	{
		showDefaultDialog(new AboutLayout(), L"About");
	}

	void showPreferencesDialog()
	{
		showDefaultDialog(new PreferencesLayout(mParentProcessor), L"Preferences");
	}

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	AdmvAudioProcessor* mParentProcessor = NULL;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Slider> mGonioScaleValue;
    ScopedPointer<Slider> mSpectroMagnitudeScale;
    ScopedPointer<Label> mGonioPlaceholder;
    ScopedPointer<Label> mSpectroPlaceholder;
    ScopedPointer<Slider> mSpectroFreqScale;
    ScopedPointer<TextButton> mAboutButton;
    ScopedPointer<TextButton> mOptionsBtn;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainLayout)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_4057432C47622D16__
