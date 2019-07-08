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
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/tracklist.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiArrangerBgWidget,
               midi_arranger_bg_widget,
               ARRANGER_BG_WIDGET_TYPE)

static void
draw_borders (MidiArrangerBgWidget * self,
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
midi_arranger_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  gpointer data)
{
  MidiArrangerBgWidget * self =
    Z_MIDI_ARRANGER_BG_WIDGET (widget);

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);


  /* px per key adjusted for border width */
  double adj_px_per_key =
    MW_PIANO_ROLL->px_per_key + 1;
  /*double adj_total_key_px =*/
    /*MW_PIANO_ROLL->total_key_px + 126;*/

  /*handle horizontal drawing*/
  double y_offset;
  for (int i = 0; i < 128; i++)
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
      if ((PIANO_ROLL->drum_mode &&
          PIANO_ROLL->drum_descriptors[i].value ==
            MIDI_ARRANGER->hovered_note) ||
          (!PIANO_ROLL->drum_mode &&
           PIANO_ROLL->piano_descriptors[i].value ==
             MIDI_ARRANGER->hovered_note))
        {
          cairo_set_source_rgba (
            cr, 1, 1, 1, 0.06);
              cairo_rectangle (
                cr,
                rect.x,
                /* + 1 since the border is bottom */
                y_offset + 1,
                rect.width,
                adj_px_per_key);
          cairo_fill (cr);
        }
    }

  return 0;
}

MidiArrangerBgWidget *
midi_arranger_bg_widget_new (RulerWidget *    ruler,
                             ArrangerWidget * arranger)
{
  MidiArrangerBgWidget * self =
    g_object_new (MIDI_ARRANGER_BG_WIDGET_TYPE,
                  NULL);

  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->ruler = ruler;
  ab_prv->arranger = arranger;

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (midi_arranger_draw_cb), NULL);

  return self;
}

static void
midi_arranger_bg_widget_class_init (
  MidiArrangerBgWidgetClass * _klass)
{
}

static void
midi_arranger_bg_widget_init (
  MidiArrangerBgWidget *self )
{
}
