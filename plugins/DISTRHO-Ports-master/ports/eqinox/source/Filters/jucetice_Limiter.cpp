/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2007 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  Paul Kellet
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

#include "jucetice_Limiter.h"


Limiter::Limiter ()
{
	fParam1 = 0.60f; //thresh
    fParam2 = 0.60f; //trim
    fParam3 = 0.15f; //attack
    fParam4 = 0.50f; //release
    fParam5 = 0.40f; //knee

    setParameter (0, fParam1);
    setParameter (1, fParam2);
    setParameter (2, fParam3);
    setParameter (3, fParam4);
    setParameter (4, fParam5);

    gain = 1.0f;
}

Limiter::~Limiter()
{
}

void Limiter::setParameter (int index, float value)
{
    switch(index)
    {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam4 = value; break;
    case 3: fParam3 = value; break;
    case 4: fParam5 = value; break;
    }

    //calcs here
    if (fParam5 > 0.5f) //soft knee
    {
        thresh = (float) pow (10.0, 1.0 - (2.0 * fParam1));
    }
    else //hard knee
    {
        thresh = (float) pow (10.0, (2.0 * fParam1) - 2.0);
    }
    trim = (float) pow (10.0, (2.0 * fParam2) - 1.0);
    att = (float) pow (10.0, -0.01 - 2.0 * fParam3);
    rel = (float) pow (10.0, -2.0 - (3.0 * fParam4));
}

float Limiter::getParameter (int index)
{
	float v = 0;

    switch(index)
    {
        case 0: v = fParam1; break;
        case 1: v = fParam2; break;
        case 2: v = fParam4; break;
        case 3: v = fParam3; break;
        case 4: v = fParam5; break;
    }
    return v;
}

/*
void Limiter::getParameterName (int index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "Thresh"); break;
    case 1: strcpy(label, "Output"); break;
    case 3: strcpy(label, "Attack"); break;
    case 2: strcpy(label, "Release"); break;
    case 4: strcpy(label, "Knee"); break;
  }
}

void long2string(long value, char *string) { sprintf(string, "%ld", value); }

void Limiter::getParameterDisplay (int index, char *text)
{
	switch(index)
  {
    case 0: long2string((long)(40.0*fParam1 - 40.0),text); break;
    case 1: long2string((long)(40.0*fParam2 - 20.0),text); break;
    case 3: long2string((long)(-301030.1 / (getSampleRate() * log10(1.0 - att))),text); break;
    case 2: long2string((long)(-301.0301 / (getSampleRate() * log10(1.0 - rel))),text); break;
    case 4: if(fParam5<0.5) strcpy(text, "HARD");
            else strcpy(text, "SOFT"); break;
  }
}

void Limiter::getParameterLabel (int index, char *label)
{
	switch(index)
  {
    case 0: strcpy(label, "dB"); break;
    case 1: strcpy(label, "dB"); break;
    case 3: strcpy(label, "µs"); break;
    case 2: strcpy(label, "ms"); break;
    case 4: strcpy(label, ""); break;
  }
}
*/

//--------------------------------------------------------------------------------
// process

void Limiter::out (AudioSampleBuffer& buffer, int sampleFrames)
{
	const float *in1 = buffer.getReadPointer (0);
	const float *in2 = buffer.getReadPointer (1);
	float *out1 = buffer.getWritePointer (0);
	float *out2 = buffer.getWritePointer (1);
	float g, at, re, tr, th, lev, ol, or_;

    th = thresh;
    g = gain;
    at = att;
    re = rel;
    tr = trim;

    --in1;
    --in2;
    --out1;
    --out2;

    if (fParam5 > 0.5) //soft knee
    {
	  while(--sampleFrames >= 0)
	  {
          ol = *++in1;
          or_ = *++in2;

          lev = (float)(1.0 / (1.0 + th * fabs(ol + or_)));
          if(g>lev) { g=g-at*(g-lev); } else { g=g+re*(lev-g); }

          *++out1 = (ol * tr * g);
		  *++out2 = (or_ * tr * g);
	  }
    }
    else
    {
        while(--sampleFrames >= 0)
        {
          ol = *++in1;
          or_ = *++in2;

          lev = (float)(0.5 * g * fabs(ol + or_));

          if (lev > th)
          {
            g = g - (at * (lev - th));
          }
          else //below threshold
          {
            g = g + (float)(re * (1.0 - g));
          }

          *++out1 = (ol * tr * g);
		  *++out2 = (or_ * tr * g);
        }
    }
    gain = g;
}

