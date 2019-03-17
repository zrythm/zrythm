/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Events for calling refresh on widgets.
 */

#include "audio/automation_lane.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/arranger_playhead.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_clip_editor.h"
#include "gui/widgets/audio_ruler.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/center_dock_bot_box.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/connections.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/ruler_marker.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/top_dock_edge.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/stack.h"
#include "zrythm.h"

#define ET_POP(ev) \
  ((EventType) stack_pop (ev->et_stack))
#define ARG_POP(ev) \
  ((void *) stack_pop (ev->arg_stack))

static int
on_playhead_changed ()
{
  /*if (automatic && (aa++ % 2 != 0))*/
    /*{*/
      /*return 0;*/
    /*}*/
  if (MAIN_WINDOW)
    {
      if (TOP_BAR->digital_transport)
        {
          gtk_widget_queue_draw (
            GTK_WIDGET (TOP_BAR->digital_transport));
        }
      if (TIMELINE_ARRANGER_PLAYHEAD)
        {
          /*gtk_widget_queue_allocate (*/
            /*GTK_WIDGET (MW_TIMELINE));*/
        }
      if (TL_RULER_PLAYHEAD)
        {
          gtk_widget_queue_allocate (
            GTK_WIDGET (MW_RULER));
        }
      if (MW_PIANO_ROLL)
        {
          if (MIDI_RULER)
            {
              gtk_widget_queue_allocate (
                GTK_WIDGET (MIDI_RULER));
            }
          if (MIDI_ARRANGER)
            {
              /*gtk_widget_queue_allocate (*/
                /*GTK_WIDGET (MIDI_ARRANGER));*/
            }
          if (MIDI_MODIFIER_ARRANGER)
            {
              /*gtk_widget_queue_allocate (*/
                /*GTK_WIDGET (MIDI_MODIFIER_ARRANGER));*/
            }
        }
      if (MW_AUDIO_CLIP_EDITOR)
        {
          if (AUDIO_RULER)
            gtk_widget_queue_allocate (
              GTK_WIDGET (AUDIO_RULER));
          if (AUDIO_ARRANGER)
            {
              /*ARRANGER_WIDGET_GET_PRIVATE (*/
                /*AUDIO_ARRANGER);*/
              /*g_object_ref (ar_prv->playhead);*/
              /*gtk_container_remove (*/
                /*AUDIO_ARRANGER,*/
                /*ar_prv->playhead);*/
              /*gtk_overlay_add_overlay (*/
                /*AUDIO_ARRANGER,*/
                /*ar_prv->playhead);*/
              /*g_object_unref (ar_prv->playhead);*/
              /*gtk_widget_queue_draw (*/
                /*GTK_WIDGET (ar_prv->playhead));*/
              /*gtk_widget_queue_allocate (*/
                /*GTK_WIDGET (ar_prv->playhead));*/
            /*gtk_widget_queue_allocate (*/
              /*GTK_WIDGET (AUDIO_ARRANGER));*/
            /*arranger_widget_refresh (*/
              /*Z_ARRANGER_WIDGET (AUDIO_ARRANGER));*/
            /*gtk_widget_queue_resize (AUDIO_ARRANGER);*/
            /*gtk_widget_queue_draw (AUDIO_ARRANGER);*/
              gtk_widget_queue_allocate (
                GTK_WIDGET (AUDIO_ARRANGER));
            }
        }
    }
  /*aa = 1;*/
  return FALSE;
}

static void
on_track_state_changed (Track * track)
{
  if (track->widget)
    {
      track_widget_block_all_signal_handlers (
        track->widget);
      track_widget_refresh (track->widget);
      track_widget_unblock_all_signal_handlers (
        track->widget);
    }

  Channel * chan = track_get_channel (track);
  if (chan && chan->widget)
    {
      channel_widget_block_all_signal_handlers (
        chan->widget);
      channel_widget_refresh (chan->widget);
      channel_widget_unblock_all_signal_handlers (
        chan->widget);
    }
}

