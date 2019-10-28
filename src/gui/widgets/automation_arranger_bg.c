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

#include "zrythm.h"
#include "project.h"
#include "settings/settings.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_arranger_bg.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/tracklist.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (AutomationArrangerBgWidget,
               automation_arranger_bg_widget,
               ARRANGER_BG_WIDGET_TYPE)

static gboolean
automation_arranger_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  gpointer data)
{
  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr, &rect);

  int height =
    gtk_widget_get_allocated_height (widget);

  /* draw automation related stuff */
  Region * r =
    clip_editor_get_region_for_widgets (
      CLIP_EDITOR);
  g_return_val_if_fail (r && r->at, FALSE);
  ArrangerObject * r_obj =
    (ArrangerObject *) r;

  Track * track =
    arranger_object_get_track (r_obj);

  float normalized_val =
    automation_track_get_normalized_val_at_pos (
      r->at, PLAYHEAD);
  if (normalized_val < 0.f)
    normalized_val =
      automatable_real_val_to_normalized (
        r->at->automatable,
        automatable_get_val (
          r->at->automatable));

  int y_px =
    height -
      (int)
      ((float) height * normalized_val);

  /* show line at current val */
  cairo_set_source_rgba (
    cr,
    track->color.red,
    track->color.green,
    track->color.blue,
    0.3);
  cairo_set_line_width (cr, 1);
  cairo_move_to (cr, rect.x, y_px);
  cairo_line_to (cr, rect.x + rect.width, y_px);
  cairo_stroke (cr);

  /* show shade under the line */
  /*cairo_set_source_rgba (*/
    /*cr,*/
    /*track->color.red,*/
    /*track->color.green,*/
    /*track->color.blue,*/
    /*0.06);*/
  /*int allocated_h =*/
    /*gtk_widget_get_allocated_height (*/
      /*GTK_WIDGET (al->widget));*/
  /*cairo_rectangle (*/
    /*cr,*/
    /*rect.x, wy + y_px,*/
    /*rect.width, allocated_h - y_px);*/
  /*cairo_fill (cr);*/

  return FALSE;
}

AutomationArrangerBgWidget *
automation_arranger_bg_widget_new (
  RulerWidget *    ruler,
  ArrangerWidget * arranger)
{
  AutomationArrangerBgWidget * self =
    g_object_new (AUTOMATION_ARRANGER_BG_WIDGET_TYPE,
                  NULL);

  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->ruler = ruler;
  ab_prv->arranger = arranger;

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (automation_arranger_draw_cb), NULL);

  return self;
}

static void
automation_arranger_bg_widget_class_init (
  AutomationArrangerBgWidgetClass * _klass)
{
}

static void
automation_arranger_bg_widget_init (
  AutomationArrangerBgWidget *self )
{
}
