/*
 ===============================================================================
 
 LUFSMeterAudioProcessorEditor.cpp
 
 
 This file is part of the LUFS Meter audio measurement plugin.
 Copyright 2011-12 by Klangfreund, Samuel Gaehwiler.
 
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


#include "LUFSMeterAudioProcessor.h"
#include "LUFSMeterAudioProcessorEditor.h"


//==============================================================================
LUFSMeterAudioProcessorEditor::LUFSMeterAudioProcessorEditor (LUFSMeterAudioProcessor* ownerFilter)
  : AudioProcessorEditor (ownerFilter),
    momentaryLoudnessValue (var(-300.0)),
    shortTermLoudnessValue (var(-300.0)),
    loudnessRangeStartValue (var(-300.0)),
    loudnessRangeEndValue (var(-300.0)),
    loudnessRangeValue(var(0.0)),
    integratedLoudnessValue (var(-300.0)),
    distanceBetweenLoudnessBarAndTop (10),
    distanceBetweenLoudnessBarAndBottom (32),
    backgroundGridCaption (distanceBetweenLoudnessBarAndTop, 
                           distanceBetweenLoudnessBarAndBottom, 
                           getProcessor()->loudnessBarMinValue, 
                           getProcessor()->loudnessBarMaxValue),
    momentaryLoudnessBar (getProcessor()->loudnessBarMinValue,
                          getProcessor()->loudnessBarMaxValue),
    momentaryLoudnessBarSum (momentaryLoudnessValue,
                             getProcessor()->loudnessBarMinValue,
                             getProcessor()->loudnessBarMaxValue),
    shortTermLoudnessBar (shortTermLoudnessValue,
                          getProcessor()->loudnessBarMinValue, 
                          getProcessor()->loudnessBarMaxValue),
    loudnessRangeBar (loudnessRangeStartValue,
                      loudnessRangeEndValue,
                      getProcessor()->loudnessBarMinValue,
                      getProcessor()->loudnessBarMaxValue),
    integratedLoudnessBar (integratedLoudnessValue,
                           getProcessor()->loudnessBarMinValue, 
                           getProcessor()->loudnessBarMaxValue),
    momentaryLoudnessCaption (String(), "M"),
    shortTermLoudnessCaption (String(), "S"),
    loudnessRangeCaption (String(), "LRA"),
    integratedLoudnessCaption (String(), "I"),
    momentaryLoudnessHistory (momentaryLoudnessValue, getProcessor()->loudnessBarMinValue, getProcessor()->loudnessBarMaxValue),
    shortTermLoudnessHistory (shortTermLoudnessValue, getProcessor()->loudnessBarMinValue, getProcessor()->loudnessBarMaxValue),
    loudnessRangeHistory (loudnessRangeStartValue, loudnessRangeEndValue, getProcessor()->loudnessBarMinValue, getProcessor()->loudnessBarMaxValue),
    integratedLoudnessHistory (integratedLoudnessValue, getProcessor()->loudnessBarMinValue, getProcessor()->loudnessBarMaxValue),
    preferencesPane(getProcessor()->loudnessBarWidth,
                    getProcessor()->loudnessBarMinValue,
                    getProcessor()->loudnessBarMaxValue,
                    getProcessor()->showIntegratedLoudnessHistory,
                    getProcessor()->showLoudnessRangeHistory,
                    getProcessor()->showShortTermLoudnessHistory,
                    getProcessor()->showMomentaryLoudnessHistory)
{
    LookAndFeel::setDefaultLookAndFeel(&lookAndFeelV3);
    
    // Add the background
    addAndMakeVisible (&backgroundGrid);
    addAndMakeVisible (&backgroundGridCaption);
    addAndMakeVisible(&backgroundVerticalLinesAndCaption);
    
    // TEMP:
    // Add a label that will display the current timecode and status..
    //    addAndMakeVisible (&infoLabel);
    //    infoLabel.setColour (Label::textColourId, Colours::green);
    
    Colour momentaryLoudnessColour = Colours::darkgreen;
    Colour momentaryLoudnessSumColour = Colours::darkgreen.darker().darker();
    Colour loudnessRangeColour = Colours::blue.darker();
    Colour loudnessRangeColourTransparent = loudnessRangeColour.withAlpha(0.3f);
    Colour integratedLoudnessColour = Colours::yellow.darker().darker();
    
    
    // Add the meter bars
    momentaryLoudnessBarSum.setColour (momentaryLoudnessSumColour);
    addAndMakeVisible(&momentaryLoudnessBarSum);
    momentaryLoudnessBar.setColour (momentaryLoudnessColour);
    addAndMakeVisible (&momentaryLoudnessBar);
    
    addAndMakeVisible (&shortTermLoudnessBar);

    loudnessRangeBar.setColour (loudnessRangeColourTransparent);
    addAndMakeVisible (&loudnessRangeBar);
    
    integratedLoudnessBar.setColour (integratedLoudnessColour);
    addAndMakeVisible (&integratedLoudnessBar);
    
    
    // Add the numeric values
    momentaryLoudnessNumeric.setColour(momentaryLoudnessColour);
    addAndMakeVisible (&momentaryLoudnessNumeric);
    momentaryLoudnessNumeric.getLoudnessValueObject().referTo(momentaryLoudnessValue);
    
    addAndMakeVisible (&shortTermLoudnessNumeric);
    shortTermLoudnessNumeric.getLoudnessValueObject().referTo(shortTermLoudnessValue);
    
    loudnessRangeNumeric.setColour(loudnessRangeColour);
    addAndMakeVisible (&loudnessRangeNumeric);
    loudnessRangeNumeric.getLoudnessValueObject().referTo(loudnessRangeValue);
    
    integratedLoudnessNumeric.setColour(integratedLoudnessColour);
    addAndMakeVisible (&integratedLoudnessNumeric);
    integratedLoudnessNumeric.getLoudnessValueObject().referTo(integratedLoudnessValue);
    
    
    // Add the captions
    const int fontHeight = 16;
    const Font fontForCaptions (fontHeight);
    const Justification justification (Justification::horizontallyCentred);
    
    momentaryLoudnessCaption.setFont(fontForCaptions);
    momentaryLoudnessCaption.setColour (Label::textColourId, momentaryLoudnessColour);
    // momentaryLoudnessCaption.setColour (Label::backgroundColourId, Colours::red);
    momentaryLoudnessCaption.setJustificationType(justification);
    addAndMakeVisible (&momentaryLoudnessCaption);
    
    shortTermLoudnessCaption.setFont(fontForCaptions);
    shortTermLoudnessCaption.setColour (Label::textColourId, Colours::green);
    shortTermLoudnessCaption.setJustificationType(justification);
    addAndMakeVisible (&shortTermLoudnessCaption);

    loudnessRangeCaption.setFont(fontForCaptions);
    loudnessRangeCaption.setColour (Label::textColourId, loudnessRangeColour);
    loudnessRangeCaption.setJustificationType(justification);
    addAndMakeVisible (&loudnessRangeCaption);
    
    integratedLoudnessCaption.setFont(fontForCaptions);
    integratedLoudnessCaption.setColour (Label::textColourId, integratedLoudnessColour);
    integratedLoudnessCaption.setJustificationType(justification);
    addAndMakeVisible (&integratedLoudnessCaption);
    
    
    // Add the loudness history graphs
    addAndMakeVisible(&loudnessHistoryGroup);
    
    loudnessRangeHistory.setColour (loudnessRangeColourTransparent);
    loudnessRangeHistory.setVisible (bool(getProcessor()->showLoudnessRangeHistory.getValue()));
    loudnessHistoryGroup.addChildComponent(&loudnessRangeHistory);
    
    momentaryLoudnessHistory.setColour(momentaryLoudnessSumColour);
    momentaryLoudnessHistory.setVisible (bool(getProcessor()->showMomentaryLoudnessHistory.getValue()));
    loudnessHistoryGroup.addChildComponent(&momentaryLoudnessHistory);

    shortTermLoudnessHistory.setVisible (bool(getProcessor()->showShortTermLoudnessHistory.getValue()));
    loudnessHistoryGroup.addChildComponent(&shortTermLoudnessHistory);

    integratedLoudnessHistory.setColour (integratedLoudnessColour);
    integratedLoudnessHistory.setVisible (bool(getProcessor()->showIntegratedLoudnessHistory.getValue()));
    loudnessHistoryGroup.addChildComponent(&integratedLoudnessHistory);
    
    
    // Add the reset button
    resetButton.addListener(this);
    resetButton.setButtonText("reset");
    resetButton.setColour(TextButton::buttonColourId, Colours::darkred);
    resetButton.setColour(TextButton::textColourOffId, Colours::lightgrey);
    addAndMakeVisible (&resetButton);
    
    // Add the preferences pane
    addAndMakeVisible (&preferencesPane);
    preferencesPane.setTopLeftPosition(- preferencesPane.getWidthWithoutHandle(), 50);
    
    // Add the triangular resizer component for the bottom-right of the UI.
    addAndMakeVisible (resizer = new ResizableCornerComponent (this, &resizeLimits));
    
    resizeLimits.setMinimumSize(150, 150);
    
    // Set our component's initial size to be the last one that was stored in 
    // the filter's settings.
    setSize (ownerFilter->lastUIWidth,
             ownerFilter->lastUIHeight);
    
    // Listen to some Values
    getProcessor()->loudnessBarWidth.addListener (this);
    getProcessor()->showIntegratedLoudnessHistory.addListener (this);
    getProcessor()->showLoudnessRangeHistory.addListener (this);
    getProcessor()->showShortTermLoudnessHistory.addListener (this);
    getProcessor()->showMomentaryLoudnessHistory.addListener (this);
    getProcessor()->numberOfInputChannels.addListener (this);
    
    momentaryLoudnessHistory.reset();
    shortTermLoudnessHistory.reset();
    loudnessRangeHistory.reset();
    integratedLoudnessHistory.reset();
    
    // Start the timer which will refresh the GUI elements.
    const int refreshIntervalInMilliseconds = 50;
    startTimer (refreshIntervalInMilliseconds);
}

LUFSMeterAudioProcessorEditor::~LUFSMeterAudioProcessorEditor()
{
    if (getProcessor())
    {
        getProcessor()->loudnessBarWidth.removeListener (this);
        getProcessor()->showIntegratedLoudnessHistory.removeListener (this);
        getProcessor()->showLoudnessRangeHistory.removeListener (this);
        getProcessor()->showShortTermLoudnessHistory.removeListener (this);
        getProcessor()->showMomentaryLoudnessHistory.removeListener (this);
    }
}

//==============================================================================
void LUFSMeterAudioProcessorEditor::paint (Graphics& g)
{
    // Draw the background
    g.fillAll(Colours::black);
}

void LUFSMeterAudioProcessorEditor::resized()
{
    // DEB("Height of main component = " + String(getHeight()))
    
    resizeGuiComponents();

    // Some more fix sized components:
    // TEMP
    //infoLabel.setBounds (10, 4, 380, 25);
    
//    resetButton.setBounds(10, getHeight()-35, 80, 25);
    resetButton.setBounds(12, 12, 50, 25);
    
    resizer->setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
    
    getProcessor()->lastUIWidth = getWidth();
    getProcessor()->lastUIHeight = getHeight();
}


// This timer periodically updates the labels.
void LUFSMeterAudioProcessorEditor::timerCallback()
{
//    AudioPlayHead::CurrentPositionInfo newPos (getProcessor()->lastPosInfo);
//    
//    if (lastDisplayedPosition != newPos)
//        displayPositionInfo (newPos);
    
    // momentary loudness values
    // -------------------------
    momentaryLoudnessValue.setValue (getProcessor()->getMomentaryLoudness());

    momentaryLoudnessBar.setLoudness (getProcessor()->getMomentaryLoudnessForIndividualChannels());
    
    /*
    // source:
    const Array<float>& momentaryLoudnessFromEbu128LM = getProcessor()->getMomentaryLoudnessForIndividualChannels();
    // destination:
    Array<var> momentaryLoudness;
    
    
    for (int k = 0; k != momentaryLoudnessFromEbu128LM.size() ; ++k)
    {
        double momentaryLoudnessOfTheKthChannel = double (momentaryLoudnessFromEbu128LM[k]);
        momentaryLoudness.add (momentaryLoudnessOfTheKthChannel);
    }
    */

    // short loudness values
    // ---------------------
    float shortTermLoudness = getProcessor()->getShortTermLoudness();
    jassert(shortTermLoudness > -400);
    shortTermLoudnessValue.setValue(shortTermLoudness);

    // loudness range values
    // ---------------------
    float loudnessRange = getProcessor()->getLoudnessRange();
    jassert(loudnessRange > -400);
    loudnessRangeValue.setValue(loudnessRange);
    
    float loudnessRangeStart = getProcessor()->getLoudnessRangeStart();
    jassert (loudnessRangeStart > -400);
    loudnessRangeStartValue.setValue(loudnessRangeStart);
    
    float loudnessRangeEnd = getProcessor()->getLoudnessRangeEnd();
    jassert (loudnessRangeEnd > -400);
    loudnessRangeEndValue.setValue(loudnessRangeEnd);
    
    // integrated loudness values
    // --------------------------
    float integratedLoudness = getProcessor()->getIntegratedLoudness();
    jassert(integratedLoudness > -400);
    integratedLoudnessValue.setValue(integratedLoudness);
}


