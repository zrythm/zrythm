/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Events for calling refresh on widgets.
 */

#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/modulator.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/arranger_playhead.h"
#include "gui/widgets/arranger_bg.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_region.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/center_dock_bot_box.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/header_notebook.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/modulator_view.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/editor_selection_info.h"
#include "gui/widgets/pinned_tracklist.h"
#include "gui/widgets/route_target_selector.h"
#include "gui/widgets/ruler_marker.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/timeline_selection_info.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_visibility_tree.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/visibility.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/stack.h"
#include "zrythm.h"

#define ET_POP(ev) \
  ((EventType) stack_pop (ev->et_stack))
#define ARG_POP(ev) \
  ((void *) stack_pop (ev->arg_stack))

static void
redraw_all_arranger_bgs ()
{
  arranger_widget_redraw_bg (
    Z_ARRANGER_WIDGET (MW_TIMELINE));
  arranger_widget_redraw_bg (
    Z_ARRANGER_WIDGET (MW_MIDI_MODIFIER_ARRANGER));
  arranger_widget_redraw_bg (
    Z_ARRANGER_WIDGET (MW_MIDI_ARRANGER));
  arranger_widget_redraw_bg (
    Z_ARRANGER_WIDGET (MW_PINNED_TIMELINE));
}

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
      if (EDITOR_RULER)
        {
          gtk_widget_queue_allocate (
            GTK_WIDGET (EDITOR_RULER));
        }
      if (MW_MIDI_EDITOR_SPACE)
        {
          if (MW_MIDI_ARRANGER)
            {
              /*gtk_widget_queue_allocate (*/
                /*GTK_WIDGET (MIDI_ARRANGER));*/
            }
          if (MW_MIDI_MODIFIER_ARRANGER)
            {
              /*gtk_widget_queue_allocate (*/
                /*GTK_WIDGET (MIDI_MODIFIER_ARRANGER));*/
            }
          midi_editor_space_widget_refresh_labels (
            MW_MIDI_EDITOR_SPACE, 0);
        }
      /*if (MW_AUDIO_CLIP_EDITOR)*/
        /*{*/
          /*if (AUDIO_RULER)*/
            /*gtk_widget_queue_allocate (*/
              /*GTK_WIDGET (AUDIO_RULER));*/
          /*if (AUDIO_ARRANGER)*/
            /*{*/
              /*[>ARRANGER_WIDGET_GET_PRIVATE (<]*/
                /*[>AUDIO_ARRANGER);<]*/
              /*[>g_object_ref (ar_prv->playhead);<]*/
              /*[>gtk_container_remove (<]*/
                /*[>AUDIO_ARRANGER,<]*/
                /*[>ar_prv->playhead);<]*/
              /*[>gtk_overlay_add_overlay (<]*/
                /*[>AUDIO_ARRANGER,<]*/
                /*[>ar_prv->playhead);<]*/
              /*[>g_object_unref (ar_prv->playhead);<]*/
              /*[>gtk_widget_queue_draw (<]*/
                /*[>GTK_WIDGET (ar_prv->playhead));<]*/
              /*[>gtk_widget_queue_allocate (<]*/
                /*[>GTK_WIDGET (ar_prv->playhead));<]*/
            /*[>gtk_widget_queue_allocate (<]*/
              /*[>GTK_WIDGET (AUDIO_ARRANGER));<]*/
            /*[>arranger_widget_refresh (<]*/
              /*[>Z_ARRANGER_WIDGET (AUDIO_ARRANGER));<]*/
            /*[>gtk_widget_queue_resize (AUDIO_ARRANGER);<]*/
            /*[>gtk_widget_queue_draw (AUDIO_ARRANGER);<]*/
              /*gtk_widget_queue_allocate (*/
                /*GTK_WIDGET (AUDIO_ARRANGER));*/
            /*}*/
        /*}*/
    }
  /*aa = 1;*/
  return FALSE;
}

static void
on_channel_output_changed (
  Channel * ch)
{
  if (ch->widget)
    {
      route_target_selector_widget_refresh (
        ch->widget->output, ch);
    }
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
  redraw_all_arranger_bgs ();
  gtk_widget_set_visible (
    GTK_WIDGET (MW_RULER->range),
    PROJECT->has_range);
  gtk_widget_queue_allocate (
    GTK_WIDGET (MW_RULER));
}

