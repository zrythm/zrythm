/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/clip.h"
#include "audio/modulator.h"
#include "audio/pool.h"
#include "audio/stretcher.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_selection_info.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/header_notebook.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/modulator_view.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/route_target_selector.h"
#include "gui/widgets/ruler_marker.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bot_box.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/timeline_selection_info.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_visibility_tree.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/tracklist_header.h"
#include "gui/widgets/visibility.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "utils/stack.h"
#include "zrythm.h"

#include <glib/gi18n.h>

static void
redraw_all_arranger_bgs ()
{
  arranger_widget_redraw_whole (MW_TIMELINE);
  arranger_widget_redraw_whole (MW_PINNED_TIMELINE);
  arranger_widget_redraw_whole (MW_MIDI_ARRANGER);
  arranger_widget_redraw_whole (
    MW_MIDI_MODIFIER_ARRANGER);
  arranger_widget_redraw_whole (
    MW_AUTOMATION_ARRANGER);
  arranger_widget_redraw_whole (MW_CHORD_ARRANGER);
  arranger_widget_redraw_whole (MW_AUDIO_ARRANGER);
}

static void
on_arranger_selections_in_transit (
  ArrangerSelections * sel)
{
  g_return_if_fail (sel);

  switch (sel->type)
  {
  case ARRANGER_SELECTIONS_TYPE_TIMELINE:
    event_viewer_widget_refresh (
      MW_TIMELINE_EVENT_VIEWER);
    if (TL_SELECTIONS->num_regions > 0)
      {
        ZRegion * r = TL_SELECTIONS->regions[0];
        switch (r->id.type)
          {
          case REGION_TYPE_MIDI:
            arranger_widget_redraw_whole (
              MW_MIDI_ARRANGER);
            arranger_widget_redraw_whole (
              MW_MIDI_MODIFIER_ARRANGER);
            break;
          case REGION_TYPE_AUTOMATION:
            arranger_widget_redraw_whole (
              MW_AUTOMATION_ARRANGER);
            break;
          case REGION_TYPE_CHORD:
            arranger_widget_redraw_whole (
              MW_CHORD_ARRANGER);
            break;
          case REGION_TYPE_AUDIO:
            arranger_widget_redraw_whole (
              MW_AUDIO_ARRANGER);
            break;
          }
        ruler_widget_redraw_whole (EDITOR_RULER);
      }
    break;
  case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
    clip_editor_redraw_region (CLIP_EDITOR);
    break;
  default:
    break;
  }
}

/**
 * @param manually Whether the position was changed
 *   by the user.
 */
static void
on_playhead_changed (
  int manually)
{
  if (MAIN_WINDOW)
    {
      if (TOP_BAR->digital_transport)
        {
          gtk_widget_queue_draw (
            GTK_WIDGET (TOP_BAR->digital_transport));
        }
      if (MW_RULER)
        {
          if (manually)
            ruler_widget_redraw_whole (MW_RULER);
          else
            ruler_widget_redraw_playhead (MW_RULER);
        }
      if (EDITOR_RULER)
        {
          if (manually)
            ruler_widget_redraw_whole (
              EDITOR_RULER);
          else
            ruler_widget_redraw_playhead (
              EDITOR_RULER);
        }
      if (MW_MIDI_EDITOR_SPACE)
        {
          if (MW_MIDI_ARRANGER)
            {
              if (manually)
                arranger_widget_redraw_whole (
                  MW_MIDI_ARRANGER);
              else
                arranger_widget_redraw_playhead (
                  MW_MIDI_ARRANGER);
            }
          if (MW_MIDI_MODIFIER_ARRANGER)
            {
              if (manually)
                arranger_widget_redraw_whole (
                  MW_MIDI_MODIFIER_ARRANGER);
              else
                arranger_widget_redraw_playhead (
                  MW_MIDI_MODIFIER_ARRANGER);
            }
          piano_roll_keys_widget_refresh (
            MW_PIANO_ROLL_KEYS);
        }
      if (MW_TIMELINE)
        {
          if (manually)
            arranger_widget_redraw_whole (
              MW_TIMELINE);
          else
            arranger_widget_redraw_playhead (
              MW_TIMELINE);
        }
      if (MW_PINNED_TIMELINE)
        {
          if (manually)
            arranger_widget_redraw_whole (
              MW_PINNED_TIMELINE);
          else
            arranger_widget_redraw_playhead (
              MW_PINNED_TIMELINE);
        }
      if (MW_AUTOMATION_ARRANGER)
        {
          if (manually)
            arranger_widget_redraw_whole (
              MW_AUTOMATION_ARRANGER);
          else
            arranger_widget_redraw_playhead (
              MW_AUTOMATION_ARRANGER);
        }
      if (MW_AUDIO_ARRANGER)
        {
          if (manually)
            arranger_widget_redraw_whole (
              MW_AUDIO_ARRANGER);
          else
            arranger_widget_redraw_playhead (
              MW_AUDIO_ARRANGER);
        }
    }
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
      /*track_widget_block_all_signal_handlers (*/
        /*track->widget);*/
      track_widget_force_redraw (track->widget);
      /*track_widget_unblock_all_signal_handlers (*/
        /*track->widget);*/
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
  /*gtk_widget_set_visible (*/
    /*GTK_WIDGET (MW_RULER->range),*/
    /*PROJECT->has_range);*/
  gtk_widget_queue_allocate (
    GTK_WIDGET (MW_RULER));
  ruler_widget_redraw_whole (
    (RulerWidget *) MW_RULER);
  ruler_widget_redraw_whole (
    (RulerWidget *) EDITOR_RULER);
}

