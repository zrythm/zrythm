/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/undo_manager.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/exporter.h"
#include "audio/marker_track.h"
#include "gui/backend/arranger_object.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/dialogs/bounce_dialog.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "gui/widgets/dialogs/export_midi_file_dialog.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/strv_builder.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#define ACTION_IS(x) \
  (self->action == UI_OVERLAY_ACTION_##x)

/**
 * Hides the cut dashed line from hovered regions
 * and redraws them.
 *
 * Used when alt was unpressed.
 */
void
timeline_arranger_widget_set_cut_lines_visible (
  ArrangerWidget * self)
{
#if 0
  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_REGION,
      self->hover_x,
      self->hover_y);

  if (obj)
    {
      ArrangerObjectWidget * obj_w =
        Z_ARRANGER_OBJECT_WIDGET (obj->widget);
      ARRANGER_OBJECT_WIDGET_GET_PRIVATE (obj_w);

      GdkModifierType mask;
      z_gtk_widget_get_mask (
        GTK_WIDGET (obj_w),
        &mask);
      int alt_pressed =
        mask & GDK_ALT_MASK;

      /* if not cutting hide the cut line
       * from the region immediately */
      int show_cut =
        arranger_object_widget_should_show_cut_lines (
                                                            obj_w,
          alt_pressed);

      if (show_cut != ao_prv->show_cut)
        {
          ao_prv->show_cut = show_cut;

          gtk_widget_queue_draw (
            GTK_WIDGET (obj_w));
        }
    }
#endif
}

TrackLane *
timeline_arranger_widget_get_track_lane_at_y (
  ArrangerWidget * self,
  double y)
{
  Track * track =
    timeline_arranger_widget_get_track_at_y (
      self, y);
  if (!track || !track->lanes_visible)
    return NULL;

  /* y local to track */
  int y_local =
    track_widget_get_local_y (
      track->widget, self, (int) y);

  TrackLane * lane;
  for (int j = 0; j < track->num_lanes; j++)
    {
      lane = track->lanes[j];

      if (y_local >= lane->y &&
          y_local < lane->y + lane->height)
        return lane;
    }

  return NULL;
}

Track *
timeline_arranger_widget_get_track_at_y (
  ArrangerWidget * self,
  double y)
{
  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (
        /* ignore invisible tracks */
        !track->visible ||
        /* ignore tracks in the other timeline */
        self->is_pinned != track_is_pinned (track))
        continue;

      if (!track_get_should_be_visible (track))
        continue;

      if (!track->widget)
        {
          g_message (
            "no track widget for %s", track->name);
          continue;
        }

      if (ui_is_child_hit (
            GTK_WIDGET (self),
            GTK_WIDGET (track->widget),
            0, 1, 0, y, 0, 1))
        return track;
    }

  return NULL;
}

/**
 * Returns the hit AutomationTrack at y.
 */
AutomationTrack *
timeline_arranger_widget_get_at_at_y (
  ArrangerWidget * self,
  double           y)
{
  Track * track =
    timeline_arranger_widget_get_track_at_y (
      self, y);
  if (!track)
    return NULL;

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  if (!atl || !track->automation_visible)
    return NULL;

  /* y local to track */
  int y_local =
    track_widget_get_local_y (
      track->widget, self, (int) y);

  for (int j = 0; j < atl->num_ats; j++)
    {
      AutomationTrack * at = atl->ats[j];

      if (!at->created || !at->visible)
        continue;

      if (y_local >= at->y &&
          y_local < at->y + at->height)
        return at;
    }

  return NULL;
}

/**
 * Create a ZRegion at the given Position in the
 * given Track's given TrackLane.
 *
 * @param type The type of region to create.
 * @param pos The pre-snapped position.
 * @param track Track, if non-automation.
 * @param lane TrackLane, if midi/audio region.
 * @param at AutomationTrack, if automation Region.
 */
