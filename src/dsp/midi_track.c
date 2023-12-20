// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 */

#include "zrythm-config.h"

#include <inttypes.h>
#include <stdlib.h>

#include "dsp/automation_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/channel_track.h"
#include "dsp/midi_event.h"
#include "dsp/midi_note.h"
#include "dsp/midi_track.h"
#include "dsp/position.h"
#include "dsp/region.h"
#include "dsp/track.h"
#include "dsp/velocity.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/stoat.h"

#include <gtk/gtk.h>

/**
 * Initializes an midi track.
 */
void
midi_track_init (Track * self)
{
  self->type = TRACK_TYPE_MIDI;
  gdk_rgba_parse (&self->color, "#F79616");

  self->icon_name = g_strdup ("signal-midi");
}

void
midi_track_setup (Track * self)
{
  channel_track_setup (self);
}

/**
 * Set MIDI CC's to Touch mode temporarily.
 *
 * Used when starting recording on a MIDI-based track.
 */
void
midi_track_enable_automation_auto_record (Track * self)
{
  g_return_if_fail (self->in_signal_type == TYPE_EVENT);

  AutomationTracklist * atl = &self->automation_tracklist;
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * cur_at = atl->ats[i];
      if (
        cur_at->port_id.flags & PORT_FLAG_MIDI_AUTOMATABLE
        && cur_at->automation_mode == AUTOMATION_MODE_READ
        && cur_at->record_mode == AUTOMATION_RECORD_MODE_TOUCH)
        {
          cur_at->automation_mode_before = AUTOMATION_MODE_READ;
          cur_at->automation_mode = AUTOMATION_MODE_RECORD;
          cur_at->before_modes_set = true;
          array_append (
            atl->ats_in_record_mode, atl->num_ats_in_record_mode, cur_at);
        }
    }
}

/**
 * Unset MIDI CC Touch mode if set temporarily.
 *
 * Used when stopping recording on a MIDI-based track.
 */
void
midi_track_disable_automation_auto_record (Track * self)
{
  g_return_if_fail (self->in_signal_type == TYPE_EVENT);

  for (int i = 0; i < self->automation_tracklist.num_ats; i++)
    {
      AutomationTrack * cur_at = self->automation_tracklist.ats[i];
      if (cur_at->before_modes_set)
        {
          cur_at->automation_mode = cur_at->automation_mode_before;
          cur_at->before_modes_set = false;
        }
    }
}

/**
 * Frees the track.
 *
 * TODO
 */
void
midi_track_free (Track * track)
{
}
