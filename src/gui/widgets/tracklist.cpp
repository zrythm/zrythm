// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/tracklist_selections.h"
#include "dsp/audio_bus_track.h"
#include "dsp/channel.h"
#include "dsp/chord_track.h"
#include "dsp/instrument_track.h"
#include "dsp/scale.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/add_track_menu_button.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/string.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TracklistWidget, tracklist_widget, GTK_TYPE_BOX)

#define DEFAULT_SCROLL_SPEED 4.0
#define SCROLL_SPEED_ACCELERATION 1.3
#define SCROLL_THRESHOLD_PX 24

static void
remove_scroll_sources (TracklistWidget * self)
{
  if (self->unpinned_scroll_scroll_up_id != 0)
    {
      g_source_remove (self->unpinned_scroll_scroll_up_id);
      self->unpinned_scroll_scroll_up_id = 0;
    }
  if (self->unpinned_scroll_scroll_down_id != 0)
    {
      g_source_remove (self->unpinned_scroll_scroll_down_id);
      self->unpinned_scroll_scroll_down_id = 0;
    }
  self->scroll_speed = DEFAULT_SCROLL_SPEED;
}

GMenu *
tracklist_widget_generate_add_track_menu (void)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = z_gtk_create_menu_item (
    _ ("Add _MIDI Track"), NULL, "app.create-midi-track");
  g_menu_append_item (menu, menuitem);

  menuitem = z_gtk_create_menu_item (
    _ ("Add Audio Track"), NULL, "app.create-audio-track");
  g_menu_append_item (menu, menuitem);

  menuitem =
    z_gtk_create_menu_item (_ ("Import File..."), NULL, "app.import-file");
  g_menu_append_item (menu, menuitem);

  GMenu * bus_submenu = g_menu_new ();
  menuitem = z_gtk_create_menu_item (
    _ ("Add Audio FX Track"), NULL, "app.create-audio-bus-track");
  g_menu_append_item (bus_submenu, menuitem);
  menuitem = z_gtk_create_menu_item (
    _ ("Add MIDI FX Track"), NULL, "app.create-midi-bus-track");
  g_menu_append_item (bus_submenu, menuitem);
  g_menu_append_section (menu, NULL, G_MENU_MODEL (bus_submenu));

  GMenu * group_submenu = g_menu_new ();
  menuitem = z_gtk_create_menu_item (
    _ ("Add Audio Group Track"), NULL, "app.create-audio-group-track");
  g_menu_append_item (group_submenu, menuitem);
  menuitem = z_gtk_create_menu_item (
    _ ("Add MIDI Group Track"), NULL, "app.create-midi-group-track");
  g_menu_append_item (group_submenu, menuitem);
  g_menu_append_section (menu, NULL, G_MENU_MODEL (group_submenu));

  menuitem = z_gtk_create_menu_item (
    _ ("Add Folder Track"), NULL, "app.create-folder-track");
  g_menu_append_item (menu, menuitem);

  return menu;
}

static void
on_dnd_leave (GtkDropTarget * drop_target, TracklistWidget * self)
{
  remove_scroll_sources (self);

  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (track_get_should_be_visible (track) && track->widget)
        {
          track_widget_do_highlight (track->widget, 0, 0, 0);
        }
    }
}

static gboolean
scroll_down_source (gpointer data)
{
  TracklistWidget * self = Z_TRACKLIST_WIDGET (data);
  GtkAdjustment *   vadj =
    gtk_scrolled_window_get_vadjustment (self->unpinned_scroll);
  gtk_adjustment_set_value (
    vadj,
    MIN (
      gtk_adjustment_get_value (vadj) + self->scroll_speed,
      gtk_adjustment_get_upper (vadj)));
  self->scroll_speed *= SCROLL_SPEED_ACCELERATION;

  return G_SOURCE_CONTINUE;
}

