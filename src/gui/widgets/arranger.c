/*
 * gui/widgets/arranger.c - The timeline containing regions
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "zrythm_app.h"
#include "project.h"
#include "settings_manager.h"
#include "gui/widgets/region.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_editor.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ArrangerWidget, arranger_widget, GTK_TYPE_OVERLAY)

#define MW_RULER MAIN_WINDOW->ruler
#define T_MIDI self->type == ARRANGER_TYPE_MIDI
#define T_TIMELINE self->type == ARRANGER_TYPE_TIMELINE

/**
 * Sets up the MIDI editor for the given region.
 */
void
arranger_widget_set_channel (ArrangerWidget * arranger, Channel * channel)
{
  if (arranger->type == ARRANGER_TYPE_MIDI)
    {
      /*if (SELECTIONS.num_channels > 0 &&*/
          /*SELECTIONS.channels[SELECTIONS.num_channels - 1] == channel)*/

      selections_set_channel (channel);

      gtk_notebook_set_current_page (MAIN_WINDOW->bot_notebook, 0);

      char * label = g_strdup_printf ("%s",
                                      channel->name);
      gtk_label_set_text (MIDI_EDITOR->midi_name_label,
                          label);
      g_free (label);

      color_area_widget_set_color (MIDI_EDITOR->midi_track_color,
                                   &channel->color);

      /* remove all previous children and add new */
      GList *children, *iter;
      children = gtk_container_get_children(GTK_CONTAINER(MIDI_EDITOR->midi_arranger));
      for(iter = children; iter != NULL; iter = g_list_next(iter))
        {
          if (iter->data !=  MIDI_EDITOR->midi_arranger->bg)
            gtk_container_remove (GTK_CONTAINER (MIDI_EDITOR->midi_arranger),
                                  GTK_WIDGET (iter->data));
        }
      g_list_free(children);
      for (int i = 0; i < channel->track->num_regions; i++)
        {
          Region * region = channel->track->regions[i];
          for (int j = 0; j < region->num_midi_notes; j++)
            {
              gtk_overlay_add_overlay (GTK_OVERLAY (MIDI_EDITOR->midi_arranger),
                                       GTK_WIDGET (region->midi_notes[j]->widget));
            }
        }
      gtk_widget_queue_allocate (GTK_WIDGET (MIDI_EDITOR->midi_arranger));
      gtk_widget_show_all (GTK_WIDGET (MIDI_EDITOR->midi_arranger));

      GtkAdjustment * adj = gtk_scrolled_window_get_vadjustment (
                                      MIDI_EDITOR->piano_roll_arranger_scroll);
      gtk_adjustment_set_value (adj,
                                gtk_adjustment_get_upper (adj) / 2);
      gtk_scrolled_window_set_vadjustment (MIDI_EDITOR->piano_roll_arranger_scroll,
                                           adj);
    }
}

/**
 * Gets x position in pixels
 */
int
arranger_get_x_pos_in_px (Position * pos)
{
  return (pos->bars - 1) * MW_RULER->px_per_bar +
  (pos->beats - 1) * MW_RULER->px_per_beat +
  (pos->quarter_beats - 1) * MW_RULER->px_per_quarter_beat +
  pos->ticks * MW_RULER->px_per_tick +
  SPACE_BEFORE_START;
}

/**
 * Gets called to set the position/size of each overlayed widget.
 */
