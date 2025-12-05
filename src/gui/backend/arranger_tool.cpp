// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/arranger_tool.h"

namespace zrythm::gui::backend
{

ArrangerTool::ArrangerTool (QObject * parent) : QObject (parent) { }

int
ArrangerTool::toolValue () const
{
  return static_cast<int> (tool_);
}

void
ArrangerTool::setToolValue (int tool)
{
  auto new_tool = static_cast<ToolType> (tool);
  if (tool_ != new_tool)
    {
      tool_ = new_tool;
      Q_EMIT toolValueChanged (tool);
    }
}

void
init_from (
  ArrangerTool          &obj,
  const ArrangerTool    &other,
  utils::ObjectCloneType clone_type)
{
  obj.tool_ = other.tool_;
}

ArrangerTool::~ArrangerTool () = default;

void
to_json (nlohmann::json &j, const ArrangerTool &tool)
{
  j = tool.tool_;
}
void
from_json (const nlohmann::json &j, ArrangerTool &tool)
{
  j.get_to (tool.tool_);
}

} // namespace zrythm::gui::backend