void
timeline_arranger_widget_create_region (
  ArrangerWidget *  self,
  const RegionType  type,
  Track *           track,
  TrackLane *       lane,
  AutomationTrack * at,
  const Position *  pos)
{
  bool autofilling =
    self->action == UI_OVERLAY_ACTION_AUTOFILLING;

  /* if autofilling, the action is already set */
  if (!autofilling)
    {
      self->action =
        UI_OVERLAY_ACTION_CREATING_RESIZING_R;
    }

  g_message ("creating region");

  Position end_pos;
  position_set_min_size (
    pos, &end_pos,
    self->snap_grid);

  /* create a new region */
  ZRegion * region = NULL;
  switch (type)
    {
    case REGION_TYPE_MIDI:
      region =
        midi_region_new (
          pos, &end_pos,
          track_get_name_hash (track),
          /* create on lane 0 if creating in main
           * track */
          lane ? lane->pos : 0,
          lane ? lane->num_regions :
            track->lanes[0]->num_regions);
      break;
    case REGION_TYPE_AUDIO:
      break;
    case REGION_TYPE_CHORD:
      region =
        chord_region_new (
          pos, &end_pos,
          P_CHORD_TRACK->num_chord_regions);
      break;
    case REGION_TYPE_AUTOMATION:
      g_return_if_fail (at);
      region =
        automation_region_new (
          pos, &end_pos,
          track_get_name_hash (track),
          at->index,
          at->num_regions);
      break;
    }

  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  self->start_object = r_obj;
  /*region_set_end_pos (*/
    /*region, &end_pos, AO_UPDATE_ALL);*/
  /*long length =*/
    /*region_get_full_length_in_ticks (region);*/
  /*position_from_ticks (*/
    /*&region->true_end_pos, length);*/
  /*region_set_true_end_pos (*/
    /*region, &region->true_end_pos, AO_UPDATE_ALL);*/
  /*position_init (&tmp);*/
  /*region_set_clip_start_pos (*/
    /*region, &tmp, AO_UPDATE_ALL);*/
  /*region_set_loop_start_pos (*/
    /*region, &tmp, AO_UPDATE_ALL);*/
  /*region_set_loop_end_pos (*/
    /*region, &region->true_end_pos, AO_UPDATE_ALL);*/

  switch (type)
    {
    case REGION_TYPE_MIDI:
      track_add_region (
        track, region, NULL,
        lane ? lane->pos :
        (track->num_lanes == 1 ?
         0 : track->num_lanes - 2), F_GEN_NAME,
        F_PUBLISH_EVENTS);
      break;
    case REGION_TYPE_AUDIO:
      break;
    case REGION_TYPE_CHORD:
      track_add_region (
        track, region, NULL,
        -1, F_GEN_NAME, F_PUBLISH_EVENTS);
      break;
    case REGION_TYPE_AUTOMATION:
      track_add_region (
        track, region, at,
        -1, F_GEN_NAME, F_PUBLISH_EVENTS);
      break;
    }

  /* set visibility */
  /*arranger_object_gen_widget (r_obj);*/
  /*arranger_object_set_widget_visibility_and_state (*/
     /*r_obj, 1);*/

  arranger_object_set_position (
    r_obj, &r_obj->end_pos,
    ARRANGER_OBJECT_POSITION_TYPE_END,
    F_NO_VALIDATE);
  arranger_object_select (
    r_obj, F_SELECT,
    autofilling ? F_APPEND : F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
}

/**
 * Wrapper for
 * timeline_arranger_widget_create_chord() or
 * timeline_arranger_widget_create_scale().
 *
 * @param y the y relative to the
 *   ArrangerWidget.
 */
void
timeline_arranger_widget_create_chord_or_scale (
  ArrangerWidget * self,
  Track *                  track,
  double                   y,
  const Position *         pos)
{
  int track_height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (track->widget));
  double wy;
  gtk_widget_translate_coordinates (
    GTK_WIDGET (self),
    GTK_WIDGET (track->widget),
    0, (int) y, NULL, &wy);

  if (y >= track_height / 2.0)
    timeline_arranger_widget_create_scale (
      self, track, pos);
  else
    timeline_arranger_widget_create_region (
      self, REGION_TYPE_CHORD, track, NULL,
      NULL, pos);
}

/**
 * Create a ScaleObject at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_scale (
  ArrangerWidget * self,
  Track *            track,
  const Position *         pos)
{
  g_warn_if_fail (track->type == TRACK_TYPE_CHORD);

  self->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;

  /* create a new scale */
  MusicalScale * descr =
    musical_scale_new (SCALE_AEOLIAN, NOTE_A);
  ScaleObject * scale =
    scale_object_new (descr);
  ArrangerObject * scale_obj =
    (ArrangerObject *) scale;

  /* add it to scale track */
  chord_track_add_scale (track, scale);

  /*arranger_object_gen_widget (scale_obj);*/

  /* set visibility */
  /*arranger_object_set_widget_visibility_and_state (*/
    /*scale_obj, 1);*/

  arranger_object_pos_setter (
    scale_obj, pos);

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, scale);
  arranger_object_select (
    scale_obj, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
}

/**
 * Create a Marker at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
void
timeline_arranger_widget_create_marker (
  ArrangerWidget * self,
  Track *            track,
  const Position *         pos)
{
  g_warn_if_fail (
    track->type == TRACK_TYPE_MARKER);

  self->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;

  /* create a new marker */
  Marker * marker =
    marker_new (_("Custom Marker"));
  ArrangerObject * marker_obj =
    (ArrangerObject *) marker;

  /* add it to marker track */
  marker_track_add_marker (
    track, marker);

  /*arranger_object_gen_widget (marker_obj);*/

  /* set visibility */
  /*arranger_object_set_widget_visibility_and_state (*/
    /*marker_obj, 1);*/

  arranger_object_pos_setter (
    marker_obj, pos);

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, marker);
  arranger_object_select (
    marker_obj, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
}

/**
 * Determines the selection time (objects/range)
 * and sets it.
 */
void
timeline_arranger_widget_set_select_type (
  ArrangerWidget * self,
  double           y)
{
  Track * track =
    timeline_arranger_widget_get_track_at_y (
      self, y);

  if (track)
    {
      if (track_widget_is_cursor_in_range_select_half (
            track->widget, y))
        {
          /* set resizing range flags */
          self->resizing_range = true;
          self->resizing_range_start = true;
          self->action =
            UI_OVERLAY_ACTION_RESIZING_R;
        }
      else
        {
          /* select objects */
          self->resizing_range = false;
        }
    }
  else
    {
      /* TODO something similar as above based on
       * visible space */
      self->resizing_range = false;
    }

  /*arranger_widget_refresh_all_backgrounds ();*/
  /*gtk_widget_queue_allocate (*/
    /*GTK_WIDGET (MW_RULER));*/
}