static gboolean
get_child_position (GtkOverlay   *overlay,
                    GtkWidget    *widget,
                    GdkRectangle *allocation,
                    gpointer      user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) overlay;

  if (T_MIDI)
    {
      if (IS_MIDI_NOTE_WIDGET (widget))
        {
          MidiNoteWidget * midi_note_widget = MIDI_NOTE_WIDGET (widget);

          allocation->x = arranger_get_x_pos_in_px (&midi_note_widget->midi_note->start_pos);
          allocation->y = MAIN_WINDOW->midi_editor->piano_roll_labels->px_per_note *
            (127 - midi_note_widget->midi_note->val);
          allocation->width =
            arranger_get_x_pos_in_px (&midi_note_widget->midi_note->end_pos) -
            allocation->x;
          allocation->height = MAIN_WINDOW->midi_editor->piano_roll_labels->px_per_note;
        }
    }
  else if (T_TIMELINE)
    {
      if (IS_REGION_WIDGET (widget))
        {
          RegionWidget * rw = REGION_WIDGET (widget);

          gint wx, wy;
          gtk_widget_translate_coordinates(
                    GTK_WIDGET (rw->region->track->widget->track_box),
                    GTK_WIDGET (overlay),
                    0,
                    0,
                    &wx,
                    &wy);

          allocation->x = arranger_get_x_pos_in_px (&rw->region->start_pos);
          allocation->y = wy;
          allocation->width = arranger_get_x_pos_in_px (&rw->region->end_pos) -
            allocation->x;
          allocation->height =
            gtk_widget_get_allocated_height (
              GTK_WIDGET (rw->region->track->widget->track_box));
        }
      else if (IS_AUTOMATION_POINT_WIDGET (widget))
        {
          AutomationPointWidget * ap_widget = AUTOMATION_POINT_WIDGET (widget);
          AutomationPoint * ap = ap_widget->ap;
          Automatable * a = ap->at->automatable;

          gint wx, wy;
          gtk_widget_translate_coordinates(
                    GTK_WIDGET (ap->at->widget->at_grid),
                    GTK_WIDGET (overlay),
                    0,
                    0,
                    &wx,
                    &wy);

          if (ap->type == AUTOMATION_POINT_VALUE)
            {
              allocation->x = arranger_get_x_pos_in_px (&ap->pos) -
                AP_WIDGET_POINT_SIZE / 2;
              allocation->y = (wy + automation_point_get_y_in_px (ap)) -
                AP_WIDGET_POINT_SIZE / 2;
              allocation->width = AP_WIDGET_POINT_SIZE;
              allocation->height = AP_WIDGET_POINT_SIZE;
            }
          else
            {
              /*allocation->x = arranger_get_x_pos_in_px (&ap->pos) -*/
                /*AP_WIDGET_CURVE_W / 2;*/
              /*allocation->y = (wy + automation_point_get_y_in_px (ap)) -*/
                /*AP_WIDGET_CURVE_H / 2;*/
              /*allocation->width = AP_WIDGET_CURVE_W;*/
              /*allocation->height = AP_WIDGET_CURVE_H;*/
            }
        }
    }


  return TRUE;
}

/**
 * For timeline use only.
 */
static Track *
get_track_at_y (double y)
{
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Track * track = MIXER->channels[i]->track;

      GtkAllocation allocation;
      gtk_widget_get_allocation (GTK_WIDGET (track->widget->track_automation_paned),
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (MW_TIMELINE),
                GTK_WIDGET (track->widget->track_automation_paned),
                0,
                0,
                &wx,
                &wy);

      if (y > -wy && y < ((-wy) + allocation.height))
        {
          return track;
        }
    }
  return NULL;
}

/**
 * For timeline use only.
 */
static AutomationTrack *
get_automation_track_at_y (double y)
{
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Track * track = MIXER->channels[i]->track;

      for (int j = 0; j < track->num_automation_tracks; j++)
        {
          /*g_message ("at %d of %d", j, i);*/
          AutomationTrack * at = track->automation_tracks[j];
          if (at->widget)
            {
              GtkAllocation allocation;
              gtk_widget_get_allocation (GTK_WIDGET (at->widget->at_grid),
                                         &allocation);

              gint wx, wy;
              gtk_widget_translate_coordinates(
                        GTK_WIDGET (MW_TIMELINE),
                        GTK_WIDGET (at->widget->at_grid),
                        0,
                        y,
                        &wx,
                        &wy);

              if (wy >= 0 && wy <= allocation.height)
                {
                  return at;
                }
            }
        }
    }

  return NULL;
}

static int
get_note_at_y (double y)
{
  return 128 - y / PIANO_ROLL_LABELS->px_per_note;
}

/**
 * Returns the curve automation point associated with the curve, if (x,y) falls on
 * the curve, otherwise NULL.
 */
