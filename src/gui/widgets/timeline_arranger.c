/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * The timeline containing regions and other
 * objects.
 */

#include "zrythm.h"
#include "project.h"
#include "settings/settings.h"
#include "audio/automation_lane.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/audio_track.h"
#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/chord.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/master_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "actions/undoable_action.h"
#include "actions/create_chords_action.h"
#include "actions/undo_manager.h"
#include "utils/arrays.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

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
  if (Z_IS_REGION_WIDGET (widget))
    {
      RegionWidget * rw = Z_REGION_WIDGET (widget);
      REGION_WIDGET_GET_PRIVATE (rw);
      Track * track = rw_prv->region->track;
      /*TRACK_WIDGET_GET_PRIVATE (track->widget);*/

      if (!track->widget)
        track->widget = track_widget_new (track);

      gint wx, wy;
      gtk_widget_translate_coordinates(
        GTK_WIDGET (track->widget),
        GTK_WIDGET (self),
        0, 0,
        &wx, &wy);

      allocation->x =
        ui_pos_to_px_timeline (
          &rw_prv->region->start_pos,
          1);
      allocation->y = wy;
      allocation->width =
        (ui_pos_to_px_timeline (
          &rw_prv->region->end_pos,
          1) - allocation->x) - 1;

      allocation->height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget));
      if (track->bot_paned_visible)
        {
          allocation->height -=
            gtk_widget_get_allocated_height (
              track_widget_get_bottom_paned (track->widget)) + 1;
        }
    }
  else if (Z_IS_AUTOMATION_POINT_WIDGET (widget))
    {
      AutomationPointWidget * ap_widget =
        Z_AUTOMATION_POINT_WIDGET (widget);
      AutomationPoint * ap = ap_widget->ap;
      /*Automatable * a = ap->at->automatable;*/

      gint wx, wy;
      if (!ap->at->al ||
          !ap->at->track->bot_paned_visible)
        return;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (ap->at->al->widget),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);

      allocation->x =
        ui_pos_to_px_timeline (&ap->pos, 1) -
        AP_WIDGET_POINT_SIZE / 2;
      allocation->y =
        (wy + automation_point_get_y_in_px (ap)) -
        AP_WIDGET_POINT_SIZE / 2;
      allocation->width = AP_WIDGET_POINT_SIZE;
      allocation->height = AP_WIDGET_POINT_SIZE;
    }
  else if (Z_IS_AUTOMATION_CURVE_WIDGET (widget))
    {
      AutomationCurveWidget * acw =
        Z_AUTOMATION_CURVE_WIDGET (widget);
      AutomationCurve * ac = acw->ac;
      /*Automatable * a = ap->at->automatable;*/

      gint wx, wy;
      if (!ac->at->al ||
          !ac->at->track->bot_paned_visible)
        return;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (ac->at->al->widget),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);
      AutomationPoint * prev_ap =
        automation_track_get_ap_before_curve (ac->at, ac);
      AutomationPoint * next_ap =
        automation_track_get_ap_after_curve (ac->at, ac);

      allocation->x =
        ui_pos_to_px_timeline (
          &prev_ap->pos,
          1) - AC_Y_HALF_PADDING;
      int prev_y = automation_point_get_y_in_px (prev_ap);
      int next_y = automation_point_get_y_in_px (next_ap);
      allocation->y = (wy + (prev_y > next_y ? next_y : prev_y) - AC_Y_HALF_PADDING);
      allocation->width =
        (ui_pos_to_px_timeline (
          &next_ap->pos,
          1) - allocation->x) + AC_Y_HALF_PADDING;
      allocation->height = (prev_y > next_y ? prev_y - next_y : next_y - prev_y) + AC_Y_PADDING;
    }
  else if (Z_IS_CHORD_WIDGET (widget))
    {
      ChordWidget * cw = Z_CHORD_WIDGET (widget);
      Track * track = (Track *) CHORD_TRACK;

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (track->widget),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);

      allocation->x =
        ui_pos_to_px_timeline (
          &cw->chord->pos,
          1);
      Position tmp;
      position_set_to_pos (&tmp,
                           &cw->chord->pos);
      position_set_beat (
        &tmp,
        tmp.beats + 1);
      allocation->y = wy;
      allocation->width =
        (ui_pos_to_px_timeline (
          &tmp,
          1) - allocation->x) - 1;
      allocation->height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget));
    }
}

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
timeline_arranger_widget_get_cursor (
  UiOverlayAction action,
  Tool            tool)
{
  TimelineArrangerWidget * self =
    MW_TIMELINE;
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (tool == TOOL_SELECT_NORMAL &&
      action == UI_OVERLAY_ACTION_NONE)
    {
      RegionWidget * rw =
        timeline_arranger_widget_get_hit_region (
          self,
          ar_prv->hover_x,
          ar_prv->hover_y);

      REGION_WIDGET_GET_PRIVATE (rw);
      /* TODO chords, aps */

      int is_hit =
        rw != NULL;
      int is_resize_l =
        rw && rw_prv->resize_l;
      int is_resize_r =
        rw && rw_prv->resize_r;

      if (is_hit && is_resize_l)
        {
          return ARRANGER_CURSOR_RESIZING_L;
        }
      else if (is_hit && is_resize_r)
        {
          return ARRANGER_CURSOR_RESIZING_R;
        }
      else if (is_hit)
        {
          return ARRANGER_CURSOR_GRAB;
        }
      else
        {
          Track * track =
            timeline_arranger_widget_get_track_at_y (
              ar_prv->hover_y);

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
  return -1;
}


Track *
timeline_arranger_widget_get_track_at_y (double y)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      GtkAllocation allocation;
      gtk_widget_get_allocation (
        GTK_WIDGET (track->widget),
        &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
        GTK_WIDGET (MW_TIMELINE),
        GTK_WIDGET (track->widget),
        0,
        y,
        &wx,
        &wy);

      if (wy >= 0 && wy <= allocation.height)
        {
          return track;
        }
    }
  return NULL;
}