/**
 * Snaps the region's start point.
 *
 * @param new_start_pos Position to snap to.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
static inline int
snap_region_l (
  ArrangerWidget * self,
  ZRegion *        region,
  Position *       new_pos,
  int              dry_run)
{
  ArrangerObjectResizeType type =
    ARRANGER_OBJECT_RESIZE_NORMAL;
  if ACTION_IS (RESIZING_L_LOOP)
    type = ARRANGER_OBJECT_RESIZE_LOOP;
  else if ACTION_IS (RESIZING_L_FADE)
    type = ARRANGER_OBJECT_RESIZE_FADE;
  else if ACTION_IS (STRETCHING_L)
    type = ARRANGER_OBJECT_RESIZE_STRETCH;

  /* negative positions not allowed */
  if (!position_is_positive (new_pos))
    return -1;

  if (SNAP_GRID_ANY_SNAP (self->snap_grid) &&
        !self->shift_held &&
        type != ARRANGER_OBJECT_RESIZE_FADE)
    {
      Track * track =
        arranger_object_get_track (
          (ArrangerObject *) region);
      position_snap (
        &self->earliest_obj_start_pos,
        new_pos, track,
        NULL, self->snap_grid);
    }

  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  Position cmp_pos;
  if (type == ARRANGER_OBJECT_RESIZE_FADE)
    {
      cmp_pos = r_obj->fade_out_pos;
    }
  else
    {
      cmp_pos = r_obj->end_pos;
    }
  if (position_is_after_or_equal (
        new_pos, &cmp_pos))
    return -1;
  else if (!dry_run)
    {
      int is_valid = 0;
      double diff = 0;

      if (type == ARRANGER_OBJECT_RESIZE_FADE)
        {
          is_valid =
            arranger_object_validate_pos (
              r_obj, new_pos,
              ARRANGER_OBJECT_POSITION_TYPE_FADE_IN);
          diff =
            position_to_ticks (new_pos) -
            position_to_ticks (&r_obj->fade_in_pos);
        }
      else
        {
          is_valid =
            arranger_object_validate_pos (
              r_obj, new_pos,
              ARRANGER_OBJECT_POSITION_TYPE_START);
          diff =
            position_to_ticks (new_pos) -
            position_to_ticks (&r_obj->pos);
        }

      if (is_valid)
        {
          arranger_object_resize (
            r_obj, true, type, diff, true);
        }
    }

  return 0;
}

/**
 * Snaps both the transients (to show in the GUI)
 * and the actual regions.
 *
 * @param pos Absolute position in the timeline.
 * @param dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen).
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
timeline_arranger_widget_snap_regions_l (
  ArrangerWidget * self,
  Position *       pos,
  int              dry_run)
{
  ArrangerObject * start_r_obj =
    self->start_object;

  /* get delta with first clicked region's start
   * pos */
  double delta;
  if (ACTION_IS (RESIZING_L_FADE))
    {
      delta =
        position_to_ticks (pos) -
        (position_to_ticks (
           &start_r_obj->pos) +
         position_to_ticks (
           &start_r_obj->fade_in_pos));
    }
  else
    {
      delta =
        position_to_ticks (pos) -
        position_to_ticks (
          &start_r_obj->pos);
    }

  /* new start pos for each region, calculated by
   * adding delta to the region's original start
   * pos */
  Position new_pos;

  ZRegion * region;
  ArrangerObject * r_obj;
  int ret;
  for (int i = 0;
       i < TL_SELECTIONS->num_regions;
       i++)
    {
      /* main trans region */
      region =
        TL_SELECTIONS->regions[i];
      r_obj = (ArrangerObject *) region;

      /* calculate new start position */
      if (ACTION_IS (RESIZING_L_FADE))
        {
          position_set_to_pos (
            &new_pos, &r_obj->fade_in_pos);
        }
      else
        {
          position_set_to_pos (
            &new_pos, &r_obj->pos);
        }
      position_add_ticks (&new_pos, delta);

      ret =
        snap_region_l (
          self, region, &new_pos, dry_run);

      if (ret)
        return ret;
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED,
    TL_SELECTIONS);

  return 0;
}