void LUFSMeterAudioProcessorEditor::buttonClicked(Button* button)
{
    if (button == &resetButton)
    {
        getProcessor()->ebu128LoudnessMeter.reset();
        momentaryLoudnessHistory.reset();
        shortTermLoudnessHistory.reset();
        loudnessRangeHistory.reset();
        integratedLoudnessHistory.reset();
    }
}

void LUFSMeterAudioProcessorEditor::valueChanged (Value & value)
{
    if (value.refersToSameSourceAs (getProcessor()->loudnessBarWidth) ||
        value.refersToSameSourceAs (getProcessor()->numberOfInputChannels))
            // Because the width of the of the MultiChannelLoudnessBar needs to be a multiple
            // of the numberOfInputChannels.
    {
        resizeGuiComponents();
    }
    
    else if (value.refersToSameSourceAs (getProcessor()->showIntegratedLoudnessHistory))
    {
        integratedLoudnessHistory.setVisible (bool(getProcessor()->showIntegratedLoudnessHistory.getValue()));
    }
    
    else if (value.refersToSameSourceAs (getProcessor()->showLoudnessRangeHistory))
    {
        loudnessRangeHistory.setVisible (bool(getProcessor()->showLoudnessRangeHistory.getValue()));
    }
    
    else if (value.refersToSameSourceAs (getProcessor()->showShortTermLoudnessHistory))
    {
        shortTermLoudnessHistory.setVisible (bool(getProcessor()->showShortTermLoudnessHistory.getValue()));
    }
    
    else if (value.refersToSameSourceAs (getProcessor()->showMomentaryLoudnessHistory))
    {
        momentaryLoudnessHistory.setVisible (bool(getProcessor()->showMomentaryLoudnessHistory.getValue()));
    }
}