static void
on_automation_track_added (
  AutomationTrack * at)
{
  /*AutomationTracklist * atl =*/
    /*track_get_automation_tracklist (at->track);*/
  /*if (atl && atl->widget)*/
    /*automation_tracklist_widget_refresh (*/
      /*atl->widget);*/

  Track * track =
    automation_track_get_track (at);
  g_return_if_fail (track);
  if (Z_IS_TRACK_WIDGET (track->widget))
    {
      TrackWidget * tw =
        (TrackWidget *) track->widget;
      track_widget_update_size (tw);
    }

  arranger_widget_redraw_whole (
    MW_TIMELINE);
  arranger_widget_redraw_whole (
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

#if 0
static void
on_clip_editor_region_changed ()
{
  /* TODO */
  /*gtk_notebook_set_current_page (MAIN_WINDOW->bot_notebook, 0);*/
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);

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

      region_identifier_copy (
        &CLIP_EDITOR->region_id_cache, &r->id);
      CLIP_EDITOR->had_region = 1;
    }
  else
    {
      gtk_stack_set_visible_child (
        GTK_STACK (MW_CLIP_EDITOR),
        GTK_WIDGET (
          MW_CLIP_EDITOR->no_selection_label));
      CLIP_EDITOR->had_region = 0;
    }
}
#endif

static void
on_automation_value_changed (
  Port * port)
{
  /*AutomationTrack * at =*/
    /*automatable_get_automation_track (a);*/
  /*if (at && at->widget)*/
    /*automation_track_widget_update_current_val (*/
      /*at->widget);*/
}

static void
on_plugin_added (Plugin * plugin)
{
  Track * track =
    plugin_get_track (plugin);
  /*AutomationTracklist * automation_tracklist =*/
    /*track_get_automation_tracklist (track);*/
  if (track && track->widget)
    track_widget_force_redraw (track->widget);
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
  /* redraw slots */
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

  /* change inspector page */
  inspector_widget_refresh (MW_INSPECTOR);
}

