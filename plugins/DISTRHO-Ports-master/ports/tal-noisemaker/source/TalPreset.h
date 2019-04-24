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
    int   midiMap[256]; // 255 Midi Values to Map

    Array<SplinePoint*> splinePoints;
  
    TalPreset()
	{
		// Set values to 0
		for (int i = 0; i < NUMPARAM; i++) 
        {
			programData[i] = 0.0f;
		}

        // Init default values
        
        programData[VOLUME] = 0.5f;
        programData[FILTERTYPE] = 1.0f;
        programData[CUTOFF] = 1.0f;
        programData[OSC1VOLUME] = 0.8f;
        programData[OSC2VOLUME] = 0.0f;
        programData[OSC3VOLUME] = 0.8f;
        programData[OSC1WAVEFORM] = 1.0f;
        programData[OSC2WAVEFORM] = 1.0f;

        programData[OSCMASTERTUNE] = 0.5f;
        programData[OSC1TUNE] = 0.25f;
        programData[OSC2TUNE] = 0.5f;
        programData[OSC1FINETUNE] = 0.5f;
        programData[OSC2FINETUNE] = 0.5f;

        programData[FILTERCONTOUR] = 0.5f;
        programData[FILTERSUSTAIN] = 1.0f;

        programData[AMPSUSTAIN] = 1.0f;

        programData[VOICES] = 1.0f;
        programData[PORTAMENTOMODE] = 1.0f;

        programData[LFO1AMOUNT] = 0.5f;
        programData[LFO2AMOUNT] = 0.5f;

        programData[LFO1DESTINATION] = 1.0f;
        programData[LFO2DESTINATION] = 1.0f;

        programData[OSC1PW] = 0.5f;

        programData[OSC1PHASE] = 0.5f;
        programData[TRANSPOSE] = 0.5f;

        programData[FREEADDESTINATION] = 1.0f;

        programData[REVERBDECAY] = 0.5f;
        programData[REVERBLOWCUT] = 1.0f;
        programData[REVERBHIGHCUT] = 0.0f;
        programData[OSCBITCRUSHER] = 1.0f;
        programData[TAB1OPEN] = 1.0f;
        programData[TAB2OPEN] = 1.0f;
        programData[ENVELOPEEDITORDEST1] = 1.0f;
        programData[ENVELOPEEDITORSPEED] = 1.0f;

		// Init default midiMap
		for (int i = 0; i < 256; i++) 
		{
			midiMap[i] = 0;
		}
		name = "default";
	}

    ~TalPreset() 
    {
        splinePoints.clear();
    }

    void setPoints(Array<SplinePoint*> splinePoints)
    {
        this->splinePoints = splinePoints;
    }

    Array<SplinePoint*> getPoints()
    {
       return this->splinePoints;
    }
};
#endif
