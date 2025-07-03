// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/editor_settings.h"
#include "utils/icloneable.h"

#include <QtQmlIntegration>

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUDIO_CLIP_EDITOR (CLIP_EDITOR->audio_clip_editor_)

/**
 * Audio clip editor serializable backend.
 *
 * The actual widgets should reflect the* information here.
 */
class AudioClipEditor final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    gui::backend::EditorSettings * editorSettings READ getEditorSettings
      CONSTANT FINAL)

public:
  AudioClipEditor (QObject * parent = nullptr) : QObject (parent) { }

  // =========================================================
  // QML interface
  // =========================================================

  gui::backend::EditorSettings * getEditorSettings () const
  {
    return editor_settings_.get ();
  }

  // =========================================================

  auto &get_selected_object_ids () { return selected_objects_; }

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
  friend void to_json (nlohmann::json &j, const AudioClipEditor &editor)
  {
    j[kEditorSettingsKey] = editor.editor_settings_;
  }
  friend void from_json (const nlohmann::json &j, AudioClipEditor &editor)
  {
    j.at (kEditorSettingsKey).get_to (editor.editor_settings_);
  }

private:
  utils::QObjectUniquePtr<gui::backend::EditorSettings> editor_settings_{
    new gui::backend::EditorSettings{ this }
  };

  // unused? only added to satisfy ArrangerObjectFactory
  structure::arrangement::ArrangerObjectSelectionManager::UuidSet
    selected_objects_;
};

/**
 * @}
 */
