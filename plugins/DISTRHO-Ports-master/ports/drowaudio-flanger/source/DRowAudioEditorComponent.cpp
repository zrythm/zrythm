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
	customLookAndFeel = new dRowLookAndFeel;
        customLookAndFeel->setColour(Label::textColourId, Colours::white);
	setLookAndFeel(customLookAndFeel);

	for (int i = 0; i < noParams; i++)
	{
		sliders.add( new Slider(String("param") + String(i)) );
		addAndMakeVisible( sliders[i]);

		String labelName = ownerFilter->getParameterName(i);
		sliderLabels.add(new Label(String("Label") + String(i), labelName));
		addAndMakeVisible(sliderLabels[i]);
		sliderLabels[i]->setJustificationType(Justification::left);
		sliderLabels[i]->attachToComponent(sliders[i], false);

		sliders[i]->addListener (this);
		ownerFilter->getParameterPointer(i)->setupSlider(*sliders[i]);
		sliders[i]->setTextBoxStyle(Slider::TextBoxRight, false, 60, 18);

		sliders[i]->setColour (Slider::thumbColourId, Colours::grey);
		sliders[i]->setColour (Slider::textBoxTextColourId, Colour (0xff78f4ff));
		sliders[i]->setColour (Slider::textBoxBackgroundColourId, Colours::black);
		sliders[i]->setColour (Slider::textBoxOutlineColourId, Colour (0xff0D2474));
	}

	sliders[RATE]->setSliderStyle(Slider::RotaryVerticalDrag);
	sliders[RATE]->setColour(Slider::rotarySliderFillColourId, Colours::grey);
	sliders[RATE]->setTextBoxStyle(Slider::TextBoxBelow, false, 70, 18);
	sliderLabels[RATE]->attachToComponent(sliders[RATE], false);
	sliderLabels[RATE]->setJustificationType(Justification::centred);
	sliders[DEPTH]->setSliderStyle(Slider::RotaryVerticalDrag);
	sliders[DEPTH]->setColour(Slider::rotarySliderFillColourId, Colours::grey);
	sliders[DEPTH]->setTextBoxStyle(Slider::TextBoxBelow, false, 70, 18);
	sliderLabels[DEPTH]->attachToComponent(sliders[DEPTH], false);
	sliderLabels[DEPTH]->setJustificationType(Justification::centred);

    // set our component's size
    setSize (250, 225);

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

	g.setColour(backgroundColour);
	g.fillRoundedRectangle(0, 0, getWidth(), getHeight(), 10);

	ColourGradient topHighlight(Colours::white.withAlpha(0.3f),
								0, 0,
								Colours::white.withAlpha(0.0f),
								0, 0 + 15,
								false);

	ColourGradient topBrush(topHighlight);
	g.setGradientFill(topBrush);
	g.fillRoundedRectangle(0, 0, getWidth(), 30, 10);

	ColourGradient outlineGradient(Colours::white,
								  0, 00,
								  backgroundColour.withBrightness(0.5f),
								  0, 20,
								  false);
	g.setGradientFill(outlineGradient);
	g.drawRoundedRectangle(0, 0, getWidth(), getHeight(), 10, 1.0f);

}

void DRowAudioEditorComponent::resized()
{
	sliders[RATE]->setBounds(getWidth()*0.5f - 100, getHeight()-195, 100, 100);
	sliders[DEPTH]->setBounds(getWidth()*0.5f, getHeight()-195, 90, 100);
	sliders[FEEDBACK]->setBounds(5, getHeight()-70, getWidth()-10, 20);
	sliders[MIX]->setBounds(5, getHeight()-25, getWidth()-10, 20);
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

    // we use this lock to make sure the processBlock() method isn't writing to the
    // lastMidiMessage variable while we're trying to read it, but be extra-careful to
    // only hold the lock for a minimum amount of time..
    filter->getCallbackLock().enter();

    for(int i = 0; i < noParams; i++)
            tempParamVals[i] =  filter->getScaledParameter (i);

    // ..release the lock ASAP
    filter->getCallbackLock().exit();


    /* Update our slider.

       (note that it's important here to tell the slider not to send a change
       message, because that would cause it to call the filter with a parameter
       change message again, and the values would drift out.
    */
    for(int i = 0; i < noParams; i++)
            sliders[i]->setValue (tempParamVals[i], dontSendNotification);
}