static void
on_automation_track_added (AutomationTrack * at)
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (at->track);
  if (atl && atl->widget)
    automation_tracklist_widget_refresh (
      atl->widget);

  timeline_arranger_widget_refresh_children (
    MW_TIMELINE);
  timeline_arranger_widget_refresh_children (
    MW_PINNED_TIMELINE);

  visibility_widget_refresh (MW_VISIBILITY);
}

static void
on_track_added (Track * track)
{
  if (!MAIN_WINDOW || !MW_CENTER_DOCK)
    return;

  if (MW_MIXER)
    mixer_widget_hard_refresh (MW_MIXER);
  if (MW_TRACKLIST)
    tracklist_widget_hard_refresh (MW_TRACKLIST);

  /* needs to be called later because tracks need
   * time to get allocated */
  EVENTS_PUSH (ET_REFRESH_ARRANGER, NULL);
}

/**
 * Called when setting clip editor region using
 * g_idle_add to give the widgets time to have
 * widths.
 */
static int
refresh_editor_ruler_and_arranger ()
{
  if (!ui_is_widget_revealed (EDITOR_RULER))
    {
      g_usleep (1000);
      return TRUE;
    }

  /* ruler must be refreshed first to get the
   * correct px when calling ui_* functions */
  editor_ruler_widget_refresh (
    EDITOR_RULER);

  /* remove all previous children and add new */
  arranger_widget_refresh (
    Z_ARRANGER_WIDGET (MW_MIDI_ARRANGER));
  arranger_widget_refresh (
    Z_ARRANGER_WIDGET (
      MW_MIDI_MODIFIER_ARRANGER));
  arranger_widget_refresh (
    Z_ARRANGER_WIDGET (
      MW_AUTOMATION_ARRANGER));
  arranger_widget_refresh (
    Z_ARRANGER_WIDGET (
      MW_CHORD_ARRANGER));

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
      gtk_stack_set_visible_child (
        GTK_STACK (MW_CLIP_EDITOR),
        GTK_WIDGET (MW_CLIP_EDITOR->main_box));

      clip_editor_inner_widget_refresh (
        MW_CLIP_EDITOR_INNER);

      g_idle_add (
        refresh_editor_ruler_and_arranger,
        NULL);
    }
  else
    {
      gtk_stack_set_visible_child (
        GTK_STACK (MW_CLIP_EDITOR),
        GTK_WIDGET (
          MW_CLIP_EDITOR->no_selection_label));
    }

  CLIP_EDITOR->region_cache = r;
}

static void
on_automation_value_changed (
  Automatable * a)
{
  AutomationTrack * at =
    automatable_get_automation_track (a);
  Region * r;
  for (int i = 0; i < at->num_regions; i++)
    {
      r = at->regions[i];
      if (GTK_IS_WIDGET (r->widget))
        Z_AUTOMATION_REGION_WIDGET (
          r->widget)->cache = 0;
    }
  if (at && at->widget)
    automation_track_widget_update_current_val (
      at->widget);
}

static void
on_plugin_added (Plugin * plugin)
{
  Channel * channel = plugin->track->channel;
  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (channel->track);
  if (automation_tracklist &&
      automation_tracklist->widget)
    automation_tracklist_widget_refresh (
      automation_tracklist->widget);
}

static void
on_modulator_added (Modulator * modulator)
{
  on_plugin_added (modulator->plugin);

  modulator_view_widget_refresh (
    MW_MODULATOR_VIEW,
    modulator->track);
}

static void
on_plugins_removed (Channel * ch)
{
  ChannelSlotWidget * csw;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      csw =
        ch->widget->inserts[i];
      gtk_widget_unset_state_flags (
        GTK_WIDGET (csw),
        GTK_STATE_FLAG_SELECTED);
      gtk_widget_queue_draw (
        GTK_WIDGET (csw));
    }
}

