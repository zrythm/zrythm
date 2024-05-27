/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "actions/arranger_selections.h"
#include "dsp/position.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/selection_info.h"
#include "gui/widgets/timeline_selection_info.h"
#include "project.h"
#include "utils/flags.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  TimelineSelectionInfoWidget,
  timeline_selection_info_widget,
  GTK_TYPE_STACK)

DEFINE_START_POS;

/** Earliest start position on drag start. */
static Position earliest_start_pos;

/** Clone to fill in during drag_start. */
static TimelineSelections * tl_clone;

static void
on_drag_begin_w_object ()
{
  tl_clone = timeline_selections_clone (TL_SELECTIONS);
}

#define DEFINE_DRAG_END_POS_SET(caps, cc, sc, pos_name_caps, pos_name) \
  static void on_drag_end_##sc##_##pos_name (cc * sc) \
  { \
    Position new_pos; \
    position_set_to_pos (&new_pos, &sc->pos_name); \
    /* set the actual pos back to the prev pos */ \
    position_set_to_pos ( \
      &TL_SELECTIONS->sc##s[0]->pos_name, &tl_clone->sc##s[0]->pos_name); \
    timeline_selections_free (tl_clone); \
    if (position_is_equal (&TL_SELECTIONS->sc##s[0]->pos_name, &new_pos)) \
      return; \
    UndoableAction * ua = \
      (UndoableAction *) edit_timeline_selections_action_new ( \
        TL_SELECTIONS, ETS_##caps##_##pos_name_caps, 0, NULL, &new_pos); \
    undo_manager_perform (UNDO_MANAGER, ua); \
  }

DEFINE_DRAG_END_POS_SET (REGION, Region, region, CLIP_START_POS, clip_start_pos);
DEFINE_DRAG_END_POS_SET (REGION, Region, region, LOOP_START_POS, loop_start_pos);
DEFINE_DRAG_END_POS_SET (REGION, Region, region, LOOP_END_POS, loop_end_pos);

#define DEFINE_DRAG_END_POS_RESIZE_L(cc, sc) \
  static void on_drag_end_##sc##_resize_l (cc * sc) \
  { \
    Position new_pos; \
    position_set_to_pos (&new_pos, &sc->start_pos); \
    /* set the actual pos back to the prev pos */ \
    position_set_to_pos ( \
      &TL_SELECTIONS->sc##s[0]->start_pos, &tl_clone->sc##s[0]->start_pos); \
    timeline_selections_free (tl_clone); \
    if (position_is_equal (&TL_SELECTIONS->sc##s[0]->start_pos, &new_pos)) \
      return; \
    long ticks_diff = \
      position_to_ticks (&new_pos) \
      - position_to_ticks (&TL_SELECTIONS->sc##s[0]->start_pos); \
    UndoableAction * ua = \
      (UndoableAction *) edit_timeline_selections_action_new ( \
        TL_SELECTIONS, ETS_RESIZE_L, ticks_diff, NULL, NULL); \
    undo_manager_perform (UNDO_MANAGER, ua); \
  }

DEFINE_DRAG_END_POS_RESIZE_L (Region, region);

#define DEFINE_DRAG_END_POS_RESIZE_R(cc, sc) \
  static void on_drag_end_##sc##_resize_r (cc * sc) \
  { \
    Position new_pos; \
    position_set_to_pos (&new_pos, &sc->end_pos); \
    /* set the actual pos back to the prev pos */ \
    position_set_to_pos ( \
      &TL_SELECTIONS->sc##s[0]->end_pos, &tl_clone->sc##s[0]->end_pos); \
    timeline_selections_free (tl_clone); \
    if (position_is_equal (&TL_SELECTIONS->sc##s[0]->end_pos, &new_pos)) \
      return; \
    long ticks_diff = \
      position_to_ticks (&new_pos) \
      - position_to_ticks (&TL_SELECTIONS->sc##s[0]->end_pos); \
    UndoableAction * ua = \
      (UndoableAction *) edit_timeline_selections_action_new ( \
        TL_SELECTIONS, ETS_RESIZE_R, ticks_diff, NULL, NULL); \
    undo_manager_perform (UNDO_MANAGER, ua); \
  }

DEFINE_DRAG_END_POS_RESIZE_R (Region, region);

static void
on_drag_begin ()
{
  timeline_selections_set_cache_poses (TL_SELECTIONS);
  timeline_selections_get_start_pos (
    TL_SELECTIONS, &earliest_start_pos, F_NO_TRANSIENTS);
}

static void
on_drag_end ()
{
  Position start_pos;
  timeline_selections_get_start_pos (TL_SELECTIONS, &start_pos, F_NO_TRANSIENTS);
  long ticks_diff =
    position_to_ticks (&start_pos) - position_to_ticks (&earliest_start_pos);
  /* remove the diff since it will get added
   * in the moving action */
  timeline_selections_add_ticks (TL_SELECTIONS, -ticks_diff, 0, AO_UPDATE_ALL);
  UndoableAction * ua = (UndoableAction *) move_timeline_selections_action_new (
    TL_SELECTIONS, ticks_diff,
    timeline_selections_get_first_track (TL_SELECTIONS, F_TRANSIENTS)->pos
      - timeline_selections_get_first_track (TL_SELECTIONS, F_NO_TRANSIENTS)->pos,
    0);
  undo_manager_perform (UNDO_MANAGER, ua);
}

static void
get_pos (const Position * obj, Position * pos)
{
  position_set_to_pos (pos, obj);
}

static void
set_pos (Position * obj, const Position * pos)
{
  if (position_is_after_or_equal (pos, START_POS))
    {
      timeline_selections_add_ticks (
        TL_SELECTIONS, position_to_ticks (pos) - position_to_ticks (obj), 0,
        AO_UPDATE_ALL);
      position_set_to_pos (obj, pos);
    }
}

void
timeline_selection_info_widget_refresh (
  TimelineSelectionInfoWidget * self,
  TimelineSelections *          ts)
{
  GtkWidget * fo = timeline_selections_get_first_object (ts, 0);
  GtkWidget * lo = timeline_selections_get_last_object (ts, 0);
  int         only_object = fo == lo;

  selection_info_widget_clear (self->selection_info);
  gtk_stack_set_visible_child (
    GTK_STACK (self), GTK_WIDGET (self->no_selection_label));

#define ADD_WIDGET(widget) \
  selection_info_widget_add_info ( \
    self->selection_info, NULL, GTK_WIDGET (widget));

  /* if only object selected, show its specifics */
  if (only_object)
    {
      if (Z_IS_REGION_WIDGET (fo))
        {
          REGION_WIDGET_GET_PRIVATE (fo);
          Region * r = region_get_main_trans_region (rw_prv->region);

          DigitalMeterWidget * dm;
          dm = digital_meter_widget_new_for_position (
            r, on_drag_begin_w_object, region_get_start_pos,
            region_start_pos_setter, on_drag_end_region_resize_l, _ ("start"));
          digital_meter_set_draw_line (dm, 1);
          ADD_WIDGET (dm);
          dm = digital_meter_widget_new_for_position (
            r, on_drag_begin_w_object, region_get_end_pos,
            region_end_pos_setter, on_drag_end_region_resize_r, _ ("end"));
          digital_meter_set_draw_line (dm, 1);
          ADD_WIDGET (dm);
          dm = digital_meter_widget_new_for_position (
            r, on_drag_begin_w_object, region_get_clip_start_pos,
            region_clip_start_pos_setter, on_drag_end_region_clip_start_pos,
            _ ("clip start (rel.)"));
          digital_meter_set_draw_line (dm, 1);
          ADD_WIDGET (dm);
          dm = digital_meter_widget_new_for_position (
            r, on_drag_begin_w_object, region_get_loop_start_pos,
            region_loop_start_pos_setter, on_drag_end_region_loop_start_pos,
            _ ("loop start (rel.)"));
          digital_meter_set_draw_line (dm, 1);
          ADD_WIDGET (dm);
          dm = digital_meter_widget_new_for_position (
            r, on_drag_begin_w_object, region_get_loop_end_pos,
            region_loop_end_pos_setter, on_drag_end_region_loop_end_pos,
            _ ("loop end (rel.)"));
          digital_meter_set_draw_line (dm, 1);
          ADD_WIDGET (dm);

          gtk_stack_set_visible_child (
            GTK_STACK (self), GTK_WIDGET (self->selection_info));
        }
      else if (Z_IS_AUTOMATION_POINT_WIDGET (fo))
        {
        }
      else if (Z_IS_CHORD_OBJECT_WIDGET (fo))
        {
          ChordObjectWidget * mw = Z_CHORD_OBJECT_WIDGET (fo);
          ChordObject *       chord_object =
            chord_object_get_main_chord_object (mw->chord_object);

          DigitalMeterWidget * dm;
          dm = digital_meter_widget_new_for_position (
            chord_object, on_drag_begin, chord_object_get_pos,
            chord_object_pos_setter, on_drag_end, _ ("position"));
          digital_meter_set_draw_line (dm, 1);
          ADD_WIDGET (dm);

          gtk_stack_set_visible_child (
            GTK_STACK (self), GTK_WIDGET (self->selection_info));
        }
      else if (Z_IS_SCALE_OBJECT_WIDGET (fo))
        {
          ScaleObjectWidget * mw = Z_SCALE_OBJECT_WIDGET (fo);
          ScaleObject *       scale_object =
            scale_object_get_main_scale_object (mw->scale_object);

          DigitalMeterWidget * dm;
          dm = digital_meter_widget_new_for_position (
            scale_object, on_drag_begin, scale_object_get_pos,
            scale_object_pos_setter, on_drag_end, _ ("position"));
          digital_meter_set_draw_line (dm, 1);
          ADD_WIDGET (dm);

          gtk_stack_set_visible_child (
            GTK_STACK (self), GTK_WIDGET (self->selection_info));
        }
      else if (Z_IS_MARKER_WIDGET (fo))
        {
          MarkerWidget * mw = Z_MARKER_WIDGET (fo);
          Marker *       marker = marker_get_main_marker (mw->marker);

          DigitalMeterWidget * dm;
          dm = digital_meter_widget_new_for_position (
            marker, on_drag_begin, marker_get_pos, marker_pos_setter,
            on_drag_end, _ ("position"));
          digital_meter_set_draw_line (dm, 1);
          ADD_WIDGET (dm);

          gtk_stack_set_visible_child (
            GTK_STACK (self), GTK_WIDGET (self->selection_info));
        }
    } /* endif only_object */
  /* if > 1 objects selected, only show position
   * for moving */
  else
    {
      Position * pos = calloc (1, sizeof (Position));
      timeline_selections_get_start_pos (TL_SELECTIONS, pos, F_NO_TRANSIENTS);
      DigitalMeterWidget * dm;
      dm = digital_meter_widget_new_for_position (
        pos, on_drag_begin, get_pos, set_pos, on_drag_end, _ ("position"));
      digital_meter_set_draw_line (dm, 1);
      ADD_WIDGET (dm);

      gtk_stack_set_visible_child (
        GTK_STACK (self), GTK_WIDGET (self->selection_info));
    }
}

static void
timeline_selection_info_widget_class_init (
  TimelineSelectionInfoWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "timeline-selection-info");
}

static void
timeline_selection_info_widget_init (TimelineSelectionInfoWidget * self)
{
  self->no_selection_label =
    GTK_LABEL (gtk_label_new (_ ("No object selected")));
  gtk_widget_set_visible (GTK_WIDGET (self->no_selection_label), 1);
  self->selection_info = g_object_new (SELECTION_INFO_WIDGET_TYPE, NULL);
  gtk_widget_set_visible (GTK_WIDGET (self->selection_info), 1);

  gtk_stack_add_named (
    GTK_STACK (self), GTK_WIDGET (self->no_selection_label), "no-selection");
  gtk_stack_add_named (
    GTK_STACK (self), GTK_WIDGET (self->selection_info), "selection-info");

  gtk_widget_set_size_request (GTK_WIDGET (self), -1, 38);
}
