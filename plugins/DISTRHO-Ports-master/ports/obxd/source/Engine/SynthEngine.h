/*
	==============================================================================
	This file is part of Obxd synthesizer.

	Copyright ) 2013-2014 Filatov Vadim
	
	Contact author via email :
	justdat_@_e1.ru

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
#pragma once

#include "../PluginProcessor.h"
#include "ObxdVoice.h"
#include "Motherboard.h"
#include "Params.h"
#include "ParamSmoother.h"

class SynthEngine
{
private:
	Motherboard synth;
	ParamSmoother cutoffSmoother;
	ParamSmoother pitchWheelSmoother;
	ParamSmoother modWheelSmoother;
	float sampleRate;
	//JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEngine)
public:
	SynthEngine():
		cutoffSmoother(),
		//synth = new Motherboard();
		pitchWheelSmoother(),
		modWheelSmoother()
	{
	}
	~SynthEngine()
	{
		//delete synth;
	}
	void setPlayHead(float bpm,float retrPos)
	{
		synth.mlfo.hostSyncRetrigger(bpm,retrPos);
	}
	void setSampleRate(float sr)
	{
		sampleRate = sr;
		cutoffSmoother.setSampleRate(sr);
		pitchWheelSmoother.setSampleRate(sr);
		modWheelSmoother.setSampleRate(sr);
		synth.setSampleRate(sr);
	}
	void processSample(float *left,float *right)
	{
		processCutoffSmoothed(cutoffSmoother.smoothStep());
		procPitchWheelSmoothed(pitchWheelSmoother.smoothStep());
		procModWheelSmoothed(modWheelSmoother.smoothStep());

		synth.processSample(left,right);
	}
	void allNotesOff()
	{
		for(int i = 0 ;  i < 128;i++)
			{
				procNoteOff(i);
			}
	}
	void allSoundOff()
	{
		allNotesOff();
		for(int i = 0 ; i < Motherboard::MAX_VOICES;i++)
			{
				synth.voices[i].ResetEnvelope();
			}
	}
	void sustainOn()
	{
		synth.sustainOn();
	}
	void sustainOff()
	{
		synth.sustainOff();
	}
	void procLfoSync(float val)
	{
		if(val > 0.5)
			synth.mlfo.setSynced();
		else
			synth.mlfo.setUnsynced();
	}
	void procAsPlayedAlloc(float val)
	{
		synth.asPlayedMode = val > 0.5;
	}
	void procNoteOn(int noteNo,float velocity)
	{
		synth.setNoteOn(noteNo,velocity);
	}
	void procNoteOff(int noteNo)
	{
		synth.setNoteOff(noteNo);
	}
	void procEconomyMode(float val)
	{
		synth.economyMode = val>0.5;
	}
#define ForEachVoice(expr) \
	for(int i = 0 ; i < synth.MAX_VOICES;i++) \
		{\
			synth.voices[i].expr;\
		}\

	void procAmpVelocityAmount(float val)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].vamp= val;
		}
	}
	void procFltVelocityAmount(float val)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].vflt= val;
		}
	}
	void procModWheel(float val)
	{
		modWheelSmoother.setSteep(val);
	}
	void procModWheelSmoothed(float val)
	{
		synth.vibratoAmount = val;
	}
	void procModWheelFrequency(float val)
	{
		synth.vibratoLfo.setFrequency (logsc(val,3,10));
		synth.vibratoEnabled = val>0.05;
	}
	void procPitchWheel(float val)
	{
		pitchWheelSmoother.setSteep(val);
		//for(int i = 0 ; i < synth->MAX_VOICES;i++)
		//{
		//	synth->voices[i]->pitchWheel = val;
		//}
	}
	inline void procPitchWheelSmoothed(float val)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].pitchWheel = val;
		}
	}
	void setVoiceCount(float param)
	{
		synth.setVoiceCount(roundToInt((param*7) +1));
	}
	void procPitchWheelAmount(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].pitchWheelAmt = param>0.5?12:2;
		}
	}
	void procPitchWheelOsc2Only(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].pitchWheelOsc2Only = param>0.5;
		}
	}
	void processPan(float param,int idx)
	{
		synth.pannings[idx-1] = param;
	}
	void processTune(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.tune = param*2-1;
		}
	}
	void processLegatoMode(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].legatoMode = roundToInt(param*3 + 1) -1;
		}
	}
	void processOctave(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.oct = (roundToInt(param*4) -2)*12;
		}
	}
	void processFilterKeyFollow(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].fltKF = param;
		}
	}
	void processSelfOscPush(float param)
	{
		ForEachVoice(selfOscPush = param>0.5);
		ForEachVoice(flt.selfOscPush = param>0.5);
	}
	void processUnison(float param)
	{
		synth.uni = param>0.5f;
	}
	void processPortamento(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].porta =logsc(1-param,0.14,250,150);
		}
	}
	void processVolume(float param)
	{
		synth.Volume = linsc(param,0,0.30);
	}
	void processLfoFrequency(float param)
	{
		synth.mlfo.setRawParam(param);
		synth.mlfo.setFrequency(logsc(param,0,50,120));
	}
	void processLfoSine(float param)
	{
		if(param>0.5)
		{
			synth.mlfo.waveForm |=1;
		}
		else
		{
			synth.mlfo.waveForm&=~1;
		}
	}
	void processLfoSquare(float param)
	{
		if(param>0.5)
		{
			synth.mlfo.waveForm |=2;
		}
		else
		{
			synth.mlfo.waveForm&=~2;
		}
	}
	void processLfoSH(float param)
	{
		if(param>0.5)
		{
			synth.mlfo.waveForm |=4;
		}
		else
		{
			synth.mlfo.waveForm&=~4;
		}
	}
	void processLfoAmt1(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].lfoa1 = logsc(logsc(param,0,1,60),0,60,10);
		}
	}
	void processLfoOsc1(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].lfoo1 = param>0.5;
		}
	}
	void processLfoOsc2(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].lfoo2 = param>0.5;
		}
	}
	void processLfoFilter(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].lfof = param>0.5;
		}
	}
	void processLfoPw1(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].lfopw1 = param>0.5;
		}
	}
	void processLfoPw2(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].lfopw2 = param>0.5;
		}
	}
	void processLfoAmt2(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].lfoa2 = linsc(param,0,0.7);
		}
	}
	void processDetune(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.totalDetune = logsc(param,0.001,0.90);
		}
	}
	void processPulseWidth(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.pulseWidth = linsc(param,0.0,0.95);
		}
	}
	void processPwEnv(float param)
	{
		ForEachVoice (pwenvmod=linsc(param,0,0.85));
	}
	void processPwOfs(float param)
	{
		ForEachVoice(pwOfs = linsc(param,0,0.75));
	}
	void processPwEnvBoth(float param)
	{
		ForEachVoice(pwEnvBoth = param>0.5);
	}
	void processInvertFenv(float param)
	{
		ForEachVoice(invertFenv = param>0.5);
	}
	void processPitchModBoth(float param)
	{
		ForEachVoice(pitchModBoth = param>0.5);
	}
	void processOsc2Xmod(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.xmod= param*24;
		}
	}
	void processEnvelopeToPitch(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].envpitchmod= param*36;
		}
	}
	void processOsc2HardSync(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.hardSync = param>0.5;
		}
	}
	void processOsc1Pitch(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.osc1p = (param * 48);
		}
	}
	void processOsc2Pitch(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.osc2p = (param * 48);
		}
	}
	void processPitchQuantization(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.quantizeCw = param>0.5;
		}
	}
	void processOsc1Mix(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.o1mx = param;
		}
	}
	void processOsc2Mix(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.o2mx = param;
		}
	}
	void processNoiseMix(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.nmx = logsc(param,0,1,35);
		}
	}
	void processBrightness(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].setBrightness(  linsc(param,7000,26000));
		}
	}
	void processOsc2Det(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.osc2Det = logsc(param,0.001,0.6);
		}
	}

	void processOsc1Saw(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.osc1Saw = param>0.5;
		}
	}
	void processOsc1Pulse(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.osc1Pul = param>0.5;
		}
	}
	void processOsc2Saw(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.osc2Saw= param>0.5;
		}
	}
	void processOsc2Pulse(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].osc.osc2Pul= param>0.5;
		}
	}

	void processCutoff(float param)
	{
		cutoffSmoother.setSteep( linsc(param,0,120));
	//	for(int i = 0 ; i < synth->MAX_VOICES;i++)
	//	{
			//synth->voices[i]->cutoff = logsc(param,60,19000,30);
		//	synth->voices[i]->cutoff = linsc(param,0,120);
	//	}
	}
	inline void processCutoffSmoothed(float param)
	{
		ForEachVoice(cutoff=param);
	}
	void processBandpassSw(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			//synth.voices[i].cutoff = logsc(param,60,19000,30);
			synth.voices[i].flt.bandPassSw = param>0.5;
		}
	}
	void processResonance(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].flt.setResonance(0.991-logsc(1-param,0,0.991,40));
		}
	}
	void processFourPole(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			//synth.voices[i].flt ;
			synth.voices[i].fourpole = param>0.5;
		}
	}
	void processMultimode(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			//synth.voices[i].flt ;
			synth.voices[i].flt.setMultimode(linsc(param,0,1));
		}
	}
	void processOversampling(float param)
	{
		synth.SetOversample(param>0.5);
	}
	void processFilterEnvelopeAmt(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].fenvamt = linsc(param,0,140);
		}
	}
	void processLoudnessEnvelopeAttack(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].env.setAttack(logsc(param,4,60000,900));
		}
	}
	void processLoudnessEnvelopeDecay(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].env.setDecay(logsc(param,4,60000,900));
		}
	}
	void processLoudnessEnvelopeRelease(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].env.setRelease(logsc(param,8,60000,900));
		}
	}
	void processLoudnessEnvelopeSustain(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].env.setSustain(param);
		}
	}
	void processFilterEnvelopeAttack(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].fenv.setAttack(logsc(param,1,60000,900));
		}
	}
	void processFilterEnvelopeDecay(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].fenv.setDecay(logsc(param,1,60000,900));
		}
	}
	void processFilterEnvelopeRelease(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].fenv.setRelease(logsc(param,1,60000,900));
		}
	}
	void processFilterEnvelopeSustain(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].fenv.setSustain(param);
		}
	}
	void processEnvelopeDetune(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].setEnvDer(linsc(param,0.0,1));
		}
	}
	void processFilterDetune(float param)
	{
for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].FltDetAmt = linsc(param,0.0,18);
		}
	}
	void processPortamentoDetune(float param)
	{
		for(int i = 0 ; i < synth.MAX_VOICES;i++)
		{
			synth.voices[i].PortaDetuneAmt = linsc(param,0.0,0.75);
		}
	}
	void processLoudnessDetune(float param)
	{
		ForEachVoice(levelDetuneAmt = linsc(param,0.0,0.67));
	}

		 
};
