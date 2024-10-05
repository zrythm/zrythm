// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_CLIP_EDITOR_H__
#define __AUDIO_AUDIO_CLIP_EDITOR_H__

#include "gui/cpp/backend/editor_settings.h"

#include "common/utils/icloneable.h"

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
    : public EditorSettings,
      public ICloneable<AudioClipEditor>,
      public ISerializable<AudioClipEditor>
{
public:
  DECLARE_DEFINE_FIELDS_METHOD ();

  void init_after_cloning (const AudioClipEditor &other) override
  {
    *this = other;
  }
};

/**
 * @}
 */

#endif
