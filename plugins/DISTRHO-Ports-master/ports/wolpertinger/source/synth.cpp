#include <cfloat>
#include "synth.h"
#include "tabbed-editor.h"
#include "about.h"


/*
Versions
0.4	ADSR Volume Envelope
	8x / 16x Oversampling
	Bugfix: Set sample rate correctly
0.3 Virtual Keyboard Component, GUI niceties
	Changed back to Juce
0.2 MIDI parameter changes now update the GUI
    Binaries no longer linked to OpenGL
    Debug and Release binaries included
0.1 initial release
*/



AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wolp();
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter(const String& commandLine)
{
    return new wolp();
}


// ------------------------- SynthVoice -------------------------

template <int oversampling>
wolpVoice<oversampling>::wolpVoice(wolp *s): synth(s)
{
	setCurrentPlaybackSampleRate(((AudioProcessor*)s)->getSampleRate());
}

template <int oversampling>
void wolpVoice<oversampling>::startNote(const int midiNoteNumber,
							 const float velocity,
							 SynthesiserSound* sound,
							 const int currentPitchWheelPosition)
{
	phase= low= band= high= 0;
	freq= synth->getnotefreq(midiNoteNumber);
	vol= velocity;
	cyclecount= 0;
	samples_synthesized= 0;
	playing= true;
	env.setAttack(synth->getparam(wolp::attack));
	env.setDecay(synth->getparam(wolp::decay));
	env.setSustain(synth->getparam(wolp::sustain));
	env.setRelease(synth->getparam(wolp::release));
	env.resetTime();
}

template <int oversampling>
void wolpVoice<oversampling>::stopNote(float, const bool allowTailOff)
{
	//if(!allowTailOff) clearCurrentNote();
	playing= false;
}


template <int oversampling>
void wolpVoice<oversampling>::process(float* p1, float* p2, int samples)
{
	float param_cutoff= synth->getparam(wolp::cutoff);
	float cutoff= param_cutoff * freq;
	float vol= this->vol * synth->getparam(wolp::gain);
	int nfilters= int(synth->getparam(wolp::nfilters));
	float *waveSamples;

	generator.setFrequency(getSampleRate(), freq);
	generator.setMultipliers(synth->getparam(wolp::gsaw), synth->getparam(wolp::grect), synth->getparam(wolp::gtri));
	waveSamples= generator.generateSamples(samples);

	double sampleStep= 1.0/getSampleRate();

	int sampleCount= samples_synthesized;

	for(int i= 0; i<samples; i++)
	{
		double val= waveSamples[i];

		double envVol= env.getValue();

		val= filter.run(val, nfilters);

		p1[i]+= val*vol*envVol;
		p2[i]+= val*vol*envVol;

		if( !((++sampleCount) & 31) )
		{
			synth->cutoff_filter.setparams(synth->getparam(wolp::velocity) * 10000,
										   synth->getparam(wolp::inertia) * 100);

			float cut= synth->cutoff_filter.run(cutoff, this->vol*envVol * 32.0f/getSampleRate());

			float fmax= synth->getparam(wolp::filtermax);
			float fmin= synth->getparam(wolp::filtermin);
			if(cut > fmax) cut= fmax;
			if(cut < fmin) cut= fmin;

			filter.setparams(getSampleRate(), cut,
							 synth->params[wolp::bandwidth]*cut, synth->params[wolp::resonance] * 2);

			synth->setParameter(wolp::curcutoff, sqrt(cut/20000));
//			synth->setParameterNotifyingHost(wolp::curcutoff, sqrt(cut/20000));
			synth->sendChangeMessage();
		}

		env.advance(sampleStep, playing);
		samples_synthesized= sampleCount;
		if(env.isFinished()) { clearCurrentNote(); break; }
	}
}

template <int oversampling>
void wolpVoice<oversampling>::renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
{
	if(getCurrentlyPlayingNote()>=0)
	{
		float **outbuf= outputBuffer.getArrayOfWritePointers();
		process(outbuf[0]+startSample, outbuf[1]+startSample, numSamples);
	}
}




