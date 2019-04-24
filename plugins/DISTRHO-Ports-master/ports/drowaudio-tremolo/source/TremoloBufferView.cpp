/*
 *  TremoloBufferView.cpp
 *  haydxn_tutorial
 *
 *  Created by David Rowland on 27/12/2008.
 *  Copyright 2008 UWE. All rights reserved.
 *
 */

#include "TremoloBufferView.h"

#define pi 3.14159265358979323846264338327950288

//==============================================================================
TremoloBufferView::TremoloBufferView (float* buffer, int size)
	:	tremoloBufferSize(size),
		tremoloBuffer(buffer)
		
{
//	tremoloBufferSize = size;
//	tremoloBuffer = buffer;
}

TremoloBufferView::~TremoloBufferView ()
{
	deleteAllChildren();
}

//==============================================================================
void TremoloBufferView::resized ()
{
    refreshBuffer();
}

void TremoloBufferView::paint (Graphics& g)
{
	g.fillAll (Colour::greyLevel(0.4f));
	g.setColour (Colour (0xFF455769).contrasting (0.6));
	g.fillPath (displayPath);
	g.setColour (Colour (0xFF455769).contrasting (0.3).withAlpha (0.8f));
	g.strokePath (displayPath, PathStrokeType (2.0f));
}

void TremoloBufferView::refreshBuffer()
{
	const int w = getWidth();
	const int h = getHeight();
	
	// recalculate display scale
	float displayScaleX = (tremoloBufferSize + 1) / (w - 4.0f);
	float displayScaleY = (h - 4.0f);
	
	displayPath.clear();	
    displayPath.startNewSubPath (2.0f, h - (displayScaleY * tremoloBuffer[0]) - 2);
	for (int i = 25; i < tremoloBufferSize; i += 25)
	{
		displayPath.lineTo ((i / displayScaleX) + 2, h - (displayScaleY * tremoloBuffer[i]) - 2);
	}
	displayPath.lineTo (w - 2, h - (displayScaleY * tremoloBuffer[0]) - 2);
	displayPath.lineTo (w - 2, h);
	displayPath.lineTo (2.0f, h);
	displayPath.closeSubPath();
	
	repaint();
}

//==============================================================================