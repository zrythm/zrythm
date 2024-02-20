// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "actions/actions.h"
#include "actions/arranger_selections.h"
#include "dsp/automation_region.h"
#include "dsp/automation_track.h"
#include "dsp/channel.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/control_port.h"
#include "dsp/instrument_track.h"
#include "dsp/marker_track.h"
#include "dsp/midi_region.h"
#include "dsp/track.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_draw.h"
#include "gui/widgets/arranger_minimap.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/scale_selector_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (ArrangerWidget, arranger_widget, GTK_TYPE_WIDGET)

#define FOREACH_TYPE(func) \
  func (TIMELINE, timeline); \
  func (MIDI, midi); \
  func (AUDIO, audio); \
  func (AUTOMATION, automation); \
  func (MIDI_MODIFIER, midi_modifier); \
  func (CHORD, chord)

#define ACTION_IS(x) (self->action == UI_OVERLAY_ACTION_##x)

#define TYPE(x) ARRANGER_WIDGET_TYPE_##x

#define TYPE_IS(x) (self->type == TYPE (x))

#define SCROLL_PADDING 8

#define MAGIC_SCROLL_FACTOR 2.5

/**
 * Returns if the arranger can scroll vertically.
 */
bool
arranger_widget_can_scroll_vertically (ArrangerWidget * self)
{
  if (
    (self->type == TYPE (TIMELINE) && self->is_pinned)
    || self->type == TYPE (MIDI_MODIFIER) || self->type == TYPE (AUDIO)
    || self->type == TYPE (AUTOMATION))
    return false;

  return true;
}

/**
 * Get just the values, adjusted properly for special cases
 * (like pinned timeline).
 */
EditorSettings
arranger_widget_get_editor_setting_values (ArrangerWidget * self)
{
  EditorSettings * settings = arranger_widget_get_editor_settings (self);
  EditorSettings   ret = { 0 };
  g_return_val_if_fail (settings, ret);
  ret = *settings;
  if (!arranger_widget_can_scroll_vertically (self))
    {
      ret.scroll_start_y = 0;
    }

  return ret;
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ArrangerWidget * self);

const char *
arranger_widget_get_type_str (ArrangerWidgetType type)
{
  static const char * arranger_widget_type_str[] = {
    "timeline", "midi", "midi modifier", "audio", "chord", "automation",
  };
  return arranger_widget_type_str[type];
}

/**
 * Returns true if MIDI arranger and track mode
 * is enabled.
 */
bool
arranger_widget_get_drum_mode_enabled (ArrangerWidget * self)
{
  if (self->type != ARRANGER_WIDGET_TYPE_MIDI)
    return false;

  if (!CLIP_EDITOR->has_region)
    return false;

  Track * tr = clip_editor_get_track (CLIP_EDITOR);
  g_return_val_if_fail (tr, false);

  return tr->drum_mode;
}

/**
 * Returns the playhead's x coordinate in absolute
 * coordinates.
 */
int
arranger_widget_get_playhead_px (ArrangerWidget * self)
{
  RulerWidget * ruler = arranger_widget_get_ruler (self);
  return ruler_widget_get_playhead_px (ruler, false);
}

/**
 * Sets the cursor on the arranger and all of its
 * children.
 */
