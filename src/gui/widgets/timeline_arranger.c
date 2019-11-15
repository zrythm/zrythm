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

G_DEFINE_TYPE (TimelineArrangerWidget,
               timeline_arranger_widget,
               ARRANGER_WIDGET_TYPE)

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
timeline_arranger_widget_set_allocation (
  TimelineArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  if (Z_IS_REGION_WIDGET (widget))
    {
      RegionWidget * rw = (RegionWidget*) (widget);
      REGION_WIDGET_GET_PRIVATE (rw);
      Region * region = rw_prv->region;
      ArrangerObject * r_obj =
        (ArrangerObject *) region;
      TrackLane * lane = region->lane;
      Track * track =
        arranger_object_get_track (r_obj);

      /* if region has a tmp_lane then it means
       * that we are in an operation we should
       * display it there instead of its original
       * lane */
      if (ar_prv->action !=
            UI_OVERLAY_ACTION_NONE &&
          region->tmp_lane)
        {
          lane = region->tmp_lane;
          track = lane->track;
        }

      if (!track->widget)
        track->widget = track_widget_new (track);

      allocation->x =
        ui_pos_to_px_timeline (
          &r_obj->pos,
          1);
      allocation->width =
        (ui_pos_to_px_timeline (
          &r_obj->end_pos,
          1) - allocation->x) - 1;
      if (allocation->width < 1)
        allocation->width = 1;

      gint wx, wy;
      if (arranger_object_is_lane (r_obj))
        {
          if (!track->lanes_visible ||
              !GTK_IS_WIDGET (lane->widget))
            return;

          gtk_widget_translate_coordinates(
            (GtkWidget *) (lane->widget),
            (GtkWidget *) (self),
            0, 0,
            &wx, &wy);

          allocation->y = wy;

          allocation->height =
            gtk_widget_get_allocated_height (
              GTK_WIDGET (lane->widget));
        }
      else
        {
          gtk_widget_translate_coordinates(
            (GtkWidget *) (track->widget),
            (GtkWidget *) (self),
            0, 0,
            &wx, &wy);

          if (region->type == REGION_TYPE_CHORD)
            {
              if (!self->chord_obj_height)
                {
                  int textw, texth;
                  PangoLayout * layout =
                    z_cairo_create_default_pango_layout (
                      widget);
                  z_cairo_get_text_extents_for_widget (
                    widget, layout,
                    "Aa", &textw, &texth);
                  g_object_unref (layout);
                  self->chord_obj_height = texth;
                }

              allocation->y = wy;
              /* full height minus the space the
               * scales would require, plus some
               * padding */
              allocation->height =
                gtk_widget_get_allocated_height (
                  GTK_WIDGET (track->widget)) -
                (self->chord_obj_height +
                   Z_CAIRO_TEXT_PADDING * 4);

            }
          else if (region->type ==
                     REGION_TYPE_AUTOMATION)
            {
              AutomationTrack * at =
                region->at;
              if (!at->created ||
                  !track->automation_visible)
                return;

              gtk_widget_translate_coordinates (
                (GtkWidget *) (at->widget),
                (GtkWidget *) (self),
                0, 0, &wx, &wy);
              allocation->y = wy;
              allocation->height =
                gtk_widget_get_allocated_height (
                  (GtkWidget *) (at->widget));
            }
          else
            {
              allocation->y = wy;
              allocation->height =
                gtk_widget_get_allocated_height (
                  (GtkWidget *) (track->widget));
            }
        }
    }
  else if (Z_IS_SCALE_OBJECT_WIDGET (widget))
    {
      ScaleObjectWidget * sw =
        (ScaleObjectWidget *) (widget);
      ScaleObject * so = sw->scale_object;
      ArrangerObject * obj =
        (ArrangerObject *) so;
      Track * track = P_CHORD_TRACK;

      gint wx, wy;
      gtk_widget_translate_coordinates (
        (GtkWidget *) (track->widget),
        (GtkWidget *) (self),
        0, 0, &wx, &wy);

      allocation->x =
        ui_pos_to_px_timeline (
          &obj->pos, 1);
      allocation->width =
        sw->textw + SCALE_OBJECT_WIDGET_TRIANGLE_W +
        Z_CAIRO_TEXT_PADDING * 2;

      int track_height =
        gtk_widget_get_allocated_height (
          (GtkWidget *) (track->widget));
      int obj_height =
        sw->texth + Z_CAIRO_TEXT_PADDING * 2;
      allocation->y =
        (wy + track_height) - obj_height;
      allocation->height = obj_height;
    }
  else if (Z_IS_MARKER_WIDGET (widget))
    {
      MarkerWidget * mw =
        (MarkerWidget *) (widget);
      ArrangerObject * obj =
        (ArrangerObject *) mw->marker;
      Track * track = P_MARKER_TRACK;

      gint wx, wy;
      gtk_widget_translate_coordinates (
        (GtkWidget *) (track->widget),
        (GtkWidget *) (self),
        0, 0, &wx, &wy);

      allocation->x =
        ui_pos_to_px_timeline (
          &obj->pos, 1);
      allocation->width =
        mw->textw + MARKER_WIDGET_TRIANGLE_W +
        Z_CAIRO_TEXT_PADDING * 2;

      int track_height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget));
      int obj_height =
        mw->texth + Z_CAIRO_TEXT_PADDING * 2;
      allocation->y =
        (wy + track_height) - obj_height;
      allocation->height = obj_height;
    }
}

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
timeline_arranger_widget_get_cursor (
  TimelineArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  ArrangerObject * r_obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_REGION,
      ar_prv->hover_x, ar_prv->hover_y);
  ArrangerObject * s_obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_SCALE_OBJECT,
      ar_prv->hover_x, ar_prv->hover_y);
  ArrangerObject * m_obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_MARKER,
      ar_prv->hover_x, ar_prv->hover_y);

  int is_hit = r_obj || s_obj || m_obj;

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      switch (P_TOOL)
        {
        case TOOL_SELECT_NORMAL:
        {
          if (is_hit)
            {
              if (r_obj)
                {
                  if (ar_prv->alt_held)
                    return ARRANGER_CURSOR_CUT;

                  ArrangerObjectWidget * obj_w =
                    Z_ARRANGER_OBJECT_WIDGET (
                      r_obj->widget);
                  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (obj_w);
                  int is_resize_l =
                    ao_prv->resize_l;
                  int is_resize_r =
                    ao_prv->resize_r;
                  int is_resize_loop =
                    ao_prv->resize_loop;
                  if (is_resize_l &&
                      is_resize_loop)
                    return
                      ARRANGER_CURSOR_RESIZING_L_LOOP;
                  else if (is_resize_l)
                    return
                      ARRANGER_CURSOR_RESIZING_L;
                  else if (is_resize_r &&
                           is_resize_loop)
                    return
                      ARRANGER_CURSOR_RESIZING_R_LOOP;
                  else if (is_resize_r)
                    return
                      ARRANGER_CURSOR_RESIZING_R;
                }
              return ARRANGER_CURSOR_GRAB;
            }
          else
            {
              Track * track =
                timeline_arranger_widget_get_track_at_y (
                self, ar_prv->hover_y);

              if (track)
                {
                  if (track_widget_is_cursor_in_top_half (
                        track->widget,
                        ar_prv->hover_y))
                    {
                      /* set cursor to normal */
                      return ARRANGER_CURSOR_SELECT;
                    }
                  else
                    {
                      /* set cursor to range selection */
                      return ARRANGER_CURSOR_RANGE;
                    }
                }
              else
                {
                  /* set cursor to normal */
                  return ARRANGER_CURSOR_SELECT;
                }
            }
        }
          break;
        case TOOL_SELECT_STRETCH:
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
    case UI_OVERLAY_ACTION_RESIZING_L:
      if (self->resizing_range)
        ac = ARRANGER_CURSOR_RANGE;
      else
        ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
      if (self->resizing_range)
        ac = ARRANGER_CURSOR_RANGE;
      else
        ac = ARRANGER_CURSOR_RESIZING_L_LOOP;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      if (self->resizing_range)
        ac = ARRANGER_CURSOR_RANGE;
      else
        ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      if (self->resizing_range)
        ac = ARRANGER_CURSOR_RANGE;
      else
        ac = ARRANGER_CURSOR_RESIZING_R_LOOP;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
}

