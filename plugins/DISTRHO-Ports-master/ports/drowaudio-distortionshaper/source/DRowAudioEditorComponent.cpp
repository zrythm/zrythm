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
	// set up the custom look and feel
	lookAndFeel = new dRowLookAndFeel();
        lookAndFeel->setColour (Label::textColourId, (Colours::black).withBrightness(0.9f));
        lookAndFeel->setColour (Slider::thumbColourId, Colours::grey);
        lookAndFeel->setColour (Slider::textBoxTextColourId, Colour (0xff78f4ff));
        lookAndFeel->setColour (Slider::textBoxBackgroundColourId, Colours::black);
        lookAndFeel->setColour (Slider::textBoxOutlineColourId, Colour (0xff0D2474));
	setLookAndFeel(lookAndFeel);

        for (int i = 0; i < noParams; i++)
        {
                Value* const value = new Value();
                value->addListener(this);
                value->setValue(ownerFilter->getScaledParameter(i));
                values.add(value);
        }

	// set up the sliders and labels based on their parameters
	for (int i = 0; i < X1; i++)
	{
            Slider* const slider = new Slider(String("param") + String(i));

            String labelName = parameterNames[i];

            if (labelName == "Input Gain")
                labelName = "Input";
            if (labelName == "Pre Filter")
                labelName = "Pre";
            if (labelName == "Post Filter")
                labelName = "Post";

            slider->addListener (this);
            slider->getValueObject().referTo (*values[i]);

            Label* const label = new Label(String("Label") + String(i), labelName);
            label->setJustificationType(Justification::centred);
            label->attachToComponent(slider, false);
            //label->setFont(12);

            ownerFilter->getParameterPointer(i)->setupSlider(*slider);

            addAndMakeVisible(slider);
            addAndMakeVisible(label);

            sliders.add(slider);
            labels.add(label);
	}

	sliders[INGAIN]->setSliderStyle(Slider::LinearVertical);
	sliders[INGAIN]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
	sliders[OUTGAIN]->setSliderStyle(Slider::LinearVertical);
	sliders[OUTGAIN]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);

	sliders[PREFILTER]->setSliderStyle(Slider::LinearVertical);
	sliders[PREFILTER]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
	sliders[POSTFILTER]->setSliderStyle(Slider::LinearVertical);
	sliders[POSTFILTER]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);

	addAndMakeVisible( graphComponent = new GraphComponent(ownerFilter->getDistortionBuffer(), ownerFilter->getDistortionBufferSize()) );
	graphComponent->setValuesToReferTo(*values[X1], *values[Y1], *values[X2], *values[Y2]);

    // set the UI component's size
    setSize (500, 220);

    // register ourselves with the filter - it will use its ChangeBroadcaster base
    // class to tell us when something has changed, and this will call our changeListenerCallback()
    // method.
    ownerFilter->addChangeListener (this);

    updateParametersFromFilter();
}

DRowAudioEditorComponent::~DRowAudioEditorComponent()
{
    getFilter()->removeChangeListener (this);
    values.clear();
    sliders.clear();
    labels.clear();
    deleteAllChildren();
}

//==============================================================================
void DRowAudioEditorComponent::paint (Graphics& g)
{
	// draw rounded background with highlight at top
	Colour backgroundColour(0xFF455769);
	backgroundColour = backgroundColour.withBrightness(0.4f);

	g.setColour(backgroundColour);
	g.fillRoundedRectangle(0, 0, getWidth(), getHeight(), 10);

	ColourGradient topHighlight(Colours::white.withAlpha(0.3f),
								0, 0,
								Colours::white.withAlpha(0.0f),
								0, 0 + 15,
								false);
	g.setGradientFill(topHighlight);
	g.fillRoundedRectangle(0, 0, getWidth(), 30, 10);

	ColourGradient outlineGradient(Colours::white,
								  0, 00,
								  backgroundColour.withBrightness(0.5f),
								  0, 20,
								  false);
	g.setGradientFill(outlineGradient);
	g.drawRoundedRectangle(0, 0, getWidth(), getHeight(), 10, 1.0f);


	int width = getWidth();

	// linea around graph display
	Rectangle<int> outlineRect((0.5*width)-101, 9, 202, 202);

	g.setColour(Colours::black);
	g.drawHorizontalLine(9, (0.5*width)-101, (0.5*width)+101);
	g.setColour(Colour().greyLevel(0.5));
	g.drawHorizontalLine(210, (0.5*width)-101, (0.5*width)+101);

	ColourGradient sideGradient(Colour().greyLevel(0.3),
								outlineRect.getX(), outlineRect.getY(),
								Colour().greyLevel(0.5),
								outlineRect.getX(), outlineRect.getBottom(),
								false);
	g.setGradientFill(sideGradient);
	g.fillRect(outlineRect.getX(), outlineRect.getY(), outlineRect.getWidth(), outlineRect.getHeight());
}

void DRowAudioEditorComponent::resized()
{
	int height = getHeight();
	int width = getWidth();

	sliders[INGAIN]->setBounds(10, 30, 60, height-40);
	sliders[PREFILTER]->setBounds(80, 30, 60, height-40);

	graphComponent->setBounds((0.5*width)-100, 10, 200, height - 20);

	sliders[POSTFILTER]->setBounds(width-140, 30, 60, height-40);
	sliders[OUTGAIN]->setBounds(width-70, 30, 60, height-40);
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

void DRowAudioEditorComponent::valueChanged(Value& value)
{
	DRowAudioFilter* currentFilter = getFilter();

	for (int i = 0; i < noParams; i++)
        {
		if ( value.refersToSameSourceAs(*values[i]) )
                {
                    currentFilter->setScaledParameterNotifyingHost (i, (float) values[i]->getValue());
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

    for (int i = 0; i < noParams; i++)
         tempParamVals[i] =  filter->getScaledParameter (i);

    // ..release the lock ASAP
    filter->getCallbackLock().exit();

    // Update our sliders
    for (int i = 0; i < noParams; i++)
         values[i]->setValue (tempParamVals[i]);
}
