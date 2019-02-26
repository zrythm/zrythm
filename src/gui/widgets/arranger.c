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
 * Arranger parent class containing common
 * logic for the timeline and piano roll.
 */

#include "zrythm.h"
#include "gui/widgets/region.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_playhead.h"
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
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_modifier_arranger_bg.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/piano_roll.h"
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
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (ArrangerWidget,
                            arranger_widget,
                            GTK_TYPE_OVERLAY)

#define GET_PRIVATE ARRANGER_WIDGET_GET_PRIVATE (self)

/**
 * Gets each specific arranger.
 *
 * Useful boilerplate
 */
#define GET_ARRANGER_ALIASES(self) \
  TimelineArrangerWidget *     timeline = NULL; \
  MidiArrangerWidget *         midi_arranger = NULL; \
  MidiModifierArrangerWidget * midi_modifier_arranger = NULL; \
  if (ARRANGER_IS_MIDI (self)) \
    { \
      midi_arranger = Z_MIDI_ARRANGER_WIDGET (self); \
    } \
  else if (ARRANGER_IS_TIMELINE (self)) \
    { \
      timeline = Z_TIMELINE_ARRANGER_WIDGET (self); \
    } \
  else if (ARRANGER_IS_MIDI_MODIFIER (self)) \
    { \
      midi_modifier_arranger = \
        Z_MIDI_MODIFIER_ARRANGER_WIDGET (self); \
    }

ArrangerWidgetPrivate *
arranger_widget_get_private (ArrangerWidget * self)
{
  return arranger_widget_get_instance_private (self);
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

  GET_ARRANGER_ALIASES (self);

  if (Z_IS_ARRANGER_PLAYHEAD_WIDGET (widget))
    {
      if (timeline)
        allocation->x =
          ui_pos_to_px_timeline (
            &TRANSPORT->playhead_pos,
            1) - 1; /* minus half the width */
      else if ((midi_arranger ||
               midi_modifier_arranger) &&
               PIANO_ROLL->region)
        {
          Position pos;
          region_timeline_pos_to_local (
            PIANO_ROLL->region,
            &TRANSPORT->playhead_pos,
            &pos);
          allocation->x =
            ui_pos_to_px_piano_roll (
              &pos,
              1) - 1; /* minus half the width */
        }
      allocation->y = 0;
      allocation->width = 2;
      allocation->height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));
    }
  if (midi_arranger)
    {
      midi_arranger_widget_set_allocation (
        midi_arranger,
        widget,
        allocation);
    }
  else if (timeline)
    {
      timeline_arranger_widget_set_allocation (
        timeline,
        widget,
        allocation);
    }
  else if (midi_modifier_arranger)
    {
      midi_modifier_arranger_widget_set_allocation (
        midi_modifier_arranger,
        widget,
        allocation);
    }
  return TRUE;
}

void
arranger_widget_get_hit_widgets_in_range (
  ArrangerWidget *  self,
  GType             type,
  double            start_x,
  double            start_y,
  double            offset_x,
  double            offset_y,
  GtkWidget **      array, ///< array to fill
  int *             array_size) ///< array_size to fill
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
                start_x,
                start_y,
                &wx,
                &wy);

      int x_hit, y_hit;
      if (offset_x < 0)
        {
          x_hit = wx + offset_x <= allocation.width &&
            wx > 0;
        }
      else
        {
          x_hit = wx <= allocation.width &&
            wx + offset_x > 0;
        }
      if (!x_hit) continue;
      if (offset_y < 0)
        {
          y_hit = wy + offset_y <= allocation.height &&
            wy > 0;
        }
      else
        {
          y_hit = wy <= allocation.height &&
            wy + offset_y > 0;
        }
      if (!y_hit) continue;

      if (type == MIDI_NOTE_WIDGET_TYPE &&
          Z_IS_MIDI_NOTE_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == REGION_WIDGET_TYPE &&
               Z_IS_REGION_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == AUTOMATION_POINT_WIDGET_TYPE &&
               Z_IS_AUTOMATION_POINT_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == CHORD_WIDGET_TYPE &&
               Z_IS_CHORD_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
    }
}

