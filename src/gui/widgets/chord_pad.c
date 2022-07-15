// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/chord_descriptor.h"
#include "audio/chord_track.h"
#include "audio/midi_event.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/chord_pad.h"
#include "gui/widgets/chord_selector_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (ChordPadWidget, chord_pad_widget, GTK_TYPE_WIDGET)

static ChordDescriptor *
get_chord_descriptor (ChordPadWidget * self)
{
  ChordDescriptor * descr =
    CHORD_EDITOR->chords[self->chord_idx];
  g_return_val_if_fail (descr, NULL);

  return descr;
}

static void
send_note_offs (ChordPadWidget * self)
{
  Track * track = TRACKLIST_SELECTIONS->tracks[0];

  if (track && track->in_signal_type == TYPE_EVENT)
    {
      ChordDescriptor * descr = get_chord_descriptor (self);

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
  g_debug ("pressed");

  ChordPadWidget * self = Z_CHORD_PAD_WIDGET (user_data);

  Track * track = TRACKLIST_SELECTIONS->tracks[0];

  if (track && track->in_signal_type == TYPE_EVENT)
    {
      ChordDescriptor * descr = get_chord_descriptor (self);

      /* send note ons at time 0 */
      Port * port = track->processor->midi_in;
      midi_events_add_note_ons_from_chord_descr (
        port->midi_events, descr, 1, VELOCITY_DEFAULT, 0,
        F_QUEUED);
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
  g_debug ("released");

  ChordPadWidget * self = Z_CHORD_PAD_WIDGET (user_data);

  send_note_offs (self);
}

static void
on_edit_chord_pressed (GtkButton * btn, gpointer user_data)
{
  ChordPadWidget * self = Z_CHORD_PAD_WIDGET (user_data);

  ChordSelectorWindowWidget * chord_selector =
    chord_selector_window_widget_new (self->chord_idx);

  g_debug ("presenting chord selector window");
  gtk_window_present (GTK_WINDOW (chord_selector));
}

static void
on_drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ChordPadWidget * self)
{
  g_message ("drag update");

  if (
    !self->drag_started
    && gtk_drag_check_threshold (
      GTK_WIDGET (self), (int) self->drag_start_x,
      (int) self->drag_start_y,
      (int) (self->drag_start_x + offset_x),
      (int) (self->drag_start_y + offset_y)))
    {
      self->drag_started = true;

      send_note_offs (self);

      ChordDescriptor * descr = get_chord_descriptor (self);
      WrappedObjectWithChangeSignal * wrapped_obj =
        wrapped_object_with_change_signal_new (
          descr, WRAPPED_OBJECT_TYPE_CHORD_DESCR);
      GdkContentProvider * content_providers[] = {
        gdk_content_provider_new_typed (
          WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, wrapped_obj),
      };
      GdkContentProvider * provider =
        gdk_content_provider_new_union (
          content_providers, G_N_ELEMENTS (content_providers));

      GtkNative * native =
        gtk_widget_get_native (GTK_WIDGET (self->btn));
      g_return_if_fail (native);
      GdkSurface * surface = gtk_native_get_surface (native);

      GdkDevice * device =
        gtk_gesture_get_device (GTK_GESTURE (gesture));

      /* begin drag */
      gdk_drag_begin (
        surface, device, provider,
        GDK_ACTION_MOVE | GDK_ACTION_COPY, offset_x, offset_y);
    }
}

static void
on_drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  ChordPadWidget * self)
{
  g_debug ("drag begin");
  self->drag_start_x = start_x;
  self->drag_start_y = start_y;

  gtk_gesture_set_sequence_state (
    GTK_GESTURE (gesture),
    gtk_gesture_single_get_current_sequence (
      GTK_GESTURE_SINGLE (gesture)),
    GTK_EVENT_SEQUENCE_CLAIMED);
}

static void
on_drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ChordPadWidget * self)
{
  g_debug ("drag end");

  send_note_offs (self);

  self->drag_started = false;
  self->drag_start_x = 0;
  self->drag_start_y = 0;
}

#if 0
static void
on_dnd_begin (
  GtkWidget *      widget,
  GdkDragContext * context,
  ChordPadWidget *    self)
{
  g_message ("dnd drag begin");

  gtk_drag_set_icon_name (
    context, "minuet-scales", 0, 0);
}
#endif