AutomationTrack *
timeline_arranger_widget_get_automation_track_at_y (double y)
{
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      AutomationTracklist * automation_tracklist =
        track_get_automation_tracklist (track);
      if (!automation_tracklist ||
          !track->bot_paned_visible)
        continue;

      for (int j = 0;
           j < automation_tracklist->
             num_automation_lanes;
           j++)
        {
          AutomationLane * al =
            automation_tracklist->automation_lanes[j];

          /* TODO check the rest */
          if (al->visible && al->widget)
            {
              GtkAllocation allocation;
              gtk_widget_get_allocation (
                GTK_WIDGET (al->widget),
                &allocation);

              gint wx, wy;
              gtk_widget_translate_coordinates(
                GTK_WIDGET (MW_TIMELINE),
                GTK_WIDGET (al->widget),
                0,
                y,
                &wx,
                &wy);

              if (wy >= 0 && wy <= allocation.height)
                {
                  return al->at;
                }
            }
        }
    }

  return NULL;
}

ChordWidget *
timeline_arranger_widget_get_hit_chord (
  TimelineArrangerWidget *  self,
  double                    x,
  double                    y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      CHORD_WIDGET_TYPE);
  if (widget)
    {
      return Z_CHORD_WIDGET (widget);
    }
  return NULL;
}

RegionWidget *
timeline_arranger_widget_get_hit_region (
  TimelineArrangerWidget *  self,
  double                    x,
  double                    y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      REGION_WIDGET_TYPE);
  if (widget)
    {
      return Z_REGION_WIDGET (widget);
    }
  return NULL;
}

AutomationPointWidget *
timeline_arranger_widget_get_hit_ap (
  TimelineArrangerWidget *  self,
  double            x,
  double            y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      AUTOMATION_POINT_WIDGET_TYPE);
  if (widget)
    {
      return Z_AUTOMATION_POINT_WIDGET (widget);
    }
  return NULL;
}

AutomationCurveWidget *
timeline_arranger_widget_get_hit_curve (
  TimelineArrangerWidget * self,
  double x,
  double y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      AUTOMATION_CURVE_WIDGET_TYPE);
  if (widget)
    {
      return Z_AUTOMATION_CURVE_WIDGET (widget);
    }
  return NULL;
}

void
timeline_arranger_widget_update_inspector (
  TimelineArrangerWidget *self)
{
  inspector_widget_show_selections (
    INSPECTOR_CHILD_REGION,
    (void **) TL_SELECTIONS->regions,
    TL_SELECTIONS->num_regions);
  inspector_widget_show_selections (
    INSPECTOR_CHILD_AP,
    (void **) TL_SELECTIONS->automation_points,
    TL_SELECTIONS->num_automation_points);
  inspector_widget_show_selections (
    INSPECTOR_CHILD_CHORD,
    (void **) TL_SELECTIONS->chords,
    TL_SELECTIONS->num_chords);
}