static void
refresh_for_selections_type (
  ArrangerSelectionsType type)
{
  switch (type)
    {
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      event_viewer_widget_refresh (
        MW_TIMELINE_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      clip_editor_redraw_region (CLIP_EDITOR);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      clip_editor_redraw_region (CLIP_EDITOR);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      clip_editor_redraw_region (CLIP_EDITOR);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    default:
      g_return_if_reached ();
    }
}

static void
on_arranger_selections_changed (
  ArrangerSelections * sel)
{
  int size = 0;
  /*ArrangerObject ** objs =*/
    /*arranger_selections_get_all_objects (*/
      /*sel, &size);*/
  for (int i = 0; i < size; i++)
    {
      /*ArrangerObject * obj = objs[i];*/
      /*g_warn_if_fail (*/
        /*arranger_object_is_main (obj));*/
      /*ArrangerObjectWidget * obj_w =*/
        /*arranger_object_get_widget (obj);*/
      /*if (!obj_w)*/
        /*continue;*/
      /*arranger_object_widget_force_redraw (obj_w);*/
    }

  refresh_for_selections_type (sel->type);

  inspector_widget_refresh (MW_INSPECTOR);
}

static void
arranger_selections_change_redraw_everything (
  ArrangerSelections * sel)
{
  switch (sel->type)
    {
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      arranger_widget_redraw_whole (
        MW_TIMELINE);
      arranger_widget_redraw_whole (
        MW_PINNED_TIMELINE);
      event_viewer_widget_refresh (
        MW_TIMELINE_EVENT_VIEWER);
      arranger_widget_redraw_whole (
        MW_MIDI_ARRANGER);
      arranger_widget_redraw_whole (
        MW_MIDI_MODIFIER_ARRANGER);
      arranger_widget_redraw_whole (
        MW_CHORD_ARRANGER);
      arranger_widget_redraw_whole (
        MW_AUTOMATION_ARRANGER);
      ruler_widget_redraw_whole (
        EDITOR_RULER);
      break;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      clip_editor_redraw_region (CLIP_EDITOR);
      arranger_widget_redraw_whole (
        MW_MIDI_ARRANGER);
      arranger_widget_redraw_whole (
        MW_MIDI_MODIFIER_ARRANGER);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      clip_editor_redraw_region (CLIP_EDITOR);
      arranger_widget_redraw_whole (
        MW_CHORD_ARRANGER);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      clip_editor_redraw_region (CLIP_EDITOR);
      arranger_widget_redraw_whole (
        MW_AUTOMATION_ARRANGER);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    default:
      g_return_if_reached ();
    }
}

static void
on_arranger_selections_created (
  ArrangerSelections * sel)
{
  arranger_selections_change_redraw_everything (
    sel);
#if 0
  arranger_selections_redraw (sel);
  switch (sel->type)
    {
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      event_viewer_widget_refresh (
        MW_TIMELINE_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      clip_editor_redraw_region (CLIP_EDITOR);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      clip_editor_redraw_region (CLIP_EDITOR);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      clip_editor_redraw_region (CLIP_EDITOR);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    default:
      g_return_if_reached ();
    }
#endif
}

static void
on_arranger_selections_moved (
  ArrangerSelections * sel)
{
  arranger_selections_change_redraw_everything (
    sel);
}

static void
on_arranger_selections_removed (
  ArrangerSelections * sel)
{
  arranger_selections_change_redraw_everything (
    sel);

#if 0
  switch (type)
    {
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      arranger_widget_redraw_whole (
        MW_TIMELINE);
      arranger_widget_redraw_whole (
        MW_PINNED_TIMELINE);
      event_viewer_widget_refresh (
        MW_TIMELINE_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      clip_editor_redraw_region (CLIP_EDITOR);
      arranger_widget_redraw_whole (
        MW_MIDI_ARRANGER);
      arranger_widget_redraw_whole (
        MW_MIDI_MODIFIER_ARRANGER);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      clip_editor_redraw_region (CLIP_EDITOR);
      arranger_widget_redraw_whole (
        MW_CHORD_ARRANGER);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      clip_editor_redraw_region (CLIP_EDITOR);
      arranger_widget_redraw_whole (
        MW_AUTOMATION_ARRANGER);
      event_viewer_widget_refresh (
        MW_EDITOR_EVENT_VIEWER);
      break;
    default:
      g_return_if_reached ();
    }
#endif
}

static void
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
  track_widget_force_redraw (track->widget);
  inspector_widget_refresh (MW_INSPECTOR);
}

static void
on_track_name_changed (Track * track)
{
  /* refresh all because tracks routed to/from are
   * also affected */
  mixer_widget_soft_refresh (MW_MIXER);
  track_widget_force_redraw (track->widget);
  inspector_widget_refresh (MW_INSPECTOR);
  visibility_widget_refresh (MW_VISIBILITY);
}

static void
on_arranger_object_changed (
  ArrangerObject * obj)
{
  g_return_if_fail (obj);

  /* parent region, if any */
  ArrangerObject * parent_r_obj =
    (ArrangerObject *)
    arranger_object_get_region (obj);

  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      {
        /*ArrangerObject * prev_ap_obj =*/
          /*(ArrangerObject *)*/
          /*automation_region_get_prev_ap (*/
            /*((AutomationPoint *) obj)->region,*/
            /*(AutomationPoint *) obj);*/

        /* redraw this ap and also the previous one
         * if any */
        for (int i = 0; i < 2; i++)
          {
            /*ArrangerObject * _obj = NULL;*/
            /*if (i == 0)*/
              /*_obj = obj;*/
            /*else if (prev_ap_obj)*/
              /*_obj = prev_ap_obj;*/
            /*else*/
              /*break;*/

            /*if (_obj &&*/
                /*Z_IS_ARRANGER_OBJECT_WIDGET (*/
                  /*_obj->widget))*/
              /*{*/
                /*arranger_object_widget_force_redraw (*/
                  /*(ArrangerObjectWidget *)*/
                  /*_obj->widget);*/
              /*}*/
          }
      }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        /*MidiNote * mn =*/
          /*(MidiNote *)*/
          /*arranger_object_get_main (obj);*/
        /*ArrangerObject * vel_obj =*/
          /*(ArrangerObject *) mn->vel;*/
        /*if (vel_obj->widget)*/
          /*{*/
            /*arranger_object_set_widget_visibility_and_state (*/
              /*vel_obj, 1);*/
          /*}*/
      }
      break;
    case ARRANGER_OBJECT_TYPE_REGION:
      /* redraw editor ruler if region
       * positions were changed */
      ruler_widget_redraw_whole (
        EDITOR_RULER);
      break;
    case ARRANGER_OBJECT_TYPE_MARKER:
      {
        /*MarkerWidget * mw =*/
          /*(MarkerWidget *) obj->widget;*/
        /*if (Z_IS_MARKER_WIDGET (mw))*/
          /*{*/
            /*marker_widget_recreate_pango_layouts (*/
              /*mw);*/
          /*}*/
      }
      break;
    default:
      break;
    }

  /* redraw parent region */
  if (parent_r_obj)
    {
      /*ArrangerObjectWidget * obj_w =*/
        /*(ArrangerObjectWidget *)*/
        /*parent_r_obj->widget;*/
      /*if (obj_w)*/
        /*arranger_object_widget_force_redraw (*/
          /*obj_w);*/
    }

  /* redraw this */
  /*ArrangerObjectWidget * obj_w =*/
    /*(ArrangerObjectWidget *) obj->widget;*/
  /*if (obj_w)*/
    /*arranger_object_widget_force_redraw (obj_w);*/

  /* refresh arranger */
  arranger_object_queue_redraw (obj);
  /*ArrangerWidget * arranger =*/
    /*arranger_object_get_arranger (obj);*/
  /*arranger_widget_redraw_whole (arranger);*/
}

