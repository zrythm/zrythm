/*
 * Copyright (C) 2019 Alexandros Theodotou
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

/** \file
 */

#include "actions/copy_plugins_action.h"
#include "actions/create_tracks_action.h"
#include "actions/move_plugins_action.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/accel.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/file_browser.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "plugins/lv2/symap.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm.h"

G_DEFINE_TYPE (DragDestBoxWidget,
               drag_dest_box_widget,
               GTK_TYPE_EVENT_BOX)

static gboolean
on_drag_motion (GtkWidget        *widget,
               GdkDragContext   *context,
               gint              x,
               gint              y,
               guint             time,
               gpointer          user_data)
{
  GdkAtom target =
    gtk_drag_dest_find_target (
      widget, context, NULL);

  if (target == GDK_NONE)
    {
      gtk_drag_unhighlight (widget);
      gdk_drag_status (context,
                       0,
                       time);
      return FALSE;
    }

  /* if target atom matches FILE DESCR (see
   * file_browser.c gtk_selection_data_set) */
  if (target == GET_ATOM (TARGET_ENTRY_FILE_DESCR))
    {
      FileDescriptor * fd =
        MW_FILE_BROWSER->selected_file_descr;

      if (fd->type >= FILE_TYPE_DIR)
        {
          gtk_drag_unhighlight (widget);
          gdk_drag_status (context,
                           0,
                           time);
          return FALSE;
        }
      else
        {
          gtk_drag_highlight (widget);
          gdk_drag_status (context,
                           GDK_ACTION_COPY,
                           time);
          return TRUE;
        }
    }
  else if (target ==
           GET_ATOM (TARGET_ENTRY_PLUGIN_DESCR))
    {
      gtk_drag_highlight (widget);
      gdk_drag_status (context,
                       GDK_ACTION_COPY,
                       time);
      return TRUE;
    }
  else if (target ==
           GET_ATOM (TARGET_ENTRY_PLUGIN))
    {
      GdkModifierType mask;
      gtk_drag_highlight (widget);
      gdk_window_get_pointer (
        gtk_widget_get_window (widget),
        NULL, NULL, &mask);
      if (mask & GDK_CONTROL_MASK)
        gdk_drag_status (
          context, GDK_ACTION_COPY, time);
      else
        gdk_drag_status (
          context, GDK_ACTION_MOVE, time);
      return TRUE;
    }

  return FALSE;
}

