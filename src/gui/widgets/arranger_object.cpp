// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/automation_region.h"
#include "dsp/chord_track.h"
#include "dsp/fade.h"
#include "dsp/marker_track.h"
#include "dsp/tracklist.h"
#include "gui/backend/arranger_object.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/chord_region.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_notebook.h"
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
#include "gui/widgets/tracklist.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n-lib.h>

#define TYPE(x) ArrangerObjectType::ARRANGER_OBJECT_TYPE_##x

/**
 * Returns if the current position is for resizing
 * L.
 *
 * @param x X in local coordinates.
 */
bool
arranger_object_is_resize_l (ArrangerObject * self, const int x)
{
  if (!arranger_object_type_has_length (self->type))
    return false;

  if (self->full_rect.width < UI_RESIZE_CURSOR_SPACE * 3)
    return false;

  if (x < UI_RESIZE_CURSOR_SPACE)
    {
      return true;
    }
  return false;
}

/**
 * Returns if the current position is for resizing
 * R.
 *
 * @param x X in local coordinates.
 */
bool
arranger_object_is_resize_r (ArrangerObject * self, const int x)
{
  if (!arranger_object_type_has_length (self->type))
    return false;

  if (self->full_rect.width < UI_RESIZE_CURSOR_SPACE * 3)
    return false;

  long     size_frames = self->end_pos.frames - self->pos.frames;
  Position pos;
  position_from_frames (&pos, size_frames);
  int width_px =
    arranger_widget_pos_to_px (arranger_object_get_arranger (self), &pos, 0);

  if (x > width_px - UI_RESIZE_CURSOR_SPACE)
    {
      return true;
    }
  return false;
}

/**
 * Returns if the current position is for moving the
 * fade in/out mark (timeline only).
 *
 * @param in True for fade in, false for fade out.
 * @param x X in local coordinates.
 * @param only_handle Whether to only check if this
 *   is inside the fade handle. If this is false,
 *   \ref only_outer will be considered.
 * @param only_outer Whether to only check if this
 *   is inside the fade's outer (unplayed) region.
 *   If this is false, the whole fade area will
 *   be considered.
 * @param check_lane Whether to check the lane
 *   region instead of the main one (if region).
 */
bool
arranger_object_is_fade (
  ArrangerObject * self,
  bool             in,
  const int        x,
  int              y,
  bool             only_handle,
  bool             only_outer,
  bool             check_lane)
{
  if (!arranger_object_can_fade (self))
    return false;

  const int    fade_in_px = ui_pos_to_px_timeline (&self->fade_in_pos, 0);
  const int    fade_out_px = ui_pos_to_px_timeline (&self->fade_out_pos, 0);
  const int    fade_pt_halfwidth = ARRANGER_OBJECT_FADE_POINT_HALFWIDTH;
  GdkRectangle full_rect;
  Track *      track = arranger_object_get_track (self);
  if (check_lane && track->lanes_visible)
    {
      Region * r = (Region *) self;
      region_get_lane_full_rect (r, &full_rect);
      y -= full_rect.y - self->full_rect.y;
    }
  else
    {
      full_rect = self->full_rect;
    }

  bool ret = false;
  if (only_handle)
    {
      if (in)
        {
          ret =
            x >= fade_in_px - fade_pt_halfwidth
            && x <= fade_in_px + fade_pt_halfwidth && y <= fade_pt_halfwidth;
        }
      else
        {
          ret =
            x >= fade_out_px - fade_pt_halfwidth
            && x <= fade_out_px + fade_pt_halfwidth && y <= fade_pt_halfwidth;
        }
    }
  else if (only_outer)
    {
      if (in)
        {
          ret = x <= fade_in_px && fade_in_px > 0 && (double) y <= full_rect.height * (1.0 - fade_get_y_normalized ((double) x / fade_in_px, &self->fade_in_opts, 1));
        }
      else
        {
          ret = x >= fade_out_px && full_rect.width - fade_out_px > 0 && (double) y <= full_rect.height * (1.0 - fade_get_y_normalized ((double) (x - fade_out_px) / (full_rect.width - fade_out_px), &self->fade_out_opts, 0));
        }
    }
  else
    {
      if (in)
        {
          ret = x <= fade_in_px;
        }
      else
        {
          ret = x >= fade_out_px;
        }
    }

  return ret;
}