void
arranger_widget_set_cursor (ArrangerWidget * self, ArrangerCursor cursor)
{
#define SET_X_CURSOR(x) ui_set_##x##_cursor (GTK_WIDGET (self));

#define SET_CURSOR_FROM_NAME(name) \
  ui_set_cursor_from_name (GTK_WIDGET (self), name);

  switch (cursor)
    {
    case ARRANGER_CURSOR_SELECT:
      SET_X_CURSOR (pointer);
      break;
    case ARRANGER_CURSOR_SELECT_STRETCH:
      SET_X_CURSOR (pointer_stretch);
      break;
    case ARRANGER_CURSOR_EDIT:
      SET_X_CURSOR (pencil);
      break;
    case ARRANGER_CURSOR_AUTOFILL:
      SET_X_CURSOR (brush);
      break;
    case ARRANGER_CURSOR_CUT:
      SET_X_CURSOR (cut_clip);
      break;
    case ARRANGER_CURSOR_RAMP:
      SET_X_CURSOR (line);
      break;
    case ARRANGER_CURSOR_ERASER:
      SET_X_CURSOR (eraser);
      break;
    case ARRANGER_CURSOR_AUDITION:
      SET_X_CURSOR (speaker);
      break;
    case ARRANGER_CURSOR_GRAB:
      SET_X_CURSOR (hand);
      break;
    case ARRANGER_CURSOR_GRABBING:
      SET_CURSOR_FROM_NAME ("grabbing");
      break;
    case ARRANGER_CURSOR_GRABBING_COPY:
      SET_CURSOR_FROM_NAME ("copy");
      break;
    case ARRANGER_CURSOR_GRABBING_LINK:
      SET_CURSOR_FROM_NAME ("link");
      break;
    case ARRANGER_CURSOR_RESIZING_L:
    case ARRANGER_CURSOR_RESIZING_L_FADE:
      SET_X_CURSOR (left_resize);
      break;
    case ARRANGER_CURSOR_STRETCHING_L:
      SET_X_CURSOR (left_stretch);
      break;
    case ARRANGER_CURSOR_RESIZING_L_LOOP:
      SET_X_CURSOR (left_resize_loop);
      break;
    case ARRANGER_CURSOR_RESIZING_R:
    case ARRANGER_CURSOR_RESIZING_R_FADE:
      SET_X_CURSOR (right_resize);
      break;
    case ARRANGER_CURSOR_STRETCHING_R:
      SET_X_CURSOR (right_stretch);
      break;
    case ARRANGER_CURSOR_RESIZING_R_LOOP:
      SET_X_CURSOR (right_resize_loop);
      break;
    case ARRANGER_CURSOR_RESIZING_UP:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ARRANGER_CURSOR_RESIZING_UP_FADE_IN:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ARRANGER_CURSOR_RESIZING_UP_FADE_OUT:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ARRANGER_CURSOR_RANGE:
      SET_X_CURSOR (time_select);
      break;
    case ARRANGER_CURSOR_FADE_IN:
      SET_X_CURSOR (fade_in);
      break;
    case ARRANGER_CURSOR_FADE_OUT:
      SET_X_CURSOR (fade_out);
      break;
    case ARRANGER_CURSOR_RENAME:
      SET_CURSOR_FROM_NAME ("text");
      break;
    case ARRANGER_CURSOR_PANNING:
      SET_CURSOR_FROM_NAME ("all-scroll");
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

/**
 * Returns whether the cursor  at y is in the top
 * half of the arranger.
 */
static bool
is_cursor_in_top_half (ArrangerWidget * self, double y)
{
  int height = gtk_widget_get_height (GTK_WIDGET (self));
  return y < ((double) height / 2.0);
}

/**
 * Sets whether selecting objects or range.
 */
static void
set_select_type (ArrangerWidget * self, double y)
{
  if (self->type == TYPE (TIMELINE))
    {
      timeline_arranger_widget_set_select_type (self, y);
    }
  else if (self->type == TYPE (AUDIO))
    {
      if (is_cursor_in_top_half (self, y))
        {
          self->resizing_range = false;
        }
      else
        {
          self->resizing_range = true;
          self->resizing_range_start = true;
          self->action = UI_OVERLAY_ACTION_RESIZING_R;
        }
    }
}

SnapGrid *
arranger_widget_get_snap_grid (ArrangerWidget * self)
{
  if (
    self == MW_MIDI_MODIFIER_ARRANGER || self == MW_MIDI_ARRANGER
    || self == MW_AUTOMATION_ARRANGER || self == MW_AUDIO_ARRANGER
    || self == MW_CHORD_ARRANGER)
    {
      return SNAP_GRID_EDITOR;
    }
  else if (self == MW_TIMELINE || self == MW_PINNED_TIMELINE)
    {
      return SNAP_GRID_TIMELINE;
    }
  g_return_val_if_reached (NULL);
}

typedef struct ObjectOverlapInfo
{
  /**
   * When rect is NULL, this is a special case for
   * automation points. The object will only
   * be added if the cursor is on the automation
   * point or within n px from the curve.
   */
  GdkRectangle * rect;

  /** X, or -1 to not check x. */
  double x;

  /** Y, or -1 to not check y. */
  double y;

  /** Position for x or rect->x (cached). */
  Position start_pos;

  /**
   * Position for rect->x + rect->width.
   *
   * If rect is NULL, this is the same as
   * \ref start_pos.
   */
  Position end_pos;

  GPtrArray *      arr;
  ArrangerObject * obj;
} ObjectOverlapInfo;

/**
 * Adds the object to the array if it or its transient overlaps with the
 * rectangle, or with \ref x \ref y if \ref rect is NULL.
 *
 * @return Whether the object was added or not.
 */
HOT static bool
add_object_if_overlap (ArrangerWidget * self, ObjectOverlapInfo * nfo)
{
  GdkRectangle *   rect = nfo->rect;
  double           x = nfo->x;
  double           y = nfo->y;
  ArrangerObject * obj = nfo->obj;
  RulerWidget *    ruler = arranger_widget_get_ruler (self);

  g_return_val_if_fail (IS_ARRANGER_OBJECT (obj), false);
  g_return_val_if_fail (
    (math_doubles_equal (x, -1) || x >= 0.0)
      && (math_doubles_equal (y, -1) || y >= 0.0),
    false);

  if (obj->deleted_temporarily)
    {
      return false;
    }

  /* --- optimization to skip expensive calculations for most objects --- */

  bool orig_visible =
    arranger_object_should_orig_be_visible (obj, self) && obj->transient;

  /* skip objects that end before the rect */
  if (arranger_object_type_has_length (obj->type))
    {
      Position g_obj_end_pos;
      Position g_obj_end_pos_transient;
      if (arranger_object_type_has_global_pos (obj->type))
        {
          g_obj_end_pos = obj->end_pos;
          if (orig_visible)
            g_obj_end_pos_transient = obj->transient->end_pos;
        }
      else
        {
          ZRegion * r = arranger_object_get_region (obj);
          g_return_val_if_fail (IS_REGION_AND_NONNULL (r), false);
          g_obj_end_pos = r->base.pos;
          position_add_ticks (&g_obj_end_pos, obj->end_pos.ticks);
          if (orig_visible)
            {
              g_obj_end_pos_transient = r->base.pos;
              position_add_ticks (
                &g_obj_end_pos_transient, obj->transient->end_pos.ticks);
            }
        }
      if (position_is_before (&g_obj_end_pos, &nfo->start_pos))
        {
          if (orig_visible)
            {
              if (position_is_before (&g_obj_end_pos_transient, &nfo->start_pos))
                return false;
            }
          else
            return false;
        }
    }

  /* skip objects that start a few pixels after the end */
  Position g_obj_start_pos;
  Position g_obj_start_pos_transient;
  if (arranger_object_type_has_global_pos (obj->type))
    {
      g_obj_start_pos = obj->pos;
      if (orig_visible)
        g_obj_start_pos_transient = obj->transient->pos;
    }
  else
    {
      ZRegion * r = arranger_object_get_region (obj);
      g_return_val_if_fail (IS_REGION_AND_NONNULL (r), false);
      g_obj_start_pos = r->base.pos;
      position_add_ticks (&g_obj_start_pos, obj->pos.ticks);
      if (orig_visible)
        {
          g_obj_start_pos_transient = r->base.pos;
          position_add_ticks (
            &g_obj_start_pos_transient, obj->transient->pos.ticks);
        }
    }
  const double ticks_to_add = -12.0 / ruler->px_per_tick;
  position_add_ticks (&g_obj_start_pos, ticks_to_add);
  if (orig_visible)
    {
      position_add_ticks (&g_obj_start_pos_transient, ticks_to_add);
    }
  if (position_is_after (&g_obj_start_pos, &nfo->end_pos))
    {
      if (orig_visible)
        {
          if (position_is_after (&g_obj_start_pos_transient, &nfo->end_pos))
            return false;
        }
      else
        return false;
    }

  /* --- end optimization --- */

  bool is_same_arranger = arranger_object_get_arranger (obj) == self;
  if (!is_same_arranger)
    return false;

  arranger_object_set_full_rectangle (obj, self);
  bool add = false;
  if (rect)
    {
      if (
        (ui_rectangle_overlap (&obj->full_rect, rect) ||
         /* also check original (transient) */
         (orig_visible && obj->transient
          && ui_rectangle_overlap (&obj->transient->full_rect, rect))))
        {
          add = true;
        }
    }
  else if (
    (ui_is_point_in_rect_hit (
       &obj->full_rect, x >= 0 ? true : false, y >= 0 ? true : false, x, y, 0, 0)
     ||
     /* also check original (transient) */
     (orig_visible
      && ui_is_point_in_rect_hit (
        &obj->transient->full_rect, x >= 0 ? true : false,
        y >= 0 ? true : false, x, y, 0, 0))))
    {
      if (obj->type == ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
        {
          /* object to check for automation point curve cross (either main
           * object or transient) */
          AutomationPoint * ap_to_check =
            (AutomationPoint *) ((orig_visible
                                  && ui_is_point_in_rect_hit (
                                    &obj->transient->full_rect, x >= 0 ? true : false,
                                    y >= 0 ? true : false, x, y, 0, 0))
                                   ? obj->transient
                                   : obj);

          /* handle special case for automation points */
          if (
            !automation_point_is_point_hit (ap_to_check, x, y)
            && !automation_point_is_curve_hit (ap_to_check, x, y, 16.0))
            {
              return false;
            }
        } /* endif automation point */

      add = true;
    }

  if (add)
    {
      g_ptr_array_add (nfo->arr, obj);
    }

  return add;
}

/**
 * Fills in the given array with the
 * ArrangerObject's of the given type that appear
 * in the given rect, or at the given coords if
 * \ref rect is NULL.
 *
 * @param rect The rectangle to search in.
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 */
OPTIMIZE_O3
static void
get_hit_objects (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  GdkRectangle *     rect,
  double             x,
  double             y,
  GPtrArray *        arr)
{
  g_return_if_fail (self && arr);

  if (!math_doubles_equal (x, -1) && x < 0.0)
    {
      g_critical ("invalid x: %f", x);
      return;
    }
  if (!math_doubles_equal (y, -1) && y < 0.0)
    {
      g_critical ("invalid y: %f", y);
      return;
    }

  ArrangerObject * obj = NULL;

  /* skip if haven't drawn yet */
  if (self->first_draw)
    {
      return;
    }

  int start_y = rect ? rect->y : (int) y;

  /* prepare struct to pass for each object */
  ObjectOverlapInfo nfo;
  nfo.rect = rect;
  nfo.x = x;
  nfo.y = y;
  arranger_widget_px_to_pos (self, rect ? rect->x : x, &nfo.start_pos, true);
  if (rect)
    {
      arranger_widget_px_to_pos (
        self, rect->x + rect->width, &nfo.end_pos, F_PADDING);
    }
  else
    {
      nfo.end_pos = nfo.start_pos;
    }
  nfo.arr = arr;

  switch (self->type)
    {
    case TYPE (TIMELINE):
      if (
        type != ARRANGER_OBJECT_TYPE_ALL && type != ARRANGER_OBJECT_TYPE_REGION
        && type != ARRANGER_OBJECT_TYPE_MARKER
        && type != ARRANGER_OBJECT_TYPE_SCALE_OBJECT)
        break;

      /* add overlapping regions */
      if (
        type == ARRANGER_OBJECT_TYPE_ALL || type == ARRANGER_OBJECT_TYPE_REGION)
        {
          /* midi and audio regions */
          for (int i = 0; i < TRACKLIST->num_tracks; i++)
            {
              Track * track = TRACKLIST->tracks[i];

              /* skip tracks if not visible or this
               * is timeline and pin status doesn't
               * match */
              if (
                !track->visible
                || (TYPE (TIMELINE) && track_is_pinned (track) != self->is_pinned))
                {
                  continue;
                }

              /* skip if track should not be visible */
              if (!track_get_should_be_visible (track))
                continue;

              if (G_LIKELY (track->widget))
                {
                  int track_y =
                    track_widget_get_local_y (track->widget, self, start_y);

                  /* skip if track starts after the
                   * rect */
                  if (track_y + (rect ? rect->height : 0) < 0)
                    {
                      continue;
                    }

                  double full_track_height =
                    track_get_full_visible_height (track);

                  /* skip if track ends before the rect */
                  if (track_y > full_track_height)
                    {
                      continue;
                    }
                }

              for (int j = 0; j < track->num_lanes; j++)
                {
                  TrackLane * lane = track->lanes[j];
                  for (int k = 0; k < lane->num_regions; k++)
                    {
                      ZRegion * r = lane->regions[k];
                      g_warn_if_fail (IS_REGION (r));
                      obj = (ArrangerObject *) r;
                      nfo.obj = obj;
                      bool ret = add_object_if_overlap (self, &nfo);
                      if (!ret)
                        {
                          /* check lanes */
                          if (!track->lanes_visible)
                            continue;
                          GdkRectangle lane_rect;
                          region_get_lane_full_rect (
                            lane->regions[k], &lane_rect);
                          if (
                            ((rect && ui_rectangle_overlap (&lane_rect, rect))
                             || (!rect && ui_is_point_in_rect_hit (&lane_rect, true, true, x, y, 0, 0)))
                            && arranger_object_get_arranger (obj) == self
                            && !obj->deleted_temporarily)
                            {
                              g_ptr_array_add (arr, obj);
                            }
                        }
                    }
                }

              /* chord regions */
              for (int j = 0; j < track->num_chord_regions; j++)
                {
                  ZRegion * cr = track->chord_regions[j];
                  obj = (ArrangerObject *) cr;
                  nfo.obj = obj;
                  add_object_if_overlap (self, &nfo);
                }

              /* automation regions */
              AutomationTracklist * atl = track_get_automation_tracklist (track);
              if (atl && track->automation_visible)
                {
                  for (size_t j = 0; j < atl->visible_ats->len; j++)
                    {
                      AutomationTrack * at =
                        g_ptr_array_index (atl->visible_ats, j);

                      for (int k = 0; k < at->num_regions; k++)
                        {
                          obj = (ArrangerObject *) at->regions[k];
                          nfo.obj = obj;
                          add_object_if_overlap (self, &nfo);
                        }
                    }
                }
            }
        }

      /* add overlapping scales */
      if (
        (type == ARRANGER_OBJECT_TYPE_ALL
         || type == ARRANGER_OBJECT_TYPE_SCALE_OBJECT)
        && track_get_should_be_visible (P_CHORD_TRACK))
        {
          for (int j = 0; j < P_CHORD_TRACK->num_scales; j++)
            {
              ScaleObject * scale = P_CHORD_TRACK->scales[j];
              obj = (ArrangerObject *) scale;
              nfo.obj = obj;
              add_object_if_overlap (self, &nfo);
            }
        }

      /* add overlapping markers */
      if (
        (type == ARRANGER_OBJECT_TYPE_ALL || type == ARRANGER_OBJECT_TYPE_MARKER)
        && track_get_should_be_visible (P_MARKER_TRACK))
        {
          for (int j = 0; j < P_MARKER_TRACK->num_markers; j++)
            {
              Marker * marker = P_MARKER_TRACK->markers[j];
              obj = (ArrangerObject *) marker;
              nfo.obj = obj;
              add_object_if_overlap (self, &nfo);
            }
        }
      break;
    case TYPE (MIDI):
      /* add overlapping midi notes */
      if (
        type == ARRANGER_OBJECT_TYPE_ALL
        || type == ARRANGER_OBJECT_TYPE_MIDI_NOTE)
        {
          ZRegion * r = clip_editor_get_region (CLIP_EDITOR);
          if (!r)
            break;

          /* add main region notes */
          for (int i = 0; i < r->num_midi_notes; i++)
            {
              MidiNote * mn = r->midi_notes[i];
              obj = (ArrangerObject *) mn;
              nfo.obj = obj;
              add_object_if_overlap (self, &nfo);
            }

          if (g_settings_get_boolean (S_UI, "ghost-notes"))
            {
              /* add other region notes for same track
               * (ghosted) */
              Track * track = arranger_object_get_track ((ArrangerObject *) r);
              g_return_if_fail (track);

              for (int i = 0; i < track->num_lanes; i++)
                {
                  TrackLane * lane = track->lanes[i];
                  for (int j = 0; j < lane->num_regions; j++)
                    {
                      ZRegion * cur_r = lane->regions[j];
                      if (cur_r == r)
                        continue;
                      for (int k = 0; k < cur_r->num_midi_notes; k++)
                        {
                          MidiNote * mn = cur_r->midi_notes[k];
                          obj = (ArrangerObject *) mn;
                          nfo.obj = obj;
                          add_object_if_overlap (self, &nfo);
                        }
                    }
                }
            }
        }
      break;
    case TYPE (MIDI_MODIFIER):
      /* add overlapping midi notes */
      if (
        type == ARRANGER_OBJECT_TYPE_ALL
        || type == ARRANGER_OBJECT_TYPE_VELOCITY)
        {
          ZRegion * r = clip_editor_get_region (CLIP_EDITOR);
          if (!r)
            break;

          for (int i = 0; i < r->num_midi_notes; i++)
            {
              MidiNote * mn = r->midi_notes[i];
              g_return_if_fail (IS_MIDI_NOTE (mn));
              Velocity * vel = mn->vel;
              g_return_if_fail (IS_ARRANGER_OBJECT (vel));
              obj = (ArrangerObject *) vel;
              nfo.obj = obj;
              add_object_if_overlap (self, &nfo);
            }
        }
      break;
    case TYPE (CHORD):
      /* add overlapping midi notes */
      if (
        type == ARRANGER_OBJECT_TYPE_ALL
        || type == ARRANGER_OBJECT_TYPE_CHORD_OBJECT)
        {
          ZRegion * r = clip_editor_get_region (CLIP_EDITOR);
          if (!r)
            break;

          for (int i = 0; i < r->num_chord_objects; i++)
            {
              ChordObject * co = r->chord_objects[i];
              obj = (ArrangerObject *) co;
              g_return_if_fail (co->chord_index < CHORD_EDITOR->num_chords);
              nfo.obj = obj;
              add_object_if_overlap (self, &nfo);
            }
        }
      break;
    case TYPE (AUTOMATION):
      /* add overlapping midi notes */
      if (
        type == ARRANGER_OBJECT_TYPE_ALL
        || type == ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
        {
          ZRegion * r = clip_editor_get_region (CLIP_EDITOR);
          if (!r)
            break;

          for (int i = 0; i < r->num_aps; i++)
            {
              AutomationPoint * ap = r->aps[i];
              obj = (ArrangerObject *) ap;
              nfo.obj = obj;
              add_object_if_overlap (self, &nfo);
            }
        }
      break;
    case TYPE (AUDIO):
      /* no objects in audio arranger yet */
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

/**
 * Fills in the given array with the ArrangerObject's
 * of the given type that appear in the given
 * range.
 *
 * @param rect The rectangle to search in.
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 */
void
arranger_widget_get_hit_objects_in_rect (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  GdkRectangle *     rect,
  GPtrArray *        arr)
{
  get_hit_objects (self, type, rect, 0, 0, arr);
}

/**
 * Filters out objects from frozen tracks.
 */
static void
filter_out_frozen_objects (ArrangerWidget * self, GPtrArray * objs_arr)
{
  if (self->type != ARRANGER_WIDGET_TYPE_TIMELINE)
    {
      return;
    }

  for (int i = (int) objs_arr->len - 1; i >= 0; i--)
    {
      ArrangerObject * obj = g_ptr_array_index (objs_arr, i);
      Track *          track = arranger_object_get_track (obj);
      g_return_if_fail (track);

      if (track->frozen)
        {
          g_ptr_array_remove_index (objs_arr, (guint) i);
        }
    }
}

/**
 * Fills in the given array with the ArrangerObject's
 * of the given type that appear in the given
 * ranger.
 *
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 */
void
arranger_widget_get_hit_objects_at_point (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  double             x,
  double             y,
  GPtrArray *        arr)
{
  get_hit_objects (self, type, NULL, x, y, arr);
}

/**
 * Returns if the arranger is in a moving-related
 * operation or starting a moving-related operation.
 *
 * Useful to know if we need transient widgets or
 * not.
 */
bool
arranger_widget_is_in_moving_operation (ArrangerWidget * self)
{
  if (
    self->action == UI_OVERLAY_ACTION_STARTING_MOVING
    || self->action == UI_OVERLAY_ACTION_STARTING_MOVING_COPY
    || self->action == UI_OVERLAY_ACTION_STARTING_MOVING_LINK
    || self->action == UI_OVERLAY_ACTION_MOVING
    || self->action == UI_OVERLAY_ACTION_MOVING_COPY
    || self->action == UI_OVERLAY_ACTION_MOVING_LINK)
    return true;

  return false;
}

/**
 * Moves the ArrangerSelections by the given
 * amount of ticks.
 *
 * @param ticks_diff Ticks to move by.
 * @param copy_moving 1 if copy-moving.
 */
static void
move_items_x (ArrangerWidget * self, const double ticks_diff)
{
  ArrangerSelections * sel = arranger_widget_get_selections (self);
  g_return_if_fail (sel);

  EVENTS_PUSH_NOW (ET_ARRANGER_SELECTIONS_IN_TRANSIT, sel);

  arranger_selections_add_ticks (sel, ticks_diff);
  /*g_debug ("adding %f ticks to selections", ticks_diff);*/

  if (sel->type == ARRANGER_SELECTIONS_TYPE_AUTOMATION)
    {
      /* re-sort the automation region */
      ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
      g_return_if_fail (region);
      automation_region_force_sort (region);
    }

  transport_recalculate_total_bars (TRANSPORT, sel);

  EVENTS_PUSH_NOW (ET_ARRANGER_SELECTIONS_IN_TRANSIT, sel);
}

/**
 * Gets the float value at the given Y coordinate
 * relative to the automation arranger.
 */
static float
get_fvalue_at_y (ArrangerWidget * self, double y)
{
  float height = (float) gtk_widget_get_height (GTK_WIDGET (self));

  ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
  g_return_val_if_fail (
    region && region->id.type == REGION_TYPE_AUTOMATION, -1.f);
  AutomationTrack * at = region_get_automation_track (region);

  /* get ratio from widget */
  float  widget_value = height - (float) y;
  float  widget_ratio = CLAMP (widget_value / height, 0.f, 1.f);
  Port * port = port_find_from_identifier (&at->port_id);
  float  automatable_value =
    control_port_normalized_val_to_real (port, widget_ratio);

  return automatable_value;
}

/**
 * Move selected arranger objects vertically.
 */
static void
move_items_y (ArrangerWidget * self, double offset_y)
{
  ArrangerSelections * sel = arranger_widget_get_selections (self);
  g_return_if_fail (sel);

  switch (self->type)
    {
    case TYPE (AUTOMATION):
      if (AUTOMATION_SELECTIONS->num_automation_points)
        {
          double offset_y_normalized =
            -offset_y / (double) gtk_widget_get_height (GTK_WIDGET (self));
          g_warn_if_fail (self->sel_at_start);
          (void) get_fvalue_at_y;
          for (int i = 0; i < AUTOMATION_SELECTIONS->num_automation_points; i++)
            {
              AutomationPoint * ap = AUTOMATION_SELECTIONS->automation_points[i];
              AutomationSelections * automation_sel =
                (AutomationSelections *) self->sel_at_start;
              AutomationPoint * start_ap = automation_sel->automation_points[i];

              automation_point_set_fvalue (
                ap, start_ap->normalized_val + (float) offset_y_normalized,
                F_NORMALIZED, F_PUBLISH_EVENTS);
            }
          ArrangerObject * start_ap_obj = self->start_object;
          g_return_if_fail (start_ap_obj);
          /*arranger_object_widget_update_tooltip (*/
          /*Z_ARRANGER_OBJECT_WIDGET (*/
          /*start_ap_obj->widget), 1);*/
        }
      break;
    case TYPE (TIMELINE):
      {
        /* old = original track
         * last = track where last change happened
         * (new) = track at hover position */
        Track * track = timeline_arranger_widget_get_track_at_y (
          self, self->start_y + offset_y);
        Track * old_track =
          timeline_arranger_widget_get_track_at_y (self, self->start_y);
        Track * last_track = tracklist_get_visible_track_after_delta (
          TRACKLIST, old_track, self->visible_track_diff);

        TrackLane * lane = timeline_arranger_widget_get_track_lane_at_y (
          self, self->start_y + offset_y);
        TrackLane * old_lane =
          timeline_arranger_widget_get_track_lane_at_y (self, self->start_y);
        TrackLane * last_lane = NULL;
        if (old_lane)
          {
            Track * old_lane_track = track_lane_get_track (old_lane);
            last_lane = old_lane_track->lanes[old_lane->pos + self->lane_diff];
          }

        AutomationTrack * at =
          timeline_arranger_widget_get_at_at_y (self, self->start_y + offset_y);
        AutomationTrack * old_at =
          timeline_arranger_widget_get_at_at_y (self, self->start_y);
        AutomationTrack * last_at = NULL;
        if (track && old_at)
          {
            last_at = automation_tracklist_get_visible_at_after_delta (
              &track->automation_tracklist, old_at, self->visible_at_diff);
          }

        /* if new track is equal, move lanes or
         * automation lanes */
        if (
          track && last_track && track == last_track
          && self->visible_track_diff == 0)
          {
            if (old_lane && lane && last_lane && old_lane != lane)
              {
                int cur_diff = lane->pos - old_lane->pos;
                int delta = lane->pos - last_lane->pos;
                if (delta != 0)
                  {
                    bool moved = timeline_selections_move_regions_to_new_lanes (
                      TL_SELECTIONS, delta);
                    if (moved)
                      {
                        self->lane_diff = cur_diff;
                      }
                  }
              }
            else if (at && old_at && last_at && at != old_at)
              {
                int cur_diff = automation_tracklist_get_visible_at_diff (
                  &track->automation_tracklist, old_at, at);
                int delta = automation_tracklist_get_visible_at_diff (
                  &track->automation_tracklist, last_at, at);
                if (delta != 0)
                  {
                    bool moved = timeline_selections_move_regions_to_new_ats (
                      TL_SELECTIONS, delta);
                    if (moved)
                      {
                        self->visible_at_diff = cur_diff;
                      }
                  }
              }
          }
        /* otherwise move tracks */
        else if (track && last_track && old_track && track != last_track)
          {
            int cur_diff =
              tracklist_get_visible_track_diff (TRACKLIST, old_track, track);
            int delta =
              tracklist_get_visible_track_diff (TRACKLIST, last_track, track);
            if (delta != 0)
              {
                bool moved = timeline_selections_move_regions_to_new_tracks (
                  TL_SELECTIONS, delta);
                if (moved)
                  {
                    self->visible_track_diff = cur_diff;
                  }
              }
          }
      }
      break;
    case TYPE (MIDI):
      {
        int y_delta;
        /* first note selected */
        int first_note_selected = ((MidiNote *) self->start_object)->val;
        /* note at cursor */
        int note_at_cursor = piano_roll_keys_widget_get_key_from_y (
          MW_PIANO_ROLL_KEYS, self->start_y + offset_y);

        y_delta = note_at_cursor - first_note_selected;
        y_delta = midi_arranger_calc_deltamax_for_note_movement (y_delta);

        for (int i = 0; i < MA_SELECTIONS->num_midi_notes; i++)
          {
            MidiNote * midi_note = MA_SELECTIONS->midi_notes[i];
            /*ArrangerObject * mn_obj =*/
            /*(ArrangerObject *) midi_note;*/
            midi_note_set_val (
              midi_note, (midi_byte_t) ((int) midi_note->val + y_delta));
            /*if (Z_IS_ARRANGER_OBJECT_WIDGET (*/
            /*mn_obj->widget))*/
            /*{*/
            /*arranger_object_widget_update_tooltip (*/
            /*Z_ARRANGER_OBJECT_WIDGET (*/
            /*mn_obj->widget), 0);*/
            /*}*/
          }

        /*midi_arranger_listen_notes (*/
        /*self, 1);*/
      }
      break;
    case TYPE (CHORD):
      {
        int y_delta;
        /* first chord selected */
        ChordObject * first_co = (ChordObject *) self->start_object;
        int           first_chord_selected = first_co->chord_index;
        /* chord at cursor */
        int chord_at_cursor =
          chord_arranger_widget_get_chord_at_y (self->start_y + offset_y);

        y_delta = chord_at_cursor - first_chord_selected;
        y_delta = chord_arranger_calc_deltamax_for_chord_movement (y_delta);

        for (int i = 0; i < CHORD_SELECTIONS->num_chord_objects; i++)
          {
            ChordObject * co = CHORD_SELECTIONS->chord_objects[i];
            co->chord_index = co->chord_index + y_delta;
          }
      }
      break;
    default:
      break;
    }
}

void
arranger_widget_select_all (ArrangerWidget * self, bool select, bool fire_events)
{
  ArrangerSelections * sel =
    arranger_widget_get_selections ((ArrangerWidget *) self);
  g_return_if_fail (sel);

  if (select)
    {
      GPtrArray * objs_arr = g_ptr_array_new_full (200, NULL);
      arranger_widget_get_all_objects (self, objs_arr);
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          arranger_object_select (obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
        }
      g_ptr_array_unref (objs_arr);

      if (fire_events)
        {
          EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CREATED, sel);
        }
    }
  else
    {
      g_debug ("deselecting all");
      if (arranger_selections_has_any (sel))
        {
          arranger_selections_clear (sel, F_NO_FREE, F_NO_PUBLISH_EVENTS);

          if (fire_events)
            {
              EVENTS_PUSH_NOW (ET_ARRANGER_SELECTIONS_REMOVED, sel);
            }
        }
    }
}

/**
 * Returns the EditorSettings corresponding to
 * the given arranger.
 */
EditorSettings *
arranger_widget_get_editor_settings (ArrangerWidget * self)
{
  switch (self->type)
    {
    case ARRANGER_WIDGET_TYPE_TIMELINE:
      return &PRJ_TIMELINE->editor_settings;
      break;
    case ARRANGER_WIDGET_TYPE_AUTOMATION:
      return &AUTOMATION_EDITOR->editor_settings;
      break;
    case ARRANGER_WIDGET_TYPE_AUDIO:
      return &AUDIO_CLIP_EDITOR->editor_settings;
      break;
    case ARRANGER_WIDGET_TYPE_MIDI:
    case ARRANGER_WIDGET_TYPE_MIDI_MODIFIER:
      return &PIANO_ROLL->editor_settings;
      break;
    case ARRANGER_WIDGET_TYPE_CHORD:
      return &CHORD_EDITOR->editor_settings;
      break;
    default:
      break;
    }

  g_return_val_if_reached (NULL);
}

static GMenu *
gen_context_menu_audio (ArrangerWidget * self, GMenu * menu, gdouble x, gdouble y)
{
  return g_menu_new ();
}

static GMenu *
gen_context_menu_chord (ArrangerWidget * self, GMenu * menu, gdouble x, gdouble y)
{
  return g_menu_new ();
}

static GMenu *
gen_context_menu_midi_modifier (
  ArrangerWidget * self,
  GMenu *          menu,
  gdouble          x,
  gdouble          y)
{
  return g_menu_new ();
}

/**
 * Show context menu at the given point.
 *
 * @param x X in absolute coordinates.
 * @param y Y in absolute coordinates.
 */
static void
show_context_menu (ArrangerWidget * self, gdouble x, gdouble y)
{
  GMenu *     menu = NULL;
  GMenuItem * menuitem;
  switch (self->type)
    {
    case TYPE (TIMELINE):
      menu = timeline_arranger_widget_gen_context_menu (self, menu, x, y);
      break;
    case TYPE (MIDI):
      menu = midi_arranger_widget_gen_context_menu (self, menu, x, y);
      break;
    case TYPE (MIDI_MODIFIER):
      menu = gen_context_menu_midi_modifier (self, menu, x, y);
      break;
    case TYPE (CHORD):
      menu = gen_context_menu_chord (self, menu, x, y);
      break;
    case TYPE (AUTOMATION):
      menu = automation_arranger_widget_gen_context_menu (self, menu, x, y);
      break;
    case TYPE (AUDIO):
      menu = gen_context_menu_audio (self, menu, x, y);
      break;
    }

  g_return_if_fail (menu);

  ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
    self, ARRANGER_OBJECT_TYPE_ALL, x, y);

  if (!obj)
    {
      GMenu * create_submenu = g_menu_new ();

      /* add "create object" menu item */
      char action_name[500];
      sprintf (action_name, "app.create-arranger-obj(('%p',%f,%f))", self, x, y);
      menuitem = z_gtk_create_menu_item (_ ("Create Object"), NULL, action_name);
      g_menu_append_item (create_submenu, menuitem);

      g_menu_append_section (menu, _ ("Create"), G_MENU_MODEL (create_submenu));
    }

  const EditorSettings settings =
    arranger_widget_get_editor_setting_values (self);
  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x - settings.scroll_start_x,
    y - settings.scroll_start_y, menu);
}

static void
on_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  ArrangerWidget *  self)
{
  g_message ("right mb released");
#if 0
  if (n_press != 1)
    return;

  MAIN_WINDOW->last_focused = GTK_WIDGET (self);

  /* if object clicked and object is unselected,
   * select it */
  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_ALL, x, y);
  if (obj)
    {
      if (!arranger_object_is_selected (obj))
        {
          arranger_object_select (
            obj, F_SELECT, F_NO_APPEND,
            F_NO_PUBLISH_EVENTS);
        }
    }

  show_context_menu (self, x, y);
#endif
}

static void
auto_scroll (ArrangerWidget * self, int x, int y)
{
  /* figure out if we should scroll */
  bool scroll_h = false, scroll_v = false;
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_MOVING:
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_SELECTING:
    case UI_OVERLAY_ACTION_RAMPING:
      scroll_h = true;
      scroll_v = true;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_RESIZING_L:
    case UI_OVERLAY_ACTION_STRETCHING_L:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
    case UI_OVERLAY_ACTION_STRETCHING_R:
    case UI_OVERLAY_ACTION_AUTOFILLING:
    case UI_OVERLAY_ACTION_AUDITIONING:
      scroll_h = true;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      scroll_v = true;
      break;
    default:
      break;
    }

  if (!arranger_widget_can_scroll_vertically (self))
    {
      scroll_v = false;
    }

  if (!scroll_h && !scroll_v)
    return;

  EditorSettings * settings = arranger_widget_get_editor_settings (self);
  g_return_if_fail (settings);
  int h_scroll_speed = 20;
  int v_scroll_speed = 10;
  int border_distance = 5;
  int scroll_width = gtk_widget_get_width (GTK_WIDGET (self));
  int scroll_height = gtk_widget_get_height (GTK_WIDGET (self));
  int v_delta = 0;
  int h_delta = 0;
  int adj_x = settings->scroll_start_x;
  int adj_y = settings->scroll_start_y;
  if (y + border_distance >= adj_y + scroll_height)
    {
      v_delta = v_scroll_speed;
    }
  else if (y - border_distance <= adj_y)
    {
      v_delta = -v_scroll_speed;
    }
  if (x + border_distance >= adj_x + scroll_width)
    {
      h_delta = h_scroll_speed;
    }
  else if (x - border_distance <= adj_x)
    {
      h_delta = -h_scroll_speed;
    }

  if (!scroll_h)
    h_delta = 0;
  if (!scroll_v)
    v_delta = 0;

  if (settings->scroll_start_x + h_delta < 0)
    {
      h_delta -= settings->scroll_start_x + h_delta;
    }
  if (settings->scroll_start_y + v_delta < 0)
    {
      v_delta -= settings->scroll_start_y + v_delta;
    }
  editor_settings_append_scroll (settings, h_delta, v_delta, F_VALIDATE);
  self->offset_x_from_scroll += h_delta;
  self->offset_y_from_scroll += v_delta;

  return;
}

/**
 * Called from MainWindowWidget because the
 * events don't reach here.
 */
void
arranger_widget_on_key_release (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  ArrangerWidget *        self)
{
  self->key_is_pressed = 0;

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      self->ctrl_held = 0;
    }

  if (z_gtk_keyval_is_shift (keyval))
    {
      self->shift_held = 0;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      self->alt_held = 0;
    }

  ArrangerSelections * sel = arranger_widget_get_selections (self);

  if (ACTION_IS (STARTING_MOVING))
    {
      if (self->alt_held && self->can_link)
        self->action = UI_OVERLAY_ACTION_MOVING_LINK;
      else if (
        self->ctrl_held && !arranger_selections_contains_unclonable_object (sel))
        self->action = UI_OVERLAY_ACTION_MOVING_COPY;
      else
        self->action = UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (MOVING) && self->alt_held && self->can_link)
    {
      self->action = UI_OVERLAY_ACTION_MOVING_LINK;
    }
  else if (
    ACTION_IS (MOVING) && self->ctrl_held
    && !arranger_selections_contains_unclonable_object (sel))
    {
      self->action = UI_OVERLAY_ACTION_MOVING_COPY;
    }
  else if (ACTION_IS (MOVING_LINK) && !self->alt_held && self->can_link)
    {
      self->action =
        self->ctrl_held ? UI_OVERLAY_ACTION_MOVING_COPY : UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (MOVING_COPY) && !self->ctrl_held)
    {
      self->action = UI_OVERLAY_ACTION_MOVING;
    }

  if (self->type == TYPE (TIMELINE))
    {
      timeline_arranger_widget_set_cut_lines_visible (self);
    }

  /*arranger_widget_update_visibility (self);*/

  arranger_widget_refresh_cursor (self);
}

/**
 * Called from MainWindowWidget because some
 * events don't reach here.
 */
