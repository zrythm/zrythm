/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2008 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2008 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  rockhardbuns
   @tweaker Lucio Asnaghi
   @tweaker falkTX

 ==============================================================================
*/

#ifndef DISTRHO_VEX_WAVE_RENDERER_HEADER_INCLUDED
#define DISTRHO_VEX_WAVE_RENDERER_HEADER_INCLUDED

#ifdef CARLA_EXPORT
 #include "juce_audio_basics.h"
 #include "resources/Resources.h"
#else
 #include "../StandardHeader.h"
 #include "resources/Resources.h"
#endif

struct OscSet {
    float phase;
    float phaseOffset;
    float phaseInc;
    float phaseIncOffset;
    float cut;
    float buf[4];
};

struct WaveTableNames {
    const char* name;
    const char* data;
};

class WaveRenderer
{
public:
    WaveRenderer()
        : cycle(256), //base pitch 86 Hz
          tableSize(0),
          daTable(nullptr),
          sWave("sine"),
          loadWave(true)
    {
    }

    ~WaveRenderer()
    {
        //M.setSize(0);
        daTable = nullptr;
    }

    const String& getCurrentWaveName() const
    {
        return sWave;
    }

    void setWaveLater(const String& waveName)
    {
        sWave = waveName;
        loadWave = true;
    }

