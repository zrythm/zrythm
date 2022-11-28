// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <math.h>

#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/clip.h"
#include "audio/modulator_track.h"
#include "audio/pool.h"
#include "audio/router.h"
#include "audio/stretcher.h"
#include "audio/track.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/channel_sends_expander.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_key.h"
#include "gui/widgets/chord_pack_browser.h"
#include "gui/widgets/chord_pad_panel.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/editor_toolbar.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/header.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/modulator.h"
#include "gui/widgets/modulator_view.h"
#include "gui/widgets/monitor_section.h"
#include "gui/widgets/panel_file_browser.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/plugin_strip_expander.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/route_target_selector.h"
#include "gui/widgets/ruler_marker.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bot_box.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/timeline_toolbar.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_properties_expander.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/tracklist_header.h"
#include "gui/widgets/transport_controls.h"
#include "plugins/plugin_gtk.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/log.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/objects.h"
#include "utils/stack.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

static void
on_project_selection_type_changed (void)
{
  const char * class = "selected-element";
  const char * selectable_class = "selectable-element";

  z_gtk_widget_remove_style_class (
    GTK_WIDGET (MW_TRACKLIST), class);
  z_gtk_widget_remove_style_class (
    GTK_WIDGET (MW_TIMELINE_PANEL->timelines_plus_ruler),
    class);
  gtk_widget_add_css_class (
    GTK_WIDGET (MW_TIMELINE_PANEL->timelines_plus_ruler),
    selectable_class);
  z_gtk_widget_remove_style_class (
    GTK_WIDGET (MW_TIMELINE_PANEL->tracklist_top), class);
  gtk_widget_add_css_class (
    GTK_WIDGET (MW_TIMELINE_PANEL->tracklist_top),
    selectable_class);
  z_gtk_widget_remove_style_class (
    GTK_WIDGET (MW_CLIP_EDITOR_INNER), class);
  gtk_widget_add_css_class (
    GTK_WIDGET (MW_CLIP_EDITOR_INNER), selectable_class);
  z_gtk_widget_remove_style_class (
    GTK_WIDGET (MW_MIXER), class);
  gtk_widget_add_css_class (
    GTK_WIDGET (MW_MIXER), selectable_class);

  switch (PROJECT->last_selection)
    {
    case SELECTION_TYPE_TRACKLIST:
      gtk_widget_add_css_class (
        GTK_WIDGET (MW_TIMELINE_PANEL->tracklist_top), class);
      z_gtk_widget_remove_style_class (
        GTK_WIDGET (MW_TIMELINE_PANEL->tracklist_top),
        selectable_class);
      gtk_widget_add_css_class (GTK_WIDGET (MW_MIXER), class);
      z_gtk_widget_remove_style_class (
        GTK_WIDGET (MW_MIXER), selectable_class);
      break;
    case SELECTION_TYPE_TIMELINE:
      gtk_widget_add_css_class (
        GTK_WIDGET (MW_TIMELINE_PANEL->timelines_plus_ruler),
        class);
      z_gtk_widget_remove_style_class (
        GTK_WIDGET (MW_TIMELINE_PANEL->timelines_plus_ruler),
        selectable_class);
      break;
    case SELECTION_TYPE_INSERT:
    case SELECTION_TYPE_MIDI_FX:
    case SELECTION_TYPE_INSTRUMENT:
    case SELECTION_TYPE_MODULATOR:
      break;
    case SELECTION_TYPE_EDITOR:
      gtk_widget_add_css_class (
        GTK_WIDGET (MW_CLIP_EDITOR_INNER), class);
      z_gtk_widget_remove_style_class (
        GTK_WIDGET (MW_CLIP_EDITOR_INNER), selectable_class);
      break;
    }
}

static void
on_arranger_selections_in_transit (ArrangerSelections * sel)
{
  g_return_if_fail (sel);

  event_viewer_widget_refresh_for_selections (sel);
}

/**
 * @param manually Whether the position was changed
 *   by the user.
 */
static void
on_playhead_changed (bool manually)
{
  if (MAIN_WINDOW)
    {
      if (MW_DIGITAL_TRANSPORT)
        {
          gtk_widget_queue_draw (
            GTK_WIDGET (MW_DIGITAL_TRANSPORT));
        }
      if (MW_MIDI_EDITOR_SPACE)
        {
          piano_roll_keys_widget_refresh (MW_PIANO_ROLL_KEYS);
        }
#if 0
      if (manually)
        {
          arranger_widget_foreach (
            foreach_arranger_handle_playhead_auto_scroll);
        }
#endif
    }
}

static void
on_channel_output_changed (Channel * ch)
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
  Channel * chan = track_get_channel (track);
  if (chan && chan->widget)
    {
      channel_widget_refresh (chan->widget);
    }

  if (TRACKLIST_SELECTIONS->tracks[0] == track)
    {
      inspector_track_widget_show_tracks (
        MW_TRACK_INSPECTOR, TRACKLIST_SELECTIONS, true);
    }
}

static void
on_automation_track_added (AutomationTrack * at)
{
  /*AutomationTracklist * atl =*/
  /*track_get_automation_tracklist (at->track);*/
  /*if (atl && atl->widget)*/
  /*automation_tracklist_widget_refresh (*/
  /*atl->widget);*/

  Track * track = automation_track_get_track (at);
  g_return_if_fail (track);
  if (Z_IS_TRACK_WIDGET (track->widget))
    {
      TrackWidget * tw = (TrackWidget *) track->widget;
      track_widget_update_size (tw);
    }
}

