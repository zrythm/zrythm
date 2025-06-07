// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/tool.h"

namespace zrythm::gui::backend
{

Tool::Tool (QObject * parent) : QObject (parent) { }

int
Tool::getToolValue () const
{
  return static_cast<int> (tool_);
}

void
Tool::setToolValue (int tool)
{
  auto new_tool = static_cast<ToolType> (tool);
  if (tool_ != new_tool)
    {
      tool_ = new_tool;
      Q_EMIT toolValueChanged (tool);
    }
}

void
init_from (Tool &obj, const Tool &other, utils::ObjectCloneType clone_type)
{
  obj.tool_ = other.tool_;
}
} // namespace zrythm::gui::backend
