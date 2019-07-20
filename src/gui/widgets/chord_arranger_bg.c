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
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_arranger_bg.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/tracklist.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ChordArrangerBgWidget,
               chord_arranger_bg_widget,
               ARRANGER_BG_WIDGET_TYPE)

static void
draw_borders (
  ChordArrangerBgWidget * self,
  cairo_t *              cr,
  int                    x_from,
  int                    x_to,
  double                 y_offset)
{
  cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
  cairo_set_line_width (cr, 0.5);
  cairo_move_to (cr, x_from, y_offset);
  cairo_line_to (cr, x_to, y_offset);
  cairo_stroke (cr);
}

static gboolean
chord_arranger_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  gpointer data)
{
  ChordArrangerBgWidget * self =
    Z_CHORD_ARRANGER_BG_WIDGET (widget);

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);

  /* px per key adjusted for border width */
  double adj_px_per_key =
    MW_CHORD_EDITOR_SPACE->px_per_key + 1;

  /*handle horizontal drawing*/
  double y_offset;
  for (int i = 0; i <= CHORD_EDITOR->num_chords; i++)
    {
      y_offset =
        adj_px_per_key * i;
      if (y_offset > rect.y &&
          y_offset < (rect.y + rect.height))
        draw_borders (
          self,
          cr,
          rect.x,
          rect.x + rect.width,
          y_offset);
      if (i == MW_CHORD_ARRANGER->hovered_index)
        {
          double height;
          if (MW_CHORD_ARRANGER->hovered_index ==
                CHORD_EDITOR->num_chords)
            height =
              rect.height - (y_offset + 1);
          else
            height = adj_px_per_key;
          cairo_set_source_rgba (
            cr, 1, 1, 1, 0.06);
              cairo_rectangle (
                cr,
                rect.x,
                /* + 1 since the border is bottom */
                y_offset + 1,
                rect.width,
                height);
          cairo_fill (cr);
        }
    }

  return FALSE;
}

ChordArrangerBgWidget *
chord_arranger_bg_widget_new (
  RulerWidget *    ruler,
  ArrangerWidget * arranger)
{
  ChordArrangerBgWidget * self =
    g_object_new (CHORD_ARRANGER_BG_WIDGET_TYPE,
                  NULL);

  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->ruler = ruler;
  ab_prv->arranger = arranger;

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (chord_arranger_draw_cb), NULL);

  return self;
}

static void
chord_arranger_bg_widget_class_init (
  ChordArrangerBgWidgetClass * _klass)
{
}

static void
chord_arranger_bg_widget_init (
  ChordArrangerBgWidget *self )
{
}