static AutomationPoint *
get_hit_curve (ArrangerWidget * self, double x, double y)
{
  GList *children, *iter;

  /* go through each overlay child */
  children = gtk_container_get_children(GTK_CONTAINER(MAIN_WINDOW->timeline));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);

      /* if automation point */
      if (IS_AUTOMATION_POINT_WIDGET (widget))
        {
          AutomationPoint * ap = (AUTOMATION_POINT_WIDGET (widget))->ap;
          if (ap->type != AUTOMATION_POINT_CURVE)
            continue;

          AutomationPoint * prev_ap, * next_ap;
          prev_ap = automation_track_get_prev_ap (ap->at,
                                                  ap);
          next_ap = automation_track_get_next_ap (ap->at,
                                                  ap);

          GtkAllocation allocation;
          gtk_widget_get_allocation (widget,
                                     &allocation);

          gint prev_wx, prev_wy;
          gtk_widget_translate_coordinates(
                    GTK_WIDGET (prev_ap->widget),
                    GTK_WIDGET (MAIN_WINDOW->timeline),
                    0,
                    0,
                    &prev_wx,
                    &prev_wy);

          /*g_message ("x %d y %d prev wx %d prev wy %d",*/
                     /*x, y, prev_wx, prev_wy);*/
          /* if after prev */
          if (x >= prev_wx &&
              y >= prev_wy)
            {
              gint wx, wy;
              gtk_widget_translate_coordinates(
                        GTK_WIDGET (next_ap->widget),
                        GTK_WIDGET (MAIN_WINDOW->timeline),
                        0,
                        0,
                        &wx,
                        &wy);
              /* if before next */
              if (x <= wx &&
                  y <= wy)
                {
                  int ww = wx - prev_wx;
                  int prev_y_px = automation_point_get_y_in_px (prev_ap);
                  int curr_y_px = automation_point_get_y_in_px (next_ap);
                  int height = prev_y_px > curr_y_px ?
                    prev_y_px - curr_y_px :
                    curr_y_px - prev_y_px;
                  if (automation_point_curve_get_y_px (prev_ap,
                                                       x,
                                                       ww,
                                                       height) == (int) y)
                    {
                      g_message ("YES");
                    }
                }
            }
        }
    }
  return NULL;

}



static GtkWidget *
get_hit_widget (ArrangerWidget *  self,
                ArrangerChildType type,
                double            x,
                double            y)
{
  GList *children, *iter;

  /* go through each overlay child */
  children = gtk_container_get_children (GTK_CONTAINER (self));
  for(iter = children; iter != NULL; iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);

      GtkAllocation allocation;
      gtk_widget_get_allocation (widget,
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (self),
                GTK_WIDGET (widget),
                x,
                y,
                &wx,
                &wy);

      /* if hit */
      if (wx >= 0 &&
          wx <= allocation.width &&
          wy >= 0 &&
          wy <= allocation.height)
        {
          if (type == ARRANGER_CHILD_TYPE_MIDI_NOTE &&
              IS_MIDI_NOTE_WIDGET (widget))
            {
              return widget;
            }
          else if (type == ARRANGER_CHILD_TYPE_REGION &&
                   IS_REGION_WIDGET (widget))
            {
              return widget;
            }
          else if (type == ARRANGER_CHILD_TYPE_AP &&
                   IS_AUTOMATION_POINT_WIDGET (widget))
            {
              g_message ("wx %d wy %d", wx, wy);
              return widget;
            }
        }
    }
  return NULL;
}

static RegionWidget *
get_hit_region (ArrangerWidget *  self,
                double            x,
                double            y)
{
  GtkWidget * widget = get_hit_widget (self,
                                       ARRANGER_CHILD_TYPE_REGION,
                                       x,
                                       y);
  if (widget)
    {
      return REGION_WIDGET (widget);
    }
  return NULL;
}

static MidiNoteWidget *
get_hit_midi_note (ArrangerWidget *  self,
                   double            x,
                   double            y)
{
  GtkWidget * widget = get_hit_widget (self,
                                       ARRANGER_CHILD_TYPE_MIDI_NOTE,
                                       x,
                                       y);
  if (widget)
    {
      return MIDI_NOTE_WIDGET (widget);
    }
  return NULL;
}

