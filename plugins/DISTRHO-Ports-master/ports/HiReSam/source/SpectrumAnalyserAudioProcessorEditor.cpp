/*
  ==============================================================================

    SpectrumAnalyserAudioProcessorEditor.cpp
    Created: 10 Jun 2014 8:19:00pm
    Author:  Samuel Gaehwiler

  ==============================================================================
*/

#include "SpectrumAnalyserAudioProcessor.h"
#include "SpectrumAnalyserAudioProcessorEditor.h"


//==============================================================================
SpectrumAnalyserAudioProcessorEditor::SpectrumAnalyserAudioProcessorEditor (SpectrumAnalyserAudioProcessor* ownerFilter,
                                                                            Value& repaintSpectrumViewer,
                                                                            drow::Buffer& spectrumMagnitudeBuffer,
                                                                            Value& detectedFrequency)
    : AudioProcessorEditor (ownerFilter),
      spectrumViewer (repaintSpectrumViewer, spectrumMagnitudeBuffer, detectedFrequency)
{
    sampleRate.addListener (this);
    // The sampleRate has already been set in the SpectrumAnalyserAudioProcessor
    // before the creation of the SpectrumAnalyserAudioProcessorEditor.
    // Because of this it is important to have the referTo call after the
    // addListener. Only in this order the valueChanged member function
    // will be called implicitly.
    sampleRate.referTo (getProcessor()->sampleRate);
    
    header.setText("High Resolution Spectrum Analyse Meter", dontSendNotification);
    Font headerFont = Font (18.0f);
    header.setFont (headerFont);
    header.setJustificationType(Justification::centred);
    header.setColour (Label::textColourId, Colours::lightgoldenrodyellow);
    header.setColour (Label::backgroundColourId, Colours::black);
    
    samWithBubble.referToFrequencyValue (spectrumViewer.getFrequencyToDisplay());
    
    addAndMakeVisible (&header);
    addAndMakeVisible (&spectrumViewer);
    addAndMakeVisible (&samWithBubble);
    
    // Add the triangular resizer component for the bottom-right of the UI.
    resizeLimits.setMinimumSize(360, 320);
    addAndMakeVisible (resizer = new ResizableCornerComponent (this, &resizeLimits));
    
    // The plugin's initial editor size.
    setSize (1000, 500);
}

SpectrumAnalyserAudioProcessorEditor::~SpectrumAnalyserAudioProcessorEditor()
{
}

//==============================================================================
void SpectrumAnalyserAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void SpectrumAnalyserAudioProcessorEditor::resized()
{
    header.setBounds(0, 0, getWidth(), 24);
    
    spectrumViewer.setBounds (0, header.getHeight(), getWidth(), getHeight() - header.getHeight());

    const int widthForSamWithBubble = 190;
    const int maxHeight = getHeight() - header.getHeight();
    const int height = jmin (190, maxHeight);
    samWithBubble.setBounds(getWidth() - widthForSamWithBubble, 0, widthForSamWithBubble, height);
    
    resizer->setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
}

void SpectrumAnalyserAudioProcessorEditor::valueChanged (Value & value)
{
    if (value.refersToSameSourceAs (sampleRate))
    {
        spectrumViewer.setSampleRate (sampleRate.getValue());
    }
}

SpectrumAnalyserAudioProcessor* SpectrumAnalyserAudioProcessorEditor::getProcessor() const
{
    return static_cast <SpectrumAnalyserAudioProcessor*> (getAudioProcessor());
}