static gboolean
scroll_up_source (gpointer data)
{
  TracklistWidget * self = Z_TRACKLIST_WIDGET (data);
  GtkAdjustment *   vadj =
    gtk_scrolled_window_get_vadjustment (self->unpinned_scroll);
  gtk_adjustment_set_value (
    vadj,
    MAX (
      gtk_adjustment_get_value (vadj) - self->scroll_speed,
      gtk_adjustment_get_lower (vadj)));
  self->scroll_speed *= SCROLL_SPEED_ACCELERATION;

  return G_SOURCE_CONTINUE;
}

static GdkDragAction
on_dnd_motion (
  GtkDropTarget *   drop_target,
  gdouble           x,
  gdouble           y,
  TracklistWidget * self)
{
  TrackWidget * hit_tw = NULL;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      if (!track_get_should_be_visible (track))
        continue;

      if (!track->widget)
        continue;

      if (ui_is_child_hit (
            GTK_WIDGET (self), GTK_WIDGET (track->widget), 1, 1, x, y, 0, 1))
        {
          hit_tw = track->widget;
        }
      else
        {
          track_widget_do_highlight (track->widget, (int) x, (int) y, 0);
        }
    }

  if (hit_tw)
    {
      graphene_point_t wpt;
      graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT ((float) x, (float) y);
      bool             success = gtk_widget_compute_point (
        GTK_WIDGET (self), GTK_WIDGET (hit_tw), &tmp_pt, &wpt);
      g_return_val_if_fail (success, GDK_ACTION_ASK);
      track_widget_do_highlight (hit_tw, (int) wpt.x, (int) wpt.y, 1);
    }

  int height = gtk_widget_get_height (GTK_WIDGET (self->unpinned_scroll));
  if (
    height > SCROLL_THRESHOLD_PX * 2 + SCROLL_THRESHOLD_PX
    && ui_is_child_hit (
      GTK_WIDGET (self), GTK_WIDGET (self->unpinned_scroll), 1, 1, x, y, 0, 1))
    {
      graphene_point_t wpt;
      graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT ((float) x, (float) y);
      bool             success = gtk_widget_compute_point (
        GTK_WIDGET (self), GTK_WIDGET (self->unpinned_scroll), &tmp_pt, &wpt);
      g_return_val_if_fail (success, GDK_ACTION_ASK);

      /* autoscroll */
      if (height - wpt.y < SCROLL_THRESHOLD_PX)
        {
          if (self->unpinned_scroll_scroll_down_id == 0)
            {
              g_message (
                "begin autoscroll tracklist down: height %d y %f", height,
                wpt.y);
              self->scroll_speed = DEFAULT_SCROLL_SPEED;
              self->unpinned_scroll_scroll_down_id =
                g_timeout_add (100, scroll_down_source, self);
            }
          if (self->unpinned_scroll_scroll_up_id != 0)
            {
              g_source_remove (self->unpinned_scroll_scroll_up_id);
              self->unpinned_scroll_scroll_up_id = 0;
              self->scroll_speed = DEFAULT_SCROLL_SPEED;
            }
        }
      else if (wpt.y < SCROLL_THRESHOLD_PX)
        {
          if (self->unpinned_scroll_scroll_up_id == 0)
            {
              g_message (
                "begin autoscroll tracklist up: height %d y %f", height, wpt.y);
              self->scroll_speed = DEFAULT_SCROLL_SPEED;
              self->unpinned_scroll_scroll_up_id =
                g_timeout_add (100, scroll_up_source, self);
            }
          if (self->unpinned_scroll_scroll_down_id != 0)
            {
              g_source_remove (self->unpinned_scroll_scroll_down_id);
              self->unpinned_scroll_scroll_down_id = 0;
              self->scroll_speed = DEFAULT_SCROLL_SPEED;
            }
        }
      else
        {
          remove_scroll_sources (self);
        }
    }
  else
    {
      remove_scroll_sources (self);
    }

  return GDK_ACTION_MOVE;
}

static void
on_drop_instrument_onto_midi_track (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  const PluginDescriptor * descr = (PluginDescriptor *) user_data;

  if (string_is_equal (response, "create"))
    {
      PluginSetting * setting = plugin_setting_new_default (descr);
      plugin_setting_activate (setting);
      plugin_setting_free (setting);
    }
}

