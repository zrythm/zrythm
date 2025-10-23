// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_lane.h"

namespace zrythm::structure::tracks
{

void
TrackLane::generate_name (size_t index)
{
  setName (format_qstr (QObject::tr (default_format_str), index + 1));
}

TrackLane::~TrackLane () = default;
}
