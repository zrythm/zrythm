// SPDX-FileCopyrightText: © 2020, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once
#include "gui/backend/backend/editor_settings.h"
#include "structure/arrangement/arranger_object_all.h"
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
class Timeline final : public QObject, public EditorSettings
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_EDITOR_SETTINGS_QML_PROPERTIES

public:
  Timeline (QObject * parent = nullptr);

public:
  auto &get_selected_object_ids () { return selected_objects_; }

  friend void init_from (
    Timeline              &obj,
    const Timeline        &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kTracksWidthKey = "tracksWidth";
  friend void           to_json (nlohmann::json &j, const Timeline &p)
  {
    to_json (j, static_cast<const EditorSettings &> (p));
    j[kTracksWidthKey] = p.tracks_width_;
  }
  friend void from_json (const nlohmann::json &j, Timeline &p)
  {
    from_json (j, static_cast<EditorSettings &> (p));
    j.at (kTracksWidthKey).get_to (p.tracks_width_);
  }

private:
  /** Width of the left side of the timeline panel. */
  int tracks_width_ = 0;

  zrythm::structure::arrangement::ArrangerObjectSelectionManager::UuidSet
    selected_objects_;
};
