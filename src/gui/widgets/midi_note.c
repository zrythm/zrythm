/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "utils/ui.h"

G_DEFINE_TYPE (MidiNoteWidget,
               midi_note_widget,
               GTK_TYPE_BOX)

/**
 * Space on the edges to show resize cursors
 */
#define RESIZE_CURSOR_SPACE 9

static gboolean
midi_note_draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  MidiNoteWidget * self)
{
  if (!GTK_IS_WIDGET (self))
    return FALSE;

  /*g_message ("drawing %d", self->midi_note->id);*/
  guint width, height;
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (widget);

  width =
    gtk_widget_get_allocated_width (widget);
  height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  MidiNote * mn = self->midi_note;
  Region * region = mn->region;
  GdkRGBA * track_color =
    &region->lane->track->color;
  Position global_start_pos;
  midi_note_get_global_start_pos (
    mn, &global_start_pos);
  ChordObject * co =
    chord_track_get_chord_at_pos (
      P_CHORD_TRACK, &global_start_pos);
  ScaleObject * so =
    chord_track_get_scale_at_pos (
      P_CHORD_TRACK, &global_start_pos);
  GdkRGBA color;
  int in_scale =
    so && musical_scale_is_key_in_scale (
      so->scale, mn->val % 12);
  int in_chord =
    co && chord_descriptor_is_key_in_chord (
      co->descr, mn->val % 12);

  if (PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_BOTH &&
      in_scale && in_chord)
    {
      gdk_rgba_parse (&color, "#FF22FF");
    }
  else if ((PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_SCALE ||
      PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_BOTH) && in_scale)
    {
      gdk_rgba_parse (&color, "#662266");
    }
  else if ((PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_CHORD ||
      PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_BOTH) && in_chord)
    {
      gdk_rgba_parse (&color, "#BB22BB");
    }
  else
    {
      gdk_rgba_parse (
        &color,
        gdk_rgba_to_string (track_color));
    }

  /* draw notes of main region */
  if (region == CLIP_EDITOR->region)
    {
      cairo_set_source_rgba (
        cr,
        color.red,
        color.green,
        color.blue,
        midi_note_is_transient (self->midi_note) ? 0.7 : 1);
      if (midi_note_is_selected (self->midi_note))
        {
          cairo_set_source_rgba (
            cr, color.red + 0.4,
            color.green + 0.2,
            color.blue + 0.2, 1);
        }
      if (PIANO_ROLL->drum_mode)
        {
          z_cairo_diamond (cr, 0, 0, width, height);
        }
      else
        {
          z_cairo_rounded_rectangle (
            cr, 0, 0, width, height, 1.0, 4.0);
        }
      cairo_fill(cr);
    }
  /* draw other notes */
  else
    {
      cairo_set_source_rgba (
        cr, color.red, color.green,
        color.blue, 0.5);
      if (PIANO_ROLL->drum_mode)
        {
          z_cairo_diamond (cr, 0, 0, width, height);
        }
      else
        {
          z_cairo_rounded_rectangle (
            cr, 0, 0, width, height, 1.0, 4.0);
        }
      cairo_fill(cr);
    }

  char * str =
    g_strdup_printf (
      "%s<sup>%d</sup>",
      chord_descriptor_note_to_string (
        self->midi_note->val % 12),
      self->midi_note->val / 12 - 2);
  if (DEBUGGING &&
      midi_note_is_transient (self->midi_note))
    {
      char * tmp =
        g_strdup_printf (
          "%s [t]", str);
      g_free (str);
      str = tmp;
    }

  GdkRGBA c2;
  ui_get_contrast_text_color (
    &color, &c2);
  gdk_cairo_set_source_rgba (cr, &c2);
  if (DEBUGGING || !PIANO_ROLL->drum_mode)
    z_cairo_draw_text (cr, str);
  g_free (str);

 return FALSE;
}

static gboolean
on_motion (GtkWidget *      widget,
           GdkEventMotion * event,
           MidiNoteWidget * self)
{
  if (event->type == GDK_MOTION_NOTIFY)
    {
      self->resize_l =
        midi_note_widget_is_resize_l (
          self, event->x);
      self->resize_r =
        midi_note_widget_is_resize_r (
          self, event->x);
    }

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
      bot_bar_change_status (
        "MIDI Note - Click and drag to move around ("
        "hold Shift to disable snapping)");
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }

  return FALSE;
}