static void
on_drag_data_received (GtkWidget        *widget,
               GdkDragContext   *context,
               gint              x,
               gint              y,
               GtkSelectionData *data,
               guint             info,
               guint             time,
               gpointer          user_data)
{
  GdkAtom target =
    gtk_selection_data_get_target (data);

  if (target == GDK_NONE)
    return;

  if (target == GET_ATOM (TARGET_ENTRY_FILE_DESCR))
    {
      FileDescriptor * fd =
        * (gpointer *)
          gtk_selection_data_get_data (data);

      /* reject the drop if not applicable */
      if (fd->type >= FILE_TYPE_DIR)
        return;

      UndoableAction * ua =
        create_tracks_action_new (
          TRACK_TYPE_AUDIO,
          NULL,
          fd,
          TRACKLIST->num_tracks,
          1);

      undo_manager_perform (UNDO_MANAGER, ua);
    }
  else if ((target ==
            GET_ATOM (TARGET_ENTRY_PLUGIN_DESCR)))
    {
      PluginDescriptor * pd =
        * (gpointer *)
          gtk_selection_data_get_data (data);

      TrackType tt;
      if (g_strcmp0 (pd->category,
                     "Instrument"))
        tt = TRACK_TYPE_INSTRUMENT;
      else
        tt = TRACK_TYPE_BUS;

      UndoableAction * ua =
        create_tracks_action_new (
          tt,
          pd,
          NULL,
          TRACKLIST->num_tracks,
          1);

      undo_manager_perform (UNDO_MANAGER, ua);
    }
  else if ((target =
            GET_ATOM (TARGET_ENTRY_PLUGIN)))
    {
      /* NOTE this is a cloned pointer, don't use
       * it */
      Plugin * pl =
        (Plugin *)
        gtk_selection_data_get_data (data);
      pl = project_get_plugin (pl->id);
      g_warn_if_fail (pl);

      /* determine if moving or copying */
      GdkDragAction action =
        gdk_drag_context_get_selected_action (
          context);

      UndoableAction * ua = NULL;
      if (action == GDK_ACTION_COPY)
        {
          ua =
            copy_plugins_action_new (
              MIXER_SELECTIONS,
              NULL, 0);
        }
      else if (action == GDK_ACTION_MOVE)
        {
          ua =
            move_plugins_action_new (
              MIXER_SELECTIONS,
              NULL, 0);
        }
      g_warn_if_fail (ua);

      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  g_message ("drag");

}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{

}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{


}

static gboolean
on_motion (GtkWidget * widget,
           GdkEvent *event,
           DragDestBoxWidget * self)
{
  if (gdk_event_get_event_type (event) ==
      GDK_ENTER_NOTIFY)
    {
      if (self->type ==
            DRAG_DEST_BOX_TYPE_MIXER)
        bot_bar_change_status (
          "Drag and drop a plugin here from the "
          "browser");
      else if (self->type ==
                 DRAG_DEST_BOX_TYPE_TRACKLIST)
        bot_bar_change_status (
          "Drag and drop a plugin or audio file "
          "here from the browser");
    }
  else if (gdk_event_get_event_type (event) ==
           GDK_LEAVE_NOTIFY)
    {
      bot_bar_change_status ("");
    }

  return FALSE;
}

static void
show_context_menu (DragDestBoxWidget * self)
{
  GtkWidget *menu;
  GtkMenuItem * menu_item;
  menu = gtk_menu_new();

  menu_item =
    z_gtk_create_menu_item (
      "Add _Instrument Track",
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      NULL,
      0,
      "win.create-ins-track");
  gtk_menu_shell_append (GTK_MENU_SHELL(menu),
                         GTK_WIDGET (menu_item));
  menu_item =
    z_gtk_create_menu_item (
      "Add _Audio Track",
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      NULL,
      0,
      "win.create-audio-track");
  gtk_menu_shell_append (GTK_MENU_SHELL(menu),
                         GTK_WIDGET (menu_item));
  menu_item =
    z_gtk_create_menu_item (
      "Add _Bus Track",
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      NULL,
      0,
      "win.create-bus-track");
  gtk_menu_shell_append (GTK_MENU_SHELL(menu),
                         GTK_WIDGET (menu_item));

  gtk_widget_show_all(menu);
  gtk_menu_attach_to_widget (GTK_MENU (menu),
                             GTK_WIDGET (self),
                             NULL);
  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  DragDestBoxWidget * self =
    Z_DRAG_DEST_BOX_WIDGET (user_data);

  tracklist_widget_select_all_tracks (
    MW_TRACKLIST, F_NO_SELECT);

  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  DragDestBoxWidget * self =
    Z_DRAG_DEST_BOX_WIDGET (user_data);

  GdkModifierType state_mask;
  ui_get_modifier_type_from_gesture (
    GTK_GESTURE_SINGLE (gesture),
    &state_mask);

  if (self == TRACKLIST_DRAG_DEST_BOX)
    {
      if (!(state_mask & GDK_SHIFT_MASK ||
          state_mask & GDK_CONTROL_MASK))
        tracklist_widget_select_all_tracks (
          MW_TRACKLIST, F_NO_SELECT);
    }

  mixer_selections_clear (
    MIXER_SELECTIONS);
  tracklist_selections_clear (
    TRACKLIST_SELECTIONS);
}

/**
 * Creates a drag destination box widget.
 */
DragDestBoxWidget *
drag_dest_box_widget_new (GtkOrientation  orientation,
                          int             spacing,
                          DragDestBoxType type)
{
  /* create */
  DragDestBoxWidget * self =
    g_object_new (DRAG_DEST_BOX_WIDGET_TYPE,
                  NULL);

  self->type = type;

  if (type == DRAG_DEST_BOX_TYPE_MIXER)
    {
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        20,
        -1);
    }

  /* make expandable */
  gtk_widget_set_vexpand (GTK_WIDGET (self),
                          1);
  gtk_widget_set_hexpand (GTK_WIDGET (self),
                          1);

  /* set as drag dest */
  GtkTargetEntry entries[3];
  entries[0].target = TARGET_ENTRY_PLUGIN_DESCR;
  entries[0].flags = GTK_TARGET_SAME_APP;
  entries[0].info =
    symap_map (
      ZSYMAP, TARGET_ENTRY_PLUGIN_DESCR);
  entries[1].target = TARGET_ENTRY_FILE_DESCR;
  entries[1].flags = GTK_TARGET_SAME_APP;
  entries[1].info =
    symap_map (ZSYMAP, TARGET_ENTRY_FILE_DESCR);
  entries[2].target = TARGET_ENTRY_PLUGIN;
  entries[2].flags = GTK_TARGET_SAME_APP;
  entries[2].info =
    symap_map (ZSYMAP, TARGET_ENTRY_PLUGIN);
  gtk_drag_dest_set (GTK_WIDGET (self),
                     GTK_DEST_DEFAULT_ALL,
                     entries,
                     3,
                     GDK_ACTION_COPY);

  /* connect signal */
  g_signal_connect (
    GTK_WIDGET (self), "drag-motion",
    G_CALLBACK(on_drag_motion), NULL);
  g_signal_connect (
    GTK_WIDGET (self), "drag-data-received",
    G_CALLBACK(on_drag_data_received), NULL);

  /* show */
  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  return self;
}

/**
 * GTK boilerplate.
 */
static void
drag_dest_box_widget_init (DragDestBoxWidget * self)
{
  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  self->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp),
    GDK_BUTTON_SECONDARY);
  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));


  /* make widget able to notify */
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
}

static void
drag_dest_box_widget_class_init (
  DragDestBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "drag-dest-box");
}
