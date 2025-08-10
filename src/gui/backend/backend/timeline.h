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
class Timeline : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::gui::backend::EditorSettings * editorSettings READ getEditorSettings
      CONSTANT FINAL)
  QML_UNCREATABLE ("")

public:
  Timeline (QObject * parent = nullptr);

  // =========================================================
  // QML interface
  // =========================================================

  gui::backend::EditorSettings * getEditorSettings () const
  {
    return editor_settings_.get ();
  }

  // =========================================================

public:
  auto &get_selected_object_ids () { return selected_objects_; }

  friend void init_from (
    Timeline              &obj,
    const Timeline        &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kEditorSettingsKey = "editorSettings"sv;

  static constexpr auto kTracksWidthKey = "tracksWidth";
  friend void           to_json (nlohmann::json &j, const Timeline &p)
  {
    j[kEditorSettingsKey] = p.editor_settings_;
    j[kTracksWidthKey] = p.tracks_width_;
  }
  friend void from_json (const nlohmann::json &j, Timeline &p)
  {
    j.at (kEditorSettingsKey).get_to (p.editor_settings_);
    j.at (kTracksWidthKey).get_to (p.tracks_width_);
  }

private:
  utils::QObjectUniquePtr<gui::backend::EditorSettings> editor_settings_{
    new gui::backend::EditorSettings{ this }
  };

  /** Width of the left side of the timeline panel. */
  int tracks_width_ = 0;

  zrythm::structure::arrangement::ArrangerObjectSelectionManager::UuidSet
    selected_objects_;
};