static gboolean
on_dnd_drop (
  GtkDropTarget * drop_target,
  const GValue *  value,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  TracklistWidget * self = Z_TRACKLIST_WIDGET (user_data);
  g_message ("dnd data received on tracklist");

  remove_scroll_sources (self);

  /* get track widget at the x,y point */
  TrackWidget * hit_tw = NULL;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      if (!track_get_should_be_visible (track))
        continue;

      if (ui_is_child_hit (
            GTK_WIDGET (self), GTK_WIDGET (track->widget), 1, 1, x, y, 0, 1))
        {
          hit_tw = track->widget;
        }
      else
        {
          track_widget_do_highlight (track->widget, (int) x, (int) y, 0);
        }
    }

  if (!hit_tw)
    return false;

  Track * this_track = hit_tw->track;
  g_return_val_if_fail (IS_TRACK_AND_NONNULL (this_track), false);

  hit_tw->highlight_loc = TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_NONE;

  if (G_VALUE_HOLDS (value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (g_value_get_object (value));
      switch (wrapped_obj->type)
        {
        case WrappedObjectType::WRAPPED_OBJECT_TYPE_SUPPORTED_FILE:
          /* TODO */
          ui_show_message_literal (
            _ ("Unimplemented"), _ ("Operation unimplemented"));
          return false;
          break;
        case WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR:
          {
            PluginDescriptor * descr = (PluginDescriptor *) wrapped_obj->obj;
            if (
              plugin_descriptor_is_instrument (descr)
              && this_track->type == TrackType::TRACK_TYPE_MIDI)
              {
                /* TODO convert track to instrument */
                AdwMessageDialog * dialog =
                  ADW_MESSAGE_DIALOG (adw_message_dialog_new (
                    GTK_WINDOW (MAIN_WINDOW), _ ("Create New Instrument Track?"),
                    _ ("Attempted to drop an instrument onto a "
                       "MIDI track, which is not supported. "
                       "Create a new instrument track instead?")));
                adw_message_dialog_add_responses (
                  ADW_MESSAGE_DIALOG (dialog), "cancel", _ ("_Cancel"),
                  "create", _ ("Create _Instrument Track"), NULL);
                adw_message_dialog_set_response_appearance (
                  ADW_MESSAGE_DIALOG (dialog), "create", ADW_RESPONSE_SUGGESTED);
                adw_message_dialog_set_default_response (
                  ADW_MESSAGE_DIALOG (dialog), "create");
                adw_message_dialog_set_close_response (
                  ADW_MESSAGE_DIALOG (dialog), "cancel");

                PluginDescriptor * descr_clone = plugin_descriptor_clone (descr);
                g_signal_connect_data (
                  dialog, "response",
                  G_CALLBACK (on_drop_instrument_onto_midi_track), descr_clone,
                  plugin_descriptor_free_closure, (GConnectFlags) 0);

                gtk_window_present (GTK_WINDOW (dialog));
                return true;
              }
            else if (
              plugin_descriptor_is_effect (descr)
              && this_track->out_signal_type == PortType::Audio)
              {
                /* TODO append insert if space left */
              }

            ui_show_message_literal (
              _ ("Unimplemented"), _ ("Operation unimplemented"));
            return false;
          }
          break;
        case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
          break;
        default:
          ui_show_message_literal (
            _ ("Unsupported"), _ ("Dragged item is not supported"));
          return false;
        }
    }
  else if (
    G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)
    || G_VALUE_HOLDS (value, G_TYPE_FILE))
    {
      /* TODO */
      ui_show_message_literal (("Unimplemented"), _ ("Operation unimplemented"));
      return false;
    }
  else
    {
      ui_show_message_literal (
        ("Unsupported"), _ ("Dragged item is not supported"));
      return false;
    }

  /* determine if moving or copying */
  GdkDragAction action = z_gtk_drop_target_get_selected_action (drop_target);

  graphene_point_t wpt;
  graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT ((float) x, (float) y);
  bool             success = gtk_widget_compute_point (
    GTK_WIDGET (self), GTK_WIDGET (hit_tw), &tmp_pt, &wpt);
  g_return_val_if_fail (success, false);

  /* determine position to move to */
  TrackWidgetHighlight location =
    track_widget_get_highlight_location (hit_tw, (int) wpt.y);

  tracklist_handle_move_or_copy (TRACKLIST, this_track, location, action);

  return true;
}

