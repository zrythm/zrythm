#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
ReFinedAudioProcessorEditor::ReFinedAudioProcessorEditor (ReFinedAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      background (ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize)),
      redSlider (ImageCache::getFromMemory(BinaryData::red_png, BinaryData::red_pngSize), ImageCache::getFromMemory(BinaryData::vu_red_png, BinaryData::vu_red_pngSize), *p.parameters, "red"),
      greenSlider (ImageCache::getFromMemory(BinaryData::green_png, BinaryData::green_pngSize), ImageCache::getFromMemory (BinaryData::vu_green_png, BinaryData::vu_green_pngSize), *p.parameters, "green"),
      blueSlider (ImageCache::getFromMemory(BinaryData::blue_png, BinaryData::blue_pngSize), ImageCache::getFromMemory(BinaryData::vu_blue_png, BinaryData::vu_blue_pngSize), *p.parameters, "blue"),
      x2Button (*p.parameters, "x2"),
      visualisation (p.getDsp())
{
    setLookAndFeel(refinedLookAndFeel);

    addAndMakeVisible(redSlider);
    addAndMakeVisible(greenSlider);
    addAndMakeVisible(blueSlider);
    addAndMakeVisible(x2Button);

    addAndMakeVisible(visualisation);

    setSize (450, 300);
    startTimer(100);
}

ReFinedAudioProcessorEditor::~ReFinedAudioProcessorEditor()
{
}

//==============================================================================
void ReFinedAudioProcessorEditor::paint (Graphics& g)
{
    g.setColour(Colours::black);
    g.drawImageAt(background, 0, 0);

}

void ReFinedAudioProcessorEditor::resized()
{
    const int sliderWidth = 64;
    const int sliderHeight = 64;
    const int ySlider = 221;
    const int yButton = 7;
    const int xRed = 65;
    const int xGreen = 193;
    const int xBlue = 321;
    const int xButton = 209;
    const int buttonWidth = 32;
    const int buttonHeight = 16;

    redSlider.setBounds(xRed, ySlider, sliderWidth, sliderHeight);
    greenSlider.setBounds(xGreen, ySlider, sliderWidth, sliderHeight);
    blueSlider.setBounds(xBlue, ySlider, sliderWidth, sliderHeight);
    x2Button.setBounds(xButton, yButton, buttonWidth, buttonHeight);
    visualisation.setBounds(31, 35, 388, 150);
}

void ReFinedAudioProcessorEditor::timerCallback()
{
    const RefineDsp& dsp(processor.getDsp());
    const float transient = dsp.getTransient();
    const float nonTransient = dsp.getNonTransient();
    const float level = dsp.getLevel();

    redSlider.setVuValue(nonTransient);
    greenSlider.setVuValue(level);
    blueSlider.setVuValue(transient);
}
