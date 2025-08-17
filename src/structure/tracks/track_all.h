// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/audio_bus_track.h"
#include "structure/tracks/audio_group_track.h"
#include "structure/tracks/audio_track.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/folder_track.h"
#include "structure/tracks/instrument_track.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/midi_bus_track.h"
#include "structure/tracks/midi_group_track.h"
#include "structure/tracks/midi_track.h"
#include "structure/tracks/modulator_track.h"

namespace zrythm::structure::tracks
{
inline Track *
from_variant (const TrackPtrVariant &variant)
{
  return std::visit ([&] (auto &&t) -> Track * { return t; }, variant);
}
}
