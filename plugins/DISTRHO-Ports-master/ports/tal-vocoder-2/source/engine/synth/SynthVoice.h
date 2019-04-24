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

#if !defined(__SynthVoice_h)
#define __SynthVoice_h

#include "math.h"
#include "vco.h"
#include "Adsr.h"
#include "Portamento.h"
#include "audioutils.h"
#include "OscNoise.h"

class SynthVoice
{
private:
	bool isNoteOn;

	float resonance;
	float keyfollow;
	float filterContour;

    float velocity;

    float mastertune;
    float transpose;
    float detune;

    float noiseVolume;

	Vco *vco;
	Adsr *ampAdsr;

    OscNoise* oscNoise;

	Portamento *portamento;
	int portamentoMode;
	float portamentoValue;

	AudioUtils audioUtils;

public:
	int noteNumber;

	SynthVoice(float sampleRate)
	{
		initialize(sampleRate);
	}

	~SynthVoice()
	{
		delete this->ampAdsr;
		delete this->portamento;
		delete this->vco;
        delete this->oscNoise;
	}

private:
	void initialize(float sampleRate)
	{
		this->isNoteOn = false;
		this->noteNumber = 60;

		this->portamentoMode = 0;
		this->portamentoValue = 0.0f;

        this->mastertune = 0.0f;
        this->transpose = 0.0f;

        this->noiseVolume = 0.0f; 

		this->vco = new Vco(sampleRate);
        this->oscNoise = new OscNoise(sampleRate);
		this->ampAdsr = new Adsr(sampleRate);
		this->portamento = new Portamento(sampleRate);
	}

	inline void processAmp(float *sample)
	{ 
		*sample *= this->ampAdsr->tick(isNoteOn, false);
	}

public:

	bool isNotePlaying()
	{
		return this->ampAdsr->isNotePlaying(isNoteOn);
	}

	void setNoteOn(int note, bool slide, float velocity)
	{
		switch (portamentoMode)
		{
		case 1:
			this->ampAdsr->resetState();
			this->portamento->setUpNote((float)note);
			break;
		case 2:
			if (!isNotePlaying()) this->portamento->setUpNote((float)note);
            if (!isNoteOn) this->portamento->setUpNote((float)note);
		case 3:
			if (!isNoteOn)
			{
				this->ampAdsr->resetState();
			}
			break;
		}

		// FIXME: Maybe not required
		if (!this->isNotePlaying())
		{
			this->vco->resetVco();
			this->ampAdsr->resetAll();
		}

		this->isNoteOn = true;
		this->noteNumber = note;
	}

	Vco* getVco()
	{
		return this->vco;
	}

	void setNoteOff(int note)
	{
		this->isNoteOn = false;
	}

	bool getIsNoteOn()
	{
		return isNoteOn;
	}

	void setPortamentoMode(float value)
	{
		this->portamentoMode = (int)value;
	}

	void setPortamento(float value)
	{
		this->portamentoValue = value;
	}

	void setMastertune(float value)
	{
        this->mastertune = audioUtils.getOscFineTuneValue(value);
	}

	void setTranspose(float value)
	{
        this->transpose = value;
	}

    void setOsc1Volume(float value)
    {
        this->vco->setOsc1Volume(value);
    }

    void setNoiseVolume(float value)
    {
        this->noiseVolume = value; 
    }

    void setOsc2Volume(float value)
    {
        this->vco->setOsc2Volume(value);
    }

    void setSubOscVolume(float value)
    {
        this->vco->setOsc3Volume(value);
    }

	inline bool process(float* sampleL, float* sampleR, float cutoff)
	{
		float sample = 0.0f;
		if (this->isNotePlaying())
		{
			float masterNote = this->portamento->tick((float)noteNumber, portamentoValue, portamentoMode > 0.5f);
            //masterNote += this->pitchwheelHandler->getPitch() + this->mastertune + this->transpose;
            masterNote += this->mastertune + this->transpose;
            //masterNote *= (this->detuneFactor * this->detune + 1.0f);
            this->vco->process(&sample, masterNote);

            if (this->noiseVolume > 0.0f)
            {
                sample += this->oscNoise->getNextSample() * this->noiseVolume;
            }

			*sampleL += sample;
			*sampleR += sample;
			return true;
		}

		return false;
	}
};
#endif