static void
update_inspector (ArrangerWidget *self)
{
  if (ARRANGER_IS_MIDI (self))
    {
      midi_arranger_widget_update_inspector (
        Z_MIDI_ARRANGER_WIDGET (self));
    }
  else if (ARRANGER_IS_TIMELINE (self))
    {
      timeline_arranger_widget_update_inspector (
        Z_TIMELINE_ARRANGER_WIDGET (self));
    }
}

/**
 * Selects the region.
 */
void
arranger_widget_toggle_select (ArrangerWidget *  self,
               GType             type,
               void *            child,
               int               append)
{
  void ** array;
  int * num;

  if (ARRANGER_IS_TIMELINE (self))
    {
      if (type == REGION_WIDGET_TYPE)
        {
          array = (void **) TIMELINE_SELECTIONS->regions;
          num = &TIMELINE_SELECTIONS->num_regions;
        }
      else if (type == AUTOMATION_POINT_WIDGET_TYPE)
        {
          array =
            (void **) TIMELINE_SELECTIONS->automation_points;
          num = &TIMELINE_SELECTIONS->num_automation_points;
        }
      else if (type == CHORD_WIDGET_TYPE)
        {
          array =
            (void **) TIMELINE_SELECTIONS->chords;
          num = &TIMELINE_SELECTIONS->num_chords;
        }
    }
  if (ARRANGER_IS_MIDI (self))
    {
      if (type == MIDI_NOTE_WIDGET_TYPE)
        {
          array =
            (void **) MIDI_ARRANGER_SELECTIONS->midi_notes;
          num = &MIDI_ARRANGER_SELECTIONS->num_midi_notes;
        }
    }

  if (!append)
    {
      /* deselect existing selections */
      for (int i = 0; i < (*num); i++)
        {
          void * r = array[i];
          if (type == REGION_WIDGET_TYPE)
            {
              region_widget_select (((Region *)r)->widget, 0);
            }
          else if (type == CHORD_WIDGET_TYPE)
            {
              chord_widget_select (((Chord *)r)->widget, 0);

            }
        }
      *num = 0;
    }

  /* if already selected */
  if (array_contains (array,
                      *num,
                      child))
    {
      /* deselect */
      array_delete (array,
                    *num,
                    child);
      if (type == REGION_WIDGET_TYPE)
        {
          region_widget_select (((Region *)child)->widget, 0);
        }
      else if (type == CHORD_WIDGET_TYPE)
        {
          chord_widget_select (((Chord *)child)->widget, 0);
        }
    }
  else /* not selected */
    {
      /* select */
      array_append (array,
                    (*num),
                    child);
      if (type == REGION_WIDGET_TYPE)
        {
          region_widget_select (((Region *)child)->widget, 1);
        }
      else if (type == CHORD_WIDGET_TYPE)
        {
          chord_widget_select (((Chord *)child)->widget, 1);
        }
    }
}

/**
 * Refreshes all arranger backgrounds.
 */