/**
 * Hides the cut dashed line from hovered regions
 * and redraws them.
 *
 * Used when alt was unpressed.
 */
void
timeline_arranger_widget_set_cut_lines_visible (
  TimelineArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_REGION,
      ar_prv->hover_x,
      ar_prv->hover_y);

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
}

TrackLane *
timeline_arranger_widget_get_track_lane_at_y (
  TimelineArrangerWidget * self,
  double y)
{
  int i, j;
  Track * track;
  TrackLane * lane;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      for (j = 0; j < track->num_lanes; j++)
        {
          lane = track->lanes[j];

          if (!lane->widget ||
              !GTK_IS_WIDGET (lane->widget))
            continue;

          if (ui_is_child_hit (
                GTK_WIDGET (self),
                GTK_WIDGET (lane->widget),
                0, 1, 0, y, 0, 0))
            return lane;
        }
    }

  return NULL;
}

Track *
timeline_arranger_widget_get_track_at_y (
  TimelineArrangerWidget * self,
  double y)
{
  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (!track->visible)
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
timeline_arranger_widget_get_automation_track_at_y (
  TimelineArrangerWidget * self,
  double                   y)
{
  int i, j;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      AutomationTracklist * atl =
        track_get_automation_tracklist (track);
      if (!atl ||
          !track->automation_visible ||
          (track->pinned && (
             self == MW_TIMELINE)) ||
          (!track->pinned && (
             self == MW_PINNED_TIMELINE)))
        continue;

      AutomationTrack * at;
      for (j = 0; j < atl->num_ats; j++)
        {
          at = atl->ats[j];

          if (!at->created)
            continue;

          /* TODO check the rest */
          if (at->visible && at->widget &&
              ui_is_child_hit (
                GTK_WIDGET (MW_TIMELINE),
                GTK_WIDGET (at->widget),
                0, 1, 0, y, 0, 0))
            return at;
        }
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
  TimelineArrangerWidget * self,
  const RegionType         type,
  Track *                  track,
  TrackLane *              lane,
  AutomationTrack *        at,
  const Position *         pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ar_prv->action =
    UI_OVERLAY_ACTION_CREATING_RESIZING_R;

  Position end_pos;
  position_set_min_size (
    pos, &end_pos,
    ar_prv->snap_grid);

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
  ar_prv->start_object = r_obj;
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
  arranger_object_gen_widget (r_obj);
  arranger_object_set_widget_visibility_and_state (
     r_obj, 1);

  arranger_object_set_position (
    r_obj, &r_obj->end_pos,
    ARRANGER_OBJECT_POSITION_TYPE_END,
    F_CACHED, F_NO_VALIDATE, AO_UPDATE_ALL);
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND);
}