static void
on_midi_note_selection_changed ()
{
 Region * region = CLIP_EDITOR->region;

  if (region && region->widget)
    gtk_widget_queue_draw (
      GTK_WIDGET (region->widget));

  gtk_widget_queue_allocate (
    GTK_WIDGET (MW_MIDI_MODIFIER_ARRANGER));

  editor_selection_info_widget_refresh (
    MW_MAS_INFO, MA_SELECTIONS);
}

static void
on_midi_note_changed (MidiNote * midi_note)
{
  Velocity * vel;
  for (int i = 0; i < 2; i++)
    {
      if (i == 0)
        {
          midi_note =
            midi_note_get_main_midi_note (
              midi_note);

          /* select the velocities */
          for (int j = 0; j < 2; j++)
            {
              if (j == 0)
                vel =
                  velocity_get_main_velocity (
                    midi_note->vel);
              else if (j == 1)
                vel =
                  velocity_get_main_trans_velocity (
                    midi_note->vel);

              if (GTK_IS_WIDGET (vel->widget))
                {
                  if (velocity_is_selected (vel))
                    {
                      gtk_widget_set_state_flags (
                        GTK_WIDGET (vel->widget),
                        GTK_STATE_FLAG_SELECTED,
                        0);
                    }
                  else
                    {
                      gtk_widget_unset_state_flags (
                        GTK_WIDGET (vel->widget),
                        GTK_STATE_FLAG_SELECTED);
                    }
                  gtk_widget_queue_draw (
                    GTK_WIDGET (vel->widget));
                }

            }
        }
      else if (i == 1)
        midi_note =
          midi_note_get_main_trans_midi_note (midi_note);

      if (GTK_IS_WIDGET (midi_note->widget))
        {
          if (midi_note_is_selected (midi_note))
            {
              gtk_widget_set_state_flags (
                GTK_WIDGET (midi_note->widget),
                GTK_STATE_FLAG_SELECTED,
                0);
            }
          else
            {
              gtk_widget_unset_state_flags (
                GTK_WIDGET (midi_note->widget),
                GTK_STATE_FLAG_SELECTED);
            }
          gtk_widget_queue_draw (
            GTK_WIDGET (midi_note->widget));
        }
    }

  if (midi_note->region->widget)
    gtk_widget_queue_draw (
      GTK_WIDGET (midi_note->region->widget));
}

static inline void
on_mixer_selections_changed ()
{
  Plugin * pl;
  Channel * ch;
  ChannelSlotWidget * csw;

  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (!track_type_has_channel (track->type))
        continue;

      ch = track->channel;

      for (int j = 0; j < STRIP_SIZE; j++)
        {
          pl = ch->plugins[j];
          csw =
            ch->widget->inserts[j];
          /*GtkStateFlags state_flags =*/
            /*gtk_widget_get_state_flags (*/
              /*GTK_WIDGET (csw));*/

          if (pl)
            {
              if (plugin_is_selected (pl))
                {
                  /*if (state_flags !=*/
                      /*GTK_STATE_FLAG_SELECTED)*/
                    /*{*/
                      /*g_message ("1");*/
                      gtk_widget_set_state_flags (
                        GTK_WIDGET (csw),
                        GTK_STATE_FLAG_SELECTED,
                        0);
                      gtk_widget_queue_draw (
                        GTK_WIDGET (csw));
                    /*}*/
                }
              else
                {
                  /*if (state_flags ==*/
                      /*GTK_STATE_FLAG_SELECTED)*/
                    /*{*/
                      /*g_message ("2");*/
                      gtk_widget_unset_state_flags (
                        GTK_WIDGET (csw),
                        GTK_STATE_FLAG_SELECTED);
                      gtk_widget_queue_draw (
                        GTK_WIDGET (csw));
                    /*}*/
                }
            }
          else
            {
              /*if (state_flags ==*/
                  /*GTK_STATE_FLAG_SELECTED)*/
                /*{*/
                  /*g_message ("3");*/
                  gtk_widget_unset_state_flags (
                    GTK_WIDGET (csw),
                    GTK_STATE_FLAG_SELECTED);
                  gtk_widget_queue_draw (
                    GTK_WIDGET (csw));
                /*}*/
            }
        }
    }
  inspector_widget_refresh (MW_INSPECTOR);
}

