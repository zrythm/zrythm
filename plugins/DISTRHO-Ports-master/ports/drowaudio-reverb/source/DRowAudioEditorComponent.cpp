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

#include "includes.h"
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

	// set up the sliders and labels based on their parameters
	for (int i = 0; i < noParams; i++)
	{
		sliders.add( new Slider(String("param") + String(i)) );
		addAndMakeVisible( sliders[i]);

		String labelName = parameterNames[i];
		labels.add(new Label(String("Label") + String(i), labelName));
		labels[i]->setJustificationType(Justification::topLeft);
		labels[i]->attachToComponent(sliders[i], false);
		labels[i]->setFont(11);

		sliders[i]->addListener (this);
		sliders[i]->setRange (0.0, 1.0, 0.01);
		sliders[i]->setTextBoxStyle(Slider::TextBoxRight, false, 62, 20);

		ownerFilter->getParameterPointer(i)->setupSlider(*sliders[i]);
	}

	// add a few labels for the section headings
	labels.add(new Label(String("General"), String("General")));
	labels.add(new Label(String("Early Reflections"), String("Early Reflections")));
	labels.add(new Label(String("Reverb"), String("Reverb")));
	labels.add(new Label(String("Output"), String("Output")));
	for(int i = LABELGENERAL; i < noLabels; ++i) {
		addAndMakeVisible(labels[i]);
		labels[i]->setJustificationType(Justification::centredTop);
	}

	// set up a few slider to be vertical instead of the dfault horizontal
	sliders[DELTIME]->setSliderStyle(Slider::LinearVertical);
	sliders[DELTIME]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
	labels[DELTIME]->setJustificationType(Justification::centredTop);

	sliders[FBCOEFF]->setSliderStyle(Slider::LinearVertical);
	sliders[FBCOEFF]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);

	sliders[LOWEQ]->setSliderStyle(Slider::LinearVertical);
	sliders[LOWEQ]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
	sliders[HIGHEQ]->setTextBoxStyle(Slider::TextBoxBelow, false, 60, 20);
	sliders[HIGHEQ]->setSliderStyle(Slider::LinearVertical);
	sliders[WETDRYMIX]->setTextBoxStyle(Slider::TextBoxBelow, false, 62, 20);
	sliders[WETDRYMIX]->setSliderStyle(Slider::LinearVertical);


    // set the UI component's size
    setSize (548, 300);

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


	int height = getHeight();
	int width = getWidth();

	// draw section dividing lines
	lookAndFeel->drawInsetLine(g, width-208, 2, width-208, height, 2);
	lookAndFeel->drawInsetLine(g, 0, height-120, width-208, height-120, 2);
	lookAndFeel->drawInsetLine(g, (width-208)*0.5f, height-120, (width-208)*0.5f, height, 2);

	// draw section heading lines
	g.setColour(Colours::white.withAlpha(0.6f));
	g.drawHorizontalLine(24, 10, 160);
	g.drawHorizontalLine(24, 220, 335);

	g.drawHorizontalLine(24, 350, 410);
	g.drawHorizontalLine(24, 468, 540);

	g.drawHorizontalLine(194, 10,  25);
	g.drawHorizontalLine(194, 143, 160);

	g.drawHorizontalLine(194, 180, 220);
	g.drawHorizontalLine(194, 278, 330);
}

void DRowAudioEditorComponent::resized()
{
	int height = getHeight();
	int width = getWidth();

	labels[LABELGENERAL]->setBounds(0, 10, width-170, 20);
	sliders[PREDELAY]->setBounds(5, 60, 210, 20);
	sliders[SPREAD]->setBounds(5, 110, 210, 20);
	sliders[EARLYLATEMIX]->setBounds(5, 155, 210, 20);

	sliders[DELTIME]->setBounds(220, 55, 55, 120);
	sliders[FBCOEFF]->setBounds(280, 55, 55, 120);
	labels[DELTIME]->setSize(60, 24);
	labels[FBCOEFF]->setSize(60, 24);

	labels[LABELEARLYREFLECTIONS]->setBounds(0, 180, 165, 20);
	sliders[ROOMSHAPE]->setBounds(5, 225, 155, 20);
	sliders[EARLYDECAY]->setBounds(5, 275, 155, 20);

	labels[LABELREVERB]->setBounds(165, 180, 165, 20);
	sliders[FILTERCF]->setBounds(175, 225, 155, 20);
	sliders[DIFFUSION]->setBounds(175, 275, 155, 20);

	labels[LABELOUTPUT]->setBounds(width-195, 10, 170, 20);
	sliders[LOWEQ]->setBounds(width-200, 55, 60, height-60);
	sliders[HIGHEQ]->setBounds(width-135, 55, 60, height-60);
	sliders[WETDRYMIX]->setBounds(width-70, 55, 62, height-60);
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

    // Update our sliders
    for(int i = 0; i < noParams; i++)
            sliders[i]->setValue (tempParamVals[i], dontSendNotification);
}