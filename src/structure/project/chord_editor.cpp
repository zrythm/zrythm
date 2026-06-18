// SPDX-FileCopyrightText: © 2019-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/chord_editor.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

ChordEditor::ChordEditor (QObject * parent) : EditorSettings (parent) { }

void
init_from (
  ChordEditor                   &obj,
  const ChordEditor             &other,
  zrythm::utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<EditorSettings &> (obj),
    static_cast<const EditorSettings &> (other), clone_type);
}

void
to_json (nlohmann::json &j, const ChordEditor &editor)
{
  to_json (j, static_cast<const EditorSettings &> (editor));
}

void
from_json (const nlohmann::json &j, ChordEditor &editor)
{
  from_json (j, static_cast<EditorSettings &> (editor));
}

}
