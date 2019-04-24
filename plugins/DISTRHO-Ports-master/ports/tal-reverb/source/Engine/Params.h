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


#ifndef Params_H
#define Params_H

/*-----------------------------------------------------------------------------

Kunz Patrick

Parameter table

-----------------------------------------------------------------------------*/

enum SYNTHPARAMETERS
{
  // Controllable values [0.0..1.0
  UNUSED= 0,
  DRY,
  WET,

  ROOMSIZE,
  PREDELAY,

  HIGHCUT,
  LOWCUT,
  DAMP,
  STEREO,

  // Number of controllable synth paramenters
  NUMPARAM,
  NUMPROGRAMS = 10,
};

class Params
{
  public:
  float *parameters;

  Params()  {
    parameters= new float[NUMPARAM];

    // Zero program values
    for(int j=0; j<NUMPARAM; j++) {
      parameters[j]= 0.0f;
    }
  }
};
#endif