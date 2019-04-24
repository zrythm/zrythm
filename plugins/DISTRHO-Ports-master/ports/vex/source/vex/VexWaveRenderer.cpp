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

#include "VexWaveRenderer.h"

WaveTableNames WaveRenderer::waveTableNames[kWaveTableSize] = {
    { "asym_saw",     Wavetables::asym_saw     },
    { "bass_tone",    Wavetables::bass_tone    },
    { "buzz_1",       Wavetables::buzz_1       },
    { "buzz_2",       Wavetables::buzz_2       },
    { "dark_strings", Wavetables::dark_strings },
    { "deep_ring_1",  Wavetables::deep_ring_1  },
    { "deep_ring_2",  Wavetables::deep_ring_2  },
    { "epiano_tone",  Wavetables::epiano_tone  },
    { "ghost_1",      Wavetables::ghost_1      },
    { "ghost_2",      Wavetables::ghost_2      },
    { "ghost_3",      Wavetables::ghost_3      },
    { "ghost_4",      Wavetables::ghost_4      },
    { "grind_1",      Wavetables::grind_1      },
    { "grind_2",      Wavetables::grind_2      },
    { "more_strings", Wavetables::more_strings },
    { "multi_pulse",  Wavetables::multi_pulse  },
    { "organ_1",      Wavetables::organ_1      },
    { "organ_2",      Wavetables::organ_2      },
    { "one_string",   Wavetables::one_string   },
    { "phasing_sqr",  Wavetables::phasing_sqr  },
    { "pulse",        Wavetables::pulse        },
    { "saw",          Wavetables::saw          },
    { "sharp_1",      Wavetables::sharp_1      },
    { "sharp_2",      Wavetables::sharp_2      },
    { "sine",         Wavetables::sine         },
    { "soft_1",       Wavetables::soft_1       },
    { "soft_2",       Wavetables::soft_2       },
    { "soft_3",       Wavetables::soft_3       },
    { "soft_4",       Wavetables::soft_4       },
    { "square",       Wavetables::square       },
    { "string_fuzz",  Wavetables::string_fuzz  },
    { "strings_1",    Wavetables::strings_1    },
    { "strings_2",    Wavetables::strings_2    },
    { "syn_choir_1",  Wavetables::syn_choir_1  },
    { "syn_choir_2",  Wavetables::syn_choir_2  },
    { "syn_choir_3",  Wavetables::syn_choir_3  },
    { "thin_1",       Wavetables::thin_1       },
    { "thin_2",       Wavetables::thin_2       },
    { "two_strings",  Wavetables::two_strings  },
    { "voice_1",      Wavetables::voice_1      },
    { "voice_2",      Wavetables::voice_2      }
};

int WaveRenderer::getWaveTableSize()
{
    return kWaveTableSize;
}

String WaveRenderer::getWaveTableName(const int index)
{
    jassert (index >= 0);
    jassert (index < kWaveTableSize);

    return String(WaveRenderer::waveTableNames[index].name);
}
