// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_COLOR_AREA_H__
#define __GUI_WIDGETS_COLOR_AREA_H__

/**
 * @file
 *
 * Color picker for a channel strip.
 */

#include <string>

#include "common/utils/color.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define COLOR_AREA_WIDGET_TYPE (color_area_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ColorAreaWidget,
  color_area_widget,
  Z,
  COLOR_AREA_WIDGET,
  GtkWidget)

/**
 * Type of ColorAreaWidget this is. */
enum class ColorAreaType
{
  /** Generic, only fill with color. */
  COLOR_AREA_TYPE_GENERIC,

  /**
   * Track, for use in TrackWidget implementations.
   *
   * It will show an icon and an index inside the
   * color box.
   */
  COLOR_AREA_TYPE_TRACK,
};

class Track;

using ColorAreaWidget = struct _ColorAreaWidget
{
  GtkWidget parent_instance;

  /** Color pointer to set/read value. */
  Color color;

  /** The type. */
  ColorAreaType type;

  /** Track, if track. */
  Track * track;

  bool hovered;

  /** Used during drawing. */
  // GPtrArray * parents;

  GdkTexture * track_icon;
  std::string  last_track_icon_name;
};

/**
 * Creates a generic color widget using the given color pointer.
 *
 * FIXME currently not used, should be used instead of manually changing the
 * color.
 */
void
color_area_widget_setup_generic (ColorAreaWidget * self, Color * color);

/**
 * Creates a ColorAreaWidget for use inside TrackWidget implementations.
 */
void
color_area_widget_setup_track (ColorAreaWidget * self, Track * track);

/**
 * Changes the color.
 *
 * Track types don't need to do this since the color is read directly from the
 * Track.
 */
void
color_area_widget_set_color (ColorAreaWidget * widget, Color color);

#endif