void
timeline_arranger_widget_select_all (
  TimelineArrangerWidget *  self,
  int                       select)
{
  TL_SELECTIONS->num_regions = 0;
  TL_SELECTIONS->num_automation_points = 0;

  /* select chords */
  ChordTrack * ct = tracklist_get_chord_track ();
  for (int i = 0; i < ct->num_chords; i++)
    {
      Chord * chord = ct->chords[i];
      if (chord->visible)
        {
          chord_widget_select (
            chord->widget, select);

          if (select)
            {
              /* select  */
              array_append (
                TL_SELECTIONS->chords,
                TL_SELECTIONS->num_chords,
                chord);
            }
          else
            {
              array_delete (
                TL_SELECTIONS->chords,
                TL_SELECTIONS->num_chords,
                chord);
            }
        }
    }

  /* select everything else */
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * chan = MIXER->channels[i];

      if (chan->visible)
        {
          int num_regions;
          InstrumentTrack * it;
          AudioTrack *      at;
          AutomationTracklist *
            automation_tracklist =
              track_get_automation_tracklist (
                chan->track);

          if (chan->track->type ==
                TRACK_TYPE_INSTRUMENT)
            {
              it = (InstrumentTrack *) chan->track;
              num_regions = it->num_regions;
            }
          else if (chan->track->type ==
                    TRACK_TYPE_AUDIO)
            {
              at = (AudioTrack *) chan->track;
              num_regions = at->num_regions;
            }
          else
            num_regions = 0;

          for (int j = 0; j < num_regions; j++)
            {
              Region * r;

              if (chan->track->type ==
                    TRACK_TYPE_INSTRUMENT)
                r = (Region *) it->regions[j];
              else if (chan->track->type ==
                         TRACK_TYPE_AUDIO)
                r = (Region *) at->regions[j];

              region_widget_select (
                r->widget, select);

              if (select)
                {
                  /* select  */
                  array_append (
                    TL_SELECTIONS->regions,
                    TL_SELECTIONS->num_regions,
                    r);
                }
            }
          if (chan->track->bot_paned_visible)
            {
              for (int j = 0;
                   j < automation_tracklist->
                     num_automation_lanes;
                   j++)
                {
                  AutomationLane * al =
                    automation_tracklist->
                      automation_lanes[j];
                  if (al->visible)
                    {
                      for (int k = 0;
                           k < al->at->
                             num_automation_points;
                           k++)
                        {
                          /*AutomationPoint * ap = at->automation_points[k];*/

                          /*ap_widget_select (ap->widget, select);*/

                          /*if (select)*/
                            /*{*/
                              /*[> select  <]*/
                              /*array_append ((void **)self->tl_automation_points,*/
                                            /*&self->num_tl_automation_points,*/
                                            /*ap);*/
                            /*}*/
                        }
                    }
                }
            }
        }
    }

  /**
   * Deselect range if deselecting all.
   */
  if (!select)
    {
      project_set_has_range (0);
    }
}

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
timeline_arranger_widget_show_context_menu (
  TimelineArrangerWidget * self)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_with_label("Do something");

  /*g_signal_connect(menuitem, "activate",*/
                   /*(GCallback) view_popup_menu_onDoSomething, treeview);*/

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