gboolean
arranger_widget_on_key_press (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  ArrangerWidget *        self)
{
  g_debug ("arranger widget key action");

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      self->ctrl_held = 1;
    }

  if (z_gtk_keyval_is_shift (keyval))
    {
      self->shift_held = 1;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      self->alt_held = 1;
    }

  ArrangerSelections * sel = arranger_widget_get_selections (self);
  g_return_val_if_fail (sel, false);

  if (ACTION_IS (STARTING_MOVING))
    {
      if (
        self->ctrl_held && !arranger_selections_contains_unclonable_object (sel))
        self->action = UI_OVERLAY_ACTION_MOVING_COPY;
      else
        self->action = UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (MOVING) && self->alt_held && self->can_link)
    {
      self->action = UI_OVERLAY_ACTION_MOVING_LINK;
    }
  else if (
    ACTION_IS (MOVING) && self->ctrl_held
    && !arranger_selections_contains_unclonable_object (sel))
    {
      self->action = UI_OVERLAY_ACTION_MOVING_COPY;
    }
  else if (ACTION_IS (MOVING_LINK) && !self->alt_held)
    {
      self->action =
        self->ctrl_held ? UI_OVERLAY_ACTION_MOVING_COPY : UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (MOVING_COPY) && !self->ctrl_held)
    {
      self->action = UI_OVERLAY_ACTION_MOVING;
    }
  else if (ACTION_IS (NONE))
    {
      if (arranger_selections_has_any (sel))
        {
          double move_ticks = 0.0;
          if (self->ctrl_held)
            {
              Position tmp;
              position_set_to_bar (&tmp, 2);
              move_ticks = tmp.ticks;
            }
          else
            {
              SnapGrid * sg = arranger_widget_get_snap_grid (self);
              move_ticks = (double) snap_grid_get_snap_ticks (sg);
            }

          /* check arrow movement */
          bool have_arrow_movement = false;
          if (keyval == GDK_KEY_Left)
            {
              have_arrow_movement = true;
              Position min_possible_pos;
              arranger_widget_get_min_possible_position (
                self, &min_possible_pos);

              /* get earliest object */
              ArrangerObject * obj = arranger_selections_get_first_object (sel);

              if (obj->pos.ticks - move_ticks >= min_possible_pos.ticks)
                {
                  GError * err = NULL;
                  bool     ret = arranger_selections_action_perform_move (
                    sel, -move_ticks, 0, 0, 0, 0, 0, NULL, F_NOT_ALREADY_MOVED,
                    &err);
                  if (!ret)
                    {
                      HANDLE_ERROR (err, "%s", _ ("Failed to move selection"));
                    }

                  /* scroll left if needed */
                  arranger_widget_scroll_until_obj (
                    self, obj, 1, 0, 1, SCROLL_PADDING);
                }
            }
          else if (keyval == GDK_KEY_Right)
            {
              have_arrow_movement = true;
              GError * err = NULL;
              bool     ret = arranger_selections_action_perform_move (
                sel, move_ticks, 0, 0, 0, 0, 0, NULL, F_NOT_ALREADY_MOVED, &err);
              if (!ret)
                {
                  HANDLE_ERROR (err, "%s", _ ("Failed to move selection"));
                }

              /* get latest object */
              ArrangerObject * obj =
                arranger_selections_get_last_object (sel, true);

              /* scroll right if needed */
              arranger_widget_scroll_until_obj (
                self, obj, 1, 0, 0, SCROLL_PADDING);
            }
          else if (keyval == GDK_KEY_Down)
            {
              have_arrow_movement = true;
              if (self == MW_MIDI_ARRANGER || self == MW_MIDI_MODIFIER_ARRANGER)
                {
                  int        pitch_delta = 0;
                  MidiNote * mn =
                    midi_arranger_selections_get_lowest_note (MA_SELECTIONS);
                  ArrangerObject * obj = (ArrangerObject *) mn;

                  if (self->ctrl_held)
                    {
                      if (mn->val - 12 >= 0)
                        pitch_delta = -12;
                    }
                  else
                    {
                      if (mn->val - 1 >= 0)
                        pitch_delta = -1;
                    }

                  if (pitch_delta)
                    {
                      GError * err = NULL;
                      bool ret = arranger_selections_action_perform_move_midi (
                        sel, 0, pitch_delta, F_NOT_ALREADY_MOVED, &err);
                      if (!ret)
                        {
                          HANDLE_ERROR (
                            err, "%s",
                            _ ("Failed to move "
                               "selection"));
                        }

                      /* scroll down if needed */
                      arranger_widget_scroll_until_obj (
                        self, obj, 0, 0, 0, SCROLL_PADDING);
                    }
                }
              else if (self == MW_CHORD_ARRANGER)
                {
                  GError * err = NULL;
                  bool     ret = arranger_selections_action_perform_move_chord (
                    sel, 0, 1, F_NOT_ALREADY_MOVED, &err);
                  if (!ret)
                    {
                      HANDLE_ERROR (err, "%s", _ ("Failed to move chords"));
                    }
                }
              else if (self == MW_TIMELINE)
                {
                  /* TODO check if can be moved */
                  /*action =*/
                  /*arranger_selections_action_new_move (*/
                  /*sel, 0, -1, 0, 0, 0);*/
                  /*undo_manager_perform (*/
                  /*UNDO_MANAGER, action);*/
                }
            }
          else if (keyval == GDK_KEY_Up)
            {
              have_arrow_movement = true;
              if (self == MW_MIDI_ARRANGER || self == MW_MIDI_MODIFIER_ARRANGER)
                {
                  int        pitch_delta = 0;
                  MidiNote * mn =
                    midi_arranger_selections_get_highest_note (MA_SELECTIONS);
                  ArrangerObject * obj = (ArrangerObject *) mn;

                  if (self->ctrl_held)
                    {
                      if (mn->val + 12 < 128)
                        pitch_delta = 12;
                    }
                  else
                    {
                      if (mn->val + 1 < 128)
                        pitch_delta = 1;
                    }

                  if (pitch_delta)
                    {
                      GError * err = NULL;
                      bool ret = arranger_selections_action_perform_move_midi (
                        sel, 0, pitch_delta, F_NOT_ALREADY_MOVED, &err);
                      if (!ret)
                        {
                          HANDLE_ERROR (
                            err, "%s",
                            _ ("Failed to move "
                               "selection"));
                        }

                      /* scroll up if needed */
                      arranger_widget_scroll_until_obj (
                        self, obj, 0, 1, 0, SCROLL_PADDING);
                    }
                }
              else if (self == MW_CHORD_ARRANGER)
                {
                  GError * err = NULL;
                  bool     ret = arranger_selections_action_perform_move_chord (
                    sel, 0, -1, F_NOT_ALREADY_MOVED, &err);
                  if (!ret)
                    {
                      HANDLE_ERROR (
                        err, "%s",
                        _ ("Failed to move "
                           "selection"));
                    }
                }
              else if (self == MW_TIMELINE)
                {
                  /* TODO check if can be moved */
                  /*action =*/
                  /*arranger_selections_action_new_move (*/
                  /*sel, 0, 1, 0, 0, 0);*/
                  /*undo_manager_perform (*/
                  /*UNDO_MANAGER, action);*/
                }
            } /* endif key up */

          if (have_arrow_movement && self->type == TYPE (MIDI))
            {
              midi_arranger_listen_notes (self, true);
              /* FIXME remember the ID just in case the
               * arranger gets deleted before this is caleld */
              g_timeout_add (
                400, midi_arranger_unlisten_notes_source_func, self);
            }

        } /* arranger selections has any */
    }

  if (self->type == TYPE (TIMELINE))
    {
      timeline_arranger_widget_set_cut_lines_visible (self);
    }

  arranger_widget_refresh_cursor (self);

  /* if space pressed, allow the shortcut func to be called */
  /* FIXME this whole function should not check
   * for keyvals and return false always */
  if (
    keyval == GDK_KEY_space || (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_6)
    || keyval == GDK_KEY_A || keyval == GDK_KEY_a || keyval == GDK_KEY_M
    || keyval == GDK_KEY_m || keyval == GDK_KEY_Q || keyval == GDK_KEY_q
    || keyval == GDK_KEY_less || keyval == GDK_KEY_Delete
    || keyval == GDK_KEY_greater || keyval == GDK_KEY_F2
    || keyval == GDK_KEY_KP_4 || keyval == GDK_KEY_KP_6 || keyval == GDK_KEY_Tab
    || keyval == GDK_KEY_ISO_Left_Tab)
    {
      g_debug ("ignoring keyval used for shortcuts");
      return false;
    }
  else
    g_debug ("not ignoring keyval %x", keyval);

  /* if key was Esc, cancel any drag and adjust
   * the undo/redo stacks */
  if (
    keyval == GDK_KEY_Escape && gtk_gesture_is_active (GTK_GESTURE (self->drag)))
    {
      UNDO_MANAGER->redo_stack_locked = true;
      UndoableAction * last_action = undo_manager_get_last_action (UNDO_MANAGER);
      gtk_gesture_set_state (
        GTK_GESTURE (self->drag), GTK_EVENT_SEQUENCE_DENIED);
      UndoableAction * new_last_action =
        undo_manager_get_last_action (UNDO_MANAGER);
      if (new_last_action != last_action)
        {
          undo_manager_undo (UNDO_MANAGER, NULL);
        }
      UNDO_MANAGER->redo_stack_locked = false;
    }

  return true;
}

/**
 * Sets the highlight rectangle.
 *
 * @param rect The rectangle with absolute positions, or NULL
 *   to unset/unhighlight.
 */
void
arranger_widget_set_highlight_rect (ArrangerWidget * self, GdkRectangle * rect)
{
  if (rect)
    {
      self->is_highlighted = true;
      /*self->prev_highlight_rect =*/
      /*self->highlight_rect;*/
      self->highlight_rect = *rect;
    }
  else
    {
      self->is_highlighted = false;
    }

  EVENTS_PUSH (ET_ARRANGER_HIGHLIGHT_CHANGED, self);
}

/**
 * On button press.
 *
 * This merely sets the number of clicks and
 * objects clicked on. It is always called
 * before drag_begin, so the logic is done in drag_begin.
 */
static void
click_pressed (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  ArrangerWidget *  self)
{
  g_debug ("arranger click pressed - npress %d", n_press);

  /* set number of presses */
  self->n_press = n_press;

  /* set modifier button states */
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  if (state & GDK_SHIFT_MASK)
    self->shift_held = 1;
  if (state & GDK_CONTROL_MASK)
    self->ctrl_held = 1;

  PROJECT->last_selection =
    self->type == ARRANGER_WIDGET_TYPE_TIMELINE
      ? SELECTION_TYPE_TIMELINE
      : SELECTION_TYPE_EDITOR;
  EVENTS_PUSH (ET_PROJECT_SELECTION_TYPE_CHANGED, NULL);
}

static void
click_stopped (GtkGestureClick * click, ArrangerWidget * self)
{
  g_debug ("arranger click stopped");

  self->n_press = 0;
}

/**
 * Called when an item needs to be created at the
 * given position.
 *
 * @param autofilling Whether this is part of an
 *   autofill action.
 */
void
arranger_widget_create_item (
  ArrangerWidget * self,
  double           start_x,
  double           start_y,
  bool             autofilling)
{
  /* something will be created */
  Position          pos;
  Track *           track = NULL;
  AutomationTrack * at = NULL;
  int               note, chord_index;
  ZRegion *         region = NULL;

  /* get the position */
  arranger_widget_px_to_pos (self, start_x, &pos, F_PADDING);

  /* make sure the position is positive */
  Position init_pos;
  position_init (&init_pos);
  if (position_is_before (&pos, &init_pos))
    {
      position_init (&pos);
    }

  /* snap it */
  if (autofilling || (!self->shift_held && SNAP_GRID_ANY_SNAP (self->snap_grid)))
    {
      Track * track_for_snap = NULL;
      if (self->type == TYPE (TIMELINE))
        {
          track_for_snap =
            timeline_arranger_widget_get_track_at_y (self, start_y);
        }

      SnapGrid * sg = snap_grid_clone (self->snap_grid);
      /* if autofilling, make sure that snapping is enabled */
      if (autofilling)
        {
          sg->snap_to_grid = true;
        }

      position_snap (
        &self->earliest_obj_start_pos, &pos, track_for_snap, NULL, sg);

      snap_grid_free (sg);
    }

  g_message ("creating item at %f,%f", start_x, start_y);

  switch (self->type)
    {
    case TYPE (TIMELINE):
      {
        /* figure out if we are creating a region or
         * automation point */
        at = timeline_arranger_widget_get_at_at_y (self, start_y);
        track = timeline_arranger_widget_get_track_at_y (self, start_y);

        GError * err = NULL;
        bool     success = true;
        if (at) /* if creating automation region */
          {
            success = timeline_arranger_widget_create_region (
              self, REGION_TYPE_AUTOMATION, track, NULL, at, &pos, &err);
          }
        else if (track) /* else if double click inside track */
          {
            TrackLane * lane =
              timeline_arranger_widget_get_track_lane_at_y (self, start_y);
            switch (track->type)
              {
              case TRACK_TYPE_INSTRUMENT:
                success = timeline_arranger_widget_create_region (
                  self, REGION_TYPE_MIDI, track, lane, NULL, &pos, &err);
                break;
              case TRACK_TYPE_MIDI:
                success = timeline_arranger_widget_create_region (
                  self, REGION_TYPE_MIDI, track, lane, NULL, &pos, &err);
                break;
              case TRACK_TYPE_AUDIO:
                break;
              case TRACK_TYPE_MASTER:
                break;
              case TRACK_TYPE_CHORD:
                timeline_arranger_widget_create_chord_or_scale (
                  self, track, start_y, &pos);
                break;
              case TRACK_TYPE_AUDIO_BUS:
                break;
              case TRACK_TYPE_AUDIO_GROUP:
                break;
              case TRACK_TYPE_MARKER:
                timeline_arranger_widget_create_marker (self, track, &pos);
                break;
              default:
                /* TODO */
                break;
              }
          }
        if (!success)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to create region"));
            return;
          }
      }
      break;
    case TYPE (MIDI):
      /* find the note and region at x,y */
      note = piano_roll_keys_widget_get_key_from_y (MW_PIANO_ROLL_KEYS, start_y);
      region = clip_editor_get_region (CLIP_EDITOR);

      /* create a note */
      if (region)
        {
          midi_arranger_widget_create_note (
            self, &pos, note, (ZRegion *) region);
        }
      break;
    case TYPE (MIDI_MODIFIER):
    case TYPE (AUDIO):
      break;
    case TYPE (CHORD):
      /* find the chord and region at x,y */
      chord_index = chord_arranger_widget_get_chord_at_y (start_y);
      region = clip_editor_get_region (CLIP_EDITOR);

      /* create a chord object */
      if (region && chord_index < CHORD_EDITOR->num_chords)
        {
          chord_arranger_widget_create_chord (self, &pos, chord_index, region);
        }
      break;
    case TYPE (AUTOMATION):
      region = clip_editor_get_region (CLIP_EDITOR);

      if (region)
        {
          automation_arranger_widget_create_ap (
            self, &pos, start_y, region, autofilling);
        }
      break;
    }

  if (!autofilling)
    {
      /* set the start selections */
      ArrangerSelections * sel = arranger_widget_get_selections (self);
      g_return_if_fail (sel);
      self->sel_at_start = arranger_selections_clone (sel);
    }
}

/**
 * Called to autofill at the given position.
 *
 * In the case of velocities, this will set the velocity
 * wherever hit.
 *
 * In the case of automation, this will create or edit the
 * automation point at the given position.
 *
 * In other cases, this will create an object with the default
 * length at the given position, unless an object already
 * exists there.
 */
NONNULL static void
autofill (ArrangerWidget * self, double x, double y)
{
  /* make sure values are valid */
  x = MAX (x, 0);
  y = MAX (y, 0);

  /* start autofill if not started yet */
  if (self->action != UI_OVERLAY_ACTION_AUTOFILLING)
    {
      self->action = UI_OVERLAY_ACTION_AUTOFILLING;
      ArrangerSelections * sel = arranger_widget_get_selections (self);
      g_return_if_fail (sel);

      /* clear the actual selections to append
       * created objects */
      arranger_selections_clear (sel, F_NO_FREE, F_NO_PUBLISH_EVENTS);

      /* also clear the selections at start so we
       * can append the affected objects */
      if (self->sel_at_start)
        {
          arranger_selections_clear (
            self->sel_at_start, F_FREE, F_NO_PUBLISH_EVENTS);
        }
      if (!self->sel_at_start)
        {
          self->sel_at_start = arranger_selections_clone (sel);
        }

      ZRegion * clip_editor_region = clip_editor_get_region (CLIP_EDITOR);
      if (clip_editor_region)
        {
          self->region_at_start = (ZRegion *) arranger_object_clone (
            (ArrangerObject *) clip_editor_region);
        }
      else
        {
          self->region_at_start = NULL;
        }
    }

  if (self->type == TYPE (MIDI_MODIFIER))
    {
      midi_modifier_arranger_set_hit_velocity_vals (self, x, y, true);
    }
  else if (self->type == TYPE (AUTOMATION))
    {
      /* move aps or create ap */
      if (!automation_arranger_move_hit_aps (self, x, y))
        {
          arranger_widget_create_item (self, x, y, true);
        }
    }
  else
    {
      ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
        self, ARRANGER_OBJECT_TYPE_ALL, x, y);

      /* don't write over object */
      if (obj)
        {
          g_message (
            "object already exists at %f,%f, "
            "skipping",
            x, y);
          return;
        }
      arranger_widget_create_item (self, x, y, true);
    }
}

static void
drag_cancel (
  GtkGesture *       gesture,
  GdkEventSequence * sequence,
  ArrangerWidget *   self)
{
  g_message ("drag cancelled");
}

/**
 * Sets the start pos of the earliest object and
 * the flag whether the earliest object exists.
 */
NONNULL static void
set_earliest_obj (ArrangerWidget * self)
{
  g_debug ("setting earliest object...");

  ArrangerSelections * sel = arranger_widget_get_selections (self);
  g_return_if_fail (sel);
  if (arranger_selections_has_any (sel))
    {
      arranger_selections_get_start_pos (
        sel, &self->earliest_obj_start_pos, F_GLOBAL);
      self->earliest_obj_exists = 1;
    }
  else
    {
      self->earliest_obj_exists = 0;
    }

  g_debug ("earliest object exists: %d", self->earliest_obj_exists);
}

/**
 * Checks for the first object hit, sets the
 * appropriate action and selects it.
 *
 * @return If an object was handled or not.
 */