void
arranger_widget_refresh_all_backgrounds ()
{
  ArrangerWidgetPrivate * ar_prv;

  ar_prv =
    arranger_widget_get_private (
      Z_ARRANGER_WIDGET (MW_TIMELINE));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      Z_ARRANGER_WIDGET (MIDI_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      Z_ARRANGER_WIDGET (MIDI_MODIFIER_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
}

void
arranger_widget_select_all (ArrangerWidget *  self,
                            int               select)
{
  GET_ARRANGER_ALIASES (self);

  if (midi_arranger)
    {
      midi_arranger_widget_select_all (
        midi_arranger,
        select);
    }
  else if (timeline)
    {
      timeline_arranger_widget_select_all (
        timeline,
        select);
    }
  else if (midi_modifier_arranger)
    {
      /* TODO */
    }
  update_inspector (self);
}


static void
show_context_menu (ArrangerWidget * self)
{
  if (ARRANGER_IS_MIDI (self))
    {
      midi_arranger_widget_show_context_menu (
        Z_MIDI_ARRANGER_WIDGET (self));
    }
  else if (ARRANGER_IS_TIMELINE (self))
    {
      timeline_arranger_widget_show_context_menu (
        Z_TIMELINE_ARRANGER_WIDGET (self));
    }
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;

  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

static gboolean
on_key_action (GtkWidget *widget,
               GdkEventKey  *event,
               gpointer   user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;

  if (event->state & GDK_CONTROL_MASK &&
      event->type == GDK_KEY_PRESS &&
      event->keyval == GDK_KEY_a)
    {
      arranger_widget_select_all (self, 1);
    }

  return FALSE;
}

/**
 * On button press.
 *
 * This merely sets the number of clicks and
 * objects clicked on. It is always called
 * before drag_begin, so the logic is done in drag_begin.
 */
static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;
  GET_PRIVATE;

  ar_prv->n_press = n_press;
  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (
      GTK_GESTURE_SINGLE (gesture));
  const GdkEvent * event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);
  if (state_mask & GDK_SHIFT_MASK)
    ar_prv->shift_held = 1;
}

static void
drag_begin (GtkGestureDrag *   gesture,
            gdouble            start_x,
            gdouble            start_y,
            ArrangerWidget *   self)
{
  GET_PRIVATE;

  ar_prv->start_x = start_x;
  ar_prv->start_y = start_y;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (
      GTK_GESTURE_SINGLE (gesture));
  const GdkEvent * event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);

  GET_ARRANGER_ALIASES (self);

  /* get hit arranger objects */
  /* FIXME move this part to multipress, makes
   * more sense and makes this function smaller/
   * easier to understand.
   */
  MidiNoteWidget *        midi_note_widget = NULL;
  RegionWidget *          rw = NULL;
  AutomationCurveWidget * ac_widget = NULL;
  AutomationPointWidget * ap_widget = NULL;
  ChordWidget *           chord_widget = NULL;
  if (midi_arranger)
    {
      midi_arranger = Z_MIDI_ARRANGER_WIDGET (self);
      midi_note_widget =
        midi_arranger_widget_get_hit_midi_note (
          midi_arranger, start_x, start_y);
    }
  else if (timeline)
    {
      timeline = Z_TIMELINE_ARRANGER_WIDGET (self);
      rw =
        timeline_arranger_widget_get_hit_region (
          timeline, start_x, start_y);
      ac_widget =
        timeline_arranger_widget_get_hit_curve (
          timeline, start_x, start_y);
      ap_widget =
        timeline_arranger_widget_get_hit_ap (
          timeline, start_x, start_y);
      chord_widget =
        timeline_arranger_widget_get_hit_chord (
          timeline, start_x, start_y);
    }

  /* if something is hit */
  int is_hit =
    midi_note_widget || rw || ac_widget || ap_widget ||
    chord_widget;
  if (is_hit)
    {
      /* set selections, positions, actions, cursor */
      if (midi_note_widget)
        {
          midi_arranger_widget_on_drag_begin_note_hit (
            midi_arranger,
            state_mask,
            start_x,
            midi_note_widget);
        }
      else if (rw)
        {
          timeline_arranger_widget_on_drag_begin_region_hit (
            timeline,
            state_mask,
            start_x,
            rw);
        }
      else if (chord_widget)
        {
          timeline_arranger_widget_on_drag_begin_chord_hit (
            timeline,
            state_mask,
            start_x,
            chord_widget);
        }
      else if (ap_widget)
        {
          timeline_arranger_widget_on_drag_begin_ap_hit (
            timeline,
            state_mask,
            start_x,
            ap_widget);
        }
      else if (ac_widget)
        {
          timeline->start_ac =
            ac_widget->ac;
        }

      /* find start pos */
      position_init (&ar_prv->start_pos);
      position_set_bar (&ar_prv->start_pos, 2000);
      if (timeline)
        {
          timeline_arranger_widget_find_start_poses (
            timeline);
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_find_start_poses (
            midi_arranger);
        }
    }
  else /* nothing hit */
    {
      /* single click */
      if (ar_prv->n_press == 1)
        {
          /* selection */
          ar_prv->action =
            UI_OVERLAY_ACTION_STARTING_SELECTION;

          /* deselect all */
          arranger_widget_select_all (self, 0);

          /* if timeline, set either selecting
           * objects or selecting range */
          if (timeline)
            {
              timeline_arranger_widget_set_select_type (
                timeline,
                start_y);
            }
        }
      /* double click, something will be created */
      else if (ar_prv->n_press == 2)
        {
          Position pos;
          Track * track = NULL;
          AutomationTrack * at = NULL;
          int note;
          Region * region = NULL;

          /* get the position */
          if (timeline)
            ui_px_to_pos_timeline (
              start_x, &pos, 1);
          else if (midi_arranger ||
                   midi_modifier_arranger)
            ui_px_to_pos_piano_roll (
              start_x, &pos, 1);

          if (timeline)
            {
              /* figure out if we are creating a region or
               * automation point */
              at =
                timeline_arranger_widget_get_automation_track_at_y (start_y);
              if (!at)
                track =
                  timeline_arranger_widget_get_track_at_y (start_y);

              /* creating automation point */
              if (at)
                {
                  timeline_arranger_widget_create_ap (
                    timeline,
                    at,
                    track,
                    &pos,
                    start_y);
                }
              /* double click inside a track */
              else if (track)
                {
                  switch (track->type)
                    {
                    case TRACK_TYPE_INSTRUMENT:
                    case TRACK_TYPE_AUDIO:
                      timeline_arranger_widget_create_region (
                        timeline,
                        track,
                        &pos);
                      break;
                    case TRACK_TYPE_MASTER:
                      break;
                    case TRACK_TYPE_CHORD:
                      timeline_arranger_widget_create_chord (
                        timeline,
                        track,
                        &pos);
                    case TRACK_TYPE_BUS:
                      break;
                    }
                }
            }
          else if (midi_arranger)
            {
              /* find the note and region at x,y */
              note =
                (MW_PIANO_ROLL->piano_roll_labels->total_px
                  - start_y) /
                MW_PIANO_ROLL->piano_roll_labels->px_per_note;
              region = PIANO_ROLL->region;

              /* create a note */
              if (region)
                {
                  midi_arranger_widget_create_note (
                    midi_arranger,
                    &pos,
                    note,
                    (MidiRegion *) region);
                }
            }

          /* something is (likely) added so reallocate */
          gtk_widget_queue_allocate (GTK_WIDGET (self));
        }
    }

  /* update inspector */
  update_inspector (self);
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  ArrangerWidget * self =
    Z_ARRANGER_WIDGET (user_data);
  GET_PRIVATE;

  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (
      GTK_GESTURE_SINGLE (gesture));
  const GdkEvent * event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);
  if (state_mask & GDK_SHIFT_MASK)
    ar_prv->shift_held = 1;
  else
    ar_prv->shift_held = 0;

  GET_ARRANGER_ALIASES (self);

  /* set action to selecting if starting selection. this
   * is because drag_update never gets called if it's just
   * a click, so we can check at drag_end and see if
   * anything was selected */
  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_SELECTION)
    {
      ar_prv->action =
        UI_OVERLAY_ACTION_SELECTING;
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      ar_prv->action =
        UI_OVERLAY_ACTION_MOVING;
    }

  /* if drawing a selection */
  if (ar_prv->action ==
        UI_OVERLAY_ACTION_SELECTING)
    {
      /* find and select objects inside selection */
      if (timeline)
        {
          timeline_arranger_widget_select (
            timeline,
            offset_x,
            offset_y);
        }
      else if (ARRANGER_IS_MIDI (self))
        {
          midi_arranger_widget_select (
            midi_arranger,
            offset_x,
            offset_y);
        }
    } /* endif UI_OVERLAY_ACTION_SELECTING */

  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_RESIZING_L)
    {

      /* get new pos */
      Position pos;
      if (timeline)
        ui_px_to_pos_timeline (
          ar_prv->start_x + offset_x, &pos, 1);
      else if (midi_arranger ||
               midi_modifier_arranger)
        ui_px_to_pos_piano_roll (
          ar_prv->start_x + offset_x, &pos, 1);

      /* snap selections based on new pos */
      if (timeline)
        {
          timeline_arranger_widget_snap_regions_l (
            timeline,
            &pos);
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_snap_midi_notes_l (
            midi_arranger,
            &pos);
        }
    } /* endif RESIZING_L */
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_RESIZING_R)
    {
      Position pos;

      /* get position */
      if (timeline)
        ui_px_to_pos_timeline (
          ar_prv->start_x + offset_x, &pos, 1);
      else if (midi_arranger ||
               midi_modifier_arranger)
        ui_px_to_pos_piano_roll (
          ar_prv->start_x + offset_x, &pos, 1);

      if (timeline)
        {
          if (timeline->resizing_range)
            timeline_arranger_widget_snap_range_r (
              &pos);
          else
            timeline_arranger_widget_snap_regions_r (
              Z_TIMELINE_ARRANGER_WIDGET (self),
              &pos);
        }
      else if (ARRANGER_IS_MIDI (self))
        {
          midi_arranger_widget_snap_midi_notes_r (
            Z_MIDI_ARRANGER_WIDGET (self),
            &pos);
        }
    } /*endif RESIZING_R */

  /* if moving the selection */
  else if (ar_prv->action == UI_OVERLAY_ACTION_MOVING)
    {
      /* get the offset pos (so we can add it to the start
       * positions and then snap it) */
      Position diff_pos;
      int is_negative = offset_x < 0;
      if (timeline)
        ui_px_to_pos_timeline (
          abs (offset_x), &diff_pos, 0);
      else if (midi_arranger ||
               midi_modifier_arranger)
        ui_px_to_pos_piano_roll (
          abs (offset_x), &diff_pos, 0);
      long ticks_diff = position_to_ticks (&diff_pos);
      if (is_negative)
        ticks_diff = - ticks_diff;

      /* get new start pos and snap it */
      Position new_start_pos;
      position_set_to_pos (&new_start_pos,
                           &ar_prv->start_pos);
      position_add_ticks (&new_start_pos, ticks_diff);
      if (SNAP_GRID_ANY_SNAP(ar_prv->snap_grid))
        position_snap (NULL,
                       &new_start_pos,
                       NULL,
                       NULL,
                       ar_prv->snap_grid);

      /* get frames difference from snapped new position to
       * start pos */
      ticks_diff = position_to_ticks (&new_start_pos) -
        position_to_ticks (&ar_prv->start_pos);

      if (timeline)
        {
          timeline_arranger_widget_move_items_x (
            timeline,
            ticks_diff);
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_move_items_x (
            midi_arranger,
            ticks_diff);
        }

      /* handle moving up/down */
      if (timeline)
        {
          timeline_arranger_widget_move_items_y (
            timeline,
            offset_y);
        }
      else if (midi_arranger)
        {
          midi_arranger_widget_move_items_y (
            midi_arranger,
            offset_y);
        }
    } /* endif MOVING */

  /* things have changed, reallocate */
  gtk_widget_queue_allocate(GTK_WIDGET (self));

  /* redraw midi ruler if region positions were changed */
  if (timeline && TIMELINE_SELECTIONS->num_regions > 0 &&
      (ar_prv->action == UI_OVERLAY_ACTION_MOVING ||
      ar_prv->action == UI_OVERLAY_ACTION_RESIZING_L ||
      ar_prv->action == UI_OVERLAY_ACTION_RESIZING_R))
    gtk_widget_queue_draw (GTK_WIDGET (MIDI_RULER));

  /* update last offsets */
  ar_prv->last_offset_x = offset_x;
  ar_prv->last_offset_y = offset_y;

  /* update inspector */
  update_inspector (self);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;
  GET_PRIVATE;

  /* reset start coordinates and offsets */
  ar_prv->start_x = 0;
  ar_prv->start_y = 0;
  ar_prv->last_offset_x = 0;
  ar_prv->last_offset_y = 0;

  ar_prv->shift_held = 0;

  GET_ARRANGER_ALIASES (self);

  if (midi_arranger)
    {
      midi_arranger_widget_on_drag_end (
        midi_arranger);
    }
  else if (timeline)
    {
      timeline_arranger_widget_on_drag_end (
        timeline);
    }
  else if (midi_modifier_arranger)
    {
      /* TODO */
    }

  /* if clicked on nothing */
  if (ar_prv->action ==
           UI_OVERLAY_ACTION_STARTING_SELECTION)
    {
      /* deselect all */
      arranger_widget_select_all (self, 0);
    }

  /* reset action */
  ar_prv->action = UI_OVERLAY_ACTION_NONE;

  /* queue redraw to hide the selection */
  gtk_widget_queue_draw (GTK_WIDGET (ar_prv->bg));
}