static void
on_range_selection_changed ()
{
  ARRANGER_WIDGET_GET_PRIVATE (
    MW_TIMELINE);
  gtk_widget_queue_draw (
    GTK_WIDGET (
      ar_prv->bg));
}

static void
on_automation_lane_added (AutomationLane * al)
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (al->at->track);
  if (atl && atl->widget)
    automation_tracklist_widget_refresh (
      atl->widget);

  timeline_arranger_widget_refresh_children (
    MW_TIMELINE);
}

static void
on_track_added (Track * track)
{
  if (!MAIN_WINDOW || !MW_CENTER_DOCK)
    return;

  if (MW_MIXER)
    mixer_widget_refresh (MW_MIXER);
  if (MW_TRACKLIST)
    tracklist_widget_hard_refresh (MW_TRACKLIST);

  /* needs to be called later because tracks need
   * time to get allocated */
  if (MW_TIMELINE)
    g_idle_add (
      (GSourceFunc) arranger_widget_refresh,
      Z_ARRANGER_WIDGET (MW_TIMELINE));
}

static void
on_track_select_changed (Track * track)
{
  if (track->widget)
    track_widget_select (track->widget,
                         track->selected);
}

/**
 * Called when setting clip editor region using
 * g_idle_add to give the widgets time to have
 * widths.
 */
static int
refresh_midi_ruler_and_arranger ()
{
  if (!ui_is_widget_revealed (MIDI_RULER))
    {
      g_usleep (1000);
      return TRUE;
    }

  /* ruler must be refreshed first to get the
   * correct px when calling ui_* functions */
  midi_ruler_widget_refresh (
    MIDI_RULER);

  /* remove all previous children and add new */
  arranger_widget_refresh (
    Z_ARRANGER_WIDGET (MIDI_ARRANGER));
  arranger_widget_refresh (
    Z_ARRANGER_WIDGET (
      MIDI_MODIFIER_ARRANGER));

  CLIP_EDITOR->region_changed = 0;

  return FALSE;
}

/**
 * Called when setting clip editor region using
 * g_idle_add to give the widgets time to have
 * widths.
 */
static int
refresh_audio_ruler_and_arranger ()
{
  if (!ui_is_widget_revealed (AUDIO_RULER))
    {
      g_usleep (1000);
      return TRUE;
    }

  /* ruler must be refreshed first to get the
   * correct px when calling ui_* functions */
  audio_ruler_widget_refresh ();

  /* remove all previous children and add new */

  CLIP_EDITOR->region_changed = 0;

  return FALSE;
}

static void
on_clip_editor_region_changed ()
{
  /* TODO */
  /*gtk_notebook_set_current_page (MAIN_WINDOW->bot_notebook, 0);*/
  Region * r = CLIP_EDITOR->region;

  if (r)
    {
      if (r->type == REGION_TYPE_MIDI)
        {
          gtk_stack_set_visible_child (
            GTK_STACK (MW_CLIP_EDITOR),
            GTK_WIDGET (MW_PIANO_ROLL));

          gtk_label_set_text (
            MW_PIANO_ROLL->midi_name_label,
            track_get_name (
              r->track));

          color_area_widget_set_color (
            MW_PIANO_ROLL->color_bar,
            &r->track->color);

          g_idle_add (
            refresh_midi_ruler_and_arranger,
            NULL);
        }
      else if (r->type == REGION_TYPE_AUDIO)
        {
          gtk_stack_set_visible_child (
            GTK_STACK (MW_CLIP_EDITOR),
            GTK_WIDGET (MW_AUDIO_CLIP_EDITOR));

          gtk_label_set_text (
            MW_AUDIO_CLIP_EDITOR->track_name,
            track_get_name (
              r->track));

          color_area_widget_set_color (
            MW_AUDIO_CLIP_EDITOR->color_bar,
            &r->track->color);

          g_idle_add (
            refresh_audio_ruler_and_arranger,
            NULL);
        }
    }
  else
    {
      gtk_stack_set_visible_child (
        GTK_STACK (MW_CLIP_EDITOR),
        GTK_WIDGET (
          MW_CLIP_EDITOR->no_selection_label));
    }
  g_message ("clip editor region changed");
}

