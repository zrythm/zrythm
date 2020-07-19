/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automation_region.h"
#include "audio/chord_track.h"
#include "audio/fade.h"
#include "audio/marker_track.h"
#include "gui/backend/arranger_object.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/chord_region.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/track.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n-lib.h>

#define TYPE(x) \
  ARRANGER_OBJECT_TYPE_##x

/**
 * Queues a redraw in the area covered by this
 * object.
 */
void
arranger_object_queue_redraw (
  ArrangerObject * self)
{
  g_return_if_fail (IS_ARRANGER_OBJECT (self));

  ArrangerWidget * arranger =
    arranger_object_get_arranger (self);
  g_return_if_fail (arranger);
  GdkRectangle arranger_rect;
  arranger_widget_get_visible_rect (
    arranger, &arranger_rect);

  /* if arranger is not visible ignore */
  if (arranger_rect.width < 2 &&
      arranger_rect.height < 2)
    {
#if 0
      arranger_object_print (self);
      g_message (
        "%s: arranger not visible, ignoring",
        __func__);
#endif
      return;
    }

  /* set rectangle if not initialized yet */
  if (self->full_rect.width == 0 &&
      self->full_rect.height == 0)
    arranger_object_set_full_rectangle (
      self, arranger);

  GdkRectangle full_rect = self->full_rect;

  /* add some padding to the full rect for any
   * effects or things like automation points */
  static const int padding = 6;
  if (self->type == TYPE (AUTOMATION_POINT))
    {
      full_rect.x = MAX (full_rect.x - padding, 0);
      full_rect.y = MAX (full_rect.y - padding, 0);
      full_rect.width += padding * 2;
      full_rect.height += padding * 2;
    }

  GdkRectangle draw_rect;
  int draw_rect_visible =
    arranger_object_get_draw_rectangle (
      self, &arranger_rect, &full_rect,
      &draw_rect);

  /* if draw rect is not visible ignore */
  if (!draw_rect_visible)
    {
#if 0
      arranger_object_print (self);
      g_message (
        "%s: draw rect not visible, ignoring",
        __func__);
#endif
      return;
    }

  arranger_widget_redraw_rectangle (
    arranger, &draw_rect);

  /* if region and lanes are visible, redraw
   * lane too */
  if (self->type == TYPE (REGION))
    {
      Track * track =
        arranger_object_get_track (self);
      if (track->lanes_visible &&
          arranger_object_can_have_lanes (self))
        {
          ZRegion * r = (ZRegion *) self;
          region_get_lane_full_rect (
            r, &full_rect);
          arranger_object_get_draw_rectangle (
            self, &arranger_rect, &full_rect,
            &draw_rect);
          arranger_widget_redraw_rectangle (
            arranger, &draw_rect);
        }
    }
}

/**
 * Returns if the current position is for resizing
 * L.
 *
 * @param x X in local coordinates.
 */
