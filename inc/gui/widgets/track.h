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

/**
 * \file
 *
 * Track widget to be shown in the tracklist.
 */

#ifndef __GUI_WIDGETS_TRACK_H__
#define __GUI_WIDGETS_TRACK_H__

#include <stdbool.h>

#include <gtk/gtk.h>

#define TRACK_WIDGET_TYPE (track_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackWidget,
  track_widget,
  Z,
  TRACK_WIDGET,
  GtkWidget)

typedef struct _ArrangerWidget      ArrangerWidget;
typedef struct AutomationModeWidget AutomationModeWidget;
typedef struct CustomButtonWidget   CustomButtonWidget;
typedef struct _MeterWidget         MeterWidget;
typedef struct Track                Track;
typedef struct _TrackCanvasWidget   TrackCanvasWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define TRACK_BUTTON_SIZE 18

/** Padding between each button. */
#define TRACK_BUTTON_PADDING 6

/** Padding between the track edges and the
 * buttons */
#define TRACK_BUTTON_PADDING_FROM_EDGE 3

#define TRACK_BOT_BUTTONS_SHOULD_BE_VISIBLE(height) \
  (height \
   >= (TRACK_BUTTON_SIZE + TRACK_BUTTON_PADDING_FROM_EDGE) * 2 \
        + TRACK_BUTTON_PADDING)

#define TRACK_COLOR_AREA_WIDTH 18

#define TRACK_ICON_NAME_MONO_COMPAT "mono"
#define TRACK_ICON_NAME_RECORD "media-record"
#define TRACK_ICON_NAME_SOLO "solo"
#define TRACK_ICON_NAME_MUTE "mute"
#define TRACK_ICON_NAME_LISTEN "listen"
#define TRACK_ICON_NAME_SHOW_UI "jam-icons-screen"
#define TRACK_ICON_NAME_SHOW_AUTOMATION_LANES "node-type-cusp"
#define TRACK_ICON_NAME_SHOW_TRACK_LANES "format-justify-fill"
#define TRACK_ICON_NAME_LOCK "document-decrypt"
#define TRACK_ICON_NAME_FREEZE "fork-awesome-snowflake-o"
#define TRACK_ICON_NAME_PLUS "add"
#define TRACK_ICON_NAME_MINUS "remove"
#define TRACK_ICON_NAME_BUS "effect"
#define TRACK_ICON_NAME_CHORDS "minuet-chords"
#define TRACK_ICON_NAME_SHOW_MARKERS "kdenlive-show-markers"
#define TRACK_ICON_NAME_MIDI "instrument"
#define TRACK_ICON_NAME_TEMPO "filename-bpm-amarok"
#define TRACK_ICON_NAME_MODULATOR "modulator"
#define TRACK_ICON_NAME_FOLD "fluentui-folder-regular"
#define TRACK_ICON_NAME_FOLD_OPEN \
  "fluentui-folder-open-regular"
#define TRACK_ICON_NAME_MONITOR_AUDIO "audition"

