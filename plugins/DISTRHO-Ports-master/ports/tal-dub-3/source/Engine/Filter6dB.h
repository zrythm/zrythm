/*
==============================================================================
This file is part of Tal-Dub-III by Patrick Kunz.

Copyright(c) 2005-2009 Patrick Kunz, TAL
Togu Audio Line, Inc.
http://kunz.corrupt.ch

This file may be licensed under the terms of of the
GNU General Public License Version 2 (the ``GPL'').

Software distributed under the License is distributed
on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
express or implied. See the GPL for the specific language
governing rights and limitations.

You should have received a copy of the GPL along with this
program. If not, go to http://www.gnu.org/licenses/gpl.html
or write to the Free Software Foundation, Inc.,  
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
==============================================================================
*/

#ifndef __Filter6dB_h_
#define __Filter6dB_h_

class Filter6dB 
{
private:
	float pi;
	float v2, iv2;
	float ay1, ay2, amf;
	float az1, az2;
	float at1;

	float kfc, kfcr, kacr, k2vg;

	// temporary variables
	float tmp, a, b;
	float cutoff, sampleRateFactor;

public:
	Filter6dB(float sampleRate) 
	{
		az1 = az2 = ay1 = ay2 = amf = 0.0f;
		at1 = 0.0f;

		pi = 3.1415926535f;
		v2 = 2.0f;   // twice the 'thermal voltage of a transistor'
		iv2 = 1.0f/v2;
		cutoff = 0.0f;

		setCoefficients(cutoff);

		sampleRateFactor = 44100.0f / sampleRate;
		if (sampleRateFactor > 1.0f) 
		{
			sampleRateFactor = 1.0f;
		}
	}

	void setCoefficients(float cutoffIn)
	{
		// Resonance [0..1]
		// Cutoff from 0 (0Hz) to 1 (nyquist)
		kfc  = cutoffIn * sampleRateFactor * 0.38f; // ~sr/2 + tanh approximation correction

		// Frequency & amplitude correction
		kfcr = 1.8730f * (kfc*kfc*kfc) + 0.4955f * (kfc * kfc) - 0.6490f * kfc + 0.9988f;
		kacr = -6.1f * (kfc * kfc) + 1.0f * kfc + 1.2f;

		//k2vg = v2*(1.0f - exp(-pi * kfcr * kfc)); // Original Filter Tuning
		tmp= -pi * kfcr * kfc; // Filter Tuning
		k2vg= v2*(1.0f-(1.0f+tmp+tmp*tmp*0.5f+tmp*tmp*tmp*0.16666667f+tmp*tmp*tmp*tmp*0.0416666667f+tmp*tmp*tmp*tmp*tmp*0.00833333333f));
	}

	inline void process(float *input) 
	{
		// Filter based on the text "Non linear digital implementation of the moog ladder filter" by Antti Houvilainen
		// Adopted from Csound code at http://www.kunstmusik.com/udo/cache/moogladder.udo

		float resonance = 0.0f; 

		float inWithRes = tanhApp( (*input - 7.1f * resonance * amf * kacr)*iv2 ); 

		ay1= az1 + k2vg * (inWithRes-at1);
		at1 = tanhApp(ay1*iv2);

		az1= ay1;

		// 1/2-sample delay for phase compensation
		amf = ay1 * 0.875f + az2 * 0.125f;
		az2 = amf;

		// Some second order distortion and corner feedback
		if (amf > 0.0f)
		{
			amf *= 0.999f;
		}

		*input = amf;
	}

	inline float tanhApp(float x) 
	{
		// Original
		//return tanh(x);
		return x;

		//// Approx
		//x *= 2.0f;
		//a = fabs(x);
		//b = 6.0f + a * (3.0f + a); // 6, 3
		//return (x * b) / (a * b + 12.0f);
	}

	inline float tanhClipper(float x) 
	{
		x *= 4.0f;
		a = fabs(x);
		b = 6.0f + a * (3.0f + a);
		return (x * b) / (a * b + 12.0f);
	}

};
#endif