static void
on_track_color_changed (Track * track)
{
  channel_widget_refresh (track->channel->widget);
  track_widget_refresh (track->widget);
  inspector_widget_refresh (MW_INSPECTOR);
}

static void
on_track_name_changed (Track * track)
{
  /* refresh all because tracks routed to/from are
   * also affected */
  mixer_widget_soft_refresh (MW_MIXER);
  track_widget_refresh (track->widget);
  inspector_widget_refresh (MW_INSPECTOR);
  visibility_widget_refresh (MW_VISIBILITY);
}

static void
on_track_changed (Track * track)
{
  if (GTK_IS_WIDGET (track->widget))
    {
      gtk_widget_set_visible (
        GTK_WIDGET (track->widget),
        track->visible);
      if (track_is_selected (track))
        {
          gtk_widget_set_state_flags (
            GTK_WIDGET (track->widget),
            GTK_STATE_FLAG_SELECTED,
            0);
        }
      else
        {
          gtk_widget_unset_state_flags (
            GTK_WIDGET (track->widget),
            GTK_STATE_FLAG_SELECTED);
        }
      gtk_widget_queue_draw (
        GTK_WIDGET (track->widget));
    }
}

static void
on_plugin_visibility_changed (Plugin * pl)
{
  if (pl->visible)
    plugin_open_ui (pl);
  else if (!pl->visible)
    plugin_close_ui (pl);

  if (pl->track->type ==
      TRACK_TYPE_INSTRUMENT &&
      pl->track->widget &&
      Z_IS_INSTRUMENT_TRACK_WIDGET (
        pl->track->widget))
    instrument_track_widget_refresh_buttons (
      Z_INSTRUMENT_TRACK_WIDGET (
        pl->track->widget));

  if (pl->track->channel->widget &&
      Z_IS_CHANNEL_WIDGET (pl->track->channel->widget))
    gtk_widget_queue_draw (
      GTK_WIDGET (
        pl->track->channel->widget->inserts[
          channel_get_plugin_index (
            pl->track->channel, pl)]));
}

/*static int*/
/*update_adj ()*/
/*{*/
  /*GtkAdjustment * adj =*/
    /*gtk_scrolled_window_get_hadjustment (*/
      /*MW_CENTER_DOCK->timeline_scroll);*/
  /*gtk_adjustment_set_value (*/
    /*adj,*/
    /*gtk_adjustment_get_value (adj) + gtk_adjustment_get_step_increment (adj));*/
  /*gtk_scrolled_window_set_hadjustment(*/
    /*MW_CENTER_DOCK->timeline_scroll,*/
    /*adj);*/

  /*return FALSE;*/
/*}*/

