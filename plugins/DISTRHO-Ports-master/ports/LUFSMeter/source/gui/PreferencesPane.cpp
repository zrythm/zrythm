/*
 ===============================================================================
 
 PreferencesPane.cpp
 
 
 This file is part of the LUFS Meter audio measurement plugin.
 Copyright 2012-14 by Klangfreund, Samuel Gaehwiler.
 
 -------------------------------------------------------------------------------
 
 The LUFS Meter can be redistributed and/or modified under the terms of the GNU 
 General Public License Version 2, as published by the Free Software Foundation.
 A copy of the license is included with these source files. It can also be found
 at www.gnu.org/licenses.
 
 The LUFS Meter is distributed WITHOUT ANY WARRANTY.
 See the GNU General Public License for more details.
 
 -------------------------------------------------------------------------------
 
 To release a closed-source product which uses the LUFS Meter or parts of it,
 a commercial license is available. Visit www.klangfreund.com/lufsmeter for more
 information.
 
 ===============================================================================
 */


#include "PreferencesPane.h"


//==============================================================================
PreferencesPane::PreferencesPane (const Value& loudnessBarWidth,
                                  const Value& loudnessBarMinValue,
                                  const Value& loudnessBarMaxValue,
                                  const Value& showIntegratedLoudnessHistoryValue,
                                  const Value& showLoudnessRangeHistoryValue,
                                  const Value& showShortTermLoudnessHistoryValue,
                                  const Value& showMomentaryLoudnessHistoryValue)
  : loudnessHistoryGroup (String(), "History Graph"),
    showIntegratedLoudnessHistory ("I"),
    showLoudnessRangeHistory("LRA"),
    showShortTimeLoudnessHistory ("S"),
    showMomentaryLoudnessHistory ("M")
{
    // Get the icons from the embedded zip.
    // ------------------------------------
    // Source: JuceDemo WidgetsDemo.cpp:616
    StringArray iconNames;
    OwnedArray<Drawable> iconsFromZipFile;
    
    const bool dontKeepInternalCopyOfData = false;
    MemoryInputStream iconsFileStream (BinaryData::icons_zip, BinaryData::icons_zipSize, dontKeepInternalCopyOfData);
    const bool dontDeleteStreamWhenDestroyed = false;
    ZipFile icons (&iconsFileStream, dontDeleteStreamWhenDestroyed);
    
    for (int i = 0; i < icons.getNumEntries(); ++i)
    {
        ScopedPointer<InputStream> svgFileStream (icons.createStreamForEntry (i));
        
        if (svgFileStream != 0)
        {
            // DBG(icons.getEntry(i)->filename);
            iconNames.add (icons.getEntry(i)->filename);
            iconsFromZipFile.add (Drawable::createFromImageDataStream (*svgFileStream));
        }
    }
    
    ScopedPointer<DrawableComposite> wrench = dynamic_cast <DrawableComposite*> (iconsFromZipFile [iconNames.indexOf ("wrenchByIonicons.svg")]->createCopy());
    AnimatedSidePanel::setCaptionAndIcon("Preferences", wrench);

    loudnessBarSizeLeftIcon = dynamic_cast <DrawableComposite*> (iconsFromZipFile [iconNames.indexOf ("barsWide.svg")]->createCopy());
    addAndMakeVisible (loudnessBarSizeLeftIcon);
    loudnessBarSizeRightIcon = dynamic_cast <DrawableComposite*> (iconsFromZipFile [iconNames.indexOf ("barsNarrow.svg")]->createCopy());
    addAndMakeVisible (loudnessBarSizeRightIcon);
    loudnessBarRangeLeftIcon = dynamic_cast <DrawableComposite*> (iconsFromZipFile [iconNames.indexOf ("rangeArrow.svg")]->createCopy());
    addAndMakeVisible (loudnessBarRangeLeftIcon);
    
    resized();
    
    
    const bool isReadOnly = false;
    const int textEntryBoxWidth = 0;
    const int textEntryBoxHeight = 0;
    
    loudnessBarSize.setRange (-300.0, -5.0, 1.0); // This value multiplied by -1 results in the used with
        // for each loudness bar. Negative values have been chosen to invert the behaviour of the slider
        // (left side: big bars, right side: small bars).
    loudnessBarSize.getValueObject().referTo(loudnessBarWidth);
    loudnessBarSize.setTextBoxStyle(Slider::NoTextBox, isReadOnly, textEntryBoxWidth, textEntryBoxHeight);
    loudnessBarSize.setColour(Slider::trackColourId, Colours::black);
    addAndMakeVisible (&loudnessBarSize);

    loudnessBarRange.setRange (-100, 0.0, 1.0);
    loudnessBarRange.getMinValueObject().referTo (loudnessBarMinValue);
    loudnessBarRange.getMaxValueObject().referTo (loudnessBarMaxValue);
    loudnessBarRange.setTextBoxStyle (Slider::NoTextBox, isReadOnly, textEntryBoxWidth, textEntryBoxHeight);
    loudnessBarRange.setPopupDisplayEnabled (true, true, this);
    loudnessBarRange.setTextValueSuffix (" LUFS");
    addAndMakeVisible (&loudnessBarRange);
    
    addAndMakeVisible (&loudnessHistoryGroup);
    
    showIntegratedLoudnessHistory.setClickingTogglesState (true);
    showIntegratedLoudnessHistory.getToggleStateValue().referTo (showIntegratedLoudnessHistoryValue);
    showIntegratedLoudnessHistory.setWantsKeyboardFocus (false);
    loudnessHistoryGroup.addAndMakeVisible (&showIntegratedLoudnessHistory);
    
    showLoudnessRangeHistory.setClickingTogglesState (true);
    showLoudnessRangeHistory.getToggleStateValue().referTo (showLoudnessRangeHistoryValue);
    showLoudnessRangeHistory.setWantsKeyboardFocus (false);
    loudnessHistoryGroup.addAndMakeVisible (&showLoudnessRangeHistory);
    
    showShortTimeLoudnessHistory.setClickingTogglesState (true);
    showShortTimeLoudnessHistory.getToggleStateValue().referTo (showShortTermLoudnessHistoryValue);
    showShortTimeLoudnessHistory.setWantsKeyboardFocus (false);
    loudnessHistoryGroup.addAndMakeVisible (&showShortTimeLoudnessHistory);
    
    showMomentaryLoudnessHistory.setClickingTogglesState (true);
    showMomentaryLoudnessHistory.getToggleStateValue().referTo (showMomentaryLoudnessHistoryValue);
    showMomentaryLoudnessHistory.setWantsKeyboardFocus (false);
    loudnessHistoryGroup.addAndMakeVisible (&showMomentaryLoudnessHistory);


	// Colours
	// -------

	setBackgroundColour (Colours::white.withAlpha (0.5f));

	loudnessBarSize.setColour (Slider::thumbColourId, Colours::black);
	loudnessBarRange.setColour (Slider::thumbColourId, Colours::black);
	loudnessBarRange.setColour (Slider::trackColourId, Colours::black);

	Colour loudnessHistoryGroupColour = Colour (0xff000000); // (Colour (0xff00ad00)); //Colour (0xff7f7f7f);
	loudnessHistoryGroup.setColour (GroupComponent::outlineColourId, loudnessHistoryGroupColour);
	loudnessHistoryGroup.setColour (GroupComponent::textColourId, loudnessHistoryGroupColour);

	Colour textColourOn = Colours::black;
	Colour buttonOnColour = Colour (0xffffff00);
	Colour textColourOff = Colour (0xff9a9a9a);
	Colour buttonOffColour = Colour (0xff383838);
	showIntegratedLoudnessHistory.setColour (TextButton::buttonOnColourId, Colours::yellow.darker ().darker ());
	showIntegratedLoudnessHistory.setColour (TextButton::textColourOnId, textColourOn);
	showIntegratedLoudnessHistory.setColour (TextButton::buttonColourId, buttonOffColour);
	showIntegratedLoudnessHistory.setColour (TextButton::textColourOffId, textColourOff);
	showLoudnessRangeHistory.setColour (TextButton::buttonOnColourId, Colours::blue.darker ());
	showLoudnessRangeHistory.setColour (TextButton::textColourOnId, textColourOn);
	showLoudnessRangeHistory.setColour (TextButton::buttonColourId, buttonOffColour);
	showLoudnessRangeHistory.setColour (TextButton::textColourOffId, textColourOff);
	showShortTimeLoudnessHistory.setColour (TextButton::buttonOnColourId, Colours::green);
	showShortTimeLoudnessHistory.setColour (TextButton::textColourOnId, textColourOn);
	showShortTimeLoudnessHistory.setColour (TextButton::buttonColourId, buttonOffColour);
	showShortTimeLoudnessHistory.setColour (TextButton::textColourOffId, textColourOff);
	showMomentaryLoudnessHistory.setColour (TextButton::buttonOnColourId, Colours::darkgreen);
	showMomentaryLoudnessHistory.setColour (TextButton::textColourOnId, textColourOn);
	showMomentaryLoudnessHistory.setColour (TextButton::buttonColourId, buttonOffColour);
	showMomentaryLoudnessHistory.setColour (TextButton::textColourOffId, textColourOff);
}