    void actuallySetWave()
    {
        if (sWave == "asym_saw") {
            daTable = (uint16*) Wavetables::asym_saw;
            tableSize = Wavetables::asym_saw_size / 2;
        } else if (sWave == "bass_tone") {
            daTable = (uint16*) Wavetables::bass_tone;
            tableSize = Wavetables::bass_tone_size / 2;
        } else if (sWave == "buzz_1") {
            daTable = (uint16*) Wavetables::buzz_1;
            tableSize = Wavetables::buzz_1_size / 2;
        } else if (sWave == "buzz_2") {
            daTable = (uint16*) Wavetables::buzz_2;
            tableSize = Wavetables::buzz_2_size / 2;
        } else if (sWave == "dark_strings") {
            daTable = (uint16*) Wavetables::dark_strings;
            tableSize = Wavetables::dark_strings_size / 2;
        } else if (sWave == "deep_ring_1") {
            daTable = (uint16*) Wavetables::deep_ring_1;
            tableSize = Wavetables::deep_ring_1_size / 2;
        } else if (sWave == "deep_ring_2") {
            daTable = (uint16*) Wavetables::deep_ring_2;
            tableSize = Wavetables::deep_ring_2_size / 2;
        } else if (sWave == "epiano_tone") {
            daTable = (uint16*) Wavetables::epiano_tone;
            tableSize = Wavetables::epiano_tone_size / 2;
        } else if (sWave == "ghost_1") {
            daTable = (uint16*) Wavetables::ghost_1;
            tableSize = Wavetables::ghost_1_size / 2;
        } else if (sWave == "ghost_2") {
            daTable = (uint16*) Wavetables::ghost_2;
            tableSize = Wavetables::ghost_2_size / 2;
        } else if (sWave == "ghost_3") {
            daTable = (uint16*) Wavetables::ghost_3;
            tableSize = Wavetables::ghost_3_size / 2;
        } else if (sWave == "ghost_4") {
            daTable = (uint16*) Wavetables::ghost_4;
            tableSize = Wavetables::ghost_4_size / 2;
        } else if (sWave == "grind_1") {
            daTable = (uint16*) Wavetables::grind_1;
            tableSize = Wavetables::grind_1_size / 2;
        } else if (sWave == "grind_2") {
            daTable = (uint16*) Wavetables::grind_2;
            tableSize = Wavetables::grind_2_size / 2;
        } else if (sWave == "more_strings") {
            daTable = (uint16*) Wavetables::more_strings;
            tableSize = Wavetables::more_strings_size / 2;
        } else if (sWave == "multi_pulse") {
            daTable = (uint16*) Wavetables::multi_pulse;
            tableSize = Wavetables::multi_pulse_size / 2;
        } else if (sWave == "one_string") {
            daTable = (uint16*) Wavetables::one_string;
            tableSize = Wavetables::one_string_size / 2;
        } else if (sWave == "organ_1") {
            daTable = (uint16*) Wavetables::organ_1;
            tableSize = Wavetables::organ_1_size / 2;
        } else if (sWave == "organ_2") {
            daTable = (uint16*) Wavetables::organ_2;
            tableSize = Wavetables::organ_2_size / 2;
        } else if (sWave == "phasing_sqr") {
            daTable = (uint16*) Wavetables::phasing_sqr;
            tableSize = Wavetables::phasing_sqr_size / 2;
        } else if (sWave == "pulse") {
            daTable = (uint16*) Wavetables::pulse;
            tableSize = Wavetables::pulse_size / 2;
        } else if (sWave == "saw") {
            daTable = (uint16*) Wavetables::saw;
            tableSize = Wavetables::saw_size / 2;
        } else if (sWave == "sharp_1") {
            daTable = (uint16*) Wavetables::sharp_1;
            tableSize = Wavetables::sharp_1_size / 2;
        } else if (sWave == "sharp_2") {
            daTable = (uint16*) Wavetables::sharp_2;
            tableSize = Wavetables::sharp_2_size / 2;
        } else if (sWave == "sine") {
            daTable = (uint16*) Wavetables::sine;
            tableSize = Wavetables::sine_size / 2;
        } else if (sWave == "soft_1") {
            daTable = (uint16*) Wavetables::soft_1;
            tableSize = Wavetables::soft_1_size / 2;
        } else if (sWave == "soft_2") {
            daTable = (uint16*) Wavetables::soft_2;
            tableSize = Wavetables::soft_2_size / 2;
        } else if (sWave == "soft_3") {
            daTable = (uint16*) Wavetables::soft_3;
            tableSize = Wavetables::soft_3_size / 2;
        } else if (sWave == "soft_4") {
            daTable = (uint16*) Wavetables::soft_4;
            tableSize = Wavetables::soft_4_size / 2;
        } else if (sWave == "square") {
            daTable = (uint16*) Wavetables::square;
            tableSize = Wavetables::square_size / 2;
        } else if (sWave == "strings_1") {
            daTable = (uint16*) Wavetables::strings_1;
            tableSize = Wavetables::strings_1_size / 2;
        } else if (sWave == "strings_2") {
            daTable = (uint16*) Wavetables::strings_2;
            tableSize = Wavetables::strings_2_size / 2;
        } else if (sWave == "string_fuzz") {
            daTable = (uint16*) Wavetables::string_fuzz;
            tableSize = Wavetables::string_fuzz_size / 2;
        } else if (sWave == "syn_choir_1") {
            daTable = (uint16*) Wavetables::syn_choir_1;
            tableSize = Wavetables::syn_choir_1_size / 2;
        } else if (sWave == "syn_choir_2") {
            daTable = (uint16*) Wavetables::syn_choir_2;
            tableSize = Wavetables::syn_choir_2_size / 2;
        } else if (sWave == "syn_choir_3") {
            daTable = (uint16*) Wavetables::syn_choir_3;
            tableSize = Wavetables::syn_choir_3_size / 2;
        } else if (sWave == "thin_1") {
            daTable = (uint16*) Wavetables::thin_1;
            tableSize = Wavetables::thin_1_size / 2;
        } else if (sWave == "thin_2") {
            daTable = (uint16*) Wavetables::thin_2;
            tableSize = Wavetables::thin_2_size / 2;
        } else if (sWave == "two_strings") {
            daTable = (uint16*) Wavetables::two_strings;
            tableSize = Wavetables::two_strings_size / 2;
        } else if (sWave == "voice_1") {
            daTable = (uint16*) Wavetables::voice_1;
            tableSize = Wavetables::voice_1_size / 2;
        } else if (sWave == "voice_2") {
            daTable = (uint16*) Wavetables::voice_2;
            tableSize = Wavetables::voice_2_size / 2;
        }

        loadWave = false;

#if 0
// TODO - this fails on linux (host directory not plugin one)
//		String waveName = (File::getSpecialLocation(File::currentExecutableFile)).getParentDirectory().getFullPathName();

		File location = (File::getSpecialLocation(File::userHomeDirectory)).getChildFile(".vex");
		if (! location.exists ())
		    location.createDirectory ();

		String waveName;
		waveName << location.getFullPathName()
		         << "/"
		         << sWave
		         << ".raw";

		DBG( waveName );

		File f(waveName);
		if (f.existsAsFile ())
		{
		    tableSize = int(f.getSize() / 2);
		    if (tableSize > 0)
		    {
			    M.setSize(0);
			    f.loadFileAsData(M);
			    daTable = (uint16*) M.getData();
		    }
        }
        else
        {
            tableSize = int(Wavetables::sine_size / 2);
		    if (tableSize > 0)
		    {
                M.setSize (0);
                M.append (Wavetables::sine, Wavetables::sine_size);
		        daTable = (uint16*) M.getData();
		    }
        }
		loadWave = false;
#endif
    }

