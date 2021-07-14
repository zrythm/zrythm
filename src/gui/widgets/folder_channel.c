/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include <time.h>
#include <sys/time.h>

#include "actions/tracklist_selections.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/master_track.h"
#include "audio/meter.h"
#include "audio/track.h"
#include "gui/widgets/balance_control.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/folder_channel.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/fader_buttons.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/expander_box.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/plugin_strip_expander.h"
#include "gui/widgets/route_target_selector.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  FolderChannelWidget, folder_channel_widget,
  GTK_TYPE_EVENT_BOX)

static void
on_drag_data_received (
  GtkWidget        *widget,
  GdkDragContext   *context,
  gint              x,
  gint              y,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  FolderChannelWidget * self)
{
  g_message ("drag data received");
  Track * this = self->track;

  /* determine if moving or copying */
  GdkDragAction action =
    gdk_drag_context_get_selected_action (
      context);

  int w =
    gtk_widget_get_allocated_width (widget);

  /* determine position to move to */
  int pos;
  if (x < w / 2)
    {
      if (this->pos <=
          MW_MIXER->start_drag_track->pos)
        pos = this->pos;
      else
        {
          Track * prev =
            tracklist_get_prev_visible_track (
              TRACKLIST, this);
          pos =
            prev ? prev->pos : this->pos;
        }
    }
  else
    {
      if (this->pos >=
          MW_MIXER->start_drag_track->pos)
        pos = this->pos;
      else
        {
          Track * next =
            tracklist_get_next_visible_track (
              TRACKLIST, this);
          pos =
            next ? next->pos : this->pos;
        }
    }

  tracklist_selections_select_foldable_children (
    TRACKLIST_SELECTIONS);

  UndoableAction * ua = NULL;
  if (action == GDK_ACTION_COPY)
    {
      ua =
        tracklist_selections_action_new_copy (
          TRACKLIST_SELECTIONS, pos);
    }
  else if (action == GDK_ACTION_MOVE)
    {
      ua =
        tracklist_selections_action_new_move (
          TRACKLIST_SELECTIONS, pos);
    }

  g_warn_if_fail (ua);

  undo_manager_perform (
    UNDO_MANAGER, ua);
}

static void
on_drag_data_get (
  GtkWidget        *widget,
  GdkDragContext   *context,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  FolderChannelWidget * self)
{
  g_message ("drag data get");

  /* Not really needed since the selections are
   * used. just send master */
  gtk_selection_data_set (
    data,
    gdk_atom_intern_static_string (
      TARGET_ENTRY_TRACK),
    32,
    (const guchar *) &P_MASTER_TRACK,
    sizeof (P_MASTER_TRACK));
}

/**
 * For drag n drop.
 */
static void
on_dnd_drag_begin (
  GtkWidget      *widget,
  GdkDragContext *context,
  FolderChannelWidget * self)
{
  Track * track = self->track;
  self->selected_in_dnd = 1;
  MW_MIXER->start_drag_track = track;

  if (self->n_press == 1)
    {
      int ctrl = 0, selected = 0;

      ctrl = self->ctrl_held_at_start;

      if (tracklist_selections_contains_track (
            TRACKLIST_SELECTIONS,
            track))
        selected = 1;

      /* no control & not selected */
      if (!ctrl && !selected)
        {
          tracklist_selections_select_single (
            TRACKLIST_SELECTIONS,
            track, F_PUBLISH_EVENTS);
        }
      else if (!ctrl && selected)
        { }
      else if (ctrl && !selected)
        tracklist_selections_add_track (
          TRACKLIST_SELECTIONS, track, 1);
    }
}

/**
 * Highlights/unhighlights the folder_channels
 * appropriately.
 */
