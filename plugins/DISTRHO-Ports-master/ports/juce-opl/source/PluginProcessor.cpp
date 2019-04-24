#include "PluginProcessor.h"
#include "PluginGui.h"
#include "EnumFloatParameter.h"
#include "IntFloatParameter.h"
#include "SbiLoader.h"

#include <iterator>

const char *AdlibBlasterAudioProcessor::PROGRAM_INDEX = "Program Index";

//==============================================================================
AdlibBlasterAudioProcessor::AdlibBlasterAudioProcessor()
	: i_program(-1)
{
	// Initalize OPL
	velocity = false;
	Opl = new Hiopl();
	Opl->SetSampleRate(44100);
	Opl->EnableWaveformControl();

	// Initialize parameters

	const String waveforms[] = {"Sine", "Half Sine", "Abs Sine", "Quarter Sine", "Alternating Sine", "Camel Sine", "Square", "Logarithmic Sawtooth"};
	params.push_back(new EnumFloatParameter("Carrier Wave",
		StringArray(waveforms, sizeof(waveforms)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Modulator Wave",
		StringArray(waveforms, sizeof(waveforms)/sizeof(String)))
	);

	const String frq_multipliers[] = {
		"x0.5", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x10", "x12", "x12", "x15", "x15"
	};
	params.push_back(new EnumFloatParameter("Carrier Frequency Multiplier",
		StringArray(frq_multipliers, sizeof(frq_multipliers)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Modulator Frequency Multiplier",
		StringArray(frq_multipliers, sizeof(frq_multipliers)/sizeof(String)))
	);

	const String levels[] = {"0.00 dB", "0.75 dB", "1.50 dB", "2.25 dB", "3.00 dB", "3.75 dB", "4.50 dB", "5.25 dB", "6.00 dB", "6.75 dB", "7.50 dB", "8.25 dB", "9.00 dB", "9.75 dB", "10.50 dB", "11.25 dB", "12.00 dB", "12.75 dB", "13.50 dB", "14.25 dB", "15.00 dB", "15.75 dB", "16.50 dB", "17.25 dB", "18.00 dB", "18.75 dB", "19.50 dB", "20.25 dB", "21.00 dB", "21.75 dB", "22.50 dB", "23.25 dB", "24.00 dB", "24.75 dB", "25.50 dB", "26.25 dB", "27.00 dB", "27.75 dB", "28.50 dB", "29.25 dB", "30.00 dB", "30.75 dB", "31.50 dB", "32.25 dB", "33.00 dB", "33.75 dB", "34.50 dB", "35.25 dB", "36.00 dB", "36.75 dB", "37.50 dB", "38.25 dB", "39.00 dB", "39.75 dB", "40.50 dB", "41.25 dB", "42.00 dB", "42.75 dB", "43.50 dB", "44.25 dB", "45.00 dB", "45.75 dB", "46.50 dB", "47.25 dB"};
	params.push_back(new EnumFloatParameter("Carrier Attenuation",
		StringArray(levels, sizeof(levels)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Modulator Attenuation",
		StringArray(levels, sizeof(levels)/sizeof(String)))
	);

	const String depth[] = {"Light", "Heavy"};
	params.push_back(new EnumFloatParameter("Tremolo Depth",
		StringArray(depth, sizeof(depth)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Vibrato Depth",
		StringArray(depth, sizeof(depth)/sizeof(String)))
	);

	const String onoff[] = {"Disable", "Enable"};
	params.push_back(new EnumFloatParameter("Carrier Tremolo",
		StringArray(onoff, sizeof(onoff)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Carrier Vibrato",
		StringArray(onoff, sizeof(onoff)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Carrier Sustain",
		StringArray(onoff, sizeof(onoff)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Carrier Keyscale Rate",
		StringArray(onoff, sizeof(onoff)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Modulator Tremolo",
		StringArray(onoff, sizeof(onoff)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Modulator Vibrato",
		StringArray(onoff, sizeof(onoff)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Modulator Sustain",
		StringArray(onoff, sizeof(onoff)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Modulator Keyscale Rate",
		StringArray(onoff, sizeof(onoff)/sizeof(String)))
	);

	const String ksrs[] = {"None","1.5 dB/8ve","3 dB/8ve","6 dB/8ve"};
	params.push_back(new EnumFloatParameter("Carrier Keyscale Level",
		StringArray(ksrs, sizeof(ksrs)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Modulator Keyscale Level",
		StringArray(ksrs, sizeof(ksrs)/sizeof(String)))
	);

	const String algos[] = {"Frequency Modulation", "Additive"};
	params.push_back(new EnumFloatParameter("Algorithm",
		StringArray(algos, sizeof(algos)/sizeof(String)))
	);

	params.push_back(new IntFloatParameter("Modulator Feedback", 0, 7));
	params.push_back(new IntFloatParameter("Carrier Attack", 0, 15));
	params.push_back(new IntFloatParameter("Carrier Decay", 0, 15));
	params.push_back(new IntFloatParameter("Carrier Sustain Level", 0, 15));
	params.push_back(new IntFloatParameter("Carrier Release", 0, 15));
	params.push_back(new IntFloatParameter("Modulator Attack", 0, 15));
	params.push_back(new IntFloatParameter("Modulator Decay", 0, 15));
	params.push_back(new IntFloatParameter("Modulator Sustain Level", 0, 15));
	params.push_back(new IntFloatParameter("Modulator Release", 0, 15));

	const String sensitivitySettings[] = {"None", "Low", "High"};
	params.push_back(new EnumFloatParameter("Carrier Velocity Sensitivity",
		StringArray(sensitivitySettings, sizeof(sensitivitySettings)/sizeof(String)))
	);
	params.push_back(new EnumFloatParameter("Modulator Velocity Sensitivity",
		StringArray(sensitivitySettings, sizeof(sensitivitySettings)/sizeof(String)))
	);

	const String emulators[] = {"DOSBox", "ZDoom"};
	params.push_back(new EnumFloatParameter("Emulator",
		StringArray(emulators, sizeof(emulators)/sizeof(String)))
	);

	const String percussion[] = { "Off", "Bass drum", "Snare", "Tom", "Cymbal", "Hi-hat" };
	params.push_back(new EnumFloatParameter("Percussion Mode",
		StringArray(percussion, sizeof(percussion) / sizeof(String)))
	);

	for(unsigned int i = 0; i < params.size(); i++) {
		paramIdxByName[params[i]->getName()] = i;
	}

	initPrograms();

	for(std::map<String,std::vector<float>>::iterator it = programs.begin(); it != programs.end(); ++it) {
		program_order.push_back(it->first);
	}
	
	setCurrentProgram(0);
	for (int i = 0; i < Hiopl::CHANNELS+1; i++) {
		active_notes[i] = NO_NOTE;
		channel_enabled[i] = true;
	}
	currentScaledBend = 1.0f;

	for (int i = 1; i <= Hiopl::CHANNELS; ++i)
		available_channels.push_back(i);
}

void AdlibBlasterAudioProcessor::initPrograms()
{
	// these ones from the Syndicate in-game music
    const float i_params_0[] = {
        0.000000f, 0.330000f,  // waveforms
        0.066667f, 0.133333f,  // frq multipliers
        0.142857f, 0.412698f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 1.0f, 0.0f,  // tre / vib / sus / ks
        0.0f, 0.0f, 1.0f, 1.0f,  // tre / vib / sus / ks
        0.000000f, 0.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.000000f,            // feedback
        0.5f, 0.3f, 0.3f, 0.3f,  // adsr
        0.5f, 0.3f, 0.1f, 0.6f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_0 (i_params_0, i_params_0 + sizeof(i_params_0) / sizeof(float));
    programs["Mercenary Bass"] = std::vector<float>(v_i_params_0);

    const float i_params_19189[] = {
        0.000000f, 0.000000f,  // waveforms
        0.066667f, 0.200000f,  // frq multipliers
        0.000000f, 0.285714f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 0.0f, 1.0f,  // tre / vib / sus / ks
        0.0f, 0.0f, 0.0f, 1.0f,  // tre / vib / sus / ks
        0.000000f, 0.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.571429f,            // feedback
        1.0f, 1.0f, 0.0f, 0.3f,  // adsr
        1.0f, 0.5f, 0.2f, 0.3f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_19189 (i_params_19189, i_params_19189 + sizeof(i_params_19189) / sizeof(float));
    programs["Patrol Bass"] = std::vector<float>(v_i_params_19189);

    const float i_params_38377[] = {
		0.000000f, 0.160000f,  // waveforms
        0.066667f, 0.066667f,  // frq multipliers
        0.000000f, 0.460317f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 0.0f, 0.0f,  // tre / vib / sus / ks
        0.0f, 0.0f, 0.0f, 0.0f,  // tre / vib / sus / ks
        0.000000f, 0.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.000000f,            // feedback
        1.0f, 0.3f, 0.5f, 0.5f,  // adsr
        1.0f, 0.1f, 0.9f, 1.0f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_38377 (i_params_38377, i_params_38377 + sizeof(i_params_38377) / sizeof(float));
    programs["Subdue Bass"] = std::vector<float>(v_i_params_38377);

    const float i_params_38392[] = {
        0.000000f, 0.000000f,  // waveforms
        0.000000f, 0.000000f,  // frq multipliers
        0.000000f, 0.000000f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 1.0f, 0.0f,  // tre / vib / sus / ks
        0.0f, 0.0f, 0.0f, 0.0f,  // tre / vib / sus / ks
        0.000000f, 0.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.000000f,            // feedback
        0.1f, 0.1f, 0.7f, 0.1f,  // adsr
        0.1f, 0.9f, 0.1f, 0.1f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_38392 (i_params_38392, i_params_38392 + sizeof(i_params_38392) / sizeof(float));
    programs["Dark Future Sweep"] = std::vector<float>(v_i_params_38392);

    const float i_params_39687[] = {
        0.000000f, 0.000000f,  // waveforms
        0.066667f, 0.333333f,  // frq multipliers
        0.000000f, 0.301587f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 0.0f, 0.0f,  // tre / vib / sus / ks
        0.0f, 0.0f, 0.0f, 1.0f,  // tre / vib / sus / ks
        0.000000f, 1.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.571429f,            // feedback
        1.0f, 0.3f, 0.1f, 0.3f,  // adsr
        1.0f, 0.7f, 0.0f, 0.4f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_39687 (i_params_39687, i_params_39687 + sizeof(i_params_39687) / sizeof(float));
    programs["Sinister Bass"] = std::vector<float>(v_i_params_39687);

    const float i_params_76784[] = {
        0.000000f, 0.330000f,  // waveforms
        0.066667f, 0.133333f,  // frq multipliers
        0.000000f, 0.428571f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 1.0f, 0.0f,  // tre / vib / sus / ks
        0.0f, 0.0f, 0.0f, 0.0f,  // tre / vib / sus / ks
        0.000000f, 0.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.000000f,            // feedback
        1.0f, 0.3f, 0.4f, 0.4f,  // adsr
        1.0f, 0.4f, 0.5f, 0.3f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_76784 (i_params_76784, i_params_76784 + sizeof(i_params_76784) / sizeof(float));
    programs["Buzcut Bass"] = std::vector<float>(v_i_params_76784);

    const float i_params_97283[] = {
        0.000000f, 0.330000f,  // waveforms
        0.133333f, 0.400000f,  // frq multipliers
        0.000000f, 0.365079f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 0.0f, 1.0f,  // tre / vib / sus / ks
        0.0f, 0.0f, 0.0f, 1.0f,  // tre / vib / sus / ks
        0.000000f, 0.660000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.000000f,            // feedback
        0.6f, 0.7f, 0.0f, 0.2f,  // adsr
        0.6f, 0.7f, 0.1f, 0.1f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_97283 (i_params_97283, i_params_97283 + sizeof(i_params_97283) / sizeof(float));
    programs["Death Toll Bell"] = std::vector<float>(v_i_params_97283);

	// The start of the Dune 2 introduction
    const float i_params_3136[] = {
        0.000000f, 0.330000f,  // waveforms
        0.133333f, 0.133333f,  // frq multipliers
        0.000000f, 0.333333f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 0.0f, 0.0f,  // tre / vib / sus / ks
        0.0f, 0.0f, 0.0f, 0.0f,  // tre / vib / sus / ks
        0.000000f, 0.330000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.571429f,            // feedback
        1.0f, 0.1f, 0.1f, 0.3f,  // adsr
        1.0f, 0.4f, 0.2f, 0.3f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_3136 (i_params_3136, i_params_3136 + sizeof(i_params_3136) / sizeof(float));
    programs["Westwood Chime"] = std::vector<float>(v_i_params_3136);

    const float i_params_7254[] = {
        0.000000f, 0.160000f,  // waveforms
        0.066667f, 0.066667f,  // frq multipliers
        0.253968f, 0.476190f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        1.0f, 1.0f, 1.0f, 1.0f,  // tre / vib / sus / ks
        1.0f, 1.0f, 0.0f, 0.0f,  // tre / vib / sus / ks
        0.000000f, 0.330000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.571429f,            // feedback
        0.1f, 0.1f, 0.1f, 0.1f,  // adsr
        0.2f, 0.1f, 0.1f, 0.0f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_7254 (i_params_7254, i_params_7254 + sizeof(i_params_7254) / sizeof(float));
    programs["Desert Pipe"] = std::vector<float>(v_i_params_7254);

    const float i_params_20108[] = {
        0.000000f, 0.000000f,  // waveforms
        0.400000f, 0.066667f,  // frq multipliers
        0.238095f, 0.000000f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        1.0f, 1.0f, 1.0f, 0.0f,  // tre / vib / sus / ks
        0.0f, 1.0f, 1.0f, 0.0f,  // tre / vib / sus / ks
        0.000000f, 0.330000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.000000f,            // feedback
        0.1f, 0.1f, 0.1f, 0.1f,  // adsr
        0.1f, 0.1f, 0.1f, 0.1f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_20108 (i_params_20108, i_params_20108 + sizeof(i_params_20108) / sizeof(float));
    programs["Y2180 Strings"] = std::vector<float>(v_i_params_20108);

    const float i_params_27550[] = {
        0.500000f, 0.000000f,  // waveforms
        0.000000f, 0.066667f,  // frq multipliers
        0.238095f, 0.793651f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 1.0f, 0.0f, 0.0f,  // tre / vib / sus / ks
        0.0f, 0.0f, 1.0f, 1.0f,  // tre / vib / sus / ks
        0.000000f, 0.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.571429f,            // feedback
        1.0f, 0.0f, 1.0f, 1.0f,  // adsr
        0.9f, 0.1f, 0.0f, 1.0f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_27550 (i_params_27550, i_params_27550 + sizeof(i_params_27550) / sizeof(float));
    programs["Emperor Chord"] = std::vector<float>(v_i_params_27550);

    const float i_params_harpsi[] = {
        0.330000f, 0.160000f,  // waveforms
        0.066667f, 0.200000f,  // frq multipliers
        0.142857f, 0.260000f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 1.0f, 0.0f,  // tre / vib / sus / ks
        0.0f, 1.0f, 1.0f, 1.0f,  // tre / vib / sus / ks
        0.000000f, 0.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.000000f,            // feedback
        0.85f, 0.3f, 0.3f, 0.3f,  // adsr
        0.85f, 0.3f, 0.1f, 0.6f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_harpsi (i_params_harpsi, i_params_harpsi + sizeof(i_params_harpsi) / sizeof(float));
    programs["Harpsi"] = std::vector<float>(v_i_params_harpsi);

    const float i_params_tromba[] = {
        0.000000f, 0.160000f,  // waveforms
        0.066667f, 0.000000f,  // frq multipliers
        0.142857f, 0.220000f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        0.0f, 0.0f, 1.0f, 0.0f,  // tre / vib / sus / ks
        1.0f, 0.0f, 1.0f, 1.0f,  // tre / vib / sus / ks
        0.000000f, 0.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.500000f,            // feedback
        0.45f, 0.3f, 0.3f, 0.3f,  // adsr
        0.45f, 0.45f, 0.1f, 0.6f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_tromba (i_params_tromba, i_params_tromba + sizeof(i_params_tromba) / sizeof(float));
    programs["Tromba"] = std::vector<float>(v_i_params_tromba);

    const float i_params_bassdrum[] = {
        0.000000f, 0.500000f,  // waveforms
        0.000000f, 0.000000f,  // frq multipliers
        0.000000f, 0.090000f,  // attenuation
		0.000000f, 0.000000f,  // tremolo / vibrato depth
        1.0f, 1.0f, 1.0f, 0.0f,  // tre / vib / sus / ks
        1.0f, 1.0f, 1.0f, 1.0f,  // tre / vib / sus / ks
        0.000000f, 0.000000f,  // KSR/8ve
        0.000000f,            // algorithm
        0.500000f,            // feedback
        1.00f, 0.5f, 0.3f, 0.4f,  // adsr
        1.00f, 0.75f, 0.5f, 0.5f,  // adsr
		0.0f, 0.0f,				 // velocity sensitivity
		0.0f,					// emulator
		0.0f,					// percussion mode
    };
    std::vector<float> v_i_params_bassdrum (i_params_bassdrum, i_params_bassdrum + sizeof(i_params_bassdrum) / sizeof(float));
    programs["bassdrum"] = std::vector<float>(v_i_params_bassdrum);

}

void AdlibBlasterAudioProcessor::applyPitchBend()
{   // apply the currently configured pitch bend to all active notes.
	for (int i = 1; i <= Hiopl::CHANNELS; i++) {
		if (NO_NOTE != active_notes[i]) {
			float f = (float)MidiMessage::getMidiNoteInHertz(active_notes[i]);
			f *= currentScaledBend;
			Opl->SetFrequency(i, f);
		}
	}
}

AdlibBlasterAudioProcessor::~AdlibBlasterAudioProcessor()
{
	for (unsigned int i=0; i < params.size(); ++i)
		delete params[i];
	delete Opl;
}

//==============================================================================
const String AdlibBlasterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int AdlibBlasterAudioProcessor::getNumParameters()
{
	return (int)params.size();
}

float AdlibBlasterAudioProcessor::getParameter (int index)
{
    return params[index]->getParameter();
}

void AdlibBlasterAudioProcessor::setIntParameter (String name, int value)
{
	int i = paramIdxByName[name];
	IntFloatParameter* p = (IntFloatParameter*)params[i];
	p->setParameterValue(value);
	setParameter(i, p->getParameter());
}

void AdlibBlasterAudioProcessor::setEnumParameter (String name, int index)
{
	int i = paramIdxByName[name];
	EnumFloatParameter* p = (EnumFloatParameter*)params[i];
	p->setParameterIndex(index);
	setParameter(i, p->getParameter());
}

int AdlibBlasterAudioProcessor::getIntParameter (String name)
{
	int i = paramIdxByName[name];
	IntFloatParameter* p = (IntFloatParameter*)params[i];
	return p->getParameterValue();
}

int AdlibBlasterAudioProcessor::getEnumParameter (String name)
{
	int i = paramIdxByName[name];
	EnumFloatParameter* p = (EnumFloatParameter*)params[i];
	return p->getParameterIndex();
}

bool AdlibBlasterAudioProcessor::getBoolParameter(String name)
{
	return 0 != getEnumParameter(name);
}

// Parameters which apply directly to the OPL
void AdlibBlasterAudioProcessor::setParameter (int index, float newValue)
{
	FloatParameter* p = params[index];
	p->setParameter(newValue);
	String name = p->getName();
	int osc = 2;	// Carrier
	if (name.startsWith("Modulator")) {
		osc = 1;
	}
	if (name.endsWith("Wave")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->SetWaveform(c, osc, (Waveform)((EnumFloatParameter*)p)->getParameterIndex());
	} else if (name.endsWith("Attenuation")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->SetAttenuation(c, osc, ((EnumFloatParameter*)p)->getParameterIndex());
	} else if (name.endsWith("Frequency Multiplier")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->SetFrequencyMultiple(c, osc, (FreqMultiple)((EnumFloatParameter*)p)->getParameterIndex());
	} else if (name.endsWith("Attack")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->SetEnvelopeAttack(c, osc, ((IntFloatParameter*)p)->getParameterValue());
	} else if (name.endsWith("Decay")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->SetEnvelopeDecay(c, osc, ((IntFloatParameter*)p)->getParameterValue());
	} else if (name.endsWith("Sustain Level")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->SetEnvelopeSustain(c, osc, ((IntFloatParameter*)p)->getParameterValue());
	} else if (name.endsWith("Release")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->SetEnvelopeRelease(c, osc, ((IntFloatParameter*)p)->getParameterValue());
	} else if (name.endsWith("Feedback")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->SetModulatorFeedback(c, ((IntFloatParameter*)p)->getParameterValue());
	} else if (name.endsWith("Keyscale Level")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->SetKsl(c, osc, ((EnumFloatParameter*)p)->getParameterIndex());
	} else if (name.endsWith("Keyscale Rate")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->EnableKsr(c, osc, ((EnumFloatParameter*)p)->getParameterIndex() > 0);
	} else if (name.endsWith("Sustain")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->EnableSustain(c, osc, ((EnumFloatParameter*)p)->getParameterIndex() > 0);
	} else if (name.endsWith("Tremolo")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->EnableTremolo(c, osc, ((EnumFloatParameter*)p)->getParameterIndex() > 0);
	} else if (name.endsWith("Vibrato")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->EnableVibrato(c, osc, ((EnumFloatParameter*)p)->getParameterIndex() > 0);
	} else if (name.endsWith("Algorithm")) {
		for(int c=1;c<=Hiopl::CHANNELS;c++) Opl->EnableAdditiveSynthesis(c, ((EnumFloatParameter*)p)->getParameterIndex() > 0);
	} else if (name.startsWith("Tremolo Depth")) {
		Opl->TremoloDepth(((EnumFloatParameter*)p)->getParameterIndex() > 0);
	} else if (name.startsWith("Vibrato Depth")) {
		Opl->VibratoDepth(((EnumFloatParameter*)p)->getParameterIndex() > 0);
	} else if (name.startsWith("Emulator")) {
		Opl->SetEmulator((Emulator)((EnumFloatParameter*)p)->getParameterIndex());
	} else if (name.startsWith("Percussion")) {
		Opl->SetPercussionMode(((EnumFloatParameter*)p)->getParameterIndex() > 0);
	}
}

void AdlibBlasterAudioProcessor::loadInstrumentFromFile(String filename)
{
	FILE* f = fopen(filename.toUTF8(), "rb");
	unsigned char buf[MAX_INSTRUMENT_FILE_SIZE_BYTES];
	int n = (int)fread(buf, 1, MAX_INSTRUMENT_FILE_SIZE_BYTES, f);
	fclose(f);
	SbiLoader* loader = new SbiLoader();
	loader->loadInstrumentData(n, buf, this);
	delete loader;
	updateGuiIfPresent();
}

void AdlibBlasterAudioProcessor::saveInstrumentToFile(String filename)
{
	// http://www.shikadi.net/moddingwiki/SBI_Format
	const Bit32u sbi_registers[] = {
		0x20, 0x23, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83, 0xe0, 0xe3, 0xc0
	};
	FILE* f = fopen(filename.toUTF8(), "wb");
	if (f) {
		fwrite("SBI\x1d", 1, 4, f);
		fwrite("JuceOPLVSTi instrument         \0", 1, 32, f);
		for (int i = 0; i < 11; i++) {
			Bit8u regVal = Opl->_ReadReg(sbi_registers[i]);
			fwrite(&regVal, 1, 1, f);
		}
		fwrite("     ", 1, 5, f);
		fclose(f);
	}
}

// Used to configure parameters from .SBI instrument file
void AdlibBlasterAudioProcessor::setParametersByRegister(int register_base, int op, uint8 value)
{
	const String operators[] = {"Modulator", "Carrier"};
	register_base &= 0xF0;
	switch (register_base) {
	case 0x20:
		setEnumParameter(operators[op] + " Tremolo", (value & 0x80) ? 1 : 0);
		setEnumParameter(operators[op] + " Vibrato", (value & 0x40) ? 1 : 0);
		setEnumParameter(operators[op] + " Sustain", (value & 0x20) ? 1 : 0);
		setEnumParameter(operators[op] + " Keyscale Rate", (value & 0x10) ? 1 : 0);
		setEnumParameter(operators[op] + " Frequency Multiplier", value & 0x0f);
		break;
	case 0x40:
		setEnumParameter(operators[op] + " Keyscale Level", (value & 0xc0) >> 6);
		setEnumParameter(operators[op] + " Attenuation", value & 0x3f);
		break;
	case 0x60:
		setIntParameter(operators[op] + " Attack", (value & 0xf0) >> 4);
		setIntParameter(operators[op] + " Decay", value & 0x0f);
		break;
	case 0x80:
		setIntParameter(operators[op] + " Sustain Level", (value & 0xf0) >> 4);
		setIntParameter(operators[op] + " Release", value & 0x0f);
		break;
	case 0xC0:
		setIntParameter("Modulator Feedback", (value & 0xe) >> 1);
		setEnumParameter("Algorithm", value & 0x1);
		break;
	case 0xE0:
		setEnumParameter(operators[op] + " Wave", value & 0x7);
		break;
	default:
		break;
	}
}

const String AdlibBlasterAudioProcessor::getParameterName (int index)
{
	return params[index]->getName();
}

const String AdlibBlasterAudioProcessor::getParameterText (int index)
{
    return params[index]->getParameterText();
}

const String AdlibBlasterAudioProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String AdlibBlasterAudioProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

bool AdlibBlasterAudioProcessor::isInputChannelStereoPair (int index) const
{
    return false;
}

bool AdlibBlasterAudioProcessor::isOutputChannelStereoPair (int index) const
{
    return true; //// Jeff-Russ changed to true for AU version. for vsti make it false
}

bool AdlibBlasterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AdlibBlasterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AdlibBlasterAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double AdlibBlasterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AdlibBlasterAudioProcessor::getNumPrograms()
{
    return (int)programs.size();
}

int AdlibBlasterAudioProcessor::getCurrentProgram()
{
    return i_program;
}

void AdlibBlasterAudioProcessor::updateGuiIfPresent()
{
	PluginGui* gui = (PluginGui*)getActiveEditor();
	if (gui) {
		gui->updateFromParameters();
	}
}

void AdlibBlasterAudioProcessor::setCurrentProgram (int index)
{
	if (i_program==index)
		return;

	i_program = index;
	std::vector<float> &v_params = programs[getProgramName(index)];
	for (unsigned int i = 0; i < params.size() && i < v_params.size(); i++) {
		setParameter(i, v_params[i]);
	}
	updateGuiIfPresent();
}

const String AdlibBlasterAudioProcessor::getProgramName (int index)
{
    return program_order[index];
}

void AdlibBlasterAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void AdlibBlasterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	Opl->SetSampleRate((int)sampleRate);
	Opl->EnableWaveformControl();
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void AdlibBlasterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

static const Drum DRUM_INDEX[] = { BDRUM, SNARE, TOM, CYMBAL, HIHAT };

void AdlibBlasterAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	buffer.clear(0, 0, buffer.getNumSamples());
	MidiBuffer::Iterator midi_buffer_iterator(midiMessages);

	MidiMessage midi_message;
	int sample_number;
	int perc = getEnumParameter("Percussion Mode");
	while (midi_buffer_iterator.getNextEvent(midi_message,sample_number)) {
		if (midi_message.isNoteOn()) {
			//note on at sample_number samples after 
			//the beginning of the current buffer
			int n = midi_message.getNoteNumber();
			float noteHz = (float)MidiMessage::getMidiNoteInHertz(n);
			int ch;

			if (perc > 0) {				
				for (int i = 1; i <= Hiopl::CHANNELS; i++) {
					Opl->SetFrequency(i, noteHz, false);
				}
				Opl->HitPercussion(DRUM_INDEX[perc - 1]);
			} else {
				if (!available_channels.empty())
				{
					ch = available_channels.front();
					available_channels.pop_front();
				}
				else
				{
					ch = used_channels.back(); // steal earliest/longest running active channel if out of free channels
					used_channels.pop_back();
					Opl->KeyOff(ch);
				}

				used_channels.push_front(ch);

				switch (getEnumParameter("Carrier Velocity Sensitivity")) {
				case 0:
					Opl->SetAttenuation(ch, 2, getEnumParameter("Carrier Attenuation"));
					break;
				case 1:
					Opl->SetAttenuation(ch, 2, 32 - (midi_message.getVelocity() / 4));
					break;
				case 2:
					Opl->SetAttenuation(ch, 2, 63 - (midi_message.getVelocity() / 2));
					break;
				}
				switch (getEnumParameter("Modulator Velocity Sensitivity")) {
				case 0:
					Opl->SetAttenuation(ch, 1, getEnumParameter("Modulator Attenuation"));
					break;
				case 1:
					Opl->SetAttenuation(ch, 1, 32 - (midi_message.getVelocity() / 4));
					break;
				case 2:
					Opl->SetAttenuation(ch, 1, 63 - (midi_message.getVelocity() / 2));
					break;
				}
				Opl->KeyOn(ch, noteHz);
				active_notes[ch] = n;
				applyPitchBend();
			}
		}
		else if (midi_message.isNoteOff()) {
			if (perc > 0) {
				Opl->ReleasePercussion();
			}
			else {
				int n = midi_message.getNoteNumber();
				int ch = 1;
				while (ch <= Hiopl::CHANNELS && n != active_notes[ch]) {
					ch += 1;
				}
				if (ch <= Hiopl::CHANNELS)
				{
					for (auto i = used_channels.begin(); i != used_channels.end(); ++i)
					{
						if (*i == ch)
						{
							used_channels.erase(i);
							available_channels.push_back(ch);

							break;
						}
					}

					Opl->KeyOff(ch);
					active_notes[ch] = NO_NOTE;
				}
			}
		}
		else if (midi_message.isPitchWheel()) {
			int bend = midi_message.getPitchWheelValue() - 0x2000;	// range -8192 to 8191
			// 1.05946309436 == (2^(1/1200))^100 == 1 semitone == 100 cents
			currentScaledBend = 1.0f + bend * .05775f / 8192;
			applyPitchBend();
		}
	}
/// Jeff-Russ: getSampleData(int) is deprecated. use getWritePointer(int)
	Opl->Generate(buffer.getNumSamples(), buffer.getWritePointer(0));
    
/// Jeff-Russ added loop to copy left channel to right channel. uncomment when building to {0,2} AU
//    const float* LChanRead  = buffer.getReadPointer(0, 0);
//    float* RChanWrite = buffer.getWritePointer(1, 0);
//    for (int i = 0; i < buffer.getNumSamples(); i++) { RChanWrite[i] = LChanRead[i]; }
}

//==============================================================================
bool AdlibBlasterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* AdlibBlasterAudioProcessor::createEditor()
{
	PluginGui* gui = new PluginGui(this);
	gui->updateFromParameters();
	return gui;
}

//==============================================================================

// JUCE requires that JSON names are valid Identifiers (restricted character set) even though the JSON standard allows any valid string.
// Per http://www.juce.com/forum/topic/problem-spaces-json-identifier jules allowed Strings to bypass the assertion check
// but a regression occured in 4317f60 re-enabling this check, so we need to sanitize the names.
// Technically, the code still works without this, but it will trigger a gargantuan number of assertions hindering effective debugging.
Identifier stringToIdentifier(const String &s)
{
	return s.replaceCharacters(" ", "_");
}

void AdlibBlasterAudioProcessor::getStateInformation(MemoryBlock& destData)
{
	ReferenceCountedObjectPtr<DynamicObject> v(new DynamicObject);

	v->setProperty(stringToIdentifier(PROGRAM_INDEX), i_program);

	for (int i = 0; i < getNumParameters(); i++) {
		double p = getParameter(i);

		v->setProperty(stringToIdentifier(getParameterName(i)), p);
	}

	String s = JSON::toString(v.get());

	destData.setSize(s.length());
	destData.copyFrom(s.getCharPointer(), 0, destData.getSize());
}

void AdlibBlasterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	if (sizeInBytes < 1)
		return;

	const char *first = static_cast<const char *>(data);
	const char *last = first + sizeInBytes - 1;

	// simple check for JSON data - assume always object
	if (*first=='{' && *last=='}')
	{
		// json format
		String s(first, sizeInBytes);
		var v = JSON::fromString(s);

		{
			var program = v[stringToIdentifier(PROGRAM_INDEX)];

			if (!program.isVoid())
				i_program = program;
		}

		for (int i=0; i < getNumParameters(); ++i)
		{
			var param = v[stringToIdentifier(getParameterName(i))];

			if (!param.isVoid())
				setParameter(i, param);
		}

		updateGuiIfPresent();

		return;
	}

	float* fdata = (float*)data;
	const int parametersToLoad = std::min<int>(sizeInBytes / sizeof(float), getNumParameters());

	for (int i = 0; i < parametersToLoad; i++) {
		setParameter(i, fdata[i]);
	}
}

bool AdlibBlasterAudioProcessor::isChannelEnabled(const int idx) const {
	return channel_enabled[idx];
}

// @param idx 1-based channel index
void AdlibBlasterAudioProcessor::disableChannel(const int idx)
{
	if (isChannelEnabled(idx)) {
		std::deque<int>::iterator pos = std::find(available_channels.begin(), available_channels.end(), idx);
		if (pos != available_channels.end()) {
			available_channels.erase(pos);
			channel_enabled[idx] = false;
		}
	}
}

void AdlibBlasterAudioProcessor::enableChannel(const int idx)
{
	if (!isChannelEnabled(idx)) {
		available_channels.push_back(idx);
		channel_enabled[idx] = true;
	}
}

void AdlibBlasterAudioProcessor::toggleChannel(const int idx)
{
	isChannelEnabled(idx) ? disableChannel(idx) : enableChannel(idx);
}

size_t AdlibBlasterAudioProcessor::nChannelsEnabled()
{
	return available_channels.size();
}

const char* CHANNEL_DISABLED_STRING = "x";

// @param idx 1-based channel index
const char* AdlibBlasterAudioProcessor::getChannelEnvelopeStage(int idx) const
{
	return isChannelEnabled(idx) ? Opl->GetState(idx) : CHANNEL_DISABLED_STRING;
}


//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdlibBlasterAudioProcessor();
}