/**
 * Wrapper for
 * timeline_arranger_widget_create_chord() or
 * timeline_arranger_widget_create_scale().
 *
 * @param y the y relative to the
 *   TimelineArrangerWidget.
 */
void
timeline_arranger_widget_create_chord_or_scale (
  TimelineArrangerWidget * self,
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
  TimelineArrangerWidget * self,
  Track *            track,
  const Position *         pos)
{
  g_warn_if_fail (track->type == TRACK_TYPE_CHORD);

  ARRANGER_WIDGET_GET_PRIVATE (self);

  ar_prv->action =
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

  arranger_object_gen_widget (scale_obj);

  /* set visibility */
  arranger_object_set_widget_visibility_and_state (
    scale_obj, 1);

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
  TimelineArrangerWidget * self,
  Track *            track,
  const Position *         pos)
{
  g_warn_if_fail (
    track->type == TRACK_TYPE_MARKER);

  ARRANGER_WIDGET_GET_PRIVATE (self);

  ar_prv->action =
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

  arranger_object_gen_widget (marker_obj);

  /* set visibility */
  arranger_object_set_widget_visibility_and_state (
    marker_obj, 1);

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
  TimelineArrangerWidget * self,
  double                   y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

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
          ar_prv->action =
            UI_OVERLAY_ACTION_RESIZING_R;
        }
    }
  else
    {
      /* TODO something similar as above based on
       * visible space */
      self->resizing_range = 0;
    }

  arranger_widget_refresh_all_backgrounds ();
  gtk_widget_queue_allocate (
    GTK_WIDGET (MW_RULER));
}

/**
 * First determines the selection type (objects/
 * range), then either finds and selects items or
 * selects a range.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
void
timeline_arranger_widget_select (
  TimelineArrangerWidget * self,
  double                   offset_x,
  double                   offset_y,
  int                      delete)
{
  int i;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (!delete)
    /* deselect all */
    arranger_widget_select_all (
      Z_ARRANGER_WIDGET (self), 0);