static void
do_highlight (
  FolderChannelWidget * self,
  gint x,
  gint y)
{
  /* if we are closer to the start of selection than
   * the end */
  int w =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));
  if (x < w / 2)
    {
      /* highlight left */
      gtk_drag_highlight (
        GTK_WIDGET (
          self->highlight_left_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (
          self->highlight_left_box),
        2, -1);

      /* unhilight right */
      gtk_drag_unhighlight (
        GTK_WIDGET (
          self->highlight_right_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (
          self->highlight_right_box),
        -1, -1);
    }
  else
    {
      /* highlight right */
      gtk_drag_highlight (
        GTK_WIDGET (
          self->highlight_right_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (
          self->highlight_right_box),
        2, -1);

      /* unhilight left */
      gtk_drag_unhighlight (
        GTK_WIDGET (
          self->highlight_left_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (
          self->highlight_left_box),
        -1, -1);
    }
}

static void
on_drag_motion (
  GtkWidget *widget,
  GdkDragContext *context,
  gint x,
  gint y,
  guint time,
  FolderChannelWidget * self)
{
  GdkModifierType mask;
  z_gtk_widget_get_mask (
    widget, &mask);
  if (mask & GDK_CONTROL_MASK)
    gdk_drag_status (context, GDK_ACTION_COPY, time);
  else
    gdk_drag_status (context, GDK_ACTION_MOVE, time);

  do_highlight (self, x, y);
}

static void
on_drag_leave (
  GtkWidget *      widget,
  GdkDragContext * context,
  guint            time,
  FolderChannelWidget *  self)
{
  g_message ("on_drag_leave");

  /*do_highlight (self);*/
  gtk_drag_unhighlight (
    GTK_WIDGET (self->highlight_left_box));
  gtk_widget_set_size_request (
    GTK_WIDGET (self->highlight_left_box),
    -1, -1);
  gtk_drag_unhighlight (
    GTK_WIDGET (self->highlight_right_box));
  gtk_widget_set_size_request (
    GTK_WIDGET (self->highlight_right_box),
    -1, -1);
}

/**
 * Callback when somewhere in the folder_channel is
 * pressed.
 *
 * Only responsible for setting the tracklist
 * selection and should not do anything else.
 */
static void
on_whole_folder_channel_press (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  FolderChannelWidget * self)
{
  self->n_press = n_press;

  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));
  self->ctrl_held_at_start =
    state_mask & GDK_CONTROL_MASK;
}

static void
on_drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  FolderChannelWidget *  self)
{
  self->selected_in_dnd = 0;
  self->dragged = 0;
}

static void
on_drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  FolderChannelWidget *  self)
{
  self->dragged = true;
}

static bool
on_btn_release (
  GtkWidget *      widget,
  GdkEventButton * event,
  FolderChannelWidget *  self)
{
  if (self->dragged || self->selected_in_dnd)
    return false;

  Track * track = self->track;
  if (self->n_press == 1)
    {
      PROJECT->last_selection =
        SELECTION_TYPE_TRACKLIST;

      bool ctrl = event->state & GDK_CONTROL_MASK;
      bool shift = event->state & GDK_SHIFT_MASK;
      tracklist_selections_handle_click (
        track, ctrl, shift, self->dragged);
    }

  return FALSE;
}

static void
refresh_color (
  FolderChannelWidget * self)
{
  Track * track = self->track;
  color_area_widget_setup_track (
    self->color_top, track);
  color_area_widget_set_color (
    self->color_top, &track->color);
  color_area_widget_set_color (
    self->color_left, &track->color);
}

static void
setup_folder_channel_icon (
  FolderChannelWidget * self)
{
  Track * track = self->track;
  gtk_image_set_from_icon_name (
    self->icon, track->icon_name,
    GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->icon),
    track_is_enabled (track));
}

static void
refresh_name (FolderChannelWidget * self)
{
  Track * track = self->track;
  if (track_is_enabled (track))
    {
      gtk_label_set_text (
        GTK_LABEL (self->name->label), track->name);
    }
  else
    {
      char * markup =
        g_strdup_printf (
          "<span foreground=\"grey\">%s</span>",
          track->name);
      gtk_label_set_markup (
        GTK_LABEL (self->name->label), markup);
    }

  gtk_label_set_angle (self->name->label, 90);
}

/**
 * Updates everything on the widget.
 *
 * It is reduntant but keeps code organized. Should fix if it causes lags.
 */
void
folder_channel_widget_refresh (
  FolderChannelWidget * self)
{
  refresh_name (self);
  refresh_color (self);
  setup_folder_channel_icon (self);

#define ICON_NAME_FOLD "fluentui-folder-regular"
#define ICON_NAME_FOLD_OPEN "fluentui-folder-open-regular"

  Track * track = self->track;
  z_gtk_button_set_icon_name (
    GTK_BUTTON (self->fold_toggle),
    track->folded ?
      ICON_NAME_FOLD : ICON_NAME_FOLD_OPEN);

#undef ICON_NAME_FOLD
#undef ICON_NAME_FOLD_OPEN

  g_signal_handler_block (
    self->fold_toggle,
    self->fold_toggled_handler_id);
  gtk_toggle_button_set_active (
    self->fold_toggle, !track->folded);
  g_signal_handler_unblock (
    self->fold_toggle,
    self->fold_toggled_handler_id);

  if (track_is_selected (track))
    {
      /* set selected or not */
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_SELECTED, 0);
    }
  else
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_SELECTED);
    }
}

static void
show_context_menu (
  FolderChannelWidget * self)
{
  GtkWidget *menu;
  GtkMenuItem *menuitem;
  menu = gtk_menu_new();
  Track * track = self->track;

#define APPEND(mi) \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), \
    GTK_WIDGET (menuitem));

