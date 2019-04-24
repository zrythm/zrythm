/*
 *  GraphComponent.cpp
 *  haydxn_tutorial
 *
 *  Created by David Rowland on 27/12/2008.
 *  Copyright 2008 UWE. All rights reserved.
 *
 */

#include "GraphComponent.h"

//==============================================================================
/* This will calculate a quadratic bezier based on a co-ordinate pair and a given X
 */
//float quadraticBezier (float x, float a, float b)
//{
//	// adapted from BEZMATH.PS (1993)
//	// by Don Lancaster, SYNERGETICS Inc.
//	// http://www.tinaja.com/text/bezmath.html
//
//	float epsilon = 0.00001;
//	a = jmax(0.0f, jmin(1.0f, a));
//	b = jmax(0.0f, jmin(1.0f, b));
//	if (a == 0.5){
//		a += epsilon;
//	}
//
//	// solve t from x (an inverse operation)
//	float om2a = 1 - 2*a;
//	float t = (sqrt(a*a + om2a*x) - a)/om2a;
//	float y = (1-2*b)*(t*t) + (2*b)*t;
//	return y;
//}
//
///*	Cubic bezier
// */
//
//// Helper functions:
//float slopeFromT (float t, float A, float B, float C){
//	float dtdx = 1.0/(3.0*A*t*t + 2.0*B*t + C);
//	return dtdx;
//}
//
//float xFromT (float t, float A, float B, float C, float D){
//	float x = A*(t*t*t) + B*(t*t) + C*t + D;
//	return x;
//}
//
//float yFromT (float t, float E, float F, float G, float H){
//	float y = E*(t*t*t) + F*(t*t) + G*t + H;
//	return y;
//}
//
//float cubicBezier (float x, float a, float b, float c, float d)
//{
//
//	float y0a = 0.00; // initial y
//	float x0a = 0.00; // initial x
//	float y1a = b;    // 1st influence y
//	float x1a = a;    // 1st influence x
//	float y2a = d;    // 2nd influence y
//	float x2a = c;    // 2nd influence x
//	float y3a = 1.00; // final y
//	float x3a = 1.00; // final x
//
//	float A =   x3a - 3*x2a + 3*x1a - x0a;
//	float B = 3*x2a - 6*x1a + 3*x0a;
//	float C = 3*x1a - 3*x0a;
//	float D =   x0a;
//
//	float E =   y3a - 3*y2a + 3*y1a - y0a;
//	float F = 3*y2a - 6*y1a + 3*y0a;
//	float G = 3*y1a - 3*y0a;
//	float H =   y0a;
//
//	// Solve for t given x (using Newton-Raphelson), then solve for y given t.
//	// Assume for the first guess that t = x.
//	float currentt = x;
//	int nRefinementIterations = 5;
//	for (int i=0; i < nRefinementIterations; i++){
//		float currentx = xFromT (currentt, A,B,C,D);
//		float currentslope = slopeFromT (currentt, A,B,C);
//		currentt -= (currentx - x)*(currentslope);
//		currentt = jlimit(0.0f, 1.0f, currentt);
//	}
//
//	float y = yFromT (currentt,  E,F,G,H);
//	return y;
//}
//
///*	Cubic bezier through two points
// */
//// Helper functions.
//float B0 (float t){
//	return (1-t)*(1-t)*(1-t);
//}
//float B1 (float t){
//	return  3*t* (1-t)*(1-t);
//}
//float B2 (float t){
//	return 3*t*t* (1-t);
//}
//float B3 (float t){
//	return t*t*t;
//}
//float  findx (float t, float x0, float x1, float x2, float x3){
//	return x0*B0(t) + x1*B1(t) + x2*B2(t) + x3*B3(t);
//}
//float  findy (float t, float y0, float y1, float y2, float y3){
//	return y0*B0(t) + y1*B1(t) + y2*B2(t) + y3*B3(t);
//}
//
//float cubicBezierNearlyThroughTwoPoints(float x, float a, float b, float c, float d)
//{
//
//	float y = 0;
//	float epsilon = 0.00001;
//	float min_param_a = 0.0 + epsilon;
//	float max_param_a = 1.0 - epsilon;
//	float min_param_b = 0.0 + epsilon;
//	float max_param_b = 1.0 - epsilon;
//	a = jmax(min_param_a, jmin(max_param_a, a));
//	b = jmax(min_param_b, jmin(max_param_b, b));
//
//	float x0 = 0;
//	float y0 = 0;
//	float x4 = a;
//	float y4 = b;
//	float x5 = c;
//	float y5 = d;
//	float x3 = 1;
//	float y3 = 1;
//	float x1,y1,x2,y2; // to be solved.
//
//	// arbitrary but reasonable
//	// t-values for interior control points
//	float t1 = 0.3;
//	float t2 = 0.7;
//
//	float B0t1 = B0(t1);
//	float B1t1 = B1(t1);
//	float B2t1 = B2(t1);
//	float B3t1 = B3(t1);
//	float B0t2 = B0(t2);
//	float B1t2 = B1(t2);
//	float B2t2 = B2(t2);
//	float B3t2 = B3(t2);
//
//	float ccx = x4 - x0*B0t1 - x3*B3t1;
//	float ccy = y4 - y0*B0t1 - y3*B3t1;
//	float ffx = x5 - x0*B0t2 - x3*B3t2;
//	float ffy = y5 - y0*B0t2 - y3*B3t2;
//
//	x2 = (ccx - (ffx*B1t1)/B1t2) / (B2t1 - (B1t1*B2t2)/B1t2);
//	y2 = (ccy - (ffy*B1t1)/B1t2) / (B2t1 - (B1t1*B2t2)/B1t2);
//	x1 = (ccx - x2*B2t1) / B1t1;
//	y1 = (ccy - y2*B2t1) / B1t1;
//
//	x1 = jmax(0+epsilon, jmin(1-epsilon, x1));
//	x2 = jmax(0+epsilon, jmin(1-epsilon, x2));
//
//	// Note that this function also requires cubicBezier()!
//	y = cubicBezier (x, x1,y1, x2,y2);
//	y = jmax(0.0f, jmin(1.0f, y));
//	return y;
//}

