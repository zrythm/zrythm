// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "dsp/snap_grid.h"
#include "gui/backend/audio_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "zrythm_app.h"

void
audio_arranger_widget_snap_range_r (
  ArrangerWidget * self,
  Position *       pos)
{
  if (self->resizing_range_start)
    {
      /* set range 1 at current point */
      ui_px_to_pos_editor (
        self->start_x, &AUDIO_SELECTIONS->sel_start, true);
      if (SNAP_GRID_ANY_SNAP (self->snap_grid) && !self->shift_held)
        {
          position_snap_simple (
            &AUDIO_SELECTIONS->sel_start, SNAP_GRID_EDITOR);
        }
      position_set_to_pos (
        &AUDIO_SELECTIONS->sel_end,
        &AUDIO_SELECTIONS->sel_start);

      self->resizing_range_start = false;
    }

  /* set range */
  if (SNAP_GRID_ANY_SNAP (self->snap_grid) && !self->shift_held)
    {
      position_snap_simple (pos, SNAP_GRID_EDITOR);
    }
  position_set_to_pos (&AUDIO_SELECTIONS->sel_end, pos);
  audio_selections_set_has_range (AUDIO_SELECTIONS, true);
}

/**
 * Returns whether the cursor is inside a fade
 * area.
 *
 * @param fade_in True to check for fade in, false
 *   to check for fade out.
 * @param resize True to check for whether resizing
 *   the fade (left <=> right), false to check
 *   for whether changing the fade curviness up/down.
 */
bool
audio_arranger_widget_is_cursor_in_fade (
  ArrangerWidget * self,
  double           x,
  double           y,
  bool             fade_in,
  bool             resize)
{
  ZRegion *        r = clip_editor_get_region (CLIP_EDITOR);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  g_return_val_if_fail (IS_REGION_AND_NONNULL (r), false);

  const int start_px =
    ui_pos_to_px_editor (&r_obj->pos, F_PADDING);
  const int end_px =
    ui_pos_to_px_editor (&r_obj->end_pos, F_PADDING);
  const int fade_in_px =
    start_px
    + ui_pos_to_px_editor (&r_obj->fade_in_pos, F_PADDING);
  const int fade_out_px =
    start_px
    + ui_pos_to_px_editor (&r_obj->fade_out_pos, F_PADDING);

  const int resize_padding = 4;

#if 0
  g_message (
    "x %f start px %d end px %d fade in px %d "
    "fade out px %d",
    x, start_px, end_px, fade_in_px, fade_out_px);
#endif

  int fade_in_px_w_padding = fade_in_px + resize_padding;
  int fade_out_px_w_padding = fade_out_px - resize_padding;

  if (fade_in && x >= start_px && x <= fade_in_px_w_padding)
    {
      if (resize)
        return x >= fade_in_px - resize_padding;
      else
        return true;
    }
  if (!fade_in && x >= fade_out_px_w_padding && x < end_px)
    {
      if (resize)
        return x <= fade_out_px + resize_padding;
      else
        return true;
    }

  return false;
}

NONNULL static double
get_region_gain_y (ArrangerWidget * self, ZRegion * region)
{
  int   height = gtk_widget_get_height (GTK_WIDGET (self));
  float gain_fader_val =
    math_get_fader_val_from_amp (region->gain);
  return height * (1.0 - (double) gain_fader_val);
}

/**
 * Returns whether the cursor touches the gain line.
 */
bool
audio_arranger_widget_is_cursor_gain (
  ArrangerWidget * self,
  double           x,
  double           y)
{
  ZRegion *        r = clip_editor_get_region (CLIP_EDITOR);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  g_return_val_if_fail (IS_REGION_AND_NONNULL (r), false);

  const int start_px =
    ui_pos_to_px_editor (&r_obj->pos, F_PADDING);
  const int end_px =
    ui_pos_to_px_editor (&r_obj->end_pos, F_PADDING);

  double gain_y = get_region_gain_y (self, r);

  const int padding = 4;

  /*g_message ("y %f gain y %f", y, gain_y);*/

  return x >= start_px && x < end_px && y >= gain_y - padding
         && y <= gain_y + padding;
}

UiOverlayAction
audio_arranger_widget_get_action_on_drag_begin (
  ArrangerWidget * self)
{
  self->action = UI_OVERLAY_ACTION_NONE;
  ArrangerCursor   cursor = arranger_widget_get_cursor (self);
  ZRegion *        r = clip_editor_get_region (CLIP_EDITOR);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  g_return_val_if_fail (IS_REGION_AND_NONNULL (r), false);

  switch (cursor)
    {
    case ARRANGER_CURSOR_RESIZING_UP:
      self->fval_at_start = r->gain;
      return UI_OVERLAY_ACTION_RESIZING_UP;
    case ARRANGER_CURSOR_RESIZING_UP_FADE_IN:
      self->dval_at_start = r_obj->fade_in_opts.curviness;
      return UI_OVERLAY_ACTION_RESIZING_UP_FADE_IN;
    case ARRANGER_CURSOR_RESIZING_UP_FADE_OUT:
      self->dval_at_start = r_obj->fade_out_opts.curviness;
      return UI_OVERLAY_ACTION_RESIZING_UP_FADE_OUT;
    /* note cursor is opposite */
    case ARRANGER_CURSOR_RESIZING_R_FADE:
      position_set_to_pos (
        &self->fade_pos_at_start, &r_obj->fade_in_pos);
      return UI_OVERLAY_ACTION_RESIZING_L_FADE;
    case ARRANGER_CURSOR_RESIZING_L_FADE:
      position_set_to_pos (
        &self->fade_pos_at_start, &r_obj->fade_out_pos);
      return UI_OVERLAY_ACTION_RESIZING_R_FADE;
    case ARRANGER_CURSOR_RANGE:
      return UI_OVERLAY_ACTION_RESIZING_R;
    case ARRANGER_CURSOR_SELECT:
      return UI_OVERLAY_ACTION_STARTING_SELECTION;
    default:
      break;
    }

  g_return_val_if_reached (UI_OVERLAY_ACTION_SELECTING);
}