/**
 * Returns if the current position is for resizing
 * L.
 */
int
midi_note_widget_is_resize_l (
  MidiNoteWidget * self,
  int              x)
{
  if (x < RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * L.
 */
int
midi_note_widget_is_resize_r (
  MidiNoteWidget * self,
  int              x)
{
  GtkAllocation allocation;
  gtk_widget_get_allocation (
    GTK_WIDGET (self),
    &allocation);

  if (x > allocation.width - RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

void
midi_note_widget_update_tooltip (
  MidiNoteWidget * self,
  int              show)
{
  const char * note =
    chord_descriptor_note_to_string (
      self->midi_note->val % 12);

  /* set tooltip text */
  char * tooltip =
    g_strdup_printf (
      "[%s%d] %d",
      note,
      self->midi_note->val / 12 - 2,
      self->midi_note->vel->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);

  /* set tooltip window */
  /*if (show)*/
    /*{*/
      /*tooltip =*/
        /*g_strdup_printf (*/
          /*"%s%d",*/
          /*note,*/
          /*self->midi_note->val / 12 - 2);*/
      /*gtk_label_set_text (self->tooltip_label,*/
                          /*tooltip);*/
      /*gtk_window_present (self->tooltip_win);*/

      /*g_free (tooltip);*/
    /*}*/
  /*else*/
    /*gtk_widget_hide (*/
      /*GTK_WIDGET (self->tooltip_win));*/
}

static void
on_destroy (
  GtkWidget * widget,
  MidiNoteWidget * self)
{
  g_message ("on destroy %p", widget);
}

DEFINE_ARRANGER_OBJECT_WIDGET_SELECT (
  MidiNote, midi_note,
  midi_arranger_selections, MA_SELECTIONS);

/**
 * Destroys the widget completely.
 */
void
midi_note_widget_destroy (MidiNoteWidget *self)
{
  gtk_widget_set_sensitive (
    GTK_WIDGET (self), 0);
  gtk_container_remove (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->drawing_area));

  gtk_widget_destroy (
    GTK_WIDGET (self));

  g_object_unref (self);
}

MidiNoteWidget *
midi_note_widget_new (MidiNote * midi_note)
{
  MidiNoteWidget * self =
    g_object_new (MIDI_NOTE_WIDGET_TYPE,
                  "visible", 1,
                  NULL);

  self->midi_note = midi_note;

  /* set tooltip text */
  char * tooltip =
    g_strdup_printf (
      "[%s%d] %d",
      chord_descriptor_note_to_string (
        self->midi_note->val % 12),
      self->midi_note->val / 12 - 2,
      self->midi_note->vel->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);

  return self;
}

static void
midi_note_widget_class_init (
  MidiNoteWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "midi-note");
}

static void
midi_note_widget_init (MidiNoteWidget * self)
{
  self->drawing_area =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_widget_set_hexpand (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->drawing_area));

  gtk_widget_set_events (
    GTK_WIDGET (self->drawing_area),
    GDK_ENTER_NOTIFY_MASK |
    GDK_LEAVE_NOTIFY_MASK |
    GDK_POINTER_MOTION_MASK
    );

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "destroy",
    G_CALLBACK (on_destroy), self);
  g_signal_connect (
    G_OBJECT (self->drawing_area), "draw",
    G_CALLBACK (midi_note_draw_cb), self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drawing_area),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);

  /* set tooltip window */
  /*self->tooltip_win =*/
    /*GTK_WINDOW (gtk_window_new (GTK_WINDOW_POPUP));*/
  /*gtk_window_set_type_hint (*/
    /*self->tooltip_win,*/
    /*GDK_WINDOW_TYPE_HINT_TOOLTIP);*/
  /*self->tooltip_label =*/
    /*GTK_LABEL (gtk_label_new ("label"));*/
  /*gtk_widget_set_visible (*/
    /*GTK_WIDGET (self->tooltip_label), 1);*/
  /*gtk_container_add (*/
    /*GTK_CONTAINER (self->tooltip_win),*/
    /*GTK_WIDGET (self->tooltip_label));*/
  /*gtk_window_set_position (*/
    /*self->tooltip_win, GTK_WIN_POS_MOUSE);*/

  g_object_ref (self);
}
