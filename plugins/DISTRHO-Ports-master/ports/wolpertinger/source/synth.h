#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h>
#include <vector>
#include "juce_PluginHeaders.h"
#include "filters.h"
#include "ADSRenv.h"
#include "simd.h"


template<typename T> T sqr(T v) { return v*v; }


inline uint64_t rdtsc()
{
	union
	{
		uint64_t val;
		struct { uint32_t lo; uint32_t hi; };
	} ret;
	asm ("rdtsc\n": "=a" (ret.lo), "=d" (ret.hi));
	return ret.val;
}


class wolpSound: public SynthesiserSound
{
	public:
		bool appliesToNote(const int midiNoteNumber) override { return true; }
		bool appliesToChannel(const int midiChannel) override { return true; }
};


class chebyshev_7pole_generated
{
	/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
	   Command line: /www/usr/fisher/helpers/mkfilter -Ch -5.0000000000e-02 -Lp -o 7 -a 2.2786458333e-02 0.0000000000e+00 -l */

	#define NZEROS 7
	#define NPOLES 7
	#define GAIN   8.185223519e+08

	public:

	chebyshev_7pole_generated() { memset(xv, 0, sizeof(xv)); memset(yv, 0, sizeof(yv)); }

	double xv[NZEROS+1], yv[NPOLES+1];

	double run(double nextvalue)
	{
		xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7];
		xv[7] = nextvalue / GAIN;
		yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7];
		yv[7] =   (xv[0] + xv[7]) + 7 * (xv[1] + xv[6]) + 21 * (xv[2] + xv[5])
					 + 35 * (xv[3] + xv[4])
					 + (  0.7582559865 * yv[0]) + ( -5.4896632588 * yv[1])
					 + ( 17.0657116090 * yv[2]) + (-29.5302444680 * yv[3])
					 + ( 30.7191651920 * yv[4]) + (-19.2116577140 * yv[5])
					 + (  6.6884324971 * yv[6]);
		return yv[7];
	}

	#undef NZEROS
	#undef NPOLES
	#undef GAIN
};

class chebyshev_7pole
{
	/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
	   Command line: /www/usr/fisher/helpers/mkfilter -Ch -5.0000000000e-02 -Lp -o 7 -a 2.2786458333e-02 0.0000000000e+00 -l */

	#define NZEROS 7
	#define NPOLES 7
	#define GAIN   3.749197811e+07	//8.185223519e+08
	#define IGAIN  (1.0/GAIN)

	public:

	uint32_t index;
	chebyshev_7pole(): index(0) { memset(xv, 0, sizeof(xv)); memset(yv, 0, sizeof(yv)); }

	double xv[NZEROS+1], yv[NPOLES+1];

	double run(double nextvalue)
	{
#if 0
		index++;
		xv[index+7&7]= nextvalue*IGAIN;
		yv[index+7&7]= (xv[index+0&7] + xv[index+7&7]) + 7 * (xv[index+1&7] + xv[index+6&7]) + 21 * (xv[index+2&7] + xv[index+5&7])
					 + 35 * (xv[index+3&7] + xv[index+4&7])
					 + (  0.7582559865 * yv[index+0&7]) + ( -5.4896632588 * yv[index+1&7])
					 + ( 17.0657116090 * yv[index+2&7]) + (-29.5302444680 * yv[index+3&7])
					 + ( 30.7191651920 * yv[index+4&7]) + (-19.2116577140 * yv[index+5&7])
					 + (  6.6884324971 * yv[index+6&7]);
		return yv[index+7&7];
#else
		xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7];
		xv[7] = nextvalue * IGAIN;
		yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7];
        yv[7] =   (xv[0] + xv[7]) + 7 * (xv[1] + xv[6]) + 21 * (xv[2] + xv[5])
                     + 35 * (xv[3] + xv[4])
                     + (  0.5583379245 * yv[0]) + ( -4.2029878877 * yv[1])
                     + ( 13.6048706560 * yv[2]) + (-24.5500454730 * yv[3])
                     + ( 26.6748933120 * yv[4]) + (-17.4541474870 * yv[5])
                     + (  6.3690755413 * yv[6]);
