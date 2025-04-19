// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_CLIP_EDITOR_H__
#define __AUDIO_AUDIO_CLIP_EDITOR_H__

#include "gui/backend/backend/editor_settings.h"
#include "utils/icloneable.h"

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
      public ICloneable<AudioClipEditor>,
      public zrythm::utils::serialization::ISerializable<AudioClipEditor>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_EDITOR_SETTINGS_QML_PROPERTIES
public:
  AudioClipEditor (QObject * parent = nullptr) : QObject (parent) { }

public:
  DECLARE_DEFINE_FIELDS_METHOD ();

  void
  init_after_cloning (const AudioClipEditor &other, ObjectCloneType clone_type)
    override
  {
    static_cast<EditorSettings &> (*this) =
      static_cast<const EditorSettings &> (other);
  }
};

/**
 * @}
 */

#endif
