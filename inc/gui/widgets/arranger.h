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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_ARRANGER_H__
#define __GUI_WIDGETS_ARRANGER_H__

#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll.h"
#include "audio/position.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define ARRANGER_WIDGET_TYPE ( \
  arranger_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (ArrangerWidget,
                          arranger_widget,
                          Z,
                          ARRANGER_WIDGET,
                          GtkOverlay)

/** Object selection macros. */
#define ARRANGER_WIDGET_SELECT_MIDI_NOTE( \
  self, child, select, append) \
  arranger_widget_select ( \
    Z_ARRANGER_WIDGET (self), \
    MIDI_NOTE_WIDGET_TYPE, \
    (void *) child, select, append);
#define ARRANGER_WIDGET_SELECT_REGION( \
  self, child, select, append) \
  arranger_widget_select ( \
    Z_ARRANGER_WIDGET (self), \
    REGION_WIDGET_TYPE, \
    (void *) child, select, append);
#define ARRANGER_WIDGET_SELECT_CHORD( \
  self, child, select, append) \
  arranger_widget_select ( \
    Z_ARRANGER_WIDGET (self), \
    CHORD_WIDGET_TYPE, (void *) child, select, append);
#define ARRANGER_WIDGET_SELECT_AUTOMATION_POINT( \
  self, child, select, append) \
  arranger_widget_select ( \
    Z_ARRANGER_WIDGET (self), \
    AUTOMATION_POINT_WIDGET_TYPE, \
    (void *) child, select, append);

#define ARRANGER_WIDGET_GET_PRIVATE(self) \
  ArrangerWidgetPrivate * ar_prv = \
    arranger_widget_get_private (Z_ARRANGER_WIDGET (self));
#define ARRANGER_IS_TIMELINE(self) \
  (Z_IS_TIMELINE_ARRANGER_WIDGET (self))
#define ARRANGER_IS_MIDI(self) \
  (Z_IS_MIDI_ARRANGER_WIDGET (self))
#define ARRANGER_IS_MIDI_MODIFIER(self) \
  (Z_IS_MIDI_MODIFIER_ARRANGER_WIDGET (self))

/**
 * Updates the visibility of the transient/non-
 * transient objects based on the current action.
 *
 * E.g. when moving or resizing, it hides the original
 * objects and only shows the transients. When copy-
 * moving, it shows both.
 *
 * @param cc CamelCase.
 * @param lc lower_case.
 */
#define ARRANGER_SET_OBJ_VISIBILITY( \
  array, size,cc,lc) \
  cc * lc; \
  for (int i = 0; i < size; i++) \
    { \
      lc = array[i]; \
      if (ar_prv->action == \
            UI_OVERLAY_ACTION_MOVING) \
        { \
          _trans_visible = 1; \
          _non_trans_visible = 0; \
        } \
      else if (ar_prv->action == \
                 UI_OVERLAY_ACTION_MOVING_COPY || \
               ar_prv->action == \
                 UI_OVERLAY_ACTION_MOVING_LINK) \
        { \
          _trans_visible = 1; \
          _non_trans_visible = 1; \
        } \
      else if (ar_prv->action == \
                UI_OVERLAY_ACTION_RESIZING_L || \
              ar_prv->action == \
                UI_OVERLAY_ACTION_RESIZING_R || \
              ar_prv->action == \
                UI_OVERLAY_ACTION_CREATING_RESIZING_R) \
        { \
          _trans_visible = 1; \
          _non_trans_visible = 0; \
        } \
      else \
        { \
          _trans_visible = 0; \
          _non_trans_visible = 1; \
        } \
      _lane_visible = \
        lc##_get_track (lc)->lanes_visible; \
      if (_lane_visible) \
        { \
          gtk_widget_set_visible ( \
            GTK_WIDGET ( \
              ((cc *) lc->obj_info.lane_trans)->widget), \
            _trans_visible); \
          gtk_widget_set_visible ( \
            GTK_WIDGET ( \
              ((cc *) lc->obj_info.lane)->widget), \
            _non_trans_visible); \
        } \
      else \
        { \
          gtk_widget_set_visible ( \
            GTK_WIDGET ( \
              ((cc *) lc->obj_info.lane_trans)->widget), 0); \
          gtk_widget_set_visible ( \
            GTK_WIDGET ( \
              ((cc *) lc->obj_info.lane)->widget), 0); \
        } \
      gtk_widget_set_visible ( \
        GTK_WIDGET ( \
          ((cc *) lc->obj_info.main_trans)->widget), \
        _trans_visible); \
      gtk_widget_set_visible ( \
        GTK_WIDGET ( \
          ((cc *) lc->obj_info.main)->widget), \
        _non_trans_visible); \
    }

/**
 * Moves an object with length (Region, MidiNote,
 * etc.) by the given ticks_diff.
 */
#define ARRANGER_MOVE_OBJ_BY_TICKS_W_LENGTH( \
  obj, obj_name, prev_start_pos, ticks_diff, \
  tmp_pos, tmp_length_ticks) \
  /*g_message ("1 endpos ticks %ld", obj->end_pos.total_ticks);*/ \
  tmp_length_ticks = \
    obj->end_pos.total_ticks - \
    obj->start_pos.total_ticks; \
  position_set_to_pos ((tmp_pos), \
                       (prev_start_pos)); \
  /*g_message ("---length ticks %ld, ticks diff %ld", \
             tmp_length_ticks, ticks_diff);*/ \
  position_add_ticks ( \
    (tmp_pos), \
    ticks_diff + tmp_length_ticks); \
  /*g_message ("updated pos ticks %ld", (tmp_pos)->total_ticks); \
  position_print (tmp_pos);*/ \
  obj_name##_set_end_pos (obj, (tmp_pos)); \
  /*g_message ("endpos ticks after setting %ld", obj->end_pos.total_ticks);*/ \
  position_set_to_pos ((tmp_pos), \
                       (prev_start_pos)); \
  position_add_ticks ((tmp_pos), \
                      ticks_diff); \
  obj_name##_set_start_pos (obj, (tmp_pos)); \
  /*g_message ("startpos ticks after setting %ld", obj->start_pos.total_ticks); \
   g_message ("99 set start pos to "); \
     position_print (&obj->start_pos) */;

