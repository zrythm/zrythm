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

#if !defined(__VocoderEngine_h)
#define __VocoderEngine_h

#include "audioutils.h"
#include "Params.h"
#include "HighPass.h"
#include "./synth/OscNoise.h"
#include "./synth/VoiceManager.h"
#include "./synth/SynthVoice.h"
#include "./vocoder/FftVocoder.h"
#include "./harmonic/HarmonicEngine.h"
#include "./chorus/ChorusEngine.h"

class VocoderEngine 
{
public:
	VoiceManager *voiceManager;
	OscNoise *noiseGenerator;

    HighPass* highPass;

    FftVocoder *vocoder;

	AudioUtils audioUtils;

    HarmonicEngine *harmonicEngine;
    ChorusEngine *chorusEngine;

    int clipDuration;

    bool inputClip;
    float volume;
    bool inputMode;

	VocoderEngine(float sampleRate) 
	{
		initialize(sampleRate);
	}

	~VocoderEngine()
	{
		delete noiseGenerator;
        delete voiceManager;
        delete vocoder;
        delete highPass;
        delete harmonicEngine;
        delete chorusEngine;
	}

	void initialize(float sampleRate)
	{
        if (sampleRate <= 0)
        {
            sampleRate = 44100.0f;
        }

        this->volume = 0.5f;
        this->inputMode = false;
        this->clipDuration = 200;
        this->inputClip = false;

		this->noiseGenerator = new OscNoise(sampleRate);
		this->voiceManager = new VoiceManager(sampleRate);
        this->vocoder = new FftVocoder(4, sampleRate, 2 * 256);
        this->highPass = new HighPass();
        this->highPass->setCutoff(0.8f);

        this->harmonicEngine = new HarmonicEngine(sampleRate);
        this->chorusEngine = new ChorusEngine(sampleRate);
	}

    void setNoteOn(int note, float velocity)
    {
        this->voiceManager->setNoteOn(note, velocity);
    }

    void setNoteOff(int note)
    {
        this->voiceManager->setNoteOff((int)note);
    }

    void reset()
    {
        this->voiceManager->reset();
    }

    void setVolume(float value)
    {
        this->volume = audioUtils.getLogScaledVolume(value, 2.0f);
    }

