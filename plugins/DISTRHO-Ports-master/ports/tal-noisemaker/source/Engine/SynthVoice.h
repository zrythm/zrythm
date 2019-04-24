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
#include "Vco.h"
#include "FilterHandler.h"
#include "Adsr.h"
#include "AdsrHandler.h"
#include "Portamento.h"
#include "LfoHandler1.h"
#include "LfoHandler2.h"
#include "PitchwheelHandler.h"
#include "VelocityHandler.h"
#include "AudioUtils.h"
#include "HighPass.h"
#include "OscNoise.h"
#include "../EnvelopeEditor/EnvelopeEditorVoiceHandler.h"
#include "../EnvelopeEditor/EnvelopeEditorHandler.h"

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
    float detuneFactor;
    float detune;
    float vintageNoise;

	Vco *vco;
	FilterHandler *filterHandler;
	Adsr *filterAdsr;
	Adsr *ampAdsr;
	AdsrHandler *freeAdsr;
	Portamento *portamento;
	OscNoise *oscNoise;

	LfoHandler1 *lfoHandler1;
	LfoHandler2 *lfoHandler2;
    VelocityHandler *velocityHandler;
    PitchwheelHandler *pitchwheelHandler;
    EnvelopeEditorHandler *envelopeEditorHandler;
    EnvelopeEditorVoiceHandler *envelopeEditorVoiceHandler;

	int countPostFilter;
	int countSilentFilter;

	int portamentoMode;
	float portamentoValue;

	AudioUtils audioUtils;

public:
	int noteNumber;

	SynthVoice(
        float sampleRate, 
        LfoHandler1 *lfoHandler1, 
        LfoHandler2 *lfoHandler2,
        VelocityHandler *velocityHandler, 
        PitchwheelHandler *pitchwheelHandler,
        EnvelopeEditorHandler *envelopeEditorHandler)
	{
		this->lfoHandler1 = lfoHandler1;
		this->lfoHandler2 = lfoHandler2;
        this->velocityHandler = velocityHandler;
        this->pitchwheelHandler = pitchwheelHandler;
        this->envelopeEditorHandler = envelopeEditorHandler;

        this->envelopeEditorVoiceHandler = new EnvelopeEditorVoiceHandler(envelopeEditorHandler);
		initialize(sampleRate);
	}

	~SynthVoice()
	{
		delete this->filterHandler;
		delete this->filterAdsr;
		delete this->ampAdsr;
		delete this->freeAdsr;
		delete this->portamento;
        delete this->oscNoise;
        delete this->envelopeEditorVoiceHandler;
		delete this->vco;
	}

private:
	void initialize(float sampleRate)
	{
		this->isNoteOn = false;
		this->noteNumber = 60;
		this->resonance = 0.0f;
		this->keyfollow = 0.0f;

		this->filterContour = 0;

		this->countPostFilter = 0;
		this->countSilentFilter = 0;

		this->portamentoMode = 0;
		this->portamentoValue = 0.0f;

        this->velocity = 1.0f;

        this->mastertune = 0.0f;
        this->transpose = 0.0f;

        this->detuneFactor = 1.0f;
        this->detune = 0.0f;
        this->vintageNoise = 0.0f;

        this->oscNoise = new OscNoise((float)sampleRate);

		this->freeAdsr = new AdsrHandler((float)sampleRate);
		this->freeAdsr->setSustain(0.0f);
		this->freeAdsr->setRelease(1.0f);

		this->vco = new Vco(sampleRate, this->lfoHandler1, this->lfoHandler2, this->freeAdsr, this->envelopeEditorHandler, this->envelopeEditorVoiceHandler);

		this->filterHandler = new FilterHandler(sampleRate);
		this->filterAdsr = new Adsr(sampleRate);
		this->ampAdsr = new Adsr(sampleRate);

		this->portamento = new Portamento(sampleRate);
	}

	inline void processFilter(float *sample, float cutoff)
	{	
        cutoff += this->velocityHandler->getCutoff() * this->velocity + this->pitchwheelHandler->getCutoff();
		cutoff += this->keyfollow * (((float)this->noteNumber - 72.0f) / 512.0f);
        cutoff += this->lfoHandler1->getFilter() + this->lfoHandler2->getFilter();
        cutoff += this->envelopeEditorHandler->getFilterValue(this->envelopeEditorVoiceHandler->getValue());

        this->filterAdsr->tick(isNoteOn);
        float contourAdsr = this->filterAdsr->getValueFasterAttack();
        float contourAmount = filterContour + this->velocityHandler->getContour() * velocity;
		cutoff += contourAmount * contourAdsr + this->freeAdsr->getFilter();
        cutoff = cutoff * cutoff;

		if (cutoff > 1.0f) cutoff = 1.0f;
		if (cutoff < 0.0f) cutoff = 0.0f;
        this->filterHandler->process(sample, cutoff, this->resonance);
	}

	inline void processAmp(float *sample)
	{ 
		*sample *= this->ampAdsr->tick(isNoteOn);
	}

	inline void processFreeEnvelope()
	{
		this->freeAdsr->process(isNoteOn);
	}

    inline void calcRandomDetuneFactor()
    {
        this->detuneFactor = ((((float)rand()/(float)RAND_MAX) - 0.5f) * 0.005f);
    }

	inline void prepareFilterForNextNote(const float cutoff)
	{
        if (countPostFilter == 0)
        {
            this->filterHandler->reset();
        }
        
        // Fast filter prepare for next note
		if (countPostFilter++ < 2000)
		{
		    float silentSample = 0.0f;
			this->filterAdsr->resetState();
			this->freeAdsr->resetState();
			this->lfoHandler1->triggerPhase();
			this->lfoHandler2->triggerPhase();
            processFilter(&silentSample, cutoff + 0.1f);
		}
	}