static void
on_track_added (void)
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

static void
on_automation_value_changed (Port * port)
{
  PortIdentifier * id = &port->id;

  if (id->flags2 & PORT_FLAG2_CHANNEL_SEND_AMOUNT)
    {
      Track * tr = port_get_track (port, true);
      if (track_is_selected (tr))
        {
          gtk_widget_queue_draw (GTK_WIDGET (
            MW_TRACK_INSPECTOR->sends->slots[id->port_index]));
        }
    }
}

static void
on_plugin_added (Plugin * plugin)
{
  /*Track * track =*/
  /*plugin_get_track (plugin);*/
}

static void
on_plugin_crashed (Plugin * plugin)
{
  char * str = g_strdup_printf (
    _ ("Plugin '%s' has crashed and has been "
       "disabled."),
    plugin->setting->descr->name);
  ui_show_error_message (true, str);
  g_free (str);
}

static void
on_plugin_state_changed (Plugin * pl)
{
  Track * track = plugin_get_track (pl);
  if (track && track->channel && track->channel->widget)
    {
      /* redraw slot */
      switch (pl->id.slot_type)
        {
        case PLUGIN_SLOT_MIDI_FX:
          plugin_strip_expander_widget_redraw_slot (
            MW_TRACK_INSPECTOR->midi_fx, pl->id.slot);
          break;
        case PLUGIN_SLOT_INSERT:
          plugin_strip_expander_widget_redraw_slot (
            MW_TRACK_INSPECTOR->inserts, pl->id.slot);
          plugin_strip_expander_widget_redraw_slot (
            track->channel->widget->inserts, pl->id.slot);
          break;
        default:
          break;
        }
    }
}

static void
on_modulator_added (Plugin * modulator)
{
  on_plugin_added (modulator);

  Track * track = plugin_get_track (modulator);
  modulator_view_widget_refresh (MW_MODULATOR_VIEW, track);
}

static void
on_plugins_removed (Track * tr)
{
  /* redraw slots */
  if (tr && tr->channel && tr->channel->widget)
    {
      plugin_strip_expander_widget_set_state_flags (
        tr->channel->widget->inserts, -1,
        GTK_STATE_FLAG_SELECTED, false);
    }

  /* change inspector page */
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);

  /* refresh modulator view */
  modulator_view_widget_refresh (
    MW_MODULATOR_VIEW, P_MODULATOR_TRACK);
}

static void
refresh_for_selections_type (ArrangerSelectionsType type)
{
  switch (type)
    {
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      event_viewer_widget_refresh (
        MW_TIMELINE_EVENT_VIEWER, false);
      break;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      event_viewer_widget_refresh (
        MW_MIDI_EVENT_VIEWER, false);
      break;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      event_viewer_widget_refresh (
        MW_CHORD_EVENT_VIEWER, false);
      break;
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      event_viewer_widget_refresh (
        MW_AUTOMATION_EVENT_VIEWER, false);
      break;
    case ARRANGER_SELECTIONS_TYPE_AUDIO:
      event_viewer_widget_refresh (
        MW_AUDIO_EVENT_VIEWER, false);
      break;
    default:
      g_return_if_reached ();
    }
  bot_dock_edge_widget_update_event_viewer_stack_page (
    MW_BOT_DOCK_EDGE);
}

static void
on_arranger_selections_changed (ArrangerSelections * sel)
{
  refresh_for_selections_type (sel->type);
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);

  timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
}

static void
arranger_selections_change_redraw_everything (
  ArrangerSelections * sel)
{
  switch (sel->type)
    {
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      event_viewer_widget_refresh (
        MW_TIMELINE_EVENT_VIEWER, false);
      break;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      event_viewer_widget_refresh (
        MW_MIDI_EVENT_VIEWER, false);
      break;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      event_viewer_widget_refresh (
        MW_CHORD_EVENT_VIEWER, false);
      break;
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      event_viewer_widget_refresh (
        MW_AUTOMATION_EVENT_VIEWER, false);
      break;
    case ARRANGER_SELECTIONS_TYPE_AUDIO:
      event_viewer_widget_refresh (
        MW_AUDIO_EVENT_VIEWER, false);
      break;
    default:
      g_return_if_reached ();
    }
  bot_dock_edge_widget_update_event_viewer_stack_page (
    MW_BOT_DOCK_EDGE);
}

static void
on_arranger_selections_created (ArrangerSelections * sel)
{
  arranger_selections_change_redraw_everything (sel);
}

static void
on_arranger_selections_moved (ArrangerSelections * sel)
{
  arranger_selections_change_redraw_everything (sel);
}

static void
on_arranger_selections_removed (ArrangerSelections * sel)
{
  MW_TIMELINE->hovered_object = NULL;
  MW_MIDI_ARRANGER->hovered_object = NULL;
  MW_MIDI_MODIFIER_ARRANGER->hovered_object = NULL;
  MW_AUTOMATION_ARRANGER->hovered_object = NULL;
  MW_AUDIO_ARRANGER->hovered_object = NULL;
  MW_CHORD_ARRANGER->hovered_object = NULL;

  timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
}