/**
 * Handle fade in/out curviness drag.
 */
void
audio_arranger_widget_fade_up (
  ArrangerWidget * self,
  double           offset_y,
  bool             fade_in)
{
  ZRegion *        r = clip_editor_get_region (CLIP_EDITOR);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  g_return_if_fail (IS_REGION_AND_NONNULL (r));

  CurveOptions * opts =
    fade_in ? &r_obj->fade_in_opts : &r_obj->fade_out_opts;
  double delta = (self->last_offset_y - offset_y) * 0.008;
  opts->curviness = CLAMP (opts->curviness + delta, -1.0, 1.0);

  if (fade_in)
    {
      EVENTS_PUSH (ET_AUDIO_REGION_FADE_IN_CHANGED, r);
    }
  else
    {
      EVENTS_PUSH (ET_AUDIO_REGION_FADE_OUT_CHANGED, r);
    }
}

void
audio_arranger_widget_update_gain (
  ArrangerWidget * self,
  double           offset_y)
{
  ZRegion * r = clip_editor_get_region (CLIP_EDITOR);
  g_return_if_fail (IS_REGION_AND_NONNULL (r));

  int height = gtk_widget_get_height (GTK_WIDGET (self));

  /* get current Y value */
  double gain_y = self->start_y + offset_y;

  /* convert to fader value (0.0 - 1.0) and clamp
   * to limits */
  double gain_fader = CLAMP (1.0 - gain_y / height, 0.02, 1.0);

  /* set */
  r->gain = math_get_amp_val_from_fader ((float) gain_fader);

  EVENTS_PUSH (ET_AUDIO_REGION_GAIN_CHANGED, r);
}

/**
 * Updates the fade position during drag update.
 *
 * @param pos Absolute position in the editor.
 * @param fade_in Whether we are resizing the fade in
 *   or fade out position.
 * @parram dry_run Don't resize; just check
 *   if the resize is allowed.
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
audio_arranger_widget_snap_fade (
  ArrangerWidget * self,
  Position *       pos,
  bool             fade_in,
  bool             dry_run)
{
  ZRegion *        r = clip_editor_get_region (CLIP_EDITOR);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  g_return_val_if_fail (IS_REGION_AND_NONNULL (r), -1);

  /* get delta with region's start pos */
  double delta =
    position_to_ticks (pos) -
    (position_to_ticks (
       &r_obj->pos) +
     position_to_ticks (
       fade_in
       ? &r_obj->fade_in_pos
       : &r_obj->fade_out_pos));

  /* new start pos, calculated by
   * adding delta to the region's original start
   * pos */
  Position new_pos;
  position_set_to_pos (
    &new_pos,
    fade_in ? &r_obj->fade_in_pos : &r_obj->fade_out_pos);
  position_add_ticks (&new_pos, delta);

  /* negative positions not allowed */
  if (!position_is_positive (&new_pos))
    return -1;

  if (SNAP_GRID_ANY_SNAP (self->snap_grid) && !self->shift_held)
    {
      Track * track = arranger_object_get_track (r_obj);
      position_snap (
        &self->fade_pos_at_start, &new_pos, track, NULL,
        self->snap_grid);
    }

  if (!arranger_object_is_position_valid (
        r_obj, &new_pos,
        fade_in ? ARRANGER_OBJECT_POSITION_TYPE_FADE_IN
                : ARRANGER_OBJECT_POSITION_TYPE_FADE_OUT))
    {
      return -1;
    }

  if (!dry_run)
    {
      double diff =
        position_to_ticks (&new_pos)
        - position_to_ticks (
          fade_in ? &r_obj->fade_in_pos : &r_obj->fade_out_pos);
      GError * err = NULL;
      bool     success = arranger_object_resize (
        r_obj, fade_in, ARRANGER_OBJECT_RESIZE_FADE, diff,
        true, &err);
      if (!success)
        {
          HANDLE_ERROR_LITERAL (
            err, "Failed to resize object");
          return -1;
        }
    }

  if (fade_in)
    {
      EVENTS_PUSH (ET_AUDIO_REGION_FADE_IN_CHANGED, r);
    }
  else
    {
      EVENTS_PUSH (ET_AUDIO_REGION_FADE_OUT_CHANGED, r);
    }

  return 0;
}