    void setNoiseVolume(float value)
    {
        value = audioUtils.getLogScaledVolume(value, 1.0f);

        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
			voices[i]->setNoiseVolume(value);
		}
    }

    void setPulseVolume(float value)
    {
        value = audioUtils.getLogScaledVolume(value, 1.0f);

        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
			voices[i]->setOsc1Volume(value);
		}
    }

    void setSawVolume(float value)
    {
        value = audioUtils.getLogScaledVolume(value, 1.0f);

        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
			voices[i]->setOsc2Volume(value);
		}
    }

    void setSubOscVolume(float value)
    {
        value = audioUtils.getLogScaledVolume(value, 1.0f);

        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
			voices[i]->setSubOscVolume(value);
		}
    }

    void setTranspose(float value)
    {
        value = audioUtils.getTranspose(value);

        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
			voices[i]->setTranspose(value);
		}
    }

    void setPortamentoIntensity(float value)
    {
        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
            voices[i]->setPortamento(value);
		}

        this->setPortamentoMode();
    }

    void setPortamentoMode()
    {
        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
            if (voiceManager->getNumberOfVoices() > 1)
            {
                voices[i]->setPortamentoMode(1);
            }
            else
            {
                voices[i]->setPortamentoMode(2);
            }
		}
    }

    void setTune(float value)
    {
        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
            voices[i]->setMastertune(value);
		}
    }

    void setPulseFineTune(float value)
    {
        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
            Vco *vco = voices[i]->getVco();
            vco->setOsc1FineTune(value);
		}
    }

    void setPulseTune(float value)
    {
        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
            Vco *vco = voices[i]->getVco();
            vco->setOsc1Tune(value);
		}
    }

    void setSawFineTune(float value)
    {
        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
            Vco *vco = voices[i]->getVco();
            vco->setOsc2FineTune(value);
		}
    }

    void setSawTune(float value)
    {
        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
            Vco *vco = voices[i]->getVco();
            vco->setOsc2Tune(value);
		}
    }

    void setOscSync(bool value)
    {
        SynthVoice** voices = voiceManager->getAllVoices();
		for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		{
            Vco *vco = voices[i]->getVco();
            vco->setOscSync(value);
		}
    }

    void setInputMode(bool inputMode)
    {
        this->inputMode = inputMode;
    }

    void setChorus(bool chorus)
    {
        this->chorusEngine->setEnablesChorus(chorus, false);
    }

    void setPolyMode(bool polyMode)
    {
        if (polyMode)
        {
            voiceManager->setNumberOfVoices(6);
        }
        else
        {
            voiceManager->setNumberOfVoices(1);
        }

        this->setPortamentoMode();
    }

    void setEnvelopeRelease(float value)
    {
        value = audioUtils.getLogScaledValue(value);
        EnvelopeManager *envelopeManager = this->vocoder->getEnvelopeManager();
        envelopeManager->setReleaseTime(value);
    }

    void setEsserIntensity(float value)
    {
        value = audioUtils.getLogScaledValue(value);
        EnvelopeManager *envelopeManager = this->vocoder->getEnvelopeManager();
        envelopeManager->setEsserIntensity(value);
    }

    void setHarmonicIntensity(float value)
    {
        this->harmonicEngine->setIntensity(value);
    }

    void setVocoderBandVolume(float value, int band)
    {
        value = audioUtils.getLogScaledVolume(value, 2.0f);
        EnvelopeManager *envelopeManager = this->vocoder->getEnvelopeManager();
        envelopeManager->setVocoderBandVolume(value, band);
    }

    void setSampleRate(float sampleRate)
    {
        this->initialize(sampleRate);
    }

    bool doesClip()
    {
        if (this->inputClip)
        {
            this->clipDuration = 0;
            this->inputClip = false;
        }
        else
        {
            this->clipDuration++;
            if (this->clipDuration > 20)
            {
                this->clipDuration = 20;
            }
        }

        if (this->clipDuration < 20)
        {
            return true;
        }

        return false;
    }

    void checkClip(float volume)
    {
        volume = fabs(volume);

        if (volume > 1.0f)
        {
            this->inputClip = true;
        }
    }

	void process(float *inputL, float *inputR) 
	{
        // denormalisation noise
        float noiseDenormal = this->noiseGenerator->getNextSample() * 0.0000000001f;
		float carrierL = noiseDenormal;
		float carrierR = noiseDenormal;

        float modulation = 0.0f;

        if (!inputMode)
        {
            // limit modulation signal
            modulation = (*inputL + *inputR) * 0.5f;
            this->checkClip(modulation);

		    // synth stuff
		    bool playingNotes = false;
            SynthVoice** voices = voiceManager->getAllVoices();
		    for (int i = 0; i < voiceManager->MAX_VOICES; i++)
		    {
			    playingNotes |= voices[i]->process(&carrierL, &carrierR, 1.0f);
		    }
        }
        else
        {
            this->checkClip((*inputR + *inputL) * 0.5f);
            modulation = *inputR;
            carrierL = *inputL;
        }

        // high pass modulation signal
        highPass->tick(&modulation);

        // limit signal
        if (modulation > 1.0f) modulation = 1.0f;
        if (modulation < -1.0f) modulation = -1.0f;
        if (carrierL > 1.0f) carrierL = 1.0f;
        if (carrierL < -1.0f) carrierL = -1.0f;

        // add harmonics if enabled
        harmonicEngine->process(&carrierL);

        // convolution / vocoder
        float result = 0.0f;
        vocoder->process(modulation * 2.0f, carrierL * 2.0f, &result);

        *inputR = result * this->volume;
        *inputL = *inputR;

        // Chorus
        this->chorusEngine->process(inputL, inputR);
	}
};
#endif

