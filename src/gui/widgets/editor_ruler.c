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
#include "utils/flags.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  EditorRulerWidget,
  editor_ruler_widget,
  RULER_WIDGET_TYPE)

#define ACTION_IS(x) \
  (rw_prv->action == UI_OVERLAY_ACTION_##x)
#define TARGET_IS(x) \
  (rw_prv->target == RW_TARGET_##x)

static gboolean
editor_ruler_draw_cb (
  GtkWidget * widget,
  cairo_t *cr,
  EditorRulerWidget * self)
{
  /* engine is run only set after everything is
   * set up so this is a good way to decide if we
   * should draw or not */
  if (!g_atomic_int_get (&AUDIO_ENGINE->run) ||
      !CLIP_EDITOR->region)
    return FALSE;

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (
    cr, &rect);

  int height =
    gtk_widget_get_allocated_height (widget);

  /* get a visible region */
  Region * region = CLIP_EDITOR->region;
  region =
    region_get_main (region);
  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  Track * track =
    arranger_object_get_track (region_obj);

  int px_start, px_end;

  /* draw the main region */
  cairo_set_source_rgba (
    cr, 1, track->color.green + 0.2,
    track->color.blue + 0.2, 1.0);
  if (arranger_object_should_be_visible (
        region_obj))
    {
      px_start =
        ui_pos_to_px_editor (
          &region_obj->pos, 1);
      px_end =
        ui_pos_to_px_editor (
          &region_obj->end_pos, 1);
      cairo_rectangle (
        cr, px_start, 0,
        px_end - px_start, height / 4.0);
      cairo_fill (cr);
    }

  /* draw its transient if copy-moving */
  region =
    region_get_main_trans (region);
  region_obj =
    (ArrangerObject *) region;
  if (arranger_object_should_be_visible (
        region_obj))
    {
      px_start =
        ui_pos_to_px_editor (
          &region_obj->pos, 1);
      px_end =
        ui_pos_to_px_editor (
          &region_obj->end_pos, 1);
      cairo_rectangle (
        cr, px_start, 0,
        px_end - px_start, height / 4.0);
      cairo_fill (cr);
    }

  /* draw the other regions */
  cairo_set_source_rgba (
    cr,
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
          ArrangerObject * other_region_obj =
            (ArrangerObject *) other_region;
          if (!g_strcmp0 (region->name,
                         other_region->name))
            continue;

          px_start =
            ui_pos_to_px_editor (
              &other_region_obj->pos, 1);
          px_end =
            ui_pos_to_px_editor (
              &other_region_obj->end_pos, 1);
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
          ArrangerObject * region_obj =
            (ArrangerObject *) CLIP_EDITOR->region;
          long start_ticks =
            position_to_ticks (
              &region_obj->pos);
          long loop_start_ticks =
            position_to_ticks (
              &region_obj->loop_start_pos) +
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
          ArrangerObject * region_obj =
            (ArrangerObject *) CLIP_EDITOR->region;
          long start_ticks =
            position_to_ticks (
              &region_obj->pos);
          long loop_end_ticks =
            position_to_ticks (
              &region_obj->loop_end_pos) +
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
          ArrangerObject * region_obj =
            (ArrangerObject *) CLIP_EDITOR->region;
          long start_ticks =
            position_to_ticks (
              &region_obj->pos);
          long clip_start_ticks =
            position_to_ticks (
              &region_obj->clip_start_pos) +
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
editor_ruler_on_drag_begin_no_marker_hit (
  EditorRulerWidget * self,
  gdouble             start_x,
  gdouble             start_y)
{
  RULER_WIDGET_GET_PRIVATE (self);

  Position pos;
  ui_px_to_pos_editor (
    start_x, &pos, 1);
  if (!rw_prv->shift_held)
    position_snap_simple (
      &pos, SNAP_GRID_MIDI);
  transport_move_playhead (&pos, 1);
  rw_prv->action =
    UI_OVERLAY_ACTION_STARTING_MOVING;
  rw_prv->target = RW_TARGET_PLAYHEAD;
}

void
editor_ruler_on_drag_update (
  EditorRulerWidget * self,
  gdouble             offset_x,
  gdouble             offset_y)
{
  RULER_WIDGET_GET_PRIVATE (self);

  if (ACTION_IS (MOVING))
    {
      Position editor_pos;
      Position region_local_pos;
      Region * r = CLIP_EDITOR->region;
      ArrangerObject * r_obj =
        (ArrangerObject *) r;

      /* convert px to position */
      if (self)
        ui_px_to_pos_editor (
          rw_prv->start_x + offset_x,
          &editor_pos, 1);

      /* snap if not shift held */
      if (!rw_prv->shift_held)
        position_snap_simple (
          &editor_pos, SNAP_GRID_MIDI);

      position_from_ticks (
        &region_local_pos,
        position_to_ticks (&editor_pos) -
        position_to_ticks (&r_obj->pos));

      if (TARGET_IS (LOOP_START))
        {
          /* make the position local to the region
           * for less calculations */
          /* if position is acceptable */
          if (position_compare (
                &region_local_pos,
                &r_obj->loop_end_pos) < 0 &&
              position_compare (
                &region_local_pos,
                &r_obj->clip_start_pos) >= 0)
            {
              /* set it */
              arranger_object_set_position (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_LOOP_START,
                F_NO_CACHED, F_VALIDATE,
                AO_UPDATE_ALL);
              transport_update_position_frames (
                TRANSPORT);
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (LOOP_END))
        {
          /* if position is acceptable */
          if (position_compare (
                &region_local_pos,
                &r_obj->loop_start_pos) > 0 &&
              position_compare (
                &region_local_pos,
                &r_obj->clip_start_pos) > 0)
            {
              /* set it */
              arranger_object_set_position (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_LOOP_END,
                F_NO_CACHED, F_VALIDATE,
                AO_UPDATE_ALL);
              transport_update_position_frames (
                TRANSPORT);
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (CLIP_START))
        {
          /* if position is acceptable */
          if (position_is_before_or_equal (
                &region_local_pos,
                &r_obj->loop_start_pos))
            {
              /* set it */
              arranger_object_set_position (
                r_obj, &region_local_pos,
                ARRANGER_OBJECT_POSITION_TYPE_CLIP_START,
                F_NO_CACHED, F_VALIDATE,
                AO_UPDATE_ALL);
              transport_update_position_frames (
                TRANSPORT);
              EVENTS_PUSH (
                ET_CLIP_MARKER_POS_CHANGED, self);
            }
        }
      else if (TARGET_IS (PLAYHEAD))
        {
          Position timeline_start,
                   timeline_end;
          position_init (&timeline_start);
          position_init (&timeline_end);
          position_add_bars (
            &timeline_end,
            TRANSPORT->total_bars);

          /* if position is acceptable */
          if (position_is_after_or_equal (
                &editor_pos, &timeline_start) &&
              position_is_before_or_equal (
                &editor_pos, &timeline_end))
            {
              transport_move_playhead (
                &editor_pos, 1);
              EVENTS_PUSH (
                ET_PLAYHEAD_POS_CHANGED, NULL);
            }

          ruler_marker_widget_update_tooltip (
            rw_prv->playhead, 1);
        }
    }
}

void
editor_ruler_on_drag_end (
  EditorRulerWidget * self)
{
  RULER_WIDGET_GET_PRIVATE (self);

  /* hide tooltips */
  if (TARGET_IS (PLAYHEAD))
    {
      ruler_marker_widget_update_tooltip (
        rw_prv->playhead, 0);
    }
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
