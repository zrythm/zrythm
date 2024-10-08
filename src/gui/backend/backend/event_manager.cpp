// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/dsp/audio_region.h"
#include "common/dsp/automation_region.h"
#include "common/dsp/automation_track.h"
#include "common/dsp/automation_tracklist.h"
#include "common/dsp/channel.h"
#include "common/dsp/channel_track.h"
#include "common/dsp/clip.h"
#include "common/dsp/modulator_track.h"
#include "common/dsp/pool.h"
#include "common/dsp/router.h"
#include "common/dsp/stretcher.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/plugins/plugin_gtk.h"
#include "common/plugins/plugin_manager.h"
#include "common/utils/logger.h"
#include "common/utils/mpmc_queue.h"
#include "common/utils/object_pool.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/piano_roll.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/arranger_minimap.h"
#include "gui/backend/gtk_widgets/arranger_object.h"
#include "gui/backend/gtk_widgets/arranger_wrapper.h"
#include "gui/backend/gtk_widgets/audio_arranger.h"
#include "gui/backend/gtk_widgets/audio_editor_space.h"
#include "gui/backend/gtk_widgets/automation_arranger.h"
#include "gui/backend/gtk_widgets/automation_editor_space.h"
#include "gui/backend/gtk_widgets/bot_bar.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/channel.h"
#include "gui/backend/gtk_widgets/channel_sends_expander.h"
#include "gui/backend/gtk_widgets/chord_arranger.h"
#include "gui/backend/gtk_widgets/chord_editor_space.h"
#include "gui/backend/gtk_widgets/chord_key.h"
#include "gui/backend/gtk_widgets/chord_pack_browser.h"
#include "gui/backend/gtk_widgets/chord_pad_panel.h"
#include "gui/backend/gtk_widgets/clip_editor.h"
#include "gui/backend/gtk_widgets/clip_editor_inner.h"
#include "gui/backend/gtk_widgets/color_area.h"
#include "gui/backend/gtk_widgets/editor_ruler.h"
#include "gui/backend/gtk_widgets/editor_toolbar.h"
#include "gui/backend/gtk_widgets/event_viewer.h"
#include "gui/backend/gtk_widgets/foldable_notebook.h"
#include "gui/backend/gtk_widgets/inspector_track.h"
#include "gui/backend/gtk_widgets/left_dock_edge.h"
#include "gui/backend/gtk_widgets/main_notebook.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/midi_arranger.h"
#include "gui/backend/gtk_widgets/midi_editor_space.h"
#include "gui/backend/gtk_widgets/midi_modifier_arranger.h"
#include "gui/backend/gtk_widgets/mixer.h"
#include "gui/backend/gtk_widgets/modulator.h"
#include "gui/backend/gtk_widgets/modulator_view.h"
#include "gui/backend/gtk_widgets/monitor_section.h"
#include "gui/backend/gtk_widgets/panel_file_browser.h"
#include "gui/backend/gtk_widgets/piano_roll_keys.h"
#include "gui/backend/gtk_widgets/plugin_browser.h"
#include "gui/backend/gtk_widgets/plugin_strip_expander.h"
#include "gui/backend/gtk_widgets/right_dock_edge.h"
#include "gui/backend/gtk_widgets/route_target_selector.h"
#include "gui/backend/gtk_widgets/snap_grid.h"
#include "gui/backend/gtk_widgets/timeline_arranger.h"
#include "gui/backend/gtk_widgets/timeline_panel.h"
#include "gui/backend/gtk_widgets/timeline_ruler.h"
#include "gui/backend/gtk_widgets/timeline_toolbar.h"
#include "gui/backend/gtk_widgets/toolbox.h"
#include "gui/backend/gtk_widgets/track.h"
#include "gui/backend/gtk_widgets/track_properties_expander.h"
#include "gui/backend/gtk_widgets/tracklist.h"
#include "gui/backend/gtk_widgets/tracklist_header.h"
#include "gui/backend/gtk_widgets/transport_controls.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

static void
on_project_selection_type_changed (void)
{
  const char * klass = "selected-element";
  const char * selectable_class = "selectable-element";

  gtk_widget_remove_css_class (GTK_WIDGET (MW_TRACKLIST), klass);
  gtk_widget_remove_css_class (
    GTK_WIDGET (MW_TIMELINE_PANEL->timelines_plus_ruler), klass);
  gtk_widget_add_css_class (
    GTK_WIDGET (MW_TIMELINE_PANEL->timelines_plus_ruler), selectable_class);
  gtk_widget_remove_css_class (
    GTK_WIDGET (MW_TIMELINE_PANEL->tracklist_top), klass);
  gtk_widget_add_css_class (
    GTK_WIDGET (MW_TIMELINE_PANEL->tracklist_top), selectable_class);
  gtk_widget_remove_css_class (GTK_WIDGET (MW_CLIP_EDITOR_INNER), klass);
  gtk_widget_add_css_class (GTK_WIDGET (MW_CLIP_EDITOR_INNER), selectable_class);
  gtk_widget_remove_css_class (GTK_WIDGET (MW_MIXER), klass);
  gtk_widget_add_css_class (GTK_WIDGET (MW_MIXER), selectable_class);

  switch (PROJECT->last_selection_)
    {
    case Project::SelectionType::Tracklist:
      gtk_widget_add_css_class (
        GTK_WIDGET (MW_TIMELINE_PANEL->tracklist_top), klass);
      gtk_widget_remove_css_class (
        GTK_WIDGET (MW_TIMELINE_PANEL->tracklist_top), selectable_class);
      gtk_widget_add_css_class (GTK_WIDGET (MW_MIXER), klass);
      gtk_widget_remove_css_class (GTK_WIDGET (MW_MIXER), selectable_class);
      break;
    case Project::SelectionType::Timeline:
      gtk_widget_add_css_class (
        GTK_WIDGET (MW_TIMELINE_PANEL->timelines_plus_ruler), klass);
      gtk_widget_remove_css_class (
        GTK_WIDGET (MW_TIMELINE_PANEL->timelines_plus_ruler), selectable_class);
      break;
    case Project::SelectionType::Insert:
    case Project::SelectionType::MidiFX:
    case Project::SelectionType::Instrument:
    case Project::SelectionType::Modulator:
      break;
    case Project::SelectionType::Editor:
      gtk_widget_add_css_class (GTK_WIDGET (MW_CLIP_EDITOR_INNER), klass);
      gtk_widget_remove_css_class (
        GTK_WIDGET (MW_CLIP_EDITOR_INNER), selectable_class);
      break;
    }
}