#define FIND_ENCLOSED_WIDGETS_OF_TYPE( \
  caps,cc,sc) \
  cc * sc; \
  cc##Widget * sc##_widget; \
  GtkWidget *  sc##_widgets[800]; \
  int          num_##sc##_widgets = 0; \
  arranger_widget_get_hit_widgets_in_range ( \
    (ArrangerWidget *) (self), \
    caps##_WIDGET_TYPE, \
    ar_prv->start_x, \
    ar_prv->start_y, \
    offset_x, \
    offset_y, \
    sc##_widgets, \
    &num_##sc##_widgets)

  FIND_ENCLOSED_WIDGETS_OF_TYPE (
    REGION, Region, region);
  for (i = 0; i < num_region_widgets; i++)
    {
      region_widget =
        (RegionWidget *) (region_widgets[i]);
      REGION_WIDGET_GET_PRIVATE (region_widget);

      region =
        region_get_main (
          rw_prv->region);

      if (delete)
        {
          /* delete the enclosed region */
          track_remove_region (
            region->lane->track, region,
            F_PUBLISH_EVENTS, F_FREE);
        }
      else
        {
          /* select the enclosed region */
          arranger_object_select (
            (ArrangerObject *) region,
            F_SELECT, F_APPEND);
        }
    }

  FIND_ENCLOSED_WIDGETS_OF_TYPE (
    SCALE_OBJECT, ScaleObject, scale_object);
  for (i = 0; i < num_scale_object_widgets; i++)
    {
      scale_object_widget =
        Z_SCALE_OBJECT_WIDGET (
          scale_object_widgets[i]);

      scale_object =
        scale_object_get_main (
          scale_object_widget->scale_object);

      if (delete)
        chord_track_remove_scale (
          P_CHORD_TRACK,
          scale_object, F_FREE);
      else
        arranger_object_select (
          (ArrangerObject *) scale_object,
          F_SELECT, F_APPEND);
    }

  FIND_ENCLOSED_WIDGETS_OF_TYPE (
    MARKER, Marker, marker);
  for (i = 0; i < num_marker_widgets; i++)
    {
      marker_widget =
        Z_MARKER_WIDGET (
          marker_widgets[i]);

      marker =
        marker_get_main (
          marker_widget->marker);

      if (delete)
        marker_track_remove_marker (
          P_MARKER_TRACK, marker, F_FREE);
      else
        arranger_object_select (
          (ArrangerObject *) marker,
          F_SELECT, F_APPEND);
    }

#undef FIND_ENCLOSED_WIDGETS_OF_TYPE
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
  TimelineArrangerWidget * self,
  Region *                 region,
  Position *               new_start_pos,
  int                      dry_run)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
        !ar_prv->shift_held)
    position_snap (
      NULL, new_start_pos, region->lane->track,
      NULL, ar_prv->snap_grid);

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
            ar_prv->action ==
              UI_OVERLAY_ACTION_RESIZING_L_LOOP,
            diff, AO_UPDATE_ALL);
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
  TimelineArrangerWidget * self,
  Position *               pos,
  int                      dry_run)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ArrangerObject * start_r_obj =
    ar_prv->start_object;

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
        region_get_main (
          TL_SELECTIONS->regions[i]);
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
  TimelineArrangerWidget * self,
  Region * region,
  Position * new_end_pos,
  int        dry_run)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
        !ar_prv->shift_held)
    {
      switch (region->type)
        {
        case REGION_TYPE_CHORD:
          position_snap (
            NULL, new_end_pos, P_CHORD_TRACK,
            NULL, ar_prv->snap_grid);
          break;
        case REGION_TYPE_MIDI:
        case REGION_TYPE_AUDIO:
          position_snap (
            NULL, new_end_pos, region->lane->track,
            NULL, ar_prv->snap_grid);
          break;
        case REGION_TYPE_AUTOMATION:
          position_snap (
            NULL, new_end_pos, NULL,
            NULL, ar_prv->snap_grid);
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
            ar_prv->action ==
              UI_OVERLAY_ACTION_RESIZING_R_LOOP,
            diff, AO_UPDATE_ALL);

          /* if creating also set the loop points
           * appropriately */
          if (ar_prv->action ==
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
  TimelineArrangerWidget * self,
  Position *               pos,
  int                      dry_run)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ArrangerObject * start_r_obj =
    ar_prv->start_object;

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

      /* main region */
      region =
        region_get_main (region);
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
  Position *               pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (MW_TIMELINE);

  if (MW_TIMELINE->resizing_range_start)
    {
      /* set range 1 at current point */
      ui_px_to_pos_timeline (
        ar_prv->start_x,
        &PROJECT->range_1,
        1);
      if (SNAP_GRID_ANY_SNAP (
            ar_prv->snap_grid) &&
          !ar_prv->shift_held)
        position_snap_simple (
          &PROJECT->range_1,
          SNAP_GRID_TIMELINE);
      position_set_to_pos (
        &PROJECT->range_2,
        &PROJECT->range_1);

      MW_TIMELINE->resizing_range_start = 0;
    }

  /* set range */
  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
      !ar_prv->shift_held)
    position_snap_simple (
      pos,
      SNAP_GRID_TIMELINE);
  position_set_to_pos (
    &PROJECT->range_2, pos);
  project_set_has_range (1);

  EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED,
               NULL);

  arranger_widget_refresh_all_backgrounds ();
}

/**
 * Sets width to ruler width and height to
 * tracklist height.
 */
void
timeline_arranger_widget_set_size (
  TimelineArrangerWidget * self)
{
  // set the size
  int ww, hh;
  if (self->is_pinned)
    {
    /*gtk_widget_get_size_request (*/
      /*GTK_WIDGET (MW_PINNED_TRACKLIST),*/
      /*&ww,*/
      /*&hh);*/
      ww = 80;
      hh = 80;
    }
  else
    gtk_widget_get_size_request (
      GTK_WIDGET (MW_TRACKLIST),
      &ww,
      &hh);
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    (int) rw_prv->total_px,
    hh);
}

#define COMPARE_AND_SET(pos) \
  if ((pos)->bars > self->last_timeline_obj_bars) \
    self->last_timeline_obj_bars = (pos)->bars;

/**
 * Updates last timeline objet so that timeline can be
 * expanded/contracted accordingly.
 */
