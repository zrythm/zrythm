/*
	==============================================================================
	This file is part of Tal-NoiseMaker by Patrick Kunz.

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

#if !defined(__DCBlock_h)
#define __DCBlock_h

class DCBlock 
{
 public:
  float inputs, outputs, lastOutput;

  DCBlock() 
  {
    lastOutput = inputs = outputs = 0.0f;
  }

  ~DCBlock()
  {
  }

  inline void tick(float *sample, float cutoff) 
  {
    outputs     = *sample-inputs+(0.999f-cutoff*0.4f)*outputs;
    inputs      = *sample;
    lastOutput  = outputs;
    *sample     = lastOutput;
  }
};

#endif
