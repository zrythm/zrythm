/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file */

#include "audio/channel.h"
#include "audio/channel_track.h"
#include "audio/chord_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  VelocityWidget,
  velocity_widget,
  ARRANGER_OBJECT_WIDGET_TYPE)

/**
 * Space on the edge to show resize cursor
 */
#define RESIZE_CURSOR_SPACE 9

static gboolean
draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  VelocityWidget * self)
{
  GtkStyleContext *context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  MidiNote * mn = self->velocity->midi_note;
  Region * region = mn->region;
  Position global_start_pos;
  midi_note_get_global_start_pos (
    mn, &global_start_pos);
  ChordObject * co =
    chord_track_get_chord_at_pos (
      P_CHORD_TRACK, &global_start_pos);
  ScaleObject * so =
    chord_track_get_scale_at_pos (
      P_CHORD_TRACK, &global_start_pos);
  int in_scale =
    so && musical_scale_is_key_in_scale (
      so->scale, mn->val % 12);
  int in_chord =
    co && chord_descriptor_is_key_in_chord (
      chord_object_get_chord_descriptor (co),
      mn->val % 12);

  GdkRGBA color;
  if (PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_BOTH &&
      in_scale && in_chord)
    {
      color = UI_COLORS->highlight_both;
    }
  else if ((PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_SCALE ||
      PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_BOTH) && in_scale)
    {
      color = UI_COLORS->highlight_in_scale;
    }
  else if ((PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_CHORD ||
      PIANO_ROLL->highlighting ==
        PR_HIGHLIGHT_BOTH) && in_chord)
    {
      color = UI_COLORS->highlight_in_chord;
    }
  else
    {
      color =
        region->lane->track->color;
    }

  /* draw velocities of main region */
  if (region == CLIP_EDITOR->region)
    {
      cairo_set_source_rgba (
        cr,
        color.red,
        color.green,
        color.blue,
        velocity_is_transient (
          self->velocity) ? 0.7 : 1);
      if (velocity_is_selected (self->velocity))
        {
          cairo_set_source_rgba (
            cr, color.red + 0.4,
            color.green + 0.2,
            color.blue + 0.2, 1);
        }
      cairo_rectangle(cr, 0, 0, width, height);
      cairo_fill(cr);
    }
  /* draw other notes */
  else
    {
      cairo_set_source_rgba (
        cr, color.red, color.green,
        color.blue, 0.5);
      cairo_rectangle(cr, 0, 0, width, height);
      cairo_fill(cr);
    }

  if (DEBUGGING &&
      velocity_is_transient (self->velocity))
    {
      GdkRGBA c2;
      ArrangerObject * r_obj =
        (ArrangerObject *)
        self->velocity->midi_note->region;
      Track * track =
        arranger_object_get_track (r_obj);
      ui_get_contrast_color (
        &track->color, &c2);
      gdk_cairo_set_source_rgba (cr, &c2);
      PangoLayout * layout =
        z_cairo_create_default_pango_layout (
          widget);
      z_cairo_draw_text (
        cr, widget,
        layout, "[t]");
      g_object_unref (layout);
    }

 return FALSE;
}

/**
 * Creates a velocity.
 */
VelocityWidget *
velocity_widget_new (Velocity * velocity)
{
  VelocityWidget * self =
    g_object_new (VELOCITY_WIDGET_TYPE,
                  NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) velocity);

  self->velocity = velocity;

  return self;
}

static void
velocity_widget_class_init (
  VelocityWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "velocity");
}

static void
velocity_widget_init (VelocityWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (draw_cb), self);
}
