/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#include "DRowAudioEditorComponent.h"

//==============================================================================
DRowAudioEditorComponent::DRowAudioEditorComponent (DRowAudioFilter* const ownerFilter)
    : AudioProcessorEditor (ownerFilter)
{
	// set look and feel
	customLookAndFeel = new dRowLookAndFeel;
        customLookAndFeel->setColour(Label::textColourId, (Colours::black).withBrightness(0.9f));
	setLookAndFeel(customLookAndFeel);

	for (int i = 0; i < noParams; i++)
	{
            Slider* const slider = new Slider(String("param") + String(i));

            String labelName = ownerFilter->getParameterName(i);

            if (labelName == "Input Gain")
                labelName = "In Gain";
            else if (labelName == "Output Gain")
                labelName = "Out Gain";

            slider->addListener (this);
            slider->setColour (Slider::thumbColourId, Colours::grey);
            slider->setColour (Slider::textBoxTextColourId, Colour (0xff78f4ff));
            slider->setColour (Slider::textBoxBackgroundColourId, Colour(0xFF455769).withBrightness(0.15f));
            slider->setColour (Slider::textBoxOutlineColourId, (Colour (0xffffffff)).withBrightness(0.5f));
            slider->setTextBoxStyle (Slider::TextBoxRight, false, 60, 18);

            Label* const label = new Label(String("Label") + String(i), labelName);
            label->setJustificationType(Justification::left);
            label->attachToComponent(slider, false);

            ownerFilter->getParameterPointer(i)->setupSlider(*slider);

            addAndMakeVisible(slider);
            addAndMakeVisible(label);

            sliders.add(slider);
            sliderLabels.add(label);
	}

	sliders[INGAIN]->setSliderStyle(Slider::RotaryVerticalDrag);
	sliders[INGAIN]->setColour(Slider::rotarySliderFillColourId, Colours::grey);
	sliders[INGAIN]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 18);
	sliderLabels[INGAIN]->attachToComponent(sliders[INGAIN], false);
	sliderLabels[INGAIN]->setJustificationType(Justification::centred);
	sliders[COLOUR]->setSliderStyle(Slider::RotaryVerticalDrag);
	sliders[COLOUR]->setColour(Slider::rotarySliderFillColourId, Colours::grey);
	sliders[COLOUR]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 18);
	sliderLabels[COLOUR]->attachToComponent(sliders[COLOUR], false);
	sliderLabels[COLOUR]->setJustificationType(Justification::centred);
	sliders[OUTGAIN]->setSliderStyle(Slider::RotaryVerticalDrag);
	sliders[OUTGAIN]->setColour(Slider::rotarySliderFillColourId, Colours::grey);
	sliders[OUTGAIN]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 18);
	sliderLabels[OUTGAIN]->attachToComponent(sliders[OUTGAIN], false);
	sliderLabels[OUTGAIN]->setJustificationType(Justification::centred);

    // set our component's initial size to be the last one that was stored in the filter's settings
    setSize (250, 200);

    // register ourselves with the filter - it will use its ChangeBroadcaster base
    // class to tell us when something has changed, and this will call our changeListenerCallback()
    // method.
    ownerFilter->addChangeListener (this);

    updateParametersFromFilter();
}

DRowAudioEditorComponent::~DRowAudioEditorComponent()
{
    getFilter()->removeChangeListener (this);
    sliders.clear();
    sliderLabels.clear();
    deleteAllChildren();
}

//==============================================================================
void DRowAudioEditorComponent::paint (Graphics& g)
{
	Colour backgroundColour(0xFF455769);
	backgroundColour = backgroundColour.withBrightness(0.4f);

	g.setColour(backgroundColour);
	g.fillRoundedRectangle(0, 0, getWidth(), getHeight(), 10);

	ColourGradient topHighlight(Colours::white.withAlpha(0.3f),
								0, 0,
								Colours::white.withAlpha(0.0f),
								0, 0 + 15,
								false);

	ColourGradient topColor(topHighlight);
	g.setGradientFill(topColor);
	g.fillRoundedRectangle(0, 0, getWidth(), 30, 10);

	ColourGradient outlineGradient(Colours::white,
								  0, 0,
								  backgroundColour.withBrightness(0.5f),
								  0, 20,
								  false);
	g.setGradientFill(outlineGradient);
	g.drawRoundedRectangle(0, 0, getWidth(), getHeight(), 10, 1.0f);
}

void DRowAudioEditorComponent::resized()
{
	sliders[PRE]->setBounds(5, 30, getWidth()-10, 20);
	sliders[INGAIN]->setBounds(getWidth()*0.5f - 110, 75, 70, 70);
	sliders[COLOUR]->setBounds(getWidth()*0.5f - 35, 75, 70, 70);
	sliders[OUTGAIN]->setBounds(getWidth()*0.5f + 40, 75, 70, 70);
	sliders[POST]->setBounds(5, getHeight()-25, getWidth()-10, 20);
}

//==============================================================================
void DRowAudioEditorComponent::sliderValueChanged (Slider* changedSlider)
{
    DRowAudioFilter* currentFilter = getFilter();

	for (int i = 0; i < noParams; i++)
        {
		if ( changedSlider == sliders[i] )
                {
			currentFilter->setScaledParameterNotifyingHost (i, (float) sliders[i]->getValue());
                        break;
                }
        }
}

void DRowAudioEditorComponent::sliderDragStarted(Slider* changedSlider)
{
	DRowAudioFilter* currentFilter = getFilter();

	for (int i = 0; i < noParams; i++)
        {
		if ( changedSlider == sliders[i] )
                {
			currentFilter->beginParameterChangeGesture(i);
                        break;
                }
        }
}
void DRowAudioEditorComponent::sliderDragEnded(Slider* changedSlider)
{
	DRowAudioFilter* currentFilter = getFilter();

	for (int i = 0; i < noParams; i++)
        {
		if ( changedSlider == sliders[i] )
                {
			currentFilter->endParameterChangeGesture(i);
                        break;
                }
        }
}

void DRowAudioEditorComponent::changeListenerCallback (ChangeBroadcaster* source)
{
    // this is the filter telling us that it's changed, so we'll update our
    // display of the time, midi message, etc.
    updateParametersFromFilter();
}

//==============================================================================
void DRowAudioEditorComponent::updateParametersFromFilter()
{
    DRowAudioFilter* const filter = getFilter();

    float tempParamVals[noParams];

    // we use this lock to make sure the processBlock() method isn't writing
    // to variables while we're trying to read it them
    filter->getCallbackLock().enter();

    for (int i = 0; i < noParams; i++)
         tempParamVals[i] =  filter->getScaledParameter (i);

    // ..release the lock ASAP
    filter->getCallbackLock().exit();


    /* Update our sliders

       (note that it's important here to tell the slider not to send a change
       message, because that would cause it to call the filter with a parameter
       change message again, and the values would drift out.
    */
    for (int i = 0; i < noParams; i++)
         sliders[i]->setValue (tempParamVals[i], dontSendNotification);
}
