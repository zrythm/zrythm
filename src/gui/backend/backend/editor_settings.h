// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_EDITOR_SETTINGS_H__
#define __GUI_BACKEND_EDITOR_SETTINGS_H__

#include "utils/iserializable.h"
#include "utils/math.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

using namespace zrythm;

#define DEFINE_EDITOR_SETTINGS_QML_PROPERTIES \
  Q_PROPERTY (double x READ getX WRITE setX NOTIFY xChanged) \
  double getX () const \
  { \
    return scroll_start_x_; \
  } \
  void setX (double x) \
  { \
    auto clamped_x = clamp_scroll_start_x (x); \
    if (utils::math::floats_equal (scroll_start_x_, clamped_x)) \
      return; \
\
    scroll_start_x_ = clamped_x; \
    Q_EMIT xChanged (scroll_start_x_); \
  } \
\
  Q_SIGNAL void xChanged (double x); \
\
  Q_PROPERTY (double y READ getY WRITE setY NOTIFY yChanged) \
  double getY () const \
  { \
    return scroll_start_y_; \
  } \
  void setY (double y) \
  { \
    auto clamped_y = clamp_scroll_start_y (y); \
    if (utils::math::floats_equal (scroll_start_y_, clamped_y)) \
      return; \
    scroll_start_y_ = clamped_y; \
    Q_EMIT yChanged (scroll_start_y_); \
  } \
\
  Q_SIGNAL void yChanged (double y); \
\
  Q_PROPERTY ( \
    double horizontalZoomLevel READ getHorizontalZoomLevel WRITE \
      setHorizontalZoomLevel NOTIFY horizontalZoomLevelChanged) \
  double getHorizontalZoomLevel () const \
  { \
    return hzoom_level_; \
  } \
  void setHorizontalZoomLevel (double hzoom_level) \
  { \
    if (utils::math::floats_equal (hzoom_level_, hzoom_level)) \
      return; \
    hzoom_level_ = hzoom_level; \
    Q_EMIT horizontalZoomLevelChanged (hzoom_level); \
  } \
\
  Q_SIGNAL void horizontalZoomLevelChanged (double hzoom_level);

/**
 * Common editor settings.
 */
class EditorSettings : public utils::serialization::ISerializable<EditorSettings>
{
public:
  double clamp_scroll_start_x (double x);

  double clamp_scroll_start_y (double y);

  /**
   * Appends the given deltas to the scroll x/y values.
   */
  void append_scroll (double dx, double dy, bool validate);

protected:
  void copy_members_from (const EditorSettings &other)
  {
    scroll_start_x_ = other.scroll_start_x_;
    scroll_start_y_ = other.scroll_start_y_;
    hzoom_level_ = other.hzoom_level_;
  }

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /** Horizontal scroll start position. */
  double scroll_start_x_ = 0;

  /** Vertical scroll start position. */
  double scroll_start_y_ = 0;

  /** Horizontal zoom level. */
  double hzoom_level_ = 1.0;
};

class Timeline;
class PianoRoll;
class AutomationEditor;
class ChordEditor;
class AudioClipEditor;
using EditorSettingsVariant = std::
  variant<Timeline, PianoRoll, AutomationEditor, ChordEditor, AudioClipEditor>;
using EditorSettingsPtrVariant = to_pointer_variant<EditorSettingsVariant>;

/**
 * @}
 */

#endif