/**
 * Moves an object without length (AutomationPoint,
 * Chord, etc.) by the given ticks_diff.
 */
#define ARRANGER_MOVE_OBJ_BY_TICKS( \
  obj, obj_name, prev_start_pos, ticks_diff, \
  tmp_pos) \
  position_set_to_pos ((tmp_pos), \
                       (prev_start_pos)); \
  position_add_ticks ((tmp_pos), \
                      ticks_diff); \
  obj_name##_set_pos (obj, (tmp_pos));


typedef struct _ArrangerBgWidget ArrangerBgWidget;
typedef struct MidiNote MidiNote;
typedef struct SnapGrid SnapGrid;
typedef struct AutomationPoint AutomationPoint;
typedef struct _ArrangerPlayheadWidget ArrangerPlayheadWidget;

typedef struct _ArrangerBgWidget ArrangerBgWidget;

typedef enum ArrangerCursor
{
  /** Invalid cursor. */
  ARRANGER_CURSOR_NONE,
  ARRANGER_CURSOR_SELECT,
  ARRANGER_CURSOR_EDIT,
  ARRANGER_CURSOR_ERASER,
  ARRANGER_CURSOR_AUDITION,
  ARRANGER_CURSOR_RAMP,
  ARRANGER_CURSOR_GRAB,
  ARRANGER_CURSOR_GRABBING,
  ARRANGER_CURSOR_RESIZING_R,
  ARRANGER_CURSOR_RESIZING_L,
  ARRANGER_CURSOR_GRABBING_COPY,
  ARRANGER_CURSOR_GRABBING_LINK,
  ARRANGER_CURSOR_RANGE,
} ArrangerCursor;

typedef struct
{
  ArrangerBgWidget *       bg;
  ArrangerPlayheadWidget * playhead;
  GtkGestureDrag *         drag;
  GtkGestureMultiPress *   multipress;
  GtkGestureMultiPress *   right_mouse_mp;
  GtkEventController *     motion_controller;
  double                   last_offset_x;  ///< for dragging regions, selections
  double                   last_offset_y;  ///< for selections
  UiOverlayAction          action;
  double                   start_x; ///< for dragging
  double                   start_y; ///< for dragging

  double                   start_pos_px; ///< for moving regions

  Position                 start_pos; ///< useful for moving
  Position                 end_pos; ///< for moving regions
   gboolean                 key_is_pressed;

  /** Current hovering positions. */
  double                   hover_x;
  double                   hover_y;
  /* end */

  int                      n_press; ///< for multipress
  SnapGrid *               snap_grid; ///< associated snap grid
  /**
   * If shift was held down during the press.
   */
  int                      shift_held;

  /**
   * Ctrl button held.
   */
  int                      ctrl_held;

  gint64                   last_frame_time;
} ArrangerWidgetPrivate;

typedef struct _ArrangerWidgetClass
{
  GtkOverlayClass       parent_class;
} ArrangerWidgetClass;

/**
 * Creates a timeline widget using the given timeline data.
 */
void
arranger_widget_setup (ArrangerWidget *   self,
                       SnapGrid *         snap_grid);

ArrangerWidgetPrivate *
arranger_widget_get_private (ArrangerWidget * self);

/**
 * Sets the cursor on the arranger and all of its
 * children.
 */
void
arranger_widget_set_cursor (
  ArrangerWidget * self,
  ArrangerCursor   cursor);

int
arranger_widget_pos_to_px (
  ArrangerWidget * self,
  Position * pos,
  int        use_padding);

void
arranger_widget_refresh_cursor (
  ArrangerWidget * self);

/**
 * Gets the corresponding scrolled window.
 */
GtkScrolledWindow *
arranger_widget_get_scrolled_window (
  ArrangerWidget * self);

/**
 * Wrapper for ui_px_to_pos depending on the arranger
 * type.
 */
void
arranger_widget_px_to_pos (
  ArrangerWidget * self,
  double           px,
  Position *       pos,
  int              has_padding);

/**
 * Refreshes all arranger backgrounds.
 */
void
arranger_widget_refresh_all_backgrounds ();

void
arranger_widget_get_hit_widgets_in_range (
  ArrangerWidget *  self,
  GType             type,
  double            start_x,
  double            start_y,
  double            offset_x,
  double            offset_y,
  GtkWidget **      array, ///< array to fill
  int *             array_size); ///< array_size to fill

void
arranger_widget_select_all (
  ArrangerWidget *  self,
  int               select);

/**
 * Returns if the arranger is in a moving-related
 * operation or starting a moving-related operation.
 *
 * Useful to know if we need transient widgets or
 * not.
 */
int
arranger_widget_is_in_moving_operation (
  ArrangerWidget * self);

/**
 * Selects the object, optionally appending it to
 * the selected items or making it the only
 * selected item.
 */
void
arranger_widget_select (
  ArrangerWidget * self,
  GType            type,
  void *           child,
  int              select,
  int              append);

/**
 * Readd children.
 */
int
arranger_widget_refresh (
  ArrangerWidget * self);

#endif
