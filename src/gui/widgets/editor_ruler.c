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

#include "audio/engine.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_marker.h"
#include "project.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (EditorRulerWidget,
               editor_ruler_widget,
               RULER_WIDGET_TYPE)

static gboolean
editor_ruler_draw_cb (GtkWidget * widget,
         cairo_t *cr,
         EditorRulerWidget * self)
{
  /* engine is run only set after everything is set up
   * so this is a good way to decide if we should draw
   * or not */
  if (!g_atomic_int_get (&AUDIO_ENGINE->run) ||
      !CLIP_EDITOR->region)
    return FALSE;

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);

  /*GtkStyleContext *context;*/
  /*context =*/
    /*gtk_widget_get_style_context (*/
      /*GTK_WIDGET (self));*/

  /*width = gtk_widget_get_allocated_width (widget);*/
  int height =
    gtk_widget_get_allocated_height (widget);

  /* get a visible region */
  Region * region = CLIP_EDITOR->region;
  region =
    region_get_main_region (region);

  Track * track = region_get_track (region);

  int px_start, px_end;

  /* draw the main region */
  cairo_set_source_rgba (
    cr, 1, track->color.green + 0.2,
    track->color.blue + 0.2, 1.0);
  if (region_should_be_visible (&region))
    {
      px_start =
        ui_pos_to_px_editor (
          &region->start_pos, 1);
      px_end =
        ui_pos_to_px_editor (
          &region->end_pos, 1);
      cairo_rectangle (
        cr, px_start, 0,
        px_end - px_start, height / 4.0);
      cairo_fill (cr);
    }

  /* draw its transient if copy-moving */
  region =
    region_get_main_trans_region (region);
  if (region_should_be_visible (&region))
    {
      px_start =
        ui_pos_to_px_editor (
          &region->start_pos, 1);
      px_end =
        ui_pos_to_px_editor (
          &region->end_pos, 1);
      cairo_rectangle (
        cr, px_start, 0,
        px_end - px_start, height / 4.0);
      cairo_fill (cr);
    }

  /* draw the other regions */
  cairo_set_source_rgba (cr,
                         track->color.red,
                         track->color.green,
                         track->color.blue,
                         0.5);
  Region * other_region;
  TrackLane * lane;
  for (int j = 0; j < track->num_lanes; j++)
    {
      lane = track->lanes[j];

      for (int i = 0; i < lane->num_regions; i++)
        {
          other_region = lane->regions[i];
          if (!g_strcmp0 (region->name,
                         other_region->name))
            continue;

          px_start =
            ui_pos_to_px_editor (
              &other_region->start_pos, 1);
          px_end =
            ui_pos_to_px_editor (
              &other_region->end_pos, 1);
          cairo_rectangle (
            cr, px_start, 0,
            px_end - px_start, height / 4.0);
          cairo_fill (cr);
        }
    }

 return FALSE;
}

/**
 * Called when allocating the children of the
 * RulerWidget to allocate the RulerMarkerWidget.
 */