LUFSMeterAudioProcessor* LUFSMeterAudioProcessorEditor::getProcessor() const
{
    return static_cast <LUFSMeterAudioProcessor*> (getAudioProcessor());
}


void LUFSMeterAudioProcessorEditor::resizeGuiComponents ()
{
    // Distances
    // ---------
    const int loudnessBarWidth = int(getProcessor()->loudnessBarWidth.getValue()) * -1;
    
    // Ensure that the momentaryLoudnessBarWidth % numberOfChannels = 0.
    int momentaryLoudnessBarWidth = loudnessBarWidth;
    // Determine widthOfIndividualChannel.
    const int numberOfChannels = getProcessor()->getMomentaryLoudnessForIndividualChannels().size();
    if (numberOfChannels != 0)
    {
        const int widthOfIndividualChannel = loudnessBarWidth / numberOfChannels;
           // Integer division.
        momentaryLoudnessBarWidth = numberOfChannels * widthOfIndividualChannel;
    }
    
    const int spaceBetweenBars = jmin (loudnessBarWidth/5, 10);
        // This distance is also used for the border on the right side.
    const int heightOfNumericValues = loudnessBarWidth/3;
    const int heightOfLoudnessCaptions = heightOfNumericValues;
    distanceBetweenLoudnessBarAndBottom = heightOfNumericValues
                                          + heightOfLoudnessCaptions;
    const int loudnessBarBottomPosition = getHeight()
                                       - distanceBetweenLoudnessBarAndBottom;
    const int heightOfLoudnessBar = loudnessBarBottomPosition
                                    - distanceBetweenLoudnessBarAndTop;
    const int loudnessBarNumericTopPosition = getHeight()
                                           - distanceBetweenLoudnessBarAndBottom;
    const int loudnessBarCaptionTopPosition = getHeight()
                                           - heightOfLoudnessCaptions;
    const int backgroundGridCaptionWidth = 35;
    
    // Background Grid
    backgroundGrid.setBounds(0,
                    distanceBetweenLoudnessBarAndTop,
                    getWidth(),
                    heightOfLoudnessBar);

    // Font for the loudnessCaptions
    const int fontHeight = heightOfLoudnessCaptions;
    const Font fontForCaptions (fontHeight);
    
    // Momentary Loudness
    const int momentaryLoudnessBarX = getWidth() - spaceBetweenBars 
                                                 - momentaryLoudnessBarWidth;
    momentaryLoudnessBarSum.setBounds(momentaryLoudnessBarX,
                                      distanceBetweenLoudnessBarAndTop,
                                      momentaryLoudnessBarWidth,
                                      heightOfLoudnessBar);
    momentaryLoudnessBar.setBounds(momentaryLoudnessBarX,
                                   distanceBetweenLoudnessBarAndTop,
                                   momentaryLoudnessBarWidth,
                                   heightOfLoudnessBar);
    momentaryLoudnessNumeric.setBounds (momentaryLoudnessBarX,
                                        loudnessBarNumericTopPosition,
                                        momentaryLoudnessBarWidth,
                                        heightOfNumericValues);
    momentaryLoudnessCaption.setBounds(momentaryLoudnessBarX,
                                       loudnessBarCaptionTopPosition,
                                       momentaryLoudnessBarWidth,
                                       heightOfLoudnessCaptions);
    momentaryLoudnessCaption.setFont(fontForCaptions);
    
    // Short Term Loudness
    const int shortTermLoudnessBarX = momentaryLoudnessBarX - spaceBetweenBars - loudnessBarWidth;
    shortTermLoudnessBar.setBounds(shortTermLoudnessBarX,
                                   distanceBetweenLoudnessBarAndTop,
                                   loudnessBarWidth,
                                   heightOfLoudnessBar);
    shortTermLoudnessNumeric.setBounds (shortTermLoudnessBarX,
                                        loudnessBarNumericTopPosition,
                                        loudnessBarWidth,
                                        heightOfNumericValues);
    shortTermLoudnessCaption.setBounds(shortTermLoudnessBarX,
                                       loudnessBarCaptionTopPosition,
                                       loudnessBarWidth,
                                       heightOfLoudnessCaptions);
    shortTermLoudnessCaption.setFont(fontForCaptions);
    
    // Loudness Range
    const int loudnessRangeBarX = shortTermLoudnessBarX - spaceBetweenBars - loudnessBarWidth;
    loudnessRangeBar.setBounds (loudnessRangeBarX,
                                distanceBetweenLoudnessBarAndTop,
                                loudnessBarWidth,
                                heightOfLoudnessBar);
    loudnessRangeNumeric.setBounds (loudnessRangeBarX,
                                    loudnessBarNumericTopPosition,
                                    loudnessBarWidth,
                                    heightOfNumericValues);
    loudnessRangeCaption.setBounds (loudnessRangeBarX,
                                    loudnessBarCaptionTopPosition,
                                    loudnessBarWidth,
                                    heightOfLoudnessCaptions);
    loudnessRangeCaption.setFont(fontForCaptions);
    
    // Integrated Loudness
    const int integratedLoudnessBarX = loudnessRangeBarX - spaceBetweenBars - loudnessBarWidth;
    integratedLoudnessBar.setBounds(integratedLoudnessBarX,
                                    distanceBetweenLoudnessBarAndTop,
                                    loudnessBarWidth,
                                    heightOfLoudnessBar);
    integratedLoudnessNumeric.setBounds (integratedLoudnessBarX,
                                         loudnessBarNumericTopPosition,
                                         loudnessBarWidth,
                                         heightOfNumericValues);
    integratedLoudnessCaption.setBounds(integratedLoudnessBarX,
                                        loudnessBarCaptionTopPosition,
                                        loudnessBarWidth,
                                        heightOfLoudnessCaptions);
    integratedLoudnessCaption.setFont(fontForCaptions);
    
    // Background grid caption
    const int backgroundGridCaptionX = integratedLoudnessBarX - spaceBetweenBars - backgroundGridCaptionWidth;
    backgroundGridCaption.setBounds(backgroundGridCaptionX, 0, backgroundGridCaptionWidth, loudnessBarBottomPosition + 32);
    
    // Background vertical lines and caption
    backgroundVerticalLinesAndCaption.setBounds(0, distanceBetweenLoudnessBarAndTop, jmax(backgroundGridCaptionX, 0), loudnessBarBottomPosition + 32 - distanceBetweenLoudnessBarAndTop);
    
    // Loudness history
    loudnessHistoryGroup.setBounds(0,
                                   distanceBetweenLoudnessBarAndTop,
                                   jmax(backgroundGridCaptionX, 0),
                                   heightOfLoudnessBar);
}

