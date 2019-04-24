/*
  ==============================================================================

    SamWithBubble.cpp
    Created: 12 Jul 2014 9:17:15am
    Author:  Samuel Gaehwiler

  ==============================================================================
*/

#include "JuceHeader.h"
#include "SamWithBubble.h"

//==============================================================================
SamWithBubble::SamWithBubble()
  : frequencyValue (var(0))
{
    frequencyValue.addListener (this);
    
    pitchLabel.setColour (Label::textColourId, Colours::lightgoldenrodyellow);
//    pitchLabel.setColour (Label::backgroundColourId, Colours::red);
    Font pitchLabelFont = Font (16.0f);
    pitchLabel.setFont (pitchLabelFont);
    pitchLabel.setJustificationType(Justification::horizontallyCentred);
    addAndMakeVisible (&pitchLabel);
    
    setInterceptsMouseClicks(false, false);
}

SamWithBubble::~SamWithBubble()
{
    frequencyValue.removeListener (this);
}

void SamWithBubble::paint (Graphics& g)
{
    // image
    const int borderRight = 37;
    const int imageHeight = 80;
    const int imageWidth = 80;
    g.drawImage(ImageCache::getFromMemory (BinaryData::Samuel_Gaehwiler_png, BinaryData::Samuel_Gaehwiler_pngSize), getWidth() - imageWidth - borderRight, getHeight() - imageHeight, imageWidth, imageHeight, 0, 0, 300, 300);
    
    // bubble
    g.setColour (Colours::black);
    g.fillPath (bubblePath);
//    g.setColour (Colour (0xff6f6f6f));
    g.setColour (Colours::lightgoldenrodyellow);
    g.strokePath (bubblePath, PathStrokeType (2.5f));

    // bubble label and text
    pitchLabel.setBounds (44, 23, getWidth() - 80, 20);
    
    g.setColour (Colours::lightgoldenrodyellow);
    g.setFont (12.0f);
    Rectangle<int> bubblePathBounds = bubblePath.getBounds().getSmallestIntegerContainer();
    bubblePathBounds.removeFromTop (50);
    bubblePathBounds.removeFromLeft (20);
    const int maximumNumberOfLines = 3;
    const String bubbleText = String("Zviel Arbet? Plugin Idea?\n") + "078 624 68 64\n" + "sam@klangfreund.com";
    g.drawFittedText(bubbleText, bubblePathBounds, Justification::horizontallyCentred, maximumNumberOfLines);

    
    //    g.setColour (Colours::black);
    //    g.setFont (15.0f);
    //    g.drawFittedText ("Zviel Arbet? HiRe sam@klangfreund.com!",
    //                      0, 0, getWidth(), getHeight(),
    //                      Justification::centred, 1);

//    g.fillAll (Colours::white);   // clear the background
//
//    g.setColour (Colours::grey);
//    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component
//
//    g.setColour (Colours::lightblue);
//    g.setFont (14.0f);
//    g.drawText ("SamWithBubble", getLocalBounds(),
//                Justification::centred, true);   // draw some placeholder text
}

void SamWithBubble::resized()
{
    bubblePath.clear();
    //    bubblePath.startNewSubPath (136.0f, 80.0f);
    //    bubblePath.quadraticTo (176.0f, 24.0f, 328.0f, 32.0f);
    //    bubblePath.quadraticTo (472.0f, 40.0f, 472.0f, 104.0f);
    //    bubblePath.quadraticTo (472.0f, 192.0f, 232.0f, 176.0f);
    //    bubblePath.lineTo (184.0f, 216.0f);
    //    bubblePath.lineTo (200.0f, 168.0f);
    //    bubblePath.quadraticTo (96.0f, 136.0f, 136.0f, 80.0f);
    //    bubblePath.closeSubPath();
    float mostRight = getWidth() - 10.0f;
    float width = getWidth() - 30.0f;
    float height = 130.0f;
    bubblePath.startNewSubPath (mostRight - width, 0.292f * height);
    bubblePath.quadraticTo (mostRight - 0.881f * width, 0.0f, mostRight - 0.429f * width, 0.042f * height);
    bubblePath.quadraticTo (mostRight, 0.083f * height, mostRight, 0.416f * height);
    bubblePath.quadraticTo (mostRight, 0.875f * height, mostRight - 0.69f * width, 0.792f * height);
    bubblePath.lineTo (mostRight - 0.635f * width, height); // the tip of the bubble
    bubblePath.lineTo (mostRight - 0.810f * width, 0.75f * height);
    bubblePath.quadraticTo (mostRight - 1.119f * width, 0.635f * height, mostRight - width, 0.292f * height);
    bubblePath.closeSubPath();
}

void SamWithBubble::referToFrequencyValue (const Value & valueToReferTo)
{
    frequencyValue.referTo(valueToReferTo);
    // pitchLabel.getTextValue().referTo(valueToReferTo);
}

void SamWithBubble::valueChanged (Value & value)
{
    if (value.refersToSameSourceAs(frequencyValue))
    {
        const int frequency = frequencyValue.getValue();
        String pitchString = frequencyValue.toString();
        pitchString << " Hz";
        
        if (frequency > 7)
        {
            // TO DAVE: At 7 Hz, the function below returns the String "-1" (without a letter in front of it).
            pitchString << " (" << drow::Pitch::fromFrequency (frequency).getMidiNoteName() << ")";
        }

        pitchLabel.setText(pitchString, NotificationType::dontSendNotification);
    }
    
}