static void
on_automation_value_changed (
  Automatable * a)
{
  AutomationTrack * at =
    automatable_get_automation_track (a);
  if (at && at->al && at->al->widget)
    automation_lane_widget_update_current_val (
      at->al->widget);
}

static void
on_plugin_added (Plugin * plugin)
{
  Channel * channel = plugin->channel;
  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (channel->track);
  automation_tracklist_update (
    automation_tracklist);
  if (automation_tracklist &&
      automation_tracklist->widget)
    automation_tracklist_widget_refresh (
      automation_tracklist->widget);
  connections_widget_refresh (MW_CONNECTIONS);
}

static void
on_midi_note_changed (MidiNote * midi_note)
{
  Region * r = (Region *) midi_note->midi_region;

  if (r->widget)
    gtk_widget_queue_draw (GTK_WIDGET (r->widget));
}

static int
update_adj ()
{
  GtkAdjustment * adj =
    gtk_scrolled_window_get_hadjustment (
      MW_CENTER_DOCK->timeline_scroll);
  gtk_adjustment_set_value (
    adj,
    gtk_adjustment_get_value (adj) + gtk_adjustment_get_step_increment (adj));
  gtk_scrolled_window_set_hadjustment(
    MW_CENTER_DOCK->timeline_scroll,
    adj);

  return FALSE;
}

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 */
int
events_process ()
{
  gint64 curr_time = g_get_monotonic_time ();

  EventType et;
  void * arg;

  /*int i = 0;*/
  /*g_message ("starting processing");*/
  while (!stack_is_empty (EVENTS->et_stack))
    {
      /*i++;*/
      et = ET_POP (EVENTS);
      arg = ARG_POP (EVENTS);

      switch (et)
        {
        case ET_REGION_REMOVED:
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (MW_TIMELINE));
          break;
        case ET_TL_SELECTIONS_CHANGED:
          inspector_widget_refresh ();
          break;
        case ET_RULER_SIZE_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (arg)); // ruler widget
          if (arg == MW_RULER)
            arranger_widget_refresh (
              Z_ARRANGER_WIDGET (
                MW_TIMELINE));
          else if (arg == MIDI_RULER)
            {
              arranger_widget_refresh (
                Z_ARRANGER_WIDGET (
                  MIDI_ARRANGER));
              arranger_widget_refresh (
                Z_ARRANGER_WIDGET (
                  MIDI_MODIFIER_ARRANGER));
            }
          else if (arg == AUDIO_RULER)
            {
              arranger_widget_refresh (
                Z_ARRANGER_WIDGET (
                  AUDIO_ARRANGER));
            }
          break;
        case ET_CLIP_MARKER_POS_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (arg)); // ruler widget
          gtk_widget_queue_draw (
            GTK_WIDGET (
              CLIP_EDITOR->region->widget));
          break;
        case ET_TIMELINE_LOOP_MARKER_POS_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (MW_RULER));
          break;
        case ET_TIMELINE_SONG_MARKER_POS_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (MW_RULER));
          break;
        case ET_LAST_TIMELINE_OBJECT_CHANGED:
          TRANSPORT->total_bars =
            MW_TIMELINE->last_timeline_obj_bars +
            8;
          snap_grid_update_snap_points (
            SNAP_GRID_TIMELINE);

          timeline_ruler_widget_refresh ();
          timeline_arranger_widget_set_size ();
          timeline_minimap_widget_refresh (
            MW_TIMELINE_MINIMAP);
          break;
        case ET_AUTOMATION_VALUE_CHANGED:
          on_automation_value_changed (
            (Automatable *) arg);
          break;
        case ET_RANGE_SELECTION_CHANGED:
          on_range_selection_changed ();
          break;
        case ET_TOOL_CHANGED:
          toolbox_widget_refresh (MW_TOOLBOX);
          arranger_widget_refresh_cursor (
            Z_ARRANGER_WIDGET (MW_TIMELINE));
          if (MIDI_ARRANGER &&
              gtk_widget_get_realized (
                GTK_WIDGET (MIDI_ARRANGER)))
            arranger_widget_refresh_cursor (
              Z_ARRANGER_WIDGET (MIDI_ARRANGER));
          if (MIDI_MODIFIER_ARRANGER &&
              gtk_widget_get_realized (
                GTK_WIDGET (MIDI_MODIFIER_ARRANGER)))
            arranger_widget_refresh_cursor (
              Z_ARRANGER_WIDGET (
                MIDI_MODIFIER_ARRANGER));
          break;
        case ET_TIME_SIGNATURE_CHANGED:
          ruler_widget_refresh (
            Z_RULER_WIDGET (MW_RULER));
          ruler_widget_refresh (
            Z_RULER_WIDGET (MIDI_RULER));
          break;
        case ET_PLAYHEAD_POS_CHANGED:
          g_idle_add (on_playhead_changed,
                      NULL);
          break;
        case ET_CLIP_EDITOR_REGION_CHANGED:
          on_clip_editor_region_changed ();
          break;
        case ET_AUTOMATION_LANE_AUTOMATION_TRACK_CHANGED:
          break;
        case ET_TRACK_BOT_PANED_VISIBILITY_CHANGED:
          tracklist_widget_soft_refresh (
            MW_TRACKLIST);
          break;
        case ET_TRACK_ADDED:
          on_track_added ((Track *) arg);
          break;
        case ET_TRACK_SELECT_CHANGED:
          on_track_select_changed ((Track *) arg);
          break;
        case ET_MIDI_NOTE_CHANGED:
          on_midi_note_changed ((MidiNote *) arg);
          break;
        case ET_TIMELINE_VIEWPORT_CHANGED:
          timeline_minimap_widget_refresh (
            MW_TIMELINE_MINIMAP);
          ruler_widget_refresh (
            Z_RULER_WIDGET (MW_RULER));
          ruler_widget_refresh (
            Z_RULER_WIDGET (MIDI_RULER));
          break;
        case ET_TRACK_STATE_CHANGED:
          on_track_state_changed (
            (Track *) arg);
          break;
        case ET_UNDO_REDO_ACTION_DONE:
          header_bar_widget_refresh_undo_redo_buttons (
            MW_HEADER_BAR);
          break;
        case ET_MIDI_NOTE_CREATED:
        case ET_MIDI_NOTE_REMOVED:
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (MIDI_ARRANGER));
          break;
        case ET_REGION_CREATED:
        case ET_CHORD_CREATED:
        case ET_CHORD_REMOVED:
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (MW_TIMELINE));
          break;
        case ET_RULER_STATE_CHANGED:
          timeline_ruler_widget_refresh ();
          break;
        case ET_AUTOMATION_LANE_ADDED:
          on_automation_lane_added (
            (AutomationLane *) arg);
          break;
        case ET_PLUGIN_ADDED:
          on_plugin_added (
            (Plugin *) arg);
          break;
        }
    }
  /*g_message ("processed %d events", i);*/

  g_usleep (8000);

  return TRUE;
}

/**
 * Must be called from a GTK thread.
 */
void
events_init (Events * self)
{
  self->et_stack = stack_new (64);
  self->arg_stack = stack_new (64);

  /*g_idle_add_full (*/
    /*G_PRIORITY_HIGH_IDLE + 30,*/
    /*(GSourceFunc) events_process,*/
    /*NULL, NULL);*/
  g_idle_add ((GSourceFunc) events_process, NULL);
}