static void
on_arranger_selections_in_transit (ArrangerSelections * sel)
{
  z_return_if_fail (sel);

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
          gtk_widget_queue_draw (GTK_WIDGET (MW_DIGITAL_TRANSPORT));
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
  if (ch->widget_)
    {
      route_target_selector_widget_refresh (
        ch->widget_->output, ch->get_track ());
    }
}

static void
on_track_state_changed (Track * track)
{
  if (track->has_channel ())
    {
      auto channel_track = dynamic_cast<ChannelTrack *> (track);
      if (channel_track->channel_->widget_)
        {
          channel_widget_refresh (channel_track->channel_->widget_);
        }
    }

  if (track->is_selected ())
    {
      inspector_track_widget_show_tracks (
        MW_TRACK_INSPECTOR, TRACKLIST_SELECTIONS.get (), true);
    }
}

static void
on_automation_track_added (AutomationTrack * at)
{
  auto track = at->get_track ();
  z_return_if_fail (track);
  if (Z_IS_TRACK_WIDGET (track->widget_))
    {
      TrackWidget * tw = (TrackWidget *) track->widget_;
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

  /* needs to be called later because tracks need time to get allocated */
  EVENTS_PUSH (EventType::ET_REFRESH_ARRANGER, nullptr);
}

static void
on_automation_value_changed (Port * port)
{
  PortIdentifier * id = &port->id_;

  if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags2, id->flags2_,
      PortIdentifier::Flags2::ChannelSendAmount))
    {
      auto tr = port->get_track (true);
      if (tr->is_selected ())
        {
          gtk_widget_queue_draw (
            GTK_WIDGET (MW_TRACK_INSPECTOR->sends->slots[id->port_index_]));
        }
    }
}

static void
on_plugin_added (zrythm::plugins::Plugin * plugin)
{
  /*Track * track =*/
  /*plugin_get_track (plugin);*/
}

static void
on_plugin_crashed (zrythm::plugins::Plugin * plugin)
{
  ui_show_message_printf (
    _ ("Plugin Crashed"), _ ("Plugin '%s' has crashed and has been disabled."),
    plugin->get_name ().c_str ());
}

static void
on_plugin_state_changed (zrythm::plugins::Plugin * pl)
{
  auto track = pl->get_track ();
  if (track && track->has_channel ())
    {
      auto channel_track = dynamic_cast<ChannelTrack *> (track);
      if (channel_track->channel_->widget_)
        {
          auto ch = channel_track->channel_;
          auto plugin_slot = pl->id_.slot_;
          /* redraw slot */
          switch (pl->id_.slot_type_)
            {
            case zrythm::plugins::PluginSlotType::MidiFx:
              plugin_strip_expander_widget_redraw_slot (
                MW_TRACK_INSPECTOR->midi_fx, plugin_slot);
              break;
            case zrythm::plugins::PluginSlotType::Insert:
              plugin_strip_expander_widget_redraw_slot (
                MW_TRACK_INSPECTOR->inserts, plugin_slot);
              plugin_strip_expander_widget_redraw_slot (
                ch->widget_->inserts, plugin_slot);
              break;
            default:
              break;
            }
        }
    }
}

static void
on_modulator_added (zrythm::plugins::Plugin * modulator)
{
  on_plugin_added (modulator);

  auto track = dynamic_cast<ModulatorTrack *> (modulator->get_track ());
  modulator_view_widget_refresh (MW_MODULATOR_VIEW, track);
}

static void
on_plugins_removed (Track * tr)
{
  /* change inspector page */
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);

  /* refresh modulator view */
  modulator_view_widget_refresh (MW_MODULATOR_VIEW, P_MODULATOR_TRACK);
}

static void
refresh_for_selections_type (ArrangerSelections::Type type)
{
  switch (type)
    {
    case ArrangerSelections::Type::Timeline:
      event_viewer_widget_refresh (MW_TIMELINE_EVENT_VIEWER, false);
      break;
    case ArrangerSelections::Type::Midi:
      event_viewer_widget_refresh (MW_MIDI_EVENT_VIEWER, false);
      break;
    case ArrangerSelections::Type::Chord:
      event_viewer_widget_refresh (MW_CHORD_EVENT_VIEWER, false);
      break;
    case ArrangerSelections::Type::Automation:
      event_viewer_widget_refresh (MW_AUTOMATION_EVENT_VIEWER, false);
      break;
    case ArrangerSelections::Type::Audio:
      event_viewer_widget_refresh (MW_AUDIO_EVENT_VIEWER, false);
      break;
    default:
      z_return_if_reached ();
    }
  bot_dock_edge_widget_update_event_viewer_stack_page (MW_BOT_DOCK_EDGE);
}

static void
on_arranger_selections_changed (ArrangerSelections * sel)
{
  refresh_for_selections_type (sel->type_);
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);

  timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
}