//==============================================================================

//==============================================================================
GraphComponent::GraphComponent (float* buffer_, int bufferSize_)
:	buffer(buffer_),
	bufferSize(bufferSize_)
{
	for (int i = 0; i < noPoints; i++) {
		values.add(new Value);
		//values[i]->addListener(this);
	}

	curvePoints.add(new CurvePoint());
	addAndMakeVisible( curvePoints[0] );
	curvePoints[0]->addComponentListener(this);

	curvePoints.add(new CurvePoint());
	addAndMakeVisible( curvePoints[1] );
	curvePoints[1]->addComponentListener(this);

}

GraphComponent::~GraphComponent ()
{
	curvePoints.clear();
	values.clear();
	deleteAllChildren();
}

//==============================================================================
void GraphComponent::resized ()
{
	int width = getWidth();
	int height = getHeight();

	float x1 = values[pointX1]->getValue();
	float y1 = values[pointY1]->getValue();
	float x2 = values[pointX2]->getValue();
	float y2 = values[pointY2]->getValue();

	curvePoints[0]->setBounds((x1*width)-5, height - y1*height - 5, 10, 10);
	curvePoints[1]->setBounds((x2*width)-5, height - y2*height - 5, 10, 10);
	repaint();
}

void GraphComponent::paint (Graphics& g)
{
	// background
	g.setColour(Colours::black);
	g.fillAll();

	g.setColour(Colour::greyLevel(0.3));

	float intervalX = getWidth() / 10.0f;
	float intervalY = getHeight() / 10.0f;
	for (int i = 1; i < 10; i++)
	{
		g.drawVerticalLine((int)(i * intervalX), 0, getHeight());
		g.drawHorizontalLine((int)(i * intervalY), 0, getWidth());
	}

	g.setColour(Colour::greyLevel(0.5));
	g.drawLine(0, getHeight(), getWidth(), 0, 1);

	// cubic bezier
//	float scaleX = getWidth();
	float scaleY = getHeight();

	double scale = bufferSize / 50.0;
	double oneOverScale = 1.0 / scale;
	const double resolution = bufferSize * oneOverScale;
	const float stepX = getWidth() / resolution;
//	const float stepY = getHeight() / resolution;


	Path p3;
	p3.startNewSubPath(0, getHeight());
	for (int i = 1; i < (int)resolution; i++)
	{
		p3.lineTo(i*stepX, getHeight()-(buffer[(int)(i*scale)] * scaleY));
	}
//	p3.lineTo(getWidth(), 0);
	p3.lineTo(getWidth(), getHeight()-(buffer[bufferSize-1]*scaleY));

	g.setColour(Colours::white);
	g.strokePath(p3, PathStrokeType(2.0f));
}

//==============================================================================
void  GraphComponent::setValuesToReferTo(Value& x1, Value& y1, Value& x2, Value& y2)
{
	values[pointX1]->referTo(x1);
	values[pointY1]->referTo(y1);
	values[pointX2]->referTo(x2);
	values[pointY2]->referTo(y2);

        x1.addListener(this);
        y1.addListener(this);
        x2.addListener(this);
        y2.addListener(this);
}
