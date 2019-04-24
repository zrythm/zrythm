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
#include <climits>
#include "VoiceQueue.h"
#include "SynthEngine.h"
#include "Lfo.h"

class Motherboard
{
private:
	VoiceQueue vq;
	int totalvc;
	bool wasUni;
	bool awaitingkeys[129];
	int priorities[129];

	Decimator17 left,right;
	int asPlayedCounter;
	float lkl,lkr;
	float sampleRate,sampleRateInv;
	//JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Motherboard)
public:
	bool asPlayedMode;
	Lfo mlfo,vibratoLfo;
	float vibratoAmount;
	bool vibratoEnabled;

	float Volume;
	const static int MAX_VOICES=8;
	float pannings[MAX_VOICES];
	ObxdVoice voices[MAX_VOICES];
	bool uni;
	bool Oversample;

	bool economyMode;
	Motherboard(): left(),right()
	{
		economyMode = true;
		lkl=lkr=0;
		vibratoEnabled = true;
		asPlayedMode = false;
		asPlayedCounter = 0;
		for(int i = 0 ; i < 129 ; i++)
		{
			awaitingkeys[i] = false;
			priorities[i] = 0;
		}
		vibratoAmount = 0;
		Oversample=false;
		mlfo= Lfo();
		vibratoLfo=Lfo();
		vibratoLfo.waveForm = 1;
		uni = false;
		wasUni = false;
		Volume=0;
	//	voices = new ObxdVoice* [MAX_VOICES];
	//	pannings = new float[MAX_VOICES];
		totalvc = MAX_VOICES;
		vq = VoiceQueue(MAX_VOICES,voices);
		for(int i = 0 ; i < MAX_VOICES;++i)
		{
			pannings[i]= 0.5;
		}
	}
	~Motherboard()
	{
		//delete pannings;
		//for(int i = 0 ; i < MAX_VOICES;++i)
		//{
		//	delete voices[i];
		//}
		//delete voices;
	}
	void setVoiceCount(int count)
	{
		for(int i = count ; i < MAX_VOICES;i++)
		{
			voices[i].NoteOff();
			voices[i].ResetEnvelope();
		}
		vq.reInit(count);
		totalvc = count;
	}
	void unisonOn()
	{
		//for(int i = 0 ; i < 110;i++)
		//	awaitingkeys[i] = false;
	}
	void setSampleRate(float sr)
	{
		sampleRate = sr;
		sampleRateInv = 1 / sampleRate;
		mlfo.setSamlpeRate(sr);
		vibratoLfo.setSamlpeRate(sr);
		for(int i = 0 ; i < MAX_VOICES;++i)
		{
			voices[i].setSampleRate(sr);
		}
		SetOversample(Oversample);
	}
	void sustainOn()
	{
		for(int i = 0 ; i < MAX_VOICES;i++)
		{
			ObxdVoice* p = vq.getNext();
			p->sustOn();
		}
	}
	void sustainOff()
	{
		for(int i = 0 ; i < MAX_VOICES;i++)
		{
			ObxdVoice* p = vq.getNext();
			p->sustOff();
		}
	}
	void setNoteOn(int noteNo,float velocity)
	{
		asPlayedCounter++;
		priorities[noteNo] = asPlayedCounter;
		bool processed=false;
		if (wasUni != uni)
			unisonOn();
		if (uni)
		{
			if(!asPlayedMode)
			{
				int minmidi = 129;
				for(int i = 0 ; i < totalvc; i++)
				{
					ObxdVoice* p = vq.getNext();
					if(p->midiIndx < minmidi && p->Active)
					{
						minmidi = p->midiIndx;
					}
				}
				if(minmidi < noteNo)
				{
					awaitingkeys[noteNo] = true;
				}
				else
				{
					for(int i = 0 ; i < totalvc;i++)
					{
						ObxdVoice* p = vq.getNext();
						if(p->midiIndx > noteNo && p->Active)
						{
							awaitingkeys[p->midiIndx] = true;
							p->NoteOn(noteNo,-0.5);
						}
						else
						{
							p->NoteOn(noteNo,velocity);
						}
					}
				}
				processed = true;
			}
			else
			{
				for(int i = 0 ; i < totalvc; i++)
				{
					ObxdVoice* p = vq.getNext();
					if(p->Active)
					{
						awaitingkeys[p->midiIndx] = true;
											p->NoteOn(noteNo,-0.5);
					}
					else
					{
					p->NoteOn(noteNo,velocity);
					}
				}
				processed = true;
			}
		}
		else
		{
			for (int i = 0; i < totalvc && !processed; i++)
			{
				ObxdVoice* p = vq.getNext();
				if (!p->Active)
				{
					p->NoteOn(noteNo,velocity);
					processed = true;
				}
			}
		}
		// if voice steal occured
		if(!processed)
		{
			//
			if(!asPlayedMode)
			{
				int maxmidi = 0;
				ObxdVoice* highestVoiceAvalible = NULL;
				for(int i = 0 ; i < totalvc; i++)
				{
					ObxdVoice* p = vq.getNext();
					if(p->midiIndx > maxmidi)
					{
						maxmidi = p->midiIndx;
						highestVoiceAvalible = p;
					}
				}
				if(maxmidi < noteNo)
				{
					awaitingkeys[noteNo] = true;
				}
				else
				{
					highestVoiceAvalible->NoteOn(noteNo,-0.5);
					awaitingkeys[maxmidi] = true;
				}
			}
			else
			{
				int minPriority = INT_MAX;
				ObxdVoice* minPriorityVoice = NULL;
				for(int i = 0 ; i < totalvc; i++)
				{
					ObxdVoice* p = vq.getNext();
					if(priorities[p->midiIndx] <minPriority)
					{
						minPriority = priorities[p->midiIndx];
						minPriorityVoice = p;
					}
				}
				awaitingkeys[minPriorityVoice->midiIndx] = true;
				minPriorityVoice->NoteOn(noteNo,-0.5);
			}
		}
		wasUni = uni;
	}

