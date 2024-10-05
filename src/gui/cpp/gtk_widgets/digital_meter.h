// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Digital meter used for displaying Position,
 * BPM, etc.
 */

#ifndef __GUI_WIDGETS_DIGITAL_METER_H__
#define __GUI_WIDGETS_DIGITAL_METER_H__

#include "common/dsp/transport.h"
#include "common/utils/types.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define DIGITAL_METER_WIDGET_TYPE (digital_meter_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  DigitalMeterWidget,
  digital_meter_widget,
  Z,
  DIGITAL_METER_WIDGET,
  GtkWidget)

enum class NoteLength;
enum class NoteType;
class Position;

/**
 * @addtogroup widgets
 *
 * @{
 */

enum class DigitalMeterType
{
  DIGITAL_METER_TYPE_BPM,
  DIGITAL_METER_TYPE_POSITION,
  DIGITAL_METER_TYPE_TIMESIG,
  DIGITAL_METER_TYPE_NOTE_TYPE,
  DIGITAL_METER_TYPE_NOTE_LENGTH,
};

class SnapGrid;

typedef struct _DigitalMeterWidget
{
  GtkWidget parent_instance;

  DigitalMeterType type;

  bool is_transport;

  GtkGestureDrag * drag;
  double           last_y;
  double           last_x;
  int              height_start_pos;
  int              height_end_pos;

  /* ========= BPM ========= */
  /* for BPM */
  int num_part_start_pos;
  int num_part_end_pos;
  int dec_part_start_pos;
  int dec_part_end_pos;

  /** Used when changing the BPM. */
  bpm_t bpm_at_start;

  /** Used during update. */
  bpm_t last_set_bpm;

  /** Flag to update BPM. */
  bool update_num;
  /** Flag to update BPM decimal. */
  bool update_dec;

  /* ========= BPM end ========= */

  /* ========= position ========= */

  int bars_start_pos;
  int bars_end_pos;
  int beats_start_pos;
  int beats_end_pos;
  int sixteenths_start_pos;
  int sixteenths_end_pos;
  int ticks_start_pos;
  int ticks_end_pos;

  /** Update flags. */
  int update_bars;
  int update_beats;
  int update_sixteenths;
  int update_ticks;

  /* ========= position end ========= */

  /* ========= time ========= */

  /** For time. */
  int minutes_start_pos;
  int minutes_end_pos;
  int seconds_start_pos;
  int seconds_end_pos;
  int ms_start_pos;
  int ms_end_pos;

  /** Update flags. */
  int update_minutes;
  int update_seconds;
  int update_ms;

  /* ========= time end ========= */

  /* for note length/type */
  NoteLength * note_length;
  NoteType *   note_type;
  int          update_note_length; ///< flag to update note length
  int          start_note_length;  ///< start note length
  int          update_note_type;   ///< flag to update note type
  int          start_note_type;    ///< start note type

  /* for time sig */
  int update_timesig_top;
  /* ebeat unit */
  int update_timesig_bot;

  /** Used when changing the time signature. */
  int beats_per_bar_at_start;
  int beat_unit_at_start;

  /* ---------- FOR POSITION ---------------- */
  void * obj;

  /** Getter for Position. */
  void (*getter) (void *, Position *);
  /** Setter for Position. */
  void (*setter) (void *, Position *);
  /** Function to call on drag begin. */
  void (*on_drag_begin) (void *);
  /** Function to call on drag end. */
  void (*on_drag_end) (void *);

  /* ----------- position end --------------- */

  double hover_x;
  double hover_y;

  /** Draw line above the meter or not. */
  int draw_line;

  /** Caption to show above, NULL to not show. */
  char * caption;

  /** Cached layouts for drawing text. */
  PangoLayout * caption_layout;
  PangoLayout * seg7_layout;
  PangoLayout * normal_layout;

  bool initialized;

  GtkPopoverMenu * popover_menu;

  /**
   * Last time a scroll event was received.
   *
   * Used to check if an action should be performed.
   */
  gint64 last_scroll_time;

  bool scroll_started;
} DigitalMeterWidget;

/**
 * Creates a digital meter with the given type (
 * bpm or position).
 */
DigitalMeterWidget *
digital_meter_widget_new (
  DigitalMeterType type,
  NoteLength *     note_length,
  NoteType *       note_type,
  const char *     caption);

#define digital_meter_widget_new_for_position( \
  obj, drag_begin, getter, setter, drag_end, caption) \
  _digital_meter_widget_new_for_position ( \
    (void *) obj, (void (*) (void *)) drag_begin, \
    (void (*) (void *, Position *)) getter, \
    (void (*) (void *, Position *)) setter, (void (*) (void *)) drag_end, \
    caption)

/**
 * Creates a digital meter for an arbitrary position.
 *
 * @param obj The object to call the get/setters with.
 *
 *   E.g. Region.
 * @param get_val The getter func to get the position,
 *   passing the obj and the position to save to.
 * @param set_val The setter function to set the
 *   position.
 * @param drag_begin Function to call when
 *   starting the action.
 * @parram drag_end Function to call when ending
 *   the action.
 */
DigitalMeterWidget *
_digital_meter_widget_new_for_position (
  void * obj,
  void (*drag_begin) (void *),
  void (*get_val) (void *, Position *),
  void (*set_val) (void *, Position *),
  void (*drag_end) (void *),
  const char * caption);

void
digital_meter_set_draw_line (DigitalMeterWidget * self, int draw_line);

/**
 * Shows the widgets popover menu with the provided content
 *
 * @param menu content of the popover menu
 */
void
digital_meter_show_context_menu (DigitalMeterWidget * self, GMenu * menu);

/**
 * @}
 */

#endif