static bool
on_drag_begin_handle_hit_object (
  ArrangerWidget * self,
  const double     x,
  const double     y)
{
  ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
    self, ARRANGER_OBJECT_TYPE_ALL, x, y);

  /*(void) filter_out_frozen_objects;*/
  if (!obj || arranger_object_is_frozen (obj))
    {
      return false;
    }

  /* get x,y as local to the object */
  int wx = (int) x - obj->full_rect.x;
  int wy = (int) y - obj->full_rect.y;

  /* remember object and pos */
  self->start_object = obj;
  self->start_pos_px = x;

  /* get flags */
  bool is_fade_in_point = arranger_object_is_fade_in (obj, wx, wy, 1, 0);
  bool is_fade_out_point = arranger_object_is_fade_out (obj, wx, wy, 1, 0);
  bool is_fade_in_outer = arranger_object_is_fade_in (obj, wx, wy, 0, 1);
  bool is_fade_out_outer = arranger_object_is_fade_out (obj, wx, wy, 0, 1);
  bool is_resize_l = arranger_object_is_resize_l (obj, wx);
  bool is_resize_r = arranger_object_is_resize_r (obj, wx);
  bool is_resize_up = arranger_object_is_resize_up (obj, wx, wy);
  bool is_resize_loop = arranger_object_is_resize_loop (obj, wy);
  bool show_cut_lines =
    arranger_object_should_show_cut_lines (obj, self->alt_held);
  bool is_rename = arranger_object_is_rename (obj, wx, wy);
  bool is_selected = arranger_object_is_selected (obj);
  self->start_object_was_selected = is_selected;

  /* select object if unselected */
  switch (P_TOOL)
    {
    case TOOL_SELECT_NORMAL:
    case TOOL_SELECT_STRETCH:
    case TOOL_EDIT:
      if (!is_selected)
        {
          if (self->ctrl_held)
            {
              /* append to selections */
              arranger_object_select (obj, F_SELECT, F_APPEND, F_PUBLISH_EVENTS);
            }
          else
            {
              /* make it the only selection */
              arranger_object_select (
                obj, F_SELECT, F_NO_APPEND, F_PUBLISH_EVENTS);
              g_message ("making only selection");
            }
        }
      break;
    case TOOL_CUT:
      /* only select this object */
      arranger_object_select (obj, F_SELECT, F_NO_APPEND, F_PUBLISH_EVENTS);
      break;
    default:
      break;
    }

  /* set editor region and show editor if double
   * click */
  if (
    obj->type == ARRANGER_OBJECT_TYPE_REGION
    && self->drag_start_btn == GDK_BUTTON_PRIMARY)
    {
      clip_editor_set_region (CLIP_EDITOR, (ZRegion *) obj, F_PUBLISH_EVENTS);

      /* if double click bring up piano roll */
      if (self->n_press == 2 && !self->ctrl_held)
        {
          g_debug (
            "double clicked on region - "
            "showing piano roll");
          EVENTS_PUSH (ET_REGION_ACTIVATED, NULL);
        }
    }
  /* if midi note from a ghosted region set the clip editor
   * region */
  else if (obj->type == ARRANGER_OBJECT_TYPE_MIDI_NOTE)
    {
      ZRegion * cur_r = clip_editor_get_region (CLIP_EDITOR);
      ZRegion * r = arranger_object_get_region (obj);
      if (r != cur_r)
        {
          clip_editor_set_region (CLIP_EDITOR, r, F_PUBLISH_EVENTS);
        }
    }
  /* if open marker dialog if double click on
   * marker */
  else if (obj->type == ARRANGER_OBJECT_TYPE_MARKER)
    {
      if (self->n_press == 2 && !self->ctrl_held)
        {
          StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
            _ ("Marker name"), obj,
            (GenericStringGetter) arranger_object_get_name,
            (GenericStringSetter) arranger_object_set_name_with_action);
          gtk_window_present (GTK_WINDOW (dialog));
          self->action = UI_OVERLAY_ACTION_NONE;
          return true;
        }
    }
  /* if double click on scale, open scale
   * selector */
  else if (obj->type == ARRANGER_OBJECT_TYPE_SCALE_OBJECT)
    {
      if (self->n_press == 2 && !self->ctrl_held)
        {
          ScaleSelectorWindowWidget * scale_selector =
            scale_selector_window_widget_new ((ScaleObject *) obj);
          gtk_window_present (GTK_WINDOW (scale_selector));
          self->action = UI_OVERLAY_ACTION_NONE;
          return true;
        }
    }
  /* if double click on automation point, ask for value */
  else if (obj->type == ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
    {
      if (self->n_press == 2 && !self->ctrl_held)
        {
          StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
            _ ("Automation value"), obj,
            (GenericStringGetter) automation_point_get_fvalue_as_string,
            (GenericStringSetter) automation_point_set_fvalue_with_action);
          gtk_window_present (GTK_WINDOW (dialog));
          self->action = UI_OVERLAY_ACTION_NONE;
          return true;
        }
    }

  /* check if all selected objects are fadeable or
   * resizable */
  if (is_resize_l || is_resize_r)
    {
      ArrangerSelections * sel = arranger_widget_get_selections (self);

      bool have_unresizable = arranger_selections_contains_object_with_property (
        sel, ARRANGER_SELECTIONS_PROPERTY_HAS_LENGTH, false);
      if (have_unresizable)
        {
          ui_show_message_literal (
            _ ("Cannot Resize"),
            _ ("Cannot resize because the "
               "selection contains objects "
               "without length"));
          return false;
        }

      bool have_looped = arranger_selections_contains_object_with_property (
        sel, ARRANGER_SELECTIONS_PROPERTY_HAS_LOOPED, true);
      if ((is_resize_l || is_resize_r) && !is_resize_loop && have_looped)
        {
          bool have_unloopable =
            arranger_selections_contains_object_with_property (
              sel, ARRANGER_SELECTIONS_PROPERTY_CAN_LOOP, false);
          if (have_unloopable)
            {
              /* cancel resize since we have
               * a looped object mixed with
               * unloopable objects in the
               * selection */
              ui_show_message_literal (
                _ ("Cannot Resize"),
                _ ("Cannot resize because the "
                   "selection contains a mix of "
                   "looped and unloopable objects"));
              return false;
            }
          else
            {
              /* loop-resize since we have a
               * loopable object in the selection
               * and all other objects are
               * loopable */
              g_debug (
                "convert resize to resize-loop - "
                "have looped object in the "
                "selection");
              is_resize_loop = true;
            }
        }
    }
  if (
    is_fade_in_point || is_fade_in_outer || is_fade_out_point
    || is_fade_out_outer)
    {
      ArrangerSelections * sel = arranger_widget_get_selections (self);
      bool have_unfadeable = arranger_selections_contains_object_with_property (
        sel, ARRANGER_SELECTIONS_PROPERTY_CAN_FADE, false);
      if (have_unfadeable)
        {
          /* don't fade */
          is_fade_in_point = false;
          is_fade_in_outer = false;
          is_fade_out_point = false;
          is_fade_out_outer = false;
        }
    }

#define SET_ACTION(x) self->action = UI_OVERLAY_ACTION_##x

  g_debug ("action before");
  arranger_widget_print_action (self);

  bool drum_mode = arranger_widget_get_drum_mode_enabled (self);

  /* update arranger action */
  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      switch (P_TOOL)
        {
        case TOOL_ERASER:
          SET_ACTION (STARTING_ERASING);
          break;
        case TOOL_AUDITION:
          SET_ACTION (AUDITIONING);
          break;
        case TOOL_SELECT_NORMAL:
        case TOOL_SELECT_STRETCH:
          if (show_cut_lines)
            SET_ACTION (CUTTING);
          else if (is_fade_in_point)
            SET_ACTION (RESIZING_L_FADE);
          else if (is_fade_out_point)
            SET_ACTION (RESIZING_R_FADE);
          else if (is_resize_l && is_resize_loop)
            SET_ACTION (RESIZING_L_LOOP);
          else if (is_resize_l)
            {
              if (P_TOOL == TOOL_SELECT_NORMAL)
                {
                  SET_ACTION (RESIZING_L);
                }
              else if (P_TOOL == TOOL_SELECT_STRETCH)
                {
                  SET_ACTION (STRETCHING_L);
                }
            }
          else if (is_resize_r && is_resize_loop)
            SET_ACTION (RESIZING_R_LOOP);
          else if (is_resize_r)
            {
              if (P_TOOL == TOOL_SELECT_NORMAL)
                {
                  SET_ACTION (RESIZING_R);
                }
              else if (P_TOOL == TOOL_SELECT_STRETCH)
                {
                  SET_ACTION (STRETCHING_R);
                }
            }
          else if (is_rename)
            SET_ACTION (RENAMING);
          else if (is_fade_in_outer)
            SET_ACTION (RESIZING_UP_FADE_IN);
          else if (is_fade_out_outer)
            SET_ACTION (RESIZING_UP_FADE_OUT);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_EDIT:
          if (is_resize_l)
            SET_ACTION (RESIZING_L);
          else if (is_resize_r)
            SET_ACTION (RESIZING_R);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_CUT:
          SET_ACTION (CUTTING);
          break;
        case TOOL_RAMP:
          /* TODO */
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      switch (P_TOOL)
        {
        case TOOL_ERASER:
          SET_ACTION (STARTING_ERASING);
          break;
        case TOOL_AUDITION:
          SET_ACTION (AUDITIONING);
          break;
        case TOOL_SELECT_NORMAL:
        case TOOL_EDIT:
        case TOOL_SELECT_STRETCH:
          if ((is_resize_l) && !drum_mode)
            SET_ACTION (RESIZING_L);
          else if (is_resize_r && !drum_mode)
            SET_ACTION (RESIZING_R);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_CUT:
          /* TODO */
          break;
        case TOOL_RAMP:
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        case TOOL_EDIT:
        case TOOL_SELECT_STRETCH:
          if (is_resize_up)
            SET_ACTION (RESIZING_UP);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        default:
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_VELOCITY:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        case TOOL_EDIT:
        case TOOL_SELECT_STRETCH:
        case TOOL_RAMP:
          SET_ACTION (STARTING_MOVING);
          if (is_resize_up)
            SET_ACTION (RESIZING_UP);
          else
            SET_ACTION (NONE);
          break;
        default:
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        case TOOL_EDIT:
        case TOOL_SELECT_STRETCH:
          SET_ACTION (STARTING_MOVING);
          break;
        default:
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_SCALE_OBJECT:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        case TOOL_EDIT:
        case TOOL_SELECT_STRETCH:
          SET_ACTION (STARTING_MOVING);
          break;
        default:
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_MARKER:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        case TOOL_EDIT:
        case TOOL_SELECT_STRETCH:
          SET_ACTION (STARTING_MOVING);
          break;
        default:
          break;
        }
      break;
    default:
      g_return_val_if_reached (0);
    }

  g_debug ("action after");
  arranger_widget_print_action (self);

#undef SET_ACTION

  ArrangerSelections * orig_selections = arranger_widget_get_selections (self);

  /* set index in prev lane for selected objects
   * if timeline */
  if (self->type == ARRANGER_WIDGET_TYPE_TIMELINE)
    {
      timeline_selections_set_index_in_prev_lane (TL_SELECTIONS);
    }

  /* clone the arranger selections at this point */
  self->sel_at_start = arranger_selections_clone (orig_selections);

  /* if the action is stretching, set the
   * "before_length" on each region */
  if (
    orig_selections->type == ARRANGER_SELECTIONS_TYPE_TIMELINE
    && ACTION_IS (STRETCHING_R))
    {
      TimelineSelections * sel = (TimelineSelections *) orig_selections;
      transport_prepare_audio_regions_for_stretch (TRANSPORT, sel);
    }

  return true;
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  ArrangerWidget * self)
{
  g_debug ("arranger drag begin starting...");

  self->offset_x_from_scroll = 0;
  self->offset_y_from_scroll = 0;

  const EditorSettings settings =
    arranger_widget_get_editor_setting_values (self);
  start_x += settings.scroll_start_x;
  start_y += settings.scroll_start_y;

  self->start_x = start_x;
  self->hover_x = start_x;
  arranger_widget_px_to_pos (self, start_x, &self->start_pos, F_PADDING);
  self->start_y = start_y;
  self->hover_y = start_y;
  ;
  self->drag_update_started = false;

  /* set last project selection type */
  if (self->type == ARRANGER_WIDGET_TYPE_TIMELINE)
    {
      PROJECT->last_selection = SELECTION_TYPE_TIMELINE;
    }
  else
    {
      PROJECT->last_selection = SELECTION_TYPE_EDITOR;
    }

  self->drag_start_btn =
    gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));
  switch (self->drag_start_btn)
    {
    case GDK_BUTTON_PRIMARY:
      g_debug ("primary button clicked");
      break;
    case GDK_BUTTON_SECONDARY:
      g_debug ("secondary button clicked");
      break;
    case GDK_BUTTON_MIDDLE:
      g_debug ("middle button clicked");
      break;
    default:
      break;
    }
  g_warn_if_fail (self->drag_start_btn);

  /* check if selections can create links */
  self->can_link =
    TYPE_IS (TIMELINE) && TL_SELECTIONS->num_regions > 0
    && TL_SELECTIONS->num_scale_objects == 0 && TL_SELECTIONS->num_markers == 0;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  /* get current pos */
  arranger_widget_px_to_pos (self, self->start_x, &self->curr_pos, F_PADDING);

  /* get difference with drag start pos */
  self->curr_ticks_diff_from_start =
    position_get_ticks_diff (&self->curr_pos, &self->start_pos, NULL);

  /* handle hit object */
  int objects_hit = on_drag_begin_handle_hit_object (self, start_x, start_y);
  g_message ("objects hit %d", objects_hit);
  arranger_widget_print_action (self);

  if (objects_hit)
    {
      ArrangerSelections * sel = arranger_widget_get_selections (self);
      self->sel_at_start = arranger_selections_clone (sel);

      if (self->type == TYPE (MIDI))
        {
          midi_arranger_listen_notes (self, true);
        }
    }
  /* if nothing hit */
  else
    {
      self->sel_at_start = NULL;

      g_debug ("npress = %d", self->n_press);

      /* single click */
      if (self->n_press == 1)
        {
          switch (P_TOOL)
            {
            case TOOL_SELECT_NORMAL:
            case TOOL_SELECT_STRETCH:
              if (self->drag_start_btn == GDK_BUTTON_MIDDLE || self->alt_held)
                {
                  self->action = UI_OVERLAY_ACTION_STARTING_PANNING;
                }
              else
                {
                  /* selection */
                  self->action = UI_OVERLAY_ACTION_STARTING_SELECTION;

                  if (!self->ctrl_held)
                    {
                      /* deselect all */
                      arranger_widget_select_all (self, false, true);
                    }

                  /* set whether selecting
                   * objects or selecting range */
                  set_select_type (self, start_y);

                  /* hide range selection */
                  transport_set_has_range (TRANSPORT, false);

                  /* hide range selection if audio
                   * arranger and set appropriate
                   * action */
                  if (self->type == TYPE (AUDIO))
                    {
                      AUDIO_SELECTIONS->has_selection = false;
                      self->action =
                        audio_arranger_widget_get_action_on_drag_begin (self);
                    }
                }
              break;
            case TOOL_EDIT:
              if (
                self->type == TYPE (TIMELINE) || self->type == TYPE (MIDI)
                || self->type == TYPE (CHORD))
                {
                  if (self->ctrl_held)
                    {
                      /* autofill */
                      autofill (self, start_x, start_y);
                    }
                  else
                    {
                      /* something is created */
                      arranger_widget_create_item (
                        self, start_x, start_y, false);
                    }
                }
              else if (
                self->type == TYPE (MIDI_MODIFIER)
                || self->type == TYPE (AUTOMATION))
                {
                  /* autofill (also works for
                   * manual edit for velocity and
                   * automation */
                  autofill (self, start_x, start_y);
                }
              break;
            case TOOL_ERASER:
              /* delete selection */
              self->action = UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
              break;
            case TOOL_RAMP:
              self->action = UI_OVERLAY_ACTION_STARTING_RAMP;
              break;
            case TOOL_AUDITION:
              self->action = UI_OVERLAY_ACTION_STARTING_AUDITIONING;
              self->was_paused = TRANSPORT_IS_PAUSED;
              position_set_to_pos (&self->playhead_pos_at_start, PLAYHEAD);
              transport_set_playhead_pos (TRANSPORT, &self->start_pos);
              transport_request_roll (TRANSPORT, true);
            default:
              break;
            }
        }
      /* double click */
      else if (self->n_press == 2)
        {
          switch (P_TOOL)
            {
            case TOOL_SELECT_NORMAL:
            case TOOL_SELECT_STRETCH:
            case TOOL_EDIT:
              arranger_widget_create_item (self, start_x, start_y, false);
              break;
            case TOOL_ERASER:
              /* delete selection */
              self->action = UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
              break;
            default:
              break;
            }
        }
    }

  /* set the start pos of the earliest object and
   * the flag whether the earliest object exists */
  set_earliest_obj (self);

  arranger_widget_refresh_cursor (self);

  g_debug ("arranger drag begin done");
}

/**
 * Selects objects for the given arranger in the
 * range from start_* to offset_*.
 *
 * @param ignore_frozen Ignore frozen objects.
 * @param[in] delete Whether this is a select-delete
 *   operation.
 * @param[in] in_range Whether to select/delete
 *   objects in the range or exactly at the current
 *   point.
 */
NONNULL static void
select_in_range (
  ArrangerWidget * self,
  double           offset_x,
  double           offset_y,
  bool             in_range,
  bool             ignore_frozen,
  bool delete)
{
  ArrangerSelections * arranger_sel = arranger_widget_get_selections (self);
  g_return_if_fail (arranger_sel);
  ArrangerSelections * prev_sel = arranger_selections_clone (arranger_sel);

  if (delete &&in_range)
    {
      GPtrArray * objs_arr = g_ptr_array_new_full (200, NULL);
      arranger_selections_get_all_objects (self->sel_to_delete, objs_arr);
      if (ignore_frozen)
        {
          filter_out_frozen_objects (self, objs_arr);
        }
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          obj->deleted_temporarily = false;
        }
      arranger_selections_clear (
        self->sel_to_delete, F_NO_FREE, F_NO_PUBLISH_EVENTS);
      g_ptr_array_unref (objs_arr);
    }
  else if (!delete)
    {
      if (!self->ctrl_held)
        {
          /* deselect all */
          arranger_widget_select_all (self, false, false);
        }
    }

  GPtrArray *  objs_arr = g_ptr_array_new_full (200, NULL);
  GdkRectangle rect;
  if (in_range)
    {
      GdkRectangle _rect = {
        .x =
          (int) offset_x >= 0
            ? (int) self->start_x
            : (int) (self->start_x + offset_x),
        .y =
          (int) offset_y >= 0
            ? (int) self->start_y
            : (int) (self->start_y + offset_y),
        .width = abs ((int) offset_x),
        .height = abs ((int) offset_y),
      };
      rect = _rect;
    }
  else
    {
      GdkRectangle _rect = {
        .x = (int) (self->start_x + offset_x),
        .y = (int) (self->start_y + offset_y),
        .width = 1,
        .height = 1,
      };
      rect = _rect;
    }

  /* redraw union of last rectangle and new
   * rectangle */
  GdkRectangle union_rect;
  gdk_rectangle_union (&rect, &self->last_selection_rect, &union_rect);
  self->last_selection_rect = rect;

  switch (self->type)
    {
    case TYPE (CHORD):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_CHORD_OBJECT, &rect, objs_arr);
      if (ignore_frozen)
        {
          filter_out_frozen_objects (self, objs_arr);
        }
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          if (delete)
            {
              arranger_selections_add_object (self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
            }
        }
      break;
    case TYPE (AUTOMATION):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_AUTOMATION_POINT, &rect, objs_arr);
      if (ignore_frozen)
        {
          filter_out_frozen_objects (self, objs_arr);
        }
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          if (delete)
            {
              arranger_selections_add_object (self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
            }
        }
      break;
    case TYPE (TIMELINE):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_REGION, &rect, objs_arr);
      if (ignore_frozen)
        {
          filter_out_frozen_objects (self, objs_arr);
        }
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          if (delete)
            {
              arranger_selections_add_object (self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              /* select the enclosed region */
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
            }
        }

      g_ptr_array_remove_range (objs_arr, 0, objs_arr->len);
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_SCALE_OBJECT, &rect, objs_arr);
      if (ignore_frozen)
        {
          filter_out_frozen_objects (self, objs_arr);
        }
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          if (delete)
            {
              arranger_selections_add_object (self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
            }
        }

      g_ptr_array_remove_range (objs_arr, 0, objs_arr->len);
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_MARKER, &rect, objs_arr);
      if (ignore_frozen)
        {
          filter_out_frozen_objects (self, objs_arr);
        }
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          if (delete)
            {
              if (arranger_object_is_deletable (obj))
                {
                  arranger_selections_add_object (self->sel_to_delete, obj);
                  obj->deleted_temporarily = true;
                }
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
            }
        }
      break;
    case TYPE (MIDI):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_MIDI_NOTE, &rect, objs_arr);
      if (ignore_frozen)
        {
          filter_out_frozen_objects (self, objs_arr);
        }
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          if (delete)
            {
              arranger_selections_add_object (self->sel_to_delete, obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
            }
        }
      midi_arranger_selections_unlisten_note_diff (
        (MidiArrangerSelections *) prev_sel,
        (MidiArrangerSelections *) arranger_widget_get_selections (self));
      break;
    case TYPE (MIDI_MODIFIER):
      arranger_widget_get_hit_objects_in_rect (
        self, ARRANGER_OBJECT_TYPE_VELOCITY, &rect, objs_arr);
      if (ignore_frozen)
        {
          filter_out_frozen_objects (self, objs_arr);
        }
      for (size_t i = 0; i < objs_arr->len; i++)
        {
          ArrangerObject * obj =
            (ArrangerObject *) g_ptr_array_index (objs_arr, i);
          Velocity *       vel = (Velocity *) obj;
          MidiNote *       mn = velocity_get_midi_note (vel);
          ArrangerObject * mn_obj = (ArrangerObject *) mn;

          if (delete)
            {
              arranger_selections_add_object (self->sel_to_delete, mn_obj);
              obj->deleted_temporarily = true;
            }
          else
            {
              arranger_object_select (
                mn_obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
            }
        }
      break;
    default:
      break;
    }

  g_ptr_array_unref (objs_arr);

  if (prev_sel)
    {
      arranger_selections_free_full (prev_sel);
    }
}

static void
pan (ArrangerWidget * self, double offset_x, double offset_y)
{
  offset_x -= self->last_offset_x;
  offset_y -= self->last_offset_y;
  g_message ("panning %f %f", offset_x, offset_y);

  /* pan */
  EditorSettings * settings = arranger_widget_get_editor_settings (self);
  editor_settings_append_scroll (
    settings, (int) -offset_x, (int) -offset_y, F_VALIDATE);

  /* these are also affected */
  self->last_offset_x = MAX (0, self->last_offset_x - offset_x);
  self->hover_x = MAX (0, self->hover_x - offset_x);
  self->start_x = MAX (0, self->start_x - offset_x);
  self->last_offset_y = MAX (0, self->last_offset_y - offset_y);
  self->hover_y = MAX (0, self->hover_y - offset_y);
  self->start_y = MAX (0, self->start_y - offset_y);
}