//		yv[7] =   (xv[0] + xv[7]) + 7 * (xv[1] + xv[6]) + 21 * (xv[2] + xv[5])
//					 + 35 * (xv[3] + xv[4])
//					 + (  0.7582559865 * yv[0]) + ( -5.4896632588 * yv[1])
//					 + ( 17.0657116090 * yv[2]) + (-29.5302444680 * yv[3])
//					 + ( 30.7191651920 * yv[4]) + (-19.2116577140 * yv[5])
//					 + (  6.6884324971 * yv[6]);
		return yv[7];
#endif
	}

	#undef NZEROS
	#undef NPOLES
	#undef GAIN
};

class chebyshev_3pole
{
	/*
		Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
		parameters:

		filtertype 	= 	Chebyshev
		passtype 	= 	Lowpass
		ripple 		= 	-1
		order 		= 	3
		samplerate 	= 	768000
		corner1 	= 	18000
		corner2 	=
		adzero 		=
		logmin 		=
	*/

	#define NZEROS 3
	#define NPOLES 3
	#define GAIN   5.476029231e+03
	#define IGAIN  (1.0/GAIN)

	public:
		double xv[NZEROS+1]; double yv[NPOLES+1];

		chebyshev_3pole() { memset(xv, 0, sizeof(xv)); memset(yv, 0, sizeof(yv)); }

		double run(double nextvalue)
		{
			xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3];
			xv[3] = nextvalue * IGAIN;
			yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3];
			yv[3] =   (xv[0] + xv[3]) + 3 * (xv[1] + xv[2])
						 + (  0.8646065740 * yv[0]) + ( -2.7049827914 * yv[1])
						 + (  2.8389153049 * yv[2]);
			return yv[3];
		}
	#undef NZEROS
	#undef NPOLES
	#undef IGAIN
};


template<int oversampling>
class chebyshev_downsampling_lp
{
	public:
		double xv[3+1]; double yv[3+1];

		chebyshev_downsampling_lp() { memset(xv, 0, sizeof(xv)); memset(yv, 0, sizeof(yv)); }

		double run(double nextvalue);
};

template<>
inline double chebyshev_downsampling_lp<16>::run(double nextvalue)
{
	/*
		mkfilter/mkshape/gencode   A.J. Fisher
		parameters:

		filtertype 	= 	Chebyshev
		passtype 	= 	Lowpass
		ripple 		= 	-1
		order 		= 	3
		samplerate 	= 	768000
		corner1 	= 	18000
	*/
	xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3];
	xv[3] = nextvalue * (1.0 / 5.476029231e+03);
	yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3];
	yv[3] = (xv[0] + xv[3]) + 3 * (xv[1] + xv[2])
			+ (  0.8646065740 * yv[0]) + ( -2.7049827914 * yv[1])
			+ (  2.8389153049 * yv[2]);
	return yv[3];
}

template<>
inline double chebyshev_downsampling_lp<8>::run(double nextvalue)
{
	/*
		mkfilter/mkshape/gencode   A.J. Fisher
		parameters:

		filtertype 	= 	Chebyshev
		passtype 	= 	Lowpass
		ripple 		= 	-1
		order 		= 	3
		samplerate 	= 	768000
		corner1 	= 	18000
	*/
	xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3];
	xv[3] = nextvalue * (1.0 / 7.330193589e+02);
	yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3];
	yv[3] = (xv[0] + xv[3]) + 3 * (xv[1] + xv[2])
			+ (  0.7478260267 * yv[0]) + ( -2.4083812307 * yv[1])
			+ (  2.6496414404 * yv[2]);
	return yv[3];
}

template<>
inline double chebyshev_downsampling_lp<1>::run(double nextvalue)
{
	return nextvalue;
}