	void setNoteOff(int noteNo)
	{
		awaitingkeys[noteNo] = false;
		int reallocKey = 0;
		//Voice release case
		if(!asPlayedMode)
		{
			while(reallocKey < 129 &&(!awaitingkeys[reallocKey]))
			{
				reallocKey++;
			}
		}
		else
		{
			reallocKey = 129;
			int maxPriority = INT_MIN;
			for(int i = 0 ; i < 129;i++)
			{
				if(awaitingkeys[i] && (maxPriority < priorities[i]))
				{
					reallocKey = i;
					maxPriority = priorities[i];
				}
			}
		}
		if(reallocKey !=129)
		{
			for(int i = 0 ; i < totalvc; i++)
			{
				ObxdVoice* p = vq.getNext();
				if((p->midiIndx == noteNo) && (p->Active))
				{
					p->NoteOn(reallocKey,-0.5);
					awaitingkeys[reallocKey] = false;
				}

			}
		}
		else
		//No realloc
		{
			for (int i = 0; i < totalvc; i++)
			{
				ObxdVoice* n = vq.getNext();
				if (n->midiIndx==noteNo && n->Active)
				{
					n->NoteOff();
				}
			}
		}
	}
	void SetOversample(bool over)
	{
		if(over==true)
		{
			mlfo.setSamlpeRate(sampleRate*2);
			vibratoLfo.setSamlpeRate(sampleRate*2);
		}
		else
		{
			mlfo.setSamlpeRate(sampleRate);
			vibratoLfo.setSamlpeRate(sampleRate);
		}
		for(int i = 0 ; i < MAX_VOICES;i++)
		{
			voices[i].setHQ(over);
			if(over)
				voices[i].setSampleRate(sampleRate*2);
			else
				voices[i].setSampleRate(sampleRate);
		}
		Oversample = over;
	}
	inline float processSynthVoice(ObxdVoice& b,float lfoIn,float vibIn )
	{
		if(economyMode)
			b.checkAdsrState();
		if(b.shouldProcessed||(!economyMode))
		{
				b.lfoIn=lfoIn;
				b.lfoVibratoIn=vibIn;
				return b.ProcessSample();
		}
		return 0;
	}
	void processSample(float* sm1,float* sm2)
	{
		mlfo.update();
		vibratoLfo.update();
		float vl=0,vr=0;
		float vlo = 0 , vro = 0 ;
		float lfovalue = mlfo.getVal();
		float viblfo = vibratoEnabled?(vibratoLfo.getVal() * vibratoAmount):0;
		float lfovalue2=0,viblfo2=0;
		if(Oversample)
		{		
			mlfo.update();
		vibratoLfo.update();
		lfovalue2 = mlfo.getVal();
		viblfo2 = vibratoEnabled?(vibratoLfo.getVal() * vibratoAmount):0;
		}

		for(int i = 0 ; i < totalvc;i++)
		{
				float x1 = processSynthVoice(voices[i],lfovalue,viblfo);
				if(Oversample)
				{
					float x2 =  processSynthVoice(voices[i],lfovalue2,viblfo2);
					vlo+=x2*(1-pannings[i]);
					vro+=x2*(pannings[i]);
				}
				vl+=x1*(1-pannings[i]);
				vr+=x1*(pannings[i]);
		}
		if(Oversample)
		{
			vl = left.Calc(vl,vlo);
			vr = right.Calc(vr,vro);
		}
		*sm1 = vl*Volume;
		*sm2 = vr*Volume;
	}
};
