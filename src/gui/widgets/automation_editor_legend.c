/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/automation_point.h"
#include "audio/tracklist.h"
#include "gui/backend/clip_editor.h"
#include "gui/widgets/automation_editor_legend.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/clip_editor_inner.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  AutomationEditorLegendWidget,
  automation_editor_legend_widget,
  GTK_TYPE_DRAWING_AREA)

/* can also try Sans SemiBold */
#define AUTOMATION_EDITOR_LEGEND_FONT "Sans 8"

static gboolean
automation_editor_legend_draw (
  GtkWidget * widget,
  cairo_t *   cr,
  AutomationEditorLegendWidget * self)
{
  GtkStyleContext *context =
    gtk_widget_get_style_context (widget);
  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);
  gtk_render_background (
    context, cr, 0, 0, width, height);

#if 0

  /* get the automatable */
  ZRegion * r = clip_editor_get_region (CLIP_EDITOR);
  g_return_if_fail (r, false);

  AutomationTrack * at =
    region_get_automation_track (r);
  Port * port = automation_track_get_port (at);

  /*char str[400];*/

  /* ---- draw label ---- */

  char fontize_str[120];
  sprintf (
    fontize_str, "<span size=\"%d\">",
    12 * 1000 - 4000);
  strcat (fontize_str, "%s</span>");

  g_debug ("drawing legend");

  cairo_set_source_rgba (cr, 1, 0.5, 0.5, 1);
  /*cairo_rectangle (cr, 0, 0, 10, 10);*/
  /*cairo_fill (cr);*/

  z_cairo_draw_text (
    cr, GTK_WIDGET (self), self->layout, "test");

#endif

  return false;
}

static gboolean
on_motion (
  GtkWidget *widget,
  GdkEventMotion  *event,
  AutomationEditorLegendWidget * self)
{
  /* TODO */

  return FALSE;
}

static void
on_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  AutomationEditorLegendWidget * self)
{
  /* TODO toggle between normalized/real values */
}

void
automation_editor_legend_widget_refresh (
  AutomationEditorLegendWidget * self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
automation_editor_legend_widget_setup (
  AutomationEditorLegendWidget * self)
{
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (automation_editor_legend_draw), self);
}

static void
automation_editor_legend_widget_init (
  AutomationEditorLegendWidget * self)
{
  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));

  PangoFontDescription * desc;
  self->layout =
    gtk_widget_create_pango_layout (
      GTK_WIDGET (self), NULL);
  desc =
    pango_font_description_from_string (
      AUTOMATION_EDITOR_LEGEND_FONT);
  pango_layout_set_font_description (
    self->layout, desc);
  pango_font_description_free (desc);

  /* make it able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self), "motion-notify-event",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT(self->multipress), "pressed",
    G_CALLBACK (on_pressed),  self);
}

static void
automation_editor_legend_widget_class_init (
  AutomationEditorLegendWidgetClass * _klass)
{
}
