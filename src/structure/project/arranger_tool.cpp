// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/arranger_tool.h"
#include "utils/debug.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::project
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
      Q_EMIT effectiveToolValueChanged ();
    }
}

ArrangerTool::ToolType
ArrangerTool::effectiveToolValue () const
{
  return tool_value_override_.value_or (tool_);
}

void
ArrangerTool::setToolValueOverride (int tool)
{
  if (tool < 0 || tool > static_cast<int> (ToolType::Audition))
    {
      z_warning ("Invalid tool value override: {}", tool);
      return;
    }
  const auto new_override = static_cast<ToolType> (tool);
  if (tool_value_override_ != new_override)
    {
      tool_value_override_ = new_override;
      Q_EMIT effectiveToolValueChanged ();
    }
}

void
ArrangerTool::clearToolValueOverride ()
{
  if (tool_value_override_.has_value ())
    {
      tool_value_override_.reset ();
      Q_EMIT effectiveToolValueChanged ();
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

} // namespace zrythm::structure::project
