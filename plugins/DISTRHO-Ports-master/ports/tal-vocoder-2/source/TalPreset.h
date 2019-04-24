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

#if !defined(__TalPreset_h)
#define __TalPreset_h

class TalPreset
{
  public:
    String name;
	float programData[NUMPARAM];
    int   midiMap[255]; // 255 Midi Values to Map
  
    TalPreset() : name(T("default"))
	{
		// Set values to 0
		for (int i = 0; i < NUMPARAM; i++) 
        {
			programData[i]=  0.0f;
		}

        programData[TAL_VOLUME] = 0.5f;
        programData[ENVELOPERELEASE] = 0.2f;
        programData[TAL_TUNE] = 0.5f;
        programData[SAWVOLUME] = 0.5f;
        programData[POLYMODE] = 1.0f;

        programData[ESSERINTENSITY] = 0.5f;
        programData[PULSETUNE] = 0.5f;
        programData[SAWTUNE] = 0.5f;
        programData[PULSEFINETUNE] = 0.5f;
        programData[SAWFINETUNE] = 0.5f;

        programData[VOCODERBAND00] = 0.5f;
        programData[VOCODERBAND01] = 0.5f;
        programData[VOCODERBAND02] = 0.5f;
        programData[VOCODERBAND03] = 0.5f;
        programData[VOCODERBAND04] = 0.5f;
        programData[VOCODERBAND05] = 0.5f;
        programData[VOCODERBAND06] = 0.5f;
        programData[VOCODERBAND07] = 0.5f;
        programData[VOCODERBAND08] = 0.5f;
        programData[VOCODERBAND09] = 0.5f;
        programData[VOCODERBAND10] = 0.5f;

		// Init default midiMap
		for (int i = 0; i < 255; i++) 
		{
			midiMap[i]=  0;
		}
	}

    ~TalPreset() {}
};
#endif