void
editor_ruler_widget_set_ruler_marker_position (
  EditorRulerWidget * self,
  RulerMarkerWidget *    rm,
  GtkAllocation *       allocation)
{
  Position tmp;
  switch (rm->type)
    {
    case RULER_MARKER_TYPE_LOOP_START:
      if (CLIP_EDITOR->region)
        {
          long start_ticks =
            position_to_ticks (
              &CLIP_EDITOR->region->start_pos);
          long loop_start_ticks =
            position_to_ticks (
              &CLIP_EDITOR->region->
                loop_start_pos) +
            start_ticks;
          position_from_ticks (
            &tmp, loop_start_ticks);
          allocation->x =
            ui_pos_to_px_editor (
              &tmp,
              1);
        }
      else
        allocation->x = 0;
      allocation->y = 0;
      allocation->width = RULER_MARKER_SIZE;
      allocation->height = RULER_MARKER_SIZE;
      break;
    case RULER_MARKER_TYPE_LOOP_END:
      if (CLIP_EDITOR->region)
        {
          long start_ticks =
            position_to_ticks (
              &CLIP_EDITOR->region->start_pos);
          long loop_end_ticks =
            position_to_ticks (
              &CLIP_EDITOR->region->loop_end_pos) +
            start_ticks;
          position_from_ticks (
            &tmp, loop_end_ticks);
          allocation->x =
            ui_pos_to_px_editor (
              &tmp,
              1) - RULER_MARKER_SIZE;
        }
      else
        allocation->x = 0;
      allocation->y = 0;
      allocation->width = RULER_MARKER_SIZE;
      allocation->height = RULER_MARKER_SIZE;
      break;
    case RULER_MARKER_TYPE_CLIP_START:
      if (CLIP_EDITOR->region)
        {
          long start_ticks =
            position_to_ticks (
              &CLIP_EDITOR->region->start_pos);
          long clip_start_ticks =
            position_to_ticks (
              &CLIP_EDITOR->region->
                clip_start_pos) +
            start_ticks;
          position_from_ticks (
            &tmp, clip_start_ticks);
          allocation->x =
            ui_pos_to_px_editor (
              &tmp,
              1);
        }
      else
        allocation->x = 0;
      if (MAIN_WINDOW && EDITOR_RULER)
        {
      allocation->y =
        ((gtk_widget_get_allocated_height (
          GTK_WIDGET (EDITOR_RULER)) -
            RULER_MARKER_SIZE) - CUE_MARKER_HEIGHT) -
            1;
        }
      else
        allocation->y = RULER_MARKER_SIZE *2;
      allocation->width = CUE_MARKER_WIDTH;
      allocation->height = CUE_MARKER_HEIGHT;
      break;
    case RULER_MARKER_TYPE_PLAYHEAD:
      allocation->x =
        ui_pos_to_px_editor (
          &TRANSPORT->playhead_pos,
          1) - (PLAYHEAD_TRIANGLE_WIDTH / 2);
      allocation->y =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self)) -
          PLAYHEAD_TRIANGLE_HEIGHT;
      allocation->width = PLAYHEAD_TRIANGLE_WIDTH;
      allocation->height =
        PLAYHEAD_TRIANGLE_HEIGHT;
      break;
    default:
      break;
    }

}

void
editor_ruler_widget_refresh (
  EditorRulerWidget * self)
{
  RULER_WIDGET_GET_PRIVATE (self);

  /*adjust for zoom level*/
  rw_prv->px_per_tick =
    DEFAULT_PX_PER_TICK * rw_prv->zoom_level;
  rw_prv->px_per_sixteenth =
    rw_prv->px_per_tick * TICKS_PER_SIXTEENTH_NOTE;
  rw_prv->px_per_beat =
    rw_prv->px_per_tick * TRANSPORT->ticks_per_beat;
  rw_prv->px_per_bar =
    rw_prv->px_per_beat * TRANSPORT->beats_per_bar;

  Position pos;
  position_set_to_bar (
    &pos, TRANSPORT->total_bars + 1);
  rw_prv->total_px =
    rw_prv->px_per_tick *
    (double) position_to_ticks (&pos);

  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (EDITOR_RULER),
    (int) rw_prv->total_px,
    -1);

  gtk_widget_queue_allocate (
    GTK_WIDGET (EDITOR_RULER));
  EVENTS_PUSH (ET_RULER_SIZE_CHANGED,
               EDITOR_RULER);
}

static void
editor_ruler_widget_class_init (
  EditorRulerWidgetClass * klass)
{
}

static void
editor_ruler_widget_init (
  EditorRulerWidget * self)
{
  RULER_WIDGET_GET_PRIVATE (self);

  g_signal_connect (
    G_OBJECT (rw_prv->bg), "draw",
    G_CALLBACK (editor_ruler_draw_cb), self);

  /* add all the markers */
  RulerWidget * ruler =
    Z_RULER_WIDGET (self);
  self->loop_start =
    ruler_marker_widget_new (
      ruler, RULER_MARKER_TYPE_LOOP_START);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (self->loop_start));
  self->loop_end =
    ruler_marker_widget_new (
      ruler, RULER_MARKER_TYPE_LOOP_END);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (self->loop_end));
  self->clip_start =
    ruler_marker_widget_new (
      ruler, RULER_MARKER_TYPE_CLIP_START);
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (self->clip_start));
}