static void
on_mixer_selections_changed (void)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (!track_type_has_channel (track->type))
        continue;

      Channel * ch = track->channel;
      if (ch->widget)
        {
          plugin_strip_expander_widget_refresh (
            ch->widget->inserts);
        }
    }
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
}

static void
on_track_color_changed (Track * track)
{
  if (track_type_has_channel (track->type))
    {
      if (track->channel->widget)
        {
          channel_widget_refresh (track->channel->widget);
        }
    }
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
}

static void
on_track_name_changed (Track * track)
{
  /* refresh all because tracks routed to/from are
   * also affected */
  mixer_widget_soft_refresh (MW_MIXER);
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
}

static void
on_arranger_object_changed (ArrangerObject * obj)
{
  g_return_if_fail (IS_ARRANGER_OBJECT (obj));

  ArrangerSelections * sel =
    arranger_object_get_selections_for_type (obj->type);
  event_viewer_widget_refresh_for_selections (sel);

  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      /* redraw editor ruler if region
       * positions were changed */
      timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
      break;
    default:
      break;
    }
}

static void
on_track_changed (Track * track)
{
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));
  if (GTK_IS_WIDGET (track->widget))
    {
      if (
        gtk_widget_get_visible (GTK_WIDGET (track->widget))
        != track->visible)
        {
          gtk_widget_set_visible (
            GTK_WIDGET (track->widget), track->visible);
        }
    }
}

static void
on_plugin_window_visibility_changed (Plugin * pl)
{
  g_message ("start");
  if (!IS_PLUGIN (pl) || pl->deleting)
    {
      return;
    }

  Track * track = plugin_get_track (pl);

  if (
    track && track->channel && track->channel->widget
    && Z_IS_CHANNEL_WIDGET (track->channel->widget))
    {
      /* redraw slot */
      switch (pl->id.slot_type)
        {
        case PLUGIN_SLOT_MIDI_FX:
          plugin_strip_expander_widget_redraw_slot (
            MW_TRACK_INSPECTOR->midi_fx, pl->id.slot);
          break;
        case PLUGIN_SLOT_INSERT:
          plugin_strip_expander_widget_redraw_slot (
            MW_TRACK_INSPECTOR->inserts, pl->id.slot);
          plugin_strip_expander_widget_redraw_slot (
            track->channel->widget->inserts, pl->id.slot);
          break;
        case PLUGIN_SLOT_INSTRUMENT:
          track_properties_expander_widget_refresh (
            MW_TRACK_INSPECTOR->track_info, track);
          break;
        default:
          break;
        }
    }

  if (pl->modulator_widget)
    {
      modulator_widget_refresh (pl->modulator_widget);
    }
  g_message ("done");
}

static void
on_plugin_visibility_changed (Plugin * pl)
{
  g_debug ("start - visible: %d", pl->visible);
  if (pl->visible)
    {
      plugin_open_ui (pl);
    }
  else if (!pl->visible)
    {
      plugin_close_ui (pl);
    }

  on_plugin_window_visibility_changed (pl);
  g_debug ("done");
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
  EventManager * self,
  GPtrArray *    events_arr)
{
  MPMCQueue * q = self->mqueue;
  ZEvent *    event;

  g_ptr_array_remove_range (events_arr, 0, events_arr->len);

  /* only add events once to new array while
   * popping */
  while (event_queue_dequeue_event (q, &event))
    {
      bool already_exists = false;

      for (guint i = 0; i < events_arr->len; i++)
        {
          ZEvent * cur_event =
            (ZEvent *) g_ptr_array_index (events_arr, i);
          if (
            event->type == cur_event->type
            && event->arg == cur_event->arg)
            already_exists = true;
        }

      if (already_exists)
        {
          object_pool_return (self->obj_pool, event);
        }
      else
        {
          g_ptr_array_add (events_arr, event);
        }
    }
}

static int
soft_recalc_graph_when_paused (void * data)
{
  EventManager * self = (EventManager *) data;
  if (TRANSPORT->play_state == PLAYSTATE_PAUSED)
    {
      router_recalc_graph (ROUTER, F_SOFT);
      self->pending_soft_recalc = false;
      return G_SOURCE_REMOVE;
    }
  return G_SOURCE_CONTINUE;
}

/**
 * Processes the given event.
 *
 * The caller is responsible for putting the event
 * back in the object pool if needed.
 */