// ------------------------- Synth -------------------------


void wolp::getStateInformation (JUCE_NAMESPACE::MemoryBlock& destData)
{
	printf("getStateInformation()\n");
	XmlElement *doc= new XmlElement(String("synth"));
	for(int i= 0; i<param_size; i++)
	{
		XmlElement *p= new XmlElement("param");
		p->setAttribute(T("name"), String(paraminfos[i].internalname));
		p->setAttribute(T("val"), params[i]);
		doc->addChildElement(p);
	}
	copyXmlToBinary(*doc, destData);
	delete doc;
}

void wolp::setStateInformation (const void* data, int sizeInBytes)
{
	XmlElement *xml= getXmlFromBinary(data, sizeInBytes);
	String errorstring;

	if(xml && xml->getTagName() == String("synth"))
	{
		loaddefaultparams();
		forEachXmlChildElementWithTagName(*xml, param, T("param"))
		{
			const char *name= param->getStringAttribute(T("name")).toUTF8();
			const double val= param->getDoubleAttribute(T("val"));
			int i;
//			printf("%s = %.2f\n", name, val);
			for(i= 0; i<param_size; i++)
			{
				if( !strcmp(name, paraminfos[i].internalname) )
				{
					setParameter(i, val);
					break;
				}
			}
			if(i==param_size) errorstring+= String("Unknown parameter '") + name + "'\n";
		}
	}
	else errorstring= T("XML data corrupt\n");

	if(errorstring.length())
//		AlertWindow::showMessageBox(AlertWindow::WarningIcon, String("Wolpertinger"), errorstring);
		printf("Wolpertinger: %s\n", (const char*)errorstring.toUTF8());

	if(xml) delete xml;
}



float wolp::getparam(int idx)
{
	float v;
	switch(idx)
	{
		case gain:
			v= sqr(sqr(params[idx]));
			break;
		case clip:
			v= sqr(params[idx]);
			break;
		case gsaw:
		case grect:
		case gtri:
			v= sqr(params[idx]);
			break;
		case cutoff:
			v= sqr(params[idx]);
			break;
		case velocity:
		case inertia:
			v= sqr(params[idx]);
			break;
		case nfilters:
			return float(int(params[idx]*paraminfos[idx].max));
		case tune:
		{
			float p= params[idx]-0.5;
			return 440.0f + (p<0? -sqr(p)*220: sqr(p)*440);
		}
		case curcutoff:
		case filtermin:
		case filtermax:
			v= sqr(params[idx]);
			break;
		case attack:
		case decay:
		case sustain:
		case release:
			v= sqr(params[idx]);
			break;

		case oversampling:
			v= (params[idx] < 1.0/3+1.0/6? 1.0/16: params[idx] < 2.0/3+1.0/6? 8.0/16: 16.0/16);
			break;

		default:
			v= params[idx];
			break;
	}

	const wolp::paraminfo &i= paraminfos[idx];
	return v * (i.max-i.min); // + i.min;
}

