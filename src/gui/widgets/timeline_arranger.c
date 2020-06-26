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

#include "actions/arranger_selections.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/audio_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_object.h"
#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/exporter.h"
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_region.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_region.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/dialogs/bounce_dialog.h"
#include "gui/widgets/dialogs/export_progress_dialog.h"
#include "gui/widgets/export_midi_file_dialog.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/ruler.h"
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
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
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
        mask & GDK_MOD1_MASK;

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
        self->is_pinned != track->pinned)
        continue;

      g_return_val_if_fail (track->widget, NULL);

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

void
timeline_arranger_on_export_as_midi_file_clicked (
  GtkMenuItem * menuitem,
  ZRegion *      r)
{
  GtkDialog * dialog =
    GTK_DIALOG (
      export_midi_file_dialog_widget_new_for_region (
        GTK_WINDOW (MAIN_WINDOW),
        r));
  int res = gtk_dialog_run (dialog);
  char * filename;
  switch (res)
    {
    case GTK_RESPONSE_ACCEPT:
      // do_application_specific_something ();
      filename =
        gtk_file_chooser_get_filename (
          GTK_FILE_CHOOSER (dialog));
      g_message ("exporting to %s", filename);
      midi_region_export_to_midi_file (
        r, filename, 0, 0);
      g_free (filename);
      break;
    default:
      // do_nothing_since_dialog_was_cancelled ();
      break;
    }
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
timeline_arranger_on_quick_bounce_clicked (
  GtkMenuItem * menuitem,
  ZRegion *      r)
{
  ExportSettings settings;
  timeline_selections_mark_for_bounce (
    TL_SELECTIONS);
  settings.mode = EXPORT_MODE_REGIONS;
  export_settings_set_bounce_defaults (
    &settings, NULL, r->name);

  /* start exporting in a new thread */
  GThread * thread =
    g_thread_new (
      "bounce_thread",
      (GThreadFunc) exporter_generic_export_thread,
      &settings);

  /* create a progress dialog and block */
  ExportProgressDialogWidget * progress_dialog =
    export_progress_dialog_widget_new (
      &settings, 1, 0);
  gtk_window_set_transient_for (
    GTK_WINDOW (progress_dialog),
    GTK_WINDOW (MAIN_WINDOW));
  gtk_dialog_run (GTK_DIALOG (progress_dialog));
  gtk_widget_destroy (GTK_WIDGET (progress_dialog));

  g_thread_join (thread);

  /* create audio track with bounced material */
  exporter_create_audio_track_after_bounce (
    &settings);

  export_settings_free_members (&settings);
}

void
timeline_arranger_on_bounce_clicked (
  GtkMenuItem * menuitem,
  ZRegion *      r)
{
  BounceDialogWidget * dialog =
    bounce_dialog_widget_new (
      BOUNCE_DIALOG_REGIONS, r->name);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (GTK_WIDGET (dialog));
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
  ArrangerWidget * self,
  const RegionType         type,
  Track *                  track,
  TrackLane *              lane,
  AutomationTrack *        at,
  const Position *         pos)
{
  self->action =
    UI_OVERLAY_ACTION_CREATING_RESIZING_R;

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
          pos, &end_pos, track->pos,
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
      region =
        automation_region_new (
          pos, &end_pos, track->pos, at->index,
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
    r_obj, F_SELECT, F_NO_APPEND);
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
  gint wy;
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
    musical_scale_new (
      SCALE_AEOLIAN, NOTE_A);
  ScaleObject * scale =
    scale_object_new (
      descr, 1);
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
    scale_obj, F_SELECT, F_NO_APPEND);
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
    marker_obj, F_SELECT, F_NO_APPEND);
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
      if (track_widget_is_cursor_in_top_half (
            track->widget,
            y))
        {
          /* select objects */
          self->resizing_range = 0;
        }
      else
        {

          /* set resizing range flags */
          self->resizing_range = 1;
          self->resizing_range_start = 1;
          self->action =
            UI_OVERLAY_ACTION_RESIZING_R;
        }
    }
  else
    {
      /* TODO something similar as above based on
       * visible space */
      self->resizing_range = 0;
    }

  /*arranger_widget_refresh_all_backgrounds ();*/
  gtk_widget_queue_allocate (
    GTK_WIDGET (MW_RULER));
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

  if (SNAP_GRID_ANY_SNAP (self->snap_grid) &&
        !self->shift_held &&
        type != ARRANGER_OBJECT_RESIZE_FADE)
    {
      Track * track =
        arranger_object_get_track (
          (ArrangerObject *) region);
      position_snap (
        NULL, new_pos, track,
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
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
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

      /* caclulate new start position */
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
 * @param new_end_pos Position to snap to.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
static inline int
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

  if (SNAP_GRID_ANY_SNAP (self->snap_grid) &&
        !self->shift_held &&
        type != ARRANGER_OBJECT_RESIZE_FADE)
    {
      Track * track =
        arranger_object_get_track (
          (ArrangerObject *) region);
      position_snap (
        NULL, new_pos, track,
        NULL, self->snap_grid);
    }

  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  if (type == ARRANGER_OBJECT_RESIZE_FADE)
    {
      Position tmp;
      position_from_ticks (
        &tmp,
        r_obj->end_pos.total_ticks -
          r_obj->pos.total_ticks);
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
            r_obj, false, type, diff, true);

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
        position_snap_simple (
          &TRANSPORT->range_1,
          SNAP_GRID_TIMELINE);
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

  /*arranger_widget_refresh_all_backgrounds ();*/
}

#define COMPARE_AND_SET(pos) \
  if ((pos)->bars > self->last_timeline_obj_bars) \
    self->last_timeline_obj_bars = (pos)->bars;

/**
 * Updates last timeline objet so that timeline can be
 * expanded/contracted accordingly.
 */
/*static int*/
/*update_last_timeline_object ()*/
/*{*/
  /*if (!ZRYTHM || !MAIN_WINDOW ||*/
      /*!GTK_IS_WIDGET (MAIN_WINDOW))*/
    /*return G_SOURCE_CONTINUE;*/

  /*ArrangerWidget * self = MW_TIMELINE;*/

  /*int prev = self->last_timeline_obj_bars;*/
  /*self->last_timeline_obj_bars = 0;*/

  /*Marker * end =*/
    /*marker_track_get_end_marker (*/
      /*P_MARKER_TRACK);*/
  /*ArrangerObject * end_obj =*/
    /*(ArrangerObject *) end;*/
  /*COMPARE_AND_SET (&end_obj->pos);*/

  /*Track * track;*/
  /*ZRegion * region;*/
  /*ArrangerObject * r_obj;*/
  /*for (int i = 0; i < TRACKLIST->num_tracks; i++)*/
    /*{*/
      /*track = TRACKLIST->tracks[i];*/

      /*region =*/
        /*track_get_last_region (track);*/
      /*r_obj = (ArrangerObject *) region;*/

      /*if (region)*/
        /*COMPARE_AND_SET (&r_obj->end_pos);*/
    /*}*/

  /*if (prev != self->last_timeline_obj_bars &&*/
      /*self->last_timeline_obj_bars > 127)*/
    /*EVENTS_PUSH (ET_LAST_TIMELINE_OBJECT_CHANGED,*/
                 /*NULL);*/

  /*return G_SOURCE_CONTINUE;*/
/*}*/

#undef COMPARE_AND_SET

/**
 * To be called once at init time.
 */
/*void*/
/*timeline_arranger_widget_setup (*/
  /*ArrangerWidget * self)*/
/*{*/

  /*g_timeout_add (*/
    /*1000,*/
    /*update_last_timeline_object,*/
    /*NULL);*/
/*}*/

/**
 * Move the selected Regions to new Lanes.
 *
 * @param diff The delta to move the
 *   Tracks.
 *
 * @return 1 if moved.
 */
int
timeline_arranger_move_regions_to_new_lanes (
  ArrangerWidget * self,
  const int        diff)
{
  int i;

  /* store selected regions because they will be
   * deselected during moving */
  ZRegion * regions[600];
  int num_regions = 0;
  ZRegion * region;
  for (i = 0; i < TL_SELECTIONS->num_regions; i++)
    {
      regions[num_regions++] =
        TL_SELECTIONS->regions[i];
    }

  /* check that:
   * - all regions are in the same track
   * - only lane regions are selected
   * - the lane bounds are not exceeded */
  int compatible = 1;
  for (i = 0; i < num_regions; i++)
    {
      region = regions[i];
      if (region->id.lane_pos + diff < 0)
        {
          compatible = 0;
          break;
        }
    }
  if (TL_SELECTIONS->num_scale_objects > 0 ||
      TL_SELECTIONS->num_markers > 0)
    compatible = 0;
  if (!compatible)
    return 0;

  /* new positions are all compatible, move the
   * regions */
  for (i = 0; i < num_regions; i++)
    {
      region = regions[i];
      TrackLane * lane = region_get_lane (region);
      g_return_val_if_fail (region && lane, -1);

      TrackLane * lane_to_move_to = NULL;
      int new_lane_pos =
        lane->pos +  diff;
      g_return_val_if_fail (
        new_lane_pos >= 0, -1);
      Track * track = track_lane_get_track (lane);
      track_create_missing_lanes (
        track, new_lane_pos);
      lane_to_move_to =
        track->lanes[new_lane_pos];
      g_warn_if_fail (lane_to_move_to);

      region_move_to_lane (
        region, lane_to_move_to);
    }
  return 1;
}

/**
 * Move the selected Regions to the new Track.
 *
 * @param new_track_is_before 1 if the Region's
 *   should move to their previous tracks, 0 for
 *   their next tracks.
 *
 * @return 1 if moved.
 */
int
timeline_arranger_move_regions_to_new_tracks (
  ArrangerWidget * self,
  const int        vis_track_diff)
{
  int i;

  /* store selected regions because they will be
   * deselected during moving */
  ZRegion * regions[600];
  int num_regions = 0;
  ZRegion * region;
  ArrangerObject * r_obj;
  for (i = 0; i < TL_SELECTIONS->num_regions; i++)
    {
      regions[num_regions++] =
        TL_SELECTIONS->regions[i];
    }

  /* check that all regions can be moved to a
   * compatible track */
  int compatible = 1;
  for (i = 0; i < num_regions; i++)
    {
      region = regions[i];
      r_obj = (ArrangerObject *) region;
      Track * region_track =
        arranger_object_get_track (r_obj);
      Track * visible =
        tracklist_get_visible_track_after_delta (
          TRACKLIST,
          region_track,
          vis_track_diff);
      if (
        !visible ||
        !track_type_is_compatible_for_moving (
           region_track->type,
           visible->type) ||
        /* do not allow moving automation tracks
         * to other tracks for now */
        region->id.type == REGION_TYPE_AUTOMATION)
        {
          compatible = 0;
          break;
        }
    }
  if (!compatible)
    return 0;

  /* new positions are all compatible, move the
   * regions */
  for (i = 0; i < num_regions; i++)
    {
      region = regions[i];
      r_obj = (ArrangerObject *) region;
      Track * region_track =
        arranger_object_get_track (r_obj);
      g_warn_if_fail (region && region_track);
      Track * track_to_move_to =
        tracklist_get_visible_track_after_delta (
          TRACKLIST,
          region_track,
          vis_track_diff);
      g_warn_if_fail (track_to_move_to);

      region_move_to_track (
        region, track_to_move_to);
    }

  return 1;
}

/** Used when activating fade presets. */
typedef struct CurveOptionInfo
{
  CurveOptions     opts;
  ArrangerObject * obj;

  /** 1 for in, 0 for out. */
  int              fade_in;
} CurveOptionInfo;

static void
on_fade_preset_selected (
  GtkMenuItem *     menu_item,
  CurveOptionInfo * info)
{
  ArrangerSelections * sel_before =
    arranger_selections_clone (
      (ArrangerSelections *) TL_SELECTIONS);
  if (info->fade_in)
    {
      info->obj->fade_in_opts.algo =
        info->opts.algo;
      info->obj->fade_in_opts.curviness =
        info->opts.curviness;
    }
  else
    {
      info->obj->fade_out_opts.algo =
        info->opts.algo;
      info->obj->fade_out_opts.curviness =
        info->opts.curviness;
    }

  g_warn_if_fail (
    arranger_object_is_selected (info->obj));
  UndoableAction * ua =
    arranger_selections_action_new_edit (
      sel_before,
      (ArrangerSelections *) TL_SELECTIONS,
      ARRANGER_SELECTIONS_ACTION_EDIT_FADES,
      true);
  undo_manager_perform (UNDO_MANAGER, ua);

  g_warn_if_fail (IS_ARRANGER_OBJECT (info->obj));
  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CHANGED, info->obj);

  object_zero_and_free (info);
}