void
event_manager_process_event (EventManager * self, ZEvent * ev)
{
  switch (ev->type)
    {
    case ET_PLUGIN_LATENCY_CHANGED:
      if (!self->pending_soft_recalc)
        {
          self->pending_soft_recalc = true;
          g_idle_add (soft_recalc_graph_when_paused, self);
        }
      break;
    case ET_TRACKS_REMOVED:
      if (MW_MIXER)
        mixer_widget_hard_refresh (MW_MIXER);
      if (MW_TRACKLIST)
        tracklist_widget_hard_refresh (MW_TRACKLIST);
      tracklist_header_widget_refresh_track_count (
        MW_TRACKLIST_HEADER);
      left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
      break;
    case ET_CHANNEL_REMOVED:
      mixer_widget_hard_refresh (MW_MIXER);
      break;
    case ET_ARRANGER_OBJECT_CREATED:
      break;
    case ET_ARRANGER_OBJECT_CHANGED:
      on_arranger_object_changed ((ArrangerObject *) ev->arg);
      break;
    case ET_ARRANGER_OBJECT_REMOVED:
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
    case ET_ARRANGER_SELECTIONS_QUANTIZED:
      break;
    case ET_ARRANGER_SELECTIONS_ACTION_FINISHED:
      break;
    case ET_TRACKLIST_SELECTIONS_CHANGED:
      /* only refresh the inspector if the
       * tracklist selection changed by
       * clicking on a track */
      if (
        PROJECT->last_selection == SELECTION_TYPE_TRACKLIST
        || PROJECT->last_selection == SELECTION_TYPE_INSERT
        || PROJECT->last_selection == SELECTION_TYPE_MIDI_FX)
        {
          left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
        }
      mixer_widget_soft_refresh (MW_MIXER);
      tracklist_widget_soft_refresh (MW_TRACKLIST);
      break;
    case ET_RULER_SIZE_CHANGED:
      {
#if 0
        RulerWidget * ruler = Z_RULER_WIDGET (ev->arg);
#endif
      }
      break;
    case ET_CLIP_MARKER_POS_CHANGED:
      break;
    case ET_TIMELINE_LOOP_MARKER_POS_CHANGED:
    case ET_TIMELINE_PUNCH_MARKER_POS_CHANGED:
      break;
    case ET_TIMELINE_SONG_MARKER_POS_CHANGED:
      break;
    case ET_PLUGIN_VISIBILITY_CHANGED:
      on_plugin_visibility_changed ((Plugin *) ev->arg);
      break;
    case ET_PLUGIN_WINDOW_VISIBILITY_CHANGED:
      on_plugin_window_visibility_changed ((Plugin *) ev->arg);
      break;
    case ET_PLUGIN_STATE_CHANGED:
      {
        Plugin * pl = (Plugin *) ev->arg;
        if (IS_PLUGIN (pl))
          {
            on_plugin_state_changed (pl);
            g_atomic_int_set (
              &pl->state_changed_event_sent, 0);
          }
      }
      break;
    case ET_TRANSPORT_TOTAL_BARS_CHANGED:
      ruler_widget_refresh ((RulerWidget *) MW_RULER);
      ruler_widget_refresh ((RulerWidget *) EDITOR_RULER);
      timeline_minimap_widget_refresh (MW_TIMELINE_MINIMAP);
      break;
    case ET_AUTOMATION_VALUE_CHANGED:
      on_automation_value_changed ((Port *) ev->arg);
      break;
    case ET_RANGE_SELECTION_CHANGED:
      timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
      break;
    case ET_TOOL_CHANGED:
      toolbox_widget_refresh (MW_TOOLBOX);
      arranger_widget_refresh_cursor (
        Z_ARRANGER_WIDGET (MW_TIMELINE));
      if (
        MW_MIDI_ARRANGER
        && gtk_widget_get_realized (
          GTK_WIDGET (MW_MIDI_ARRANGER)))
        arranger_widget_refresh_cursor (
          Z_ARRANGER_WIDGET (MW_MIDI_ARRANGER));
      if (
        MW_MIDI_MODIFIER_ARRANGER
        && gtk_widget_get_realized (
          GTK_WIDGET (MW_MIDI_MODIFIER_ARRANGER)))
        arranger_widget_refresh_cursor (
          Z_ARRANGER_WIDGET (MW_MIDI_MODIFIER_ARRANGER));
      break;
    case ET_TIME_SIGNATURE_CHANGED:
      ruler_widget_refresh (Z_RULER_WIDGET (MW_RULER));
      ruler_widget_refresh (Z_RULER_WIDGET (EDITOR_RULER));
      gtk_widget_queue_draw (GTK_WIDGET (MW_DIGITAL_TIME_SIG));
      break;
    case ET_PLAYHEAD_POS_CHANGED:
      on_playhead_changed (false);
      break;
    case ET_PLAYHEAD_POS_CHANGED_MANUALLY:
      on_playhead_changed (true);
      break;
    case ET_CLIP_EDITOR_REGION_CHANGED:
      clip_editor_widget_on_region_changed (MW_CLIP_EDITOR);
      PIANO_ROLL->num_current_notes = 0;
      piano_roll_keys_widget_redraw_full (MW_PIANO_ROLL_KEYS);
      bot_dock_edge_widget_update_event_viewer_stack_page (
        MW_BOT_DOCK_EDGE);
      break;
    case ET_TRACK_AUTOMATION_VISIBILITY_CHANGED:
      {
        Track * track = (Track *) ev->arg;
        if (!IS_TRACK_AND_NONNULL (track))
          {
            g_critical ("expected track argument");
            break;
          }

        if (GTK_IS_WIDGET (track->widget))
          {
            track_widget_update_icons (track->widget);
            track_widget_update_size (track->widget);
          }
      }
      break;
    case ET_TRACK_LANES_VISIBILITY_CHANGED:
      tracklist_widget_update_track_visibility (MW_TRACKLIST);
      on_track_added ();
      break;
    case ET_TRACK_ADDED:
      on_track_added ();
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
        tracklist_widget_hard_refresh (MW_TRACKLIST);
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
      break;
    case ET_RULER_VIEWPORT_CHANGED:
      timeline_minimap_widget_refresh (MW_TIMELINE_MINIMAP);
      ruler_widget_refresh (Z_RULER_WIDGET (ev->arg));
      break;
    case ET_TRACK_STATE_CHANGED:
      for (int j = 0; j < TRACKLIST->num_tracks; j++)
        {
          on_track_state_changed (TRACKLIST->tracks[j]);
        }
      monitor_section_widget_refresh (MW_MONITOR_SECTION);
      break;
    case ET_TRACK_VISIBILITY_CHANGED:
      tracklist_widget_update_track_visibility (MW_TRACKLIST);
      tracklist_header_widget_refresh_track_count (
        MW_TRACKLIST_HEADER);
      mixer_widget_hard_refresh (MW_MIXER);
      break;
    case ET_UNDO_REDO_ACTION_DONE:
      home_toolbar_widget_refresh_undo_redo_buttons (
        MW_HOME_TOOLBAR);
      break;
    case ET_PIANO_ROLL_HIGHLIGHTING_CHANGED:
      if (MW_MIDI_EDITOR_SPACE)
        piano_roll_keys_widget_refresh (MW_PIANO_ROLL_KEYS);
      break;
    case ET_PIANO_ROLL_KEY_ON_OFF:
      piano_roll_keys_widget_redraw_full (MW_PIANO_ROLL_KEYS);
      break;
    case ET_RULER_STATE_CHANGED:
      ruler_widget_refresh ((RulerWidget *) MW_RULER);
      break;
    case ET_AUTOMATION_TRACK_ADDED:
    case ET_AUTOMATION_TRACK_REMOVED:
    case ET_AUTOMATION_TRACK_CHANGED:
      on_automation_track_added ((AutomationTrack *) ev->arg);
      break;
    case ET_PLUGINS_ADDED:
      on_plugins_removed ((Track *) ev->arg);
      break;
    case ET_PLUGIN_ADDED:
      on_plugin_added ((Plugin *) ev->arg);
      break;
    case ET_PLUGIN_CRASHED:
      on_plugin_crashed ((Plugin *) ev->arg);
      break;
    case ET_PLUGINS_REMOVED:
      on_plugins_removed ((Track *) ev->arg);
      break;
    case ET_MIXER_SELECTIONS_CHANGED:
      on_mixer_selections_changed ();
      break;
    case ET_CHANNEL_OUTPUT_CHANGED:
      on_channel_output_changed ((Channel *) ev->arg);
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
        tracklist_widget_hard_refresh (MW_TRACKLIST);
      /*if (MW_PINNED_TRACKLIST)*/
      /*pinned_tracklist_widget_hard_refresh (*/
      /*MW_PINNED_TRACKLIST);*/

      /* needs to be called later because tracks
       * need time to get allocated */
      EVENTS_PUSH (ET_REFRESH_ARRANGER, NULL);
      break;
    case ET_CHANNEL_SLOTS_CHANGED:
      {
        Channel *       ch = (Channel *) ev->arg;
        ChannelWidget * cw = ch ? ch->widget : NULL;
        if (cw)
          {
            channel_widget_update_midi_fx_and_inserts (cw);
          }
      }
      break;
    case ET_DRUM_MODE_CHANGED:
      midi_editor_space_widget_refresh (MW_MIDI_EDITOR_SPACE);
      break;
    case ET_MODULATOR_ADDED:
      on_modulator_added ((Plugin *) ev->arg);
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
      tracklist_widget_update_track_visibility (MW_TRACKLIST);
      /*arranger_widget_update_visibility (*/
      /*(ArrangerWidget *) MW_TIMELINE);*/
      break;
    case ET_LOOP_TOGGLED:
      transport_controls_widget_refresh (
        MW_TRANSPORT_CONTROLS);
      break;
    case ET_ARRANGER_SELECTIONS_IN_TRANSIT:
      on_arranger_selections_in_transit (
        (ArrangerSelections *) ev->arg);
      break;
    case ET_CHORD_KEY_CHANGED:
      for (int j = 0; j < CHORD_EDITOR->num_chords; j++)
        {
          if (
            CHORD_EDITOR->chords[j]
            == (ChordDescriptor *) ev->arg)
            {
              chord_key_widget_refresh (
                MW_CHORD_EDITOR_SPACE->chord_keys[j]);
            }
        }
      chord_pad_panel_widget_refresh (MW_CHORD_PAD_PANEL);
      break;
    case ET_CHORD_PRESET_ADDED:
    case ET_CHORD_PRESET_EDITED:
    case ET_CHORD_PRESET_REMOVED:
      chord_pack_browser_widget_refresh_presets (
        MW_CHORD_PACK_BROWSER);
      chord_pad_panel_widget_refresh_load_preset_menu (
        MW_CHORD_PAD_PANEL);
      break;
    case ET_CHORD_PRESET_PACK_ADDED:
    case ET_CHORD_PRESET_PACK_EDITED:
    case ET_CHORD_PRESET_PACK_REMOVED:
      chord_pack_browser_widget_refresh_packs (
        MW_CHORD_PACK_BROWSER);
      chord_pack_browser_widget_refresh_presets (
        MW_CHORD_PACK_BROWSER);
      chord_pad_panel_widget_refresh_load_preset_menu (
        MW_CHORD_PAD_PANEL);
      break;
    case ET_CHORDS_UPDATED:
      chord_pad_panel_widget_refresh (MW_CHORD_PAD_PANEL);
      chord_editor_space_widget_refresh_chords (
        MW_CHORD_EDITOR_SPACE);
      break;
    case ET_JACK_TRANSPORT_TYPE_CHANGED:
      g_message ("doing");
      top_bar_widget_refresh (TOP_BAR);
      break;
    case ET_SELECTING_IN_ARRANGER:
      {
        ArrangerWidget * arranger =
          Z_ARRANGER_WIDGET (ev->arg);
        event_viewer_widget_refresh_for_arranger (
          arranger, true);
        timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
      }
      break;
    case ET_TRACKS_RESIZED:
      g_warn_if_fail (ev->arg);
      break;
    case ET_CLIP_EDITOR_FIRST_TIME_REGION_SELECTED:
      gtk_widget_set_visible (
        GTK_WIDGET (MW_EDITOR_EVENT_VIEWER_STACK),
        g_settings_get_boolean (
          S_UI, "editor-event-viewer-visible"));
      bot_dock_edge_widget_update_event_viewer_stack_page (
        MW_BOT_DOCK_EDGE);
      break;
    case ET_PIANO_ROLL_MIDI_MODIFIER_CHANGED:
      break;
    case ET_BPM_CHANGED:
      ruler_widget_refresh (MW_RULER);
      ruler_widget_refresh (EDITOR_RULER);
      gtk_widget_queue_draw (GTK_WIDGET (MW_DIGITAL_BPM));

      /* these are only used in the UI so
       * no need to update them during DSP */
      quantize_options_update_quantize_points (
        QUANTIZE_OPTIONS_TIMELINE);
      quantize_options_update_quantize_points (
        QUANTIZE_OPTIONS_EDITOR);
      break;
    case ET_CHANNEL_FADER_VAL_CHANGED:
      channel_widget_redraw_fader (
        ((Channel *) ev->arg)->widget);
#if 0
      inspector_track_widget_show_tracks (
        MW_TRACK_INSPECTOR, TRACKLIST_SELECTIONS);
#endif
      break;
    case ET_PIANO_ROLL_KEY_ZOOM_CHANGED:
      midi_editor_space_widget_refresh (MW_MIDI_EDITOR_SPACE);
      break;
    case ET_PIANO_ROLL_KEY_HEIGHT_CHANGED:
      {
        gtk_widget_set_size_request (
          GTK_WIDGET (MW_PIANO_ROLL_KEYS), 10,
          (int) MW_PIANO_ROLL_KEYS->total_key_px);
      }
      break;
    case ET_MAIN_WINDOW_LOADED:
      /* show all visible plugins */
      for (int j = 0; j < TRACKLIST->num_tracks; j++)
        {
          Channel * ch = TRACKLIST->tracks[j]->channel;
          if (!ch)
            continue;

          for (int k = 0; k < STRIP_SIZE * 2 + 1; k++)
            {
              Plugin * pl = NULL;
              if (k < STRIP_SIZE)
                pl = ch->midi_fx[k];
              else if (k == STRIP_SIZE)
                pl = ch->instrument;
              else
                pl = ch->inserts[k - (STRIP_SIZE + 1)];

              if (!pl)
                continue;
              if (pl->visible)
                plugin_open_ui (pl);
            }
        }
      /* refresh modulator view */
      if (MW_MODULATOR_VIEW)
        {
          modulator_view_widget_refresh (
            MW_MODULATOR_VIEW, P_MODULATOR_TRACK);
        }

      /* show clip editor if clip selected */
      if (MW_CLIP_EDITOR && CLIP_EDITOR->has_region)
        {
          clip_editor_widget_on_region_changed (
            MW_CLIP_EDITOR);
        }

      /* refresh inspector */
      left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
      on_project_selection_type_changed ();
      main_notebook_widget_refresh (MW_MAIN_NOTEBOOK);

#ifdef CHECK_UPDATES
      zrythm_app_check_for_updates (zrythm_app);
#endif /* CHECK_UPDATES */

      /* show any pending messages */
      {
        ZrythmAppUiMessage * ui_msg;
        while (
          (ui_msg = (ZrythmAppUiMessage *) g_async_queue_try_pop (
             zrythm_app->project_load_message_queue))
          != NULL)
          {
            ui_show_message_printf (
              ui_msg->type, false, "%s", ui_msg->msg);
            zrythm_app_ui_message_free (ui_msg);
          }
      }
      break;
    case ET_SPLASH_CLOSED:
      break;
    case ET_PROJECT_SAVED:
      main_window_widget_set_project_title (
        MAIN_WINDOW, (Project *) ev->arg);
      break;
    case ET_PROJECT_LOADED:
      main_window_widget_set_project_title (
        MAIN_WINDOW, (Project *) ev->arg);
      home_toolbar_widget_refresh_undo_redo_buttons (
        MW_HOME_TOOLBAR);
      ruler_widget_set_zoom_level (
        MW_RULER, ruler_widget_get_zoom_level (MW_RULER));
      ruler_widget_set_zoom_level (
        EDITOR_RULER,
        ruler_widget_get_zoom_level (EDITOR_RULER));
      break;
    case ET_AUTOMATION_TRACKLIST_AT_REMOVED:
      /* TODO */
      break;
    case ET_CHANNEL_SEND_CHANGED:
      {
        ChannelSend *       send = (ChannelSend *) ev->arg;
        ChannelSendWidget * widget =
          channel_send_find_widget (send);
        if (widget)
          {
            gtk_widget_queue_draw (GTK_WIDGET (widget));
          }
        Track *         tr = channel_send_get_track (send);
        ChannelWidget * ch_w = tr->channel->widget;
        if (ch_w)
          {
            gtk_widget_queue_draw (
              GTK_WIDGET (ch_w->sends->slots[send->slot]));
          }
      }
      break;
    case ET_RULER_DISPLAY_TYPE_CHANGED:
      break;
    case ET_ARRANGER_HIGHLIGHT_CHANGED:
      break;
    case ET_ENGINE_ACTIVATE_CHANGED:
    case ET_ENGINE_BUFFER_SIZE_CHANGED:
    case ET_ENGINE_SAMPLE_RATE_CHANGED:
      if (!gtk_widget_in_destruction (GTK_WIDGET (MAIN_WINDOW)))
        {
          bot_bar_widget_refresh (MW_BOT_BAR);
          inspector_track_widget_show_tracks (
            MW_TRACK_INSPECTOR, TRACKLIST_SELECTIONS, false);
        }
      break;
    case ET_MIDI_BINDINGS_CHANGED:
      main_notebook_widget_refresh (MW_MAIN_NOTEBOOK);
      break;
    case ET_PORT_CONNECTION_CHANGED:
      main_notebook_widget_refresh (MW_MAIN_NOTEBOOK);
      break;
    case ET_EDITOR_FUNCTION_APPLIED:
      editor_toolbar_widget_refresh (MW_EDITOR_TOOLBAR);
      break;
    case ET_ARRANGER_SELECTIONS_CHANGED_REDRAW_EVERYTHING:
      arranger_selections_change_redraw_everything (
        ARRANGER_SELECTIONS (ev->arg));
      break;
    case ET_AUTOMATION_VALUE_VISIBILITY_CHANGED:
      break;
    case ET_PROJECT_SELECTION_TYPE_CHANGED:
      on_project_selection_type_changed ();
      break;
    case ET_AUDIO_SELECTIONS_RANGE_CHANGED:
      break;
    case ET_PLUGIN_COLLETIONS_CHANGED:
      plugin_browser_widget_refresh_collections (
        MW_PLUGIN_BROWSER);
      break;
    case ET_SNAP_GRID_OPTIONS_CHANGED:
      {
        SnapGrid * sg = (SnapGrid *) ev->arg;
        if (sg == SNAP_GRID_TIMELINE)
          {
            snap_grid_widget_refresh (
              MW_TIMELINE_TOOLBAR->snap_grid);
          }
        else if (sg == SNAP_GRID_EDITOR)
          {
            snap_grid_widget_refresh (
              MW_EDITOR_TOOLBAR->snap_grid);
          }
      }
      break;
    case ET_TRANSPORT_RECORDING_ON_OFF_CHANGED:
      gtk_toggle_button_set_active (
        MW_TRANSPORT_CONTROLS->trans_record_btn,
        TRANSPORT->recording);
      break;
    case ET_TRACK_FREEZE_CHANGED:
      arranger_selections_change_redraw_everything (
        (ArrangerSelections *) TL_SELECTIONS);
      break;
    case ET_LOG_WARNING_STATE_CHANGED:
      header_widget_refresh (MW_HEADER);
      break;
    case ET_PLAYHEAD_SCROLL_MODE_CHANGED:
      break;
    case ET_TRACK_FADER_BUTTON_CHANGED:
      on_track_state_changed ((Track *) ev->arg);
      break;
    case ET_PLUGIN_PRESET_SAVED:
    case ET_PLUGIN_PRESET_LOADED:
      {
        Plugin * pl = (Plugin *) ev->arg;
        if (pl->window)
          {
            plugin_gtk_set_window_title (pl, pl->window);
          }

        switch (ev->type)
          {
          case ET_PLUGIN_PRESET_SAVED:
            ui_show_notification (_ ("Plugin preset saved."));
            break;
          case ET_PLUGIN_PRESET_LOADED:
            ui_show_notification (_ ("Plugin preset loaded."));
            break;
          default:
            break;
          }
      }
      break;
    case ET_TRACK_FOLD_CHANGED:
      on_track_added ();
      break;
    case ET_MIXER_CHANNEL_INSERTS_EXPANDED_CHANGED:
    case ET_MIXER_CHANNEL_MIDI_FX_EXPANDED_CHANGED:
    case ET_MIXER_CHANNEL_SENDS_EXPANDED_CHANGED:
      mixer_widget_soft_refresh (MW_MIXER);
      break;
    case ET_REGION_ACTIVATED:
      bot_dock_edge_widget_show_clip_editor (
        MW_BOT_DOCK_EDGE, true);
      break;
    case ET_VELOCITIES_RAMPED:
      break;
    case ET_AUDIO_REGION_FADE_IN_CHANGED:
      break;
    case ET_AUDIO_REGION_FADE_OUT_CHANGED:
      break;
    case ET_AUDIO_REGION_GAIN_CHANGED:
      break;
    case ET_FILE_BROWSER_BOOKMARK_ADDED:
    case ET_FILE_BROWSER_BOOKMARK_DELETED:
      panel_file_browser_refresh_bookmarks (
        MW_PANEL_FILE_BROWSER);
      break;
    case ET_ARRANGER_SCROLLED:
      {
        ArrangerWidget * arranger =
          Z_ARRANGER_WIDGET (ev->arg);
        ArrangerSelections * sel =
          arranger_widget_get_selections (arranger);
        arranger_selections_change_redraw_everything (sel);
      }
      break;
    case ET_FILE_BROWSER_INSTRUMENT_CHANGED:
      /* TODO */
#if 0
      chord_pack_browser_widget_refresh_auditioner_controls (
        MW_CHORD_PACK_BROWSER);
#endif
      break;
    case ET_TRANSPORT_ROLL_REQUIRED:
      transport_request_roll (TRANSPORT, true);
      break;
    case ET_TRANSPORT_PAUSE_REQUIRED:
      transport_request_pause (TRANSPORT, true);
      break;
    case ET_TRANSPORT_MOVE_BACKWARD_REQUIRED:
      transport_move_backward (TRANSPORT, true);
      break;
    case ET_TRANSPORT_MOVE_FORWARD_REQUIRED:
      transport_move_forward (TRANSPORT, true);
      break;
    case ET_TRANSPORT_TOGGLE_LOOP_REQUIRED:
      transport_set_loop (TRANSPORT, !TRANSPORT->loop, true);
      break;
    case ET_TRANSPORT_TOGGLE_RECORDING_REQUIRED:
      transport_set_recording (
        TRANSPORT, !TRANSPORT->recording, true,
        F_PUBLISH_EVENTS);
      break;
    default:
      g_warning ("event %d not implemented yet", ev->type);
      break;
    }
}

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 */
static int
process_events (void * data)
{
  EventManager * self = (EventManager *) data;

  ZEvent * ev;
  clean_duplicates_and_copy (self, self->events_arr);

  /*g_message ("starting processing");*/
  for (guint i = 0; i < self->events_arr->len; i++)
    {
      if (i > 30)
        {
          g_message (
            "more than 30 UI events processed "
            "(%d)!",
            i);
        }

      ev = (ZEvent *) g_ptr_array_index (self->events_arr, i);

      if (!ZRYTHM_HAVE_UI)
        {
          g_message ("%s: (%d) No UI, skipping", __func__, i);
          goto return_to_pool;
        }

      /*g_message ("event type %d", ev->type);*/

      event_manager_process_event (self, ev);

return_to_pool:
      object_pool_return (self->obj_pool, ev);
    }
  /*g_message ("processed %d events", i);*/

  if (self->events_arr->len > 6)
    g_message (
      "More than 6 events processed. "
      "Optimization needed.");

  /*g_usleep (8000);*/
  /*project_validate (PROJECT);*/

  return G_SOURCE_CONTINUE;
}