static void
on_arranger_object_created (
  ArrangerObject * obj)
{
  /* refresh arranger */
  /*ArrangerWidget * arranger =*/
    /*arranger_object_get_arranger (obj);*/
  /*arranger_widget_redraw_whole (arranger);*/
  arranger_object_queue_redraw (obj);

  if (obj->type == ARRANGER_OBJECT_TYPE_MIDI_NOTE)
    {
      arranger_widget_redraw_whole (
        (ArrangerWidget *)
        MW_MIDI_MODIFIER_ARRANGER);
    }
}

static void
on_arranger_object_removed (
  ArrangerObjectType type)
{
  switch (type)
    {
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
    case ARRANGER_OBJECT_TYPE_VELOCITY:
      arranger_widget_redraw_whole (
        MW_MIDI_ARRANGER);
      arranger_widget_redraw_whole (
        MW_MIDI_MODIFIER_ARRANGER);
      break;
    case ARRANGER_OBJECT_TYPE_REGION:
    case ARRANGER_OBJECT_TYPE_SCALE_OBJECT:
    case ARRANGER_OBJECT_TYPE_MARKER:
      arranger_widget_redraw_whole (
        MW_TIMELINE);
      arranger_widget_redraw_whole (
        MW_PINNED_TIMELINE);
      break;
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      arranger_widget_redraw_whole (
        MW_CHORD_ARRANGER);
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      arranger_widget_redraw_whole (
        MW_AUTOMATION_ARRANGER);
      break;
    default:
      g_return_if_reached ();
    }
}