void
timeline_arranger_widget_on_drag_begin_region_hit (
  TimelineArrangerWidget * self,
  double                   start_x,
  RegionWidget *           rw)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  REGION_WIDGET_GET_PRIVATE (rw);

  /* open piano roll */
  Track * track = rw_prv->region->track;
  clip_editor_set_region (rw_prv->region);

  /* if double click bring up piano roll */
  if (ar_prv->n_press == 2 &&
      !ar_prv->ctrl_held)
    SHOW_CLIP_EDITOR;

  /* get x as local to the region */
  gint wx, wy;
  gtk_widget_translate_coordinates(
            GTK_WIDGET (self),
            GTK_WIDGET (rw),
            start_x,
            0,
            &wx,
            &wy);

  Region * region = rw_prv->region;
  self->start_region = rw_prv->region;
  self->start_region_clone =
    region_clone (rw_prv->region,
                  REGION_CLONE_COPY);

  /* update arranger action */
  switch (P_TOOL)
    {
    case TOOL_ERASER:
      ar_prv->action =
        UI_OVERLAY_ACTION_ERASING;
      break;
    case TOOL_AUDITION:
      ar_prv->action =
        UI_OVERLAY_ACTION_AUDITIONING;
      break;
    case TOOL_SELECT_NORMAL:
      if (region_widget_is_resize_l (rw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_L;
      else if (region_widget_is_resize_r (rw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_R;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_STARTING_MOVING;
      break;
    case TOOL_SELECT_STRETCH:
      if (region_widget_is_resize_l (rw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_STRETCHING_L;
      else if (region_widget_is_resize_r (rw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_STRETCHING_R;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_STARTING_MOVING;
      break;
    case TOOL_EDIT:
      if (region_widget_is_resize_l (rw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_L;
      else if (region_widget_is_resize_r (rw, wx))
        ar_prv->action =
          UI_OVERLAY_ACTION_RESIZING_R;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_STARTING_MOVING;
      break;
    }

  /* select region if unselected */
  if (P_TOOL == TOOL_SELECT_NORMAL ||
      P_TOOL == TOOL_SELECT_STRETCH ||
      P_TOOL == TOOL_EDIT)
    {
      /* if ctrl held & not selected, add to
       * selections */
      if (ar_prv->ctrl_held && !region->selected)
        {
          ARRANGER_WIDGET_SELECT_REGION (
            self, region, 1, 1);
        }
      /* if ctrl not held & not selected, make it
       * the only
       * selection */
      else if (!ar_prv->ctrl_held &&
               !array_contains (
                  (void **)TL_SELECTIONS->regions,
                  TL_SELECTIONS->num_regions,
                  region))
        {
          ARRANGER_WIDGET_SELECT_REGION (
            self, region, 1, 0);
        }
    }

  /* find highest and lowest selected regions */
  TL_SELECTIONS->top_region =
    TL_SELECTIONS->regions[0];
  TL_SELECTIONS->bot_region =
    TL_SELECTIONS->regions[0];
  for (int i = 0;
       i < TL_SELECTIONS->num_regions;
       i++)
    {
      Region * region = TL_SELECTIONS->regions[i];
      if (tracklist_get_track_pos (region->track) <
          tracklist_get_track_pos (TL_SELECTIONS->top_region->track))
        {
          TL_SELECTIONS->top_region = region;
        }
      if (tracklist_get_track_pos (region->track) >
          tracklist_get_track_pos (TL_SELECTIONS->bot_region->track))
        {
          TL_SELECTIONS->bot_region = region;
        }
    }
}

void
timeline_arranger_widget_on_drag_begin_chord_hit (
  TimelineArrangerWidget * self,
  double                   start_x,
  ChordWidget *            cw)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* if double click */
  if (ar_prv->n_press == 2)
    {
      /* TODO */
    }

  Chord * chord = cw->chord;
  self->start_chord = chord;

  /* update arranger action */
  ar_prv->action =
    UI_OVERLAY_ACTION_STARTING_MOVING;
  ui_set_cursor_from_name (GTK_WIDGET (cw), "grabbing");

  /* select/ deselect chords */
  if (ar_prv->ctrl_held)
    {
      /* if ctrl pressed toggle on/off */
      ARRANGER_WIDGET_SELECT_CHORD (
        self, chord, 1, 1);
    }
  else if (!array_contains (
             (void **)TL_SELECTIONS->chords,
             TL_SELECTIONS->num_chords,
             chord))
    {
      /* else if not already selected select only it */
      timeline_arranger_widget_select_all (
        self, 0);
      ARRANGER_WIDGET_SELECT_CHORD (
        self, chord, 1, 0);
    }
}

void
timeline_arranger_widget_on_drag_begin_ap_hit (
  TimelineArrangerWidget * self,
  double                   start_x,
  AutomationPointWidget *  ap_widget)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  AutomationPoint * ap = ap_widget->ap;
  ar_prv->start_pos_px = start_x;
  self->start_ap = ap;
  if (!array_contains (
        TL_SELECTIONS->automation_points,
        TL_SELECTIONS->num_automation_points,
        ap))
    {
      TL_SELECTIONS->automation_points[0] =
        ap;
      TL_SELECTIONS->num_automation_points =
        1;
    }

  /* update arranger action */
  ar_prv->action =
    UI_OVERLAY_ACTION_STARTING_MOVING;
  ui_set_cursor_from_name (GTK_WIDGET (ap_widget),
                 "grabbing");

  /* update selection */
  if (ar_prv->ctrl_held)
    {
      ARRANGER_WIDGET_SELECT_AUTOMATION_POINT (
        self, ap, 1, 1);
    }
  else
    {
      timeline_arranger_widget_select_all (self, 0);
      ARRANGER_WIDGET_SELECT_AUTOMATION_POINT (
        self, ap, 1, 0);
    }
}

void
timeline_arranger_widget_find_start_poses (
  TimelineArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0; i < TL_SELECTIONS->num_regions; i++)
    {
      Region * r = TL_SELECTIONS->regions[i];
      if (position_compare (&r->start_pos,
                            &ar_prv->start_pos) <= 0)
        {
          position_set_to_pos (&ar_prv->start_pos,
                               &r->start_pos);
        }

      /* set start poses for regions */
      position_set_to_pos (&self->region_start_poses[i],
                           &r->start_pos);
    }
  for (int i = 0; i < TL_SELECTIONS->num_chords; i++)
    {
      Chord * r = TL_SELECTIONS->chords[i];
      if (position_compare (&r->pos,
                            &ar_prv->start_pos) <= 0)
        {
          position_set_to_pos (&ar_prv->start_pos,
                               &r->pos);
        }

      /* set start poses for chords */
      position_set_to_pos (&self->chord_start_poses[i],
                           &r->pos);
    }
  for (int i = 0; i < TL_SELECTIONS->num_automation_points; i++)
    {
      AutomationPoint * ap = TL_SELECTIONS->automation_points[i];
      if (position_compare (&ap->pos,
                            &ar_prv->start_pos) <= 0)
        {
          position_set_to_pos (&ar_prv->start_pos,
                               &ap->pos);
        }

      /* set start poses for APs */
      position_set_to_pos (&self->ap_poses[i],
                           &ap->pos);
    }
}

void
timeline_arranger_widget_create_ap (
  TimelineArrangerWidget * self,
  AutomationTrack *        at,
  Track *                  track,
  Position *               pos,
  double                   start_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
      !ar_prv->shift_held)
    position_snap (NULL,
                   pos,
                   track,
                   NULL,
                   ar_prv->snap_grid);

  /*g_message ("at here: %s",*/
             /*at->automatable->label);*/

  /* if the automatable is float in this automation track */
  if (automatable_is_float (at->automatable))
    {
      /* add automation point to automation track */
      float value =
        automation_lane_widget_get_fvalue_at_y (
          at->al->widget,
          start_y);

      AutomationPoint * ap =
        automation_point_new_float (
          at,
          value,
          pos);
      automation_track_add_automation_point (
        at,
        ap,
        GENERATE_CURVE_POINTS);
      gtk_overlay_add_overlay (
        GTK_OVERLAY (self),
        GTK_WIDGET (ap->widget));
      gtk_widget_show (GTK_WIDGET (ap->widget));
      TL_SELECTIONS->automation_points[0] =
        ap;
      TL_SELECTIONS->num_automation_points =
        1;
      automation_point_widget_select (
        ap->widget, 1);
    }
}

void
timeline_arranger_widget_create_region (
  TimelineArrangerWidget * self,
  Track *                  track,
  Position *               pos)
{
  if (track->type == TRACK_TYPE_AUDIO)
    return;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
      !ar_prv->shift_held)
    {
      position_snap (NULL,
                     pos,
                     track,
                     NULL,
                     ar_prv->snap_grid);
    }
  Region * region;
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      region = (Region *) midi_region_new (track,
                               pos,
                               pos);
    }
  position_set_min_size (&region->start_pos,
                         &region->end_pos,
                         ar_prv->snap_grid);
  region_set_end_pos (region,
                      &region->end_pos);
  long length =
    region_get_full_length_in_ticks (region);
  position_from_ticks (&region->true_end_pos,
                       length);
  region_set_true_end_pos (region,
                           &region->true_end_pos);
  Position tmp;
  position_init (&tmp);
  region_set_clip_start_pos (region,
                             &tmp);
  region_set_loop_start_pos (region,
                             &tmp);
  region_set_loop_end_pos (region,
                           &region->true_end_pos);
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      track_add_region ((InstrumentTrack *)track,
                        (MidiRegion *) region);
    }
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (region->widget));
  gtk_widget_show (GTK_WIDGET (region->widget));
  ar_prv->action = UI_OVERLAY_ACTION_RESIZING_R;
  /*g_message ("created region and resizing");*/
  TL_SELECTIONS->regions[0] = region;
  TL_SELECTIONS->num_regions = 1;
  region_widget_select (region->widget, 1);
}

void
timeline_arranger_widget_create_chord (
  TimelineArrangerWidget * self,
  Track *                  track,
  Position *               pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
      !ar_prv->shift_held)
    position_snap (NULL,
                   pos,
                   track,
                   NULL,
                   ar_prv->snap_grid);
 Chord * chord = chord_new (NOTE_A,
                            1,
                            NOTE_A,
                            CHORD_TYPE_MIN,
                            0);
 position_set_to_pos (&chord->pos,
                      pos);
 Chord * chords[1] = { chord };
 UndoableAction * action =
   create_chords_action_new (chords, 1);
 undo_manager_perform (UNDO_MANAGER,
                       action);
  ar_prv->action = UI_OVERLAY_ACTION_NONE;
  TL_SELECTIONS->chords[0] = chord;
  TL_SELECTIONS->num_chords = 1;
  chord_widget_select (chord->widget, 1);
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
    timeline_arranger_widget_get_track_at_y (y);

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
  Region * region;
  RegionWidget * rw;
  Chord * chord;
  ChordWidget * cw;
  AutomationPoint * ap;
  AutomationPointWidget * apw;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (!delete)
    /* deselect all */
    arranger_widget_select_all (
      Z_ARRANGER_WIDGET (self), 0);

  /* find enclosed regions */
  GtkWidget *    region_widgets[800];
  int            num_region_widgets = 0;
  arranger_widget_get_hit_widgets_in_range (
    Z_ARRANGER_WIDGET (self),
    REGION_WIDGET_TYPE,
    ar_prv->start_x,
    ar_prv->start_y,
    offset_x,
    offset_y,
    region_widgets,
    &num_region_widgets);


  if (delete)
    {
      /* delete the enclosed regions */
      for (int i = 0; i < num_region_widgets; i++)
        {
          rw =
            Z_REGION_WIDGET (region_widgets[i]);
          REGION_WIDGET_GET_PRIVATE (rw);

          region = rw_prv->region;

          /* TODO delete region */
          g_message ("delete region");
      }
    }
  else
    {
      /* select the enclosed regions */
      for (int i = 0; i < num_region_widgets; i++)
        {
          rw =
            Z_REGION_WIDGET (region_widgets[i]);
          REGION_WIDGET_GET_PRIVATE (rw);
          region = rw_prv->region;
          ARRANGER_WIDGET_SELECT_REGION (
            self,
            region,
            1,
            1);
        }
    }

  /* find enclosed chords */
  GtkWidget *    chord_widgets[800];
  int            num_chord_widgets = 0;
  arranger_widget_get_hit_widgets_in_range (
    Z_ARRANGER_WIDGET (self),
    CHORD_WIDGET_TYPE,
    ar_prv->start_x,
    ar_prv->start_y,
    offset_x,
    offset_y,
    chord_widgets,
    &num_chord_widgets);

  if (delete)
    {
      /* delete the enclosed chords */
      for (int i = 0; i < num_chord_widgets; i++)
        {
          cw =
            Z_CHORD_WIDGET (chord_widgets[i]);

          chord = cw->chord;

          /* TODO delete chord */
          g_message ("delete chord");
      }
    }
  else
    {
      /* select the enclosed chords */
      for (int i = 0; i < num_chord_widgets; i++)
        {
          cw =
            Z_CHORD_WIDGET (chord_widgets[i]);

          chord = cw->chord;

          ARRANGER_WIDGET_SELECT_CHORD (
            self, chord, 1, 1);
        }
    }

  /* find enclosed automation_points */
  GtkWidget *    ap_widgets[800];
  int            num_ap_widgets = 0;
  arranger_widget_get_hit_widgets_in_range (
    Z_ARRANGER_WIDGET (self),
    AUTOMATION_POINT_WIDGET_TYPE,
    ar_prv->start_x,
    ar_prv->start_y,
    offset_x,
    offset_y,
    ap_widgets,
    &num_ap_widgets);

  if (delete)
    {
      /* delete the enclosed automation points */
      for (int i = 0;
           i < num_ap_widgets; i++)
        {
          apw =
            Z_AUTOMATION_POINT_WIDGET (
              ap_widgets[i]);

          ap = apw->ap;

          /* TODO delete automation point */
          g_message ("delete automation point");
        }
    }
  else
    {
      /* select the enclosed automation points */
      for (int i = 0;
           i < num_ap_widgets; i++)
        {
          apw =
            Z_AUTOMATION_POINT_WIDGET (
              ap_widgets[i]);

          ap = apw->ap;

          ARRANGER_WIDGET_SELECT_AUTOMATION_POINT (
            self, ap, 1, 1);
        }
    }
}

