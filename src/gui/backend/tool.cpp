// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/tool.h"

using namespace zrythm::gui::backend;

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
Tool::init_after_cloning (const Tool &other, ObjectCloneType clone_type)
{
  tool_ = other.tool_;
}
