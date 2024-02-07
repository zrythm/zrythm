// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
 */

#include <math.h>
#include <stdlib.h>

#include "dsp/audio_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/clip.h"
#include "dsp/engine.h"
#include "dsp/fade.h"
#include "dsp/pool.h"
#include "dsp/port.h"
#include "dsp/stretcher.h"
#include "dsp/tempo_track.h"
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

  self->rt_stretcher =
    stretcher_new_rubberband (AUDIO_ENGINE->sample_rate, 2, 1.0, 1.0, true);
}

void
audio_track_setup (Track * self)
{
  channel_track_setup (self);
}