int
arranger_object_is_resize_l (
  ArrangerObject * self,
  const int        x)
{
  if (!arranger_object_type_has_length (self->type))
    return 0;

  if (x < UI_RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * R.
 *
 * @param x X in local coordinates.
 */
int
arranger_object_is_resize_r (
  ArrangerObject * self,
  const int        x)
{
  if (!arranger_object_type_has_length (self->type))
    return 0;

  long size_frames =
    self->end_pos.frames - self->pos.frames;
  Position pos;
  position_from_frames (
    &pos, size_frames);
  int width_px =
    arranger_widget_pos_to_px (
      arranger_object_get_arranger (self),
      &pos, 0);

  if (x > width_px - UI_RESIZE_CURSOR_SPACE)
    {
      return 1;
    }
  return 0;
}

/**
 * Returns if the current position is for moving the
 * fade in mark.
 *
 * @param x X in local coordinates.
 * @param only_handle Whether to only check if this
 *   is inside the fade handle. If this is false,
 *   \ref only_outer will be considered.
 * @param only_outer Whether to only check if this
 *   is inside the fade's outter (unplayed) region.
 *   If this is false, the whole fade area will
 *   be considered.
 */
int
arranger_object_is_fade_in (
  ArrangerObject * self,
  const int        x,
  const int        y,
  int              only_handle,
  int              only_outer)
{
  if (!arranger_object_can_fade (self))
    return 0;

  int fade_in_px =
    ui_pos_to_px_timeline (&self->fade_in_pos, 0);

  if (only_handle)
    {
      return
        x >=
          fade_in_px -
            ARRANGER_OBJECT_FADE_POINT_HALFWIDTH &&
        x <=
          fade_in_px +
            ARRANGER_OBJECT_FADE_POINT_HALFWIDTH &&
        y <= ARRANGER_OBJECT_FADE_POINT_HALFWIDTH;
    }
  else if (only_outer)
    {
      return
        x <= fade_in_px && fade_in_px > 0 &&
        (double) y <=
          self->full_rect.height *
          (1.0 -
            fade_get_y_normalized (
              (double) x / fade_in_px, &self->fade_in_opts, 1));
    }
  else
    {
      return x <= fade_in_px;
    }
}

/**
 * Returns if the current position is for moving the
 * fade out mark.
 *
 * @param x X in local coordinates.
 * @param only_handle Whether to only check if this
 *   is inside the fade handle. If this is false,
 *   \ref only_outer will be considered.
 * @param only_outer Whether to only check if this
 *   is inside the fade's outter (unplayed) region.
 *   If this is false, the whole fade area will
 *   be considered.
 */
int
arranger_object_is_fade_out (
  ArrangerObject * self,
  const int        x,
  const int        y,
  int              only_handle,
  int              only_outer)
{
  if (!arranger_object_can_fade (self))
    return 0;

  int fade_out_px =
    ui_pos_to_px_timeline (&self->fade_out_pos, 0);

  if (only_handle)
    {
      return
        x >=
          fade_out_px -
            ARRANGER_OBJECT_FADE_POINT_HALFWIDTH &&
        x <=
          fade_out_px +
            ARRANGER_OBJECT_FADE_POINT_HALFWIDTH &&
        y <= ARRANGER_OBJECT_FADE_POINT_HALFWIDTH;
    }
  else if (only_outer)
    {
      return
        x >= fade_out_px &&
        self->full_rect.width - fade_out_px > 0 &&
        (double) y <=
          self->full_rect.height *
          (1.0 -
            fade_get_y_normalized (
              (double) (x - fade_out_px) /
                (self->full_rect.width - fade_out_px),
              &self->fade_out_opts, 0));
    }
  else
    {
      return x >= fade_out_px;
    }
}

/**
 * Returns if the current position is for resizing
 * up (eg, Velocity).
 *
 * @param x X in local coordinates.
 * @param y Y in local coordinates.
 */
int
arranger_object_is_resize_up (
  ArrangerObject * self,
  const int        x,
  const int        y)
{
  if (self->type ==
         ARRANGER_OBJECT_TYPE_VELOCITY)
    {
      if (y < UI_RESIZE_CURSOR_SPACE)
        return 1;
    }
  else if (self->type ==
             ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
    {
      AutomationPoint * ap =
        (AutomationPoint*) self;
      int curve_up =
        automation_point_curves_up (ap);
      if (curve_up)
        {
          if (x > AP_WIDGET_POINT_SIZE ||
              self->full_rect.height - y >
                AP_WIDGET_POINT_SIZE)
            return 1;
        }
      else
        {
          if (x > AP_WIDGET_POINT_SIZE ||
              y > AP_WIDGET_POINT_SIZE)
            return 1;
        }
    }
  return 0;
}

/**
 * Returns if the current position is for resizing
 * loop.
 *
 * @param y Y in local coordinates.
 */
int
arranger_object_is_resize_loop (
  ArrangerObject * self,
  const int        y)
{
  if (!arranger_object_type_has_length (
        self->type) ||
      !arranger_object_type_can_loop (
        self->type))
    return 0;

  if (self->type == ARRANGER_OBJECT_TYPE_REGION)
    {
      ZRegion * r = (ZRegion *) self;
      if (r->id.type == REGION_TYPE_AUDIO &&
          P_TOOL != TOOL_SELECT_STRETCH)
        {
          return 1;
        }

      if ((position_to_ticks (&self->end_pos) -
           position_to_ticks (&self->pos)) >
          position_to_ticks (&self->loop_end_pos) +
            /* add some buffer because these are not
             * accurate */
            0.1)
        {
          return 1;
        }

      /* TODO */
      int height_px = 60;
        /*gtk_widget_get_allocated_height (*/
          /*GTK_WIDGET (self));*/

      if (y > height_px / 2)
        {
          return 1;
        }
      return 0;
    }

  return 0;
}

/**
 * Returns if arranger_object widgets should show
 * cut lines.
 *
 * To be used to set the arranger_object's
 * "show_cut".
 *
 * @param alt_pressed Whether alt is currently
 *   pressed.
 */
int
arranger_object_should_show_cut_lines (
  ArrangerObject * self,
  int              alt_pressed)
{
  if (!arranger_object_type_has_length (self->type))
    return 0;

  switch (P_TOOL)
    {
    case TOOL_SELECT_NORMAL:
    case TOOL_SELECT_STRETCH:
      if (alt_pressed)
        return 1;
      else
        return 0;
      break;
    case TOOL_CUT:
      return 1;
      break;
    default:
      return 0;
      break;
    }
  g_return_val_if_reached (-1);
}

/**
 * Returns Y in pixels from the value based on the
 * allocation of the arranger.
 */
static int
get_automation_point_y (
  AutomationPoint * ap,
  ArrangerWidget *  arranger)
{
  /* ratio of current value in the range */
  float ap_ratio = ap->normalized_val;

  int allocated_h =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (arranger));
  int point =
    allocated_h -
    (int) (ap_ratio * (float) allocated_h);
  return point;
}

/**
 * Gets the full rectangle for a linked object.
 */
int
arranger_object_get_full_rect_x_for_region_child (
  ArrangerObject * self,
  ZRegion *        region,
  GdkRectangle *   full_rect)
{
  g_return_val_if_fail (region, 0);
  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  double region_start_ticks =
    region_obj->pos.total_ticks;
  Position tmp;

  /* use absolute position */
  position_from_ticks (
    &tmp,
    region_start_ticks +
      self->pos.total_ticks);
  return ui_pos_to_px_editor (&tmp, 1);
}

/**
 * @param is_transient Whether or not this object
 *   is a transient (object at original position
 *   during copy-moving).
 */
void
arranger_object_set_full_rectangle (
  ArrangerObject * self,
  ArrangerWidget * arranger)
{
#define WARN_IF_HAS_NEGATIVE_DIMENSIONS \
  g_warn_if_fail ( \
    self->full_rect.x >= 0 && \
    self->full_rect.y >= 0 && \
    self->full_rect.width >= 0 && \
    self->full_rect.height >= 0)

  switch (self->type)
    {
    case TYPE (CHORD_OBJECT):
      {
        ChordObject * co = (ChordObject *) self;
        ChordDescriptor * descr =
          chord_object_get_chord_descriptor (co);

        ZRegion * region =
          arranger_object_get_region (self);
        ArrangerObject * region_obj =
          (ArrangerObject *) region;

        double region_start_ticks =
          region_obj->pos.total_ticks;
        Position tmp;
        int adj_px_per_key =
          chord_editor_space_widget_get_chord_height (
            MW_CHORD_EDITOR_SPACE) + 1;

        /* use absolute position */
        position_from_ticks (
          &tmp,
          region_start_ticks +
          self->pos.total_ticks);
        self->full_rect.x =
          ui_pos_to_px_editor (&tmp, 1);
        self->full_rect.y =
          adj_px_per_key * co->chord_index;

        char chord_str[100];
        chord_descriptor_to_string (
          descr, chord_str);

        chord_object_recreate_pango_layouts (
          (ChordObject *) self);
        self->full_rect.width =
          self->textw +
          CHORD_OBJECT_WIDGET_TRIANGLE_W +
          Z_CAIRO_TEXT_PADDING * 2;

        self->full_rect.width =
          self->textw +
          CHORD_OBJECT_WIDGET_TRIANGLE_W +
          Z_CAIRO_TEXT_PADDING * 2;

        self->full_rect.height = adj_px_per_key;

        WARN_IF_HAS_NEGATIVE_DIMENSIONS;
      }
      break;
    case TYPE (AUTOMATION_POINT):
      {
        AutomationPoint * ap =
          (AutomationPoint *) self;
        ZRegion * region =
          arranger_object_get_region (self);
        ArrangerObject * region_obj =
          (ArrangerObject *) region;

        /* use absolute position */
        double region_start_ticks =
          region_obj->pos.total_ticks;
        Position tmp;
        position_from_ticks (
          &tmp,
          region_start_ticks +
          self->pos.total_ticks);
        self->full_rect.x =
          ui_pos_to_px_editor (&tmp, 1) -
            AP_WIDGET_POINT_SIZE / 2;
        g_warn_if_fail (self->full_rect.x >= 0);

        AutomationPoint * next_ap =
          automation_region_get_next_ap (
            region, ap, true,
            arranger->action ==
              UI_OVERLAY_ACTION_MOVING_COPY);

        if (next_ap)
          {
            ArrangerObject * next_obj =
              (ArrangerObject *) next_ap;

            /* get relative position from the
             * start AP to the next ap. */
            position_from_ticks (
              &tmp,
              next_obj->pos.total_ticks -
                self->pos.total_ticks);

            /* width is the relative position in px
             * plus half an AP_WIDGET_POINT_SIZE for
             * each side */
            self->full_rect.width =
              AP_WIDGET_POINT_SIZE +
              ui_pos_to_px_editor (&tmp, 0);

            g_warn_if_fail (
              self->full_rect.width >= 0);

            int cur_y =
              get_automation_point_y (ap, arranger);
            int next_y =
              get_automation_point_y (
                next_ap, arranger);

            self->full_rect.y =
              (cur_y > next_y ?
               next_y : cur_y) -
              AP_WIDGET_POINT_SIZE / 2;

            /* height is the relative relative diff in
             * px between the two points plus half an
             * AP_WIDGET_POINT_SIZE for each side */
            self->full_rect.height =
              (cur_y > next_y ?
               cur_y - next_y :
               next_y - cur_y) + AP_WIDGET_POINT_SIZE;
            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
        else
          {
            self->full_rect.y =
              get_automation_point_y (
                 ap, arranger) -
              AP_WIDGET_POINT_SIZE / 2;

            self->full_rect.width =
              AP_WIDGET_POINT_SIZE;
            self->full_rect.height =
              AP_WIDGET_POINT_SIZE;

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
        }
      break;
    case TYPE (REGION):
      {
        ZRegion * region =
          (ZRegion *) self;
        Track * track =
          arranger_object_get_track (self);

        if (!track->widget)
          track->widget = track_widget_new (track);

        self->full_rect.x =
          ui_pos_to_px_timeline (
            &self->pos,
            1);
        self->full_rect.width =
          (ui_pos_to_px_timeline (
            &self->end_pos,
            1) - self->full_rect.x) - 1;
        if (self->full_rect.width < 1)
          self->full_rect.width = 1;

        gint wx, wy;

        gtk_widget_translate_coordinates(
          (GtkWidget *) (track->widget),
          (GtkWidget *) (arranger),
          0, 0,
          &wx, &wy);
        /* for some reason it returns a few
         * negatives at first */
        if (wy < 0)
          wy = 0;

        if (region->id.type == REGION_TYPE_CHORD)
          {
            chord_region_recreate_pango_layouts (
              region);

            self->full_rect.y = wy;
            /* full height minus the space the
             * scales would require, plus some
             * padding */
            self->full_rect.height =
              track->main_height -
                (self->texth +
                 Z_CAIRO_TEXT_PADDING * 4);

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
        else if (region->id.type ==
                   REGION_TYPE_AUTOMATION)
          {
            AutomationTrack * at =
              region_get_automation_track (region);
            g_return_if_fail (at);
            if (!at->created ||
                !track->automation_visible)
              return;

            gtk_widget_translate_coordinates(
              (GtkWidget *) (track->widget),
              (GtkWidget *) (arranger),
              0, 0, &wx, &wy);
            /* for some reason it returns a few
             * negatives at first */
            if (wy < 0)
              wy = 0;

            self->full_rect.y =
              wy + at->y;
            self->full_rect.height = at->height;

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
        else
          {
            self->full_rect.y = wy;
            self->full_rect.height =
              track->main_height;

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
        /* leave some space for the line below
         * the region */
        self->full_rect.height--;
      }
      break;
    case TYPE (MIDI_NOTE):
      {
        MidiNote * mn = (MidiNote *) self;
        ZRegion * region =
          arranger_object_get_region (self);
        g_return_if_fail (region);
        ArrangerObject * region_obj =
          (ArrangerObject *) region;

        double region_start_ticks =
          region_obj->pos.total_ticks;
        Position tmp;
        double adj_px_per_key =
          MW_PIANO_ROLL_KEYS->px_per_key + 1.0;

        /* use absolute position */
        position_from_ticks (
          &tmp,
          region_start_ticks +
          self->pos.total_ticks);
        self->full_rect.x =
          ui_pos_to_px_editor (
            &tmp, 1);
        self->full_rect.y =
          (int)
          (adj_px_per_key *
           piano_roll_find_midi_note_descriptor_by_val (
             PIANO_ROLL, mn->val)->index);

        self->full_rect.height =
          (int) adj_px_per_key;
        if (PIANO_ROLL->drum_mode)
          {
            self->full_rect.width =
              self->full_rect.height;
            self->full_rect.x -=
              self->full_rect.width / 2;

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
        else
          {
            /* use absolute position */
            position_from_ticks (
              &tmp,
              region_start_ticks +
              self->end_pos.total_ticks);
            self->full_rect.width =
              ui_pos_to_px_editor (
                &tmp, 1) - self->full_rect.x;

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
      }
      break;
    case TYPE (SCALE_OBJECT):
      {
        Track * track = P_CHORD_TRACK;

        gint wx, wy;
        gtk_widget_translate_coordinates(
          (GtkWidget *) (track->widget),
          (GtkWidget *) (arranger),
          0, 0,
          &wx, &wy);
        /* for some reason it returns a few
         * negatives at first */
        if (wy < 0)
          wy = 0;

        self->full_rect.x =
          ui_pos_to_px_timeline (
            &self->pos, 1);
        scale_object_recreate_pango_layouts (
          (ScaleObject *) self);
        self->full_rect.width =
          self->textw +
          SCALE_OBJECT_WIDGET_TRIANGLE_W +
          Z_CAIRO_TEXT_PADDING * 2;

        int obj_height =
          self->texth + Z_CAIRO_TEXT_PADDING * 2;
        self->full_rect.y =
          (wy + track->main_height) - obj_height;
        self->full_rect.height = obj_height;

        WARN_IF_HAS_NEGATIVE_DIMENSIONS;
      }
      break;
    case TYPE (MARKER):
      {
        Track * track = P_MARKER_TRACK;

        gint wx, wy;
        gtk_widget_translate_coordinates(
          (GtkWidget *) (track->widget),
          (GtkWidget *) (arranger),
          0, 0,
          &wx, &wy);
        /* for some reason it returns a few
         * negatives at first */
        if (wy < 0)
          wy = 0;

        self->full_rect.x =
          ui_pos_to_px_timeline (
            &self->pos, 1);
        marker_recreate_pango_layouts (
          (Marker *) self);
        self->full_rect.width =
          self->textw + MARKER_WIDGET_TRIANGLE_W +
          Z_CAIRO_TEXT_PADDING * 2;

        int obj_height =
          self->texth + Z_CAIRO_TEXT_PADDING * 2;
        self->full_rect.y =
          (wy + track->main_height) - obj_height;
        self->full_rect.height = obj_height;

        WARN_IF_HAS_NEGATIVE_DIMENSIONS;
      }
      break;
    case TYPE (VELOCITY):
      {
        /* use transient or non transient note
         * depending on which is visible */
        Velocity * vel = (Velocity *) self;
        MidiNote * mn =
          velocity_get_midi_note (vel);
        g_return_if_fail (mn);
        ArrangerObject * mn_obj =
          (ArrangerObject *) mn;
        ZRegion * region =
          arranger_object_get_region (mn_obj);
        g_return_if_fail (region);
        ArrangerObject * region_obj =
          (ArrangerObject *) region;

        /* use absolute position */
        double region_start_ticks =
          region_obj->pos.total_ticks;
        Position tmp;
        position_from_ticks (
          &tmp,
          region_start_ticks +
          mn_obj->pos.total_ticks);
        self->full_rect.x =
          ui_pos_to_px_editor (&tmp, 1);
        int height =
          gtk_widget_get_allocated_height (
            GTK_WIDGET (arranger));

        int vel_px =
          (int)
          ((float) height *
            ((float) vel->vel / 127.f));
        self->full_rect.y = height - vel_px;
        self->full_rect.width = 12;
        self->full_rect.height = vel_px;

        WARN_IF_HAS_NEGATIVE_DIMENSIONS;
      }
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  if (self->full_rect.x < 0 ||
      self->full_rect.y < 0 ||
      self->full_rect.width < 0 ||
      self->full_rect.height < 0)
    {
      g_message ("Object:");
      arranger_object_print (self);
      g_warning (
        "The full rectangle of widget %p has "
        "negative dimensions: (%d,%d) w: %d h: %d. "
        "This should not happen. A rendering error "
        "is expected to occur.",
        self,
        self->full_rect.x, self->full_rect.y,
        self->full_rect.width,
        self->full_rect.height);
    }

#undef WARN_IF_HAS_NEGATIVE_DIMENSIONS
}

/**
 * Gets the draw rectangle based on the given
 * full rectangle of the arranger object.
 *
 * @param parent_rect The current arranger
 *   rectangle.
 * @param full_rect The object's full rectangle.
 *   This will usually be ArrangerObject->full_rect,
 *   unless drawing in a lane (for Region's).
 * @param draw_rect The rectangle will be set
 *   here.
 *
 * @return Whether the draw rect is visible.
 */
int
arranger_object_get_draw_rectangle (
  ArrangerObject * self,
  GdkRectangle *   parent_rect,
  GdkRectangle *   full_rect,
  GdkRectangle *   draw_rect)
{
  g_return_val_if_fail (
    full_rect->width > 0, false);

  if (!ui_rectangle_overlap (
        parent_rect, full_rect))
    return 0;

  draw_rect->x =
    MAX (full_rect->x, parent_rect->x);
  draw_rect->width =
    MIN (
      (parent_rect->x + parent_rect->width) -
        draw_rect->x,
      (full_rect->x + full_rect->width) -
      draw_rect->x);
  g_warn_if_fail (draw_rect->width >= 0);
  draw_rect->y =
    MAX (full_rect->y, parent_rect->y);
  draw_rect->height =
    MIN (
      (parent_rect->y + parent_rect->height) -
        draw_rect->y,
      (full_rect->y + full_rect->height) -
      draw_rect->y);
  g_warn_if_fail (draw_rect->height >= 0);

  return 1;
  /*g_message ("full rect: (%d, %d) w: %d h: %d",*/
    /*full_rect->x, full_rect->y,*/
    /*full_rect->width, full_rect->height);*/
  /*g_message ("draw rect: (%d, %d) w: %d h: %d",*/
    /*draw_rect->x, draw_rect->y,*/
    /*draw_rect->width, draw_rect->height);*/
}

/**
 * Draws the given object.
 *
 * To be called from the arranger's draw callback.
 *
 * @param cr Cairo context of the arranger.
 * @param rect Rectangle in the arranger.
 */
void
arranger_object_draw (
  ArrangerObject * self,
  ArrangerWidget * arranger,
  cairo_t *        cr,
  GdkRectangle *   rect)
{
  if (!ui_rectangle_overlap (
        &self->full_rect, rect))
    {
      g_message (
        "%s: object not visible, skipping",
        __func__);
    }

  switch (self->type)
    {
    case TYPE (AUTOMATION_POINT):
      automation_point_draw (
        (AutomationPoint *) self, cr, rect);
      break;
    case TYPE (REGION):
      region_draw (
        (ZRegion *) self, cr, rect);
      break;
    case TYPE (MIDI_NOTE):
      midi_note_draw (
        (MidiNote *) self, cr, rect);
      break;
    case TYPE (MARKER):
      marker_draw (
        (Marker *) self, cr, rect);
      break;
    case TYPE (SCALE_OBJECT):
      scale_object_draw (
        (ScaleObject *) self, cr, rect);
      break;
    case TYPE (CHORD_OBJECT):
      chord_object_draw (
        (ChordObject *) self, cr, rect);
      break;
    case TYPE (VELOCITY):
      velocity_draw (
        (Velocity *) self, cr, rect);
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}
