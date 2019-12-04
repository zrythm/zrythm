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

/**
 * \file
 *
 * The timeline containing regions and other
 * objects.
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
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_region.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/export_midi_file_dialog.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/inspector.h"
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
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

/**
 * To be called from get_child_position in parent
 * widget.
 *
 * Used to allocate the overlay children.
 */
/*void*/
/*timeline_arranger_widget_set_allocation (*/
  /*TimelineArrangerWidget * self,*/
  /*GtkWidget *          widget,*/
  /*GdkRectangle *       allocation)*/
/*{*/
  /*else if (Z_IS_SCALE_OBJECT_WIDGET (widget))*/
    /*{*/
    /*}*/
  /*else if (Z_IS_MARKER_WIDGET (widget))*/
    /*{*/
    /*}*/
/*}*/

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
/*ArrangerCursor*/
/*timeline_arranger_widget_get_cursor (*/
  /*ArrangerWidget * self,*/
  /*UiOverlayAction action,*/
  /*Tool            tool)*/
/*{*/
  /*ArrangerCursor ac = ARRANGER_CURSOR_SELECT;*/


/*}*/

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
  Region *      r)
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

/**
 * Create a Region at the given Position in the
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
  Region * region = NULL;
  switch (type)
    {
    case REGION_TYPE_MIDI:
      region =
        midi_region_new (
          pos, &end_pos, 1);
      break;
    case REGION_TYPE_AUDIO:
      break;
    case REGION_TYPE_CHORD:
      region =
        chord_region_new (
          pos, &end_pos, 1);
      break;
    case REGION_TYPE_AUTOMATION:
      region =
        automation_region_new (
          pos, &end_pos, 1);
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
    F_CACHED, F_NO_VALIDATE);
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
    marker_new (
      _("Custom Marker"), 1);
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
  Region *                 region,
  Position *               new_start_pos,
  int                      dry_run)
{
  if (SNAP_GRID_ANY_SNAP (self->snap_grid) &&
        !self->shift_held)
    position_snap (
      NULL, new_start_pos, region->lane->track,
      NULL, self->snap_grid);

  ArrangerObject * r_obj =
    (ArrangerObject *) region;

  if (position_is_after_or_equal (
        new_start_pos, &r_obj->end_pos))
    return -1;
  else if (!dry_run)
    {
      if (arranger_object_validate_pos (
            r_obj, new_start_pos,
            ARRANGER_OBJECT_POSITION_TYPE_START))
        {
          /*region_set_start_pos (*/
            /*region, new_start_pos,*/
            /*AO_UPDATE_ALL);*/
          long diff =
            position_to_ticks (new_start_pos) -
            position_to_ticks (
              &r_obj->pos);
          arranger_object_resize (
            r_obj, 1,
            self->action ==
              UI_OVERLAY_ACTION_RESIZING_L_LOOP,
            diff);
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
  Position *               pos,
  int                      dry_run)
{
  ArrangerObject * start_r_obj =
    self->start_object;

  /* get delta with first clicked region's start
   * pos */
  long delta;
  delta =
    position_to_ticks (pos) -
    position_to_ticks (
      &start_r_obj->cache_pos);

  /* new start pos for each region, calculated by
   * adding delta to the region's original start
   * pos */
  Position new_start_pos;

  Region * region;
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
      position_set_to_pos (
        &new_start_pos,
        &r_obj->cache_pos);
      position_add_ticks (
        &new_start_pos, delta);

      ret =
        snap_region_l (
          self, region, &new_start_pos, dry_run);

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
  Region * region,
  Position * new_end_pos,
  int        dry_run)
{
  if (SNAP_GRID_ANY_SNAP (self->snap_grid) &&
        !self->shift_held)
    {
      switch (region->type)
        {
        case REGION_TYPE_CHORD:
          position_snap (
            NULL, new_end_pos, P_CHORD_TRACK,
            NULL, self->snap_grid);
          break;
        case REGION_TYPE_MIDI:
        case REGION_TYPE_AUDIO:
          position_snap (
            NULL, new_end_pos, region->lane->track,
            NULL, self->snap_grid);
          break;
        case REGION_TYPE_AUTOMATION:
          position_snap (
            NULL, new_end_pos, NULL,
            NULL, self->snap_grid);
          break;
        }
    }

  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  if (position_is_before_or_equal (
        new_end_pos, &r_obj->pos))
    return -1;
  else if (!dry_run)
    {
      if (arranger_object_validate_pos (
            r_obj, new_end_pos,
            ARRANGER_OBJECT_POSITION_TYPE_END))
        {
          /*region_set_end_pos (*/
            /*region, new_end_pos, AO_UPDATE_ALL);*/
          long diff =
            position_to_ticks (new_end_pos) -
            position_to_ticks (&r_obj->end_pos);
          arranger_object_resize (
            r_obj, 0,
            self->action ==
              UI_OVERLAY_ACTION_RESIZING_R_LOOP,
            diff);

          /* if creating also set the loop points
           * appropriately */
          if (self->action ==
                UI_OVERLAY_ACTION_CREATING_RESIZING_R)
            {
              long full_size =
                arranger_object_get_length_in_ticks (
                  r_obj);
              Position tmp;
              position_set_to_pos (
                &tmp, &r_obj->loop_start_pos);
              position_add_ticks (
                &tmp, full_size);

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
  Position *               pos,
  int                      dry_run)
{
  ArrangerObject * start_r_obj =
    self->start_object;

  /* get delta with first clicked region's end
   * pos */
  long delta =
    position_to_ticks (pos) -
    position_to_ticks (
      &start_r_obj->cache_end_pos);

  /* new end pos for each region, calculated by
   * adding delta to the region's original end
   * pos */
  Position new_end_pos;

  Region * region;
  ArrangerObject * r_obj;
  int ret;
  for (int i = 0;
       i < TL_SELECTIONS->num_regions;
       i++)
    {
      region =
        TL_SELECTIONS->regions[i];
      r_obj = (ArrangerObject *) region;

      position_set_to_pos (
        &new_end_pos,
        &r_obj->cache_end_pos);
      position_add_ticks (
        &new_end_pos, delta);

      ret =
        snap_region_r (
          self, region, &new_end_pos, dry_run);

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
        &PROJECT->range_1,
        1);
      if (SNAP_GRID_ANY_SNAP (
            self->snap_grid) &&
          !self->shift_held)
        position_snap_simple (
          &PROJECT->range_1,
          SNAP_GRID_TIMELINE);
      position_set_to_pos (
        &PROJECT->range_2,
        &PROJECT->range_1);

      MW_TIMELINE->resizing_range_start = 0;
    }

  /* set range */
  if (SNAP_GRID_ANY_SNAP (self->snap_grid) &&
      !self->shift_held)
    position_snap_simple (
      pos,
      SNAP_GRID_TIMELINE);
  position_set_to_pos (
    &PROJECT->range_2, pos);
  project_set_has_range (1);

  EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED,
               NULL);

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
  /*Region * region;*/
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
  Region * regions[600];
  int num_regions = 0;
  Region * region;
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
      TrackLane * lane = region->lane;
      if (lane->pos + diff < 0)
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
      TrackLane * lane = region->lane;
      g_return_val_if_fail (region && lane, -1);

      TrackLane * lane_to_move_to = NULL;
      int new_lane_pos =
        lane->pos +  diff;
      g_return_val_if_fail (
        new_lane_pos >= 0, -1);
      track_create_missing_lanes (
        lane->track, new_lane_pos);
      lane_to_move_to =
        lane->track->lanes[new_lane_pos];
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
  Region * regions[600];
  int num_regions = 0;
  Region * region;
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
           visible->type))
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

/*void*/
/*timeline_arranger_widget_move_items_y (*/
  /*ArrangerWidget * self,*/
  /*double                   offset_y)*/
/*{*/

/*}*/

/**
 * Sets the default cursor in all selected regions and
 * intializes start positions.
 */
/*void*/
/*timeline_arranger_widget_on_drag_end (*/
  /*ArrangerWidget * self)*/
/*{*/

/*}*/
