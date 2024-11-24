// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Timeline backend.
 */

#ifndef __GUI_BACKEND_TIMELINE_H__
#define __GUI_BACKEND_TIMELINE_H__

#include "gui/backend/backend/editor_settings.h"

#include "utils/icloneable.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define PRJ_TIMELINE (PROJECT->timeline_)

/**
 * @brief Timeline settings.
 */
class Timeline final
    : public QObject,
      public EditorSettings,
      public ICloneable<Timeline>,
      public zrythm::utils::serialization::ISerializable<Timeline>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_EDITOR_SETTINGS_QML_PROPERTIES

public:
  Timeline (QObject * parent = nullptr);

public:
  DECLARE_DEFINE_FIELDS_METHOD ();

  void init_after_cloning (const Timeline &other) override;

public:
  /** Width of the left side of the timeline panel. */
  int tracks_width_ = 0;
};

/**
 * @}
 */

#endif
