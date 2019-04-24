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

#if !defined(__HighPass_h)
#define __HighPass_h

class HighPass 
{
 public:
  float inputs, outputs, lastOutput;

  float cutoff;

  HighPass() 
  {
    cutoff = 1.0f;
    lastOutput = inputs = outputs = 0.0f;
  }

  ~HighPass()
  {
  }

  inline void setCutoff(float value)
  {
      this->cutoff = 0.999f-value * 0.4f;
  }

  inline void tick(float *sample) 
  {
    outputs     = *sample-inputs+cutoff*outputs;
    inputs      = *sample;
    lastOutput  = outputs;
    *sample     = lastOutput;
  }
};

#endif