/**
 * Snaps the region's end point.
 *
 * @param new_end_pos New end position to snap to.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
static int
snap_region_r (
  ArrangerWidget * self,
  ZRegion *        region,
  Position *       new_pos,
  int              dry_run)
{
  ArrangerObjectResizeType type =
    ARRANGER_OBJECT_RESIZE_NORMAL;
  if ACTION_IS (RESIZING_R_LOOP)
    type = ARRANGER_OBJECT_RESIZE_LOOP;
  else if ACTION_IS (RESIZING_R_FADE)
    type = ARRANGER_OBJECT_RESIZE_FADE;
  else if ACTION_IS (STRETCHING_R)
    type = ARRANGER_OBJECT_RESIZE_STRETCH;

  /* negative positions not allowed */
  if (!position_is_positive (new_pos))
    return -1;

  if (SNAP_GRID_ANY_SNAP (self->snap_grid)
      && !self->shift_held
      && type != ARRANGER_OBJECT_RESIZE_FADE)
    {
      Track * track =
        arranger_object_get_track (
          (ArrangerObject *) region);
      position_snap (
        &self->earliest_obj_start_pos,
        new_pos, track,
        NULL, self->snap_grid);
    }

  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  if (type == ARRANGER_OBJECT_RESIZE_FADE)
    {
      Position tmp;
      position_from_ticks (
        &tmp,
        r_obj->end_pos.ticks -
          r_obj->pos.ticks);
      if (position_is_before_or_equal (
            new_pos, &r_obj->fade_in_pos) ||
          position_is_after (new_pos, &tmp))
        return -1;
    }
  else
    {
      if (position_is_before_or_equal (
            new_pos, &r_obj->pos))
        return -1;
    }

  if (!dry_run)
    {
      int is_valid = 0;
      double diff = 0;
      if (type == ARRANGER_OBJECT_RESIZE_FADE)
        {
          is_valid =
            arranger_object_validate_pos (
              r_obj, new_pos,
              ARRANGER_OBJECT_POSITION_TYPE_FADE_OUT);
          diff =
            position_to_ticks (new_pos) -
            position_to_ticks (&r_obj->fade_out_pos);
        }
      else
        {
          is_valid =
            arranger_object_validate_pos (
              r_obj, new_pos,
              ARRANGER_OBJECT_POSITION_TYPE_END);
          diff =
            position_to_ticks (new_pos) -
            position_to_ticks (&r_obj->end_pos);
        }

      if (is_valid)
        {
          arranger_object_resize (
            r_obj, Z_F_NOT_LEFT, type, diff,
            Z_F_DURING_UI_ACTION);

          /* if creating also set the loop points
           * appropriately */
          if (self->action ==
                UI_OVERLAY_ACTION_CREATING_RESIZING_R)
            {
              double full_size =
                arranger_object_get_length_in_ticks (
                  r_obj);
              Position tmp;
              position_set_to_pos (
                &tmp, &r_obj->loop_start_pos);
              position_add_ticks (&tmp, full_size);

              /* use the setters */
              arranger_object_loop_end_pos_setter (
                r_obj, &tmp);
            }
        }
    }

  return 0;
}

/**
 * Snaps both the transients (to show in the GUI)
 * and the actual regions.
 *
 * @param pos Absolute position in the timeline.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
timeline_arranger_widget_snap_regions_r (
  ArrangerWidget * self,
  Position *       pos,
  int              dry_run)
{
  ArrangerObject * start_r_obj =
    self->start_object;

  /* get delta with first clicked region's end
   * pos */
  double delta;
  if (ACTION_IS (RESIZING_R_FADE))
    {
      delta =
        position_to_ticks (pos) -
        (position_to_ticks (
          &start_r_obj->pos) +
         position_to_ticks (
           &start_r_obj->fade_out_pos));
    }
  else
    {
      delta =
        position_to_ticks (pos) -
        position_to_ticks (
          &start_r_obj->end_pos);
    }

  /* new end pos for each region, calculated by
   * adding delta to the region's original end
   * pos */
  Position new_pos;

  ZRegion * region;
  ArrangerObject * r_obj;
  int ret;
  for (int i = 0;
       i < TL_SELECTIONS->num_regions;
       i++)
    {
      region =
        TL_SELECTIONS->regions[i];
      r_obj = (ArrangerObject *) region;

      if (ACTION_IS (RESIZING_R_FADE))
        {
          position_set_to_pos (
            &new_pos, &r_obj->fade_out_pos);
        }
      else
        {
          position_set_to_pos (
            &new_pos, &r_obj->end_pos);
        }
      position_add_ticks (&new_pos, delta);

      ret =
        snap_region_r (
          self, region, &new_pos, dry_run);

      if (ret)
        return ret;
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED,
    (ArrangerSelections *) TL_SELECTIONS);

  return 0;
}

void
timeline_arranger_widget_snap_range_r (
  ArrangerWidget * self,
  Position *       pos)
{
  if (self->resizing_range_start)
    {
      /* set range 1 at current point */
      ui_px_to_pos_timeline (
        self->start_x,
        &TRANSPORT->range_1,
        1);
      if (SNAP_GRID_ANY_SNAP (
            self->snap_grid) &&
          !self->shift_held)
        {
          position_snap_simple (
            &TRANSPORT->range_1,
            SNAP_GRID_TIMELINE);
        }
      position_set_to_pos (
        &TRANSPORT->range_2,
        &TRANSPORT->range_1);

      MW_TIMELINE->resizing_range_start = 0;
    }

  /* set range */
  if (SNAP_GRID_ANY_SNAP (self->snap_grid) &&
      !self->shift_held)
    position_snap_simple (
      pos,
      SNAP_GRID_TIMELINE);
  position_set_to_pos (
    &TRANSPORT->range_2, pos);
  transport_set_has_range (TRANSPORT, true);
}

/**
 * @param fade_in 1 for in, 0 for out.
 */
