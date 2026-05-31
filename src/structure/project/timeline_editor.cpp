// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/timeline_editor.h"
#include "utils/iobject_registry.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

TimelineEditor::TimelineEditor (
  const utils::IObjectRegistry &registry,
  QObject *                     parent)
    : EditorSettings (parent)
{
}

void
init_from (
  TimelineEditor        &obj,
  const TimelineEditor  &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<EditorSettings &> (obj),
    static_cast<const EditorSettings &> (other), clone_type);
}

void
to_json (nlohmann::json &j, const TimelineEditor &p)
{
  to_json (j, static_cast<const EditorSettings &> (p));
}

void
from_json (const nlohmann::json &j, TimelineEditor &p)
{
  from_json (j, static_cast<EditorSettings &> (p));
}

}