template<int oversampling> class WaveGenerator
{
	public:
		WaveGenerator():
			phase(0.0), cyclecount(0)
		{ }

		~WaveGenerator()
		{ }

		double getNextSample()
		{
			double s= 0;
			for(int i= 0; i<oversampling; i++)
			{
				if(oversampling>1)
					s= chebyshev_lp.run(getNextRawSample());
				else
					s= getNextRawSample();
			}
			return s;
		}

		void setFrequency(double sampleRate, double noteFrequency)
		{
			sampleStep= noteFrequency / (sampleRate*oversampling);
		}

		void setMultipliers(double mSaw, double mRect, double mTri)
		{
			float div= mSaw+mRect+mTri;
			if(div==0.0f) mSaw= div= 1.0;
			div= 1.0/div;
			sawFactor= mSaw*div;
			rectFactor= mRect*div;
			triFactor= mTri*div;
		}

		float *generateSamples(int nSamples)
		{
			if(int(sampleBuffer.size())<nSamples)
				sampleBuffer.resize(nSamples);
			double s;
			for(int i= 0; i<nSamples; i++)
			{
				for(int k= oversampling; k; k--)
					s= chebyshev_lp.run(getNextRawSample());
				sampleBuffer[i]= s;
			}
			return &sampleBuffer[0];
		}

	private:
		double sawFactor, rectFactor, triFactor, sampleStep, phase;
		int cyclecount;
		std::vector<float> rawSampleBuffer;
		std::vector<float> sampleBuffer;
		chebyshev_downsampling_lp<oversampling> chebyshev_lp;

		void generateRawSampleChunk(float *buffer, int nSamples)
		{
			double saw, rect, tri;
			for(int i= 0; i<nSamples; i++)
			{
				saw= phase;
				rect= (phase<0.5? -1: 1);
				tri= (cyclecount&1? 2-(phase+1)-1: phase);
				phase+= sampleStep;
				buffer[i]= saw*sawFactor + rect*rectFactor + tri*triFactor;
			}
		}

		void generateRawSamples(float *buffer, int nSamples)
		{
			while(phase>1.0) phase-= 2.0;
			for(int i= 0; i<nSamples; )
			{
				int chunkSize= (2 - (phase+1)) / sampleStep;
				if(chunkSize>nSamples-i) chunkSize= nSamples-i;
				generateRawSampleChunk(buffer+i, chunkSize);
				i+= chunkSize;
				if(i<nSamples) phase-= 2, cyclecount++;
			}
		}

		double getNextRawSample()
		{
			double saw= phase,
				   rect= (phase<0.5? -1: 1),
				   tri= (cyclecount&1? 2-(phase+1)-1: phase);

			double val= saw*sawFactor + rect*rectFactor + tri*triFactor;

			phase+= sampleStep;
			if (phase > 1)
				cyclecount++,
				phase -= 2;

			return val;
		}
};

template<int oversampling> class wolpVoice: public SynthesiserVoice
{
	public:
		wolpVoice(class wolp *s);

		//==============================================================================
		bool canPlaySound (SynthesiserSound* sound) override { return true; }

		void startNote (const int midiNoteNumber,
						const float velocity,
						SynthesiserSound* sound,
						const int currentPitchWheelPosition) override;

		void stopNote (float, const bool allowTailOff) override;

		void pitchWheelMoved (const int newValue) override { }

		void controllerMoved (const int controllerNumber,
							  const int newValue) override { }

		//==============================================================================
		void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override;

		void setvolume(double v) { vol= v; }
		void setfreq(double f) { freq= f; }

	protected:
		void process(float* p1, float* p2, int samples);

		double phase, low, band, high, vol, freq;
		bool playing;
		int cyclecount;
		unsigned long samples_synthesized;

		WaveGenerator<oversampling> generator;
		bandpass<8> filter;
		ADSRenv env;

		class wolp *synth;
};