#define ADD_SEPARATOR \
  menuitem = \
    GTK_MENU_ITEM ( \
      gtk_separator_menu_item_new ()); \
  gtk_widget_set_visible ( \
    GTK_WIDGET (menuitem), true); \
  APPEND (menuitem)

  int num_selected =
    TRACKLIST_SELECTIONS->num_tracks;

  if (num_selected > 0)
    {
      char * str;

      if (track->type != TRACK_TYPE_MASTER &&
          track->type != TRACK_TYPE_CHORD &&
          track->type != TRACK_TYPE_MARKER &&
          track->type != TRACK_TYPE_TEMPO)
        {
          /* delete track */
          if (num_selected == 1)
            str =
              g_strdup (_("_Delete Track"));
          else
            str =
              g_strdup (_("_Delete Tracks"));
          menuitem =
            z_gtk_create_menu_item (
              str, "edit-delete", F_NO_TOGGLE,
              "win.delete-selected-tracks");
          g_free (str);
          APPEND (menuitem);

          /* duplicate track */
          if (num_selected == 1)
            str =
              g_strdup (_("_Duplicate Track"));
          else
            str =
              g_strdup (_("_Duplicate Tracks"));
          menuitem =
            z_gtk_create_menu_item (
              str, "edit-copy", F_NO_TOGGLE,
              "win.duplicate-selected-tracks");
          g_free (str);
          APPEND (menuitem);
        }

      menuitem =
        z_gtk_create_menu_item (
          num_selected == 1 ?
            _("Hide Track") :
            _("Hide Tracks"),
          "view-hidden", F_NO_TOGGLE,
          "win.hide-selected-tracks");
      APPEND (menuitem);

      menuitem =
        z_gtk_create_menu_item (
          num_selected == 1 ?
            _("Pin/Unpin Track") :
            _("Pin/Unpin Tracks"),
          "window-pin", F_NO_TOGGLE,
          "win.pin-selected-tracks");
      APPEND (menuitem);
    }

  /* add solo/mute/listen */
  ADD_SEPARATOR;

  if (tracklist_selections_contains_soloed_track (
        TRACKLIST_SELECTIONS, F_NO_SOLO))
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Solo"), "solo", F_NO_TOGGLE,
          "win.solo-selected-tracks");
      APPEND (menuitem);
    }
  if (tracklist_selections_contains_soloed_track (
        TRACKLIST_SELECTIONS, F_SOLO))
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Unsolo"), "unsolo", F_NO_TOGGLE,
          "win.unsolo-selected-tracks");
      APPEND (menuitem);
    }

  if (tracklist_selections_contains_muted_track (
        TRACKLIST_SELECTIONS, F_NO_MUTE))
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Mute"), "mute", F_NO_TOGGLE,
          "win.mute-selected-tracks");
      APPEND (menuitem);
    }
  if (tracklist_selections_contains_muted_track (
        TRACKLIST_SELECTIONS, F_MUTE))
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Unmute"), "unmute", F_NO_TOGGLE,
          "win.unmute-selected-tracks");
      APPEND (menuitem);
    }

  if (tracklist_selections_contains_listened_track (
        TRACKLIST_SELECTIONS, F_NO_LISTEN))
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Listen"), "listen",
          F_NO_TOGGLE,
          "win.listen-selected-tracks");
      APPEND (menuitem);
    }
  if (tracklist_selections_contains_listened_track (
        TRACKLIST_SELECTIONS, F_LISTEN))
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Unlisten"), "unlisten",
          F_NO_TOGGLE,
          "win.unlisten-selected-tracks");
      APPEND (menuitem);
    }

  /* add enable/disable */
  if (tracklist_selections_contains_enabled_track (
        TRACKLIST_SELECTIONS, F_ENABLED))
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Disable"), "offline",
          F_NO_TOGGLE,
          "win.disable-selected-tracks");
      APPEND (menuitem);
    }
  else
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Enable"), "online",
          F_NO_TOGGLE,
          "win.enable-selected-tracks");
      APPEND (menuitem);
    }

  ADD_SEPARATOR;
  menuitem =
    z_gtk_create_menu_item (
      _("Change color..."), "color-fill",
      F_NO_TOGGLE, "win.change-track-color");
  APPEND (menuitem);

#undef APPEND
#undef ADD_SEPARATOR

  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static void