static void
create_fade_preset_menu (
  ArrangerWidget * self,
  GMenu *          menu,
  ArrangerObject * obj,
  bool             fade_in)
{
  GMenu * section = g_menu_new ();

  GPtrArray * psets = curve_get_fade_presets ();
  for (size_t i = 0; i < psets->len; i++)
    {
      CurveFadePreset * pset =
        g_ptr_array_index (psets, i);

      char tmp[200];
      sprintf (
        tmp,
        "app.set-region-fade-%s-algorithm-preset::%s",
        fade_in ? "in" : "out", pset->id);
      GMenuItem * menuitem =
        z_gtk_create_menu_item (
          pset->label, NULL, tmp);
      g_menu_append_item (section, menuitem);
    }
  g_ptr_array_unref (psets);

  g_menu_append_section (
    menu, _("Fade Preset"),
    G_MENU_MODEL (section));
}

#if 0
/** Used when selecting a musical mode. */
typedef struct MusicalModeInfo
{
  RegionMusicalMode mode;
  ArrangerObject *  obj;
} MusicalModeInfo;

static void
on_musical_mode_toggled (
  GtkCheckMenuItem * menu_item,
  MusicalModeInfo *  info)
{
  if (!gtk_check_menu_item_get_active (menu_item))
    {
      return;
    }

  ArrangerSelections * sel_before =
    arranger_selections_clone (
      (ArrangerSelections *) TL_SELECTIONS);

  /* make the change */
  ZRegion * region = (ZRegion *) info->obj;
  region->musical_mode = info->mode;

  g_warn_if_fail (
    arranger_object_is_selected (info->obj));

  GError * err = NULL;
  bool ret =
    arranger_selections_action_perform_edit (
      sel_before,
      (ArrangerSelections *) TL_SELECTIONS,
      ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE,
      true, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to edit selections"));
    }

  g_warn_if_fail (IS_ARRANGER_OBJECT (info->obj));
  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CHANGED, info->obj);

  object_zero_and_free (info);
}

/**
 * @param fade_in 1 for in, 0 for out.
 */
static void
create_musical_mode_pset_menu (
  ArrangerWidget * self,
  GtkWidget *      menu,
  ArrangerObject * obj)
{
  GtkWidget * menuitem =
    gtk_menu_item_new_with_label (
      _("Musical Mode"));
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), menuitem);

  GtkMenu * submenu = GTK_MENU (gtk_menu_new ());
  gtk_widget_set_visible (GTK_WIDGET (submenu), 1);
  GtkMenuItem * submenu_item[
    REGION_MUSICAL_MODE_ON + 2];
  MusicalModeInfo * nfo;
  GSList * group = NULL;
  ZRegion * region = (ZRegion *) obj;
  for (int i = 0; i <= REGION_MUSICAL_MODE_ON; i++)
    {
      submenu_item[i] =
        GTK_MENU_ITEM (
          gtk_radio_menu_item_new_with_label (
            group,
            _(region_musical_mode_strings[i].str)));
      if ((RegionMusicalMode) i ==
            region->musical_mode)
        {
          gtk_check_menu_item_set_active (
            GTK_CHECK_MENU_ITEM (
              submenu_item[i]), true);
        }
      gtk_menu_shell_append (
        GTK_MENU_SHELL (submenu),
        GTK_WIDGET (submenu_item[i]));
      gtk_widget_set_visible (
        GTK_WIDGET (submenu_item[i]), true);

      group =
        gtk_radio_menu_item_get_group (
          GTK_RADIO_MENU_ITEM (submenu_item[i]));
    }

  gtk_menu_item_set_submenu (
    GTK_MENU_ITEM (menuitem),
    GTK_WIDGET (submenu));
  gtk_widget_set_visible (
    GTK_WIDGET (menuitem), 1);

  for (int i = 0; i <= REGION_MUSICAL_MODE_ON; i++)
    {
      nfo = object_new (MusicalModeInfo);
      nfo->mode = i;
      nfo->obj = obj;
      g_signal_connect (
        G_OBJECT (submenu_item[i]), "toggled",
        G_CALLBACK (on_musical_mode_toggled), nfo);
    }
}
#endif

/**
 * Show context menu at x, y.
 */
