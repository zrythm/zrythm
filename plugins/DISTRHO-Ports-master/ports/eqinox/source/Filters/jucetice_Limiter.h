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

#ifndef __JUCETICE_LIMITER_HEADER__
#define __JUCETICE_LIMITER_HEADER__

#include "../StandardHeader.h"


class Limiter
{
public:

	Limiter ();
	~Limiter ();

	void out (AudioSampleBuffer& buffer, int sampleFrames);

	void setParameter (int index, float value);
	float getParameter (int index);

/*
	void getParameterLabel (int index, char *label);
	void getParameterDisplay (int index, char *text);
	void getParameterName (int index, char *text);
*/

protected:
	float fParam1;
    float fParam2;
    float fParam3;
    float fParam4;
    float fParam5;
    float thresh, gain, att, rel, trim;
};

#endif