public:
    void reset()
    {
        countPostFilter = 0;
    }

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
			this->filterAdsr->resetAll();
			this->portamento->setUpNote((float)note);
			this->freeAdsr->resetAll();
			this->lfoHandler1->triggerPhase();
			this->lfoHandler2->triggerPhase();
            this->velocity = velocity;
            this->envelopeEditorVoiceHandler->reset();
			break;
		case 2:
			if (!isNotePlaying()) this->portamento->setUpNote((float)note);
            if (!isNoteOn) this->portamento->setUpNote((float)note);
		case 3:
			if (!isNoteOn)
			{
				this->filterAdsr->resetState();
				this->ampAdsr->resetState();
				this->freeAdsr->resetState();
				this->lfoHandler1->triggerPhase();
				this->lfoHandler2->triggerPhase();
                this->velocity = velocity;
                this->envelopeEditorVoiceHandler->reset();
			}
			break;
		}

		// FIXME: Maybe not required
		if (!this->isNotePlaying())
		{
			this->vco->resetVco();
			this->ampAdsr->resetAll();
			this->filterAdsr->resetAll();
			this->freeAdsr->resetAll();
			this->lfoHandler1->triggerPhase();
			this->lfoHandler2->triggerPhase();
            this->velocity = velocity;
            this->envelopeEditorVoiceHandler->reset();
		}

		this->isNoteOn = true;
		this->noteNumber = note;
		this->countPostFilter = 0;

        this->calcRandomDetuneFactor();
	}

	void setNoteOff(int note)
	{
		this->isNoteOn = false;
	}

	bool getIsNoteOn()
	{
		return isNoteOn;
	}

	void setResonance(float value)
	{
		this->resonance = value;
	}

	void setFilterAttack(float value)
	{
		this->filterAdsr->setAttack(value);
	}

	void setFilterDecay(float value)
	{
		this->filterAdsr->setDecay(value);
	}

	void setFilterSustain(float value)
	{
		this->filterAdsr->setSustain(value);
	}

	void setFilterRelease(float value)
	{
		this->filterAdsr->setRelease(value);
	}

	void setAmpAttack(float value)
	{
		this->ampAdsr->setAttack(value);
	}

	void setAmpDecay(float value)
	{
		this->ampAdsr->setDecay(value);
	}

	void setAmpSustain(float value)
	{
		this->ampAdsr->setSustain(value);
	}

	void setAmpRelease(float value)
	{
		this->ampAdsr->setRelease(value);
	}  

	void setKeyfollow(float value)
	{
		this->keyfollow = value * value;
	}

	void setFilterContour(float value)
	{
		this->filterContour = value;
	}

	void setOsc1Volume(float value)
	{
		vco->setOsc1Volume(value);
	}

	void setOsc2Volume(float value)
	{
		vco->setOsc2Volume(value);
	}

	void setOsc3Volume(float value)
	{
		vco->setOsc3Volume(value);
	}

	void setOsc1Waveform(Osc::Waveform waveform)
	{
		vco->setOsc1Waveform(waveform);
	}

	void setOsc2Waveform(Osc::Waveform waveform)
	{
		vco->setOsc2Waveform(waveform);
	}

	void setOsc1Tune(float value)
	{
		vco->setOsc1Tune(value);
	}

	void setOsc2Tune(float value)
	{
		vco->setOsc2Tune(value);
	}

	void setOsc1FineTune(float value)
	{
		vco->setOsc1FineTune(value);
	}

	void setOsc2FineTune(float value)
	{
		vco->setOsc2FineTune(value);
	}

	void setOscSync(bool value)
	{
		vco->setOscSync(value);
	}

	void setPortamentoMode(float value)
	{
		this->portamentoMode = (int)value;
	}

	void setPortamento(float value)
	{
		this->portamentoValue = value;
	}

	void setOsc1Pw(float value)
	{
		this->vco->setOsc1Pw(value);
	}

	void setOsc1Phase(float value)
	{
		this->vco->setOsc1Phase(value);
	}

	void setOsc1Fm(float value)
	{
		this->vco->setOsc1Fm(audioUtils.getLogScaledValue(value, 1.0f));
	}

	void setOsc2Phase(float value)
	{
		this->vco->setOsc2Phase(value);
	}

	void setFreeAdAttack(float value)
	{
		this->freeAdsr->setAttack(value);
	}

	void setFreeAdDecay(float value)
	{
		this->freeAdsr->setDecay(value);
	}

	void setFreeAdAmount(float value)
	{
		this->freeAdsr->setAmount(value);
	}

	void setLfo1Sync(float value, float rate, float bpm)
	{
		this->lfoHandler1->setSync(value > 0.0f, rate, bpm);
	}

	void setLfo1KeyTrigger(float value)
	{
		this->lfoHandler1->setKeyTrigger(value > 0.0f);
	}

	void setLfo2Sync(float value, float rate, float bpm)
	{
		this->lfoHandler2->setSync(value > 0.0f, rate, bpm);
	}

	void setLfo2KeyTrigger(float value)
	{
		this->lfoHandler2->setKeyTrigger(value > 0.0f);
	}

	void setFreeAdDestination(int value)
	{
		switch (value)
		{
		case AdsrHandler::OFF:
			this->freeAdsr->setDestination(AdsrHandler::OFF);
			break;
		case AdsrHandler::FILTER:
			this->freeAdsr->setDestination(AdsrHandler::FILTER);
			break;
		case AdsrHandler::OSC1:
			this->freeAdsr->setDestination(AdsrHandler::OSC1);
			break;
		case AdsrHandler::OSC2:
			this->freeAdsr->setDestination(AdsrHandler::OSC2);
			break;
		case AdsrHandler::PW:
			this->freeAdsr->setDestination(AdsrHandler::PW);
			break;
		case AdsrHandler::FM:
			this->freeAdsr->setDestination(AdsrHandler::FM);
			break;
		}
	}

	void setDetune(float value)
	{
        this->detune = value;
	}

	void setVintageNoise(float value)
	{
        this->vintageNoise = value;
	}

	void setMastertune(float value)
	{
        this->mastertune = value;
	}

	void setTranspose(float value)
	{
        this->transpose = value;
	}

    void setRingmodulation(float value)
    {
        this->vco->setRingmodulation(value);
    }

    void setOscBitcrusher(float value)
    {
        this->vco->setOscBitcrusher(value);
    }
    
    void setFilterDrive(float value)
    {
        this->filterHandler->setFilterDrive(value);
    }

	void setFiltertype(float value)
	{
        this->filterHandler->setFiltertype(value);
        this->countPostFilter = 0;
	}

	inline bool process(float* sampleL, float* sampleR, float cutoff)
	{
		float sample = 0.0f;
		if (this->isNotePlaying())
		{
            this->envelopeEditorVoiceHandler->tick();
			this->processFreeEnvelope();

			float masterNote = this->portamento->tick((float)noteNumber, portamentoValue, portamentoMode > 0.5f);
            masterNote += this->pitchwheelHandler->getPitch() + this->mastertune + this->transpose;
            masterNote *= (this->detuneFactor * this->detune + 1.0f);
            this->vco->process(&sample, masterNote);

            // introduce vintage noise
            if (this->vintageNoise > 0.0f)
            {
                sample += this->oscNoise->getNextSampleVintage() * this->vintageNoise * 0.2f;
            }

			this->processFilter(&sample, cutoff);
			this->processAmp(&sample);

            sample *= this->velocityHandler->getVolume(velocity);
            sample *= this->envelopeEditorHandler->getVolumeValue(this->envelopeEditorVoiceHandler->getValue());

			*sampleL += sample;
			*sampleR += sample;
			return true;
		}

		this->prepareFilterForNextNote(cutoff);
		return false;
	}
};
#endif