static void
arranger_selections_change_redraw_everything (ArrangerSelections * sel)
{
  switch (sel->type_)
    {
    case ArrangerSelections::Type::Timeline:
      event_viewer_widget_refresh (MW_TIMELINE_EVENT_VIEWER, false);
      break;
    case ArrangerSelections::Type::Midi:
      event_viewer_widget_refresh (MW_MIDI_EVENT_VIEWER, false);
      break;
    case ArrangerSelections::Type::Chord:
      event_viewer_widget_refresh (MW_CHORD_EVENT_VIEWER, false);
      break;
    case ArrangerSelections::Type::Automation:
      event_viewer_widget_refresh (MW_AUTOMATION_EVENT_VIEWER, false);
      break;
    case ArrangerSelections::Type::Audio:
      event_viewer_widget_refresh (MW_AUDIO_EVENT_VIEWER, false);
      break;
    default:
      z_return_if_reached ();
    }
  bot_dock_edge_widget_update_event_viewer_stack_page (MW_BOT_DOCK_EDGE);
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
  MW_TIMELINE->hovered_object.reset ();
  MW_MIDI_ARRANGER->hovered_object.reset ();
  MW_MIDI_MODIFIER_ARRANGER->hovered_object.reset ();
  MW_AUTOMATION_ARRANGER->hovered_object.reset ();
  MW_AUDIO_ARRANGER->hovered_object.reset ();
  MW_CHORD_ARRANGER->hovered_object.reset ();
  timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
}

static void
on_mixer_selections_changed (void)
{
  for (auto track : TRACKLIST->tracks_ | type_is<ChannelTrack> ())
    {
      auto ch = track->channel_;
      if (ch->widget_)
        {
          plugin_strip_expander_widget_refresh (ch->widget_->inserts);
        }
    }
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
}

static void
on_track_color_changed (Track * track)
{
  if (track->has_channel ())
    {
      auto channel_track = dynamic_cast<ChannelTrack *> (track);
      if (channel_track->channel_->widget_)
        {
          channel_widget_refresh (channel_track->channel_->widget_);
        }
    }
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
}

static void
on_track_name_changed (Track * track)
{
  /* refresh all because tracks routed to/from are also affected */
  mixer_widget_soft_refresh (MW_MIXER);
  left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
}

static void
on_arranger_object_changed (ArrangerObject * obj)
{
  z_return_if_fail (obj);

  ArrangerSelections * sel = obj->get_selections_for_type (obj->type_);
  event_viewer_widget_refresh_for_selections (sel);

  switch (obj->type_)
    {
    case ArrangerObject::Type::Region:
      /* redraw editor ruler if region positions were changed */
      timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
      break;
    default:
      break;
    }
}

static void
on_track_changed (Track * track)
{
  z_return_if_fail (IS_TRACK_AND_NONNULL (track));
  if (GTK_IS_WIDGET (track->widget_))
    {
      if (
        gtk_widget_get_visible (GTK_WIDGET (track->widget_)) != track->visible_)
        {
          gtk_widget_set_visible (GTK_WIDGET (track->widget_), track->visible_);
        }
    }
}

static void
on_plugin_window_visibility_changed (zrythm::plugins::Plugin * pl)
{
  if (!pl || pl->deleting_)
    {
      return;
    }

  Track * track = pl->get_track ();

  if (track && track->has_channel ())
    {
      auto channel_track = dynamic_cast<ChannelTrack *> (track);
      auto ch = channel_track->channel_;
      auto ch_widget = ch->widget_;
      if (ch_widget && Z_IS_CHANNEL_WIDGET (ch_widget))
        {
          auto slot = pl->id_.slot_;
          /* redraw slot */
          switch (pl->id_.slot_type_)
            {
            case zrythm::plugins::PluginSlotType::MidiFx:
              plugin_strip_expander_widget_redraw_slot (
                MW_TRACK_INSPECTOR->midi_fx, slot);
              break;
            case zrythm::plugins::PluginSlotType::Insert:
              plugin_strip_expander_widget_redraw_slot (
                MW_TRACK_INSPECTOR->inserts, slot);
              plugin_strip_expander_widget_redraw_slot (
                ch_widget->inserts, slot);
              break;
            case zrythm::plugins::PluginSlotType::Instrument:
              track_properties_expander_widget_refresh (
                MW_TRACK_INSPECTOR->track_info, track);
              break;
            default:
              break;
            }

          channel_widget_refresh_instrument_ui_toggle (ch_widget);
        }
    }

  if (pl->modulator_widget_)
    {
      modulator_widget_refresh (pl->modulator_widget_);
    }
}

