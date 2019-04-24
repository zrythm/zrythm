/*
	==============================================================================
	This file is part of Tal-Reverb by Patrick Kunz.

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


#if !defined(__Filter_h)
#define __Filter_h

#include "math.h"

class Filter
{
private:
	float f, k, p, scale, r, x;
	float y1, y2, y3, y4, oldx, oldy1, oldy2, oldy3;
	float fCutoff, fActualCutoff;
	float Pi;

	float sampleRateFactor;

public:

	Filter(float sampleRate)
	{
		Pi = 3.141592653f;
		y1 = y2 = y3 = y4 = oldx = oldy1 = oldy2 = oldy3 = 0.0f;

		if (sampleRate <= 0.0f) sampleRate = 44100.0f;

		sampleRateFactor= 44100.0f / sampleRate;
		if (sampleRateFactor > 1.0f)
			sampleRateFactor = 1.0f;

		fActualCutoff = -1.0f;
	}

	inline float process(float input, float fCutoff, float fRes, bool highPass)
	{
		if (fCutoff != fActualCutoff)
		{
			fActualCutoff = fCutoff;
			fCutoff *= sampleRateFactor;
			updateValues(fCutoff * fCutoff);
		}
		x = input; 

		// Four cascaded onepole filters (bilinear transform)
		y1 = (x + oldx) * p - k * y1;
		y2 = (y1 + oldy1) * p - k * y2;

		// Save values
		oldx = x;
		oldy1 = y1;
		if (highPass) input = input - y2;
		else input = y2;
		return input;
	}

	void updateValues(float fCutoff)
	{
		// Filter section MOOG VCF
		// CSound source code, Stilson/Smith CCRMA paper.
		f = fCutoff;										// [0 - 1]
		k = 3.6f * f - 1.6f * f * f - 1.0f;					// (Empirical tunning) /// !!! original (convex)

		p = (k + 1.0f) * 0.5f;							    // scale [0, 1] 
		scale = expf((1.0f - p) * 1.386249f);				// original
	}
};
#endif
