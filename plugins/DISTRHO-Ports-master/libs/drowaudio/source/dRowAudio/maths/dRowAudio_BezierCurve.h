/*
  ==============================================================================
  
  This file is part of the dRowAudio JUCE module
  Copyright 2004-12 by dRowAudio.
  
  ------------------------------------------------------------------------------
 
  dRowAudio can be redistributed and/or modified under the terms of the GNU General
  Public License (Version 2), as published by the Free Software Foundation.
  A copy of the license is included in the module distribution, or can be found
  online at www.gnu.org/licenses.
  
  dRowAudio is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  
  ==============================================================================

  The following code is adapted from:
  Golan Levin and Colaborators available from:
  http://www.flong.com/texts/code/shapers_bez/
  initialy adapted from BEZMATH.PS (1993)
  by Don Lancaster, SYNERGETICS Inc. 
  http://www.tinaja.com/text/bezmath.html
 */

#ifndef __DROWAUDIO_BEZIERCURVE_H__
#define __DROWAUDIO_BEZIERCURVE_H__

//==============================================================================
/**	This namespace contains some very useful Bezier calculations.
 */
namespace BezierCurve
{
	//==============================================================================
	//	Helper functions:
	//==============================================================================
	/*	Cubic bezier
	 */
	inline float slopeFromT (float t, float A, float B, float C)
    {
		float dtdx = 1.0f / (3.0f*A*t*t + 2.0f*B*t + C); 
		return dtdx;
	}
	
	inline float xFromT (float t, float A, float B, float C, float D)
    {
		float x = A*(t*t*t) + B*(t*t) + C*t + D;
		return x;
	}
	
	inline float yFromT (float t, float E, float F, float G, float H)
    {
		float y = E*(t*t*t) + F*(t*t) + G*t + H;
		return y;
	}
	
	//==============================================================================
	/*	Cubic bezier through two points
	 */
	inline float B0 (float t){
		return (1-t)*(1-t)*(1-t);
	}
	inline float B1 (float t){
		return  3*t* (1-t)*(1-t);
	}
	inline float B2 (float t){
		return 3*t*t* (1-t);
	}
	inline float B3 (float t){
		return t*t*t;
	}
	inline float  findx (float t, float x0, float x1, float x2, float x3){
		return x0*B0(t) + x1*B1(t) + x2*B2(t) + x3*B3(t);
	}
	inline static float  findy (float t, float y0, float y1, float y2, float y3){
		return y0*B0(t) + y1*B1(t) + y2*B2(t) + y3*B3(t);
	}
	
	//==============================================================================
	//	Bezier calculations
	//==============================================================================
	/**	Quadratic Bezier.
		This will calculate the quadratic Bezier of a given input x based on a co-ordinate pair (a, b).
		All points have to be within a unit square ie. 0 < x < 1
	 */
	inline static float quadraticBezier (float x, float a, float b)
	{
		// adapted from BEZMATH.PS (1993)
		// by Don Lancaster, SYNERGETICS Inc. 
		// http://www.tinaja.com/text/bezmath.html
		
		float epsilon = 0.00001f;
		a = jmax (0.0f, jmin (1.0f, a)); 
		b = jmax (0.0f, jmin (1.0f, b)); 
		if (a == 0.5){
			a += epsilon;
		}
		
		// solve t from x (an inverse operation)
		float om2a = 1 - 2*a;
		float t = ((float) sqrt (a*a + om2a*x) - a)/om2a;
		float y = (1-2*b)*(t*t) + (2*b)*t;
		return y;
	}