/**
 * Starts accepting events.
 */
void
event_manager_start_events (EventManager * self)
{
  g_message ("%s: starting...", __func__);

  if (self->process_source_id)
    {
      g_message ("%s: already processing events", __func__);
      return;
    }

  g_message ("%s: starting processing events...", __func__);

  self->process_source_id =
    g_timeout_add (12, process_events, self);

  g_message ("%s: done...", __func__);
}

/**
 * Creates the event queue and starts the event loop.
 *
 * Must be called from a GTK thread.
 */
EventManager *
event_manager_new (void)
{
  EventManager * self = object_new (EventManager);

  self->obj_pool = object_pool_new (
    (ObjectCreatorFunc) event_new,
    (ObjectFreeFunc) event_free, EVENT_MANAGER_MAX_EVENTS);
  self->mqueue = mpmc_queue_new ();
  mpmc_queue_reserve (
    self->mqueue,
    (size_t) EVENT_MANAGER_MAX_EVENTS * sizeof (ZEvent *));

  self->events_arr = g_ptr_array_sized_new (200);

  return self;
}

/**
 * Stops events from getting fired.
 */
void
event_manager_stop_events (EventManager * self)
{
  if (self->process_source_id)
    {
      /* remove the source func */
      g_source_remove_and_zero (self->process_source_id);
    }

  /* process any remaining events - clear the
   * queue. */
  process_events (self);

  /* clear the event queue just in case no events
   * were processed */
  ZEvent * event;
  while (event_queue_dequeue_event (self->mqueue, &event))
    {
      object_pool_return (self->obj_pool, event);
    }
  g_return_if_fail (
    self->obj_pool->num_obj_available
    == self->obj_pool->max_objects);
}

/**
 * Processes the events now.
 *
 * Must only be called from the GTK thread.
 */
void
event_manager_process_now (EventManager * self)
{
  g_return_if_fail (self);

  g_message ("processing events now...");

  /* process events now */
  process_events (self);

  g_message ("done");
}

/**
 * Removes events where the arg matches the
 * given object.
 *
 * FIXME doesn't work - infinite loop.
 */
void
event_manager_remove_events_for_obj (
  EventManager * self,
  void *         obj)
{
  MPMCQueue * q = self->mqueue;
  ZEvent *    event;
  while (event_queue_dequeue_event (q, &event))
    {
      if (event->arg == obj)
        {
          object_pool_return (self->obj_pool, event);
        }
      else
        {
          event_queue_push_back_event (q, event);
        }
    }
}

void
event_manager_free (EventManager * self)
{
  g_message ("%s: Freeing...", __func__);

  event_manager_stop_events (self);

  object_free_w_func_and_null (
    object_pool_free, self->obj_pool);
  object_free_w_func_and_null (mpmc_queue_free, self->mqueue);
  object_free_w_func_and_null (
    g_ptr_array_unref, self->events_arr);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