PreferencesPane::~PreferencesPane ()
{
}

void PreferencesPane::paint (Graphics &g)
{    
	// The PreferencesPane only has a paint function to enable the
	// JUCE_LIVE_CONSTANT functionality. Sadly, this makes the GUI of the host (!!!)
	// as well as the GUI of the LUFSMeter non-responsive with Reaper on Windows.
	// Therefore the "#ifndef    JUCE_ENABLE_LIVE_CONSTANT_EDITOR" has been added.

#if JUCE_ENABLE_LIVE_CONSTANT_EDITOR
	setBackgroundColour (JUCE_LIVE_CONSTANT (Colours::white.withAlpha (0.5f)));
#endif

	AnimatedSidePanel::paint (g);

#if JUCE_ENABLE_LIVE_CONSTANT_EDITOR
	loudnessBarSize.setColour (Slider::thumbColourId, JUCE_LIVE_CONSTANT (Colours::black));
	loudnessBarRange.setColour (Slider::thumbColourId, JUCE_LIVE_CONSTANT (Colours::black));
	loudnessBarRange.setColour (Slider::trackColourId, JUCE_LIVE_CONSTANT (Colours::black));

	Colour loudnessHistoryGroupColour = JUCE_LIVE_CONSTANT (Colour (0xff000000)); // (Colour (0xff00ad00)); //Colour (0xff7f7f7f);
	loudnessHistoryGroup.setColour (GroupComponent::outlineColourId, loudnessHistoryGroupColour);
	loudnessHistoryGroup.setColour (GroupComponent::textColourId, loudnessHistoryGroupColour);

	Colour textColourOn = JUCE_LIVE_CONSTANT (Colours::black);
	Colour buttonOnColour = JUCE_LIVE_CONSTANT (Colour (0xffffff00));
	Colour textColourOff = JUCE_LIVE_CONSTANT (Colour (0xff9a9a9a));
	Colour buttonOffColour = JUCE_LIVE_CONSTANT (Colour (0xff383838));
	showIntegratedLoudnessHistory.setColour (TextButton::buttonOnColourId, Colours::yellow.darker ().darker ());
	showIntegratedLoudnessHistory.setColour (TextButton::textColourOnId, textColourOn);
	showIntegratedLoudnessHistory.setColour (TextButton::buttonColourId, buttonOffColour);
	showIntegratedLoudnessHistory.setColour (TextButton::textColourOffId, textColourOff);
	showLoudnessRangeHistory.setColour (TextButton::buttonOnColourId, Colours::blue.darker ());
	showLoudnessRangeHistory.setColour (TextButton::textColourOnId, textColourOn);
	showLoudnessRangeHistory.setColour (TextButton::buttonColourId, buttonOffColour);
	showLoudnessRangeHistory.setColour (TextButton::textColourOffId, textColourOff);
	showShortTimeLoudnessHistory.setColour (TextButton::buttonOnColourId, Colours::green);
	showShortTimeLoudnessHistory.setColour (TextButton::textColourOnId, textColourOn);
	showShortTimeLoudnessHistory.setColour (TextButton::buttonColourId, buttonOffColour);
	showShortTimeLoudnessHistory.setColour (TextButton::textColourOffId, textColourOff);
	showMomentaryLoudnessHistory.setColour (TextButton::buttonOnColourId, Colours::darkgreen);
	showMomentaryLoudnessHistory.setColour (TextButton::textColourOnId, textColourOn);
	showMomentaryLoudnessHistory.setColour (TextButton::buttonColourId, buttonOffColour);
	showMomentaryLoudnessHistory.setColour (TextButton::textColourOffId, textColourOff);
#endif
}

