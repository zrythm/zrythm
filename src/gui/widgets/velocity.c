/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file */

#include "audio/channel.h"
#include "audio/channel_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/ui.h"

G_DEFINE_TYPE (VelocityWidget,
               velocity_widget,
               GTK_TYPE_BOX)

static gboolean
draw_cb (GtkDrawingArea * da,
         cairo_t *cr,
         gpointer data)
{
  VelocityWidget * self = Z_VELOCITY_WIDGET (data);
  guint width, height;
  GtkStyleContext *context;

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

  Region * region =
    self->velocity->midi_note->region;
  GdkRGBA * color = &region->lane->track->color;

  cairo_set_source_rgba (cr,
                         color->red - 0.2,
                         color->green - 0.2,
                         color->blue - 0.2,
                         0.7);

  /* TODO draw audio notes */
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);

 return FALSE;
}

static int
on_motion (GtkWidget *      widget,
           GdkEventMotion * event,
           VelocityWidget * self)
{
  GtkAllocation allocation;
  gtk_widget_get_allocation (widget,
                             &allocation);
  ARRANGER_WIDGET_GET_PRIVATE (
    MIDI_MODIFIER_ARRANGER);

  if (event->type == GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT,
        0);
    }
  if (event->type == GDK_MOTION_NOTIFY)
    {
      if (event->y < RESIZE_CURSOR_SPACE)
        {
          self->cursor_state =
            UI_CURSOR_STATE_RESIZE_UP;
            ui_set_cursor_from_name (widget, "n-resize");
        }
      else
        {
          self->cursor_state =
            UI_CURSOR_STATE_DEFAULT;
          if (ar_prv->action !=
                UI_OVERLAY_ACTION_RESIZING_UP)
            {
              ui_set_cursor_from_name (widget, "default");
            }
        }
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (ar_prv->action !=
            UI_OVERLAY_ACTION_RESIZING_UP)
        {
          ui_set_cursor_from_name (widget, "default");
          gtk_widget_unset_state_flags (
            GTK_WIDGET (self),
            GTK_STATE_FLAG_PRELIGHT);
        }
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
  if (show)
    {
      gtk_label_set_text (self->tooltip_label,
                          tooltip);
      gtk_window_present (self->tooltip_win);
    }
  else
    gtk_widget_hide (
      GTK_WIDGET (self->tooltip_win));

  g_free (tooltip);
}

void
velocity_widget_select (
  VelocityWidget * self,
  int              select)
{
  midi_arranger_selections_add_note (
    MA_SELECTIONS,
    self->velocity->midi_note);
  if (select)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_SELECTED,
        0);
    }
  else
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_SELECTED);
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
  gtk_widget_queue_draw (
    GTK_WIDGET (self->velocity->midi_note->widget));
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

  self->velocity = velocity;

  /* set tooltip */
  char * tooltip =
    g_strdup_printf ("%d", self->velocity->vel);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), tooltip);
  g_free (tooltip);

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

  gtk_widget_add_events (
    GTK_WIDGET (self->drawing_area),
    GDK_ALL_EVENTS_MASK);

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
