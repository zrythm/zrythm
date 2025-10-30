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
}
