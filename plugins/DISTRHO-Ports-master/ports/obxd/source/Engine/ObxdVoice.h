/*
	==============================================================================
	This file is part of Obxd synthesizer.

	Copyright � 2013-2014 Filatov Vadim
	
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
#include "ObxdOscillatorB.h"
#include "AdsrEnvelope.h"
#include "Filter.h"
#include "Decimator.h"
#include "APInterpolator.h"

class ObxdVoice
{
private:
	float SampleRate;
	float sampleRateInv;
	float Volume;
	float port;
	float velocityValue;

	float d1,d2;
	float c1,c2;

	bool hq;

	//JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObxdVoice)
public:
	bool sustainHold;
	//bool resetAdsrsOnAttack;

	AdsrEnvelope env;
	AdsrEnvelope fenv;
	ObxdOscillatorB osc;
	Filter flt;

	Random ng;

	float vamp,vflt;

	float cutoff;
	float fenvamt;

	float EnvDetune;
	float FenvDetune;

	float FltDetune;
	float FltDetAmt;

	float PortaDetune;
	float PortaDetuneAmt;

	float levelDetune;
	float levelDetuneAmt;

	float brightCoef;

	int midiIndx;

	bool Active;
	bool shouldProcessed;

	float fltKF;

	float porta;
	float prtst;

	float cutoffwas,envelopewas;

	float lfoIn;
	float lfoVibratoIn;

	float pitchWheel;
	float pitchWheelAmt;
	bool pitchWheelOsc2Only;

	float lfoa1,lfoa2;
	bool lfoo1,lfoo2,lfof;
	bool lfopw1,lfopw2;

	bool Oversample;
	bool selfOscPush;

	float envpitchmod;
	float pwenvmod;

	float pwOfs;
	bool pwEnvBoth;
	bool pitchModBoth;

	bool invertFenv;


	bool fourpole;


	DelayLine<Samples*2> lenvd,fenvd,lfod;

	ApInterpolator ap;
	float oscpsw;
	int legatoMode;
	float briHold;

	ObxdVoice() 
		: ap()
	{
		hq = false;
		selfOscPush = false;
		pitchModBoth = false;
		pwOfs = 0 ;
		invertFenv = false;
		pwEnvBoth = false;
		ng = Random(Random::getSystemRandom().nextInt64());
		sustainHold = false;
		shouldProcessed = false;
		vamp=vflt=0;
		velocityValue=0;
		lfoVibratoIn=0;
		fourpole = false;
		legatoMode = 0;
		brightCoef =briHold= 1;
		envpitchmod = 0;
		pwenvmod = 0;
		oscpsw = 0;
		cutoffwas = envelopewas=0;
		Oversample= false;
		c1=c2=d1=d2=0;
		pitchWheel=pitchWheelAmt=0;
		lfoIn=0;
		PortaDetuneAmt=0;
		FltDetAmt=0;
		levelDetuneAmt=0;
		porta =0;
		prtst=0;
		fltKF= false;
		cutoff=0;
		fenvamt = 0;
		Active = false;
		midiIndx = 30;
		levelDetune = Random::getSystemRandom().nextFloat()-0.5;
		EnvDetune = Random::getSystemRandom().nextFloat()-0.5;
		FenvDetune = Random::getSystemRandom().nextFloat()-0.5;
		FltDetune = Random::getSystemRandom().nextFloat()-0.5;
		PortaDetune =Random::getSystemRandom().nextFloat()-0.5;
	//	lenvd=new DelayLine(Samples*2);
	//	fenvd=new DelayLine(Samples*2);
	}
	~ObxdVoice()
	{
	//	delete lenvd;
	//	delete fenvd;
	}
	inline float ProcessSample()
	{
		//portamento on osc input voltage
		//implements rc circuit
		float ptNote  =tptlpupw(prtst, midiIndx-81, porta * (1+PortaDetune*PortaDetuneAmt),sampleRateInv);
		osc.notePlaying = ptNote;
		//both envelopes and filter cv need a delay equal to osc internal delay
		float lfoDelayed = lfod.feedReturn(lfoIn);
		//filter envelope undelayed
		float envm = fenv.processSample() * (1 - (1-velocityValue)*vflt);
		if(invertFenv)
			envm = -envm;
		//filter exp cutoff calculation
		float cutoffcalc = jmin(
			getPitch(
			(lfof?lfoDelayed*lfoa1:0)+
			cutoff+
			FltDetune*FltDetAmt+
			fenvamt*fenvd.feedReturn(envm)+
			-45 + (fltKF*(ptNote+40))
			)
			//noisy filter cutoff
			+(ng.nextFloat()-0.5f)*3.5f
			, (flt.SampleRate*0.5f-120.0f));//for numerical stability purposes

		//limit our max cutoff on self osc to prevent alising
		if(selfOscPush)
			cutoffcalc = jmin(cutoffcalc,19000.0f);


		//PW modulation
		osc.pw1 = (lfopw1?(lfoIn * lfoa2):0) + (pwEnvBoth?(pwenvmod * envm) : 0);
		osc.pw2 = (lfopw2?(lfoIn * lfoa2):0) + pwenvmod * envm + pwOfs;

		//Pitch modulation
		osc.pto1 =   (!pitchWheelOsc2Only? (pitchWheel*pitchWheelAmt):0 ) + ( lfoo1?(lfoIn * lfoa1):0) + (pitchModBoth?(envpitchmod * envm):0) + lfoVibratoIn;
		osc.pto2 =  (pitchWheel *pitchWheelAmt) + (lfoo2?lfoIn*lfoa1:0) + (envpitchmod * envm) + lfoVibratoIn;



		//variable sort magic - upsample trick
		float envVal = lenvd.feedReturn(env.processSample() * (1 - (1-velocityValue)*vamp));

		float oscps = osc.ProcessSample() * (1 - levelDetuneAmt*levelDetune);


		oscps = oscps - tptlpupw(c1,oscps,12,sampleRateInv);

		float x1 = oscps;
		x1 = tptpc(d2,x1,brightCoef);
		if(fourpole)
			x1 = flt.Apply4Pole(x1,(cutoffcalc)); 
		else
			x1 = flt.Apply(x1,(cutoffcalc)); 
		x1 *= (envVal);
		return x1;
	}
	void setBrightness(float val)
	{
		briHold = val;
		brightCoef = tan(jmin(val,flt.SampleRate*0.5f-10)* (juce::float_Pi)*flt.sampleRateInv);

	}
	void setEnvDer(float d)
	{
		env.setUniqueDeriviance(1 + EnvDetune*d);
		fenv.setUniqueDeriviance(1 + FenvDetune*d);
	}
	void setHQ(bool hq)
	{
		if(hq)
		{
			osc.setDecimation();
		}
		else
		{
			osc.removeDecimation();
		}
	}
	void setSampleRate(float sr)
	{
		flt.setSampleRate(sr);
		osc.setSampleRate(sr);
		env.setSampleRate(sr);
		fenv.setSampleRate(sr);
		SampleRate = sr;
		sampleRateInv = 1 / sr;
		brightCoef = tan(jmin(briHold,flt.SampleRate*0.5f-10)* (juce::float_Pi) * flt.sampleRateInv);
	}
	void checkAdsrState()
	{
		shouldProcessed = env.isActive();
	}
	void ResetEnvelope()
	{
		env.ResetEnvelopeState();
		fenv.ResetEnvelopeState();
	}
	void NoteOn(int mididx,float velocity)
	{
		if(!shouldProcessed)
		{
			//When your processing is paused we need to clear delay lines and envelopes
			//Not doing this will cause clicks or glitches
			lenvd.fillZeroes();
			fenvd.fillZeroes();
			ResetEnvelope();
		}
		shouldProcessed = true;
		if(velocity!=-0.5)
			velocityValue = velocity;
		midiIndx = mididx;
		if((!Active)||(legatoMode&1))
			env.triggerAttack();
		if((!Active)||(legatoMode&2))
			fenv.triggerAttack();
		Active = true;
	}
	void NoteOff()
	{
		if(!sustainHold)
		{
		env.triggerRelease();
		fenv.triggerRelease();
		}
		Active = false;
	}
	void sustOn()
	{
		sustainHold = true;
	}
	void sustOff()
	{
		sustainHold = false;
		if(!Active)
		{
			env.triggerRelease();
			fenv.triggerRelease();
		}

	}
};