static inline void
clean_duplicates_and_copy (
  GAsyncQueue * q,
  ZEvent **     events,
  int *         num_events)
{
  ZEvent * event;
  *num_events = 0;
  int i, already_exists = 0;;

  /* only add events once to new array while popping */
  while ((event =
            g_async_queue_try_pop_unlocked (q)))
    {
      already_exists = 0;

      for (i = 0; i < *num_events; i++)
        if (event->type == events[i]->type &&
            event->arg == events[i]->arg)
          already_exists = 1;

      if (already_exists)
        free (event);
      else
        array_append (events, (*num_events), event);
    }
}

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 */
static int
events_process (void * data)
{
  GAsyncQueue * q = (GAsyncQueue *) data;
  /*gint64 curr_time = g_get_monotonic_time ();*/
  if (q != ZRYTHM->event_queue)
    {
      g_async_queue_unref (q);
      return G_SOURCE_REMOVE;
    }

  g_async_queue_lock (q);
  ZEvent * events[60];
  ZEvent * ev;
  int num_events = 0, i;
  clean_duplicates_and_copy (
    q, events, &num_events);
  g_async_queue_unlock (q);

  /*g_message ("starting processing");*/
  for (i = 0; i < num_events; i++)
    {
      ev = events[i];
      if (ev->type < 0)
        {
          g_warn_if_reached ();
          continue;
        }

      switch (ev->type)
        {
        case ET_TRACKS_REMOVED:
          if (MW_MIXER)
            mixer_widget_hard_refresh (MW_MIXER);
          if (MW_TRACKLIST)
            tracklist_widget_hard_refresh (
              MW_TRACKLIST);
          visibility_widget_refresh (
            MW_VISIBILITY);
          break;
        case ET_CHANNEL_REMOVED:
          mixer_widget_hard_refresh (
            MW_MIXER);
          break;
        case ET_REGION_CREATED:
        case ET_MARKER_CREATED:
        case ET_SCALE_OBJECT_CREATED:
        case ET_SCALE_OBJECT_CHANGED:
        case ET_REGION_REMOVED:
        case ET_SCALE_OBJECT_REMOVED:
        case ET_MARKER_REMOVED:
          /* FIXME track is passed so only
           * refresh that track */
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (MW_TIMELINE));
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (MW_PINNED_TIMELINE));
          break;
        case ET_CHORD_OBJECT_CREATED:
        case ET_CHORD_OBJECT_CHANGED:
        case ET_CHORD_OBJECT_REMOVED:
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (MW_CHORD_ARRANGER));
          break;
        case ET_AUTOMATION_POINT_CHANGED:
          {
            AutomationPoint * ap =
              (AutomationPoint *) ev->arg;
            if (ap->index > 0 &&
                ap->index <
                  ap->region->num_aps - 1)
              {
                AutomationCurve * ac =
                  ap->region->acs[ap->index];
                if (ac &&
                    GTK_IS_WIDGET (ac->widget))
                  {
                    ac->widget->redraw = 1;
                    gtk_widget_queue_draw (
                      GTK_WIDGET (ac->widget));
                  }
              }
            Z_AUTOMATION_REGION_WIDGET (
              ap->region->widget)->cache = 0;
          }
          break;
        case ET_AUTOMATION_POINT_REMOVED:
        case ET_AUTOMATION_CURVE_REMOVED:
        case ET_AUTOMATION_POINT_CREATED:
        case ET_AUTOMATION_CURVE_CREATED:
          /* FIXME automation track is passed so
           * only refresh the automation lane */
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (
              MW_AUTOMATION_ARRANGER));
          break;
        case ET_TL_SELECTIONS_CHANGED:
          inspector_widget_refresh (MW_INSPECTOR);
          timeline_selection_info_widget_refresh (
            MW_TS_INFO, TL_SELECTIONS);
          break;
        case ET_TL_SELECTIONS_CREATED:
        case ET_TL_SELECTIONS_REMOVED:
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (
              MW_TIMELINE));
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (
              MW_PINNED_TIMELINE));
          break;
        case ET_TRACKLIST_SELECTIONS_CHANGED:
          inspector_widget_refresh (MW_INSPECTOR);
          mixer_widget_soft_refresh (MW_MIXER);
          break;
        case ET_RULER_SIZE_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (ev->arg)); // ruler widget
          if (ev->arg == MW_RULER)
            {
              arranger_widget_refresh (
                Z_ARRANGER_WIDGET (
                  MW_TIMELINE));
              arranger_widget_refresh (
                Z_ARRANGER_WIDGET (
                  MW_PINNED_TIMELINE));
            }
          else if (ev->arg == EDITOR_RULER)
            {
              if (gtk_widget_get_visible (
                    GTK_WIDGET (MW_MIDI_ARRANGER)))
                {
                  arranger_widget_refresh (
                    Z_ARRANGER_WIDGET (
                      MW_MIDI_ARRANGER));
                  arranger_widget_refresh (
                    Z_ARRANGER_WIDGET (
                      MW_MIDI_MODIFIER_ARRANGER));
                }
              if (gtk_widget_get_visible (
                    GTK_WIDGET (MW_AUDIO_ARRANGER)))
                {
                  arranger_widget_refresh (
                    Z_ARRANGER_WIDGET (
                      MW_AUDIO_ARRANGER));
                }
            }
          break;
        case ET_CLIP_MARKER_POS_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (ev->arg)); // ruler widget
          gtk_widget_queue_draw (
            GTK_WIDGET (
              CLIP_EDITOR->region->widget));
          break;
        case ET_TIMELINE_LOOP_MARKER_POS_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (MW_RULER));
          gtk_widget_queue_draw (
            GTK_WIDGET (EDITOR_RULER));
          redraw_all_arranger_bgs ();
          break;
        case ET_TIMELINE_SONG_MARKER_POS_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (MW_RULER));
          break;
        case ET_PLUGIN_VISIBILITY_CHANGED:
          on_plugin_visibility_changed (
            (Plugin *) ev->arg);
          break;
        case ET_LAST_TIMELINE_OBJECT_CHANGED:
          TRANSPORT->total_bars =
            MW_TIMELINE->last_timeline_obj_bars +
            8;
          snap_grid_update_snap_points (
            SNAP_GRID_TIMELINE);

          timeline_ruler_widget_refresh (
            MW_RULER);
          timeline_arranger_widget_set_size (
            MW_TIMELINE);
          timeline_minimap_widget_refresh (
            MW_TIMELINE_MINIMAP);
          break;
        case ET_AUTOMATION_VALUE_CHANGED:
          on_automation_value_changed (
            (Automatable *) ev->arg);
          break;
        case ET_RANGE_SELECTION_CHANGED:
          on_range_selection_changed ();
          break;
        case ET_TOOL_CHANGED:
          toolbox_widget_refresh (MW_TOOLBOX);
          arranger_widget_refresh_cursor (
            Z_ARRANGER_WIDGET (MW_TIMELINE));
          if (MW_MIDI_ARRANGER &&
              gtk_widget_get_realized (
                GTK_WIDGET (MW_MIDI_ARRANGER)))
            arranger_widget_refresh_cursor (
              Z_ARRANGER_WIDGET (MW_MIDI_ARRANGER));
          if (MW_MIDI_MODIFIER_ARRANGER &&
              gtk_widget_get_realized (
                GTK_WIDGET (MW_MIDI_MODIFIER_ARRANGER)))
            arranger_widget_refresh_cursor (
              Z_ARRANGER_WIDGET (
                MW_MIDI_MODIFIER_ARRANGER));
          break;
        case ET_TIME_SIGNATURE_CHANGED:
          ruler_widget_refresh (
            Z_RULER_WIDGET (MW_RULER));
          ruler_widget_refresh (
            Z_RULER_WIDGET (EDITOR_RULER));
          break;
        case ET_PLAYHEAD_POS_CHANGED:
          g_idle_add (on_playhead_changed,
                      NULL);
          break;
        case ET_CLIP_EDITOR_REGION_CHANGED:
          on_clip_editor_region_changed ();
          break;
        case ET_TRACK_BOT_PANED_VISIBILITY_CHANGED:
          tracklist_widget_soft_refresh (
            MW_TRACKLIST);
          break;
        case ET_TRACK_LANES_VISIBILITY_CHANGED:
          tracklist_widget_soft_refresh (
            MW_TRACKLIST);
          timeline_arranger_widget_update_visibility (
            MW_TIMELINE);
          break;
        case ET_TRACK_ADDED:
          on_track_added ((Track *) ev->arg);
          break;
        case ET_MA_SELECTIONS_CHANGED:
           on_midi_note_selection_changed();
            break;
        case ET_MIDI_NOTE_CHANGED:
          /*g_message ("mn changed %p",*/
                     /*((MidiNote *)ev->arg)->widget);*/
          on_midi_note_changed ((MidiNote *) ev->arg);
          break;
        case ET_REGION_CHANGED:
          break;
        case ET_ARRANGER_OBJECT_SELECTION_CHANGED:
          arranger_object_info_set_widget_visibility_and_state ((ArrangerObjectInfo *) ev->arg, 1);
          break;
        case ET_PLUGIN_SELECTION_CHANGED:
          on_mixer_selections_changed ();
          break;
        case ET_TRACK_CHANGED:
          on_track_changed ((Track *) ev->arg);
          break;
        case ET_TRACKS_ADDED:
          if (MW_MIXER)
            mixer_widget_hard_refresh (MW_MIXER);
          if (MW_TRACKLIST)
            tracklist_widget_hard_refresh (
              MW_TRACKLIST);
          visibility_widget_refresh (
            MW_VISIBILITY);
          break;
        case ET_TRACK_COLOR_CHANGED:
          on_track_color_changed ((Track *) ev->arg);
          break;
        case ET_TRACK_NAME_CHANGED:
          on_track_name_changed ((Track *) ev->arg);
          break;
        case ET_REFRESH_ARRANGER:
          if (MW_TIMELINE)
            arranger_widget_refresh (
              Z_ARRANGER_WIDGET (MW_TIMELINE));
          if (MW_PINNED_TIMELINE)
            arranger_widget_refresh (
              Z_ARRANGER_WIDGET (MW_TIMELINE));
          break;
        case ET_TIMELINE_VIEWPORT_CHANGED:
          timeline_minimap_widget_refresh (
            MW_TIMELINE_MINIMAP);
          ruler_widget_refresh (
            Z_RULER_WIDGET (MW_RULER));
          ruler_widget_refresh (
            Z_RULER_WIDGET (EDITOR_RULER));
          break;
        case ET_TRACK_STATE_CHANGED:
          on_track_state_changed (
            (Track *) ev->arg);
          break;
        case ET_TRACK_VISIBILITY_CHANGED:
          tracklist_widget_soft_refresh (
            MW_TRACKLIST);
          timeline_arranger_widget_refresh_children (
            MW_TIMELINE);
          timeline_arranger_widget_refresh_children (
            MW_PINNED_TIMELINE);
          track_visibility_tree_widget_refresh (
            MW_TRACK_VISIBILITY_TREE);
          break;
        case ET_UNDO_REDO_ACTION_DONE:
          home_toolbar_widget_refresh_undo_redo_buttons (
            MW_HOME_TOOLBAR);
          break;
        /* FIXME rename to MIDI_NOTE_ADDED_TO_REGION */
        case ET_MIDI_NOTE_CREATED:
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (MW_MIDI_ARRANGER));
          arranger_widget_refresh (
            Z_ARRANGER_WIDGET (
              MW_MIDI_MODIFIER_ARRANGER));
          break;
        case ET_MIDI_NOTE_REMOVED:
          /*g_object_ref (((MidiNote *) ev->arg)->widget);*/
          /*gtk_container_remove (*/
            /*GTK_CONTAINER (MIDI_ARRANGER),*/
            /*GTK_WIDGET (((MidiNote *) ev->arg)->widget));*/
          /*arranger_widget_refresh (*/
            /*Z_ARRANGER_WIDGET (MIDI_ARRANGER));*/
          midi_arranger_widget_refresh_children (
            MW_MIDI_ARRANGER);
          midi_modifier_arranger_widget_refresh_children (
            MW_MIDI_MODIFIER_ARRANGER);
          break;
        case ET_PIANO_ROLL_HIGHLIGHTING_CHANGED:
          if (MW_MIDI_EDITOR_SPACE)
            midi_editor_space_widget_refresh_labels (
              MW_MIDI_EDITOR_SPACE, 0);
          break;
        case ET_RULER_STATE_CHANGED:
          timeline_ruler_widget_refresh (
            MW_RULER);
          break;
        case ET_AUTOMATION_TRACK_ADDED:
        case ET_AUTOMATION_TRACK_REMOVED:
        case ET_AUTOMATION_TRACK_CHANGED:
          on_automation_track_added (
            (AutomationTrack *) ev->arg);
          break;
        case ET_PLUGIN_ADDED:
          on_plugin_added (
            (Plugin *) ev->arg);
          break;
        case ET_PLUGINS_REMOVED:
          on_plugins_removed ((Channel *)ev->arg);
          break;
        case ET_MIXER_SELECTIONS_CHANGED:
          on_mixer_selections_changed ();
          break;
        case ET_CHANNEL_OUTPUT_CHANGED:
          on_channel_output_changed (
            (Channel *)ev->arg);
          break;
        case ET_TRACKS_MOVED:
          if (MW_MIXER)
            mixer_widget_hard_refresh (MW_MIXER);
          if (MW_TRACKLIST)
            tracklist_widget_hard_refresh (
              MW_TRACKLIST);
          if (MW_PINNED_TRACKLIST)
            pinned_tracklist_widget_hard_refresh (
              MW_PINNED_TRACKLIST);

          visibility_widget_refresh (
            MW_VISIBILITY);

          /* needs to be called later because tracks
           * need time to get allocated */
          EVENTS_PUSH (ET_REFRESH_ARRANGER, NULL);
          break;
        case ET_CHANNEL_SLOTS_CHANGED:
          channel_widget_update_inserts (
            ((Channel *)ev->arg)->widget);
          break;
        case ET_DRUM_MODE_CHANGED:
          midi_editor_space_widget_refresh (
            MW_MIDI_EDITOR_SPACE);
          break;
        case ET_MODULATOR_ADDED:
          on_modulator_added ((Modulator *)ev->arg);
          break;
        case ET_PINNED_TRACKLIST_SIZE_CHANGED:
          /*gtk_widget_set_size_request (*/
            /*GTK_WIDGET (*/
              /*MW_CENTER_DOCK->*/
                /*pinned_timeline_scroll),*/
            /*-1,*/
            /*gtk_widget_get_allocated_height (*/
              /*GTK_WIDGET (MW_PINNED_TRACKLIST)));*/
          break;
        case ET_TRACK_LANE_ADDED:
        case ET_TRACK_LANE_REMOVED:
          tracklist_widget_soft_refresh (
            MW_TRACKLIST);
          timeline_arranger_widget_update_visibility (
            MW_TIMELINE);
          break;
        case ET_LOOP_TOGGLED:
          arranger_widget_refresh_all_backgrounds ();
          gtk_widget_queue_draw (
            GTK_WIDGET (MW_RULER));
          gtk_widget_queue_draw (
            GTK_WIDGET (EDITOR_RULER));
          break;
        case ET_MARKER_POSITIONS_CHANGED:
        case ET_CHORD_OBJECT_POSITIONS_CHANGED:
        case ET_SCALE_OBJECT_POSITIONS_CHANGED:
          /* TODO */
          break;
        case ET_REGION_POSITIONS_CHANGED:
          /* redraw midi ruler if region
           * positions were changed */
          gtk_widget_queue_allocate (
            GTK_WIDGET (EDITOR_RULER));
          gtk_widget_queue_draw (
            GTK_WIDGET (EDITOR_RULER));
          break;
        case ET_TIMELINE_OBJECTS_IN_TRANSIT:
          /*timeline_arranger_widget_refresh_children (*/
            /*MW_TIMELINE);*/
          /*timeline_arranger_widget_refresh_children (*/
            /*MW_PINNED_TIMELINE);*/
          if (TL_SELECTIONS->num_regions > 0)
            editor_ruler_widget_refresh (
              EDITOR_RULER);
          break;
        case ET_AUTOMATION_OBJECTS_IN_TRANSIT:
          /*gtk_widget_queue_allocate (*/
            /*GTK_WIDGET (MW_AUTOMATION_ARRANGER));*/
          break;
        case ET_CHORD_KEY_CHANGED:
          for (int j = 0;
               j < CHORD_EDITOR->num_chords; j++)
            {
              if (CHORD_EDITOR->chords[j] ==
                (ChordDescriptor *) ev->arg)
                {
                  gtk_widget_queue_draw (
                    GTK_WIDGET (
                      MW_CHORD_EDITOR_SPACE->
                        chord_keys[j]));
                  break;
                }
            }
          break;
        case ET_JACK_TRANSPORT_TYPE_CHANGED:
          g_message ("doing");
          top_bar_widget_refresh (
            TOP_BAR);
          break;
        case ET_MARKER_NAME_CHANGED:
          marker_widget_recreate_pango_layouts (
            (MarkerWidget *) ev->arg);
          break;
        case ET_SELECTING_IN_ARRANGER:
          arranger_widget_redraw_bg (
            Z_ARRANGER_WIDGET (ev->arg));
          break;
        default:
          g_message (
            "event %d not implemented yet",
            ev->type);
          break;
        }

      free (ev);
    }
  /*g_message ("processed %d events", i);*/

  if (num_events > 6)
    g_message ("More than 6 events processed. "
               "Optimization needed.");

  /*g_usleep (8000);*/
  project_sanity_check (PROJECT);

  return G_SOURCE_CONTINUE;
}

/**
 * Must be called from a GTK thread.
 */
GAsyncQueue *
events_init ()
{
  GAsyncQueue * queue =
    g_async_queue_new ();

  g_timeout_add (32, events_process, queue);

  return queue;
}
