/*
 * gui/widgets/midi_arranger_bg.c - MidiArranger background
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "zrythm.h"
#include "project.h"
#include "settings/settings.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/tracklist.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiArrangerBgWidget,
               midi_arranger_bg_widget,
               ARRANGER_BG_WIDGET_TYPE)

static void
draw_borders (MidiArrangerBgWidget * self,
              cairo_t * cr,
              int y_offset)
{
  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  RULER_WIDGET_GET_PRIVATE (ab_prv->ruler);
  cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
  cairo_set_line_width (cr, 0.5);
  cairo_move_to (cr, 0, y_offset);
  cairo_line_to (cr, rw_prv->total_px, y_offset);
  cairo_stroke (cr);
}

static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  MidiArrangerBgWidget * self =
    Z_MIDI_ARRANGER_BG_WIDGET (widget);

  /*handle horizontal drawing*/
  for (int i = 0; i < 128; i++)
    {
      draw_borders (
        self,
        cr,
        PIANO_ROLL->piano_roll_labels->px_per_note * i);
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

  // set the size FIXME uncomment
  /*int ww, hh;*/
  /*PianoRollLabelsWidget * piano_roll_labels =*/
    /*PIANO_ROLL->piano_roll_labels;*/
  /*gtk_widget_get_size_request (*/
    /*GTK_WIDGET (piano_roll_labels),*/
    /*&ww,*/
    /*&hh);*/
  /*gtk_widget_set_size_request (*/
    /*GTK_WIDGET (self),*/
    /*MW_RULER->total_px,*/
    /*hh);*/


  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);

  return self;
}

static void
midi_arranger_bg_widget_class_init (MidiArrangerBgWidgetClass * _klass)
{
}

static void
midi_arranger_bg_widget_init (MidiArrangerBgWidget *self )
{
}
