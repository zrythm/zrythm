/*
 * gui/widgets/midi_ruler.c - Ruler
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/engine.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/midi_ruler.h"
#include "project.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MidiRulerWidget,
               midi_ruler_widget,
               RULER_WIDGET_TYPE)

static gboolean
midi_ruler_draw_cb (GtkWidget * widget,
         cairo_t *cr,
         MidiRulerWidget * self)
{
  /* engine is run only set after everything is set up
   * so this is a good way to decide if we should draw
   * or not */
  if (!AUDIO_ENGINE->run || !PIANO_ROLL->track)
    return FALSE;

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);

  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  /*width = gtk_widget_get_allocated_width (widget);*/
  guint height =
    gtk_widget_get_allocated_height (widget);

  Track * track = PIANO_ROLL->track;
  InstrumentTrack * it = (InstrumentTrack *) track;
  cairo_set_source_rgba (cr,
                         track->color.red,
                         track->color.green,
                         track->color.blue,
                         0.8);

  int px_start, px_end;
  for (int i = 0; i < it->num_regions; i++)
    {
      Region * region = (Region *) it->regions[i];

      px_start = ui_pos_to_px (&region->start_pos, 1);
      px_end = ui_pos_to_px (&region->end_pos, 1);

      /* TODO only conditionally draw if it is within cairo
       * clip */
      cairo_rectangle (cr,
                       px_start, 0,
                       px_end - px_start, height);
      cairo_fill (cr);
    }

 return FALSE;
}

static void
midi_ruler_widget_class_init (
  MidiRulerWidgetClass * klass)
{
}

static void
midi_ruler_widget_init (
  MidiRulerWidget * self)
{
  RULER_WIDGET_GET_PRIVATE (self);

  g_signal_connect (G_OBJECT (rw_prv->bg), "draw",
                    G_CALLBACK (midi_ruler_draw_cb), self);
}