static void
on_track_changed (Track * track)
{
  if (GTK_IS_WIDGET (track->widget))
    {
      gtk_widget_set_visible (
        GTK_WIDGET (track->widget),
        track->visible);
      track_widget_force_redraw (track->widget);
    }
}

static void
on_plugin_visibility_changed (Plugin * pl)
{
  if (pl->visible)
    plugin_open_ui (pl);
  else if (!pl->visible)
    plugin_close_ui (pl);

  Track * track = plugin_get_track (pl);
  if (track->type ==
      TRACK_TYPE_INSTRUMENT &&
      track->widget &&
      Z_IS_TRACK_WIDGET (
        track->widget))
    track_widget_force_redraw (track->widget);

  if (track->channel->widget &&
      Z_IS_CHANNEL_WIDGET (
        track->channel->widget))
    gtk_widget_queue_draw (
      GTK_WIDGET (
        track->channel->widget->inserts[
          channel_get_plugin_index (
            track->channel, pl)]));
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

static void
stretch_audio_region (
  ZRegion * region)
{
  g_return_if_fail (region);
  ArrangerObject * obj = (ArrangerObject *) region;

  if (g_settings_get_int (S_UI, "musical-mode"))
    {
      region->stretching = 1;

      AudioClip * clip =
        audio_region_get_clip (region);
      double time_ratio =
        (double) clip->bpm /
        (double) TRANSPORT->bpm;
      Stretcher * stretcher =
        stretcher_new_rubberband (
          AUDIO_ENGINE->sample_rate,
          clip->channels, time_ratio, 1.0);

      /*g_message ("BEFORE STRETCHING: clip num frames: %ld, position end: %ld",*/
        /*clip->num_frames,*/
        /*obj->end_pos.frames);*/
      /*position_print (&obj->end_pos);*/

      ssize_t returned_frames =
        stretcher_stretch_interleaved (
          stretcher, clip->frames,
          (size_t) clip->num_frames,
          &region->frames);
      g_warn_if_fail (returned_frames > 0);
      region->num_frames =
        (size_t) returned_frames;
      /*g_warn_if_fail (*/
        /*returned_frames !=*/
          /*(ssize_t) new_frames_size);*/

      /*g_message ("AFTER: position end frames %ld",*/
        /*obj->end_pos.frames);*/
      /*position_print (&obj->end_pos);*/
      g_warn_if_fail (
        obj->end_pos.frames <= returned_frames);

      region->stretching = 0;
    }

}

static void
on_bpm_changed (void)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      if (track->type != TRACK_TYPE_AUDIO)
        continue;

      for (int j = 0; j < track->num_lanes; j++)
        {
          TrackLane * lane = track->lanes[j];

          for (int k = 0; k < lane->num_regions;
               k++)
            {
              stretch_audio_region (
                lane->regions[k]);
            }
        }
    }
}

