// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/automation_editor.h"

namespace zrythm::structure::arrangement
{
AutomationEditor::AutomationEditor (QObject * parent)
    : QObject (parent),
      editor_settings_ (utils::make_qobject_unique<EditorSettings> (this))
{
}

void
to_json (nlohmann::json &j, const AutomationEditor &editor)
{
  j[AutomationEditor::kEditorSettingsKey] = editor.editor_settings_;
}
void
from_json (const nlohmann::json &j, AutomationEditor &editor)
{
  j.at (AutomationEditor::kEditorSettingsKey).get_to (editor.editor_settings_);
}
}
