// SPDX-FileCopyrightText: © 2020, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/editor_settings.h"
#include "utils/icloneable.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::arrangement
{

/**
 * @brief Timeline settings.
 */
class Timeline : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::arrangement::EditorSettings * editorSettings READ
      getEditorSettings CONSTANT FINAL)
  QML_UNCREATABLE ("")

public:
  Timeline (
    const structure::arrangement::ArrangerObjectRegistry &registry,
    QObject *                                             parent = nullptr);

  // =========================================================
  // QML interface
  // =========================================================

  auto getEditorSettings () const { return editor_settings_.get (); }

  // =========================================================

public:
  friend void init_from (
    Timeline              &obj,
    const Timeline        &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kEditorSettingsKey = "editorSettings"sv;

  static constexpr auto kTracksWidthKey = "tracksWidth";
  friend void           to_json (nlohmann::json &j, const Timeline &p);
  friend void           from_json (const nlohmann::json &j, Timeline &p);

private:
  utils::QObjectUniquePtr<EditorSettings> editor_settings_;

  /** Width of the left side of the timeline panel. */
  int tracks_width_ = 0;
};
}
