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
class AudioClipEditor final
    : public QObject,
      public EditorSettings,
      public ICloneable<AudioClipEditor>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_EDITOR_SETTINGS_QML_PROPERTIES
public:
  AudioClipEditor (QObject * parent = nullptr) : QObject (parent) { }

public:
  void
  init_after_cloning (const AudioClipEditor &other, ObjectCloneType clone_type)
    override
  {
    static_cast<EditorSettings &> (*this) =
      static_cast<const EditorSettings &> (other);
  }

private:
  friend void to_json (nlohmann::json &j, const AudioClipEditor &editor)
  {
    to_json (j, static_cast<const EditorSettings &> (editor));
  }
  friend void from_json (const nlohmann::json &j, AudioClipEditor &editor)
  {
    from_json (j, static_cast<EditorSettings &> (editor));
  }
};

/**
 * @}
 */