void wolp::setParameter (int idx, float value)
{
//	if(idx!=curcutoff) printf("setParameter %s = %.2f\n", paraminfos[idx].label, value);

	switch(idx)
	{
		case curcutoff:
		{
			float cut= value;
			if(cut<params[filtermin]) cut= params[filtermin];
			if(cut>params[filtermax]) cut= params[filtermax];
			params[idx]= cut;
			cutoff_filter.setvalue(getparam(idx));
			break;
		}

		case tune:
		{
			for(int i= voices.size(); --i>=0; )
			{
				wolpVoice<8> *voice= (wolpVoice<8>*)voices.getUnchecked(i);
				int note= voice->getCurrentlyPlayingNote();
				if(note>=0) voice->setfreq(getnotefreq(note));
			}
			params[idx]= value;
			break;
		}

		case oversampling:
		{
			int nVoicesMax= 16;
			int oldVal= (int)getparam(oversampling);
			params[idx]= value;
			int val= (int)getparam(oversampling);
			if(oldVal==val) break;
			printf("oversampling: ");
			for(int i= getNumVoices(); i; i--)
				removeVoice(0);
			switch(val)
			{
				case 1:
					for(int i= 0; i<nVoicesMax ; i++)
						addVoice(new wolpVoice<1>(this));
					printf("Off\n");
					params[idx]= 1.0/16;
					break;
				case 8:
					for(int i= 0; i<nVoicesMax ; i++)
						addVoice(new wolpVoice<8>(this));
					printf("8x\n");
					params[idx]= 8.0/16;
					break;
				default:
					for(int i= 0; i<nVoicesMax ; i++)
						addVoice(new wolpVoice<16>(this));
					printf("16x\n");
					params[idx]= 16.0/16;
					break;
			}
			break;
		}

		default:
		{
			params[idx]= value;
			break;
		}
	}

    paraminfos[idx].dirty = true;

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    if (getActiveEditor())
        sendChangeMessage();
#endif
}



wolp::paraminfo wolp::paraminfos[]=
{
	{ "gain", 				"Gain",		 	0.0,	4.0,	0.3 },
	{ "clip", 				"Clip",		 	0.0,	5.0,	1.0 },
	{ "saw", 				"Saw",		 	0.0,	1.0,	1.0 },
	{ "rect", 				"Rect",		 	0.0,	1.0,	0.0 },
	{ "tri", 				"Tri",		 	0.0,	1.0,	0.0 },
	{ "tune", 				"Tune",		 	0.0, 	1.0,	0.5 },
	{ "filter_cutoff", 		"Filter X",	 	0.0, 	32.0,	0.5 },
	{ "filter_reso", 		"Resonance",	0.0,	1.0,	0.4 },
	{ "filter_bandwidth", 	"Bandwidth",	0.0,	1.0,	0.4 },
	{ "filter_velocity", 	"Velocity",	 	FLT_MIN,1.0,	0.25 },
	{ "filter_inertia", 	"Inertia",	 	0.0,	1.0,	0.25 },
	{ "filter_passes", 		"Passes",	 	0.0,	8.0,	0.5 },
	{ "filter_curcutoff", 	"Filter Freq",	0.0,	20000,	0.25 },
	{ "env_attack", 		"Attack",		0.0,	3.0,	0.25 },
	{ "env_decay", 			"Decay",		0.0,	3.0,	0.25 },
	{ "env_sustain", 		"Sustain",		0.0,	1.0,	0.5 },
	{ "env_release", 		"Release",		0.0,	4.0,	0.5 },
	{ "filter_minfreq", 	"Filter Min",	0.0,	20000,	0.1 },
	{ "filter_maxfreq", 	"Filter Max",	0.0,	20000,	1.0 },
	{ "oversampling", 		"Oversampling",	0.0,	16.0,	0.5 },
};

void wolp::loaddefaultparams()
{
	for(int i= 0; i<param_size; i++) params[i]= paraminfos[i].defval;
	params[oversampling]= 0;
	setParameter(oversampling, paraminfos[oversampling].defval);
}


wolp::wolp()
{
//	jassert macro seems to broken / always disabled
//	jassert(sizeof(paraminfos)/sizeof(paraminfos[0])==param_size);

	if( sizeof(paraminfos)/sizeof(paraminfos[0]) != param_size )
	{
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, String("Wolpertinger"), "Paraminfos size mismatch");
	}

	loaddefaultparams();

//	for(int i= 0; i<16 ; i++)
//		addVoice(new wolpVoice<8>(this));

	addSound(new wolpSound());

	setNoteStealingEnabled(true);
}

wolp::~wolp()
{
}


