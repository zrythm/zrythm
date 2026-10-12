// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/audio_clip_editor.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

void
AudioClipEditor::setSelectionStartTick (double tick)
{
  if (selection_start_tick_ == tick)
    return;
  selection_start_tick_ = tick;
  Q_EMIT selectionStartTickChanged (tick);
}

void
AudioClipEditor::setSelectionEndTick (double tick)
{
  if (selection_end_tick_ == tick)
    return;
  selection_end_tick_ = tick;
  Q_EMIT selectionEndTickChanged (tick);
}

void
init_from (
  AudioClipEditor       &obj,
  const AudioClipEditor &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<EditorSettings &> (obj),
    static_cast<const EditorSettings &> (other), clone_type);
}

void
to_json (nlohmann::json &j, const AudioClipEditor &editor)
{
  to_json (j, static_cast<const EditorSettings &> (editor));
}

void
from_json (const nlohmann::json &j, AudioClipEditor &editor)
{
  from_json (j, static_cast<EditorSettings &> (editor));
}

}