void
timeline_arranger_widget_show_context_menu (
  ArrangerWidget * self,
  double           x,
  double           y)
{
  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_ALL, x, y);

  if (obj)
    {
      int local_x = (int) (x - obj->full_rect.x);
      int local_y = (int) (y - obj->full_rect.y);

      GMenu * edit_submenu = g_menu_new ();

      /* create cut, copy, duplicate, delete */
      menuitem =
        CREATE_CUT_MENU_ITEM ("app.cut");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem =
        CREATE_COPY_MENU_ITEM ("app.copy");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem =
        CREATE_DUPLICATE_MENU_ITEM (
          "app.duplicate");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem =
        CREATE_DELETE_MENU_ITEM ("app.delete");
      g_menu_append_item (edit_submenu, menuitem);

      g_menu_append_section (
        menu, _("Edit"),
        G_MENU_MODEL (edit_submenu));

      if (timeline_selections_contains_only_regions (TL_SELECTIONS))
        {
          ZRegion * r = (ZRegion *) obj;

          if (timeline_selections_contains_only_region_types (TL_SELECTIONS, REGION_TYPE_AUDIO))
            {
              GMenu * audio_regions_submenu =
                g_menu_new ();

              if (TL_SELECTIONS->num_regions == 1)
                {
                  char tmp[200];
                  sprintf (
                    tmp, "app.detect-bpm::%p",
                    TL_SELECTIONS->regions[0]);
                  menuitem =
                    z_gtk_create_menu_item (
                      _("Detect BPM"), NULL, tmp);
                  g_menu_append_item (
                    audio_regions_submenu, menuitem);
                  /* create fade menus */
                  if (arranger_object_is_fade_in (
                        obj, local_x, local_y, 0, 0))
                    {
                      create_fade_preset_menu (
                        self,
                        audio_regions_submenu,
                        obj, true);
                    }
                  if (arranger_object_is_fade_out (
                        obj, local_x, local_y, 0, 0))
                    {
                      create_fade_preset_menu (
                        self,
                        audio_regions_submenu,
                        obj, false);
                    }

#if 0
                  /* create musical mode menu */
                  create_musical_mode_pset_menu (
                    self, menu, obj);
#endif
                }

              GMenu * region_functions_subsubmenu =
                g_menu_new ();

              for (int i = AUDIO_FUNCTION_INVERT;
                   i < AUDIO_FUNCTION_CUSTOM_PLUGIN;
                   i++)
                {
                  if (i == AUDIO_FUNCTION_NORMALIZE_RMS
                      || i == AUDIO_FUNCTION_NORMALIZE_LUFS)
                    continue;

                  char tmp[200];
                  sprintf (
                    tmp, "app.timeline-function");
                  GMenuItem * submenu_item =
                    z_gtk_create_menu_item (
                      _(audio_function_type_to_string (i)),
                      NULL, tmp);
                  g_menu_item_set_action_and_target_value (
                    submenu_item, tmp,
                    g_variant_new_int32 (i));
                  g_menu_append_item (
                    region_functions_subsubmenu,
                    submenu_item);
                }
              g_menu_append_section (
                audio_regions_submenu,
                _("Region Functions"),
                G_MENU_MODEL (
                  region_functions_subsubmenu));
              g_menu_append_section (
                menu, _("Audio Regions"),
                G_MENU_MODEL (
                  audio_regions_submenu));
            }

          if (arranger_object_get_muted (
                obj, false))
            {
              menuitem =
                CREATE_UNMUTE_MENU_ITEM (
                  "app.mute-selection::timeline");
            }
          else
            {
              menuitem =
                CREATE_MUTE_MENU_ITEM (
                  "app.mute-selection::timeline");
            }
          g_menu_append_item (menu, menuitem);

          if (timeline_selections_contains_only_region_types (TL_SELECTIONS, REGION_TYPE_MIDI))
            {
              GMenu * midi_regions_submenu =
                g_menu_new ();

              char tmp[500];
              sprintf (
                tmp, "app.export-midi-region::%p",
                r);
              menuitem =
                z_gtk_create_menu_item (
                  _("Export as MIDI file"), NULL,
                  tmp);
              g_menu_append_item (
                midi_regions_submenu, menuitem);

              g_menu_append_section (
                menu, _("MIDI Regions"),
                G_MENU_MODEL (
                  midi_regions_submenu));
            }

          menuitem =
            z_gtk_create_menu_item (
              _("Quick bounce"), NULL,
              "app.quick-bounce-selections");
          g_menu_append_item (menu, menuitem);

          menuitem =
            z_gtk_create_menu_item (
              _("Bounce..."), NULL,
              "app.bounce-selections");
          g_menu_append_item (menu, menuitem);
        }
    }
  else
    {
      menuitem =
        CREATE_PASTE_MENU_ITEM ("app.paste");
      g_menu_append_item (menu, menuitem);
    }

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

/**
 * Fade up/down.
 *
 * @param fade_in 1 for in, 0 for out.
 */
void
timeline_arranger_widget_fade_up (
  ArrangerWidget * self,
  double           offset_y,
  int              fade_in)
{
  for (int i = 0; i < TL_SELECTIONS->num_regions; i++)
    {
      ZRegion * r = TL_SELECTIONS->regions[i];
      ArrangerObject * obj =
        (ArrangerObject *) r;

      CurveOptions * opts =
        fade_in ?
          &obj->fade_in_opts : &obj->fade_out_opts;
      double delta =
        (self->last_offset_y - offset_y) * 0.008;
      opts->curviness =
        CLAMP (opts->curviness + delta, - 1.0, 1.0);
    }
}

