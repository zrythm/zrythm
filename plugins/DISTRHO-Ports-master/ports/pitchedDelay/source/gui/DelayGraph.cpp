/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  26 Aug 2012 12:42:01am

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "DelayGraph.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
DelayGraph::DelayGraph (OwnedArray<PitchedDelayTab>& tabs_, PitchedDelayAudioProcessor* ownerFilter)
    : tabs(tabs_),
      Proc(ownerFilter),
      dragging(false)
{

	//[UserPreSize]
	const int numDelays = Proc->getNumDelays();
	currentDelays.allocate(numDelays, true);
	currentVolumes.allocate(numDelays, true);
	currentPreVolumes.allocate(numDelays, true);
	currentEnabled.allocate(numDelays, true);
	currentFeedback.allocate(numDelays, true);

	//[/UserPreSize]

	setSize (600, 100);


	//[Constructor] You can add your own custom stuff here..
	startTimer(50);
	//[/Constructor]
}

DelayGraph::~DelayGraph()
{
	//[Destructor_pre]. You can add your own custom destruction code here..
	//[/Destructor_pre]



	//[Destructor]. You can add your own custom destruction code here..
	//[/Destructor]
}

//==============================================================================
void DelayGraph::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff505050));

    g.setColour (Colours::white);
    g.drawRect (0, 0, getWidth() - 0, getHeight() - 0, 1);

    //[UserPaint] Add your own custom painting code here..
	const float w = (float) getWidth();
	const float h = (float) getHeight();

	const double minDelay = 0;
	const double maxDelay = Proc->getDelay(0)->getCurrentDelayRange().getEnd();
	const int numDelays = Proc->getNumDelays();

	currentSelected = Proc->currentTab;

	g.setColour(Colour(0xFFD0D0D0));

	for (float t=(float) minDelay; t < maxDelay; t += 0.25f)
	{
		const int x = int(1 + (w-2) * (t - minDelay) / (maxDelay - minDelay));		
		float y =  3.f;

		if (float(int(t)) == t)
		{
			g.setFont(Font(10.f).boldened());
			y = 0;
		}
		else
		{
			g.setFont(Font(9.f));
		}

		g.drawVerticalLine(x, y, 12.f);
		g.drawText(String(t, 2), x+2, 0, 40, 12, Justification::bottomLeft, false);
	}
	g.drawHorizontalLine(12, 0.f, w);


	for (int i=0; i<numDelays; ++i)
	{
		DelayTabDsp* delay = Proc->getDelay(i);

		const bool enabled = delay->getParam(DelayTabDsp::kEnabled) > 0.5;
		double volume = delay->getParam(DelayTabDsp::kVolume);
		const double preVolume = delay->getParam(DelayTabDsp::kPreDelayVol);
		const double delayTime = delay->getParam(DelayTabDsp::kDelay);
		const double delayOffset = delay->getParam(DelayTabDsp::kPreDelay);
		const double feedback = delay->getParam(DelayTabDsp::kFeedback);

		currentEnabled[i] = enabled;
		currentVolumes[i] = volume;
		currentPreVolumes[i] = preVolume;
		currentDelays[i] = delayTime + delayOffset;
		currentFeedback[i] = feedback;

		const float xoffset = float(1 + (w-2) * (delayOffset - minDelay) / (maxDelay - minDelay));
		const float x = float(1 + (w-2) * (delayTime - minDelay) / (maxDelay - minDelay));
		const float y = float(h - (h-16) * (volume+60)/60);

		Colour c(currentSelected == i ? Colours::red : Colours::white);

		g.setColour(c.withAlpha(enabled ? 1.f : 0.3f));

		g.drawVerticalLine(int (x+xoffset), y, h-1);
		g.drawRect(x+xoffset-2.5f, y-2.5f, 5.f, 5.f, 1.f);

		if (currentSelected == i && delayOffset > 0)
		{
			const float xoffs = float(1 + (w-2) * (delayOffset - minDelay) / (maxDelay - minDelay));
			const float y = float(h - (h-16) * (preVolume+60)/60);
			
			Path p;
			p.startNewSubPath(xoffs, y);
			p.lineTo(xoffs, h-1.f);
			g.setColour(c.withAlpha(0.75f));
			PathStrokeType pst(0.5f);
			const float lengths[2] = {3.f, 3.f};
			const int numLengths = 2;

			pst.createDashedStroke(p, p, lengths, numLengths);

			g.strokePath(p, pst);
		}

		if (enabled)
		{
			const double volChange = Decibels::gainToDecibels(feedback / 100);

			g.setColour(c.withAlpha(currentSelected == i ? 0.3f: 0.1f));

			for (float fx = x*2; fx < w-1 && volume > -60; fx += x)
			{
				volume += volChange;
				const float y = float(h - (h-16) * (volume+60)/60);

				g.drawVerticalLine(int (fx+xoffset), y, h-1);
			}
		}
	}
    //[/UserPaint]
}

