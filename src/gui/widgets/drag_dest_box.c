/*
 * gui/widgets/drag_dest_box.c - A dnd destination box used by mixer and tracklist
 *                                widgets
 *
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

#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/ui.h"

G_DEFINE_TYPE (DragDestBoxWidget,
               drag_dest_box_widget,
               GTK_TYPE_EVENT_BOX)

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
  DragDestBoxWidget * self = Z_DRAG_DEST_BOX_WIDGET (widget);
  if (self == TRACKLIST_DRAG_DEST_BOX ||
      self == MIXER_DRAG_DEST_BOX)
    {
      PluginDescriptor * descr =
        *(gpointer *) gtk_selection_data_get_data (data);
      mixer_add_channel_from_plugin_descr (descr);
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


static void
show_context_menu (DragDestBoxWidget * self)
{
  GtkWidget *menu, *menuitem;
  menu = gtk_menu_new();

  menuitem =
    gtk_menu_item_new_with_mnemonic (
      "Add _Instrument Track");
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (menuitem),
    "win.create-ins-track");
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  menuitem =
    gtk_menu_item_new_with_mnemonic (
      "Add _Audio Track");
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (menuitem),
    "win.create-audio-track");
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  menuitem =
    gtk_menu_item_new_with_mnemonic (
      "Add _Bus Track");
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (menuitem),
    "win.create-bus-track");
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);
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

  tracklist_widget_toggle_select_all_tracks (
    MW_TRACKLIST, 0);
  g_message ("elo");

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
  g_message ("mp");

  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  const GdkEvent * event =
    gtk_gesture_get_last_event (GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);

  if (self == TRACKLIST_DRAG_DEST_BOX)
    {
      if (!(state_mask & GDK_SHIFT_MASK ||
          state_mask & GDK_CONTROL_MASK))
        tracklist_widget_toggle_select_all_tracks (
          MW_TRACKLIST, 0);
    }
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
  DragDestBoxWidget * self = g_object_new (
                            DRAG_DEST_BOX_WIDGET_TYPE,
                            NULL);

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
  GtkTargetEntry entries[1];
  entries[0].target = TARGET_ENTRY_PLUGIN_DESCR;
  entries[0].flags = GTK_TARGET_SAME_APP;
  entries[0].info = 0;
  gtk_drag_dest_set (GTK_WIDGET (self),
                     GTK_DEST_DEFAULT_ALL,
                     entries,
                     1,
                     GDK_ACTION_COPY);

  /* connect signal */
  g_signal_connect (GTK_WIDGET (self),
                    "drag-data-received",
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
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (G_OBJECT (self->right_mouse_mp),
                    "pressed",
                    G_CALLBACK (on_right_click),
                    self);
  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
}

static void
drag_dest_box_widget_class_init (
  DragDestBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "drag-dest-box");
}