#define TRACK_ICON_IS(x, name) \
  (string_is_equal (x, TRACK_ICON_NAME_##name))

#define TRACK_CB_ICON_IS(name) \
  TRACK_ICON_IS (cb->icon_name, name)

/**
 * Highlight location.
 */
typedef enum TrackWidgetHighlight
{
  TRACK_WIDGET_HIGHLIGHT_TOP,
  TRACK_WIDGET_HIGHLIGHT_BOTTOM,
  TRACK_WIDGET_HIGHLIGHT_INSIDE,
} TrackWidgetHighlight;

/**
 * Resize target.
 */
typedef enum TrackWidgetResizeTarget
{
  TRACK_WIDGET_RESIZE_TARGET_TRACK,
  TRACK_WIDGET_RESIZE_TARGET_AT,
  TRACK_WIDGET_RESIZE_TARGET_LANE,
} TrackWidgetResizeTarget;

/**
 * The TrackWidget is split into 3 parts.
 *
 * - 1. Top part contains the "main" view.
 * - 2. Lane part contains each lane.
 * - 3. Automation tracklist part contains each
 *      automation track.
 */
typedef struct _TrackWidget
{
  GtkWidget parent_instance;

  /** Main box containing the drawing area and the
   * meters on the right. */
  GtkBox * main_box;

  /** Group colors. */
  GtkBox * group_colors_box;

  GtkGestureDrag *  drag;
  GtkGestureClick * click;

  /** Right-click gesture. */
  GtkGestureClick * right_click;

  /** If drag update was called at least once. */
  int dragged;

  /** Number of clicks, used when selecting/moving/
   * dragging channels. */
  int n_press;

  /**
   * Set between enter-leave signals.
   *
   * This is because hover can continue to send
   * signals when
   * hovering over other overlayed widgets (buttons,
   * etc.).
   */
  bool bg_hovered;

  /**
   * Whether color area is currently hoverred.
   *
   * This is not mutually exclusive with
   * \ref TrackWidget.bg_hovered. The color area
   * is considered part of the BG.
   */
  bool color_area_hovered;

  /**
   * Whether the icon in the color area is
   * currently hoverred.
   *
   * This is not mutually exclusive with
   * \ref TrackWidget.color_area_hovered. The
   * icon is considered part of the color area.
   */
  bool icon_hovered;

  /**
   * Set when the drag should resize instead of dnd.
   *
   * This is used to determine if we should resize
   * on drag begin.
   */
  int resize;

  /** Set during the whole resizing action. */
  int resizing;

  /** Resize target type (track/at/lane). */
  TrackWidgetResizeTarget resize_target_type;

  /** The object to resize. */
  void * resize_target;

  /** Associated Track. */
  Track * track;

  /** Control held down on drag begin. */
  int ctrl_held_at_start;

  /** Used for highlighting. */
  GtkBox * highlight_top_box;
  GtkBox * highlight_bot_box;

  /** Whether to highlight inside the track (eg,
   * when dragging inside foldable tracks). */
  bool highlight_inside;

  /** The track selection processing was done in
   * the dnd callbacks, so no need to do it in
   * drag_end. */
  int selected_in_dnd;

  /** For drag actions. */
  double start_x;
  double start_y;
  double last_offset_y;

  /** Used during hovering to remember the last
   * known cursor position. */
  double last_x;
  double last_y;

  /** Used when mouse button is held down to
   * mark buttons as clicked. */
  int button_pressed;

  /** Currently clicked button. */
  CustomButtonWidget * clicked_button;

  /** Currently clicked automation mode button. */
  AutomationModeWidget * clicked_am;

  TrackCanvasWidget * canvas;

  /**
   * Signal handler IDs for tracks that have them.
   *
   * This is more convenient instead of having them
   * in each widget.
   */
  //gulong              record_toggle_handler_id;
  //gulong              solo_toggled_handler_id;
  //gulong              mute_toggled_handler_id;

  /** Buttons to be drawin in order. */
  CustomButtonWidget * top_buttons[8];
  int                  num_top_buttons;
  CustomButtonWidget * bot_buttons[8];
  int                  num_bot_buttons;

  MeterWidget * meter_l;
  MeterWidget * meter_r;

  /**
   * Current tooltip text.
   */
  char * tooltip_text;

  /** Last MIDI event trigger time, for MIDI
   * ports. */
  gint64 last_midi_out_trigger_time;

  /** Set to 1 to redraw. */
  int redraw;

  /** Whether the track was armed for recording
   * at the start of the current action. */
  bool was_armed;

  /** Cairo caches. */
  cairo_t *         cached_cr;
  cairo_surface_t * cached_surface;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;

  /** Popover for changing the track name. */
  GtkPopover * track_name_popover;
} TrackWidget;

const char *
track_widget_highlight_to_str (TrackWidgetHighlight highlight);

/**
 * Sets up the track widget.
 *
 * Sets color, draw callback, etc.
 */
TrackWidget *
track_widget_new (Track * track);

/**
 * Sets the Track name on the TrackWidget.
 */
void
track_widget_set_name (TrackWidget * self, const char * name);

/**
 * Callback when automation button is toggled.
 */
void
track_widget_on_show_automation_toggled (TrackWidget * self);

/**
 * Callback when automation button is toggled.
 */
void
track_widget_on_show_lanes_toggled (TrackWidget * self);

/**
 * Callback when record button is toggled.
 */
void
track_widget_on_record_toggled (TrackWidget * self);

/**
 * Returns if cursor is in the range select "half".
 *
 * Used by timeline to determine if it will select
 * objects or range.
 */
bool
track_widget_is_cursor_in_range_select_half (
  TrackWidget * self,
  double        y);

/**
 * Updates the track icons.
 */
void
track_widget_update_icons (TrackWidget * self);

/**
 * Updates the full track size and redraws the
 * track.
 */
void
track_widget_update_size (TrackWidget * self);

/**
 * Returns the highlight location based on y
 * relative to @ref self.
 */
TrackWidgetHighlight
track_widget_get_highlight_location (TrackWidget * self, int y);

/**
 * Highlights/unhighlights the Tracks
 * appropriately.
 *
 * @param highlight 1 to highlight top or bottom,
 *   0 to unhighlight all.
 */
void
track_widget_do_highlight (
  TrackWidget * self,
  gint          x,
  gint          y,
  const int     highlight);

/**
 * Converts Y from the arranger coordinates to
 * the track coordinates.
 */
int
track_widget_get_local_y (
  TrackWidget *    self,
  ArrangerWidget * arranger,
  int              arranger_y);

/**
 * Causes a redraw of the meters only.
 */
void
track_widget_redraw_meters (TrackWidget * self);

/**
 * Re-fills TrackWidget.group_colors_box.
 */
void
track_widget_recreate_group_colors (TrackWidget * self);

CustomButtonWidget *
track_widget_get_hovered_button (
  TrackWidget * self,
  int           x,
  int           y);

AutomationModeWidget *
track_widget_get_hovered_am_widget (
  TrackWidget * self,
  int           x,
  int           y);

/**
 * @}
 */

#endif