TrackWidget *
tracklist_widget_get_hit_track (TracklistWidget * self, double x, double y)
{
  /* go through each child */
  for (int i = 0; i < self->tracklist->num_tracks; i++)
    {
      Track * track = self->tracklist->tracks[i];
      if (!track_get_should_be_visible (track) || track_is_pinned (track))
        continue;

      TrackWidget * tw = track->widget;

      /* return it if hit */
      if (ui_is_child_hit (GTK_WIDGET (self), GTK_WIDGET (tw), 0, 1, x, y, 0, 0))
        return tw;
    }
  return NULL;
}

static gboolean
on_key_pressed (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  TracklistWidget *       self)
{
  if (state & GDK_CONTROL_MASK && keyval == GDK_KEY_a)
    {
      tracklist_selections_select_all (TRACKLIST_SELECTIONS, 1);

      return true;
    }

  return false;
}

/**
 * Handle ctrl+shift+scroll.
 */
void
tracklist_widget_handle_vertical_zoom_scroll (
  TracklistWidget *          self,
  GtkEventControllerScroll * scroll_controller,
  double                     dy)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (scroll_controller));
  if (!(state & GDK_CONTROL_MASK && state & GDK_SHIFT_MASK))
    return;

  GtkScrolledWindow * scroll = self->unpinned_scroll;

  double y = self->hover_y;
  double delta_y = dy;

  /* get current adjustment so we can get the difference from the cursor */
  GtkAdjustment * adj = gtk_scrolled_window_get_vadjustment (scroll);
  double          adj_val = gtk_adjustment_get_value (adj);
  double          size_before = gtk_adjustment_get_upper (adj);
  double          adj_perc = y / size_before;

  /* get px diff so we can calculate the new
   * adjustment later */
  double diff = y - adj_val;

  /* scroll down, zoom out */
  double size_after;
  double multiplier = 1.08;
  if (delta_y > 0)
    {
      size_after = size_before / multiplier;
    }
  else /* scroll up, zoom in */
    {
      size_after = size_before * multiplier;
    }

  bool can_resize = tracklist_multiply_track_heights (
    self->tracklist, delta_y > 0 ? 1 / multiplier : multiplier, false, true,
    false);
  g_debug ("can resize: %d", can_resize);
  if (can_resize)
    {
      tracklist_multiply_track_heights (
        self->tracklist, delta_y > 0 ? 1 / multiplier : multiplier, false,
        false, true);

      /* get updated adjustment and set its value
       at the same offset as before */
      adj = gtk_scrolled_window_get_vadjustment (scroll);
      gtk_adjustment_set_value (adj, adj_perc * size_after - diff);

      EVENTS_PUSH (EventType::ET_TRACKS_RESIZED, self);
    }
}

static gboolean
on_scroll (
  GtkEventControllerScroll * scroll_controller,
  gdouble                    dx,
  gdouble                    dy,
  gpointer                   user_data)
{
  TracklistWidget * self = Z_TRACKLIST_WIDGET (user_data);

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (scroll_controller));
  if (!(state & GDK_CONTROL_MASK && state & GDK_SHIFT_MASK))
    return false;

  tracklist_widget_handle_vertical_zoom_scroll (self, scroll_controller, dy);

  return true;
}

/**
 * Motion callback.
 */
static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  TracklistWidget *          self)
{
  self->hover_y = MAX (y, 0.0);
}

static void
refresh_track_widget (Track * track)
{
  /* create widget */
  if (!GTK_IS_WIDGET (track->widget))
    track->widget = track_widget_new (track);

  gtk_widget_set_visible (
    (GtkWidget *) track->widget, track_get_should_be_visible (track));

  track_widget_recreate_group_colors (track->widget);
  track_widget_update_icons (track->widget);
  track_widget_update_size (track->widget);
}

