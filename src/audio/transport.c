/*
 * audio/transport.c - transport
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/transport.h"
#include "project.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_playhead.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/ruler_playhead.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/top_bar.h"

#include <gtk/gtk.h>

/**
 * Sets BPM and does any necessary processing (like notifying interested
 * parties).
 */
void
transport_set_bpm (float bpm)
{
  if (bpm < MIN_BPM)
    bpm = MIN_BPM;
  else if (bpm > MAX_BPM)
    bpm = MAX_BPM;
  TRANSPORT->bpm = bpm;
  engine_update_frames_per_tick (TRANSPORT->beats_per_bar,
                                 bpm,
                                 AUDIO_ENGINE->sample_rate);
}

/**
 * Initialize transport
 */
void
transport_init (Transport * self)
{
  g_message ("Initializing transport");
  // set inital total number of beats
  // this is applied to the ruler
  self->total_bars = DEFAULT_TOTAL_BARS;

  /* set BPM related defaults */
  self->beats_per_bar = DEFAULT_BEATS_PER_BAR;
  self->beat_unit = DEFAULT_BEAT_UNIT;

  // set positions of playhead, start/end markers
  position_set_to_bar (&self->playhead_pos, 1);
  position_set_to_bar (&self->cue_pos, 1);
  position_set_to_bar (&self->start_marker_pos, 1);
  position_set_to_bar (&self->end_marker_pos, 128);
  position_set_to_bar (&self->loop_start_pos, 1);
  position_set_to_bar (&self->loop_end_pos, 8);

  /* set playstate */
  self->play_state = PLAYSTATE_PAUSED;

  self->loop = 1;

  transport_set_bpm (DEFAULT_BPM);

  zix_sem_init(&self->paused, 0);
}

void
transport_request_pause ()
{
  TRANSPORT->play_state = PLAYSTATE_PAUSE_REQUESTED;
}

void
transport_request_roll ()
{
  TRANSPORT->play_state = PLAYSTATE_ROLL_REQUESTED;
}

static void
on_playhead_changed ()
{
  if (MAIN_WINDOW)
    {
      if (TOP_BAR->digital_transport)
        {
          g_idle_add (
            (GSourceFunc) gtk_widget_queue_draw,
            GTK_WIDGET (TOP_BAR->digital_transport));
        }
      if (TIMELINE_ARRANGER_PLAYHEAD)
        {
          g_idle_add (
            (GSourceFunc) gtk_widget_queue_allocate,
            GTK_WIDGET (MW_TIMELINE));
        }
      if (TIMELINE_RULER_PLAYHEAD)
        {
          g_idle_add (
            (GSourceFunc) gtk_widget_queue_allocate,
            GTK_WIDGET (MW_RULER));
        }
      if (PIANO_ROLL)
        {
          if (MIDI_RULER)
            {
              g_idle_add (
                (GSourceFunc) gtk_widget_queue_allocate,
                GTK_WIDGET (MIDI_RULER));
            }
          if (MIDI_ARRANGER)
            {
              g_idle_add (
                (GSourceFunc) gtk_widget_queue_allocate,
                GTK_WIDGET (MIDI_ARRANGER));
            }
          if (MIDI_MODIFIER_ARRANGER)
            {
              g_idle_add (
                (GSourceFunc) gtk_widget_queue_allocate,
                GTK_WIDGET (MIDI_MODIFIER_ARRANGER));
            }
        }
    }
}

/**
 * Moves the playhead by the time corresponding to given samples.
 */
void
transport_add_to_playhead (int frames)
{
  if (TRANSPORT->play_state == PLAYSTATE_ROLLING)
    {
      position_add_frames (&TRANSPORT->playhead_pos,
                           frames);
      on_playhead_changed ();
    }
}

/**
 * Moves playhead to given pos
 */
void
transport_move_playhead (Position * target, ///< position to set to
                         int      panic) ///< send MIDI panic or not
{
  position_set_to_pos (&TRANSPORT->playhead_pos,
                       target);
  if (panic)
    {
      AUDIO_ENGINE->panic = 1;
    }

  on_playhead_changed ();
}

/**
 * Updates the frames in all transport positions
 */
void
transport_update_position_frames ()
{
  position_update_frames (&TRANSPORT->playhead_pos);
  position_update_frames (&TRANSPORT->cue_pos);
  position_update_frames (&TRANSPORT->start_marker_pos);
  position_update_frames (&TRANSPORT->end_marker_pos);
  position_update_frames (&TRANSPORT->loop_start_pos);
  position_update_frames (&TRANSPORT->loop_end_pos);
}

/**
 * Gets beat unit as int.
 */
int
transport_get_beat_unit ()
{
  switch (TRANSPORT->beat_unit)
    {
    case BEAT_UNIT_2:
      return 2;
    case BEAT_UNIT_4:
      return 4;
    case BEAT_UNIT_8:
      return 8;
    case BEAT_UNIT_16:
      return 16;
    }
  g_assert_not_reached ();
  return -1;
}
