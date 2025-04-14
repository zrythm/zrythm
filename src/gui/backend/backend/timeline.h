// SPDX-FileCopyrightText: © 2020, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_TIMELINE_H__
#define __GUI_BACKEND_TIMELINE_H__

#include "gui/backend/backend/editor_settings.h"
#include "gui/dsp/arranger_object_all.h"
#include "utils/icloneable.h"

#include <QtQmlIntegration>

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
  auto &get_selected_object_ids () { return selected_objects_; }

  DECLARE_DEFINE_FIELDS_METHOD ();

  void init_after_cloning (const Timeline &other, ObjectCloneType clone_type)
    override;

private:
  /** Width of the left side of the timeline panel. */
  int tracks_width_ = 0;

  ArrangerObjectSelectionManager::UuidSet selected_objects_;
};

#endif
