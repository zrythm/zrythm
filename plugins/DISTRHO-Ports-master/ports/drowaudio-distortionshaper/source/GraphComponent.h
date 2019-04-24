/*
 ==============================================================================

 JUCE library : Starting point code, v1.26
 Copyright 2005 by Julian Storer. [edited by haydxn, 3rd April 2007]
 Modified 2008 by David Rowland

 This is a modified base application project that uses the recent Juce cocoa code
 It is split into separate cpp and h files and uses some preprocessor macros

 ==============================================================================
 */
#ifndef _GRAPHCOMPONENT_H_
#define _GRAPHCOMPONENT_H_

#include "includes.h"
#include "Parameters.h"
#include "CurvePoint.h"

class GraphComponent  : public Component,
                        public ComponentListener,
                        private Value::Listener
{
private:
	//==============================================================================
	OwnedArray <Value> values;
	OwnedArray <CurvePoint> curvePoints;

	float *buffer;
	int bufferSize;

public:
	//==============================================================================
	GraphComponent (float* buffer, int bufferSize);

	~GraphComponent () override;

	enum CurvePointList {
		pointX1,
		pointY1,
		pointX2,
		pointY2,
		noPoints,
	};

	//==============================================================================
	void resized () override;

	void paint (Graphics& g) override;

	//==============================================================================

	void componentMovedOrResized (Component &component, bool wasMoved, bool wasResized) override
	{
		if (&component == curvePoints[0]) {
			float x1 = (component.getX() + (0.5 * component.getWidth())) / (float)getWidth();
			float y1 = ((getHeight() - component.getY()) - (0.5 * component.getHeight())) / (float)getHeight();

			values[pointX1]->setValue(x1);
			values[pointY1]->setValue(y1);
		}
		else if (&component == curvePoints[1]) {
			float x2 = (component.getX() + (0.5 * component.getWidth())) / (float)getWidth();
			float y2 = ((getHeight() - component.getY()) - (0.5 * component.getHeight())) / (float)getHeight();

			values[pointX2]->setValue(x2);
			values[pointY2]->setValue(y2);
		}

		resized();
	}

	void valueChanged(Value& value) override
	{
		resized();
	}

	//==============================================================================
	void setValuesToReferTo(Value& x1, Value& y1, Value& x2, Value& y2);

	//==============================================================================
};

#endif//_GRAPHCOMPONENT_H_