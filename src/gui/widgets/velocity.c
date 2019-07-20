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

G_DEFINE_TYPE (VelocityWidget,
               velocity_widget,
               GTK_TYPE_BOX)

/**
 * Space on the edge to show resize cursor
 */
#define RESIZE_CURSOR_SPACE 9

static gboolean
draw_cb (GtkDrawingArea * da,
         cairo_t *cr,
         gpointer data)
{
  VelocityWidget * self = Z_VELOCITY_WIDGET (data);
  guint width, height;
  GtkStyleContext *context;

  /*g_message ("draw %d (transient? %d)",*/
             /*self->velocity->vel,*/
             /*velocity_is_transient (self->velocity));*/

  context =
    gtk_widget_get_style_context (GTK_WIDGET (self));

  width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));
  height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  gtk_render_background (
    context, cr, 0, 0, width, height);

  MidiNote * mn = self->velocity->midi_note;
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
      chord_object_get_chord_descriptor (co),
      mn->val % 12);

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
      GdkRGBA color, c2;
      gdk_rgba_parse (
        &color,
        gdk_rgba_to_string (
          &region_get_track (
            self->velocity->midi_note->region)->
              color));
      ui_get_contrast_text_color (
        &color, &c2);
      gdk_cairo_set_source_rgba (cr, &c2);
      z_cairo_draw_text (cr, "[t]");
    }

 return FALSE;
}

/**
 * Returns if the current position is for resizing.
 */
int
velocity_widget_is_resize (
  VelocityWidget * self,
  int              y)
{
  if (y < RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

static int
on_motion (GtkWidget *      widget,
           GdkEventMotion * event,
           VelocityWidget * self)
{
  if (event->type == GDK_MOTION_NOTIFY)
    {
      self->resize =
        velocity_widget_is_resize (
          self, event->y);
    }

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
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
 * Updates the tooltips and shows the tooltip
 * window (when dragging) or not.
 */
void
velocity_widget_update_tooltip (
  VelocityWidget * self,
  int              show)
{
  /* set tooltip text */
  char * tooltip =
    g_strdup_printf ("%d", self->velocity->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);

  /* set tooltip window */
  /*if (show)*/
    /*{*/
      /*gtk_label_set_text (self->tooltip_label,*/
                          /*tooltip);*/
      /*gtk_window_present (self->tooltip_win);*/
    /*}*/
  /*else*/
    /*gtk_widget_hide (*/
      /*GTK_WIDGET (self->tooltip_win));*/

  g_free (tooltip);
}

DEFINE_ARRANGER_OBJECT_WIDGET_SELECT (
  Velocity, velocity,
  midi_arranger_selections, MA_SELECTIONS);

/**
 * Creates a velocity.
 */
VelocityWidget *
velocity_widget_new (Velocity * velocity)
{
  VelocityWidget * self =
    g_object_new (VELOCITY_WIDGET_TYPE,
                  NULL);

  self->velocity = velocity;

  /* set tooltip */
  char * tooltip =
    g_strdup_printf ("%d", self->velocity->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);

  return self;
}

gboolean
on_btn_press (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
  g_message ("press");

  return FALSE;
}

gboolean
on_btn_release (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
  g_message ("release");

  return FALSE;
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
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  self->drawing_area = GTK_DRAWING_AREA (
    gtk_drawing_area_new ());
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->drawing_area));
  gtk_widget_set_visible (
    GTK_WIDGET (self->drawing_area), 1);
  gtk_widget_set_hexpand (
    GTK_WIDGET (self->drawing_area), 1);

  /* GDK_ALL_EVENTS_MASK is needed, otherwise the
   * grab gets broken */
  gtk_widget_set_events (
    GTK_WIDGET (self->drawing_area),
    GDK_ENTER_NOTIFY_MASK |
    GDK_LEAVE_NOTIFY_MASK |
    GDK_POINTER_MOTION_MASK
    );

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self->drawing_area), "draw",
    G_CALLBACK (draw_cb), self);
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

  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);

  /* set tooltip window */
  self->tooltip_win =
    GTK_WINDOW (gtk_window_new (GTK_WINDOW_POPUP));
  gtk_window_set_type_hint (
    self->tooltip_win,
    GDK_WINDOW_TYPE_HINT_TOOLTIP);
  self->tooltip_label =
    GTK_LABEL (gtk_label_new ("label"));
  gtk_widget_set_visible (
    GTK_WIDGET (self->tooltip_label), 1);
  gtk_container_add (
    GTK_CONTAINER (self->tooltip_win),
    GTK_WIDGET (self->tooltip_label));
  gtk_window_set_position (
    self->tooltip_win, GTK_WIN_POS_MOUSE);
}