    void reset(float f, double s, OscSet& o)
    {
        if (loadWave) actuallySetWave();

        s = s * 2.0;
        f = f * 2.0f * (1.0f + o.phaseIncOffset * 0.01f);
        o.cut = float(2.0 * double_Pi * 9000.0 / s);
        o.phase = o.phaseOffset * tableSize * 0.5f;
        o.phaseInc = float(cycle / (s/f));
        o.buf[0] = 0.0f;
        o.buf[1] = 0.0f;
        o.buf[2] = 0.0f;
        o.buf[3] = 0.0f;
    }

    void setFrequency(float f, double s, OscSet& o)
    {
        s = s * 2.0;
        f = f * 2.0f;
        o.phaseInc = float((float)cycle / (s/f));
    }

    void fillBuffer(float* const buffer, const int bufferSize, OscSet& o)
    {
        if (buffer == nullptr || bufferSize == 0)
            return;

        float tmp;

        for (int i = 0; i < bufferSize; ++i)
        {
            buffer[i] = 0.0f;

            int index = roundFloatToInt(o.phase - 0.5f);
            float alpha = o.phase - (float)index;
            const float conv = 1.0f / 65535.0f;
            float sIndex = daTable[index] * conv - 0.5f;
            float sIndexp1 = daTable[(index + 1) % tableSize] * conv - 0.5f;

            tmp = sIndex + alpha * (sIndexp1 - sIndex);
            o.buf[1] = ((tmp - o.buf[1]) * o.cut) + o.buf[1];
            o.buf[2] = ((o.buf[1] - o.buf[2]) * o.cut) + o.buf[2];
            o.buf[3] = ((o.buf[2] - o.buf[3]) * o.cut) + o.buf[3];
            o.buf[0] = ((o.buf[3] - o.buf[0]) * o.cut) + o.buf[0];

            tmp = o.buf[0];
            buffer[i] += tmp;
            o.phase += o.phaseInc;
            if (o.phase > (float)tableSize)
                o.phase -= (float)tableSize;

            index = roundFloatToInt(o.phase - 0.5f);
            alpha = o.phase - (float)index;
            sIndex = daTable[index] * conv - 0.5f;
            sIndexp1 = daTable[(index + 1) % tableSize] * conv - 0.5f;

            tmp = sIndex + alpha * (sIndexp1 - sIndex);
            o.buf[1] = ((tmp - o.buf[1]) * o.cut) + o.buf[1];
            o.buf[2] = ((o.buf[1] - o.buf[2]) * o.cut) + o.buf[2];
            o.buf[3] = ((o.buf[2] - o.buf[3]) * o.cut) + o.buf[3];
            o.buf[0] = ((o.buf[3] - o.buf[0]) * o.cut) + o.buf[0];

            tmp = o.buf[0];
            buffer[i] += tmp;
            o.phase += o.phaseInc;
            if (o.phase > (float)tableSize)
                o.phase -= (float)tableSize;
        }
    }

    static int getWaveTableSize();
    static String getWaveTableName(const int index);

private:
    static const int kWaveTableSize = 41;
    static WaveTableNames waveTableNames[kWaveTableSize];

    int cycle, tableSize;
    uint16* daTable;
    //MemoryBlock M;
    String sWave;
    bool loadWave;
};

#endif // DISTRHO_VEX_WAVE_RENDERER_HEADER_INCLUDED