/**
 * @param fade_in 1 for in, 0 for out.
 */
static void
create_fade_preset_menu (
  ArrangerWidget * self,
  GtkWidget *      menu,
  ArrangerObject * obj,
  int              fade_in)
{
  GtkWidget * menuitem =
    gtk_menu_item_new_with_label (_("Fade preset"));
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), menuitem);
  GtkMenu * submenu = GTK_MENU (gtk_menu_new ());
  gtk_widget_set_visible (GTK_WIDGET (submenu), 1);
  GtkMenuItem * submenu_item;
  CurveOptionInfo * opts;

#define CREATE_ITEM(name,xalgo,curve) \
  submenu_item = \
    GTK_MENU_ITEM ( \
      gtk_menu_item_new_with_label (name)); \
  opts = calloc (1, sizeof (CurveOptionInfo)); \
  opts->opts.algo = CURVE_ALGORITHM_##xalgo; \
  opts->opts.curviness = curve; \
  opts->fade_in = fade_in; \
  opts->obj = obj; \
  g_signal_connect ( \
    G_OBJECT (submenu_item), "activate", \
    G_CALLBACK (on_fade_preset_selected), opts); \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (submenu), \
    GTK_WIDGET (submenu_item)); \
  gtk_widget_set_visible ( \
    GTK_WIDGET (submenu_item), 1)

  CREATE_ITEM (_("Linear"), SUPERELLIPSE, 0);
  CREATE_ITEM (_("Exponential"), EXPONENT, - 0.6);
  CREATE_ITEM (_("Elliptic"), SUPERELLIPSE, - 0.5);
  CREATE_ITEM (_("Vital"), VITAL, - 0.5);