void
arranger_widget_setup (ArrangerWidget *   self,
                       SnapGrid *         snap_grid)
{
  GET_PRIVATE;

  GET_ARRANGER_ALIASES (self);

  ar_prv->snap_grid = snap_grid;

  /* create and set background widgets, setup */
  if (midi_arranger)
    {
      ar_prv->bg = Z_ARRANGER_BG_WIDGET (
        midi_arranger_bg_widget_new (
          Z_RULER_WIDGET (MIDI_RULER),
          self));
      midi_arranger_widget_setup (
        midi_arranger);
    }
  else if (timeline)
    {
      ar_prv->bg = Z_ARRANGER_BG_WIDGET (
        timeline_bg_widget_new (
          Z_RULER_WIDGET (MW_RULER),
          self));
      timeline_arranger_widget_setup (
        timeline);
    }
  else if (midi_modifier_arranger)
    {
      ar_prv->bg = Z_ARRANGER_BG_WIDGET (
        midi_modifier_arranger_bg_widget_new (
          Z_RULER_WIDGET (MIDI_RULER),
          self));
      midi_modifier_arranger_widget_setup (
        midi_modifier_arranger);
    }
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (ar_prv->bg));


  /* add the playhead */
  ar_prv->playhead =
    arranger_playhead_widget_new ();
  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                           GTK_WIDGET (ar_prv->playhead));

  /* make the arranger able to notify */
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);
  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "get-child-position",
    G_CALLBACK (get_child_position), NULL);
  g_signal_connect (
    G_OBJECT(ar_prv->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(ar_prv->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(ar_prv->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (ar_prv->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (ar_prv->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT (self), "key-press-event",
    G_CALLBACK (on_key_action), self);
  g_signal_connect (
    G_OBJECT (self), "key-release-event",
    G_CALLBACK (on_key_action), self);
}

/**
 * Readd children.
 */
void
arranger_widget_refresh (
  ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  GET_ARRANGER_ALIASES (self);

  if (midi_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (MIDI_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        rw_prv->total_px,
        gtk_widget_get_allocated_height (
          GTK_WIDGET (PIANO_ROLL_LABELS)));
      midi_arranger_widget_refresh_children (
        midi_arranger);
    }
  else if (timeline)
    {
      timeline_arranger_widget_set_size ();
      timeline_arranger_widget_refresh_children (
        timeline);
    }
  else if (midi_modifier_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (MIDI_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        rw_prv->total_px,
        -1);
      midi_modifier_arranger_widget_refresh_children (
        midi_modifier_arranger);
    }

  if (ar_prv->bg)
    arranger_bg_widget_refresh (ar_prv->bg);
}

static void
arranger_widget_class_init (ArrangerWidgetClass * _klass)
{
}

static void
arranger_widget_init (ArrangerWidget *self)
{
  GET_PRIVATE;

  ar_prv->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));
  ar_prv->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  ar_prv->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (ar_prv->right_mouse_mp),
                        GDK_BUTTON_SECONDARY);
}