void
timeline_arranger_widget_snap_regions_l (
  TimelineArrangerWidget * self,
  Position *               pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0;
       i < TL_SELECTIONS->num_regions;
       i++)
    {
      Region * region =
        TL_SELECTIONS->regions[i];
      if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
            !ar_prv->shift_held)
        position_snap (NULL,
                       pos,
                       region->track,
                       NULL,
                       ar_prv->snap_grid);
      region_set_start_pos (region,
                            pos,
                            0);
    }
}

void
timeline_arranger_widget_snap_regions_r (
  TimelineArrangerWidget * self,
  Position *               pos)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  for (int i = 0;
       i < TL_SELECTIONS->num_regions;
       i++)
    {
      Region * region =
        TL_SELECTIONS->regions[i];
      if (SNAP_GRID_ANY_SNAP (ar_prv->snap_grid) &&
            !ar_prv->shift_held)
        position_snap (NULL,
                       pos,
                       region->track,
                       NULL,
                       ar_prv->snap_grid);
      if (position_compare (
            pos, &region->start_pos) > 0)
        {
          region_set_end_pos (region,
                              pos);
        }
    }
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
    &PROJECT->range_2,
    pos);
  project_set_has_range (1);

  EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED,
               NULL);

  arranger_widget_refresh_all_backgrounds ();
}