static AutomationPointWidget *
get_hit_automation_point (ArrangerWidget *  self,
                          double            x,
                          double            y)
{
  GtkWidget * widget = get_hit_widget (self,
                                       ARRANGER_CHILD_TYPE_AP,
                                       x,
                                       y);
  if (widget)
    {
      return AUTOMATION_POINT_WIDGET (widget);
    }
  return NULL;
}

static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;
  self->n_press = n_press;
  gtk_widget_grab_focus (GTK_WIDGET (self));

  /* open MIDI editor if double clicked on a region */
  if (T_TIMELINE)
    {
      RegionWidget * region_widget = get_hit_region (self, x, y);
      if (region_widget && n_press > 0)
        {
          arranger_widget_set_channel(
                      MIDI_EDITOR->midi_arranger,
                      region_widget->region->track->channel);
        }
    }
  else if (T_MIDI)
    {
      MidiNoteWidget * midi_note_widget = get_hit_midi_note (self, x, y);
      if (midi_note_widget && n_press == 2)
        {
          /* TODO */
          g_message ("note double clicked ");
        }
    }
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;
  self->start_x = start_x;
  self->start_y = start_y;

  MidiNoteWidget * midi_note_widget = T_MIDI ?
    get_hit_midi_note (self, start_x, start_y) :
    NULL;
  RegionWidget * region_widget = T_TIMELINE ?
    get_hit_region (self, start_x, start_y) :
    NULL;
  AutomationPoint * curve_ap = T_TIMELINE ?
    get_hit_curve (self, start_x, start_y) :
    NULL;
  AutomationPointWidget * ap_widget = T_TIMELINE ?
    get_hit_automation_point (self, start_x, start_y) :
    NULL;
  int is_hit = midi_note_widget || region_widget || curve_ap || ap_widget;
  if (is_hit)
    {
      if (midi_note_widget)
        {
          self->midi_note = midi_note_widget->midi_note;
          self->start_pos_px = ruler_widget_pos_to_px (&self->midi_note->start_pos);
          position_set_to_pos (&self->start_pos, &self->midi_note->start_pos);
          position_set_to_pos (&self->end_pos, &self->midi_note->end_pos);
        }
      else if (region_widget)
        {
          self->region= region_widget->region;
          self->start_pos_px = ruler_widget_pos_to_px (&self->region->start_pos);
          position_set_to_pos (&self->start_pos, &self->region->start_pos);
          position_set_to_pos (&self->end_pos, &self->region->end_pos);
        }
      else if (ap_widget)
        {
          self->ap = ap_widget->ap;
          self->start_pos_px = start_x;
        }
      else if (curve_ap)
        {
          self->ap = curve_ap;
          self->start_pos_px = start_x;
        }

      if (midi_note_widget)
        {
          switch (midi_note_widget->hover_state)
            {
            case MIDI_NOTE_HOVER_STATE_NONE:
              g_warning ("hitting midi note but midi note hover state is none, should be fixed");
              break;
            case MIDI_NOTE_HOVER_STATE_EDGE_L:
              self->action = ARRANGER_ACTION_RESIZING_L;
              break;
            case MIDI_NOTE_HOVER_STATE_EDGE_R:
              self->action = ARRANGER_ACTION_RESIZING_R;
              break;
            case MIDI_NOTE_HOVER_STATE_MIDDLE:
              self->action = ARRANGER_ACTION_MOVING;
              ui_set_cursor (GTK_WIDGET (midi_note_widget), "grabbing");
              break;
            }
        }
      else if (region_widget)
        {
          switch (region_widget->hover_state)
            {
            case REGION_HOVER_STATE_NONE:
              g_warning ("hitting region but region hover state is none, should be fixed");
              break;
            case REGION_HOVER_STATE_EDGE_L:
              self->action = ARRANGER_ACTION_RESIZING_L;
              break;
            case REGION_HOVER_STATE_EDGE_R:
              self->action = ARRANGER_ACTION_RESIZING_R;
              break;
            case REGION_HOVER_STATE_MIDDLE:
              self->action = ARRANGER_ACTION_MOVING;
              ui_set_cursor (GTK_WIDGET (region_widget), "grabbing");
              break;
            }
        }
      else if (ap_widget)
        {
          switch (ap_widget->hover_state)
            {
            case AP_HOVER_STATE_NONE:
              g_warning ("hitting AP but AP hover state is none, should be fixed");
              break;
            case AP_HOVER_STATE_MIDDLE:
              self->action = ARRANGER_ACTION_MOVING;
              ui_set_cursor (GTK_WIDGET (ap_widget), "grabbing");
              break;
            }
        }
      else if (curve_ap)
        {
          /* TODO */
        }
    }
  else /* no note hit */
    {
      if (self->n_press == 1)
        {
          /* area selection */
          self->action = ARRANGER_ACTION_SELECTING;

        }
      else if (self->n_press == 2)
        {
          Position pos;
          ruler_widget_px_to_pos (
                               &pos,
                               start_x - SPACE_BEFORE_START);

          Track * track;
          AutomationTrack * at;
          int note;
          Region * region;
          if (T_TIMELINE)
            {
              at = get_automation_track_at_y (start_y);
              if (!at)
                track = get_track_at_y (start_y);
            }
          else if (T_MIDI)
            {
              note = (MIDI_EDITOR->piano_roll_labels->total_px - start_y) /
                MIDI_EDITOR->piano_roll_labels->px_per_note;

              /* if inside a region */
              if (SELECTIONS.num_channels > 0)
                {
                  region = region_at_position (
                              SELECTIONS.channels[
                                SELECTIONS.num_channels - 1]->track,
                              &pos);
                }
            }
          if (((T_TIMELINE && track) || (T_TIMELINE && at)) ||
              (T_MIDI && region))
            {
              if (T_TIMELINE && at)
                {
                  position_snap (NULL,
                                 &pos,
                                 track,
                                 NULL,
                                 self->snap_grid);

                  /* if the automatable is float in this automation track */
                  if (automatable_is_float (at->automatable))
                    {
                      /* add automation point to automation track */
                      float value = automation_track_widget_get_fvalue_at_y (
                                                      at->widget,
                                                      start_y);

                      self->ap = automation_point_new_float (at,
                                                             AUTOMATION_POINT_VALUE,
                                                             value,
                                                             &pos);
                      automation_track_add_automation_point (at,
                                                             self->ap,
                                                             GENERATE_CURVE_POINTS);
                      gtk_overlay_add_overlay (GTK_OVERLAY (self),
                                               GTK_WIDGET (self->ap->widget));
                      gtk_widget_show (GTK_WIDGET (self->ap->widget));

                    }
                }
              else if (T_TIMELINE && track)
                {
                  position_snap (NULL,
                                 &pos,
                                 track,
                                 NULL,
                                 self->snap_grid);
                  self->region = region_new (track,
                                             &pos,
                                             &pos);
                  position_set_min_size (&self->region->start_pos,
                                         &self->region->end_pos,
                                         self->snap_grid);
                  track_add_region (track,
                                    self->region);
                  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                                           GTK_WIDGET (self->region->widget));
                  gtk_widget_show (GTK_WIDGET (self->region->widget));
                  self->action = ARRANGER_ACTION_RESIZING_R;
                }
              else if (T_MIDI && region)
                {
                  position_snap (NULL,
                                 &pos,
                                 NULL,
                                 region,
                                 self->snap_grid);
                  self->midi_note = midi_note_new (region,
                                                   &pos,
                                                   &pos,
                                                   note,
                                                   -1);
                  position_set_min_size (&self->midi_note->start_pos,
                                         &self->midi_note->end_pos,
                                         self->snap_grid);
                  region_add_midi_note (region,
                                        self->midi_note);
                  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                                           GTK_WIDGET (self->midi_note->widget));
                  gtk_widget_show (GTK_WIDGET (self->midi_note->widget));
                  self->action = ARRANGER_ACTION_RESIZING_R;
                }
              gtk_widget_queue_allocate (GTK_WIDGET (self));
            }
        }
    }
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  ArrangerWidget * self = ARRANGER_WIDGET (user_data);

  if (self->action == ARRANGER_ACTION_SELECTING)
    {
    }

  /* handle x */
  else if (self->action == ARRANGER_ACTION_RESIZING_L)
    {
      Position pos;
      ruler_widget_px_to_pos (&pos,
                 (self->start_x + offset_x) - SPACE_BEFORE_START);
      if (T_TIMELINE)
        {
          position_snap (NULL,
                         &pos,
                         self->region->track,
                         NULL,
                         self->snap_grid);
          region_set_start_pos (self->region,
                                &pos,
                                0);
        }
      else if (T_MIDI)
        {
          position_snap (NULL,
                         &pos,
                         NULL,
                         self->midi_note->region,
                         self->snap_grid);
          midi_note_set_start_pos (self->midi_note,
                                &pos);
        }
    }
  else if (self->action == ARRANGER_ACTION_RESIZING_R)
    {
      Position pos;
      ruler_widget_px_to_pos (&pos,
                 (self->start_x + offset_x) - SPACE_BEFORE_START);
      if (T_TIMELINE)
        {
          position_snap (NULL,
                         &pos,
                         self->region->track,
                         NULL,
                         self->snap_grid);
          if (position_compare (&pos, &self->region->start_pos) > 0)
            {
              region_set_end_pos (self->region,
                                    &pos);
            }
        }
      else if (T_MIDI)
        {
          position_snap (NULL,
                         &pos,
                         NULL,
                         self->midi_note->region,
                         self->snap_grid);
          if (position_compare (&pos, &self->midi_note->start_pos) > 0)
            {
              midi_note_set_end_pos (self->midi_note,
                                    &pos);
            }
        }
    }
  else if (self->action == ARRANGER_ACTION_MOVING)
    {
      Position pos;
      ruler_widget_px_to_pos (&pos,
                              self->start_pos_px + offset_x);

      if (T_TIMELINE && self->region)
        {
          position_snap (NULL,
                         &pos,
                         self->region->track,
                         NULL,
                         self->snap_grid);
          region_set_start_pos (self->region,
                                &pos,
                                1);
        }
      else if (T_TIMELINE && self->ap && self->ap->type == AUTOMATION_POINT_VALUE)
        {
          ruler_widget_px_to_pos (&pos,
                                  (self->start_pos_px + offset_x) -
                                    SPACE_BEFORE_START);
          /* get prev and next value APs */
          AutomationPoint * prev_ap = automation_track_get_prev_ap (self->ap->at,
                                                                    self->ap);
          AutomationPoint * next_ap = automation_track_get_next_ap (self->ap->at,
                                                                    self->ap);
          Position mid_pos;
          AutomationPoint * curve_ap;
          position_snap (NULL,
                         &pos,
                         self->ap->at->track,
                         NULL,
                         self->snap_grid);
          if (prev_ap && position_compare (&pos, &prev_ap->pos) >= 0)
            {
              /* set prev curve point to new midway pos */
              position_get_midway_pos (&prev_ap->pos,
                                       &pos,
                                       &mid_pos);
              curve_ap = automation_track_get_next_curve_ap (self->ap->at,
                                                             prev_ap);
              position_set_to_pos (&curve_ap->pos, &mid_pos);

              /* set pos for ap */
              position_set_to_pos (&self->ap->pos, &pos);
            }
          if (next_ap && position_compare (&pos, &next_ap->pos) <= 0)
            {
              /* set next curve point to new midway pos */
              position_get_midway_pos (&pos,
                                       &next_ap->pos,
                                       &mid_pos);
              curve_ap = automation_track_get_next_curve_ap (self->ap->at,
                                                             self->ap);
              position_set_to_pos (&curve_ap->pos, &mid_pos);

              /* set pos for ap */
              position_set_to_pos (&self->ap->pos, &pos);
            }
        }
      else if (T_TIMELINE && self->ap && self->ap->type == AUTOMATION_POINT_CURVE)
        {
          /* TODO */
        }
      else if (T_MIDI)
        {
          position_snap (NULL,
                         &pos,
                         NULL,
                         self->midi_note->region,
                         self->snap_grid);
          midi_note_set_start_pos (self->midi_note,
                                &pos);
        }
      int diff = position_to_frames (&pos) - position_to_frames (&self->start_pos);
      position_set_to_pos (&pos, &self->end_pos);
      position_add_frames (&pos,
                           diff);

      /* handle y */
      if (T_TIMELINE && self->region)
        {
          region_set_end_pos (self->region,
                              &pos);

          /* check if should be moved to new track */
          Track * track = get_track_at_y (self->start_y + offset_y);
          if (track && self->region->track != track)
            {
              Track * old_track = self->region->track;
              track_remove_region (old_track, self->region);
              track_add_region (track, self->region);
            }
        }
      else if (T_TIMELINE && self->ap && self->ap->type == AUTOMATION_POINT_VALUE)
        {
          float fval =
            automation_track_widget_get_fvalue_at_y (self->ap->at->widget,
                                                     self->start_y + offset_y);
          automation_point_update_fvalue (self->ap, fval);
        }
      else if (T_MIDI)
        {
          midi_note_set_end_pos (self->midi_note,
                                 &pos);
          /* check if should be moved to new note  */
          self->midi_note->val = get_note_at_y (self->start_y + offset_y);
        }
    }
  gtk_widget_queue_allocate(GTK_WIDGET (self));
  if (T_MIDI)
    {
      gtk_widget_queue_draw (GTK_WIDGET (self));
      gtk_widget_show_all (GTK_WIDGET (self));
    }
  else if (T_TIMELINE)
    {
      gtk_widget_queue_allocate (GTK_WIDGET (MIDI_EDITOR->midi_arranger));
    }
  self->last_offset_x = offset_x;
  self->last_offset_y = offset_y;
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;
  self->start_x = 0;
  self->start_y = 0;
  self->last_offset_x = 0;
  self->last_offset_y = 0;
  self->action = ARRANGER_ACTION_NONE;
  if (self->midi_note)
    {
      ui_set_cursor (GTK_WIDGET (self->midi_note->widget), "default");
    }
  self->midi_note = NULL;
  if (self->region)
    {
      ui_set_cursor (GTK_WIDGET (self->region->widget), "default");
    }
  self->region = NULL;
  self->ap = NULL;
  gtk_widget_queue_draw (GTK_WIDGET (self->bg));
}