void wolp::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	float **outbuf= buffer.getArrayOfWritePointers();
	int size= buffer.getNumSamples();

	memset(outbuf[0], 0, size*sizeof(float));
	memset(outbuf[1], 0, size*sizeof(float));

	renderNextBlock(buffer, midiMessages, 0, size);

	float clp= getparam(clip);
	float *out0= outbuf[0], *out1= outbuf[1];
	for(int i= 0; i<size; i++)
	{
		if(out0[i]<-clp) out0[i]= -clp; else if(out0[i]>clp) out0[i]= clp;
		if(out1[i]<-clp) out1[i]= -clp; else if(out1[i]>clp) out1[i]= clp;
	}


#if ! JUCE_AUDIOPROCESSOR_NO_GUI
	tabbed_editor *e= (tabbed_editor*)getActiveEditor();
	if(e)
	{
		for(int i= 0; i<param_size; i++)
		{
			if(paraminfos[i].dirty)
			{
				sendChangeMessage();
				paraminfos[i].dirty= false;
			}
		}

		int nPoly= 0;
		for(int i= voices.size(); --i>=0; )
		{
			wolpVoice<8> *voice= (wolpVoice<8>*)voices.getUnchecked(i);
			if(voice->getCurrentlyPlayingNote()>=0) nPoly++;
		}

//	harmful in process callback?
//		e->setPolyText(String("Poly: ") + String(nPoly));
	}
#endif
}


void wolp::renderNextBlock (AudioSampleBuffer& outputBuffer,
                                   const MidiBuffer& midiData,
                                   int startSample,
                                   int numSamples)
{
    const ScopedLock sl (lock);
	isProcessing= true;

    MidiBuffer::Iterator midiIterator (midiData);
    midiIterator.setNextSamplePosition (startSample);
    MidiMessage m (0xf4, 0.0);

    while (numSamples > 0)
    {
        int midiEventPos;
        const bool useEvent = midiIterator.getNextEvent (m, midiEventPos)
                                && midiEventPos < startSample + numSamples;

        const int numThisTime = useEvent ? midiEventPos - startSample
                                         : numSamples;

        if (numThisTime > 0)
        {
            for (int i = voices.size(); --i >= 0;)
				voices.getUnchecked (i)->renderNextBlock (outputBuffer, startSample, numThisTime);
        }

        if (useEvent)
        {
            if (m.isNoteOn())
            {
                const int channel = m.getChannel();

                noteOn (channel,
                        m.getNoteNumber(),
                        m.getFloatVelocity());
            }
            else if (m.isNoteOff())
            {
                noteOff (m.getChannel(),
                         m.getNoteNumber(),
                         m.getFloatVelocity(),
                         true);
            }
            else if (m.isAllNotesOff() || m.isAllSoundOff())
            {
                allNotesOff (m.getChannel(), true);
            }
            else if (m.isPitchWheel())
            {
                const int channel = m.getChannel();
                const int wheelPos = m.getPitchWheelValue();
                lastPitchWheelValues [channel - 1] = wheelPos;

                handlePitchWheel (channel, wheelPos);
            }
            else if (m.isController())
            {
                handleController (m.getChannel(),
                                  m.getControllerNumber(),
                                  m.getControllerValue());
                printf("controller: %s\n", MidiMessage::getControllerName(m.getControllerNumber()));
            }
            else if(m.isAftertouch())
            {
				int chan= m.getChannel();
				int note= m.getNoteNumber();
				for(int i= voices.size(); --i>=0; )
				{
					wolpVoice<8> *v= (wolpVoice<8> *)voices.getUnchecked(i);
					if(v->isPlayingChannel(chan) && v->getCurrentlyPlayingNote()==note)
					{
						v->setvolume(m.getAfterTouchValue()*(1.0/0x7F));
					}
				}
            }

            else
                puts("unknown message");
        }

        startSample += numThisTime;
        numSamples -= numThisTime;
    }

	isProcessing= false;
}


#if ! JUCE_AUDIOPROCESSOR_NO_GUI
AudioProcessorEditor* wolp::createEditor()
{
	tabbed_editor *e= new tabbed_editor(this);

	return e;
}
#endif