static int
update_last_timeline_object ()
{
  if (!ZRYTHM || !MAIN_WINDOW ||
      !GTK_IS_WIDGET (MAIN_WINDOW))
    return G_SOURCE_CONTINUE;

  TimelineArrangerWidget * self = MW_TIMELINE;

  int prev = self->last_timeline_obj_bars;
  self->last_timeline_obj_bars = 0;

  Marker * end =
    marker_track_get_end_marker (
      P_MARKER_TRACK);
  ArrangerObject * end_obj =
    (ArrangerObject *) end;
  COMPARE_AND_SET (&end_obj->pos);

  Track * track;
  Region * region;
  ArrangerObject * r_obj;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      region =
        track_get_last_region (track);
      r_obj = (ArrangerObject *) region;

      if (region)
        COMPARE_AND_SET (&r_obj->end_pos);
    }

  if (prev != self->last_timeline_obj_bars &&
      self->last_timeline_obj_bars > 127)
    EVENTS_PUSH (ET_LAST_TIMELINE_OBJECT_CHANGED,
                 NULL);

  return G_SOURCE_CONTINUE;
}

#undef COMPARE_AND_SET

/**
 * To be called once at init time.
 */
void
timeline_arranger_widget_setup (
  TimelineArrangerWidget * self)
{
  timeline_arranger_widget_set_size (
    self);

  g_timeout_add (
    1000,
    update_last_timeline_object,
    NULL);
}

/**
 * Move the selected Regions to new Lanes.
 *
 * @param diff The delta to move the
 *   Tracks.
 *
 * @return 1 if moved.
 */
static int
move_regions_to_new_lanes (
  TimelineArrangerWidget * self,
  const int                diff)
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
      region =
        region_get_lane (region);
      TrackLane * lane = region->lane;
      if (region->tmp_lane)
        lane = region->tmp_lane;
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
      region =
        region_get_lane (region);
      TrackLane * lane = region->lane;
      if (region->tmp_lane)
        lane = region->tmp_lane;
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
        region, lane_to_move_to, 1);
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
static int
move_regions_to_new_tracks (
  TimelineArrangerWidget * self,
  const int                vis_track_diff)
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
      region =
        (Region *)
        arranger_object_get_visible_counterpart (
          (ArrangerObject *) region);
      r_obj = (ArrangerObject *) region;
      Track * region_track =
        arranger_object_get_track (r_obj);
      if (region->tmp_lane)
        region_track = region->tmp_lane->track;
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
      r_obj =
        arranger_object_get_visible_counterpart (
          (ArrangerObject *) region);
      region = (Region *) r_obj;
      Track * region_track =
        arranger_object_get_track (r_obj);
      if (region->tmp_lane)
        region_track = region->tmp_lane->track;
      g_warn_if_fail (region && region_track);
      Track * track_to_move_to =
        tracklist_get_visible_track_after_delta (
          TRACKLIST,
          region_track,
          vis_track_diff);
      g_warn_if_fail (track_to_move_to);

      region_move_to_track (
        region, track_to_move_to, 1);
    }

  return 1;
}

void
timeline_arranger_widget_move_items_y (
  TimelineArrangerWidget * self,
  double                   offset_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* get old track, track where last change
   * happened, and new track */
  Track * track =
    timeline_arranger_widget_get_track_at_y (
    self, ar_prv->start_y + offset_y);
  Track * old_track =
    timeline_arranger_widget_get_track_at_y (
    self, ar_prv->start_y);
  Track * last_track =
    tracklist_get_visible_track_after_delta (
      TRACKLIST, old_track,
      self->visible_track_diff);

  /* TODO automations and other lanes */
  TrackLane * lane =
    timeline_arranger_widget_get_track_lane_at_y (
    self, ar_prv->start_y + offset_y);
  TrackLane * old_lane =
    timeline_arranger_widget_get_track_lane_at_y (
    self, ar_prv->start_y);
  TrackLane * last_lane = NULL;
  if (old_lane)
    last_lane =
      old_lane->track->lanes[
        old_lane->pos + self->lane_diff];

  /* if new track is equal, move lanes or
   * automation lanes */
  if (track && last_track &&
      track == last_track &&
      self->visible_track_diff == 0 &&
      old_lane && lane && last_lane)
    {
      int cur_diff =
        lane->pos - old_lane->pos;
      int delta =
        lane->pos - last_lane->pos;
      if (delta != 0)
        {
          int moved =
            move_regions_to_new_lanes (
              self, delta);
          if (moved)
            {
              self->lane_diff =
                cur_diff;
            }
        }
    }
  /* otherwise move tracks */
  else if (track && last_track && old_track &&
           track != last_track)
    {
      int cur_diff =
        tracklist_get_visible_track_diff (
          TRACKLIST, old_track, track);
      int delta =
        tracklist_get_visible_track_diff (
          TRACKLIST, last_track, track);
      if (delta != 0)
        {
          int moved =
            move_regions_to_new_tracks (
              self, delta);
          if (moved)
            {
              self->visible_track_diff =
                cur_diff;
            }
        }
    }
}