#undef CREATE_ITEM

  gtk_menu_item_set_submenu (
    GTK_MENU_ITEM (menuitem),
    GTK_WIDGET (submenu));
  gtk_widget_set_visible (
    GTK_WIDGET (menuitem), 1);
}

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
  UndoableAction * ua =
    arranger_selections_action_new_edit (
      sel_before,
      (ArrangerSelections *) TL_SELECTIONS,
      ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE,
      true);
  undo_manager_perform (UNDO_MANAGER, ua);

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
      nfo = calloc (1, sizeof (MusicalModeInfo));
      nfo->mode = i;
      nfo->obj = obj;
      g_signal_connect (
        G_OBJECT (submenu_item[i]), "toggled",
        G_CALLBACK (on_musical_mode_toggled), nfo);
    }
}

/**
 * Show context menu at x, y.
 */
void
timeline_arranger_widget_show_context_menu (
  ArrangerWidget * self,
  double           x,
  double           y)
{
  GtkWidget *menu, *menuitem;
  menu = gtk_menu_new();

#define APPEND_TO_MENU \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), menuitem)

  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_ALL, x, y);

  if (obj)
    {
      int local_x = (int) (x - obj->full_rect.x);
      int local_y = (int) (y - obj->full_rect.y);

      /* create cut, copy, duplicate, delete */
      menuitem =
        GTK_WIDGET (
          CREATE_CUT_MENU_ITEM ("win.cut"));
      APPEND_TO_MENU;
      menuitem =
        GTK_WIDGET (
          CREATE_COPY_MENU_ITEM ("win.copy"));
      APPEND_TO_MENU;
      menuitem =
        GTK_WIDGET (
          CREATE_DUPLICATE_MENU_ITEM (
            "win.duplicate"));
      APPEND_TO_MENU;
      menuitem =
        GTK_WIDGET (
          CREATE_DELETE_MENU_ITEM ("win.delete"));
      APPEND_TO_MENU;
      menuitem =
        gtk_separator_menu_item_new ();
      APPEND_TO_MENU;

      if (obj->type == ARRANGER_OBJECT_TYPE_REGION)
        {
          ZRegion * r = (ZRegion *) obj;

          if (arranger_object_get_muted (obj))
            {
              menuitem =
                GTK_WIDGET (
                  CREATE_UNMUTE_MENU_ITEM (
                    "win.mute-selection"));
              gtk_actionable_set_action_target (
                GTK_ACTIONABLE (menuitem),
                "s", "timeline");
            }
          else
            {
              menuitem =
                GTK_WIDGET (
                  CREATE_MUTE_MENU_ITEM (
                    "win.mute-selection"));
              gtk_actionable_set_action_target (
                GTK_ACTIONABLE (menuitem),
                "s", "timeline");
            }
          gtk_menu_shell_append (
            GTK_MENU_SHELL(menu), menuitem);

          if (r->id.type == REGION_TYPE_MIDI)
            {
              menuitem =
                gtk_menu_item_new_with_label (
                  _("Export as MIDI file"));
              gtk_menu_shell_append (
                GTK_MENU_SHELL(menu), menuitem);
              g_signal_connect (
                menuitem, "activate",
                G_CALLBACK (
                  timeline_arranger_on_export_as_midi_file_clicked),
                r);
            }

          if (r->id.type == REGION_TYPE_AUDIO)
            {
              /* create fade menus */
              if (arranger_object_is_fade_in (
                    obj, local_x, local_y, 0, 0))
                {
                  create_fade_preset_menu (
                    self, menu, obj, 1);
                }
              if (arranger_object_is_fade_out (
                    obj, local_x, local_y, 0, 0))
                {
                  create_fade_preset_menu (
                    self, menu, obj, 0);
                }

              /* create musical mode menu */
              create_musical_mode_pset_menu (
                self, menu, obj);
            }

          if (r->id.type == REGION_TYPE_MIDI)
            {
              menuitem =
                gtk_menu_item_new_with_label (
                  _("Quick bounce"));
              gtk_menu_shell_append (
                GTK_MENU_SHELL(menu), menuitem);
              g_signal_connect (
                menuitem, "activate",
                G_CALLBACK (
                  timeline_arranger_on_quick_bounce_clicked),
                r);

              menuitem =
                gtk_menu_item_new_with_label (
                  _("Bounce..."));
              gtk_menu_shell_append (
                GTK_MENU_SHELL(menu), menuitem);
              g_signal_connect (
                menuitem, "activate",
                G_CALLBACK (
                  timeline_arranger_on_bounce_clicked),
                r);
            }
        }
    }
  else
    {
      menuitem =
        GTK_WIDGET (
          CREATE_PASTE_MENU_ITEM ("win.paste"));
      APPEND_TO_MENU;
    }

#undef APPEND_TO_MENU

  gtk_menu_attach_to_widget (
    GTK_MENU (menu), GTK_WIDGET (self), NULL);
  gtk_widget_show_all (menu);
  gtk_menu_popup_at_pointer (
    GTK_MENU (menu), NULL);
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