class wolp:	public AudioProcessor,
            public Synthesiser,
            public ChangeBroadcaster,
            public MidiKeyboardStateListener
{
	public:
		enum params
		{
			gain= 0,
			clip,
			gsaw,
			grect,
			gtri,
			tune,
			cutoff,
			resonance,
			bandwidth,
			velocity,
			inertia,
			nfilters,
			curcutoff,
			attack,
			decay,
			sustain,
			release,
			filtermin,
			filtermax,
			oversampling,
			param_size
		};

		struct paraminfo
		{
			const char *internalname;
			const char *label;
			double min, max, defval;
			bool dirty;		// parameter has changed, GUI must be updated
		};
		static paraminfo paraminfos[param_size];


		wolp();
		~wolp() override;

                bool silenceInProducesSilenceOut() const override { return false; }
                double getTailLengthSeconds() const override { return 0.0; }

		const String getName() const override { return "Wolpertinger"; }
		void prepareToPlay (double sampleRate, int estimatedSamplesPerBlock) override
		{
			setCurrentPlaybackSampleRate(sampleRate);
			for(int i= 0; i<getNumVoices(); i++)
				getVoice(i)->setCurrentPlaybackSampleRate(sampleRate);
		}
		void releaseResources() override { }
		void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;
		const String getInputChannelName (const int channelIndex) const override { return String("In") + String(channelIndex); }
		const String getOutputChannelName (const int channelIndex) const override { return String("Out") + String(channelIndex); }
		bool isInputChannelStereoPair (int index) const override { return true; }
		bool isOutputChannelStereoPair (int index) const override { return true; }
		bool acceptsMidi() const override  { return true; }
		bool producesMidi() const override { return false; }
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
                bool hasEditor() const override { return true; }
		AudioProcessorEditor* createEditor() override;
#endif
		int getNumParameters() override { return param_size; }
		const String getParameterName (int idx) override { return String(paraminfos[idx].label); }
		float getParameter (int idx) override
		{
			return params[idx];
		}
		const String getParameterText (int idx) override
		{
			switch(idx)
			{
				default:
					return String(getparam(idx), 2);
			}
		}
		void setParameter (int idx, float value) override;

		//==============================================================================
		int getNumPrograms() override { return 1; };
		int getCurrentProgram() override { return 0; };
		void setCurrentProgram (int index) override { };
		const String getProgramName (int index) override { return "Default"; };
		void changeProgramName (int index, const String& newName) override {}

		//==============================================================================
		void getStateInformation (JUCE_NAMESPACE::MemoryBlock& destData) override;
		void setStateInformation (const void* data, int sizeInBytes) override;

		//==============================================================================
		double getnotefreq (int noteNumber)
		{
			noteNumber -= 12 * 6 + 9; // now 0 = A440
			return getparam(tune) * pow (2.0, noteNumber / 12.0);
		}

		float getparam(int idx);

        void loaddefaultparams();

		void renderNextBlock (AudioSampleBuffer& outputAudio,
							  const MidiBuffer& inputMidi,
							  int startSample,
							  int numSamples);

		//==============================================================================
		/** Called when one of the MidiKeyboardState's keys is pressed.

			This will be called synchronously when the state is either processing a
			buffer in its MidiKeyboardState::processNextMidiBuffer() method, or
			when a note is being played with its MidiKeyboardState::noteOn() method.

			Note that this callback could happen from an audio callback thread, so be
			careful not to block, and avoid any UI activity in the callback.
		*/
		virtual void handleNoteOn (MidiKeyboardState* source,
								   int midiChannel, int midiNoteNumber, float velocity) override
		{
//			printf("MidiKeyboard noteOn isProcessing=%s\n", isProcessing? "true": "false");
			noteOn(midiChannel, midiNoteNumber, velocity);
		}

		/** Called when one of the MidiKeyboardState's keys is released.

			This will be called synchronously when the state is either processing a
			buffer in its MidiKeyboardState::processNextMidiBuffer() method, or
			when a note is being played with its MidiKeyboardState::noteOff() method.

			Note that this callback could happen from an audio callback thread, so be
			careful not to block, and avoid any UI activity in the callback.
		*/
		virtual void handleNoteOff (MidiKeyboardState* source,
									int midiChannel, int midiNoteNumber, float velocity) override
		{
//			printf("MidiKeyboard noteOff isProcessing=%s\n", isProcessing? "true": "false");
			noteOff(midiChannel, midiNoteNumber, velocity, true);
		}

	protected:

	private:
		double params[param_size];

		velocityfilter <double> cutoff_filter;

		bool isProcessing;	// whether we are in the processBlock callback

		friend class wolpVoice<1>;
		friend class wolpVoice<8>;
		friend class wolpVoice<16>;
		friend class editor;
};


#endif // SYNTH_H