NONNULL static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ArrangerWidget * self)
{
  offset_x += self->offset_x_from_scroll;
  offset_y += self->offset_y_from_scroll;

  if (
    !self->drag_update_started
    && !gtk_drag_check_threshold (
      GTK_WIDGET (self), (int) self->start_x, (int) self->start_y,
      (int) (self->start_x + offset_x), (int) (self->start_y + offset_y)))
    {
      return;
    }

  self->drag_update_started = true;

  ArrangerSelections * sel = arranger_widget_get_selections (self);

  /* state mask needs to be updated */
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  if (state & GDK_SHIFT_MASK)
    self->shift_held = 1;
  else
    self->shift_held = 0;
  if (state & GDK_CONTROL_MASK)
    self->ctrl_held = 1;
  else
    self->ctrl_held = 0;

  arranger_widget_print_action (self);

  /* get current pos */
  arranger_widget_px_to_pos (
    self, self->start_x + offset_x, &self->curr_pos, F_PADDING);

  /* get difference with drag start pos */
  self->curr_ticks_diff_from_start =
    position_get_ticks_diff (&self->curr_pos, &self->start_pos, NULL);

  if (self->earliest_obj_exists)
    {
      /* add diff to the earliest object's start pos
       * and snap it, then get the diff ticks */
      Position earliest_obj_new_pos;
      position_set_to_pos (&earliest_obj_new_pos, &self->earliest_obj_start_pos);
      position_add_ticks (
        &earliest_obj_new_pos, self->curr_ticks_diff_from_start);

      if (position_is_before_or_equal (&earliest_obj_new_pos, &POSITION_START))
        {
          /* stop at 0.0.0.0 */
          position_set_to_pos (&earliest_obj_new_pos, &POSITION_START);
        }
      else if (!self->shift_held && SNAP_GRID_ANY_SNAP (self->snap_grid))
        {
          Track * track_for_snap = NULL;
          if (self->type == TYPE (TIMELINE))
            {
              track_for_snap = timeline_arranger_widget_get_track_at_y (
                self, self->start_y + offset_y);
            }
          position_snap (
            &self->earliest_obj_start_pos, &earliest_obj_new_pos,
            track_for_snap, NULL, self->snap_grid);
        }
      self->adj_ticks_diff = position_get_ticks_diff (
        &earliest_obj_new_pos, &self->earliest_obj_start_pos, NULL);
    }

  /* if right clicking, start erasing action */
  if (
    self->drag_start_btn == GDK_BUTTON_SECONDARY && P_TOOL == TOOL_SELECT_NORMAL
    && self->action != UI_OVERLAY_ACTION_STARTING_ERASING
    && self->action != UI_OVERLAY_ACTION_ERASING)
    {
      self->action = UI_OVERLAY_ACTION_STARTING_ERASING;
    }

  /* set action to selecting if starting
   * selection. this
   * is because drag_update never gets called if
   * it's just
   * a click, so we can check at drag_end and see if
   * anything was selected */
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      self->action = UI_OVERLAY_ACTION_SELECTING;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
      self->action = UI_OVERLAY_ACTION_DELETE_SELECTING;
      {
        arranger_selections_clear (sel, F_NO_FREE, F_NO_PUBLISH_EVENTS);
        self->sel_to_delete = arranger_selections_clone (sel);
      }
      break;
    case UI_OVERLAY_ACTION_STARTING_ERASING:
      self->action = UI_OVERLAY_ACTION_ERASING;
      {
        arranger_selections_clear (sel, F_NO_FREE, F_NO_PUBLISH_EVENTS);
        self->sel_to_delete = arranger_selections_clone (sel);
      }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      if (self->alt_held && self->can_link)
        self->action = UI_OVERLAY_ACTION_MOVING_LINK;
      else if (
        self->ctrl_held && !arranger_selections_contains_unclonable_object (sel))
        self->action = UI_OVERLAY_ACTION_MOVING_COPY;
      else
        self->action = UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_STARTING_PANNING:
      self->action = UI_OVERLAY_ACTION_PANNING;
      break;
    case UI_OVERLAY_ACTION_MOVING:
      if (self->alt_held && self->can_link)
        self->action = UI_OVERLAY_ACTION_MOVING_LINK;
      else if (
        self->ctrl_held && !arranger_selections_contains_unclonable_object (sel))
        self->action = UI_OVERLAY_ACTION_MOVING_COPY;
      break;
    case UI_OVERLAY_ACTION_MOVING_LINK:
      if (!self->alt_held)
        self->action =
          self->ctrl_held
            ? UI_OVERLAY_ACTION_MOVING_COPY
            : UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
      if (!self->ctrl_held)
        self->action =
          self->alt_held && self->can_link
            ? UI_OVERLAY_ACTION_MOVING_LINK
            : UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_STARTING_RAMP:
      self->action = UI_OVERLAY_ACTION_RAMPING;
      if (self->type == TYPE (MIDI_MODIFIER))
        {
          midi_modifier_arranger_widget_set_start_vel (self);
        }
      break;
    case UI_OVERLAY_ACTION_CUTTING:
      /* alt + move changes the action from
       * cutting to moving-link */
      if (self->alt_held && self->can_link)
        self->action = UI_OVERLAY_ACTION_MOVING_LINK;
      break;
    case UI_OVERLAY_ACTION_STARTING_AUDITIONING:
      self->action = UI_OVERLAY_ACTION_AUDITIONING;
    default:
      break;
    }

  /* update visibility */
  /*arranger_widget_update_visibility (self);*/

  switch (self->action)
    {
    /* if drawing a selection */
    case UI_OVERLAY_ACTION_SELECTING:
      {
        /* find and select objects inside selection */
        select_in_range (
          self, offset_x, offset_y, F_IN_RANGE, F_IGNORE_FROZEN, F_NO_DELETE);

        /* redraw new selections and other needed
         * things */
        EVENTS_PUSH (ET_SELECTING_IN_ARRANGER, self);
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      /* find and delete objects inside
       * selection */
      select_in_range (
        self, offset_x, offset_y, F_IN_RANGE, F_IGNORE_FROZEN, F_DELETE);
      EVENTS_PUSH (ET_SELECTING_IN_ARRANGER, self);
      break;
    case UI_OVERLAY_ACTION_ERASING:
      /* delete anything touched */
      select_in_range (
        self, offset_x, offset_y, F_NOT_IN_RANGE, F_IGNORE_FROZEN, F_DELETE);
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_FADE:
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
      /* snap selections based on new pos */
      if (self->type == TYPE (TIMELINE))
        {
          int ret = timeline_arranger_widget_snap_regions_l (
            self, &self->curr_pos, F_DRY_RUN);
          if (!ret)
            timeline_arranger_widget_snap_regions_l (
              self, &self->curr_pos, F_NOT_DRY_RUN);
        }
      else if (self->type == TYPE (AUDIO))
        {
          int ret = audio_arranger_widget_snap_fade (
            self, &self->curr_pos, true, F_DRY_RUN);
          if (!ret)
            audio_arranger_widget_snap_fade (
              self, &self->curr_pos, true, F_NOT_DRY_RUN);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
    case UI_OVERLAY_ACTION_STRETCHING_L:
      {
        /* snap selections based on new pos */
        if (self->type == TYPE (TIMELINE))
          {
            int ret = timeline_arranger_widget_snap_regions_l (
              self, &self->curr_pos, 1);
            if (!ret)
              timeline_arranger_widget_snap_regions_l (self, &self->curr_pos, 0);
          }
        else if (self->type == TYPE (MIDI))
          {
            int ret =
              midi_arranger_widget_snap_midi_notes_l (self, &self->curr_pos, 1);
            if (!ret)
              midi_arranger_widget_snap_midi_notes_l (self, &self->curr_pos, 0);
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_FADE:
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      if (self->type == TYPE (TIMELINE))
        {
          if (self->resizing_range)
            timeline_arranger_widget_snap_range_r (self, &self->curr_pos);
          else
            {
              int ret = timeline_arranger_widget_snap_regions_r (
                self, &self->curr_pos, F_DRY_RUN);
              if (!ret)
                timeline_arranger_widget_snap_regions_r (
                  self, &self->curr_pos, F_NOT_DRY_RUN);
            }
        }
      else if (self->type == TYPE (AUDIO))
        {
          int ret = audio_arranger_widget_snap_fade (
            self, &self->curr_pos, false, F_DRY_RUN);
          if (!ret)
            audio_arranger_widget_snap_fade (
              self, &self->curr_pos, false, F_NOT_DRY_RUN);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_STRETCHING_R:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      {
        if (self->type == TYPE (TIMELINE))
          {
            if (self->resizing_range)
              {
                timeline_arranger_widget_snap_range_r (self, &self->curr_pos);
              }
            else
              {
                int ret = timeline_arranger_widget_snap_regions_r (
                  self, &self->curr_pos, F_DRY_RUN);
                if (!ret)
                  {
                    timeline_arranger_widget_snap_regions_r (
                      self, &self->curr_pos, F_NOT_DRY_RUN);
                  }
              }
          }
        else if (self->type == TYPE (MIDI))
          {
            int ret = midi_arranger_widget_snap_midi_notes_r (
              self, &self->curr_pos, F_DRY_RUN);
            if (!ret)
              {
                midi_arranger_widget_snap_midi_notes_r (
                  self, &self->curr_pos, F_NOT_DRY_RUN);
              }
            move_items_y (self, offset_y);
          }
        else if (self->type == TYPE (AUDIO))
          {
            if (self->resizing_range)
              {
                audio_arranger_widget_snap_range_r (self, &self->curr_pos);
              }
          }

        transport_recalculate_total_bars (TRANSPORT, sel);
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      if (self->type == TYPE (MIDI_MODIFIER))
        {
          midi_modifier_arranger_widget_resize_velocities (self, offset_y);
        }
      else if (self->type == TYPE (AUTOMATION))
        {
          automation_arranger_widget_resize_curves (self, offset_y);
        }
      else if (self->type == TYPE (AUDIO))
        {
          audio_arranger_widget_update_gain (self, offset_y);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN:
      if (self->type == TYPE (TIMELINE))
        {
          timeline_arranger_widget_fade_up (self, offset_y, true);
        }
      else if (self->type == TYPE (AUDIO))
        {
          audio_arranger_widget_fade_up (self, offset_y, true);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT:
      if (self->type == TYPE (TIMELINE))
        {
          timeline_arranger_widget_fade_up (self, offset_y, false);
        }
      else if (self->type == TYPE (AUDIO))
        {
          audio_arranger_widget_fade_up (self, offset_y, false);
        }
      break;
    case UI_OVERLAY_ACTION_MOVING:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      move_items_x (self, self->adj_ticks_diff - self->last_adj_ticks_diff);
      move_items_y (self, offset_y);
      break;
    case UI_OVERLAY_ACTION_PANNING:
      pan (self, offset_x, offset_y);
      break;
    case UI_OVERLAY_ACTION_AUTOFILLING:
      g_message ("autofilling");
      autofill (self, self->start_x + offset_x, self->start_y + offset_y);
      break;
    case UI_OVERLAY_ACTION_AUDITIONING:
      g_debug ("auditioning");
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      /* find and select objects inside selection */
      if (self->type == TYPE (MIDI_MODIFIER))
        {
          midi_modifier_arranger_widget_ramp (self, offset_x, offset_y);
        }
      break;
    case UI_OVERLAY_ACTION_CUTTING:
      /* nothing to do, wait for drag end */
      break;
    default:
      /* TODO */
      break;
    }

  if (self->action != UI_OVERLAY_ACTION_NONE)
    auto_scroll (
      self, (int) (self->start_x + offset_x), (int) (self->start_y + offset_y));

  if (self->type == TYPE (MIDI))
    {
      midi_arranger_listen_notes (self, 1);
    }

  /* update last offsets */
  self->last_offset_x = offset_x;
  self->last_offset_y = offset_y;
  self->last_adj_ticks_diff = self->adj_ticks_diff;

  arranger_widget_refresh_cursor (self);
}

/**
 * To be called on drag_end() to handle erase
 * actions.
 */
static void
handle_erase_action (ArrangerWidget * self)
{
  if (self->sel_to_delete)
    {
      if (
        arranger_selections_has_any (self->sel_to_delete)
        && !arranger_selections_contains_undeletable_object (self->sel_to_delete))
        {
          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_delete (
            self->sel_to_delete, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to delete selection"));
            }
        }
      object_free_w_func_and_null (
        arranger_selections_free, self->sel_to_delete);
    }
}

static void
on_drag_end_automation (ArrangerWidget * self)
{
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_UP:
      {
        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_edit (
          self->sel_at_start, (ArrangerSelections *) AUTOMATION_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE, F_ALREADY_EDITED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to edit selection"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      {
        /* if something was clicked with ctrl without
         * moving*/
        if (self->ctrl_held)
          {
            /*if (self->start_region &&*/
            /*self->start_region_was_selected)*/
            /*{*/
            /*[> deselect it <]*/
            /*[>ARRANGER_WIDGET_SELECT_REGION (<]*/
            /*[>self, self->start_region,<]*/
            /*[>F_NO_SELECT, F_APPEND);<]*/
            /*}*/
          }
        else if (self->n_press == 2)
          {
            /* double click on object */
            /*g_message ("DOUBLE CLICK");*/
          }
      }
      break;
    case UI_OVERLAY_ACTION_MOVING:
      {
        AutomationSelections * sel_at_start =
          (AutomationSelections *) self->sel_at_start;
        AutomationPoint * start_ap = sel_at_start->automation_points[0];
        ArrangerObject *  start_obj = (ArrangerObject *) start_ap;
        AutomationPoint * ap = AUTOMATION_SELECTIONS->automation_points[0];
        ArrangerObject *  obj = (ArrangerObject *) ap;
        double            ticks_diff = obj->pos.ticks - start_obj->pos.ticks;
        double            norm_value_diff =
          (double) (ap->normalized_val - start_ap->normalized_val);
        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_move_automation (
          AUTOMATION_SELECTIONS, ticks_diff, norm_value_diff, F_ALREADY_MOVED,
          &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to move automation"));
          }
      }
      break;
      /* if copy-moved */
    case UI_OVERLAY_ACTION_MOVING_COPY:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;
        float  value_diff =
          ((AutomationPoint *) obj)->normalized_val
          - ((AutomationPoint *) obj->transient)->normalized_val;

        GError * err = NULL;
        bool     ret = (UndoableAction *)
          arranger_selections_action_perform_duplicate_automation (
            (ArrangerSelections *) AUTOMATION_SELECTIONS, ticks_diff,
            value_diff, F_ALREADY_MOVED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to duplicate automation"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_NONE:
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      {
        arranger_selections_clear (
          (ArrangerSelections *) AUTOMATION_SELECTIONS, F_NO_FREE,
          F_NO_PUBLISH_EVENTS);
      }
      break;
    /* if something was created */
    case UI_OVERLAY_ACTION_CREATING_MOVING:
      {
        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_create (
          AUTOMATION_SELECTIONS, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to create objects"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      handle_erase_action (self);
      break;
    case UI_OVERLAY_ACTION_AUTOFILLING:
      {
        ZRegion * region = clip_editor_get_region (CLIP_EDITOR);

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_automation_fill (
          self->region_at_start, region, true, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to fill automation"));
          }
      }
      break;
    /* if didn't click on something */
    default:
      break;
    }

  self->start_object = NULL;
}

static void
on_drag_end_midi_modifier (ArrangerWidget * self)
{
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_UP:
      {
        g_return_if_fail (self->sel_at_start);

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_edit (
          self->sel_at_start, (ArrangerSelections *) MA_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE, true, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to edit selections"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      {
        Position selection_start_pos, selection_end_pos;
        ui_px_to_pos_editor (
          self->start_x,
          self->last_offset_x >= 0 ? &selection_start_pos : &selection_end_pos,
          F_PADDING);
        ui_px_to_pos_editor (
          self->start_x + self->last_offset_x,
          self->last_offset_x >= 0 ? &selection_end_pos : &selection_start_pos,
          F_PADDING);

        /* prepare the velocities in cloned
         * arranger selections from the
         * vels at start */
        midi_modifier_arranger_widget_select_vels_in_range (
          self, self->last_offset_x);
        self->sel_at_start =
          arranger_selections_clone ((ArrangerSelections *) MA_SELECTIONS);
        MidiArrangerSelections * sel_at_start =
          (MidiArrangerSelections *) self->sel_at_start;
        for (int i = 0; i < sel_at_start->num_midi_notes; i++)
          {
            MidiNote * mn = sel_at_start->midi_notes[i];
            Velocity * vel = mn->vel;
            vel->vel = vel->vel_at_start;
          }

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_edit (
          self->sel_at_start, (ArrangerSelections *) MA_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE, true, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to edit selections"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      handle_erase_action (self);
      break;
    case UI_OVERLAY_ACTION_AUTOFILLING:
      if (arranger_selections_has_any ((ArrangerSelections *) MA_SELECTIONS))
        {
          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_edit (
            self->sel_at_start, (ArrangerSelections *) MA_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE, true, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to edit selections"));
            }
        }
      break;
    default:
      break;
    }
}

static void
on_drag_end_midi (ArrangerWidget * self)
{
  midi_arranger_listen_notes (self, 0);

  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_L:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_resize (
          (ArrangerSelections *) MA_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_L, ticks_diff, F_ALREADY_EDITED,
          &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to resize objects"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        MidiNote *       mn = (MidiNote *) obj;
        MidiNote *       mn_trans = (MidiNote *) obj->transient;
        int              pitch_diff = mn->val - mn_trans->val;
        double ticks_diff = obj->end_pos.ticks - obj->transient->end_pos.ticks;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_resize (
          (ArrangerSelections *) MA_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_R, ticks_diff, F_ALREADY_EDITED,
          &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to resize objects"));
          }
        else if (pitch_diff)
          {
            ret = arranger_selections_action_perform_move_midi (
              MA_SELECTIONS, 0, pitch_diff, F_ALREADY_MOVED, &err);
            if (!ret)
              {
                HANDLE_ERROR (err, "%s", _ ("Failed to move MIDI notes"));
              }
            else
              {
                UndoableAction * ua =
                  undo_manager_get_last_action (UNDO_MANAGER);
                ua->num_actions = 2;
              }
          }
      }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      {
        /* if something was clicked with ctrl without
         * moving*/
        if (self->ctrl_held)
          {
            if (self->start_object && self->start_object_was_selected)
              {
                /* deselect it */
                arranger_object_select (
                  self->start_object, F_NO_SELECT, F_APPEND,
                  F_NO_PUBLISH_EVENTS);
              }
          }
      }
      break;
    case UI_OVERLAY_ACTION_MOVING:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;
        int    pitch_diff =
          ((MidiNote *) obj)->val - ((MidiNote *) obj->transient)->val;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_move_midi (
          MA_SELECTIONS, ticks_diff, pitch_diff, F_ALREADY_MOVED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to move MIDI notes"));
          }
      }
      break;
    /* if copy/link-moved */
    case UI_OVERLAY_ACTION_MOVING_COPY:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;
        int    pitch_diff =
          ((MidiNote *) obj)->val - ((MidiNote *) obj->transient)->val;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_duplicate_midi (
          (ArrangerSelections *) MA_SELECTIONS, ticks_diff, pitch_diff,
          F_ALREADY_MOVED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to duplicate MIDI notes"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_NONE:
      {
        arranger_selections_clear (
          (ArrangerSelections *) MA_SELECTIONS, F_NO_FREE, F_NO_PUBLISH_EVENTS);
      }
      break;
    /* something was created */
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
    case UI_OVERLAY_ACTION_AUTOFILLING:
      {
        GError * err = NULL;
        bool     ret =
          arranger_selections_action_perform_create (MA_SELECTIONS, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to create MIDI notes"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      handle_erase_action (self);
      break;
    /* if didn't click on something */
    default:
      {
      }
      break;
    }

  self->start_object = NULL;
  /*if (self->start_midi_note_clone)*/
  /*{*/
  /*midi_note_free (self->start_midi_note_clone);*/
  /*self->start_midi_note_clone = NULL;*/
  /*}*/

  EVENTS_PUSH (ET_ARRANGER_SELECTIONS_CHANGED, MA_SELECTIONS);
}

static void
on_drag_end_chord (ArrangerWidget * self)
{
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      {
        /* if something was clicked with ctrl without
         * moving*/
        if (self->ctrl_held)
          {
            if (self->start_object && self->start_object_was_selected)
              {
                /*[> deselect it <]*/
                arranger_object_select (
                  self->start_object, F_NO_SELECT, F_APPEND,
                  F_NO_PUBLISH_EVENTS);
              }
          }
      }
      break;
    case UI_OVERLAY_ACTION_MOVING:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;
        int    chord_diff =
          ((ChordObject *) obj)->chord_index
          - ((ChordObject *) obj->transient)->chord_index;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_move_chord (
          CHORD_SELECTIONS, ticks_diff, chord_diff, F_ALREADY_MOVED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to move chords"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;
        int    chord_diff =
          ((ChordObject *) obj)->chord_index
          - ((ChordObject *) obj->transient)->chord_index;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_duplicate_chord (
          CHORD_SELECTIONS, ticks_diff, chord_diff, F_ALREADY_MOVED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to duplicate chords"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_NONE:
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      {
        arranger_selections_clear (
          (ArrangerSelections *) CHORD_SELECTIONS, F_NO_FREE,
          F_NO_PUBLISH_EVENTS);
      }
      break;
    case UI_OVERLAY_ACTION_CREATING_MOVING:
      {
        GError * err = NULL;
        bool     ret =
          arranger_selections_action_perform_create (CHORD_SELECTIONS, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to create objects"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      handle_erase_action (self);
      break;
    /* if didn't click on something */
    default:
      {
      }
      break;
    }
}

static void
on_drag_end_audio (ArrangerWidget * self)
{
  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_R:
      {
        /* if start range selection is after
         * end, fix it */
        if (
          AUDIO_SELECTIONS->has_selection
          && position_is_after (
            &AUDIO_SELECTIONS->sel_start, &AUDIO_SELECTIONS->sel_end))
          {
            Position tmp = AUDIO_SELECTIONS->sel_start;
            AUDIO_SELECTIONS->sel_start = AUDIO_SELECTIONS->sel_end;
            AUDIO_SELECTIONS->sel_end = tmp;
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_FADE:
    case UI_OVERLAY_ACTION_RESIZING_R_FADE:
      {
        ArrangerObject * obj =
          (ArrangerObject *) clip_editor_get_region (CLIP_EDITOR);
        g_return_if_fail (IS_REGION_AND_NONNULL (obj));
        bool   is_fade_in = self->action == UI_OVERLAY_ACTION_RESIZING_L_FADE;
        double ticks_diff =
          (is_fade_in ? obj->fade_in_pos.ticks : obj->fade_out_pos.ticks)
          - self->fade_pos_at_start.ticks;

        ArrangerSelections * sel =
          arranger_selections_new (ARRANGER_SELECTIONS_TYPE_TIMELINE);
        arranger_selections_add_object (sel, obj);

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_resize (
          sel,
          is_fade_in
                ? ARRANGER_SELECTIONS_ACTION_RESIZE_L_FADE
                : ARRANGER_SELECTIONS_ACTION_RESIZE_R_FADE,
          ticks_diff, F_ALREADY_EDITED, &err);
        arranger_selections_free (sel);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed resizing selection"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN:
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT:
    case UI_OVERLAY_ACTION_RESIZING_UP:
      {
        ZRegion *        r = clip_editor_get_region (CLIP_EDITOR);
        ArrangerObject * obj = (ArrangerObject *) r;
        g_return_if_fail (IS_REGION_AND_NONNULL (obj));

        /* prepare current selections */
        ArrangerSelections * sel =
          arranger_selections_new (ARRANGER_SELECTIONS_TYPE_TIMELINE);
        arranger_selections_add_object (sel, obj);

        ArrangerSelectionsActionEditType edit_type;

        /* prepare selections before */
        ArrangerSelections * sel_before =
          arranger_selections_new (ARRANGER_SELECTIONS_TYPE_TIMELINE);
        ArrangerObject * clone_obj = arranger_object_clone (obj);
        if (self->action == UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN)
          {
            clone_obj->fade_in_opts.curviness = self->dval_at_start;
            edit_type = ARRANGER_SELECTIONS_ACTION_EDIT_FADES;
          }
        else if (self->action == UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT)
          {
            clone_obj->fade_out_opts.curviness = self->dval_at_start;
            edit_type = ARRANGER_SELECTIONS_ACTION_EDIT_FADES;
          }
        else if (self->action == UI_OVERLAY_ACTION_RESIZING_UP)
          {
            ZRegion * clone_r = (ZRegion *) clone_obj;
            clone_r->gain = self->fval_at_start;
            edit_type = ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE;
          }
        else
          {
            g_return_if_reached ();
          }
        arranger_selections_add_object (sel_before, clone_obj);

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_edit (
          sel_before, sel, edit_type, F_ALREADY_EDITED, &err);
        arranger_selections_free_full (sel_before);
        arranger_selections_free (sel);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to edit selection"));
          }
      }
      break;
    default:
      break;
    }
}

NONNULL static void
on_drag_end_timeline (ArrangerWidget * self)
{
  ArrangerSelections * sel = arranger_widget_get_selections (self);
  g_return_if_fail (sel);

  switch (self->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN:
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT:
      {
        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_edit (
          self->sel_at_start, (ArrangerSelections *) TL_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_EDIT_FADES, true, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to edit timeline objects"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      if (!self->resizing_range)
        {
          ArrangerObject * obj = (ArrangerObject *) self->start_object;
          double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;

          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_resize (
            (ArrangerSelections *) TL_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_RESIZE_L, ticks_diff, F_ALREADY_EDITED,
            &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _ ("Failed to resize timeline "
                   "objects"));
            }
        }
      break;
    case UI_OVERLAY_ACTION_STRETCHING_L:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_resize (
          (ArrangerSelections *) TL_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_STRETCH_L, ticks_diff, F_ALREADY_EDITED,
          &err);
        if (!ret)
          {
            HANDLE_ERROR (
              err, "%s",
              _ ("Failed to resize timeline "
                 "objects"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_resize (
          (ArrangerSelections *) TL_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP, ticks_diff,
          F_ALREADY_EDITED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to resize selection"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_FADE:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double           ticks_diff =
          obj->fade_in_pos.ticks - obj->transient->fade_in_pos.ticks;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_resize (
          (ArrangerSelections *) TL_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_L_FADE, ticks_diff,
          F_ALREADY_EDITED, &err);
        if (!ret)
          {
            HANDLE_ERROR (
              err, "%s",
              _ ("Failed setting fade in "
                 "position"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      if (!self->resizing_range)
        {
          ArrangerObject * obj = (ArrangerObject *) self->start_object;
          double ticks_diff = obj->end_pos.ticks - obj->transient->end_pos.ticks;

          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_resize (
            (ArrangerSelections *) TL_SELECTIONS,
            ARRANGER_SELECTIONS_ACTION_RESIZE_R, ticks_diff, F_ALREADY_EDITED,
            &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed resizing selections"));
            }
        }
      break;
    case UI_OVERLAY_ACTION_STRETCHING_R:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->end_pos.ticks - obj->transient->end_pos.ticks;
        /* stretch now */
        GError * err = NULL;
        bool     success = transport_stretch_regions (
          TRANSPORT, TL_SELECTIONS, false, 0.0, Z_F_FORCE, &err);
        if (!success)
          {
            HANDLE_ERROR_LITERAL (err, "Failed stretching regions");
          }

        success = arranger_selections_action_perform_resize (
          (ArrangerSelections *) TL_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_STRETCH_R, ticks_diff, F_ALREADY_EDITED,
          &err);
        if (!success)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed resizing selections"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->end_pos.ticks - obj->transient->end_pos.ticks;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_resize (
          (ArrangerSelections *) TL_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP, ticks_diff,
          F_ALREADY_EDITED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed resizing selections"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_FADE:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double           ticks_diff =
          obj->fade_out_pos.ticks - obj->transient->fade_out_pos.ticks;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_resize (
          (ArrangerSelections *) TL_SELECTIONS,
          ARRANGER_SELECTIONS_ACTION_RESIZE_R_FADE, ticks_diff,
          F_ALREADY_EDITED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed resizing selection"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      /* if something was clicked with ctrl without
       * moving*/
      if (self->ctrl_held)
        {
          if (self->start_object && self->start_object_was_selected)
            {
              /* deselect it */
              arranger_object_select (
                self->start_object, F_NO_SELECT, F_APPEND, F_PUBLISH_EVENTS);
              g_debug ("deselecting object");
            }
        }
      else if (self->n_press == 2)
        {
          /* double click on object */
          /*g_message ("DOUBLE CLICK");*/
        }
      else if (self->n_press == 1)
        {
          /* single click on object */
          if (self->start_object)
            {
              if (self->start_object->type == ARRANGER_OBJECT_TYPE_MARKER)
                {
                  transport_move_playhead (
                    TRANSPORT, &self->start_object->pos, F_PANIC,
                    F_SET_CUE_POINT, F_PUBLISH_EVENTS);
                }
            }
        }
      break;
    case UI_OVERLAY_ACTION_MOVING:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        g_return_if_fail (obj && obj->transient);
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;

        GError * err = NULL;
        bool     ret = arranger_selections_action_perform_move_timeline (
          TL_SELECTIONS, ticks_diff, self->visible_track_diff, self->lane_diff,
          NULL, F_ALREADY_MOVED, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to move objects"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      {
        ArrangerObject * obj = (ArrangerObject *) self->start_object;
        double ticks_diff = obj->pos.ticks - obj->transient->pos.ticks;

        GError * err = NULL;
        bool     ret;
        if (ACTION_IS (MOVING_COPY))
          {
            ret = arranger_selections_action_perform_duplicate_timeline (
              TL_SELECTIONS, ticks_diff, self->visible_track_diff,
              self->lane_diff, NULL, F_ALREADY_MOVED, &err);
          }
        else if (ACTION_IS (MOVING_LINK))
          {
            ret = arranger_selections_action_perform_link (
              self->sel_at_start, (ArrangerSelections *) TL_SELECTIONS,
              ticks_diff, self->visible_track_diff, self->lane_diff,
              F_ALREADY_MOVED, &err);
          }
        else
          g_return_if_reached ();

        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to link or copy selection"));
          }
      }
      break;
    case UI_OVERLAY_ACTION_NONE:
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      arranger_selections_clear (sel, F_NO_FREE, F_NO_PUBLISH_EVENTS);
      break;
    /* if something was created */
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
    case UI_OVERLAY_ACTION_AUTOFILLING:
      if (arranger_selections_has_any (sel))
        {
          GError * err = NULL;
          bool     ret = arranger_selections_action_perform_create (sel, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to create objects"));
            }
        }
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      handle_erase_action (self);
      break;
    case UI_OVERLAY_ACTION_CUTTING:
      {
        /* get cut position */
        Position cut_pos;
        position_set_to_pos (&cut_pos, &self->curr_pos);

        if (SNAP_GRID_ANY_SNAP (self->snap_grid) && !self->shift_held)
          {
            /* keep offset is not applicable here and causes
             * errors if enabled */
            SnapGrid * sg = snap_grid_clone (self->snap_grid);
            sg->snap_to_grid_keep_offset = false;

            position_snap_simple (&cut_pos, sg);

            snap_grid_free (sg);
          }
        if (arranger_selections_can_split_at_pos (
              (ArrangerSelections *) TL_SELECTIONS, &cut_pos))
          {
            GError * err = NULL;
            bool     ret = arranger_selections_action_perform_split (
              (ArrangerSelections *) TL_SELECTIONS, &cut_pos, &err);
            if (!ret)
              {
                HANDLE_ERROR (err, "%s", _ ("Failed to split selection"));
              }
          }
      }
      break;
    case UI_OVERLAY_ACTION_RENAMING:
      {
        const char * obj_type_str =
          arranger_object_get_type_as_string (self->start_object->type);
        char * str = g_strdup_printf (_ ("%s name"), obj_type_str);
        StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
          str, self->start_object,
          (GenericStringGetter) arranger_object_get_name,
          (GenericStringSetter) arranger_object_set_name_with_action);
        gtk_window_present (GTK_WINDOW (dialog));
        self->action = UI_OVERLAY_ACTION_NONE;
        g_free (str);
      }
      break;
    default:
      break;
    }

  self->resizing_range = 0;
  self->resizing_range_start = 0;
  self->visible_track_diff = 0;
  self->lane_diff = 0;
  self->visible_at_diff = 0;

  g_debug ("drag end timeline done");
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ArrangerWidget * self)
{
  g_debug ("arranger drag end starting...");

  if (ACTION_IS (SELECTING) || ACTION_IS (DELETE_SELECTING))
    {
      EVENTS_PUSH (ET_SELECTING_IN_ARRANGER, self);
    }

#undef ON_DRAG_END

  /* if something was clicked with ctrl without
   * moving */
  if (ACTION_IS (STARTING_MOVING) && self->start_object && self->ctrl_held)
    {
      /* if was selected, deselect it */
      if (self->start_object_was_selected)
        {
          arranger_object_select (
            self->start_object, F_NO_SELECT, F_APPEND, F_PUBLISH_EVENTS);
          g_debug ("ctrl-deselecting object");
        }
      /* if was deselected, select it */
      else
        {
          /* select it */
          arranger_object_select (
            self->start_object, F_SELECT, F_APPEND, F_PUBLISH_EVENTS);
          g_debug ("ctrl-selecting object");
        }
    }

  /* handle click without drag for
   * delete-selecting */
  if (
    (self->action == UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION
     || self->action == UI_OVERLAY_ACTION_STARTING_ERASING)
    && self->drag_start_btn == GDK_BUTTON_PRIMARY)
    {
      self->action = UI_OVERLAY_ACTION_DELETE_SELECTING;
      ArrangerSelections * sel = arranger_widget_get_selections (self);
      g_return_if_fail (sel);
      arranger_selections_clear (sel, F_NO_FREE, F_NO_PUBLISH_EVENTS);
      self->sel_to_delete = arranger_selections_clone (sel);
      select_in_range (
        self, offset_x, offset_y, F_IN_RANGE, F_IGNORE_FROZEN, F_DELETE);
    }

  /* handle audition stop */
  if (
    self->action == UI_OVERLAY_ACTION_STARTING_AUDITIONING
    || self->action == UI_OVERLAY_ACTION_AUDITIONING)
    {
      if (self->was_paused)
        {
          transport_request_pause (TRANSPORT, true);
        }
      transport_set_playhead_pos (TRANSPORT, &self->playhead_pos_at_start);
    }

  switch (self->type)
    {
    case TYPE (TIMELINE):
      on_drag_end_timeline (self);
      break;
    case TYPE (AUDIO):
      on_drag_end_audio (self);
      break;
    case TYPE (MIDI):
      on_drag_end_midi (self);
      break;
    case TYPE (MIDI_MODIFIER):
      on_drag_end_midi_modifier (self);
      break;
    case TYPE (CHORD):
      on_drag_end_chord (self);
      break;
    case TYPE (AUTOMATION):
      on_drag_end_automation (self);
      break;
    default:
      break;
    }

  /* if right click, show context */
  if (gesture)
    {
      GdkEventSequence * sequence =
        gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
      GdkEvent * ev =
        gtk_gesture_get_last_event (GTK_GESTURE (gesture), sequence);
      guint btn;
      if (
        ev
        && (GDK_IS_EVENT_TYPE (ev, GDK_BUTTON_PRESS) || GDK_IS_EVENT_TYPE (ev, GDK_BUTTON_RELEASE)))
        {
          btn = gdk_button_event_get_button (ev);
          g_warn_if_fail (btn);
          if (
            btn == GDK_BUTTON_SECONDARY
            && self->action != UI_OVERLAY_ACTION_ERASING)
            {
              show_context_menu (
                self, self->start_x + offset_x, self->start_y + offset_y);
            }
        }
    }

  /* reset start coordinates and offsets */
  self->start_x = 0;
  self->start_y = 0;
  self->last_offset_x = 0;
  self->last_offset_y = 0;
  self->drag_update_started = false;
  self->last_adj_ticks_diff = 0;
  self->start_object = NULL;
  self->hovered_object = NULL;

  self->shift_held = 0;
  self->ctrl_held = 0;

  if (self->sel_at_start)
    {
      arranger_selections_free_full (self->sel_at_start);
      self->sel_at_start = NULL;
    }
  if (self->region_at_start)
    {
      arranger_object_free ((ArrangerObject *) self->region_at_start);
      self->region_at_start = NULL;
    }

  /* reset action */
  self->action = UI_OVERLAY_ACTION_NONE;

  /* queue redraw to hide the selection */
  /*gtk_widget_queue_draw (GTK_WIDGET (self->bg));*/

  arranger_widget_refresh_cursor (self);

  g_debug ("arranger drag end done");
}

/**
 * To be called after using arranger_widget_create_item() in
 * an action (ie, not from click + drag interaction with the
 * arranger) to finish the action.
 *
 * @return Whether an action was performed.
 */
bool
arranger_widget_finish_creating_item_from_action (
  ArrangerWidget * self,
  double           x,
  double           y)
{
  bool has_action = self->action != UI_OVERLAY_ACTION_NONE;

  drag_end (NULL, x, y, self);

  return has_action;
}

#if 0
/**
 * @param type The arranger object type, or -1 to
 *   search for all types.
 */
static ArrangerObject *
get_hit_timeline_object (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  double             x,
  double             y)
{
  AutomationTrack * at =
    timeline_arranger_widget_get_at_at_y (
      self, y);
  TrackLane * lane = NULL;
  if (!at)
    lane =
      timeline_arranger_widget_get_track_lane_at_y (
        self, y);
  Track * track = NULL;

  /* get the position of x */
  Position pos;
  arranger_widget_px_to_pos (
    self, x, &pos, 1);

  /* check for hit automation regions */
  if (at)
    {
      if (type != ARRANGER_OBJECT_TYPE_ALL &&
          type != ARRANGER_OBJECT_TYPE_REGION)
        return NULL;

      /* return if any region matches the
       * position */
      for (int i = 0; i < at->num_regions; i++)
        {
          ZRegion * r = at->regions[i];
          if (region_is_hit (r, pos.frames, 1))
            {
              return (ArrangerObject *) r;
            }
        }
    }
  /* check for hit regions in lane */
  else if (lane)
    {
      if (type >= ARRANGER_OBJECT_TYPE_ALL &&
          type != ARRANGER_OBJECT_TYPE_REGION)
        return NULL;

      /* return if any region matches the
       * position */
      for (int i = 0; i < lane->num_regions; i++)
        {
          ZRegion * r = lane->regions[i];
          if (region_is_hit (r, pos.frames, 1))
            {
              return (ArrangerObject *) r;
            }
        }
    }
  /* check for hit regions in main track */
  else
    {
      track =
        timeline_arranger_widget_get_track_at_y (
          self, y);

      if (track)
        {
          for (int i = 0; i < track->num_lanes; i++)
            {
              lane = track->lanes[i];
              for (int j = 0; j < lane->num_regions;
                   j++)
                {
                  ZRegion * r = lane->regions[j];
                  if (region_is_hit (
                        r, pos.frames, 1))
                    {
                      return (ArrangerObject *) r;
                    }
                }
            }
        }
    }

  return NULL;
}
#endif

/**
 * Returns the ArrangerObject of the given type
 * at (x,y).
 *
 * @param type The arranger object type, or -1 to
 *   search for all types.
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 */
ArrangerObject *
arranger_widget_get_hit_arranger_object (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  double             x,
  double             y)
{
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_widget_get_hit_objects_at_point (self, type, x, y, objs_arr);
  ArrangerObject * obj = NULL;
  if (objs_arr->len > 0)
    {
      obj = (ArrangerObject *) g_ptr_array_index (objs_arr, objs_arr->len - 1);
    }

  g_ptr_array_unref (objs_arr);

  /*g_debug ("hit obj at %f %f: %p", x, y, obj);*/

  return obj;
}

/**
 * Wrapper of the UI functions based on the arranger
 * type.
 */
int
arranger_widget_pos_to_px (ArrangerWidget * self, Position * pos, int use_padding)
{
  if (self->type == TYPE (TIMELINE))
    {
      return ui_pos_to_px_timeline (pos, use_padding);
    }
  else
    {
      return ui_pos_to_px_editor (pos, use_padding);
    }

  g_return_val_if_reached (-1);
}

/**
 * Returns the ArrangerSelections for this
 * ArrangerWidget.
 */
ArrangerSelections *
arranger_widget_get_selections (ArrangerWidget * self)
{
  switch (self->type)
    {
    case TYPE (TIMELINE):
      return (ArrangerSelections *) TL_SELECTIONS;
    case TYPE (MIDI):
    case TYPE (MIDI_MODIFIER):
      return (ArrangerSelections *) MA_SELECTIONS;
    case TYPE (AUTOMATION):
      return (ArrangerSelections *) AUTOMATION_SELECTIONS;
    case TYPE (CHORD):
      return (ArrangerSelections *) CHORD_SELECTIONS;
    case TYPE (AUDIO):
      return (ArrangerSelections *) AUDIO_SELECTIONS;
    default:
      g_critical ("should not be reached");
      return (ArrangerSelections *) TL_SELECTIONS;
    }
}

/**
 * Get all objects currently present in the arranger.
 */
void
arranger_widget_get_all_objects (ArrangerWidget * self, GPtrArray * objs_arr)
{
  RulerWidget * ruler = arranger_widget_get_ruler (self);
  GdkRectangle  rect = {
    0,
    0,
    (int) ruler->total_px,
    arranger_widget_get_total_height (self),
  };

  arranger_widget_get_hit_objects_in_rect (
    self, ARRANGER_OBJECT_TYPE_ALL, &rect, objs_arr);
}

/**
 * Returns the total height (including off-screen).
 */
int
arranger_widget_get_total_height (ArrangerWidget * self)
{
  int height = 0;
  switch (self->type)
    {
    case TYPE (TIMELINE):
      if (self->is_pinned)
        {
          height = gtk_widget_get_height (GTK_WIDGET (self));
        }
      else
        {
          height = gtk_widget_get_height (GTK_WIDGET (MW_TRACKLIST));
        }
      break;
    case TYPE (MIDI):
      height = gtk_widget_get_height (GTK_WIDGET (MW_PIANO_ROLL_KEYS));
      break;
    default:
      height = gtk_widget_get_height (GTK_WIDGET (self));
      break;
    }

  return height;
}

RulerWidget *
arranger_widget_get_ruler (ArrangerWidget * self)
{
  return self->type == TYPE (TIMELINE) ? MW_RULER : EDITOR_RULER;
}

/**
 * Returns the current visible rectangle.
 *
 * @param rect The rectangle to fill in.
 */
void
arranger_widget_get_visible_rect (ArrangerWidget * self, GdkRectangle * rect)
{
  const EditorSettings settings =
    arranger_widget_get_editor_setting_values (self);
  rect->x = settings.scroll_start_x;
  rect->y = settings.scroll_start_y;
  rect->width = gtk_widget_get_width (GTK_WIDGET (self));
  rect->height = gtk_widget_get_height (GTK_WIDGET (self));
}

bool
arranger_widget_is_playhead_visible (ArrangerWidget * self)
{
  GdkRectangle rect;
  arranger_widget_get_visible_rect (self, &rect);

  int playhead_x = arranger_widget_get_playhead_px (self);
  int min_x = MIN (self->last_playhead_px, playhead_x);
  min_x = MAX (min_x - 4, rect.x);
  int max_x = MAX (self->last_playhead_px, playhead_x);
  max_x = MIN (max_x + 4, rect.x + rect.width);

  int width = max_x - min_x;

  return width >= 0;
}

static gboolean
on_scroll (
  GtkEventControllerScroll * scroll_controller,
  gdouble                    dx,
  gdouble                    dy,
  gpointer                   user_data)
{
  ArrangerWidget * self = Z_ARRANGER_WIDGET (user_data);

#if 0
  GdkEvent * event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (scroll_controller));
  GdkDevice * source_device = gdk_event_get_device (event);
  GdkInputSource input_source = gdk_device_get_source (source_device);
  /* adjust for scroll unit */
  /* TODO */
  GdkScrollUnit scroll_unit = gtk_event_controller_scroll_get_unit (scroll_controller);
  if (scroll_unit == GDK_SCROLL_UNIT_WHEEL)
    {
      g_debug ("wheel");
      /*double page_size =*/
      /*double scroll_step = pow (page_size, 2.0 / 3.0);*/
    }
  else if (scroll_unit == GDK_SCROLL_UNIT_SURFACE)
    {
      g_debug ("unit interface");
      dx *= MAGIC_SCROLL_FACTOR;
    }
#endif

  double x = self->hover_x;
  double y = self->hover_y;

  g_debug ("scrolled to %.1f (d %d), %.1f (d %d)", x, (int) dx, y, (int) dy);

  EVENTS_PUSH (ET_ARRANGER_SCROLLED, self);

  GdkModifierType modifier_type = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (scroll_controller));

  bool ctrl_held = modifier_type & GDK_CONTROL_MASK;
  if (ctrl_held)
    {
      /* if shift also pressed, handle vertical zoom */
      if (modifier_type & GDK_SHIFT_MASK)
        {
          if (self->type == TYPE (MIDI))
            {
              midi_arranger_handle_vertical_zoom_scroll (
                self, scroll_controller, dy);
            }
          else if (self->type == TYPE (TIMELINE))
            {
              tracklist_widget_handle_vertical_zoom_scroll (
                MW_TRACKLIST, scroll_controller, dy);
            }

          /* TODO also update hover y since we're using it
           * here */
          /*self->hover_y = new_y;*/
        }
      /* else if just control pressed handle horizontal
       * zoom */
      else
        {
          RulerWidget * ruler = arranger_widget_get_ruler (self);

          /* get current adjustment so we can get the
           * difference from the cursor */
          EditorSettings * settings = arranger_widget_get_editor_settings (self);

          /* get position of cursor */
          Position cursor_pos;
          arranger_widget_px_to_pos (self, x, &cursor_pos, F_PADDING);
          /*position_print (&cursor_pos);*/

          /* get px diff so we can calculate the new
           * adjustment later */
          double diff = x - (double) settings->scroll_start_x;

          double scroll_multiplier = (dy > 0) ? (1.0 / 1.3) : 1.3;
          ruler_widget_set_zoom_level (
            ruler, ruler_widget_get_zoom_level (ruler) * scroll_multiplier);

          int new_x = arranger_widget_pos_to_px (self, &cursor_pos, F_PADDING);

          editor_settings_set_scroll_start_x (
            settings, new_x - (int) diff, F_VALIDATE);

          /* refresh relevant widgets */
          if (self->type == TYPE (TIMELINE))
            {
              arranger_minimap_widget_refresh (MW_TIMELINE_MINIMAP);
            }

          /* also update hover x since we're using it here */
          self->hover_x = new_x;
        }
    }
  else /* else if not ctrl held */
    {
      /* scroll normally */
      const int scroll_amt = RW_SCROLL_SPEED;
      int       scroll_x = 0;
      int       scroll_y = 0;

      /* if scrolling on x-axis from a touch pad */
      if (!math_doubles_equal (dx, 0.0))
        {
          scroll_x = (int) dx;
        }
      else if (modifier_type & GDK_SHIFT_MASK)
        {
          scroll_x = (int) dy;
          scroll_y = 0;
        }
      else
        {
          scroll_x = 0;

          /* if not midi modifier and not pinned timeline,
           * handle the scroll, otherwise ignore */
          if (
            self->type != TYPE (MIDI_MODIFIER)
            && !(self->type == TYPE (TIMELINE) && self->is_pinned))
            {
              scroll_y = (int) dy;
            }
        }
      EditorSettings * settings = arranger_widget_get_editor_settings (self);
      scroll_x = scroll_x * scroll_amt;
      scroll_y = scroll_y * scroll_amt;
      int scroll_x_before = settings->scroll_start_x;
      int scroll_y_before = settings->scroll_start_y;
      editor_settings_append_scroll (settings, scroll_x, scroll_y, F_VALIDATE);

      /* also adjust the drag offsets (in case we are currently
       * dragging */
      scroll_x = settings->scroll_start_x - scroll_x_before;
      scroll_y = settings->scroll_start_y - scroll_y_before;
      self->offset_x_from_scroll += scroll_x;
      self->offset_y_from_scroll += scroll_y;

      /* auto-scroll linked widgets */
      if (scroll_y != 0)
        {
          if (self->type == TYPE (TIMELINE) && !self->is_pinned)
            {
              tracklist_widget_set_unpinned_scroll_start_y (
                MW_TRACKLIST, settings->scroll_start_y);
            }
          else if (self->type == TYPE (MIDI))
            {
              midi_editor_space_widget_set_piano_keys_scroll_start_y (
                MW_MIDI_EDITOR_SPACE, settings->scroll_start_y);
            }
          else if (self->type == TYPE (CHORD))
            {
              chord_editor_space_widget_set_chord_keys_scroll_start_y (
                MW_CHORD_EDITOR_SPACE, settings->scroll_start_y);
            }
        }
    }

  return true;
}

static void
on_leave (GtkEventControllerMotion * motion_controller, ArrangerWidget * self)
{
  switch (self->type)
    {
    case TYPE (TIMELINE):
      timeline_arranger_widget_set_cut_lines_visible (self);
      break;
    case TYPE (CHORD):
      self->hovered_chord_index = -1;
      break;
    case TYPE (MIDI):
      midi_arranger_widget_set_hovered_note (self, -1);
      break;
    default:
      break;
    }

  self->hovered = false;
}

/**
 * Motion callback.
 */
static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  ArrangerWidget *           self)
{
  const EditorSettings settings =
    arranger_widget_get_editor_setting_values (self);
  x += settings.scroll_start_x;
  y += settings.scroll_start_y;
  x = MAX (x, 0.0);
  y = MAX (y, 0.0);
  self->hover_x = x;
  self->hover_y = y;

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (motion_controller));
  if (state)
    {
      /*self->alt_held = state & GDK_ALT_MASK;*/
      /*self->ctrl_held = state & GDK_CONTROL_MASK;*/
      /*self->shift_held = state & GDK_SHIFT_MASK;*/
    }

  /* highlight hovered object */
  ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
    self, ARRANGER_OBJECT_TYPE_ALL, self->hover_x, self->hover_y);
  if (obj && arranger_object_is_frozen (obj))
    {
      obj = NULL;
    }
  if (self->hovered_object != obj)
    {
      g_warn_if_fail (
        !self->hovered_object || IS_ARRANGER_OBJECT (self->hovered_object));
      self->hovered_object = obj;
    }

  if (ACTION_IS (CUTTING) && !self->alt_held && P_TOOL != TOOL_CUT)
    {
      self->action = UI_OVERLAY_ACTION_NONE;
    }

  arranger_widget_refresh_cursor (self);

  switch (self->type)
    {
    case TYPE (TIMELINE):
      timeline_arranger_widget_set_cut_lines_visible (self);
      break;
    case TYPE (CHORD):
      self->hovered_chord_index = chord_arranger_widget_get_chord_at_y (y);
      break;
    case TYPE (MIDI):
      midi_arranger_widget_set_hovered_note (
        self, piano_roll_keys_widget_get_key_from_y (MW_PIANO_ROLL_KEYS, y));
      break;
    default:
      break;
    }

  self->hovered = true;
}

static void
on_focus_leave (GtkEventControllerFocus * focus, ArrangerWidget * self)
{
  g_debug ("arranger focus out");

  self->alt_held = 0;
  self->ctrl_held = 0;
  self->shift_held = 0;
}

/**
 * Wrapper for ui_px_to_pos depending on the
 * arranger type.
 */
void
arranger_widget_px_to_pos (
  ArrangerWidget * self,
  double           px,
  Position *       pos,
  bool             has_padding)
{
  if (self->type == TYPE (TIMELINE))
    {
      ui_px_to_pos_timeline (px, pos, has_padding);
    }
  else
    {
      ui_px_to_pos_editor (px, pos, has_padding);
    }
}

static ArrangerCursor
get_audio_arranger_cursor (ArrangerWidget * self, Tool tool)
{
  ArrangerCursor  ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action = self->action;

  if (P_TOOL == TOOL_SELECT_STRETCH)
    ac = ARRANGER_CURSOR_SELECT_STRETCH;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (P_TOOL == TOOL_SELECT_NORMAL || P_TOOL == TOOL_SELECT_STRETCH)
        {
          /* gain line */
          if (audio_arranger_widget_is_cursor_gain (
                self, self->hover_x, self->hover_y))
            return ARRANGER_CURSOR_RESIZING_UP;

          if (!is_cursor_in_top_half (self, self->hover_y))
            {
              /* set cursor to range selection */
              return ARRANGER_CURSOR_RANGE;
            }

          /* resize fade in */
          /* note cursor is opposite */
          if (audio_arranger_widget_is_cursor_in_fade (
                self, self->hover_x, self->hover_y, true, true))
            {
              return ARRANGER_CURSOR_RESIZING_R_FADE;
            }
          /* resize fade out */
          if (audio_arranger_widget_is_cursor_in_fade (
                self, self->hover_x, self->hover_y, false, true))
            {
              return ARRANGER_CURSOR_RESIZING_L_FADE;
            }
          /* fade in curviness */
          if (audio_arranger_widget_is_cursor_in_fade (
                self, self->hover_x, self->hover_y, true, false))
            {
              return ARRANGER_CURSOR_RESIZING_UP_FADE_IN;
            }
          /* fade out curviness */
          if (audio_arranger_widget_is_cursor_in_fade (
                self, self->hover_x, self->hover_y, false, false))
            {
              return ARRANGER_CURSOR_RESIZING_UP_FADE_OUT;
            }

          /* set cursor to normal */
          return ac;
        }
      else if (P_TOOL == TOOL_EDIT)
        ac = ARRANGER_CURSOR_EDIT;
      else if (P_TOOL == TOOL_ERASER)
        ac = ARRANGER_CURSOR_ERASER;
      else if (P_TOOL == TOOL_RAMP)
        ac = ARRANGER_CURSOR_RAMP;
      else if (P_TOOL == TOOL_AUDITION)
        ac = ARRANGER_CURSOR_AUDITION;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_STARTING_ERASING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_STARTING_PANNING:
    case UI_OVERLAY_ACTION_PANNING:
      ac = ARRANGER_CURSOR_PANNING;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      ac = ARRANGER_CURSOR_RESIZING_UP;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN:
      ac = ARRANGER_CURSOR_RESIZING_UP_FADE_IN;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT:
      ac = ARRANGER_CURSOR_RESIZING_UP_FADE_OUT;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_FADE:
      ac = ARRANGER_CURSOR_RESIZING_L_FADE;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_FADE:
      ac = ARRANGER_CURSOR_RESIZING_R_FADE;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

static ArrangerCursor
get_midi_modifier_arranger_cursor (ArrangerWidget * self, Tool tool)
{
  ArrangerCursor  ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action = self->action;

  if (P_TOOL == TOOL_SELECT_STRETCH)
    ac = ARRANGER_CURSOR_SELECT_STRETCH;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (tool == TOOL_SELECT_NORMAL || tool == TOOL_SELECT_STRETCH)
        {
          ArrangerObject * vel_obj = arranger_widget_get_hit_arranger_object (
            (ArrangerWidget *) self, ARRANGER_OBJECT_TYPE_VELOCITY,
            self->hover_x, self->hover_y);
          int is_hit = vel_obj != NULL;

          if (is_hit)
            {
              int is_resize = arranger_object_is_resize_up (
                vel_obj, (int) self->hover_x - vel_obj->full_rect.x,
                (int) self->hover_y - vel_obj->full_rect.y);
              if (is_resize)
                {
                  return ARRANGER_CURSOR_RESIZING_UP;
                }
            }

          return ac;
        }
      else if (P_TOOL == TOOL_EDIT)
        ac = ARRANGER_CURSOR_EDIT;
      else if (P_TOOL == TOOL_ERASER)
        ac = ARRANGER_CURSOR_ERASER;
      else if (P_TOOL == TOOL_RAMP)
        ac = ARRANGER_CURSOR_RAMP;
      else if (P_TOOL == TOOL_AUDITION)
        ac = ARRANGER_CURSOR_AUDITION;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_STARTING_ERASING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_STARTING_PANNING:
    case UI_OVERLAY_ACTION_PANNING:
      ac = ARRANGER_CURSOR_PANNING;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      ac = ARRANGER_CURSOR_RESIZING_UP;
      break;
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
    case UI_OVERLAY_ACTION_SELECTING:
      /* TODO depends on tool */
      break;
    case UI_OVERLAY_ACTION_STARTING_RAMP:
    case UI_OVERLAY_ACTION_RAMPING:
      ac = ARRANGER_CURSOR_RAMP;
      break;
    /* editing */
    case UI_OVERLAY_ACTION_AUTOFILLING:
      ac = ARRANGER_CURSOR_EDIT;
      break;
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

static ArrangerCursor
get_chord_arranger_cursor (ArrangerWidget * self, Tool tool)
{
  ArrangerCursor  ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action = self->action;

  if (P_TOOL == TOOL_SELECT_STRETCH)
    ac = ARRANGER_CURSOR_SELECT_STRETCH;

  int is_hit =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self, ARRANGER_OBJECT_TYPE_CHORD_OBJECT, self->hover_x,
      self->hover_y)
    != NULL;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
          {
            if (is_hit)
              {
                return ARRANGER_CURSOR_GRAB;
              }
            else
              {
                /* set cursor to normal */
                return ARRANGER_CURSOR_SELECT;
              }
          }
          break;
        case TOOL_SELECT_STRETCH:
          ac = ARRANGER_CURSOR_SELECT_STRETCH;
          break;
        case TOOL_EDIT:
          ac = ARRANGER_CURSOR_EDIT;
          break;
        case TOOL_CUT:
          ac = ARRANGER_CURSOR_CUT;
          break;
        case TOOL_ERASER:
          ac = ARRANGER_CURSOR_ERASER;
          break;
        case TOOL_RAMP:
          ac = ARRANGER_CURSOR_RAMP;
          break;
        case TOOL_AUDITION:
          ac = ARRANGER_CURSOR_AUDITION;
          break;
        }
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_STARTING_ERASING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_STARTING_PANNING:
    case UI_OVERLAY_ACTION_PANNING:
      ac = ARRANGER_CURSOR_PANNING;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

static ArrangerCursor
get_automation_arranger_cursor (ArrangerWidget * self, Tool tool)
{
  ArrangerCursor  ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action = self->action;

  if (P_TOOL == TOOL_SELECT_STRETCH)
    ac = ARRANGER_CURSOR_SELECT_STRETCH;

  ArrangerObject * hit_obj = arranger_widget_get_hit_arranger_object (
    (ArrangerWidget *) self, ARRANGER_OBJECT_TYPE_AUTOMATION_POINT,
    self->hover_x, self->hover_y);

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
          {
            if (hit_obj)
              {
                if (
                  automation_point_is_point_hit (
                    (AutomationPoint *) hit_obj, self->hover_x, self->hover_y))
                  {
                    return ARRANGER_CURSOR_GRAB;
                  }
                else
                  {
                    return ARRANGER_CURSOR_RESIZING_UP;
                  }
              }
            else
              {
                /* set cursor to normal */
                return ac;
              }
          }
          break;
        case TOOL_SELECT_STRETCH:
          ac = ARRANGER_CURSOR_SELECT_STRETCH;
          break;
        case TOOL_EDIT:
          ac = ARRANGER_CURSOR_EDIT;
          break;
        case TOOL_CUT:
          ac = ARRANGER_CURSOR_CUT;
          break;
        case TOOL_ERASER:
          ac = ARRANGER_CURSOR_ERASER;
          break;
        case TOOL_RAMP:
          ac = ARRANGER_CURSOR_RAMP;
          break;
        case TOOL_AUDITION:
          ac = ARRANGER_CURSOR_AUDITION;
          break;
        }
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_STARTING_ERASING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_STARTING_PANNING:
    case UI_OVERLAY_ACTION_PANNING:
      ac = ARRANGER_CURSOR_PANNING;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_AUTOFILLING:
      ac = ARRANGER_CURSOR_AUTOFILL;
      break;
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
    case UI_OVERLAY_ACTION_SELECTING:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

static ArrangerCursor
get_timeline_cursor (ArrangerWidget * self, Tool tool)
{
  ArrangerCursor  ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action = self->action;

  if (P_TOOL == TOOL_SELECT_STRETCH)
    ac = ARRANGER_CURSOR_SELECT_STRETCH;

  ArrangerObject * r_obj = arranger_widget_get_hit_arranger_object (
    (ArrangerWidget *) self, ARRANGER_OBJECT_TYPE_REGION, self->hover_x,
    self->hover_y);
  ArrangerObject * s_obj = arranger_widget_get_hit_arranger_object (
    (ArrangerWidget *) self, ARRANGER_OBJECT_TYPE_SCALE_OBJECT, self->hover_x,
    self->hover_y);
  ArrangerObject * m_obj = arranger_widget_get_hit_arranger_object (
    (ArrangerWidget *) self, ARRANGER_OBJECT_TYPE_MARKER, self->hover_x,
    self->hover_y);

  if (r_obj && arranger_object_is_frozen (r_obj))
    {
      r_obj = NULL;
    }
  if (s_obj && arranger_object_is_frozen (s_obj))
    {
      s_obj = NULL;
    }
  if (m_obj && arranger_object_is_frozen (m_obj))
    {
      m_obj = NULL;
    }
  int is_hit = r_obj || s_obj || m_obj;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        case TOOL_SELECT_STRETCH:
          {
            if (is_hit)
              {
                if (r_obj)
                  {
                    if (self->alt_held)
                      return ARRANGER_CURSOR_CUT;
                    int wx = (int) self->hover_x - r_obj->full_rect.x;
                    int wy = (int) self->hover_y - r_obj->full_rect.y;
                    int is_fade_in_point =
                      arranger_object_is_fade_in (r_obj, wx, wy, 1, 0);
                    int is_fade_out_point =
                      arranger_object_is_fade_out (r_obj, wx, wy, 1, 0);
                    int is_fade_in_outer_region =
                      arranger_object_is_fade_in (r_obj, wx, wy, 0, 1);
                    int is_fade_out_outer_region =
                      arranger_object_is_fade_out (r_obj, wx, wy, 0, 1);
                    int is_resize_l = arranger_object_is_resize_l (r_obj, wx);
                    int is_resize_r = arranger_object_is_resize_r (r_obj, wx);
                    int is_resize_loop =
                      arranger_object_is_resize_loop (r_obj, wy);
                    bool is_rename = arranger_object_is_rename (r_obj, wx, wy);
                    if (is_fade_in_point)
                      return ARRANGER_CURSOR_FADE_IN;
                    else if (is_fade_out_point)
                      return ARRANGER_CURSOR_FADE_OUT;
                    else if (is_resize_l && is_resize_loop)
                      {
                        return ARRANGER_CURSOR_RESIZING_L_LOOP;
                      }
                    else if (is_resize_l)
                      {
                        if (P_TOOL == TOOL_SELECT_NORMAL)
                          return ARRANGER_CURSOR_RESIZING_L;
                        else if (P_TOOL == TOOL_SELECT_STRETCH)
                          {
                            return ARRANGER_CURSOR_STRETCHING_L;
                          }
                      }
                    else if (is_resize_r && is_resize_loop)
                      return ARRANGER_CURSOR_RESIZING_R_LOOP;
                    else if (is_resize_r)
                      {
                        if (P_TOOL == TOOL_SELECT_NORMAL)
                          return ARRANGER_CURSOR_RESIZING_R;
                        else if (P_TOOL == TOOL_SELECT_STRETCH)
                          return ARRANGER_CURSOR_STRETCHING_R;
                      }
                    else if (is_fade_in_outer_region)
                      return ARRANGER_CURSOR_FADE_IN;
                    else if (is_fade_out_outer_region)
                      return ARRANGER_CURSOR_FADE_OUT;
                    else if (is_rename)
                      return ARRANGER_CURSOR_RENAME;
                  }
                return ARRANGER_CURSOR_GRAB;
              }
            else
              {
                Track * track =
                  timeline_arranger_widget_get_track_at_y (self, self->hover_y);

                if (track)
                  {
                    if (track_widget_is_cursor_in_range_select_half (
                          track->widget, self->hover_y))
                      {
                        /* set cursor to range selection */
                        return ARRANGER_CURSOR_RANGE;
                      }
                    else
                      {
                        /* set cursor to normal */
                        return ac;
                      }
                  }
                else
                  {
                    /* set cursor to normal */
                    return ac;
                  }
              }
          }
          break;
        case TOOL_EDIT:
          ac = ARRANGER_CURSOR_EDIT;
          break;
        case TOOL_CUT:
          ac = ARRANGER_CURSOR_CUT;
          break;
        case TOOL_ERASER:
          ac = ARRANGER_CURSOR_ERASER;
          break;
        case TOOL_RAMP:
          ac = ARRANGER_CURSOR_RAMP;
          break;
        case TOOL_AUDITION:
          ac = ARRANGER_CURSOR_AUDITION;
          break;
        }
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_STARTING_ERASING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_PANNING:
    case UI_OVERLAY_ACTION_PANNING:
      ac = ARRANGER_CURSOR_PANNING;
      break;
    case UI_OVERLAY_ACTION_STRETCHING_L:
      ac = ARRANGER_CURSOR_STRETCHING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      if (self->resizing_range)
        ac = ARRANGER_CURSOR_RANGE;
      else
        ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
      ac = ARRANGER_CURSOR_RESIZING_L_LOOP;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_FADE:
      ac = ARRANGER_CURSOR_FADE_IN;
      break;
    case UI_OVERLAY_ACTION_STRETCHING_R:
      ac = ARRANGER_CURSOR_STRETCHING_R;
      break;
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
    case UI_OVERLAY_ACTION_RESIZING_R:
      if (self->resizing_range)
        ac = ARRANGER_CURSOR_RANGE;
      else
        ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      ac = ARRANGER_CURSOR_RESIZING_R_LOOP;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_FADE:
      ac = ARRANGER_CURSOR_FADE_OUT;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN:
      ac = ARRANGER_CURSOR_FADE_IN;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT:
      ac = ARRANGER_CURSOR_FADE_OUT;
      break;
    case UI_OVERLAY_ACTION_AUTOFILLING:
      ac = ARRANGER_CURSOR_AUTOFILL;
      break;
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
    case UI_OVERLAY_ACTION_SELECTING:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    case UI_OVERLAY_ACTION_RENAMING:
      ac = ARRANGER_CURSOR_RENAME;
      break;
    case UI_OVERLAY_ACTION_CUTTING:
      ac = ARRANGER_CURSOR_CUT;
      break;
    case UI_OVERLAY_ACTION_STARTING_AUDITIONING:
    case UI_OVERLAY_ACTION_AUDITIONING:
      ac = ARRANGER_CURSOR_AUDITION;
      break;
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

static ArrangerCursor
get_midi_arranger_cursor (ArrangerWidget * self, Tool tool)
{
  ArrangerCursor  ac = ARRANGER_CURSOR_SELECT;
  UiOverlayAction action = self->action;

  if (P_TOOL == TOOL_SELECT_STRETCH)
    ac = ARRANGER_CURSOR_SELECT_STRETCH;

  ArrangerObject * obj = arranger_widget_get_hit_arranger_object (
    (ArrangerWidget *) self, ARRANGER_OBJECT_TYPE_MIDI_NOTE, self->hover_x,
    self->hover_y);
  int is_hit = obj != NULL;

  bool drum_mode = arranger_widget_get_drum_mode_enabled (self);

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (
        tool == TOOL_SELECT_NORMAL || tool == TOOL_SELECT_STRETCH
        || tool == TOOL_EDIT)
        {
          int is_resize_l = 0, is_resize_r = 0;

          if (is_hit)
            {
              is_resize_l = arranger_object_is_resize_l (
                obj, (int) self->hover_x - obj->full_rect.x);
              is_resize_r = arranger_object_is_resize_r (
                obj, (int) self->hover_x - obj->full_rect.x);
            }

          if (is_hit && is_resize_l && !drum_mode)
            {
              return ARRANGER_CURSOR_RESIZING_L;
            }
          else if (is_hit && is_resize_r && !drum_mode)
            {
              return ARRANGER_CURSOR_RESIZING_R;
            }
          else if (is_hit)
            {
              return ARRANGER_CURSOR_GRAB;
            }
          else
            {
              /* set cursor to whatever it is */
              if (tool == TOOL_EDIT)
                return ARRANGER_CURSOR_EDIT;
              else
                return ac;
            }
        }
      else if (P_TOOL == TOOL_EDIT)
        ac = ARRANGER_CURSOR_EDIT;
      else if (P_TOOL == TOOL_ERASER)
        ac = ARRANGER_CURSOR_ERASER;
      else if (P_TOOL == TOOL_RAMP)
        ac = ARRANGER_CURSOR_RAMP;
      else if (P_TOOL == TOOL_AUDITION)
        ac = ARRANGER_CURSOR_AUDITION;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_STARTING_ERASING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_STARTING_PANNING:
    case UI_OVERLAY_ACTION_PANNING:
      ac = ARRANGER_CURSOR_PANNING;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
    case UI_OVERLAY_ACTION_SELECTING:
      /* TODO depends on tool */
      break;
    case UI_OVERLAY_ACTION_AUTOFILLING:
      ac = ARRANGER_CURSOR_AUTOFILL;
      break;
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

/**
 * Gets the cursor based on the current hover
 * position.
 */
ArrangerCursor
arranger_widget_get_cursor (ArrangerWidget * self)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

  switch (self->type)
    {
    case TYPE (TIMELINE):
      ac = get_timeline_cursor (self, P_TOOL);
      break;
    case TYPE (AUDIO):
      ac = get_audio_arranger_cursor (self, P_TOOL);
      break;
    case TYPE (CHORD):
      ac = get_chord_arranger_cursor (self, P_TOOL);
      break;
    case TYPE (MIDI):
      ac = get_midi_arranger_cursor (self, P_TOOL);
      break;
    case TYPE (MIDI_MODIFIER):
      ac = get_midi_modifier_arranger_cursor (self, P_TOOL);
      break;
    case TYPE (AUTOMATION):
      ac = get_automation_arranger_cursor (self, P_TOOL);
      break;
    default:
      break;
    }

  return ac;
}

/**
 * Figures out which cursor should be used based
 * on the current state and then sets it.
 */
void
arranger_widget_refresh_cursor (ArrangerWidget * self)
{
  if (!gtk_widget_get_realized (GTK_WIDGET (self)))
    return;

  ArrangerCursor ac = arranger_widget_get_cursor (self);

  arranger_widget_set_cursor (self, ac);
}

/**
 * Toggles the mute status of the selection, based
 * on the mute status of the selected object.
 *
 * This creates an undoable action and executes it.
 */
void
arranger_widget_toggle_selections_muted (
  ArrangerWidget * self,
  ArrangerObject * clicked_object)
{
  g_return_if_fail (arranger_object_can_mute (clicked_object));

  GAction * action =
    g_action_map_lookup_action (G_ACTION_MAP (MAIN_WINDOW), "mute-selection");
  GVariant * var = g_variant_new_string ("timeline");
  g_action_activate (action, var);
}

/**
 * Scroll until the given object is visible.
 *
 * @param horizontal 1 for horizontal, 2 for
 *   vertical.
 * @param up Whether scrolling up or down.
 * @param padding Padding pixels.
 */
void
arranger_widget_scroll_until_obj (
  ArrangerWidget * self,
  ArrangerObject * obj,
  int              horizontal,
  int              up,
  int              left,
  int              padding)
{
  EditorSettings * settings = arranger_widget_get_editor_settings (self);
  g_return_if_fail (settings);

  int width = gtk_widget_get_width (GTK_WIDGET (self));
  int height = gtk_widget_get_height (GTK_WIDGET (self));
  if (horizontal)
    {
      int start_px = arranger_widget_pos_to_px (self, &obj->pos, 1);
      int end_px = arranger_widget_pos_to_px (self, &obj->end_pos, 1);

      /* adjust px for objects with non-global
       * positions */
      if (!arranger_object_type_has_global_pos (obj->type))
        {
          ArrangerObject * r_obj =
            (ArrangerObject *) clip_editor_get_region (CLIP_EDITOR);
          g_return_if_fail (r_obj);
          int tmp_px = arranger_widget_pos_to_px (self, &r_obj->pos, 1);
          start_px += tmp_px;
          end_px += tmp_px;
        }

      if (
        start_px <= settings->scroll_start_x
        || end_px >= settings->scroll_start_x + width)
        {
          if (left)
            {
              editor_settings_set_scroll_start_x (
                settings, start_px - padding, F_NO_VALIDATE);
            }
          else
            {
              int tmp = (end_px + padding) - width;
              editor_settings_set_scroll_start_x (settings, tmp, F_NO_VALIDATE);
            }
        }
    }
  else
    {
      arranger_object_set_full_rectangle (obj, self);
      int start_px = obj->full_rect.y;
      int end_px = obj->full_rect.y + obj->full_rect.height;
      if (
        start_px <= settings->scroll_start_y
        || end_px >= settings->scroll_start_y + height)
        {
          if (up)
            {
              editor_settings_set_scroll_start_y (
                settings, start_px - padding, F_NO_VALIDATE);
            }
          else
            {
              int tmp = (end_px + padding) - height;
              editor_settings_set_scroll_start_y (settings, tmp, F_NO_VALIDATE);
            }
        }
    }
}

/**
 * Returns whether any arranger is in the middle
 * of an action.
 */
bool
arranger_widget_any_doing_action (void)
{
#define CHECK_ARRANGER(arranger) \
  if (arranger && arranger->action != UI_OVERLAY_ACTION_NONE) \
    return true;

  CHECK_ARRANGER (MW_TIMELINE);
  CHECK_ARRANGER (MW_PINNED_TIMELINE);
  CHECK_ARRANGER (MW_MIDI_ARRANGER);
  CHECK_ARRANGER (MW_MIDI_MODIFIER_ARRANGER);
  CHECK_ARRANGER (MW_CHORD_ARRANGER);
  CHECK_ARRANGER (MW_AUTOMATION_ARRANGER);
  CHECK_ARRANGER (MW_AUDIO_ARRANGER);

#undef CHECK_ARRANGER

  return false;
}

/**
 * Returns the earliest possible position allowed
 * in this arranger (eg, 1.1.0.0 for timeline).
 */
void
arranger_widget_get_min_possible_position (ArrangerWidget * self, Position * pos)
{
  switch (self->type)
    {
    case TYPE (TIMELINE):
      position_set_to_bar (pos, 1);
      break;
    case TYPE (MIDI):
    case TYPE (MIDI_MODIFIER):
    case TYPE (CHORD):
    case TYPE (AUTOMATION):
    case TYPE (AUDIO):
      {
        ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
        g_return_if_fail (region);
        position_set_to_pos (pos, &((ArrangerObject *) region)->pos);
        position_change_sign (pos);
      }
      break;
    }
}

void
arranger_widget_handle_playhead_auto_scroll (ArrangerWidget * self, bool force)
{
  if (!TRANSPORT_IS_ROLLING && !force)
    return;

  bool scroll_edges = false;
  bool follow = false;
  if (self->type == ARRANGER_WIDGET_TYPE_TIMELINE)
    {
      scroll_edges =
        g_settings_get_boolean (S_UI, "timeline-playhead-scroll-edges");
      follow = g_settings_get_boolean (S_UI, "timeline-playhead-follow");
    }
  else
    {
      scroll_edges =
        g_settings_get_boolean (S_UI, "editor-playhead-scroll-edges");
      follow = g_settings_get_boolean (S_UI, "editor-playhead-follow");
    }

  GdkRectangle rect;
  arranger_widget_get_visible_rect (self, &rect);

  int buffer = 5;
  int playhead_x = arranger_widget_get_playhead_px (self);

  EditorSettings * settings = arranger_widget_get_editor_settings (self);
  if (follow)
    {
      /* scroll */
      editor_settings_set_scroll_start_x (
        settings, playhead_x - rect.width / 2, true);
      g_debug ("autoscrolling to follow playhead");
    }
  else if (scroll_edges)
    {
      buffer = 32;
      /* if playhead is after the visible range +
       * buffer or if before visible range - buffer,
       * scroll */
      if (
        playhead_x > ((rect.x + rect.width) - buffer)
        || playhead_x < rect.x + buffer)
        {
          editor_settings_set_scroll_start_x (
            settings, playhead_x - buffer, true);
          /*g_debug ("autoscrolling at playhead edges");*/
        }
    }
}

static gboolean
arranger_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  if (!gtk_widget_is_visible (widget))
    {
      return true;
    }
  ArrangerWidget * self = Z_ARRANGER_WIDGET (widget);
  self->queued_playhead_px = arranger_widget_get_playhead_px (self);

  gtk_widget_queue_draw (widget);

  /* auto scroll */
  arranger_widget_handle_playhead_auto_scroll (self, false);

  return G_SOURCE_CONTINUE;
}

/**
 * Runs the given function for each arranger.
 */
void
arranger_widget_foreach (ArrangerWidgetForeachFunc func)
{
  func (MW_TIMELINE);
  func (MW_PINNED_TIMELINE);
  func (MW_MIDI_ARRANGER);
  func (MW_MIDI_MODIFIER_ARRANGER);
  func (MW_CHORD_ARRANGER);
  func (MW_AUTOMATION_ARRANGER);
  func (MW_AUDIO_ARRANGER);
}

void
arranger_widget_setup (
  ArrangerWidget *   self,
  ArrangerWidgetType type,
  SnapGrid *         snap_grid)
{
  g_debug ("setting up arranger widget...");

  g_return_if_fail (self && type >= ARRANGER_WIDGET_TYPE_TIMELINE && snap_grid);
  self->type = type;
  self->snap_grid = snap_grid;

  int icon_texture_size = 12;
  self->region_icon_texture_size = icon_texture_size;
  switch (type)
    {
    case TYPE (TIMELINE):
      /* make drag dest */
      gtk_widget_add_css_class (GTK_WIDGET (self), "timeline-arranger");
      timeline_arranger_setup_drag_dest (self);

      /* create common textures */
      self->symbolic_link_texture = z_gdk_texture_new_from_icon_name (
        "emblem-symbolic-link", icon_texture_size, icon_texture_size, 1);
      self->music_note_16th_texture = z_gdk_texture_new_from_icon_name (
        "music-note-16th", icon_texture_size, icon_texture_size, 1);
      self->fork_awesome_snowflake_texture = z_gdk_texture_new_from_icon_name (
        "fork-awesome-snowflake-o", icon_texture_size, icon_texture_size, 1);
      self->media_playlist_repeat_texture = z_gdk_texture_new_from_icon_name (
        "gnome-icon-library-media-playlist-repeat-symbolic", icon_texture_size,
        icon_texture_size, 1);
      break;
    case TYPE (AUTOMATION):
      gtk_widget_add_css_class (GTK_WIDGET (self), "automation-arranger");
      self->ap_layout = z_cairo_create_pango_layout_from_string (
        GTK_WIDGET (self), "8", PANGO_ELLIPSIZE_NONE, 0);
      break;
    case TYPE (MIDI_MODIFIER):
      gtk_widget_add_css_class (GTK_WIDGET (self), "midi-modifier-arranger");
      self->vel_layout = z_cairo_create_pango_layout_from_string (
        GTK_WIDGET (self), "8", PANGO_ELLIPSIZE_NONE, 0);
      break;
    case TYPE (AUDIO):
      gtk_widget_add_css_class (GTK_WIDGET (self), "audio-arranger");
      self->audio_layout = z_cairo_create_pango_layout_from_string (
        GTK_WIDGET (self), "8", PANGO_ELLIPSIZE_NONE, 0);
    default:
      break;
    }

  self->debug_layout = z_cairo_create_pango_layout_from_string (
    GTK_WIDGET (self), "8", PANGO_ELLIPSIZE_NONE, 0);

  GtkEventControllerScroll * scroll_controller = GTK_EVENT_CONTROLLER_SCROLL (
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll", G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (scroll_controller));

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update", G_CALLBACK (drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end), self);
  g_signal_connect (
    G_OBJECT (self->drag), "cancel", G_CALLBACK (drag_cancel), self);
  g_signal_connect (
    G_OBJECT (self->click), "pressed", G_CALLBACK (click_pressed), self);
  g_signal_connect (
    G_OBJECT (self->click), "stopped", G_CALLBACK (click_stopped), self);
  g_signal_connect (
    G_OBJECT (self->right_click), "released", G_CALLBACK (on_right_click), self);

  GtkEventControllerKey * key_controller =
    GTK_EVENT_CONTROLLER_KEY (gtk_event_controller_key_new ());
  g_signal_connect (
    G_OBJECT (key_controller), "key-pressed",
    G_CALLBACK (arranger_widget_on_key_press), self);
  g_signal_connect (
    G_OBJECT (key_controller), "key-released",
    G_CALLBACK (arranger_widget_on_key_release), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (key_controller));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "motion", G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (motion_controller));

  GtkEventController * focus = gtk_event_controller_focus_new ();
  g_signal_connect (
    G_OBJECT (focus), "leave", G_CALLBACK (on_focus_leave), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (focus));

  gtk_widget_add_tick_callback (GTK_WIDGET (self), arranger_tick_cb, self, NULL);

  gtk_widget_set_focus_on_click (GTK_WIDGET (self), true);
  gtk_widget_set_focusable (GTK_WIDGET (self), true);

  g_debug ("done setting up arranger");
}

static void
dispose (ArrangerWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (arranger_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
finalize (ArrangerWidget * self)
{
  object_free_w_func_and_null (g_object_unref, self->vel_layout);
  object_free_w_func_and_null (g_object_unref, self->ap_layout);
  object_free_w_func_and_null (g_object_unref, self->audio_layout);
  object_free_w_func_and_null (g_object_unref, self->debug_layout);

  object_free_w_func_and_null (g_object_unref, self->symbolic_link_texture);
  object_free_w_func_and_null (g_object_unref, self->music_note_16th_texture);
  object_free_w_func_and_null (
    g_object_unref, self->fork_awesome_snowflake_texture);
  object_free_w_func_and_null (
    g_object_unref, self->media_playlist_repeat_texture);

  object_free_w_func_and_null (gsk_render_node_unref, self->loop_line_node);
  object_free_w_func_and_null (
    gsk_render_node_unref, self->clip_start_line_node);

  object_free_w_func_and_null (g_ptr_array_unref, self->hit_objs_to_draw);

  G_OBJECT_CLASS (arranger_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
arranger_widget_class_init (ArrangerWidgetClass * _klass)
{
  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
  oklass->dispose = (GObjectFinalizeFunc) dispose;

  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  wklass->snapshot = arranger_snapshot;
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_GROUP);

  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (wklass, "arranger");

  gtk_widget_class_add_binding (
    wklass, GDK_KEY_space, 0, z_gtk_simple_action_shortcut_func, "s",
    "play-pause", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_space, GDK_SHIFT_MASK, z_gtk_simple_action_shortcut_func,
    "s", "record-play", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_1, 0, z_gtk_simple_action_shortcut_func, "s",
    "select-or-stretch-mode", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_2, 0, z_gtk_simple_action_shortcut_func, "s", "edit-mode",
    NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_3, 0, z_gtk_simple_action_shortcut_func, "s", "cut-mode",
    NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_4, 0, z_gtk_simple_action_shortcut_func, "s", "eraser-mode",
    NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_5, 0, z_gtk_simple_action_shortcut_func, "s", "ramp-mode",
    NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_6, 0, z_gtk_simple_action_shortcut_func, "s",
    "audition-mode", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_M, GDK_SHIFT_MASK, z_gtk_simple_action_shortcut_func, "s",
    "mute-selection::global", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_less, GDK_SHIFT_MASK, z_gtk_simple_action_shortcut_func,
    "s", "nudge-selection::left", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_greater, GDK_SHIFT_MASK, z_gtk_simple_action_shortcut_func,
    "s", "nudge-selection::right", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_A, GDK_CONTROL_MASK, z_gtk_simple_action_shortcut_func, "s",
    "select-all", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_A, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    z_gtk_simple_action_shortcut_func, "s", "clear-selection", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_Delete, 0, z_gtk_simple_action_shortcut_func, "s", "delete",
    NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_Q, 0, z_gtk_simple_action_shortcut_func, "s",
    "quick-quantize::global", NULL);
  gtk_widget_class_add_binding (
    wklass, GDK_KEY_F2, 0, z_gtk_simple_action_shortcut_func, "s",
    "rename-arranger-object", NULL);
}

static void
arranger_widget_init (ArrangerWidget * self)
{
  self->first_draw = true;

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, "Arranger", -1);

  self->popover_menu = GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  /* make widget able to focus */
  gtk_widget_set_focus_on_click (GTK_WIDGET (self), true);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->drag), GTK_PHASE_CAPTURE);

  /* allow all buttons for drag */
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->drag), 0);

  self->click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  /* allow all buttons */
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->click), 0);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->click));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->click), GTK_PHASE_CAPTURE);

  self->right_click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->right_click));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_click), GDK_BUTTON_SECONDARY);

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
}
