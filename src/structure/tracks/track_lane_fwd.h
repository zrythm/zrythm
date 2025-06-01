// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/traits.h"

namespace zrythm::structure::tracks
{
class MidiLane;
class AudioLane;
using TrackLaneVariant = std::variant<MidiLane, AudioLane>;
using TrackLanePtrVariant = to_pointer_variant<TrackLaneVariant>;
}
