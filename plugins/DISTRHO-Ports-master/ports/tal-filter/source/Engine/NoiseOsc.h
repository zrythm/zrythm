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

#ifndef NoiseOsc_H
#define NoiseOsc_H

#include "math.h"

#include <cstdlib>

class NoiseOsc
{
  public:
  float randomValue;

  NoiseOsc(float sampleRate) 
  {
    resetOsc();
  }

  void resetOsc() 
  {
    randomValue= 0;
  }

  inline float getNextSample() 
  {
    randomValue= (randomValue*2.0f+(float)rand()/RAND_MAX)*0.33333333333333333333f;
    return (randomValue-0.5f)*2.0f;
  }
};
#endif