void
timeline_arranger_widget_move_items_x (
  TimelineArrangerWidget * self,
  long                     ticks_diff)
{
  /* update region positions */
  for (int i = 0; i <
       TL_SELECTIONS->num_regions; i++)
    {
      Region * r = TL_SELECTIONS->regions[i];
      Position * prev_start_pos = &self->region_start_poses[i];
      long length_ticks = position_to_ticks (&r->end_pos) -
        position_to_ticks (&r->start_pos);
      Position tmp;
      position_set_to_pos (&tmp, prev_start_pos);
      position_add_ticks (&tmp, ticks_diff + length_ticks);
      region_set_end_pos (r, &tmp);
      position_set_to_pos (&tmp, prev_start_pos);
      position_add_ticks (&tmp, ticks_diff);
      region_set_start_pos (r, &tmp, 1);
    }

  /* update chord positions */
  for (int i = 0;
       i < TL_SELECTIONS->num_chords; i++)
    {
      Chord * r = TL_SELECTIONS->chords[i];
      Position * prev_start_pos =
        &self->chord_start_poses[i];
      Position tmp;
      position_set_to_pos (&tmp, prev_start_pos);
      position_add_ticks (&tmp, ticks_diff);
      chord_set_pos (r, &tmp);
    }

  /* update ap positions */
  for (int i = 0; i < TL_SELECTIONS->num_automation_points; i++)
    {
      AutomationPoint * ap =
        TL_SELECTIONS->automation_points[i];

      /* get prev and next value APs */
      AutomationPoint * prev_ap =
        automation_track_get_prev_ap (ap->at, ap);
      AutomationPoint * next_ap =
        automation_track_get_next_ap (ap->at, ap);

      /* get adjusted pos for this automation point */
      Position ap_pos;
      Position * prev_pos = &self->ap_poses[i];
      position_set_to_pos (&ap_pos,
                           prev_pos);
      position_add_ticks (&ap_pos, ticks_diff);

      Position mid_pos;
      AutomationCurve * ac;

      /* update midway points */
      if (prev_ap && position_compare (&ap_pos, &prev_ap->pos) >= 0)
        {
          /* set prev curve point to new midway pos */
          position_get_midway_pos (&prev_ap->pos,
                                   &ap_pos,
                                   &mid_pos);
          ac = automation_track_get_next_curve_ac (ap->at,
                                                   prev_ap);
          position_set_to_pos (&ac->pos, &mid_pos);

          /* set pos for ap */
          if (!next_ap)
            {
              position_set_to_pos (&ap->pos, &ap_pos);
            }
        }
      if (next_ap && position_compare (&ap_pos, &next_ap->pos) <= 0)
        {
          /* set next curve point to new midway pos */
          position_get_midway_pos (&ap_pos,
                                   &next_ap->pos,
                                   &mid_pos);
          ac = automation_track_get_next_curve_ac (ap->at,
                                                   ap);
          position_set_to_pos (&ac->pos, &mid_pos);

          /* set pos for ap - if no prev ap exists or if the position
           * is also after the prev ap */
          if ((prev_ap &&
                position_compare (&ap_pos, &prev_ap->pos) >= 0) ||
              (!prev_ap))
            {
              position_set_to_pos (&ap->pos, &ap_pos);
            }
        }
      else if (!prev_ap && !next_ap)
        {
          /* set pos for ap */
          position_set_to_pos (&ap->pos, &ap_pos);
        }
    }
}

