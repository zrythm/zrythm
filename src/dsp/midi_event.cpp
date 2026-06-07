// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"

namespace zrythm::dsp
{

template struct MidiEvent<units::sample_t>;
template struct MidiEvent<units::precise_tick_t>;
template struct MidiEvent<units::sample_u32_t>;
}