static void
highlight_timeline (
  ArrangerWidget * self,
  GdkModifierType  mask,
  int              x,
  int              y,
  Track *          track,
  TrackLane *      lane)
{
  /* get default size */
  int ticks =
    snap_grid_get_default_ticks (
      SNAP_GRID_TIMELINE);
  Position length_pos;
  position_from_ticks (&length_pos, ticks);
  int width_px =
    ui_pos_to_px_timeline (&length_pos, false);

  /* get snapped x */
  Position pos;
  ui_px_to_pos_timeline (x, &pos, true);
  if (!(mask & GDK_SHIFT_MASK))
    {
      position_snap_simple (
        &pos, SNAP_GRID_TIMELINE);
      x = ui_pos_to_px_timeline (&pos, true);
    }

  int height = TRACK_DEF_HEIGHT;
  /* if track, get y/height inside track */
  if (track)
    {
      height = (int) track->main_height;
      int track_y_local =
        track_widget_get_local_y (
          track->widget, self, (int) y);
      if (lane)
        {
          y -= track_y_local - lane->y;
          height = (int) lane->height;
        }
      else
        {
          y -= track_y_local;
        }
    }
  /* else if no track, get y/height under last
   * visible track */
  else
    {
      /* get y below the track */
      int y_after_last_track = 0;
      for (int i = 0; i < TRACKLIST->num_tracks;
           i++)
        {
          Track * t = TRACKLIST->tracks[i];
          if (track_get_should_be_visible (t)
              &&
              track_is_pinned (t) ==
                self->is_pinned)
            {
              y_after_last_track +=
                (int)
                track_get_full_visible_height (t);
            }
        }
      y = y_after_last_track;
    }

  GdkRectangle highlight_rect = {
    x, y, width_px, height };
  arranger_widget_set_highlight_rect (
    self, &highlight_rect);
}

static gboolean
on_dnd_drop (
  GtkDropTarget * drop_target,
  const GValue  * value,
  double          x,
  double          y,
  gpointer        data)
{
  ArrangerWidget * self =
    Z_ARRANGER_WIDGET (data);

  Track * track =
    timeline_arranger_widget_get_track_at_y (
      self, y);
  TrackLane * lane =
    timeline_arranger_widget_get_track_lane_at_y (
      self, y);
  AutomationTrack * at =
    timeline_arranger_widget_get_at_at_y (
      self, y);

  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (drop_target));

  highlight_timeline (
    self, state, (int) x, (int) y, track, lane);

  g_message (
    "dnd data dropped (timeline - is "
    "highlighted %d)",
    self->is_highlighted);

  SupportedFile * file = NULL;
  ChordDescriptor * chord_descr = NULL;
  if (G_VALUE_HOLDS (
        value,
        WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        g_value_get_object (value);
      if (wrapped_obj->type ==
            WRAPPED_OBJECT_TYPE_SUPPORTED_FILE)
        {
          file = (SupportedFile *) wrapped_obj->obj;
        }
      if (wrapped_obj->type ==
            WRAPPED_OBJECT_TYPE_CHORD_DESCR)
        {
          chord_descr =
            (ChordDescriptor *) wrapped_obj->obj;
        }
    }

  if (chord_descr
      && self->is_highlighted && track
      && track_type_has_piano_roll (track->type))
    {
      ChordDescriptor * descr = chord_descr;

      /* create chord region */
      Position pos, end_pos;
      ui_px_to_pos_timeline (
        self->highlight_rect.x, &pos, true);
      ui_px_to_pos_timeline (
        self->highlight_rect.x +
          self->highlight_rect.width,
        &end_pos, true);
      int lane_pos =
        lane ? lane->pos :
        (track->num_lanes == 1 ?
         0 : track->num_lanes - 2);
      int idx_in_lane =
        track->lanes[lane_pos]->num_regions;
      ZRegion * region =
        midi_region_new_from_chord_descr (
          &pos, descr,
          track_get_name_hash (track),
          lane_pos, idx_in_lane);
      track_add_region (
        track, region, NULL, lane_pos,
        F_GEN_NAME, F_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) region, F_SELECT,
        F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);

      GError * err = NULL;
      bool ret =
        arranger_selections_action_perform_create (
          TL_SELECTIONS, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s",
            _("Failed to create selections"));
        }
    }
  else if (
    G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)
    || G_VALUE_HOLDS (value, G_TYPE_FILE)
    || file)
    {
      if (at)
        {
          /* nothing to do */
          goto finish_data_received;
        }

      char ** uris = NULL;
      if (G_VALUE_HOLDS (value, G_TYPE_FILE))
        {
          GFile * gfile =
            g_value_get_object (value);
          StrvBuilder * uris_builder =
            strv_builder_new ();
          char * uri = g_file_get_uri (gfile);
          strv_builder_add (uris_builder, uri);
          uris =
            strv_builder_end (uris_builder);
        }
      else if (G_VALUE_HOLDS (
                 value, GDK_TYPE_FILE_LIST))
        {
          StrvBuilder * uris_builder =
            strv_builder_new ();
          GSList * l;
          for (l = g_value_get_boxed (value); l;
               l = l->next)
            {
              char * uri = g_file_get_uri (l->data);
              strv_builder_add (uris_builder, uri);
              g_free (uri);
            }
          uris =
            strv_builder_end (uris_builder);
        }

      Position pos;
      ui_px_to_pos_timeline (
        self->highlight_rect.x, &pos, true);
      tracklist_handle_file_drop (
        TRACKLIST, uris, file, track, lane, &pos,
        true);

      if (uris)
        g_strfreev (uris);
    }

finish_data_received:
  arranger_widget_set_highlight_rect (self, NULL);

  return true;
}