void PreferencesPane::resized()
{
    AnimatedSidePanel::resized();
    
    const int iconSize = 40;
    const int iconSizeSlim = 0.5 * iconSize;
    const int sliderHeight = iconSize;
    const int loudnessBarY = 2 * borderSize + titleHeight;
    const int sizeSliderWidth = getWidth() - 2 * borderSize - iconSize - iconSizeSlim - topRightHandleWidth;
    const int loudnessBarSizeLeftIconX = borderSize;
    loudnessBarSizeLeftIcon->setTransformToFit (Rectangle<float>(loudnessBarSizeLeftIconX, loudnessBarY, iconSize, iconSize),
                                                RectanglePlacement::centred);
    const int loudnessBarSizeX = loudnessBarSizeLeftIconX + iconSize;
    loudnessBarSize.setBounds (loudnessBarSizeX, loudnessBarY, sizeSliderWidth, sliderHeight);
    loudnessBarSizeRightIcon->setTransformToFit (Rectangle<float>(loudnessBarSizeX + sizeSliderWidth, loudnessBarY, iconSizeSlim, iconSize),
                                                 RectanglePlacement::centred);
    
    const int loudnessBarRangeY = loudnessBarY + sliderHeight + borderSize;
    const int rangeSliderWidth = getWidth() - 2 * borderSize - iconSizeSlim - topRightHandleWidth;
    const int loudnessBarRangeLeftIconX = borderSize;
    loudnessBarRangeLeftIcon->setTransformToFit (Rectangle<float>(loudnessBarRangeLeftIconX, loudnessBarRangeY, iconSizeSlim, iconSize),
                                                 RectanglePlacement::centred);
    loudnessBarRange.setBounds (loudnessBarRangeLeftIconX + iconSizeSlim, loudnessBarRangeY, rangeSliderWidth, sliderHeight);

    loudnessHistoryGroup.setBounds(borderSize,
                                   loudnessBarRangeY + sliderHeight + borderSize,
                                   getWidth() - 2 * borderSize - topRightHandleWidth,
                                   15 + 24 + 10);
    
    const int buttonSize = 24;
    const int spaceBetweenButtons = 6;
    int buttonX = (loudnessHistoryGroup.getWidth() - 5 * buttonSize - 3 * spaceBetweenButtons) / 2;
    const int buttonY = 15; // loudnessBarRangeY + sliderHeight + borderSize + 6;
    showIntegratedLoudnessHistory.setBounds (buttonX, buttonY, buttonSize, buttonSize);
    buttonX += buttonSize + borderSize + 6;
    showLoudnessRangeHistory.setBounds (buttonX, buttonY, 2 * buttonSize, buttonSize);
    buttonX += 2 * buttonSize + borderSize + 6;
    showShortTimeLoudnessHistory.setBounds(buttonX, buttonY, buttonSize, buttonSize);
    buttonX += buttonSize + borderSize + 6;
    showMomentaryLoudnessHistory.setBounds(buttonX, buttonY, buttonSize, buttonSize);
}