/**
 * Sets width to ruler width and height to
 * tracklist height.
 */
void
timeline_arranger_widget_set_size ()
{
  // set the size
  int ww, hh;
  TracklistWidget * tracklist = MW_TRACKLIST;
  gtk_widget_get_size_request (
    GTK_WIDGET (tracklist),
    &ww,
    &hh);
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (MW_TIMELINE),
    rw_prv->total_px,
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
  if (!GTK_IS_WIDGET (ZRYTHM))
    return FALSE;

  TimelineArrangerWidget * self = MW_TIMELINE;

  int prev = self->last_timeline_obj_bars;
  self->last_timeline_obj_bars = 0;

  COMPARE_AND_SET (&TRANSPORT->end_marker_pos);

  Track * track;
  Region * region;
  AutomationPoint * ap;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      region =
        track_get_last_region (track);
      ap =
        track_get_last_automation_point (track);

      if (region)
        COMPARE_AND_SET (&region->end_pos);

      if (ap)
        COMPARE_AND_SET (&ap->pos);
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
timeline_arranger_widget_setup ()
{
  timeline_arranger_widget_set_size ();

  g_timeout_add (
    1000,
    update_last_timeline_object,
    NULL);
}

void
timeline_arranger_widget_move_items_y (
  TimelineArrangerWidget * self,
  double                   offset_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (self->start_region)
    {
      /* check if should be moved to new track */
      Track * track = timeline_arranger_widget_get_track_at_y (ar_prv->start_y + offset_y);
      Track * old_track = self->start_region->track;
      if (track)
        {
          Track * pt =
            tracklist_get_prev_visible_track (
              old_track);
          Track * nt =
            tracklist_get_next_visible_track (
              old_track);
          Track * tt =
            tracklist_get_first_visible_track ();
          Track * bt =
            tracklist_get_last_visible_track ();
          if (self->start_region->track != track)
            {
              /* if new track is lower and bot region is not at the lowest track */
              if (track == nt &&
                  TL_SELECTIONS->bot_region->track != bt)
                {
                  /* shift all selected regions to their next track */
                  for (int i = 0; i < TL_SELECTIONS->num_regions; i++)
                    {
                      Region * region = TL_SELECTIONS->regions[i];
                      nt =
                        tracklist_get_next_visible_track (
                          region->track);
                      old_track = region->track;
                      if (old_track->type == nt->type)
                        {
                          if (nt->type ==
                                TRACK_TYPE_INSTRUMENT)
                            {
                              track_remove_region (
                                old_track,
                                region, 0);
                              track_add_region (
                                nt, region);
                            }
                          else if (nt->type ==
                                     TRACK_TYPE_AUDIO)
                            {
                              /* TODO */

                            }
                        }
                    }
                }
              else if (track == pt &&
                       TL_SELECTIONS->top_region->track != tt)
                {
                  /*g_message ("track %s top region track %s tt %s",*/
                             /*track->channel->name,*/
                             /*self->top_region->track->channel->name,*/
                             /*tt->channel->name);*/
                  /* shift all selected regions to their prev track */
                  for (int i = 0; i < TL_SELECTIONS->num_regions; i++)
                    {
                      Region * region = TL_SELECTIONS->regions[i];
                      pt =
                        tracklist_get_prev_visible_track (
                          region->track);
                      old_track = region->track;
                      if (old_track->type == pt->type)
                        {
                          if (pt->type ==
                                TRACK_TYPE_INSTRUMENT)
                            {
                              track_remove_region (
                                old_track,
                                region, 0);
                              track_add_region (
                                pt, region);
                            }
                          else if (pt->type ==
                                     TRACK_TYPE_AUDIO)
                            {
                              /* TODO */

                            }
                        }
                    }
                }
            }
        }
    }
  else if (self->start_ap)
    {
      for (int i = 0; i < TL_SELECTIONS->num_automation_points; i++)
        {
          AutomationPoint * ap =
            TL_SELECTIONS->
              automation_points[i];

          /* get adjusted y for this ap */
          /*Position region_pos;*/
          /*position_set_to_pos (&region_pos,*/
                               /*&pos);*/
          /*int diff = position_to_frames (&region->start_pos) -*/
            /*position_to_frames (&self->tl_start_region->start_pos);*/
          /*position_add_frames (&region_pos, diff);*/

          int this_y =
            automation_point_get_y_in_px (
              ap);
          int start_ap_y =
            automation_point_get_y_in_px (
              self->start_ap);
          int diff = this_y - start_ap_y;

          float fval =
            automation_lane_widget_get_fvalue_at_y (
              ap->at->al->widget,
              ar_prv->start_y + offset_y);
          automation_point_update_fvalue (ap, fval);
        }
      automation_point_widget_update_tooltip (
        self->start_ap->widget, 1);
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

  Region * region;
  Chord * chord;
  AutomationPoint * ap;
  for (int i = 0;
       i < TL_SELECTIONS->
             num_automation_points; i++)
    {
      ap =
        TL_SELECTIONS->automation_points[i];
      automation_point_widget_update_tooltip (
        ap->widget, 0);
    }

  /* if something was clicked with ctrl without
   * moving*/
  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      /* deselect it */
      if (ar_prv->ctrl_held)
        if (self->start_region_clone &&
            self->start_region_clone->selected)
          {
            ARRANGER_WIDGET_SELECT_REGION (
              self, self->start_region,
              0, 1);
          }
    }
  /* if didn't click on something */
  else
    {
      self->start_region = NULL;
      self->start_region_clone = NULL;
      self->start_ap = NULL;
    }

  self->resizing_range = 0;
  self->resizing_range_start = 0;
}

static void
add_children_from_channel_track (
  ChannelTrack * ct)
{
  if (!((Track *)ct)->bot_paned_visible)
    return;

  AutomationTracklist * atl =
    &ct->automation_tracklist;
  for (int i = 0;
       i < atl->num_automation_lanes; i++)
    {
      AutomationLane * al =
        atl->automation_lanes[i];
      if (al->visible)
        {
          for (int j = 0;
               j < al->at->num_automation_points;
               j++)
            {
              AutomationPoint * ap =
                al->at->automation_points[j];
              if (ap->widget)
                {
                  gtk_overlay_add_overlay (
                    GTK_OVERLAY (MW_TIMELINE),
                    GTK_WIDGET (ap->widget));
                }
            }
          for (int j = 0;
               j < al->at->num_automation_curves;
               j++)
            {
              AutomationCurve * ac =
                al->at->automation_curves[j];
              if (ac->widget)
                {
                  gtk_overlay_add_overlay (
                    GTK_OVERLAY (MW_TIMELINE),
                    GTK_WIDGET (ac->widget));
                }
            }
        }
    }
}

static void
add_children_from_chord_track (
  TimelineArrangerWidget * self,
  ChordTrack *             ct)
{
  for (int i = 0; i < ct->num_chords; i++)
    {
      Chord * chord = ct->chords[i];
      gtk_overlay_add_overlay (
        GTK_OVERLAY (self),
        GTK_WIDGET (chord->widget));
    }
}

static void
add_children_from_instrument_track (
  TimelineArrangerWidget * self,
  InstrumentTrack *        it)
{
  for (int i = 0; i < it->num_regions; i++)
    {
      MidiRegion * mr = it->regions[i];
      Region * r = (Region *) mr;
      gtk_overlay_add_overlay (GTK_OVERLAY (self),
                               GTK_WIDGET (r->widget));
    }
  ChannelTrack * ct = (ChannelTrack *) it;
  add_children_from_channel_track (ct);
}

static void
add_children_from_audio_track (
  TimelineArrangerWidget * self,
  AudioTrack *             audio_track)
{
  /*g_message ("adding children");*/
  for (int i = 0; i < audio_track->num_regions; i++)
    {
      AudioRegion * mr = audio_track->regions[i];
      /*g_message ("adding region");*/
      Region * r = (Region *) mr;
      gtk_overlay_add_overlay (
        GTK_OVERLAY (self),
        GTK_WIDGET (r->widget));
    }
  ChannelTrack * ct = (ChannelTrack *) audio_track;
  add_children_from_channel_track (ct);
}

static void
add_children_from_master_track (
  TimelineArrangerWidget * self,
  MasterTrack *            mt)
{
  ChannelTrack * ct = (ChannelTrack *) mt;
  add_children_from_channel_track (ct);
}

static void
add_children_from_bus_track (
  TimelineArrangerWidget * self,
  BusTrack *               mt)
{
  ChannelTrack * ct = (ChannelTrack *) mt;
  add_children_from_channel_track (ct);
}

/**
 * Readd children.
 */
void
timeline_arranger_widget_refresh_children (
  TimelineArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* remove all children except bg && playhead */
  GList *children, *iter;

  children =
    gtk_container_get_children (GTK_CONTAINER (self));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);
      if (widget != (GtkWidget *) ar_prv->bg &&
          widget != (GtkWidget *) ar_prv->playhead)
        {
          g_object_ref (widget);
          gtk_container_remove (
            GTK_CONTAINER (self),
            widget);
        }
    }
  g_list_free (children);

  /* add tracks */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (track->visible)
        {
          switch (track->type)
            {
            case TRACK_TYPE_CHORD:
              add_children_from_chord_track (
                self,
                (ChordTrack *) track);
              break;
            case TRACK_TYPE_INSTRUMENT:
              add_children_from_instrument_track (
                self,
                (InstrumentTrack *) track);
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
            case TRACK_TYPE_BUS:
              add_children_from_bus_track (
                self,
                (BusTrack *) track);
              break;
            }
        }
    }
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
timeline_arranger_widget_class_init (TimelineArrangerWidgetClass * klass)
{
}

static void
timeline_arranger_widget_init (TimelineArrangerWidget *self )
{
  g_signal_connect (
    self, "grab-focus",
    G_CALLBACK (on_focus), self);
}
