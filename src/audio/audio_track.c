// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
