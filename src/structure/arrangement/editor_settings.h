// SPDX-FileCopyrightText: © 2020-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/icloneable.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::arrangement
{

/**
 * Common editor settings.
 */
class EditorSettings : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (double x READ getX WRITE setX NOTIFY xChanged)
  Q_PROPERTY (double y READ getY WRITE setY NOTIFY yChanged)
  Q_PROPERTY (
    double horizontalZoomLevel READ getHorizontalZoomLevel WRITE
      setHorizontalZoomLevel NOTIFY horizontalZoomLevelChanged)

public:
  EditorSettings (QObject * parent = nullptr) : QObject (parent) { }

  // =========================================================
  // QML interface
  // =========================================================

  double        getX () const { return scroll_start_x_; }
  void          setX (double x);
  Q_SIGNAL void xChanged (double x);

  double        getY () const { return scroll_start_y_; }
  void          setY (double y);
  Q_SIGNAL void yChanged (double y);

  double        getHorizontalZoomLevel () const { return hzoom_level_; }
  void          setHorizontalZoomLevel (double hzoom_level);
  Q_SIGNAL void horizontalZoomLevelChanged (double hzoom_level);

  // =========================================================

  double clamp_scroll_start_x (double x);

  double clamp_scroll_start_y (double y);

  /**
   * Appends the given deltas to the scroll x/y values.
   */
  void append_scroll (double dx, double dy, bool validate);

protected:
  friend void init_from (
    EditorSettings        &obj,
    const EditorSettings  &other,
    utils::ObjectCloneType clone_type)
  {
    obj.scroll_start_x_ = other.scroll_start_x_;
    obj.scroll_start_y_ = other.scroll_start_y_;
    obj.hzoom_level_ = other.hzoom_level_;
  }

private:
  static constexpr auto kScrollStartXKey = "scrollStartX"sv;
  static constexpr auto kScrollStartYKey = "scrollStartY"sv;
  static constexpr auto kHZoomLevelKey = "hZoomLevel"sv;
  friend void to_json (nlohmann::json &j, const EditorSettings &settings);
  friend void from_json (const nlohmann::json &j, EditorSettings &settings);

private:
  /** Horizontal scroll start position. */
  double scroll_start_x_ = 0;

  /** Vertical scroll start position. */
  double scroll_start_y_ = 0;

  /** Horizontal zoom level. */
  double hzoom_level_ = 1.0;
};

}