static void
on_dnd_motion_value_ready (
  GObject* source_object,
  GAsyncResult* res,
  gpointer user_data)
{
  ArrangerWidget * self =
    Z_ARRANGER_WIDGET (user_data);
  GdkDrop * drop = GDK_DROP (source_object);
  GError * err = NULL;
  const GValue * value =
    gdk_drop_read_value_finish (drop, res, &err);
  if (err)
    {
      g_message ("error: %s", err->message);
      return;
    }

  ChordDescriptor * chord_descr = NULL;
  SupportedFile * supported_file = NULL;
  if (G_VALUE_HOLDS (
        value,
        WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        g_value_get_object (value);
      if (wrapped_obj->type ==
            WRAPPED_OBJECT_TYPE_SUPPORTED_FILE)
        {
          supported_file =
            (SupportedFile *) wrapped_obj->obj;
        }
      else if (wrapped_obj->type ==
                 WRAPPED_OBJECT_TYPE_CHORD_DESCR)
        {
          chord_descr =
            (ChordDescriptor *) wrapped_obj->obj;
        }
    }

  bool has_files = false;
  if (G_VALUE_HOLDS (
        value, GDK_TYPE_FILE_LIST)
      ||
      G_VALUE_HOLDS (value, G_TYPE_FILE))
    {
      has_files = true;
    }

  Track * track = self->hovered_track;
  TrackLane * lane = self->hovered_lane;
  AutomationTrack * at = self->hovered_at;
  if (chord_descr)
    {
      if (at || !track ||
          !track_type_has_piano_roll (track->type))
        {
          /* nothing to do */
          return;
        }

      /* highlight track */
      highlight_timeline (
        self, 0, (int) self->hover_x,
        (int) self->hover_y, track, lane);

      return;
    }
  else if (has_files || supported_file)
    {
      /* if current track exists and current track
       * supports dnd highlight */
      if (track)
        {
          if (track->type != TRACK_TYPE_MIDI &&
               track->type !=
                 TRACK_TYPE_INSTRUMENT &&
              track->type != TRACK_TYPE_AUDIO)
            {
              return;
            }

          /* track is compatible, highlight */
          highlight_timeline (
            self, 0, (int) self->hover_x,
            (int) self->hover_y, track, lane);
          g_message ("highlighting track");

          return;
        }
      /* else if no track, highlight below the
       * last track  TODO */
      else
        {
          highlight_timeline (
            self, 0, (int) self->hover_x,
            (int) self->hover_y, NULL, NULL);
        }
    }
}

static GdkDragAction
on_dnd_motion (
  GtkDropTarget * drop_target,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  ArrangerWidget * self =
    Z_ARRANGER_WIDGET (user_data);

  self->hovered_at =
    timeline_arranger_widget_get_at_at_y (self, y);
  self->hovered_lane =
    timeline_arranger_widget_get_track_lane_at_y (
      self, y);
  self->hovered_track =
    timeline_arranger_widget_get_track_at_y (
      self, y);

  arranger_widget_set_highlight_rect (self, NULL);

  GdkDrop * drop =
    gtk_drop_target_get_current_drop (
      drop_target);
  gdk_drop_read_value_async (
    drop, G_TYPE_OBJECT,
    0, NULL, on_dnd_motion_value_ready, self);

  return GDK_ACTION_MOVE;
}

static void
on_dnd_leave (
  GtkDropTarget *  drop_target,
  ArrangerWidget * self)
{
  g_message (
    "dnd leaving timeline, unhighlighting rect");

  arranger_widget_set_highlight_rect (self, NULL);
}

/**
 * Sets up the timeline arranger as a drag dest.
 */
void
timeline_arranger_setup_drag_dest (
  ArrangerWidget * self)
{
  GtkDropTarget * drop_target =
    gtk_drop_target_new (
      G_TYPE_INVALID,
      GDK_ACTION_MOVE | GDK_ACTION_COPY);
  GType types[] = {
    GDK_TYPE_FILE_LIST, G_TYPE_FILE,
    WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE };
  gtk_drop_target_set_gtypes (
    drop_target, types, G_N_ELEMENTS (types));
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (drop_target));

#if 0
  /* set as drag dest */
  GtkTargetEntry entries[] = {
    {
      (char *) TARGET_ENTRY_CHORD_DESCR,
      GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_CHORD_DESCR),
    },
    {
      (char *) TARGET_ENTRY_SUPPORTED_FILE,
      GTK_TARGET_SAME_APP,
      symap_map (
        ZSYMAP, TARGET_ENTRY_SUPPORTED_FILE),
    },
    {
      (char *) TARGET_ENTRY_URI_LIST,
      GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_URI_LIST),
    },
    {
      (char *) TARGET_ENTRY_URI_LIST,
      GTK_TARGET_OTHER_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_URI_LIST),
    },
  };
  gtk_drag_dest_set (
    GTK_WIDGET (self),
    GTK_DEST_DEFAULT_MOTION |
      GTK_DEST_DEFAULT_DROP,
    entries, G_N_ELEMENTS (entries),
    GDK_ACTION_MOVE | GDK_ACTION_COPY);
#endif

  g_signal_connect (
    drop_target, "drop",
    G_CALLBACK (on_dnd_drop), self);
  g_signal_connect (
    drop_target, "motion",
    G_CALLBACK (on_dnd_motion), self);
  g_signal_connect (
    drop_target, "leave",
    G_CALLBACK (on_dnd_leave), self);
}