// quick-and-dirty function to format a timecode string
static const String timeToTimecodeString (const double seconds)
{
    const double absSecs = fabs (seconds);
    
    const int hours = (int) (absSecs / (60.0 * 60.0));
    const int mins  = ((int) (absSecs / 60.0)) % 60;
    const int secs  = ((int) absSecs) % 60;
    
    String s;
    if (seconds < 0)
        s = "-";
    
    s << String (hours).paddedLeft ('0', 2) << ":"
    << String (mins).paddedLeft ('0', 2) << ":"
    << String (secs).paddedLeft ('0', 2) << ":"
    << String (roundToInt (absSecs * 1000) % 1000).paddedLeft ('0', 3);
    
    return s;
}

#if 0
// quick-and-dirty function to format a bars/beats string
static const String ppqToBarsBeatsString (double ppq, double /*lastBarPPQ*/, int numerator, int denominator)
{
    if (numerator == 0 || denominator == 0)
        return "1|1|0";
    
    const int ppqPerBar = (numerator * 4 / denominator);
    const double beats  = (fmod (ppq, ppqPerBar) / ppqPerBar) * numerator;
    
    const int bar    = ((int) ppq) / ppqPerBar + 1;
    const int beat   = ((int) beats) + 1;
    const int ticks  = ((int) (fmod (beats, 1.0) * 960.0));
    
    String s;
    s << bar << '|' << beat << '|' << ticks;
    return s;
}
#endif

// Updates the text in our position label.
void LUFSMeterAudioProcessorEditor::displayPositionInfo (const AudioPlayHead::CurrentPositionInfo& pos)
{
    lastDisplayedPosition = pos;
    String displayText;
    displayText.preallocateBytes (128);
    
    displayText << String (pos.bpm, 2) << " bpm, "
    << pos.timeSigNumerator << '/' << pos.timeSigDenominator
    << "  -  " << timeToTimecodeString (pos.timeInSeconds);
    //    << "  -  " << ppqToBarsBeatsString (pos.ppqPosition, pos.ppqPositionOfLastBarStart,
    //                                        pos.timeSigNumerator, pos.timeSigDenominator);
    
    if (pos.isRecording)
        displayText << "  (recording)";
    else if (pos.isPlaying)
        displayText << "  (playing)";
    
    //infoLabel.setText (displayText, false);
}