void DelayGraph::resized()
{
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void DelayGraph::mouseDown (const MouseEvent& e)
{
    //[UserCode_mouseDown] -- Add your code here...
	const int newIndex = getDelayUnderMouse(e.getMouseDownPosition());
		
	if (newIndex >= 0 && ! e.mods.isRightButtonDown())
	{
		Proc->currentTab = newIndex;
		dragging = true;
	}

    //[/UserCode_mouseDown]
}

void DelayGraph::mouseDrag (const MouseEvent& e)
{
    //[UserCode_mouseDrag] -- Add your code here...
	if (dragging)
	{
		const float w = (float) getWidth();
		const float h = (float) getHeight();

		const int mx = e.getPosition().getX();
		const int my = e.getPosition().getY();

		DelayTabDsp* delay = Proc->getDelay(Proc->currentTab);
		PitchedDelayTab* tab = tabs[Proc->currentTab];

		const double minDelay = 0;
		const double maxDelay = delay->getCurrentDelayRange().getEnd();

		const double delayTimeOffset = delay->getParam(DelayTabDsp::kPreDelay);
		const double delayTime = (mx - 1.f) / (w - 2.f) * (maxDelay - minDelay) + minDelay - delayTimeOffset;
		const double volume = jlimit(-60., 0., (h - my) / (h - 16.) * 60 - 60);


		if (tab != 0)
		{
			if (! tab->tbEnable->getToggleState())
				tab->tbEnable->setToggleState(true, sendNotification);

			tab->setDelaySeconds(delayTime, true);
			tab->sVolume->setValue(volume, sendNotification);			
		}

	}
    //[/UserCode_mouseDrag]
}

void DelayGraph::mouseUp (const MouseEvent& e)
{
    //[UserCode_mouseUp] -- Add your code here...
	dragging = false;

	if (e.mods.isRightButtonDown())
	{
		const int tabIndex = getDelayUnderMouse(e.getPosition());

		if (tabIndex >= 0)
		{
			Proc->currentTab = tabIndex;
			PitchedDelayTab* tab = tabs[Proc->currentTab];

			const int currentSync = tab->cbSync->getSelectedId();
			const int newSync = currentSync > tab->cbSync->getNumItems() - 2 ? 1 : currentSync + 1;

			tab->cbSync->setSelectedId(newSync, dontSendNotification);
		}
	}
	else if (e.getNumberOfClicks() > 1)
	{
		const int tabIndex = getDelayUnderMouse(e.getMouseDownPosition());
		if (tabIndex >= 0)
		{
			Proc->currentTab = tabIndex;
			PitchedDelayTab* tab = tabs[Proc->currentTab];

			tab->tbEnable->setToggleState(false, sendNotification);
		}

	}

    //[/UserCode_mouseUp]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void DelayGraph::timerCallback()
{
	if (currentSelected != Proc->currentTab)
	{
		repaint();
		return;
	}

	for (int i=0; i<Proc->getNumDelays(); ++i)
	{
		DelayTabDsp* delay = Proc->getDelay(i);

		const bool enabled = delay->getParam(DelayTabDsp::kEnabled) > 0.5;
		const double volume = delay->getParam(DelayTabDsp::kVolume);
		const double preVolume = delay->getParam(DelayTabDsp::kPreDelayVol);
		const double delayTimeOffset = delay->getParam(DelayTabDsp::kPreDelay);
		const double delayTime = delay->getParam(DelayTabDsp::kDelay) + delayTimeOffset;
		const double feedback = delay->getParam(DelayTabDsp::kFeedback);

		if (enabled != currentEnabled[i] || volume != currentVolumes[i] || preVolume != currentPreVolumes[i]|| delayTime != currentDelays[i] || currentFeedback[i] != feedback)
		{
			repaint();
			break;
		}
	}
}


void DelayGraph::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
	const int tabIndex = getDelayUnderMouse(e.getPosition(), 10);

	if (tabIndex >= 0)
	{
		Proc->currentTab = tabIndex;
		PitchedDelayTab* tab = tabs[Proc->currentTab];

		const double feedback = tab->sFeedback->getValue();

		tab->sFeedback->setValue(jlimit(0., 100., feedback + (wheel.deltaY > 0 ? 5 : -5)));
	}
}


int DelayGraph::getDelayUnderMouse(Point<int> mousePos, float minDistance)
{
	const float w = (float) getWidth();
	const float h = (float) getHeight();

	const double minDelay = 0;
	const double maxDelay = Proc->getDelay(0)->getCurrentDelayRange().getEnd();

	const int numDelays = Proc->getNumDelays();
	//const int mx = e.getMouseDownX();
	
	int newIndex = -1;
	float newDistancs = minDistance;

	for (int i=0; i<numDelays; ++i)
	{
		DelayTabDsp* delay = Proc->getDelay(i);
		const double delayTimeOffset = delay->getParam(DelayTabDsp::kPreDelay);
		const double delayTime = delay->getParam(DelayTabDsp::kDelay) + delayTimeOffset;
		double volume = delay->getParam(DelayTabDsp::kVolume);

		const float x = float(1 + (w-2) * (delayTime - minDelay) / (maxDelay - minDelay));
		const float y = float(h - (h-16) * (volume+60)/60);
		
		const float d = Point<float>(x, y).getDistanceFrom(mousePos.toFloat());

		if (d < newDistancs)
		{
			newIndex = i;
			newDistancs = d;
		}		
	}

	return newIndex;
}


//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

	This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="DelayGraph" componentName=""
                 parentClasses="public Component, public Timer" constructorParams="OwnedArray&lt;PitchedDelayTab&gt;&amp; tabs_, PitchedDelayAudioProcessor* ownerFilter"
                 variableInitialisers="tabs(tabs_),&#10;Proc(ownerFilter),&#10;dragging(false)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330000013"
                 fixedSize="1" initialWidth="600" initialHeight="100">
  <METHODS>
    <METHOD name="mouseDown (const MouseEvent&amp; e)"/>
    <METHOD name="mouseDrag (const MouseEvent&amp; e)"/>
    <METHOD name="mouseUp (const MouseEvent&amp; e)"/>
  </METHODS>
  <BACKGROUND backgroundColour="ff505050">
    <RECT pos="0 0 0M 0M" fill="solid: ffffff" hasStroke="1" stroke="1, mitered, butt"
          strokeColour="solid: ffffffff"/>
  </BACKGROUND>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