	//==============================================================================
	/**	Cubic Bezier.
		This will calculate the cubic Bezier of a given input x based on two co-ordinate pairs (a, b) & (c, d).
		All points have to be within a unit square ie. 0 < x < 1.
	 */
	inline static float cubicBezier (float x, float a, float b, float c, float d)
	{
		
		float y0a = 0.00f; // initial y
		float x0a = 0.00f; // initial x 
		float y1a = b;     // 1st influence y   
		float x1a = a;     // 1st influence x 
		float y2a = d;     // 2nd influence y
		float x2a = c;     // 2nd influence x
		float y3a = 1.00f; // final y 
		float x3a = 1.00f; // final x 
		
		float A =   x3a - 3*x2a + 3*x1a - x0a;
		float B = 3*x2a - 6*x1a + 3*x0a;
		float C = 3*x1a - 3*x0a;   
		float D =   x0a;
		
		float E =   y3a - 3*y2a + 3*y1a - y0a;    
		float F = 3*y2a - 6*y1a + 3*y0a;             
		float G = 3*y1a - 3*y0a;             
		float H =   y0a;
		
		// Solve for t given x (using Newton-Raphelson), then solve for y given t.
		// Assume for the first guess that t = x.
		float currentt = x;
		int nRefinementIterations = 5;
		for (int i=0; i < nRefinementIterations; i++)
        {
			float currentx = xFromT (currentt, A,B,C,D); 
			float currentslope = slopeFromT (currentt, A,B,C);
			currentt -= (currentx - x) * (currentslope);
			currentt = jlimit (0.0f, 1.0f, currentt);
		} 
		
		float y = yFromT (currentt,  E,F,G,H);
		return y;
	}
	
	//==============================================================================
	/**	Cubic Bezier Nearly Through Two Points.
		This will calculate the cubic Bezier of a given input x based on two co-ordinate pairs (a, b) & (c, d).
		The Bezier curve calculated will try to go through both the given points within reason.
		All points have to be within a unit square ie. 0 < x < 1
	 */
	inline static float cubicBezierNearlyThroughTwoPoints (float x, float a, float b, float c, float d)
	{
		
		float y = 0;
		float epsilon = 0.00001f;
		float min_param_a = 0.0f + epsilon;
		float max_param_a = 1.0f - epsilon;
		float min_param_b = 0.0f + epsilon;
		float max_param_b = 1.0f - epsilon;
		a = jmax (min_param_a, jmin (max_param_a, a));
		b = jmax (min_param_b, jmin (max_param_b, b));
		
		float x0 = 0;  
		float y0 = 0;
		float x4 = a;  
		float y4 = b;
		float x5 = c;  
		float y5 = d;
		float x3 = 1;  
		float y3 = 1;
		float x1 , y1, x2, y2; // to be solved.
		
		// arbitrary but reasonable 
		// t-values for interior control points
		float t1 = 0.3f;
		float t2 = 0.7f;
		
		float B0t1 = B0(t1);
		float B1t1 = B1(t1);
		float B2t1 = B2(t1);
		float B3t1 = B3(t1);
		float B0t2 = B0(t2);
		float B1t2 = B1(t2);
		float B2t2 = B2(t2);
		float B3t2 = B3(t2);
		
		float ccx = x4 - x0*B0t1 - x3*B3t1;
		float ccy = y4 - y0*B0t1 - y3*B3t1;
		float ffx = x5 - x0*B0t2 - x3*B3t2;
		float ffy = y5 - y0*B0t2 - y3*B3t2;
		
		x2 = (ccx - (ffx*B1t1)/B1t2) / (B2t1 - (B1t1*B2t2)/B1t2);
		y2 = (ccy - (ffy*B1t1)/B1t2) / (B2t1 - (B1t1*B2t2)/B1t2);
		x1 = (ccx - x2*B2t1) / B1t1;
		y1 = (ccy - y2*B2t1) / B1t1;
		
		x1 = jmax (0+epsilon, jmin (1-epsilon, x1));
		x2 = jmax (0+epsilon, jmin (1-epsilon, x2));
		
		// Note that this function also requires cubicBezier()!
		y = cubicBezier (x, x1, y1, x2,y2);
		y = jmax (0.0f, jmin (1.0f, y));
		return y;
	}
		
	//==============================================================================
}

#endif //__DROWAUDIO_BEZIERCURVE_H__