static void
on_plugin_visibility_changed (zrythm::plugins::Plugin * pl)
{
  z_debug ("start - visible: {}", pl->visible_);
  if (pl->visible_)
    {
      pl->open_ui ();
    }
  else if (!pl->visible_)
    {
      pl->close_ui ();
    }

  on_plugin_window_visibility_changed (pl);
  z_debug ("done");
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

void
EventManager::clean_duplicate_events_and_copy ()
{
  auto    &q = mqueue_;
  ZEvent * event;

  events_arr_.clear ();

  /* only add events once to new array while popping */
  while (q.pop_front (event))
    {
      bool already_exists = std::any_of (
        events_arr_.begin (), events_arr_.end (), [&event] (ZEvent * e) {
          return event->type_ == e->type_ && event->arg_ == e->arg_;
        });

      if (already_exists)
        {
          obj_pool_.release (event);
        }
      else
        {
          events_arr_.push_back (event);
        }
    }
}

bool
EventManager::soft_recalc_graph_when_paused ()
{
  if (TRANSPORT->play_state_ == Transport::PlayState::Paused)
    {
      ROUTER->recalc_graph (true);
      pending_soft_recalc_ = false;
      return SourceFuncRemove;
    }
  return SourceFuncContinue;
}

void
EventManager::process_event (ZEvent &ev)
{
  /* don't print super-noisy events*/
  if (ev.type_ != EventType::ET_PLAYHEAD_POS_CHANGED)
    {
      z_trace ("processing:\n{}", ev);
    }

  switch (ev.type_)
    {
    case EventType::ET_PLUGIN_LATENCY_CHANGED:
      if (!pending_soft_recalc_)
        {
          pending_soft_recalc_ = true;
          Glib::signal_idle ().connect (sigc::track_obj (
            sigc::mem_fun (*this, &EventManager::soft_recalc_graph_when_paused),
            *this));
        }
      break;
    case EventType::ET_TRACKS_REMOVED:
      if (MW_MIXER)
        mixer_widget_hard_refresh (MW_MIXER);
      if (MW_TRACKLIST)
        tracklist_widget_hard_refresh (MW_TRACKLIST);
      tracklist_header_widget_refresh_track_count (MW_TRACKLIST_HEADER);
      left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
      break;
    case EventType::ET_CHANNEL_REMOVED:
      mixer_widget_hard_refresh (MW_MIXER);
      break;
    case EventType::ET_ARRANGER_OBJECT_CREATED:
      break;
    case EventType::ET_ARRANGER_OBJECT_CHANGED:
      on_arranger_object_changed ((ArrangerObject *) ev.arg_);
      break;
    case EventType::ET_ARRANGER_OBJECT_REMOVED:
      break;
    case EventType::ET_ARRANGER_SELECTIONS_CHANGED:
      on_arranger_selections_changed (
        static_cast<ArrangerSelections *> (ev.arg_));
      break;
    case EventType::ET_ARRANGER_SELECTIONS_CREATED:
      on_arranger_selections_created (
        static_cast<ArrangerSelections *> (ev.arg_));
      break;
    case EventType::ET_ARRANGER_SELECTIONS_REMOVED:
      on_arranger_selections_removed (
        static_cast<ArrangerSelections *> (ev.arg_));
      break;
    case EventType::ET_ARRANGER_SELECTIONS_MOVED:
      on_arranger_selections_moved (static_cast<ArrangerSelections *> (ev.arg_));
      break;
    case EventType::ET_ARRANGER_SELECTIONS_QUANTIZED:
      break;
    case EventType::ET_ARRANGER_SELECTIONS_ACTION_FINISHED:
      break;
    case EventType::ET_TRACKLIST_SELECTIONS_CHANGED:
      /* only refresh the inspector if the tracklist selection changed by
       * clicking on a track or on a region */
      if (
        PROJECT->last_selection_ == Project::SelectionType::Tracklist
        || PROJECT->last_selection_ == Project::SelectionType::Insert
        || PROJECT->last_selection_ == Project::SelectionType::MidiFX
        || PROJECT->last_selection_ == Project::SelectionType::Timeline)
        {
          left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
        }
      mixer_widget_soft_refresh (MW_MIXER);
      tracklist_widget_soft_refresh (MW_TRACKLIST);
      break;
    case EventType::ET_RULER_SIZE_CHANGED:
      break;
    case EventType::ET_CLIP_MARKER_POS_CHANGED:
      break;
    case EventType::ET_TIMELINE_LOOP_MARKER_POS_CHANGED:
    case EventType::ET_TIMELINE_PUNCH_MARKER_POS_CHANGED:
      break;
    case EventType::ET_TIMELINE_SONG_MARKER_POS_CHANGED:
      break;
    case EventType::ET_PLUGIN_VISIBILITY_CHANGED:
      on_plugin_visibility_changed ((zrythm::plugins::Plugin *) ev.arg_);
      break;
    case EventType::ET_PLUGIN_WINDOW_VISIBILITY_CHANGED:
      on_plugin_window_visibility_changed ((zrythm::plugins::Plugin *) ev.arg_);
      break;
    case EventType::ET_PLUGIN_STATE_CHANGED:
      {
        auto pl = (zrythm::plugins::Plugin *) ev.arg_;
        if (pl)
          {
            on_plugin_state_changed (pl);
            pl->state_changed_event_sent_.store (false);
          }
      }
      break;
    case EventType::ET_TRANSPORT_TOTAL_BARS_CHANGED:
      ruler_widget_refresh ((RulerWidget *) MW_RULER);
      ruler_widget_refresh ((RulerWidget *) EDITOR_RULER);
      arranger_minimap_widget_refresh (MW_TIMELINE_MINIMAP);
      break;
    case EventType::ET_AUTOMATION_VALUE_CHANGED:
      on_automation_value_changed ((Port *) ev.arg_);
      break;
    case EventType::ET_RANGE_SELECTION_CHANGED:
      timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
      break;
    case EventType::ET_TOOL_CHANGED:
      toolbox_widget_refresh (MW_TOOLBOX);
      arranger_widget_refresh_cursor (Z_ARRANGER_WIDGET (MW_TIMELINE));
      if (
        MW_MIDI_ARRANGER
        && gtk_widget_get_realized (GTK_WIDGET (MW_MIDI_ARRANGER)))
        arranger_widget_refresh_cursor (Z_ARRANGER_WIDGET (MW_MIDI_ARRANGER));
      if (
        MW_MIDI_MODIFIER_ARRANGER
        && gtk_widget_get_realized (GTK_WIDGET (MW_MIDI_MODIFIER_ARRANGER)))
        arranger_widget_refresh_cursor (
          Z_ARRANGER_WIDGET (MW_MIDI_MODIFIER_ARRANGER));
      break;
    case EventType::ET_TIME_SIGNATURE_CHANGED:
      ruler_widget_refresh (Z_RULER_WIDGET (MW_RULER));
      ruler_widget_refresh (Z_RULER_WIDGET (EDITOR_RULER));
      gtk_widget_queue_draw (GTK_WIDGET (MW_DIGITAL_TIME_SIG));
      break;
    case EventType::ET_PLAYHEAD_POS_CHANGED:
      on_playhead_changed (false);
      break;
    case EventType::ET_PLAYHEAD_POS_CHANGED_MANUALLY:
      on_playhead_changed (true);
      break;
    case EventType::ET_CLIP_EDITOR_REGION_CHANGED:
      clip_editor_widget_on_region_changed (MW_CLIP_EDITOR);
      PIANO_ROLL->current_notes_.clear ();
      piano_roll_keys_widget_redraw_full (MW_PIANO_ROLL_KEYS);
      bot_dock_edge_widget_update_event_viewer_stack_page (MW_BOT_DOCK_EDGE);
      break;
    case EventType::ET_TRACK_AUTOMATION_VISIBILITY_CHANGED:
      {
        auto * track = static_cast<AutomatableTrack *> (ev.arg_);
        if (!IS_TRACK_AND_NONNULL (track))
          {
            z_error ("expected track argument");
            break;
          }

        if (GTK_IS_WIDGET (track->widget_))
          {
            track_widget_update_icons (track->widget_);
            track_widget_update_size (track->widget_);
          }
      }
      break;
    case EventType::ET_TRACK_LANES_VISIBILITY_CHANGED:
      tracklist_widget_update_track_visibility (MW_TRACKLIST);
      on_track_added ();
      break;
    case EventType::ET_TRACK_ADDED:
      on_track_added ();
      tracklist_header_widget_refresh_track_count (MW_TRACKLIST_HEADER);
      break;
    case EventType::ET_TRACK_CHANGED:
      on_track_changed ((Track *) ev.arg_);
      break;
    case EventType::ET_TRACKS_ADDED:
      if (MW_MIXER)
        mixer_widget_hard_refresh (MW_MIXER);
      if (MW_TRACKLIST)
        tracklist_widget_hard_refresh (MW_TRACKLIST);
      tracklist_header_widget_refresh_track_count (MW_TRACKLIST_HEADER);
      break;
    case EventType::ET_TRACK_COLOR_CHANGED:
      on_track_color_changed ((Track *) ev.arg_);
      break;
    case EventType::ET_TRACK_NAME_CHANGED:
      on_track_name_changed ((Track *) ev.arg_);
      break;
    case EventType::ET_REFRESH_ARRANGER:
      break;
    case EventType::ET_RULER_VIEWPORT_CHANGED:
      arranger_minimap_widget_refresh (MW_TIMELINE_MINIMAP);
      ruler_widget_refresh (Z_RULER_WIDGET (ev.arg_));
      break;
    case EventType::ET_TRACK_STATE_CHANGED:
      for (auto &track : TRACKLIST->tracks_)
        {
          on_track_state_changed (track.get ());
        }
      monitor_section_widget_refresh (MW_MONITOR_SECTION);
      break;
    case EventType::ET_TRACK_VISIBILITY_CHANGED:
      tracklist_widget_update_track_visibility (MW_TRACKLIST);
      tracklist_header_widget_refresh_track_count (MW_TRACKLIST_HEADER);
      mixer_widget_hard_refresh (MW_MIXER);
      break;
    case EventType::ET_UNDO_REDO_ACTION_DONE:
      main_window_widget_refresh_undo_redo_buttons (MAIN_WINDOW);
      main_window_widget_set_project_title (MAIN_WINDOW, PROJECT.get ());
      break;
    case EventType::ET_PIANO_ROLL_HIGHLIGHTING_CHANGED:
      if (MW_MIDI_EDITOR_SPACE)
        piano_roll_keys_widget_refresh (MW_PIANO_ROLL_KEYS);
      break;
    case EventType::ET_PIANO_ROLL_KEY_ON_OFF:
      piano_roll_keys_widget_redraw_full (MW_PIANO_ROLL_KEYS);
      break;
    case EventType::ET_RULER_STATE_CHANGED:
      ruler_widget_refresh ((RulerWidget *) MW_RULER);
      break;
    case EventType::ET_AUTOMATION_TRACK_ADDED:
    case EventType::ET_AUTOMATION_TRACK_REMOVED:
    case EventType::ET_AUTOMATION_TRACK_CHANGED:
      on_automation_track_added ((AutomationTrack *) ev.arg_);
      break;
    case EventType::ET_PLUGINS_ADDED:
      on_plugins_removed ((Track *) ev.arg_);
      break;
    case EventType::ET_PLUGIN_ADDED:
      on_plugin_added ((zrythm::plugins::Plugin *) ev.arg_);
      break;
    case EventType::ET_PLUGIN_CRASHED:
      on_plugin_crashed ((zrythm::plugins::Plugin *) ev.arg_);
      break;
    case EventType::ET_PLUGINS_REMOVED:
      on_plugins_removed ((Track *) ev.arg_);
      break;
    case EventType::ET_MIXER_SELECTIONS_CHANGED:
      on_mixer_selections_changed ();
      break;
    case EventType::ET_CHANNEL_OUTPUT_CHANGED:
      on_channel_output_changed ((Channel *) ev.arg_);
      break;
    case EventType::ET_TRACKS_MOVED:
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
      EVENTS_PUSH (EventType::ET_REFRESH_ARRANGER, nullptr);
      break;
    case EventType::ET_CHANNEL_SLOTS_CHANGED:
      {
        Channel *       ch = (Channel *) ev.arg_;
        ChannelWidget * cw = ch ? ch->widget_ : NULL;
        if (cw)
          {
            channel_widget_update_midi_fx_and_inserts (cw);
          }
      }
      break;
    case EventType::ET_DRUM_MODE_CHANGED:
      midi_editor_space_widget_refresh (MW_MIDI_EDITOR_SPACE);
      break;
    case EventType::ET_MODULATOR_ADDED:
      on_modulator_added ((zrythm::plugins::Plugin *) ev.arg_);
      break;
    case EventType::ET_PINNED_TRACKLIST_SIZE_CHANGED:
      /*gtk_widget_set_size_request (*/
      /*GTK_WIDGET (*/
      /*MW_CENTER_DOCK->*/
      /*pinned_timeline_scroll),*/
      /*-1,*/
      /*gtk_widget_get_height (*/
      /*GTK_WIDGET (MW_PINNED_TRACKLIST)));*/
      break;
    case EventType::ET_TRACK_LANE_ADDED:
    case EventType::ET_TRACK_LANE_REMOVED:
      tracklist_widget_update_track_visibility (MW_TRACKLIST);
      /*arranger_widget_update_visibility (*/
      /*(ArrangerWidget *) MW_TIMELINE);*/
      break;
    case EventType::ET_LOOP_TOGGLED:
      transport_controls_widget_refresh (MW_TRANSPORT_CONTROLS);
      break;
    case EventType::ET_ARRANGER_SELECTIONS_IN_TRANSIT:
      on_arranger_selections_in_transit ((ArrangerSelections *) ev.arg_);
      break;
    case EventType::ET_CHORD_KEY_CHANGED:
      for (size_t j = 0; j < CHORD_EDITOR->chords_.size (); ++j)
        {
          if (CHORD_EDITOR->chords_[j] == *((ChordDescriptor *) ev.arg_))
            {
              chord_key_widget_refresh (MW_CHORD_EDITOR_SPACE->chord_keys[j]);
            }
        }
      chord_pad_panel_widget_refresh (MW_CHORD_PAD_PANEL);
      break;
    case EventType::ET_CHORD_PRESET_ADDED:
    case EventType::ET_CHORD_PRESET_EDITED:
    case EventType::ET_CHORD_PRESET_REMOVED:
      chord_pack_browser_widget_refresh_presets (MW_CHORD_PACK_BROWSER);
      chord_pad_panel_widget_refresh_load_preset_menu (MW_CHORD_PAD_PANEL);
      break;
    case EventType::ET_CHORD_PRESET_PACK_ADDED:
    case EventType::ET_CHORD_PRESET_PACK_EDITED:
    case EventType::ET_CHORD_PRESET_PACK_REMOVED:
      chord_pack_browser_widget_refresh_packs (MW_CHORD_PACK_BROWSER);
      chord_pack_browser_widget_refresh_presets (MW_CHORD_PACK_BROWSER);
      chord_pad_panel_widget_refresh_load_preset_menu (MW_CHORD_PAD_PANEL);
      break;
    case EventType::ET_CHORDS_UPDATED:
      chord_pad_panel_widget_refresh (MW_CHORD_PAD_PANEL);
      chord_editor_space_widget_refresh_chords (MW_CHORD_EDITOR_SPACE);
      break;
    case EventType::ET_JACK_TRANSPORT_TYPE_CHANGED:
      break;
    case EventType::ET_SELECTING_IN_ARRANGER:
      {
        ArrangerWidget * arranger = Z_ARRANGER_WIDGET (ev.arg_);
        event_viewer_widget_refresh_for_arranger (arranger, true);
        timeline_toolbar_widget_refresh (MW_TIMELINE_TOOLBAR);
      }
      break;
    case EventType::ET_TRACKS_RESIZED:
      z_warn_if_fail (ev.arg_);
      break;
    case EventType::ET_CLIP_EDITOR_FIRST_TIME_REGION_SELECTED:
      gtk_widget_set_visible (
        GTK_WIDGET (MW_EDITOR_EVENT_VIEWER_STACK),
        g_settings_get_boolean (S_UI, "editor-event-viewer-visible"));
      bot_dock_edge_widget_update_event_viewer_stack_page (MW_BOT_DOCK_EDGE);
      break;
    case EventType::ET_PIANO_ROLL_MIDI_MODIFIER_CHANGED:
      break;
    case EventType::ET_BPM_CHANGED:
      ruler_widget_refresh (MW_RULER);
      ruler_widget_refresh (EDITOR_RULER);
      gtk_widget_queue_draw (GTK_WIDGET (MW_DIGITAL_BPM));

      /* these are only used in the UI so no need to update them during DSP */
      QUANTIZE_OPTIONS_TIMELINE->update_quantize_points ();
      QUANTIZE_OPTIONS_EDITOR->update_quantize_points ();
      break;
    case EventType::ET_CHANNEL_FADER_VAL_CHANGED:
      channel_widget_redraw_fader (((Channel *) ev.arg_)->widget_);
#if 0
      inspector_track_widget_show_tracks (
        MW_TRACK_INSPECTOR, TRACKLIST_SELECTIONS);
#endif
      break;
    case EventType::ET_PIANO_ROLL_KEY_ZOOM_CHANGED:
      midi_editor_space_widget_refresh (MW_MIDI_EDITOR_SPACE);
      break;
    case EventType::ET_PIANO_ROLL_KEY_HEIGHT_CHANGED:
      {
        gtk_widget_set_size_request (
          GTK_WIDGET (MW_PIANO_ROLL_KEYS), 10,
          (int) MW_PIANO_ROLL_KEYS->total_key_px);
      }
      break;
    case EventType::ET_MAIN_WINDOW_LOADED:
      /* show all visible plugins */
      for (auto track : TRACKLIST->tracks_ | type_is<ChannelTrack> ())
        {
          auto                 &ch = track->channel_;
          std::vector<zrythm::plugins::Plugin *> plugins;
          ch->get_plugins (plugins);
          for (auto plugin : plugins)
            {
              if (plugin->visible_)
                plugin->open_ui ();
            }
        }
      /* refresh modulator view */
      if (MW_MODULATOR_VIEW)
        {
          modulator_view_widget_refresh (MW_MODULATOR_VIEW, P_MODULATOR_TRACK);
        }

      /* show clip editor if clip selected */
      if (MW_CLIP_EDITOR && CLIP_EDITOR->has_region_)
        {
          clip_editor_widget_on_region_changed (MW_CLIP_EDITOR);
        }

      /* refresh inspector */
      left_dock_edge_widget_refresh (MW_LEFT_DOCK_EDGE);
      on_project_selection_type_changed ();
      main_notebook_widget_refresh (MW_MAIN_NOTEBOOK);

#ifdef CHECK_UPDATES
      zrythm_app->check_for_updates ();
#endif /* CHECK_UPDATES */

      /* show any pending messages */
      {
        std::lock_guard<std::mutex> lock (zrythm_app->queue_mutex_);
        while (!zrythm_app->project_load_message_queue_.empty ())
          {
            auto msg = zrythm_app->project_load_message_queue_.front ();
            ui_show_message_literal (_ ("Error"), msg.msg_);
            zrythm_app->project_load_message_queue_.pop ();
          }
      }

      if (PLUGIN_MANAGER->get_num_new_plugins () > 0)
        {
          ui_show_notification_idle_printf (
            _ ("%d new plugins detected"),
            PLUGIN_MANAGER->get_num_new_plugins ());
        }
      break;
    case EventType::ET_SPLASH_CLOSED:
      break;
    case EventType::ET_PROJECT_SAVED:
      main_window_widget_set_project_title (MAIN_WINDOW, (Project *) ev.arg_);
      break;
    case EventType::ET_PROJECT_LOADED:
      main_window_widget_set_project_title (MAIN_WINDOW, (Project *) ev.arg_);
      main_window_widget_refresh_undo_redo_buttons (MAIN_WINDOW);
      ruler_widget_set_zoom_level (
        MW_RULER, ruler_widget_get_zoom_level (MW_RULER));
      ruler_widget_set_zoom_level (
        EDITOR_RULER, ruler_widget_get_zoom_level (EDITOR_RULER));
      gtk_paned_set_position (
        MW_TIMELINE_PANEL->tracklist_timeline, PRJ_TIMELINE->tracks_width_);
      break;
    case EventType::ET_AUTOMATION_TRACKLIST_AT_REMOVED:
      /* TODO */
      break;
    case EventType::ET_CHANNEL_SEND_CHANGED:
      {
        auto send = (ChannelSend *) ev.arg_;
        auto widget = send->find_widget ();
        if (widget)
          {
            gtk_widget_queue_draw (GTK_WIDGET (widget));
          }
        auto tr = send->get_track ();
        auto ch_w = tr->channel_->widget_;
        if (ch_w)
          {
            gtk_widget_queue_draw (GTK_WIDGET (ch_w->sends->slots[send->slot_]));
          }
      }
      break;
    case EventType::ET_RULER_DISPLAY_TYPE_CHANGED:
      break;
    case EventType::ET_ARRANGER_HIGHLIGHT_CHANGED:
      break;
    case EventType::ET_ENGINE_ACTIVATE_CHANGED:
    case EventType::ET_ENGINE_BUFFER_SIZE_CHANGED:
    case EventType::ET_ENGINE_SAMPLE_RATE_CHANGED:
      if (!gtk_widget_in_destruction (GTK_WIDGET (MAIN_WINDOW)))
        {
          bot_bar_widget_refresh (MW_BOT_BAR);
          inspector_track_widget_show_tracks (
            MW_TRACK_INSPECTOR, TRACKLIST_SELECTIONS.get (), false);
        }
      break;
    case EventType::ET_MIDI_BINDINGS_CHANGED:
      main_notebook_widget_refresh (MW_MAIN_NOTEBOOK);
      break;
    case EventType::ET_PORT_CONNECTION_CHANGED:
      main_notebook_widget_refresh (MW_MAIN_NOTEBOOK);
      break;
    case EventType::ET_EDITOR_FUNCTION_APPLIED:
      editor_toolbar_widget_refresh (MW_EDITOR_TOOLBAR);
      break;
    case EventType::ET_ARRANGER_SELECTIONS_CHANGED_REDRAW_EVERYTHING:
      arranger_selections_change_redraw_everything (
        static_cast<ArrangerSelections *> (ev.arg_));
      break;
    case EventType::ET_AUTOMATION_VALUE_VISIBILITY_CHANGED:
      break;
    case EventType::ET_PROJECT_SELECTION_TYPE_CHANGED:
      on_project_selection_type_changed ();
      break;
    case EventType::ET_AUDIO_SELECTIONS_RANGE_CHANGED:
      break;
    case EventType::ET_PLUGIN_COLLECTIONS_CHANGED:
      plugin_browser_widget_refresh_collections (MW_PLUGIN_BROWSER);
      ui_show_notification (_ ("Plugin collections updated."));
      break;
    case EventType::ET_SNAP_GRID_OPTIONS_CHANGED:
      {
        auto sg = (SnapGrid *) ev.arg_;
        if (sg == SNAP_GRID_TIMELINE.get ())
          {
            snap_grid_widget_refresh (MW_TIMELINE_TOOLBAR->snap_grid);
          }
        else if (sg == SNAP_GRID_EDITOR.get ())
          {
            snap_grid_widget_refresh (MW_EDITOR_TOOLBAR->snap_grid);
          }
      }
      break;
    case EventType::ET_TRANSPORT_RECORDING_ON_OFF_CHANGED:
      gtk_toggle_button_set_active (
        MW_TRANSPORT_CONTROLS->trans_record_btn, TRANSPORT->recording_);
      break;
    case EventType::ET_TRACK_FREEZE_CHANGED:
      arranger_selections_change_redraw_everything (TL_SELECTIONS.get ());
      break;
    case EventType::ET_LOG_WARNING_STATE_CHANGED:
      break;
    case EventType::ET_PLAYHEAD_SCROLL_MODE_CHANGED:
      break;
    case EventType::ET_TRACK_FADER_BUTTON_CHANGED:
      on_track_state_changed ((Track *) ev.arg_);
      break;
    case EventType::ET_PLUGIN_PRESET_SAVED:
    case EventType::ET_PLUGIN_PRESET_LOADED:
      {
        zrythm::plugins::Plugin * pl = (zrythm::plugins::Plugin *) ev.arg_;
        if (pl->window_)
          {
            zrythm::plugins::plugin_gtk_set_window_title (pl, pl->window_);
          }

        switch (ev.type_)
          {
          case EventType::ET_PLUGIN_PRESET_SAVED:
            ui_show_notification (_ ("Plugin preset saved."));
            break;
          case EventType::ET_PLUGIN_PRESET_LOADED:
            ui_show_notification (_ ("Plugin preset loaded."));
            break;
          default:
            break;
          }
      }
      break;
    case EventType::ET_TRACK_FOLD_CHANGED:
      on_track_added ();
      break;
    case EventType::ET_MIXER_CHANNEL_INSERTS_EXPANDED_CHANGED:
    case EventType::ET_MIXER_CHANNEL_MIDI_FX_EXPANDED_CHANGED:
    case EventType::ET_MIXER_CHANNEL_SENDS_EXPANDED_CHANGED:
      mixer_widget_soft_refresh (MW_MIXER);
      break;
    case EventType::ET_REGION_ACTIVATED:
      bot_dock_edge_widget_show_clip_editor (MW_BOT_DOCK_EDGE, true);
      break;
    case EventType::ET_VELOCITIES_RAMPED:
      break;
    case EventType::ET_AUDIO_REGION_FADE_IN_CHANGED:
      break;
    case EventType::ET_AUDIO_REGION_FADE_OUT_CHANGED:
      break;
    case EventType::ET_AUDIO_REGION_GAIN_CHANGED:
      break;
    case EventType::ET_FILE_BROWSER_BOOKMARK_ADDED:
    case EventType::ET_FILE_BROWSER_BOOKMARK_DELETED:
      panel_file_browser_refresh_bookmarks (MW_PANEL_FILE_BROWSER);
      break;
    case EventType::ET_ARRANGER_SCROLLED:
      {
        ArrangerWidget *     arranger = Z_ARRANGER_WIDGET (ev.arg_);
        ArrangerSelections * sel = arranger_widget_get_selections (arranger);
        arranger_selections_change_redraw_everything (sel);
      }
      break;
    case EventType::ET_FILE_BROWSER_INSTRUMENT_CHANGED:
      /* TODO */
#if 0
      chord_pack_browser_widget_refresh_auditioner_controls (
        MW_CHORD_PACK_BROWSER);
#endif
      break;
    case EventType::ET_TRANSPORT_ROLL_REQUIRED:
      TRANSPORT->request_roll (true);
      break;
    case EventType::ET_TRANSPORT_PAUSE_REQUIRED:
      TRANSPORT->request_pause (true);
      break;
    case EventType::ET_TRANSPORT_MOVE_BACKWARD_REQUIRED:
      TRANSPORT->move_backward (true);
      break;
    case EventType::ET_TRANSPORT_MOVE_FORWARD_REQUIRED:
      TRANSPORT->move_forward (true);
      break;
    case EventType::ET_TRANSPORT_TOGGLE_LOOP_REQUIRED:
      TRANSPORT->set_loop (!TRANSPORT->loop_, true);
      break;
    case EventType::ET_TRANSPORT_TOGGLE_RECORDING_REQUIRED:
      TRANSPORT->set_recording (!TRANSPORT->recording_, true, true);
      break;
    default:
      z_warning ("event {} not implemented yet", ENUM_NAME (ev.type_));
      break;
    }
}

bool
EventManager::process_events ()
{
  clean_duplicate_events_and_copy ();

  int count = -1;
  for (auto ev : events_arr_)
    {
      ++count;
      if (count > 30)
        {
          z_warning ("more than 30 UI events processed ({})!", count);
        }

      if (ZRYTHM_HAVE_UI)
        {
          process_event (*ev);
        }
      else
        {
          z_info ("({}) No UI, skipping", count);
        }

      obj_pool_.release (ev);
    }

  if (events_arr_.size () > 6)
    z_debug ("More than 6 events processed. Optimization needed.");

  /*g_usleep (8000);*/
  /*project_validate (PROJECT);*/

  return SourceFuncContinue;
}

void
EventManager::start_events ()
{
  z_debug ("starting event manager events...");

  if (process_source_id_.connected ())
    {
      z_debug ("already processing events");
      return;
    }

  z_debug ("starting processing events...");

  process_source_id_ = Glib::signal_timeout ().connect (
    sigc::mem_fun (*this, &EventManager::process_events), 12);

  z_debug ("done");
}

EventManager::EventManager ()
{
  obj_pool_.reserve (EVENT_MANAGER_MAX_EVENTS);
  mqueue_.reserve (EVENT_MANAGER_MAX_EVENTS);

  events_arr_.reserve (200);
}

void
EventManager::stop_events ()
{
  if (process_source_id_.connected ())
    {
      /* remove the source func */
      process_source_id_.disconnect ();
    }

  /* process any remaining events - clear the queue. */
  process_events ();

  /* clear the event queue just in case no events were processed */
  ZEvent * event;
  while (mqueue_.pop_front (event))
    {
      obj_pool_.release (event);
    }
}

void
EventManager::process_now ()
{
  z_debug ("processing events now...");

  /* process events now */
  process_events ();

  z_debug ("done");
}
