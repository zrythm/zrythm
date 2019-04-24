/*
  ==============================================================================
 
    SpectrumAnalyserAudioProcessorEditor.h
    Created: 10 Jun 2014 8:19:00pm
    Author:  Samuel Gaehwiler
 
  ==============================================================================
 */

#ifndef SPECTRUM_ANALYSER_AUDIO_PROCESSOR_EDITOR_H_INCLUDED
#define SPECTRUM_ANALYSER_AUDIO_PROCESSOR_EDITOR_H_INCLUDED

#include "SpectrumAnalyserHeader.h"
#include "SpectrumAnalyserAudioProcessor.h"
#include "SamWithBubble.h"
#include "SpectrumViewer.h"


//==============================================================================
/**
*/
class SpectrumAnalyserAudioProcessorEditor  : public AudioProcessorEditor,
                                              public Value::Listener
{
public:
    SpectrumAnalyserAudioProcessorEditor (SpectrumAnalyserAudioProcessor* ownerFilter,
                                          Value& repaintSpectrumViewer,
                                          drow::Buffer& spectrumMagnitudeBuffer,
                                          Value& detectedFrequency);
    ~SpectrumAnalyserAudioProcessorEditor();

    //==============================================================================
    // This is just a standard Juce paint method...
    void paint (Graphics& g);
    
    void resized();
    
    /** The value listener method. */
    void valueChanged (Value & value);
    
private:
    SpectrumAnalyserAudioProcessor* getProcessor() const;
    
    SpectrumViewer spectrumViewer;
    Value sampleRate;
    Label header;
    SamWithBubble samWithBubble;
    
    ScopedPointer<ResizableCornerComponent> resizer;
    /** Specifies the maximum size of the window. */
    ComponentBoundsConstrainer resizeLimits;
};


#endif  // SPECTRUM_ANALYSER_AUDIO_PROCESSOR_EDITOR_H_INCLUDED
