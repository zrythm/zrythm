/*
 ==============================================================================

 JUCE library : Starting point code, v1.26
 Copyright 2005 by Julian Storer. [edited by haydxn, 3rd April 2007]
 Modified 2008 by David Rowland

 This is a modified base application project that uses the recent Juce cocoa code
 It is split into separate cpp and h files and uses some preprocessor macros

 ==============================================================================
 */
#ifndef _CURVEPOINT_H_
#define _CURVEPOINT_H_

#include "includes.h"

class CurvePoint  : public Component,
					public ComponentDragger
{
private:
	//==============================================================================
	ScopedPointer <ComponentBoundsConstrainer> constrainer;
	bool mouseIsOver;

public:
	//==============================================================================
	CurvePoint ()
	:	mouseIsOver(false)
	{
		constrainer = new ComponentBoundsConstrainer();
	}

	//==============================================================================
	void resized () override
	{
		int halfWidth = getWidth() * 0.5;
		int halfHeight = getHeight() * 0.5;
		constrainer->setMinimumOnscreenAmounts(halfHeight, halfWidth, halfHeight, halfWidth);
	}

	void paint (Graphics& g) override
	{
		g.setColour(Colours::grey);
		g.fillAll();

		if (mouseIsOver)
		{
			g.setColour(Colours::lightgrey);
			g.drawRect(0, 0, getWidth(), getHeight(), 2);
		}
	}

	void mouseDown (const MouseEvent& e) override
	{
		startDraggingComponent (this, e);
	}

	void mouseDrag (const MouseEvent& e) override
	{
		dragComponent (this, e, constrainer);
	}

	void mouseEnter (const MouseEvent& e) override
	{
		mouseIsOver = true;
		repaint();
	}

	void mouseExit (const MouseEvent& e) override
	{
		mouseIsOver = false;
		repaint();
	}

	//==============================================================================
};

#endif//_CURVEPOINT_H_