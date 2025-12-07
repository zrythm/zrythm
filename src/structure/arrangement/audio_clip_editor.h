// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/editor_settings.h"
#include "utils/icloneable.h"

#include <QtQmlIntegration>

namespace zrythm::structure::arrangement
{
/**
 * Audio clip editor serializable backend.
 *
 * The actual widgets should reflect the* information here.
 */
class AudioClipEditor : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::arrangement::EditorSettings * editorSettings READ
      getEditorSettings CONSTANT FINAL)

public:
  AudioClipEditor (QObject * parent = nullptr) : QObject (parent) { }

  // =========================================================
  // QML interface
  // =========================================================

  auto getEditorSettings () const { return editor_settings_.get (); }

  // =========================================================

public:
  friend void init_from (
    AudioClipEditor       &obj,
    const AudioClipEditor &other,
    utils::ObjectCloneType clone_type)
  {
    obj.editor_settings_ =
      utils::clone_unique_qobject (*other.editor_settings_, &obj);
  }

private:
  static constexpr auto kEditorSettingsKey = "editorSettings"sv;
  friend void to_json (nlohmann::json &j, const AudioClipEditor &editor);
  friend void from_json (const nlohmann::json &j, AudioClipEditor &editor);

private:
  utils::QObjectUniquePtr<EditorSettings> editor_settings_{ new EditorSettings{
    this } };
};

}