static void
on_invert_btn_clicked (GtkButton * btn, ChordPadWidget * self)
{
  const ChordDescriptor * descr = get_chord_descriptor (self);
  g_return_if_fail (descr);
  ChordDescriptor * descr_clone =
    chord_descriptor_clone (descr);

  if (btn == self->invert_prev_btn)
    {
      if (
        chord_descriptor_get_min_inversion (descr)
        != descr->inversion)
        {
          descr_clone->inversion--;
          chord_editor_apply_single_chord (
            CHORD_EDITOR, descr_clone, self->chord_idx,
            F_UNDOABLE);
        }
    }
  else if (btn == self->invert_next_btn)
    {
      if (
        chord_descriptor_get_max_inversion (descr)
        != descr->inversion)
        {
          descr_clone->inversion++;
          chord_editor_apply_single_chord (
            CHORD_EDITOR, descr_clone, self->chord_idx,
            F_UNDOABLE);
        }
    }
}

/**
 * Sets the chord index on the chord widget.
 */
void
chord_pad_widget_refresh (ChordPadWidget * self, int idx)
{
  self->chord_idx = idx;

  ChordDescriptor * descr = get_chord_descriptor (self);
  g_return_if_fail (descr);

  char * lbl = chord_descriptor_to_new_string (descr);
  gtk_button_set_label (self->btn, lbl);
  g_free (lbl);
}

/**
 * Creates a chord widget for the given chord index.
 */
ChordPadWidget *
chord_pad_widget_new (void)
{
  ChordPadWidget * self =
    g_object_new (CHORD_PAD_WIDGET_TYPE, NULL);

  gtk_widget_set_visible (GTK_WIDGET (self), true);
  gtk_widget_set_hexpand (GTK_WIDGET (self), true);
  gtk_widget_set_vexpand (GTK_WIDGET (self), true);

  return self;
}

static void
dispose (ChordPadWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->overlay));

  G_OBJECT_CLASS (chord_pad_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
chord_pad_widget_init (ChordPadWidget * self)
{
  self->overlay = GTK_OVERLAY (gtk_overlay_new ());
  gtk_widget_set_parent (
    GTK_WIDGET (self->overlay), GTK_WIDGET (self));
  gtk_widget_set_name (
    GTK_WIDGET (self->overlay), "chord-overlay");

  /* add main button */
  self->btn = GTK_BUTTON (gtk_button_new_with_label (""));
  gtk_widget_set_name (GTK_WIDGET (self->btn), "chord-btn");
  gtk_overlay_set_child (
    self->overlay, GTK_WIDGET (self->btn));

  GtkGestureClick * click_gesture =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    click_gesture, "pressed",
    G_CALLBACK (on_chord_click_pressed), self);
  g_signal_connect (
    click_gesture, "released",
    G_CALLBACK (on_chord_click_released), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->btn),
    GTK_EVENT_CONTROLLER (click_gesture));

  self->btn_drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    self->btn_drag, "drag-begin", G_CALLBACK (on_drag_begin),
    self);
  g_signal_connect (
    self->btn_drag, "drag-update",
    G_CALLBACK (on_drag_update), self);
  g_signal_connect (
    self->btn_drag, "drag-end", G_CALLBACK (on_drag_end),
    self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->btn),
    GTK_EVENT_CONTROLLER (self->btn_drag));

  self->btn_box =
    GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  gtk_widget_set_halign (
    GTK_WIDGET (self->btn_box), GTK_ALIGN_END);
  gtk_widget_set_valign (
    GTK_WIDGET (self->btn_box), GTK_ALIGN_START);
  gtk_overlay_add_overlay (
    self->overlay, GTK_WIDGET (self->btn_box));

  /* add edit chord btn */
  self->edit_chord_btn = GTK_BUTTON (
    gtk_button_new_from_icon_name ("minuet-scales"));
  gtk_widget_set_name (
    GTK_WIDGET (self->edit_chord_btn), "chord-btn");
  gtk_box_append (
    self->btn_box, GTK_WIDGET (self->edit_chord_btn));
  g_signal_connect (
    self->edit_chord_btn, "clicked",
    G_CALLBACK (on_edit_chord_pressed), self);

  /* add inversion buttons */
  self->invert_prev_btn = GTK_BUTTON (
    gtk_button_new_from_icon_name ("go-previous"));
  gtk_box_append (
    self->btn_box, GTK_WIDGET (self->invert_prev_btn));
  g_signal_connect (
    self->invert_prev_btn, "clicked",
    G_CALLBACK (on_invert_btn_clicked), self);

  self->invert_next_btn =
    GTK_BUTTON (gtk_button_new_from_icon_name ("go-next"));
  gtk_box_append (
    self->btn_box, GTK_WIDGET (self->invert_next_btn));
  g_signal_connect (
    self->invert_next_btn, "clicked",
    G_CALLBACK (on_invert_btn_clicked), self);
}

static void
chord_pad_widget_class_init (ChordPadWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "chord");
  gtk_widget_class_set_layout_manager_type (
    klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
