// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Custom slider widget.
 */

#ifndef __GUI_WIDGETS_BAR_SLIDER_H__
#define __GUI_WIDGETS_BAR_SLIDER_H__

#include "common/utils/types.h"
#include "common/utils/ui.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define BAR_SLIDER_WIDGET_TYPE (bar_slider_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BarSliderWidget,
  bar_slider_widget,
  Z,
  BAR_SLIDER_WIDGET,
  GtkWidget)

class Port;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Type of slider.
 */
enum class BarSliderType
{
  BAR_SLIDER_TYPE_NORMAL,

  /** Port connection multiplier. */
  BAR_SLIDER_TYPE_PORT_MULTIPLIER,

  BAR_SLIDER_TYPE_CONTROL_PORT,
};

/**
 * Draggable slider to adjust an amount (such as a
 * percentage).
 *
 * It displays the value in the background as a
 * progress bar.
 */
typedef struct _BarSliderWidget
{
  GtkWidget parent_instance;

  /** Number of decimal points to show. */
  int decimals;

  /** The suffix to show after the value (eg "%" for
   * percentages). */
  char suffix[600];

  /** The prefix to show before the value. */
  char prefix[600];

  /** Maximum value. */
  float max;

  /** Minimum value. */
  float min;

  /** Zero point. */
  float zero;

  /** Float getter. */
  GenericFloatGetter getter;

  /** Float getter for snapped values (optional). */
  GenericFloatGetter snapped_getter;

  /** Float setter. */
  GenericFloatSetter setter;

  /** Float setter for drag begin. */
  GenericFloatSetter init_setter;

  /** Float setter for drag end. */
  GenericFloatSetter end_setter;

  /** Port, if control port. */
  Port * port;

  /** Widget width. */
  int width;

  /** Widget height. */
  int height;

  /** Object to call get/set with. */
  void * object;

  /** Used when dragging. */
  GtkGestureDrag * drag;

  /** Used in gesture drag. */
  double last_x;

  /** Used in gesture drag. */
  double start_x;

  /** Update mode. */
  UiDragMode mode;

  /** Whether hovering or not. */
  int hover;

  /** The type of slider. */
  BarSliderType type;

  /** Whether to show the value in text or just the
   * prefix + suffix. */
  int show_value;

  /** Whether the user can change the value. */
  int editable;

  /** Multiply the value by 100 when showing it. */
  int convert_to_percentage;

  /** Cache layout. */
  PangoLayout * layout;

  /**
   * Cache of text extents.
   */
  float last_real_val;
  int   last_width_extent;
  int   last_height_extent;
  char  last_extent_str[3000];

} BarSliderWidget;

/**
 * Creates a bar slider widget for floats.
 *
 * @param dest Port destination, if this is a port
 *   to port connection slider.
 * @param convert_to_percentage Multiply the value
 *   by 100 when showing it.
 */
BarSliderWidget *
_bar_slider_widget_new (
  BarSliderType      type,
  GenericFloatGetter get_val,
  GenericFloatSetter set_val,
  void *             object,
  float              min,
  float              max,
  int                w,
  int                h,
  float              zero,
  int                convert_to_percentage,
  int                decimals,
  UiDragMode         mode,
  const char *       prefix,
  const char *       suffix);

/**
 * Helper to create a bar slider widget.
 */
#define bar_slider_widget_new( \
  getter, setter, obj, min, max, w, h, zero, dec, mode, suffix) \
  _bar_slider_widget_new ( \
    BarSliderType::BAR_SLIDER_TYPE_NORMAL, getter, setter, (void *) obj, min, \
    max, w, h, zero, 0, dec, mode, "", suffix)

/**
 * Wrapper.
 *
 * @param conn PortConnection pointer.
 */
#define bar_slider_widget_new_port_connection(conn, prefix) \
  _bar_slider_widget_new ( \
    BarSliderType::BAR_SLIDER_TYPE_PORT_MULTIPLIER, nullptr, nullptr, \
    (void *) conn, 0.f, 1.f, 160, 20, 0.f, 1, 0, \
    UiDragMode::UI_DRAG_MODE_CURSOR, prefix, " %")

/**
 * @}
 */

#endif