ArrangerWidget *
arranger_widget_new (ArrangerWidgetType type, SnapGrid * snap_grid)
{
  g_message ("Creating arranger %d...", type);
  ArrangerWidget * self = g_object_new (ARRANGER_WIDGET_TYPE, NULL);
  self->snap_grid = snap_grid;
  self->type = type;
  if (T_MIDI)
    {
      MAIN_WINDOW->midi_editor->midi_arranger = self;
      self->bg = GTK_DRAWING_AREA (midi_arranger_bg_widget_new ());
    }
  else if (T_TIMELINE)
    {
      MAIN_WINDOW->timeline = self;
      self->bg = GTK_DRAWING_AREA (timeline_bg_widget_new ());
    }



  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->bg));

  /* make it able to notify */
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);
  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);

  g_signal_connect (G_OBJECT (self), "get-child-position",
                    G_CALLBACK (get_child_position), NULL);
  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);


  return self;
}

static void
arranger_widget_class_init (ArrangerWidgetClass * klass)
{
}

static void
arranger_widget_init (ArrangerWidget *self )
{
  self->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
}

/**
 * Draws the selection in its background.
 *
 * Should only be called by the bg widgets when drawing.
 */
void
arranger_bg_draw_selections (ArrangerWidget * self,
                             cairo_t *        cr)
{
  double offset_x, offset_y;
  offset_x = self->start_x + self->last_offset_x > 0 ?
    self->last_offset_x :
    1 - self->start_x;
  offset_y = self->start_y + self->last_offset_y > 0 ?
    self->last_offset_y :
    1 - self->start_y;
  if (self->action == ARRANGER_ACTION_SELECTING)
    {
      cairo_set_source_rgba (cr, 0.9, 0.9, 0.9, 1.0);
      cairo_rectangle (cr,
                       self->start_x,
                       self->start_y,
                       offset_x,
                       offset_y);
      cairo_stroke (cr);
      cairo_set_source_rgba (cr, 0.3, 0.3, 0.3, 0.3);
      cairo_rectangle (cr,
                       self->start_x,
                       self->start_y,
                       offset_x,
                       offset_y);
      cairo_fill (cr);
    }
}
