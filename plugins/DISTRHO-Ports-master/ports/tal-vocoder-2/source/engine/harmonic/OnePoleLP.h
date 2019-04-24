/*
	==============================================================================
	This file is part of Tal-Vocoder by Patrick Kunz.

	Copyright(c) 2005-2010 Patrick Kunz, TAL
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

#if !defined(__OnePoleLP_h)
#define __OnePoleLP_h

class OnePoleLP 
{
 public:
  float inputs, outputs, lastOutput;


  OnePoleLP() 
  {
    lastOutput = inputs = outputs = 0.0f;
  }

  inline void tick(float *sample, float cutoff) 
  {
    float p = (cutoff*0.98f)*(cutoff*0.98f)*(cutoff*0.98f)*(cutoff*0.98f);
    outputs = (1.0f-p)*(*sample) + p*outputs;
    *sample= outputs;
  }
};

#endif
