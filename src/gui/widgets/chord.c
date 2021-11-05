/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/chord_descriptor.h"
#include "audio/chord_track.h"
#include "audio/midi_event.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/clip_editor.h"
#include "gui/widgets/chord.h"
#include "gui/widgets/chord_selector_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/symap.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  ChordWidget, chord_widget, GTK_TYPE_WIDGET)

static ChordDescriptor *
get_chord_descriptor (
  ChordWidget * self)
{
  ChordDescriptor * descr =
    CHORD_EDITOR->chords[self->idx];
  g_return_val_if_fail (descr, NULL);

  return descr;
}

static void
send_note_offs (
  ChordWidget * self)
{
  Track * track = TRACKLIST_SELECTIONS->tracks[0];

  if (track && track->in_signal_type == TYPE_EVENT)
    {
      ChordDescriptor * descr =
        get_chord_descriptor (self);

      /* send note offs at time 1 */
      Port * port = track->processor->midi_in;
      midi_events_add_note_offs_from_chord_descr (
        port->midi_events, descr, 1, 1, F_QUEUED);
    }
}

static void
on_chord_click_pressed (
  GtkGestureClick * click_gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  ChordWidget * self = Z_CHORD_WIDGET (user_data);

  Track * track = TRACKLIST_SELECTIONS->tracks[0];

  if (track && track->in_signal_type == TYPE_EVENT)
    {
      ChordDescriptor * descr =
        get_chord_descriptor (self);

      /* send note ons at time 0 */
      Port * port = track->processor->midi_in;
      midi_events_add_note_ons_from_chord_descr (
        port->midi_events, descr, 1,
        VELOCITY_DEFAULT, 0, F_QUEUED);
    }
}

static void
on_chord_click_released (
  GtkGestureClick * click_gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  ChordWidget * self = Z_CHORD_WIDGET (user_data);

  send_note_offs (self);
}

static void
on_edit_chord_pressed (
  GtkGestureClick * click_gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  ChordWidget * self = Z_CHORD_WIDGET (user_data);

  ChordDescriptor * descr =
    get_chord_descriptor (self);
  ChordSelectorWindowWidget * chord_selector =
    chord_selector_window_widget_new (descr);

  gtk_window_present (
    GTK_WINDOW (chord_selector));
}

static void
on_drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ChordWidget *    self)
{
  g_message ("drag update");

  if (!self->drag_started
      &&
      gtk_drag_check_threshold (
        GTK_WIDGET (self),
        (int) self->drag_start_x,
        (int) self->drag_start_y,
        (int) (self->drag_start_x + offset_x),
        (int) (self->drag_start_y + offset_y)))
    {
      self->drag_started = true;

      send_note_offs (self);

      ChordDescriptor * descr =
        get_chord_descriptor (self);

      char chord_descr_str[600];
      sprintf (
        chord_descr_str,
        CHORD_DESCRIPTOR_DND_PREFIX "%p", descr);

      GdkContentProvider * content_providers[] = {
        gdk_content_provider_new_typed (
          G_TYPE_STRING, chord_descr_str),
      };
      GdkContentProvider * provider =
        gdk_content_provider_new_union (
          content_providers,
          G_N_ELEMENTS (content_providers));

      GtkNative * native =
        gtk_widget_get_native (
          GTK_WIDGET (self->btn));
      g_return_if_fail (native);
      GdkSurface * surface =
        gtk_native_get_surface (native);

      GdkDevice * device =
        gtk_gesture_get_device (
          GTK_GESTURE (gesture));

      /* begin drag */
      gdk_drag_begin (
        surface, device, provider,
        GDK_ACTION_MOVE | GDK_ACTION_COPY,
        offset_x,
        offset_y);
    }
}

static void
on_drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  ChordWidget *    self)
{
  self->drag_start_x = start_x;
  self->drag_start_y = start_y;
}

static void
on_drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ChordWidget *    self)
{
  g_message ("drag end");

  self->drag_started = false;
  self->drag_start_x = 0;
  self->drag_start_y = 0;
}

#if 0
static void
on_dnd_begin (
  GtkWidget *      widget,
  GdkDragContext * context,
  ChordWidget *    self)
{
  g_message ("dnd drag begin");

  gtk_drag_set_icon_name (
    context, "minuet-scales", 0, 0);
}
#endif

/**
 * Sets the chord index on the chord widget.
 */
void
chord_widget_setup (
  ChordWidget * self,
  int           idx)
{
  self->idx = idx;

  z_gtk_widget_destroy_all_children (
    GTK_WIDGET (self->overlay));

  /* add main button */
  ChordDescriptor * descr =
    get_chord_descriptor (self);
  g_return_if_fail (descr);
  char * lbl =
    chord_descriptor_to_new_string (descr);
  self->btn =
    GTK_BUTTON (gtk_button_new_with_label (lbl));

  gtk_widget_set_visible (
    GTK_WIDGET (self->btn), true);
  g_free (lbl);
  gtk_overlay_set_child (
    self->overlay, GTK_WIDGET (self->btn));

  GtkGestureClick * click_gesture =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  g_signal_connect (
    click_gesture, "pressed",
    G_CALLBACK (on_chord_click_pressed), self);
  g_signal_connect (
    click_gesture, "released",
    G_CALLBACK (on_chord_click_released), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->btn),
    GTK_EVENT_CONTROLLER (click_gesture));

  self->btn_drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new ());
  g_signal_connect (
    self->btn_drag, "drag-begin",
    G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    self->btn_drag, "drag-update",
    G_CALLBACK (on_drag_update), self);
  g_signal_connect (
    self->btn_drag, "drag-end",
    G_CALLBACK (on_drag_end), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->btn),
    GTK_EVENT_CONTROLLER (self->btn_drag));

  /* add edit chord btn */
  self->edit_chord_btn =
    GTK_BUTTON (
      gtk_button_new_from_icon_name (
        "minuet-scales"));
  gtk_overlay_add_overlay (
    self->overlay,
    GTK_WIDGET (self->edit_chord_btn));
  gtk_widget_set_halign (
    GTK_WIDGET (self->edit_chord_btn),
    GTK_ALIGN_END);
  gtk_widget_set_valign (
    GTK_WIDGET (self->edit_chord_btn),
    GTK_ALIGN_START);
  GtkGestureClick * edit_click_gesture =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  g_signal_connect (
    edit_click_gesture, "pressed",
    G_CALLBACK (on_edit_chord_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->edit_chord_btn),
    GTK_EVENT_CONTROLLER (edit_click_gesture));
}

/**
 * Creates a chord widget for the given chord index.
 */
ChordWidget *
chord_widget_new (void)
{
  ChordWidget * self =
    g_object_new (CHORD_WIDGET_TYPE, NULL);

  gtk_widget_set_visible (GTK_WIDGET (self), true);
  gtk_widget_set_hexpand (GTK_WIDGET (self), true);
  gtk_widget_set_vexpand (GTK_WIDGET (self), true);

  return self;
}

static void
chord_widget_init (
  ChordWidget * self)
{
  self->overlay =
    GTK_OVERLAY (gtk_overlay_new ());
  gtk_widget_set_parent (
    GTK_WIDGET (self->overlay), GTK_WIDGET (self));
}

static void
chord_widget_class_init (
  ChordWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "chord");
}