/**
 * Returns if the current position is for resizing
 * up (eg, Velocity).
 *
 * @param x X in local coordinates.
 * @param y Y in local coordinates.
 */
bool
arranger_object_is_resize_up (ArrangerObject * self, const int x, const int y)
{
  if (self->type == ArrangerObjectType::ARRANGER_OBJECT_TYPE_VELOCITY)
    {
      if (y < VELOCITY_RESIZE_THRESHOLD)
        return 1;
    }
  else if (
    self->type == ArrangerObjectType::ARRANGER_OBJECT_TYPE_AUTOMATION_POINT)
    {
      AutomationPoint * ap = (AutomationPoint *) self;
      int               curve_up = automation_point_curves_up (ap);
      if (curve_up)
        {
          if (
            x > AP_WIDGET_POINT_SIZE
            || self->full_rect.height - y > AP_WIDGET_POINT_SIZE)
            return 1;
        }
      else
        {
          if (x > AP_WIDGET_POINT_SIZE || y > AP_WIDGET_POINT_SIZE)
            return 1;
        }
    }
  return 0;
}

bool
arranger_object_is_resize_loop (
  ArrangerObject * self,
  const int        y,
  bool             ctrl_pressed)
{
  if (
    !arranger_object_type_has_length (self->type)
    || !arranger_object_type_can_loop (self->type))
    return 0;

  if (self->type == ArrangerObjectType::ARRANGER_OBJECT_TYPE_REGION)
    {
      Region * r = (Region *) self;
      if (r->id.type == RegionType::REGION_TYPE_AUDIO && !ctrl_pressed)
        {
          return 1;
        }

      if (region_is_looped (r))
        {
          return 1;
        }

      /* TODO */
      int height_px = 60;
      /*gtk_widget_get_height (*/
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
 * Returns if the current position is for renaming
 * the object.
 *
 * @param x X in local coordinates.
 * @param y Y in local coordinates.
 */
bool
arranger_object_is_rename (ArrangerObject * self, const int x, const int y)
{
  /* disable for now */
  return false;

  if (self->type != ArrangerObjectType::ARRANGER_OBJECT_TYPE_REGION)
    return false;

  GdkRectangle rect = self->last_name_rect;
  /* make the clickable height area a little smaller */
  rect.height = MAX (1, rect.height - 4);
  if (ui_is_point_in_rect_hit (&rect, true, true, x, y, 0, 0))
    {
      return true;
    }

  return false;
}

bool
arranger_object_should_show_cut_lines (ArrangerObject * self, bool alt_pressed)
{
  if (!arranger_object_type_has_length (self->type))
    return 0;

  switch (P_TOOL)
    {
    case Tool::TOOL_SELECT:
      if (alt_pressed)
        return 1;
      else
        return 0;
      break;
    case Tool::TOOL_CUT:
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
get_automation_point_y (AutomationPoint * ap, ArrangerWidget * arranger)
{
  /* ratio of current value in the range */
  float ap_ratio = ap->normalized_val;

  int allocated_h = gtk_widget_get_height (GTK_WIDGET (arranger));
  allocated_h -= AUTOMATION_ARRANGER_VPADDING * 2;
  int point = allocated_h - (int) (ap_ratio * (float) allocated_h);
  return point + AUTOMATION_ARRANGER_VPADDING;
}

/**
 * Gets the full rectangle for a linked object.
 */
int
arranger_object_get_full_rect_x_for_region_child (
  ArrangerObject * self,
  Region *         region,
  GdkRectangle *   full_rect)
{
  g_return_val_if_fail (region, 0);
  ArrangerObject * region_obj = (ArrangerObject *) region;

  double   region_start_ticks = region_obj->pos.ticks;
  Position tmp;

  /* use absolute position */
  position_from_ticks (&tmp, region_start_ticks + self->pos.ticks);
  return ui_pos_to_px_editor (&tmp, 1);
}

/**
 */
void
arranger_object_set_full_rectangle (
  ArrangerObject * self,
  ArrangerWidget * arranger)
{
  g_return_if_fail (
    Z_IS_ARRANGER_WIDGET (arranger) && IS_ARRANGER_OBJECT (self));

#define WARN_IF_HAS_NEGATIVE_DIMENSIONS \
  if (self->full_rect.x < 0) \
    { \
      int diff = -self->full_rect.x; \
      self->full_rect.x = 0; \
      self->full_rect.width -= diff; \
      if (self->full_rect.width < 1) \
        { \
          self->full_rect.width = 1; \
        } \
    } \
  g_warn_if_fail ( \
    self->full_rect.x >= 0 && self->full_rect.y >= 0 \
    && self->full_rect.width >= 0 && self->full_rect.height >= 0)

  switch (self->type)
    {
    case TYPE (CHORD_OBJECT):
      {
        ChordObject *     co = (ChordObject *) self;
        ChordDescriptor * descr = chord_object_get_chord_descriptor (co);

        Region *         region = arranger_object_get_region (self);
        ArrangerObject * region_obj = (ArrangerObject *) region;

        double   region_start_ticks = region_obj->pos.ticks;
        Position tmp;
        int      adj_px_per_key =
          chord_editor_space_widget_get_chord_height (MW_CHORD_EDITOR_SPACE) + 1;

        /* use absolute position */
        position_from_ticks (&tmp, region_start_ticks + self->pos.ticks);
        self->full_rect.x = ui_pos_to_px_editor (&tmp, 1);
        self->full_rect.y = adj_px_per_key * co->chord_index;

        char chord_str[100];
        chord_descriptor_to_string (descr, chord_str);

        chord_object_recreate_pango_layouts ((ChordObject *) self);
        self->full_rect.width =
          self->textw + CHORD_OBJECT_WIDGET_TRIANGLE_W
          + Z_CAIRO_TEXT_PADDING * 2;

        self->full_rect.width =
          self->textw + CHORD_OBJECT_WIDGET_TRIANGLE_W
          + Z_CAIRO_TEXT_PADDING * 2;

        self->full_rect.height = adj_px_per_key;

        WARN_IF_HAS_NEGATIVE_DIMENSIONS;
      }
      break;
    case TYPE (AUTOMATION_POINT):
      {
        AutomationPoint * ap = (AutomationPoint *) self;
        Region *          region = arranger_object_get_region (self);
        ArrangerObject *  region_obj = (ArrangerObject *) region;

        /* use absolute position */
        double   region_start_ticks = region_obj->pos.ticks;
        Position tmp;
        position_from_ticks (&tmp, region_start_ticks + self->pos.ticks);
        self->full_rect.x =
          ui_pos_to_px_editor (&tmp, 1) - AP_WIDGET_POINT_SIZE / 2;

        AutomationPoint * next_ap = automation_region_get_next_ap (
          region, ap, true, arranger->action == UI_OVERLAY_ACTION_MOVING_COPY);

        if (next_ap)
          {
            ArrangerObject * next_obj = (ArrangerObject *) next_ap;

            /* get relative position from the start AP to the next ap. */
            position_from_ticks (&tmp, next_obj->pos.ticks - self->pos.ticks);

            /* width is the relative position in px plus half an
             * AP_WIDGET_POINT_SIZE for each side */
            self->full_rect.width =
              AP_WIDGET_POINT_SIZE + ui_pos_to_px_editor (&tmp, 0);

            g_warn_if_fail (self->full_rect.width >= 0);

            int cur_y = get_automation_point_y (ap, arranger);
            int next_y = get_automation_point_y (next_ap, arranger);

            self->full_rect.y = MAX (
              (cur_y > next_y ? next_y : cur_y) - AP_WIDGET_POINT_SIZE / 2, 0);

            /* make sure y is not negative */

            /* height is the relative relative diff in px between the two points
             * plus half an AP_WIDGET_POINT_SIZE for each side */
            self->full_rect.height =
              (cur_y > next_y ? cur_y - next_y : next_y - cur_y)
              + AP_WIDGET_POINT_SIZE;
            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
        else
          {
            self->full_rect.y = MAX (
              get_automation_point_y (ap, arranger) - AP_WIDGET_POINT_SIZE / 2,
              0);

            self->full_rect.width = AP_WIDGET_POINT_SIZE;
            self->full_rect.height = AP_WIDGET_POINT_SIZE;

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
      }
      break;
    case TYPE (REGION):
      {
        Region * region = (Region *) self;
        Track *  track = arranger_object_get_track (self);

#if 0
        if (!track->widget)
          track->widget = track_widget_new (track);
#endif

        self->full_rect.x = ui_pos_to_px_timeline (&self->pos, true);
        Position tmp;
        position_from_ticks (&tmp, self->end_pos.ticks - self->pos.ticks);
        self->full_rect.width = ui_pos_to_px_timeline (&tmp, false) - 1;
        if (self->full_rect.width < 1)
          {
            self->full_rect.width = 1;
          }

        graphene_point_t wpt = Z_GRAPHENE_POINT_INIT (0.f, 0.f);
        if (track->widget)
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (0, 0);
            bool             success = gtk_widget_compute_point (
              (GtkWidget *) (track->widget),
              arranger->is_pinned
                            ? (GtkWidget *) arranger
                            : (GtkWidget *) MW_TRACKLIST->unpinned_box,
              &tmp_pt, &wpt);
            g_return_if_fail (success);
            /* for some reason it returns a few
             * negatives at first */
            if (wpt.y < 0.f)
              wpt.y = 0.f;
          }

        if (region->id.type == RegionType::REGION_TYPE_CHORD)
          {
            chord_region_recreate_pango_layouts (region);

            self->full_rect.y = (int) wpt.y;
            /* full height minus the space the
             * scales would require, plus some
             * padding */
            self->full_rect.height =
              (int) track->main_height
              - (self->texth + Z_CAIRO_TEXT_PADDING * 4);

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
        else if (region->id.type == RegionType::REGION_TYPE_AUTOMATION)
          {
            AutomationTrack * at = region_get_automation_track (region);
            g_return_if_fail (at);
            if (!at->created || !track->automation_visible)
              return;

            self->full_rect.y = (int) wpt.y + at->y;
            self->full_rect.height = (int) at->height;

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
        else
          {
            self->full_rect.y = (int) wpt.y;
            self->full_rect.height = (int) track->main_height;

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
        Region *   region = arranger_object_get_region (self);
        g_return_if_fail (region);
        ArrangerObject * region_obj = (ArrangerObject *) region;
        Track *          track = arranger_object_get_track (self);

        double   region_start_ticks = region_obj->pos.ticks;
        Position tmp;
        double   adj_px_per_key = MW_PIANO_ROLL_KEYS->px_per_key + 1.0;

        /* use absolute position */
        position_from_ticks (&tmp, region_start_ticks + self->pos.ticks);
        self->full_rect.x = ui_pos_to_px_editor (&tmp, 1);
        const MidiNoteDescriptor * descr =
          piano_roll_find_midi_note_descriptor_by_val (
            PIANO_ROLL, track->drum_mode, mn->val);
        self->full_rect.y = (int) (adj_px_per_key * (127 - descr->value));

        self->full_rect.height = (int) adj_px_per_key;
        if (track->drum_mode)
          {
            self->full_rect.width = self->full_rect.height;
            self->full_rect.x -= self->full_rect.width / 2;

            /*WARN_IF_HAS_NEGATIVE_DIMENSIONS;*/
          }
        else
          {
            /* use absolute position */
            position_from_ticks (&tmp, region_start_ticks + self->end_pos.ticks);
            self->full_rect.width =
              ui_pos_to_px_editor (&tmp, 1) - self->full_rect.x;

            WARN_IF_HAS_NEGATIVE_DIMENSIONS;
          }
      }
      break;
    case TYPE (SCALE_OBJECT):
      {
        Track * track = P_CHORD_TRACK;

        graphene_point_t wpt = Z_GRAPHENE_POINT_INIT (0.f, 0.f);
        if (track->widget)
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (0, 0);
            bool             success = gtk_widget_compute_point (
              (GtkWidget *) (track->widget), (GtkWidget *) (arranger), &tmp_pt,
              &wpt);
            g_return_if_fail (success);
            /* for some reason it returns a few
             * negatives at first */
            if (wpt.y < 0.f)
              wpt.y = 0.f;
          }

        self->full_rect.x = ui_pos_to_px_timeline (&self->pos, 1);
        scale_object_recreate_pango_layouts ((ScaleObject *) self);
        self->full_rect.width =
          self->textw + SCALE_OBJECT_WIDGET_TRIANGLE_W
          + Z_CAIRO_TEXT_PADDING * 2;

        int obj_height = self->texth + Z_CAIRO_TEXT_PADDING * 2;
        self->full_rect.y =
          ((int) wpt.y + (int) track->main_height) - obj_height;
        self->full_rect.height = obj_height;

        WARN_IF_HAS_NEGATIVE_DIMENSIONS;
      }
      break;
    case TYPE (MARKER):
      {
        Track * track = P_MARKER_TRACK;

        graphene_point_t wpt = Z_GRAPHENE_POINT_INIT (0.f, 0.f);
        if (track->widget)
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (0, 0);
            bool             success = gtk_widget_compute_point (
              (GtkWidget *) (track->widget), (GtkWidget *) (arranger), &tmp_pt,
              &wpt);
            g_return_if_fail (success);
            /* for some reason it returns a few
             * negatives at first */
            if (wpt.y < 0.f)
              wpt.y = 0.f;
          }

        self->full_rect.x = ui_pos_to_px_timeline (&self->pos, 1);
        marker_recreate_pango_layouts ((Marker *) self);
        self->full_rect.width =
          self->textw + MARKER_WIDGET_TRIANGLE_W + Z_CAIRO_TEXT_PADDING * 2;

        int global_y_start = (int) wpt.y + (int) track->main_height;
        int obj_height = MIN (
          (int) track->main_height, self->texth + Z_CAIRO_TEXT_PADDING * 2);
        self->full_rect.y = global_y_start - obj_height;
        self->full_rect.height = obj_height;

        WARN_IF_HAS_NEGATIVE_DIMENSIONS;
      }
      break;
    case TYPE (VELOCITY):
      {
        /* use transient or non transient note
         * depending on which is visible */
        Velocity * vel = (Velocity *) self;
        MidiNote * mn = velocity_get_midi_note (vel);
        g_return_if_fail (mn);
        ArrangerObject * mn_obj = (ArrangerObject *) mn;
        Region *         region = arranger_object_get_region (mn_obj);
        g_return_if_fail (region);
        ArrangerObject * region_obj = (ArrangerObject *) region;

        /* use absolute position */
        double   region_start_ticks = region_obj->pos.ticks;
        Position tmp;
        position_from_ticks (&tmp, region_start_ticks + mn_obj->pos.ticks);
        self->full_rect.x = ui_pos_to_px_editor (&tmp, 1);

        /* adjust x to start before the MIDI note
         * so that velocity appears centered at
         * start of MIDI note */
        self->full_rect.x -= VELOCITY_WIDTH / 2;

        int height = gtk_widget_get_height (GTK_WIDGET (arranger));

        int vel_px = (int) ((float) height * ((float) vel->vel / 127.f));
        self->full_rect.y = height - vel_px;

        self->full_rect.width = VELOCITY_WIDTH;
        self->full_rect.height = vel_px;

        WARN_IF_HAS_NEGATIVE_DIMENSIONS;

        /* adjust for circle radius */
        self->full_rect.height += VELOCITY_WIDTH / 2;
        self->full_rect.y -= VELOCITY_WIDTH / 2;
      }
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  bool drum_mode = false;
  if (self->type == TYPE (MIDI_NOTE) || self->type == TYPE (VELOCITY))
    {
      Track * track = arranger_object_get_track (self);
      g_return_if_fail (IS_TRACK_AND_NONNULL (track));
      drum_mode = track->drum_mode;
    }

  if (
    (self->full_rect.x < 0 && self->type != TYPE (MIDI_NOTE) && !drum_mode)
    || (self->full_rect.y < 0 && self->type != TYPE (VELOCITY))
    || (self->full_rect.y < -VELOCITY_WIDTH / 2 && self->type == TYPE (VELOCITY))
    || self->full_rect.width < 0 || self->full_rect.height < 0)
    {
      g_message ("Object:");
      arranger_object_print (self);
      g_warning (
        "The full rectangle of widget %p has negative dimensions: (%d,%d) w: %d h: %d. This should not happen. A rendering error is expected to occur.",
        self, self->full_rect.x, self->full_rect.y, self->full_rect.width,
        self->full_rect.height);
    }

#undef WARN_IF_HAS_NEGATIVE_DIMENSIONS

  /* make sure width/height are > 0 */
  if (self->full_rect.width < 1)
    {
      self->full_rect.width = 1;
    }
  if (self->full_rect.height < 1)
    {
      self->full_rect.height = 1;
    }
}

int
arranger_object_get_draw_rectangle (
  ArrangerObject * self,
  GdkRectangle *   parent_rect,
  GdkRectangle *   full_rect,
  GdkRectangle *   draw_rect)
{
  g_return_val_if_fail (full_rect->width > 0, false);

  if (!ui_rectangle_overlap (parent_rect, full_rect))
    return 0;

  draw_rect->x = MAX (full_rect->x, parent_rect->x);
  draw_rect->width = MIN (
    (parent_rect->x + parent_rect->width) - draw_rect->x,
    (full_rect->x + full_rect->width) - draw_rect->x);
  g_warn_if_fail (draw_rect->width >= 0);
  draw_rect->y = MAX (full_rect->y, parent_rect->y);
  draw_rect->height = MIN (
    (parent_rect->y + parent_rect->height) - draw_rect->y,
    (full_rect->y + full_rect->height) - draw_rect->y);
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
 * @param rect Rectangle in the arranger.
 */
void
arranger_object_draw (
  ArrangerObject * self,
  ArrangerWidget * arranger,
  GtkSnapshot *    snapshot,
  GdkRectangle *   rect)
{
#if 0
  if (!ui_rectangle_overlap (
        &self->full_rect, rect))
    {
      g_debug (
        "%s: object not visible, skipping",
        __func__);
    }
#endif

  switch (self->type)
    {
    case TYPE (AUTOMATION_POINT):
      automation_point_draw (
        (AutomationPoint *) self, snapshot, rect, arranger->ap_layout);
      break;
    case TYPE (REGION):
      region_draw ((Region *) self, snapshot, rect);
      break;
    case TYPE (MIDI_NOTE):
      midi_note_draw ((MidiNote *) self, snapshot);
      break;
    case TYPE (MARKER):
      marker_draw ((Marker *) self, snapshot);
      break;
    case TYPE (SCALE_OBJECT):
      scale_object_draw ((ScaleObject *) self, snapshot);
      break;
    case TYPE (CHORD_OBJECT):
      chord_object_draw ((ChordObject *) self, snapshot);
      break;
    case TYPE (VELOCITY):
      velocity_draw ((Velocity *) self, snapshot);
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

bool
arranger_object_should_orig_be_visible (
  ArrangerObject * self,
  ArrangerWidget * arranger)
{
  if (!ZRYTHM_HAVE_UI)
    {
      return false;
    }

  if (!arranger)
    {
      arranger = arranger_object_get_arranger (self);
      g_return_val_if_fail (arranger, false);
    }

  /* check trans/non-trans visibility */
  if (
    ARRANGER_WIDGET_GET_ACTION (arranger, MOVING)
    || ARRANGER_WIDGET_GET_ACTION (arranger, CREATING_MOVING))
    {
      return false;
    }
  else if (
    ARRANGER_WIDGET_GET_ACTION (arranger, MOVING_COPY)
    || ARRANGER_WIDGET_GET_ACTION (arranger, MOVING_LINK))
    {
      return true;
    }
  else
    {
      return false;
    }
}

bool
arranger_object_is_hovered (ArrangerObject * self, ArrangerWidget * arranger)
{
  if (!arranger)
    {
      arranger = arranger_object_get_arranger (self);
    }
  return arranger->hovered_object == self;
}

bool
arranger_object_is_hovered_or_start_object (
  ArrangerObject * self,
  ArrangerWidget * arranger)
{
  if (!arranger)
    {
      arranger = arranger_object_get_arranger (self);
    }
  return arranger->hovered_object == self || arranger->start_object == self;
}
