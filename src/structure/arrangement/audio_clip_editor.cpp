// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/audio_clip_editor.h"
#include "utils/logger.h"
#include "utils/serialization.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
void
to_json (nlohmann::json &j, const AudioClipEditor &editor)
{
  j[AudioClipEditor::kEditorSettingsKey] = editor.editor_settings_;
}
void
from_json (const nlohmann::json &j, AudioClipEditor &editor)
{
  j.at (AudioClipEditor::kEditorSettingsKey).get_to (editor.editor_settings_);
}
}