/**
 * Refreshes each track without recreating it.
 */
void
tracklist_widget_soft_refresh (TracklistWidget * self)
{
  /** add pinned/unpinned tracks */
  for (int i = 0; i < self->tracklist->num_tracks; i++)
    {
      Track * track = self->tracklist->tracks[i];
      refresh_track_widget (track);
    }
}

/**
 * Deletes all tracks and re-adds them.
 */
void
tracklist_widget_hard_refresh (TracklistWidget * self)
{
  g_debug ("hard refreshing tracklist");

  g_object_ref (self->channel_add);
  g_object_ref (self->ddbox);

  /* remove all children */
  z_gtk_widget_remove_all_children (GTK_WIDGET (self->unpinned_box));
  z_gtk_widget_remove_all_children (GTK_WIDGET (self->pinned_box));

  /** add pinned/unpinned tracks */
  for (int i = 0; i < self->tracklist->num_tracks; i++)
    {
      Track * track = self->tracklist->tracks[i];

      track->widget = NULL;
      refresh_track_widget (track);

      if (track_is_pinned (track))
        gtk_box_append (GTK_BOX (self->pinned_box), GTK_WIDGET (track->widget));
      else
        gtk_box_append (
          GTK_BOX (self->unpinned_box), GTK_WIDGET (track->widget));
    }

  /* re-add chanel_add */
  g_return_if_fail (
    gtk_widget_get_parent (GTK_WIDGET (self->channel_add)) == NULL);
  gtk_box_append (GTK_BOX (self->unpinned_box), GTK_WIDGET (self->channel_add));

  /* re-add ddbox */
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (self->ddbox)) == NULL);
  gtk_box_append (GTK_BOX (self->unpinned_box), GTK_WIDGET (self->ddbox));

  g_object_unref (self->channel_add);
  g_object_unref (self->ddbox);

  g_debug ("done hard refreshing tracklist");
}

/**
 * Makes sure all the tracks for channels marked as
 * visible are visible.
 */
void
tracklist_widget_update_track_visibility (TracklistWidget * self)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      if (!GTK_IS_WIDGET (track->widget))
        continue;

      gtk_widget_set_visible (
        GTK_WIDGET (track->widget), track_get_should_be_visible (track));

      track_widget_update_icons (track->widget);
      track_widget_update_size (track->widget);
    }
}

static void
on_unpinned_scroll_hadj_changed (GtkAdjustment * adj, TracklistWidget * self)
{
  editor_settings_set_scroll_start_y (
    &PRJ_TIMELINE->editor_settings, (int) gtk_adjustment_get_value (adj),
    F_VALIDATE);
}

/**
 * Updates the scroll adjustment.
 */
void
tracklist_widget_set_unpinned_scroll_start_y (TracklistWidget * self, int y)
{
  GtkAdjustment * hadj =
    gtk_scrolled_window_get_vadjustment (self->unpinned_scroll);
  if (!math_doubles_equal ((double) y, gtk_adjustment_get_value (hadj)))
    {
      gtk_adjustment_set_value (hadj, (double) y);
    }
}

void
tracklist_widget_setup (TracklistWidget * self, Tracklist * tracklist)
{
  g_return_if_fail (tracklist);

  self->tracklist = tracklist;
  tracklist->widget = self;

  tracklist_widget_hard_refresh (self);

  self->pinned_size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (
    self->pinned_size_group, GTK_WIDGET (self->pinned_box));
  gtk_size_group_add_widget (
    self->pinned_size_group, GTK_WIDGET (MW_TIMELINE_PANEL->pinned_timeline));

  /* doesn't work */
  self->unpinned_size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (
    self->unpinned_size_group, GTK_WIDGET (self->unpinned_scroll));
  gtk_size_group_add_widget (
    self->unpinned_size_group, GTK_WIDGET (MW_TIMELINE_PANEL->timeline));

  /* add a signal handler to update the editor settings on
   * scroll */
  GtkAdjustment * hadj =
    gtk_scrolled_window_get_vadjustment (self->unpinned_scroll);
  self->unpinned_scroll_vall_changed_handler_id = g_signal_connect (
    hadj, "value-changed", G_CALLBACK (on_unpinned_scroll_hadj_changed), self);

  self->setup = true;
}