static inline void
clean_duplicates_and_copy (
  MPMCQueue * q,
  ZEvent **   events,
  int *       num_events)
{
  ZEvent * event;
  *num_events = 0;
  int i, already_exists = 0;;

  /* only add events once to new array while
   * popping */
  while (event_queue_dequeue_event (
           q, &event))
    {
      already_exists = 0;

      for (i = 0; i < *num_events; i++)
        if (event->type == events[i]->type &&
            event->arg == events[i]->arg)
          already_exists = 1;

      if (already_exists)
        {
          object_pool_return (
            ZRYTHM->event_obj_pool, event);
        }
      else
        {
          array_append (
            events, (*num_events), event);
        }
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
  MPMCQueue * q = (MPMCQueue *) data;
  /*gint64 curr_time = g_get_monotonic_time ();*/
  if (q != ZRYTHM->event_queue)
    {
      return G_SOURCE_REMOVE;
    }

  ZEvent * events[60];
  ZEvent * ev;
  int num_events = 0, i;
  clean_duplicates_and_copy (
    q, events, &num_events);

  /*g_message ("starting processing");*/
  for (i = 0; i < num_events; i++)
    {
      if (i > 30)
        {
          g_message (
            "more than 30 UI events processed!");
        }

      ev = events[i];
      if (ev->type < 0)
        {
          g_warn_if_reached ();
          continue;
        }

      /*g_message ("event type %d", ev->type);*/

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
          tracklist_header_widget_refresh_track_count (
            MW_TRACKLIST_HEADER);
          break;
        case ET_CHANNEL_REMOVED:
          mixer_widget_hard_refresh (
            MW_MIXER);
          break;
        case ET_ARRANGER_OBJECT_CREATED:
          on_arranger_object_created (
            (ArrangerObject *) ev->arg);
          break;
        case ET_ARRANGER_OBJECT_CHANGED:
          on_arranger_object_changed (
            (ArrangerObject *) ev->arg);
          break;
        case ET_ARRANGER_OBJECT_REMOVED:
          on_arranger_object_removed (
            (ArrangerObjectType) ev->arg);
          break;
        case ET_ARRANGER_SELECTIONS_CHANGED:
          on_arranger_selections_changed (
            ARRANGER_SELECTIONS (ev->arg));
          break;
        case ET_ARRANGER_SELECTIONS_CREATED:
          on_arranger_selections_created (
            ARRANGER_SELECTIONS (ev->arg));
          break;
        case ET_ARRANGER_SELECTIONS_REMOVED:
          on_arranger_selections_removed (
            ARRANGER_SELECTIONS (ev->arg));
          break;
        case ET_ARRANGER_SELECTIONS_MOVED:
          on_arranger_selections_moved (
            ARRANGER_SELECTIONS (ev->arg));
          break;
        case ET_TRACKLIST_SELECTIONS_CHANGED:
          /* only refresh the inspector if the
           * tracklist selection changed by
           * clicking on a track */
          if (PROJECT->last_selection ==
                SELECTION_TYPE_TRACK)
            inspector_widget_refresh (MW_INSPECTOR);
          mixer_widget_soft_refresh (MW_MIXER);
          break;
        case ET_RULER_SIZE_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (ev->arg)); // ruler widget
          if (ev->arg == MW_RULER)
            {
              arranger_widget_redraw_whole (
                Z_ARRANGER_WIDGET (
                  MW_TIMELINE));
              arranger_widget_redraw_whole (
                Z_ARRANGER_WIDGET (
                  MW_PINNED_TIMELINE));
            }
          else if (ev->arg == EDITOR_RULER)
            {
              if (gtk_widget_get_visible (
                    GTK_WIDGET (MW_MIDI_ARRANGER)))
                {
                  arranger_widget_redraw_whole (
                    Z_ARRANGER_WIDGET (
                      MW_MIDI_ARRANGER));
                  arranger_widget_redraw_whole (
                    Z_ARRANGER_WIDGET (
                      MW_MIDI_MODIFIER_ARRANGER));
                }
              if (gtk_widget_get_visible (
                    GTK_WIDGET (MW_AUDIO_ARRANGER)))
                {
                  arranger_widget_redraw_whole (
                    Z_ARRANGER_WIDGET (
                      MW_AUDIO_ARRANGER));
                }
            }
          break;
        case ET_CLIP_MARKER_POS_CHANGED:
          ruler_widget_redraw_whole (
            EDITOR_RULER);
          clip_editor_redraw_region (CLIP_EDITOR);
          break;
        case ET_TIMELINE_LOOP_MARKER_POS_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (MW_RULER));
          ruler_widget_redraw_whole (
            (RulerWidget *) MW_RULER);
          ruler_widget_redraw_whole (
            (RulerWidget *) EDITOR_RULER);
          redraw_all_arranger_bgs ();
          break;
        case ET_TIMELINE_SONG_MARKER_POS_CHANGED:
          gtk_widget_queue_allocate (
            GTK_WIDGET (MW_RULER));
          ruler_widget_redraw_whole (
            (RulerWidget *) MW_RULER);
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

          ruler_widget_refresh (
            (RulerWidget *) MW_RULER);
          ruler_widget_refresh (
            (RulerWidget *) EDITOR_RULER);
          timeline_minimap_widget_refresh (
            MW_TIMELINE_MINIMAP);
          break;
        case ET_AUTOMATION_VALUE_CHANGED:
          on_automation_value_changed (
            (Port *) ev->arg);
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
          on_playhead_changed (0);
          break;
        case ET_PLAYHEAD_POS_CHANGED_MANUALLY:
          on_playhead_changed (1);
          break;
        case ET_CLIP_EDITOR_REGION_CHANGED:
          /*on_clip_editor_region_changed ();*/
          break;
        case ET_TRACK_AUTOMATION_VISIBILITY_CHANGED:
          tracklist_widget_update_track_visibility (
            MW_TRACKLIST);
          break;
        case ET_TRACK_LANES_VISIBILITY_CHANGED:
          tracklist_widget_update_track_visibility (
            MW_TRACKLIST);
          /*arranger_widget_update_visibility (*/
            /*(ArrangerWidget *) MW_TIMELINE);*/
          /*arranger_widget_update_visibility (*/
            /*(ArrangerWidget *) MW_PINNED_TIMELINE);*/
          break;
        case ET_TRACK_ADDED:
          on_track_added ((Track *) ev->arg);
          tracklist_header_widget_refresh_track_count (
            MW_TRACKLIST_HEADER);
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
          tracklist_header_widget_refresh_track_count (
            MW_TRACKLIST_HEADER);
          break;
        case ET_TRACK_COLOR_CHANGED:
          on_track_color_changed ((Track *) ev->arg);
          break;
        case ET_TRACK_NAME_CHANGED:
          on_track_name_changed ((Track *) ev->arg);
          break;
        case ET_REFRESH_ARRANGER:
          /* remove the children of the pinned
           * timeline first because one of them
           * will be added to the unpinned
           * tracklist when unpinning */
          arranger_widget_redraw_whole (
            MW_PINNED_TIMELINE);

          if (MW_TIMELINE)
            arranger_widget_redraw_whole (
              MW_TIMELINE);
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
          tracklist_widget_update_track_visibility (
            MW_TRACKLIST);
          arranger_widget_redraw_whole (
            MW_TIMELINE);
          arranger_widget_redraw_whole (
            MW_PINNED_TIMELINE);
          track_visibility_tree_widget_refresh (
            MW_TRACK_VISIBILITY_TREE);
          tracklist_header_widget_refresh_track_count (
            MW_TRACKLIST_HEADER);
          break;
        case ET_UNDO_REDO_ACTION_DONE:
          home_toolbar_widget_refresh_undo_redo_buttons (
            MW_HOME_TOOLBAR);
          break;
        case ET_PIANO_ROLL_HIGHLIGHTING_CHANGED:
          if (MW_MIDI_EDITOR_SPACE)
            piano_roll_keys_widget_refresh (
              MW_PIANO_ROLL_KEYS);
          break;
        case ET_RULER_STATE_CHANGED:
          ruler_widget_refresh (
            (RulerWidget *) MW_RULER);
          break;
        case ET_AUTOMATION_TRACK_ADDED:
        case ET_AUTOMATION_TRACK_REMOVED:
        case ET_AUTOMATION_TRACK_CHANGED:
          on_automation_track_added (
            (AutomationTrack *) ev->arg);
          break;
        case ET_PLUGINS_ADDED:
          on_plugins_removed ((Channel *)ev->arg);
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

          /* remove the children of the pinned
           * tracklist first because one of them
           * will be added to the unpinned
           * tracklist when unpinning */
          /*z_gtk_container_remove_all_children (*/
            /*GTK_CONTAINER (MW_PINNED_TRACKLIST));*/

          if (MW_TRACKLIST)
            tracklist_widget_hard_refresh (
              MW_TRACKLIST);
          /*if (MW_PINNED_TRACKLIST)*/
            /*pinned_tracklist_widget_hard_refresh (*/
              /*MW_PINNED_TRACKLIST);*/

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
          tracklist_widget_update_track_visibility (
            MW_TRACKLIST);
          /*arranger_widget_update_visibility (*/
            /*(ArrangerWidget *) MW_TIMELINE);*/
          break;
        case ET_LOOP_TOGGLED:
          redraw_all_arranger_bgs ();
          ruler_widget_redraw_whole (
            (RulerWidget *) EDITOR_RULER);
          ruler_widget_redraw_whole (
            (RulerWidget *) MW_RULER);
          break;
        case ET_ARRANGER_SELECTIONS_IN_TRANSIT:
          on_arranger_selections_in_transit (
            (ArrangerSelections *) ev->arg);
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
        case ET_SELECTING_IN_ARRANGER:
          arranger_widget_redraw_whole (
            Z_ARRANGER_WIDGET (ev->arg));
          break;
        case ET_TRACKS_RESIZED:
          g_warn_if_fail (ev->arg);
          arranger_widget_redraw_whole (
            MW_TIMELINE);
          arranger_widget_redraw_whole (
            MW_PINNED_TIMELINE);
          break;
        case ET_CLIP_EDITOR_FIRST_TIME_REGION_SELECTED:
          gtk_widget_set_visible (
            GTK_WIDGET (MW_EDITOR_EVENT_VIEWER),
            g_settings_get_int (
              S_UI, "editor-event-viewer-visible"));
          break;
        case ET_PIANO_ROLL_MIDI_MODIFIER_CHANGED:
          arranger_widget_redraw_whole (
            (ArrangerWidget *)
            MW_MIDI_MODIFIER_ARRANGER);
          break;
        case ET_BPM_CHANGED:
          on_bpm_changed ();
          break;
        case ET_CHANNEL_FADER_VAL_CHANGED:
          channel_widget_redraw_fader (
            ((Channel *)ev->arg)->widget);
          break;
        case ET_PIANO_ROLL_KEY_HEIGHT_CHANGED:
          midi_editor_space_widget_refresh (
            MW_MIDI_EDITOR_SPACE);
          break;
        case ET_MAIN_WINDOW_LOADED:
          /* show all visible plugins */
          for (int j = 0; j < TRACKLIST->num_tracks;
               j++)
            {
              Channel * ch =
                TRACKLIST->tracks[j]->channel;
              if (!ch)
                continue;

              for (int k = 0; k < STRIP_SIZE; k++)
                {
                  Plugin * pl =
                    ch->plugins[k];
                  if (!pl)
                    continue;
                  if (pl->visible)
                    plugin_open_ui (pl);
                }
            }
          break;
        case ET_AUTOMATION_TRACKLIST_AT_REMOVED:
          /* TODO */
          break;
        case ET_TRIAL_LIMIT_REACHED:
          ui_show_message_full (
            GTK_WINDOW (MAIN_WINDOW),
            GTK_MESSAGE_INFO,
            _("Trial limit has been reached. "
            "Zrythm will now go silent"));
          break;
        default:
          g_warning (
            "event %d not implemented yet",
            ev->type);
          break;
        }

      object_pool_return (
        ZRYTHM->event_obj_pool, ev);
    }
  /*g_message ("processed %d events", i);*/

  if (num_events > 6)
    g_message ("More than 6 events processed. "
               "Optimization needed.");

  /*g_usleep (8000);*/
  /*project_sanity_check (PROJECT);*/

  return G_SOURCE_CONTINUE;
}

static void *
create_event_obj (void)
{
  return calloc (1, sizeof (ZEvent));
}

/**
 * Creates the event queue and starts the event loop.
 *
 * Must be called from a GTK thread.
 */
void
events_init (
  Zrythm * _zrythm)
{
  ObjectPool * obj_pool;
  MPMCQueue * queue;
  if (zrythm->event_queue &&
      zrythm->event_obj_pool)
    {
      obj_pool = zrythm->event_obj_pool;
      queue = zrythm->event_queue;
      zrythm->event_obj_pool = NULL;
      zrythm->event_queue = NULL;
      object_pool_free (obj_pool);
      mpmc_queue_free (queue);
    }

  obj_pool =
    object_pool_new (
      create_event_obj, EVENTS_MAX);
  queue = mpmc_queue_new ();
  mpmc_queue_reserve (
    queue,
    (size_t) EVENTS_MAX * sizeof (ZEvent *));

  zrythm->event_queue = queue;
  ZRYTHM->event_obj_pool = obj_pool;

  g_timeout_add (12, events_process, queue);
}
