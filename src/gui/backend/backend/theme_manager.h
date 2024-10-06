// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SETTINGS_THEME_MANAGER_H__
#define __SETTINGS_THEME_MANAGER_H__

#include "zrythm-config.h"

#include <QObject>
#include <QPalette>

#include <qqmlintegration.h>

namespace zrythm::gui
{

/**
 * @brief This doesn't work - see
 * https://forum.qt.io/topic/124965/how-to-define-a-property-of-type-palette/8
 *
 */
class ThemeManager final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  // active

  // disabled

  // inactive

  // zrythm color variant (accent | accent lighter | accent darker):
  // F79616 | FFA533 | D68A0C
  // more prominent: FF9F00 | FFB300 | F79616

  Q_PROPERTY (QColor accent MEMBER accent_ NOTIFY accent_changed FINAL)
  Q_PROPERTY (QColor base MEMBER base_ NOTIFY accent_changed FINAL)

  Q_PROPERTY (QPalette palette MEMBER palette_ NOTIFY palette_changed FINAL)

public:
  ThemeManager (QObject * parent = nullptr);

  ThemeManager * get_instance ();

  Q_SIGNAL void palette_changed ();
  Q_SIGNAL void accent_changed ();

public:
  QColor   accent_ = QColor ("#FF9F00");
  QColor   base_ = QColor ("#202020");
  QPalette palette_;
};

} // namespace zrythm::gui::glue

#endif // __SETTINGS_THEME_MANAGER_H__