on_right_click (
  GtkGestureMultiPress * gesture,
  gint                    n_press,
  gdouble                x,
  gdouble                y,
  FolderChannelWidget *        self)
{
  GdkModifierType state_mask =
    ui_get_state_mask (GTK_GESTURE (gesture));

  Track * track = self->track;
  if (!track_is_selected (track))
    {
      if (state_mask & GDK_SHIFT_MASK ||
          state_mask & GDK_CONTROL_MASK)
        {
          track_select (
            track, F_SELECT, 0, 1);
        }
      else
        {
          track_select (
            track, F_SELECT, 1, 1);
        }
    }
  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

static void
on_fold_toggled (
  GtkToggleButton *     toggle,
  FolderChannelWidget * self)
{
  bool folded =
    !gtk_toggle_button_get_active (toggle);

  track_set_folded (
    self->track, folded,
    F_TRIGGER_UNDO, F_AUTO_SELECT,
    F_PUBLISH_EVENTS);
}

static void
on_destroy (
  FolderChannelWidget * self)
{
  folder_channel_widget_tear_down (self);
}

FolderChannelWidget *
folder_channel_widget_new (
  Track * track)
{
  FolderChannelWidget * self =
    g_object_new (FOLDER_CHANNEL_WIDGET_TYPE, NULL);
  self->track = track;

  setup_folder_channel_icon (self);
  editable_label_widget_setup (
    self->name, track,
    (GenericStringGetter) track_get_name,
    (GenericStringSetter)
    track_set_name_with_action);

  self->fold_toggled_handler_id =
    g_signal_connect (
      self->fold_toggle, "toggled",
      G_CALLBACK (on_fold_toggled), self);
  g_signal_connect (
    self, "destroy",
    G_CALLBACK (on_destroy), NULL);

  folder_channel_widget_refresh (self);

  g_object_ref (self);

  self->setup = true;

  return self;
}

/**
 * Prepare for finalization.
 */
void
folder_channel_widget_tear_down (
  FolderChannelWidget * self)
{
  if (self->setup)
    {
      g_object_unref (self);
      self->track->folder_ch_widget = NULL;
      self->setup = false;
    }
}

static void
folder_channel_widget_class_init (
  FolderChannelWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "folder_channel.ui");
  gtk_widget_class_set_css_name (
    klass, "folder_channel");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, FolderChannelWidget, x)

  BIND_CHILD (color_left);
  BIND_CHILD (color_top);
  BIND_CHILD (grid);
  BIND_CHILD (icon_and_name_event_box);
  BIND_CHILD (name);
  BIND_CHILD (fader_buttons);
  BIND_CHILD (icon);
  BIND_CHILD (fold_toggle);
  BIND_CHILD (highlight_left_box);
  BIND_CHILD (highlight_right_box);

#undef BIND_CHILD
}

static void
folder_channel_widget_init (
  FolderChannelWidget * self)
{
  g_type_ensure (FADER_BUTTONS_WIDGET_TYPE);
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  /* set font sizes */
  GtkStyleContext * context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self->name->label));
  gtk_style_context_add_class (
    context, "folder_channel_label");
  gtk_label_set_max_width_chars (
    self->name->label, 10);

  char * entry_track = g_strdup (TARGET_ENTRY_TRACK);
  GtkTargetEntry entries[] = {
    {
      entry_track, GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_TRACK),
    },
  };

  /* set as drag source for track */
  gtk_drag_source_set (
    GTK_WIDGET (self->icon_and_name_event_box),
    GDK_BUTTON1_MASK,
    entries, G_N_ELEMENTS (entries),
    GDK_ACTION_MOVE | GDK_ACTION_COPY);

  /* set as drag dest for folder_channel (the folder_channel will
   * be moved based on which half it was dropped in,
   * left or right) */
  gtk_drag_dest_set (
    GTK_WIDGET (self),
    GTK_DEST_DEFAULT_MOTION |
      GTK_DEST_DEFAULT_DROP,
    entries, G_N_ELEMENTS (entries),
    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  g_free (entry_track);

  self->mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (
        GTK_WIDGET (
          self)));

  gtk_widget_set_hexpand (
    GTK_WIDGET (self), 0);

  self->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->icon_and_name_event_box)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp),
    GDK_BUTTON_SECONDARY);

  g_signal_connect (
    G_OBJECT (self->mp), "pressed",
    G_CALLBACK (on_whole_folder_channel_press), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin",
    G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update",
    G_CALLBACK (on_drag_update), self);
  g_signal_connect_after (
    GTK_WIDGET (self->icon_and_name_event_box),
    "drag-begin",
    G_CALLBACK(on_dnd_drag_begin), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-data-received",
    G_CALLBACK(on_drag_data_received), self);
  g_signal_connect (
    GTK_WIDGET (self->icon_and_name_event_box),
    "drag-data-get",
    G_CALLBACK (on_drag_data_get), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-motion",
    G_CALLBACK (on_drag_motion), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-leave",
    G_CALLBACK (on_drag_leave), self);
  g_signal_connect (
    GTK_WIDGET (self), "button-release-event",
    G_CALLBACK (on_btn_release), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
}
