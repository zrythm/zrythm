#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED

#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "ReFineLookAndFeel.h"
#include "Visualisation.h"

class ReFinedAudioProcessorEditor  : public AudioProcessorEditor, public Timer
{
public:
    ReFinedAudioProcessorEditor (ReFinedAudioProcessor&);
    ~ReFinedAudioProcessorEditor();

    void paint (Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:

    ReFinedAudioProcessor& processor;

    Image background;

    RefinedSlider redSlider;
    RefinedSlider greenSlider;
    RefinedSlider blueSlider;
    X2Button x2Button;

    Visualisation visualisation;    

    SharedResourcePointer<RefineLookAndFeel> refinedLookAndFeel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReFinedAudioProcessorEditor)
};


#endif  // PLUGINEDITOR_H_INCLUDED