/**
 * Prepare for finalization.
 */
void
tracklist_widget_tear_down (TracklistWidget * self)
{
  g_message ("tearing down %p...", self);

  if (self->setup)
    {

      self->setup = false;
    }

  g_message ("done");
}

static gboolean
tracklist_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  TracklistWidget * self = Z_TRACKLIST_WIDGET (user_data);

  GtkAdjustment * vadj =
    gtk_scrolled_window_get_vadjustment (self->unpinned_scroll);
  gtk_adjustment_set_value (vadj, PRJ_TIMELINE->editor_settings.scroll_start_y);

  return G_SOURCE_CONTINUE;
}

static void
dispose (TracklistWidget * self)
{
  remove_scroll_sources (self);

  G_OBJECT_CLASS (tracklist_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
tracklist_widget_init (TracklistWidget * self)
{
  gtk_box_set_spacing (GTK_BOX (self), 1);

  /** add pinned box */
  self->pinned_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 1));
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->pinned_box));
  gtk_widget_set_name (GTK_WIDGET (self->pinned_box), "tracklist-pinned-box");
  gtk_widget_set_margin_bottom (GTK_WIDGET (self->pinned_box), 3);

  self->unpinned_scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_widget_set_name (
    GTK_WIDGET (self->unpinned_scroll), "tracklist-unpinned-scroll");
  gtk_scrolled_window_set_policy (
    self->unpinned_scroll, GTK_POLICY_NEVER, GTK_POLICY_EXTERNAL);
  self->unpinned_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 1));
  gtk_widget_set_name (
    GTK_WIDGET (self->unpinned_box), "tracklist-unpinned-box");

  /* add scrolled window */
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->unpinned_scroll));
  GtkViewport * viewport = GTK_VIEWPORT (gtk_viewport_new (NULL, NULL));
  gtk_viewport_set_child (viewport, GTK_WIDGET (self->unpinned_box));
  gtk_viewport_set_scroll_to_focus (viewport, false);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (self->unpinned_scroll), GTK_WIDGET (viewport));

  /* create the drag dest box and bump its reference
   * so it doesn't get deleted. */
  self->ddbox = drag_dest_box_widget_new (
    GTK_ORIENTATION_VERTICAL, 0, DragDestBoxType::DRAG_DEST_BOX_TYPE_TRACKLIST);
  gtk_widget_set_name (GTK_WIDGET (self->ddbox), "tracklist-ddbox");

  self->channel_add = add_track_menu_button_widget_new ();

  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

  /* set as drag dest for track (the track will
   * be moved based on which half it was dropped in,
   * top or bot) */
  GtkDropTarget * drop_target = gtk_drop_target_new (
    WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, GDK_ACTION_MOVE | GDK_ACTION_COPY);
  gtk_drop_target_set_preload (drop_target, true);
  g_signal_connect (drop_target, "drop", G_CALLBACK (on_dnd_drop), self);
  g_signal_connect (drop_target, "motion", G_CALLBACK (on_dnd_motion), self);
  g_signal_connect (drop_target, "leave", G_CALLBACK (on_dnd_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));

  GtkEventControllerKey * key_controller =
    GTK_EVENT_CONTROLLER_KEY (gtk_event_controller_key_new ());
  g_signal_connect (
    G_OBJECT (key_controller), "key-pressed", G_CALLBACK (on_key_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (key_controller));

  GtkEventControllerScroll * scroll_controller = GTK_EVENT_CONTROLLER_SCROLL (
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll", G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (scroll_controller));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "motion", G_CALLBACK (on_motion), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (motion_controller));

  /* set minimum width */
  gtk_widget_set_size_request (GTK_WIDGET (self), 164, -1);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), tracklist_tick_cb, self, NULL);
}

static void
tracklist_widget_class_init (TracklistWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "tracklist");

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
