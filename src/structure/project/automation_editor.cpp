// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/automation_editor.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

void
init_from (
  AutomationEditor       &obj,
  const AutomationEditor &other,
  utils::ObjectCloneType  clone_type)
{
  init_from (
    static_cast<EditorSettings &> (obj),
    static_cast<const EditorSettings &> (other), clone_type);
}

void
to_json (nlohmann::json &j, const AutomationEditor &editor)
{
  to_json (j, static_cast<const EditorSettings &> (editor));
}

void
from_json (const nlohmann::json &j, AutomationEditor &editor)
{
  from_json (j, static_cast<EditorSettings &> (editor));
}

}
