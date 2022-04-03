/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdlib.h>

#include "audio/audio_track.h"
#include "audio/automation_tracklist.h"
#include "audio/clip.h"
#include "audio/engine.h"
#include "audio/fade.h"
#include "audio/pool.h"
#include "audio/port.h"
#include "audio/stretcher.h"
#include "audio/tempo_track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/dsp.h"
#include "utils/math.h"
#include "zrythm_app.h"

void
audio_track_init (Track * self)
{
  self->type = TRACK_TYPE_AUDIO;
  gdk_rgba_parse (&self->color, "#19664c");
  self->icon_name =
    /* signal-audio also works */
    g_strdup ("view-media-visualization");

  self->rt_stretcher = stretcher_new_rubberband (
    AUDIO_ENGINE->sample_rate, 2, 1.0, 1.0, true);
}

void
audio_track_setup (Track * self)
{
  channel_track_setup (self);
}
