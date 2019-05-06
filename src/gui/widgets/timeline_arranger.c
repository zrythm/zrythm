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

#include "actions/undoable_action.h"
#include "actions/create_timeline_selections_action.h"
#include "actions/undo_manager.h"
#include "actions/duplicate_timeline_selections_action.h"
#include "actions/move_timeline_selections_action.h"
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
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"

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
      Track * track = P_CHORD_TRACK;

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
  TimelineArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (P_TOOL == TOOL_SELECT_NORMAL)
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
      else if (P_TOOL == TOOL_SELECT_STRETCH)
        {
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
    case UI_OVERLAY_ACTION_RESIZING_R:
      if (self->resizing_range)
        ac = ARRANGER_CURSOR_RANGE;
      else
        ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    default:
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;
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
  ChordTrack * ct =
    tracklist_get_chord_track (
      TRACKLIST);
  for (int i = 0; i < ct->num_chords; i++)
    {
      ZChord * chord = ct->chords[i];
      if (chord->visible)
        {
          chord_widget_select (
            chord->widget, select);
        }
    }

  /* select everything else */
  Region * r;
  Track * track;
  AutomationPoint * ap;
  AutomationLane * al;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (!track->visible)
        continue;

      AutomationTracklist *
        automation_tracklist =
          track_get_automation_tracklist (
            track);

      for (int j = 0;
           j < track->num_regions; j++)
        {
          r = track->regions[j];

          region_widget_select (
            r->widget, select, F_NO_TRANSIENTS);
        }

      if (!track->bot_paned_visible)
        continue;

      for (int j = 0;
           j < automation_tracklist->
             num_automation_lanes;
           j++)
        {
          al =
            automation_tracklist->
              automation_lanes[j];
          if (al->visible)
            {
              for (int k = 0;
                   k < al->at->
                     num_automation_points;
                   k++)
                {
                  ap =
                    al->at->automation_points[k];

                  automation_point_widget_select (
                    ap->widget, select,
                    F_NO_TRANSIENTS);
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
  TimelineArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
  GtkWidget *menu, *menuitem;

  /*RegionWidget * clicked_region =*/
    /*timeline_arranger_widget_get_hit_region (*/
      /*self, x, y);*/
  /*ChordWidget * clicked_chord =*/
    /*timeline_arranger_widget_get_hit_chord (*/
      /*self, x, y);*/
  /*AutomationPointWidget * clicked_ap =*/
    /*timeline_arranger_widget_get_hit_ap (*/
      /*self, x, y);*/
  /*AutomationCurveWidget * ac =*/
    /*timeline_arranger_widget_get_hit_curve (*/
      /*self, x, y);*/

  menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_with_label("Do something");

  /*g_signal_connect(menuitem, "activate",*/
                   /*(GCallback) view_popup_menu_onDoSomething, treeview);*/

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

void
timeline_arranger_widget_update_visibility (
  TimelineArrangerWidget * self)
{
  Region * r;
  Region * r_transient;
  ZChord * c;
  ZChord * c_transient;
  AutomationPoint * ap;
  AutomationPoint * ap_transient;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  ARRANGER_SET_SELECTION_VISIBILITY (
    TL_SELECTIONS->regions,
    TL_SELECTIONS->transient_regions,
    TL_SELECTIONS->num_regions,
    r,
    r_transient);
  ARRANGER_SET_SELECTION_VISIBILITY (
    TL_SELECTIONS->chords,
    TL_SELECTIONS->transient_chords,
    TL_SELECTIONS->num_chords,
    c,
    c_transient);
  ARRANGER_SET_SELECTION_VISIBILITY (
    TL_SELECTIONS->automation_points,
    TL_SELECTIONS->transient_aps,
    TL_SELECTIONS->num_automation_points,
    ap,
    ap_transient);
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
  /*Track * track = rw_prv->region->track;*/
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
    case TOOL_RAMP:
      /* TODO */
      break;
    }

  int selected = region_is_selected (region);

  /* select region if unselected */
  if (P_TOOL == TOOL_SELECT_NORMAL ||
      P_TOOL == TOOL_SELECT_STRETCH ||
      P_TOOL == TOOL_EDIT)
    {
      int transients =
        arranger_widget_is_in_moving_operation (
          Z_ARRANGER_WIDGET (self));

      /* if ctrl held & not selected, add to
       * selections */
      if (ar_prv->ctrl_held &&
          !selected)
        {
          ARRANGER_WIDGET_SELECT_REGION (
            self, region, F_SELECT, F_APPEND,
            transients);
        }
      /* if ctrl not held & not selected, make it
       * the only
       * selection */
      else if (!ar_prv->ctrl_held &&
               !selected)
        {
          ARRANGER_WIDGET_SELECT_REGION (
            self, region, F_SELECT, F_NO_APPEND,
            transients);
        }
      /* if selected, create transients */
      else if (selected)
        {
          timeline_selections_create_missing_transients (
            TL_SELECTIONS);
          g_warn_if_fail (
            TL_SELECTIONS->
              transient_regions[0] &&
            GTK_IS_WIDGET (
              TL_SELECTIONS->
                transient_regions[0]->widget));
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

  ZChord * chord = cw->chord;
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
        self, chord, 1, 1, 0);
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
        self, chord, 1, 0, 0);
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
        self, ap, 1, 1, 0);
    }
  else
    {
      timeline_arranger_widget_select_all (self, 0);
      ARRANGER_WIDGET_SELECT_AUTOMATION_POINT (
        self, ap, 1, 0, 0);
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
      ZChord * r = TL_SELECTIONS->chords[i];
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
  ar_prv->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;
  ARRANGER_WIDGET_SELECT_AUTOMATION_POINT (
    self, ap, F_SELECT,
    F_NO_APPEND, F_NO_TRANSIENTS);
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
  Region * region = NULL;
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      region =
        midi_region_new (
          track, pos, pos, 1);
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
  EVENTS_PUSH (ET_REGION_CREATED,
               region);
  ar_prv->action =
    UI_OVERLAY_ACTION_CREATING_RESIZING_R;
  ARRANGER_WIDGET_SELECT_REGION (
    self, region, F_SELECT,
    F_NO_APPEND, F_NO_TRANSIENTS);
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
 ZChord * chord = chord_new (NOTE_A,
                            1,
                            NOTE_A,
                            CHORD_TYPE_MIN,
                            0);
 position_set_to_pos (&chord->pos,
                      pos);
 /*ZChord * chords[1] = { chord };*/
 /*UndoableAction * action =*/
   /*create_chords_action_new (chords, 1);*/
 /*undo_manager_perform (UNDO_MANAGER,*/
                       /*action);*/
  ar_prv->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;
  ARRANGER_WIDGET_SELECT_CHORD (
    self, chord, F_SELECT,
    F_NO_APPEND, F_NO_TRANSIENTS);
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
  int i;
  Region * region;
  RegionWidget * rw;
  ZChord * chord;
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
      for (i = 0; i < num_region_widgets; i++)
        {
          rw =
            Z_REGION_WIDGET (region_widgets[i]);
          REGION_WIDGET_GET_PRIVATE (rw);

          region = rw_prv->region;

          track_remove_region (
            region->track, region);
          free_later (region, region_free);
      }
    }
  else
    {
      /* select the enclosed regions */
      for (i = 0; i < num_region_widgets; i++)
        {
          rw =
            Z_REGION_WIDGET (region_widgets[i]);
          REGION_WIDGET_GET_PRIVATE (rw);
          region = rw_prv->region;
          ARRANGER_WIDGET_SELECT_REGION (
            self,
            region,
            F_SELECT,
            F_APPEND, F_NO_TRANSIENTS);
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
      for (i = 0; i < num_chord_widgets; i++)
        {
          cw =
            Z_CHORD_WIDGET (chord_widgets[i]);

          chord = cw->chord;

          chord_track_remove_chord (
            P_CHORD_TRACK,
            chord);
      }
    }
  else
    {
      /* select the enclosed chords */
      for (i = 0; i < num_chord_widgets; i++)
        {
          cw =
            Z_CHORD_WIDGET (chord_widgets[i]);

          chord = cw->chord;

          ARRANGER_WIDGET_SELECT_CHORD (
            self, chord, F_SELECT, F_APPEND,
            F_NO_TRANSIENTS);
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
      for (i = 0;
           i < num_ap_widgets; i++)
        {
          apw =
            Z_AUTOMATION_POINT_WIDGET (
              ap_widgets[i]);

          ap = apw->ap;

          automation_track_remove_ap (
            ap->at,
            ap);
        }
    }
  else
    {
      /* select the enclosed automation points */
      for (i = 0;
           i < num_ap_widgets; i++)
        {
          apw =
            Z_AUTOMATION_POINT_WIDGET (
              ap_widgets[i]);

          ap = apw->ap;

          ARRANGER_WIDGET_SELECT_AUTOMATION_POINT (
            self, ap, F_SELECT, F_APPEND,
            F_NO_TRANSIENTS);
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
      region_set_start_pos (region, pos);
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

          /* if creating also set the loop points
           * appropriately */
          if (ar_prv->action ==
                UI_OVERLAY_ACTION_CREATING_RESIZING_R)
            {
              long full_size =
                region_get_full_length_in_ticks (
                  region);
              position_set_to_pos (
                &region->true_end_pos,
                &region->loop_start_pos);
              position_add_ticks (
                &region->true_end_pos,
                full_size);
              position_set_to_pos (
                &region->loop_end_pos,
                &region->true_end_pos);
            }
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
  Position tmp;
  long length_ticks;

  /* update region positions */
  /*Region * r;*/
  for (int i = 0; i <
       TL_SELECTIONS->num_regions; i++)
    {
      Region * r =
        TL_SELECTIONS->transient_regions[i];

      ARRANGER_MOVE_OBJ_BY_TICKS_W_LENGTH (
        r, region,
        &self->region_start_poses[i],
        ticks_diff, &tmp, length_ticks);
    }

  /* update chord positions */
  ZChord * c;
  for (int i = 0;
       i < TL_SELECTIONS->num_chords; i++)
    {
      c = TL_SELECTIONS->transient_chords[i];
      ARRANGER_MOVE_OBJ_BY_TICKS (
        c, chord,
        &self->chord_start_poses[i],
        ticks_diff, &tmp);
    }

  /* update ap positions */
  AutomationPoint * ap;
  for (int i = 0;
       i < TL_SELECTIONS->num_automation_points;
       i++)
    {
      ap =
        TL_SELECTIONS->transient_aps[i];

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
  if (!MAIN_WINDOW || !GTK_IS_WIDGET (MAIN_WINDOW))
    return G_SOURCE_CONTINUE;

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
              TRACKLIST, old_track);
          Track * nt =
            tracklist_get_next_visible_track (
              TRACKLIST, old_track);
          Track * tt =
            tracklist_get_first_visible_track (
              TRACKLIST);
          Track * bt =
            tracklist_get_last_visible_track (
              TRACKLIST);
          /* highest selected track */
          Track * hst =
            timeline_selections_get_highest_track (
              TL_SELECTIONS,
              arranger_widget_is_in_moving_operation (
                Z_ARRANGER_WIDGET (self)));
          /* lowest selected track */
          Track * lst =
            timeline_selections_get_lowest_track (
              TL_SELECTIONS,
              arranger_widget_is_in_moving_operation (
                Z_ARRANGER_WIDGET (self)));

          if (self->start_region->track != track)
            {
              /* if new track is lower and bot region is not at the lowest track */
              if (track == nt &&
                  lst != bt)
                {
                  /* shift all selected regions to their next track */
                  for (int i = 0; i < TL_SELECTIONS->num_regions; i++)
                    {
                      Region * region = TL_SELECTIONS->transient_regions[i];
                      nt =
                        tracklist_get_next_visible_track (
                          TRACKLIST,
                          region->track);
                      old_track = region->track;
                      if (old_track->type == nt->type)
                        {
                          if (nt->type ==
                                TRACK_TYPE_INSTRUMENT)
                            {
                              track_remove_region (
                                old_track,
                                region);
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
                       hst != tt)
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
                          TRACKLIST,
                          region->track);
                      old_track = region->track;
                      if (old_track->type == pt->type)
                        {
                          if (pt->type ==
                                TRACK_TYPE_INSTRUMENT)
                            {
                              track_remove_region (
                                old_track,
                                region);
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

          /*int this_y =*/
            /*automation_point_get_y_in_px (*/
              /*ap);*/
          /*int start_ap_y =*/
            /*automation_point_get_y_in_px (*/
              /*self->start_ap);*/
          /*int diff = this_y - start_ap_y;*/

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

  /*Region * region;*/
  /*ZChord * chord;*/
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

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      /* if something was clicked with ctrl without
       * moving*/
      if (ar_prv->ctrl_held)
        if (self->start_region_clone &&
            region_is_selected (
              self->start_region_clone))
          {
            /* deselect it */
            ARRANGER_WIDGET_SELECT_REGION (
              self, self->start_region,
              F_NO_SELECT, F_APPEND,
              F_NO_TRANSIENTS);
          }
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING)
    {
      /* FIXME only checks regions */
      UndoableAction * ua =
        (UndoableAction *)
        move_timeline_selections_action_new (
          TL_SELECTIONS,
          position_to_ticks (
            &TL_SELECTIONS->
              transient_regions[0]->start_pos) -
          position_to_ticks (
            &TL_SELECTIONS->
              regions[0]->start_pos),
          timeline_selections_get_highest_track (
            TL_SELECTIONS, F_TRANSIENTS) -
          timeline_selections_get_highest_track (
            TL_SELECTIONS, F_NO_TRANSIENTS));
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  /* if copy/link-moved */
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_COPY ||
           ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_LINK)
    {
      UndoableAction * ua =
        (UndoableAction *)
        duplicate_timeline_selections_action_new (
          TL_SELECTIONS,
          position_to_ticks (
            &TL_SELECTIONS->
              transient_regions[0]->start_pos) -
          position_to_ticks (
            &TL_SELECTIONS->
              regions[0]->start_pos),
          timeline_selections_get_highest_track (
            TL_SELECTIONS, F_TRANSIENTS) -
          timeline_selections_get_highest_track (
            TL_SELECTIONS, F_NO_TRANSIENTS));
      timeline_selections_clear (
        TL_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_NONE ||
           ar_prv->action ==
             UI_OVERLAY_ACTION_STARTING_SELECTION)
    {
      timeline_selections_clear (
        TL_SELECTIONS);
    }
  /* if something was created */
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_CREATING_MOVING ||
           ar_prv->action ==
             UI_OVERLAY_ACTION_CREATING_RESIZING_R)
    {
      UndoableAction * ua =
        (UndoableAction *)
        create_timeline_selections_action_new (
          TL_SELECTIONS);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  /* if didn't click on something */
  else
    {
      self->start_region = NULL;
      self->start_region_clone = NULL;
      self->start_ap = NULL;
    }
  timeline_selections_remove_transients (
    TL_SELECTIONS);
  ar_prv->action = UI_OVERLAY_ACTION_NONE;
  timeline_arranger_widget_update_visibility (
    self);

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
      ZChord * chord = ct->chords[i];
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
      Region * r = it->regions[i];
      if (!GTK_IS_WIDGET (r->widget))
        r->widget =
          Z_REGION_WIDGET (
            midi_region_widget_new (r));
      gtk_overlay_add_overlay (
        GTK_OVERLAY (self),
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
          /*g_object_ref (widget);*/
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
            case TRACK_TYPE_GROUP:
              /* TODO */
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