/**
 * Sets the default cursor in all selected regions and
 * intializes start positions.
 */
void
timeline_arranger_widget_on_drag_end (
  TimelineArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* clear tmp_lane from selected regions */
  for (int i = 0; i < TL_SELECTIONS->num_regions;
       i++)
    {
      Region * region = TL_SELECTIONS->regions[i];
      arranger_object_set_primitive (
        Region, region, tmp_lane,
        NULL, AO_UPDATE_ALL);
    }

  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_RESIZING_L:
      if (!self->resizing_range)
        {
          ArrangerObject * main_trans_region =
            arranger_object_get_main_trans (
            TL_SELECTIONS->regions[0]);
          UndoableAction * ua =
            arranger_selections_action_new_resize (
              (ArrangerSelections *) TL_SELECTIONS,
              ARRANGER_SELECTIONS_ACTION_RESIZE_L,
              position_to_ticks (
                &main_trans_region->pos) -
              position_to_ticks (
                &main_trans_region->cache_pos));
          undo_manager_perform (
            UNDO_MANAGER, ua);
          arranger_selections_reset_counterparts (
            (ArrangerSelections *) TL_SELECTIONS,
            1);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
      if (!self->resizing_range)
        {
          ArrangerObject * main_trans_region =
            arranger_object_get_main_trans (
              TL_SELECTIONS->regions[0]);
          UndoableAction * ua =
            arranger_selections_action_new_resize (
              (ArrangerSelections *) TL_SELECTIONS,
              ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP,
              position_to_ticks (
                &main_trans_region->pos) -
              position_to_ticks (
                &main_trans_region->cache_pos));
          undo_manager_perform (
            UNDO_MANAGER, ua);
          arranger_selections_reset_counterparts (
            (ArrangerSelections *) TL_SELECTIONS,
            1);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      if (!self->resizing_range)
        {
          ArrangerObject * main_trans_region =
            arranger_object_get_main_trans (
              TL_SELECTIONS->regions[0]);
          UndoableAction * ua =
            arranger_selections_action_new_resize (
              (ArrangerSelections *) TL_SELECTIONS,
              ARRANGER_SELECTIONS_ACTION_RESIZE_R,
              position_to_ticks (
                &main_trans_region->end_pos) -
              position_to_ticks (
                &main_trans_region->cache_end_pos));
          undo_manager_perform (
            UNDO_MANAGER, ua);
          arranger_selections_reset_counterparts (
            (ArrangerSelections *) TL_SELECTIONS,
            1);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      if (!self->resizing_range)
        {
          ArrangerObject * main_trans_region =
            arranger_object_get_main_trans (
              TL_SELECTIONS->regions[0]);
          UndoableAction * ua =
            arranger_selections_action_new_resize (
              (ArrangerSelections *) TL_SELECTIONS,
              ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP,
              position_to_ticks (
                &main_trans_region->end_pos) -
              position_to_ticks (
                &main_trans_region->cache_end_pos));
          undo_manager_perform (
            UNDO_MANAGER, ua);
          arranger_selections_reset_counterparts (
            (ArrangerSelections *) TL_SELECTIONS,
            1);
        }
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      /* if something was clicked with ctrl without
       * moving*/
      if (ar_prv->ctrl_held)
        {
          if (ar_prv->start_object &&
              ar_prv->start_object_was_selected)
            {
              /* deselect it */
              arranger_object_select (
                ar_prv->start_object,
                F_NO_SELECT, F_APPEND);
            }
        }
      else if (ar_prv->n_press == 2)
        {
          /* double click on object */
          /*g_message ("DOUBLE CLICK");*/
        }
      break;
    case UI_OVERLAY_ACTION_MOVING:
      {
        Position earliest_main_pos,
                 earliest_trans_pos;
        arranger_selections_get_start_pos (
          (ArrangerSelections *) TL_SELECTIONS,
          &earliest_main_pos, F_NO_TRANSIENTS,
          F_GLOBAL);
        arranger_selections_get_start_pos (
          (ArrangerSelections *) TL_SELECTIONS,
          &earliest_trans_pos, F_TRANSIENTS,
          F_GLOBAL);
        long ticks_diff =
          earliest_main_pos.total_ticks -
            earliest_trans_pos.total_ticks;
        UndoableAction * ua =
          arranger_selections_action_new_move_timeline (
            TL_SELECTIONS,
            ticks_diff,
            self->visible_track_diff,
            self->lane_diff);
        undo_manager_perform (
          UNDO_MANAGER, ua);
        arranger_selections_reset_counterparts (
          (ArrangerSelections *) TL_SELECTIONS, 1);
      }
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      {
        Position earliest_main_pos,
                 earliest_trans_pos;
        arranger_selections_get_start_pos (
          (ArrangerSelections *) TL_SELECTIONS,
          &earliest_main_pos, F_NO_TRANSIENTS,
          F_GLOBAL);
        arranger_selections_get_start_pos (
          (ArrangerSelections *) TL_SELECTIONS,
          &earliest_trans_pos, F_TRANSIENTS,
          F_GLOBAL);
        long ticks_diff =
          earliest_main_pos.total_ticks -
            earliest_trans_pos.total_ticks;
        arranger_selections_reset_counterparts (
          (ArrangerSelections *) TL_SELECTIONS, 0);
        UndoableAction * ua =
          (UndoableAction *)
          arranger_selections_action_new_duplicate_timeline (
            TL_SELECTIONS,
            ticks_diff,
            self->visible_track_diff,
            self->lane_diff);
        arranger_selections_reset_counterparts (
          (ArrangerSelections *) TL_SELECTIONS, 1);
        arranger_selections_clear (
          (ArrangerSelections *) TL_SELECTIONS);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_NONE:
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      arranger_selections_clear (
        (ArrangerSelections *) TL_SELECTIONS);
      break;
    /* if something was created */
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      {
        arranger_selections_reset_counterparts (
          (ArrangerSelections *) TL_SELECTIONS, 0);

        UndoableAction * ua =
          arranger_selections_action_new_create (
            (ArrangerSelections *) TL_SELECTIONS);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    case UI_OVERLAY_ACTION_CUTTING:
      {
        /* get cut position */
        Position cut_pos;
        position_set_to_pos (
          &cut_pos, &ar_prv->curr_pos);

        if (SNAP_GRID_ANY_SNAP (
              ar_prv->snap_grid) &&
            !ar_prv->shift_held)
          position_snap_simple (
            &cut_pos, ar_prv->snap_grid);
        UndoableAction * ua =
          arranger_selections_action_new_split (
            (ArrangerSelections *) TL_SELECTIONS,
            &cut_pos);
        undo_manager_perform (
          UNDO_MANAGER, ua);
      }
      break;
    default:
      break;
    }

  self->resizing_range = 0;
  self->resizing_range_start = 0;
  self->visible_track_diff = 0;
  self->lane_diff = 0;
}

static void
add_children_from_channel_track (
  TimelineArrangerWidget * self,
  ChannelTrack * ct)
{
  if (!((Track *)ct)->automation_visible)
    return;

  int i,j,k;

  AutomationTracklist * atl =
    &ct->automation_tracklist;
  AutomationTrack * at;
  Region * region;
  for (i = 0; i < atl->num_ats; i++)
    {
      at = atl->ats[i];

      if (!at->created || !at->visible)
        continue;

      for (j = 0; j < at->num_regions; j++)
        {
          region = at->regions[j];

          for (k = 0; k < 2; k++)
            {
              if (k == 0)
                region =
                  region_get_main (region);
              else if (k == 1)
                region =
                  region_get_main_trans (
                    region);

              ArrangerObject * r_obj =
                (ArrangerObject *) region;

              if (!Z_IS_ARRANGER_OBJECT_WIDGET (
                    r_obj->widget))
                {
                  arranger_object_gen_widget (
                    r_obj);
                }

              arranger_widget_add_overlay_child (
                (ArrangerWidget *) self, r_obj);
            }
        }
    }
}

static void
add_children_from_chord_track (
  TimelineArrangerWidget * self,
  ChordTrack *             ct)
{
  int i, k;
  for (i = 0; i < ct->num_scales; i++)
    {
      ScaleObject * c = ct->scales[i];
      for (k = 0 ; k < 2; k++)
        {
          if (k == 0)
            c = scale_object_get_main (c);
          else if (k == 1)
            c = scale_object_get_main_trans (c);

          ArrangerObject * c_obj =
            (ArrangerObject *) c;

          if (!Z_IS_ARRANGER_OBJECT_WIDGET (
                c_obj->widget))
            {
              arranger_object_gen_widget (
                c_obj);
            }

          arranger_widget_add_overlay_child (
            (ArrangerWidget *) self, c_obj);
        }
    }

  Region * region;
  for (i = 0; i < ct->num_chord_regions; i++)
    {
      region = ct->chord_regions[i];

      for (k = 0 ; k < 2; k++)
        {
          if (k == 0)
            region =
              region_get_main (region);
          else if (k == 1)
            region =
              region_get_main_trans (region);

          ArrangerObject * r_obj =
            (ArrangerObject *) region;

          if (!Z_IS_ARRANGER_OBJECT_WIDGET (
                r_obj->widget))
            {
              arranger_object_gen_widget (
                r_obj);
            }

          arranger_widget_add_overlay_child (
            (ArrangerWidget *) self, r_obj);
        }
    }
}

static void
add_children_from_marker_track (
  TimelineArrangerWidget * self,
  Track *                  track)
{
  int i, k;
  for (i = 0; i < track->num_markers; i++)
    {
      Marker * m = track->markers[i];
      for (k = 0 ; k < 2; k++)
        {
          if (k == 0)
            m = marker_get_main (m);
          else if (k == 1)
            m = marker_get_main_trans (m);

          ArrangerObject * r_obj =
            (ArrangerObject *) m;

          if (!Z_IS_ARRANGER_OBJECT_WIDGET (
                r_obj->widget))
            {
              arranger_object_gen_widget (
                r_obj);
            }

          arranger_widget_add_overlay_child (
            (ArrangerWidget *) self, r_obj);
        }
    }
}

static inline void
add_children_from_midi_track (
  TimelineArrangerWidget * self,
  Track *                  it)
{
  g_return_if_fail (
    self && GTK_IS_OVERLAY (self) && it);

  TrackLane * lane;
  Region * r;
  int i, j, k;
  for (j = 0; j < it->num_lanes; j++)
    {
      lane = it->lanes[j];

      for (i = 0; i < lane->num_regions; i++)
        {
          r = lane->regions[i];

          for (k = 0 ; k < 4; k++)
            {
              if (k == 0)
                r = region_get_main (r);
              else if (k == 1)
                r =
                  region_get_main_trans (r);
              else if (k == 2)
                r = region_get_lane (r);
              else if (k == 3)
                r =
                  region_get_lane_trans (r);

              ArrangerObject * r_obj =
                (ArrangerObject *) r;

              if (!Z_IS_ARRANGER_OBJECT_WIDGET (
                    r_obj->widget))
                {
                  arranger_object_gen_widget (
                    r_obj);
                }

              arranger_widget_add_overlay_child (
                (ArrangerWidget *) self, r_obj);
            }
        }
    }
  add_children_from_channel_track (self, it);
}

static void
add_children_from_audio_track (
  TimelineArrangerWidget * self,
  AudioTrack *             at)
{
  add_children_from_midi_track (self, at);
}

static void
add_children_from_master_track (
  TimelineArrangerWidget * self,
  MasterTrack *            mt)
{
  ChannelTrack * ct = (ChannelTrack *) mt;
  add_children_from_channel_track (self, ct);
}

static void
add_children_from_audio_bus_track (
  TimelineArrangerWidget * self,
  AudioBusTrack *               mt)
{
  ChannelTrack * ct = (ChannelTrack *) mt;
  add_children_from_channel_track (self, ct);
}

/**
 * To be called when pinning/unpinning.
 */
void
timeline_arranger_widget_remove_children (
  TimelineArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* remove all children except bg && playhead */
  GList *children, *iter;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (self));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);
      if (widget != (GtkWidget *) ar_prv->bg &&
          widget != (GtkWidget *) ar_prv->playhead)
        {
          /*g_object_ref (widget);*/
          gtk_container_remove (
            GTK_CONTAINER (self),
            widget);
        }
    }
  g_list_free (children);
}

/**
 * Readd children.
 */
void
timeline_arranger_widget_refresh_children (
  TimelineArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  timeline_arranger_widget_remove_children (
    self);

  /* add tracks */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->visible &&
          track->pinned == self->is_pinned)
        {
          switch (track->type)
            {
            case TRACK_TYPE_CHORD:
              add_children_from_chord_track (
                self, track);
              break;
            case TRACK_TYPE_INSTRUMENT:
              add_children_from_midi_track (
                self, track);
              break;
            case TRACK_TYPE_MIDI:
              add_children_from_midi_track (
                self, track);
              break;
            case TRACK_TYPE_MASTER:
              add_children_from_master_track (
                self,
                (MasterTrack *) track);
              break;
            case TRACK_TYPE_AUDIO:
              add_children_from_audio_track (
                self,
                (AudioTrack *) track);
              break;
            case TRACK_TYPE_AUDIO_BUS:
              add_children_from_audio_bus_track (
                self,
                (AudioBusTrack *) track);
              break;
            case TRACK_TYPE_MARKER:
              add_children_from_marker_track (
                self, track);
              break;
            case TRACK_TYPE_AUDIO_GROUP:
              /* TODO */
              break;
            default:
              /* TODO */
              break;
            }
        }
    }

  arranger_widget_update_visibility (
    (ArrangerWidget *) self);

  gtk_overlay_reorder_overlay (
    GTK_OVERLAY (self),
    (GtkWidget *) ar_prv->playhead, -1);
}

/**
 * Scroll to the given position.
 */
void
timeline_arranger_widget_scroll_to (
  TimelineArrangerWidget * self,
  Position *               pos)
{
  /* TODO */

}

static gboolean
on_focus (GtkWidget       *widget,
          gpointer         user_data)
{
  /*g_message ("timeline focused");*/
  MAIN_WINDOW->last_focused = widget;

  return FALSE;
}

static void
timeline_arranger_widget_class_init (
  TimelineArrangerWidgetClass * klass)
{
}

static void
timeline_arranger_widget_init (
  TimelineArrangerWidget *self )
{
  g_signal_connect (
    self, "grab-focus",
    G_CALLBACK (on_focus), self);
}
