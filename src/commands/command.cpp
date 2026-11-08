// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "commands/command.h"

namespace zrythm::commands
{

void
MacroCommand::undo ()
{
  for (const auto &child : std::views::reverse (children_))
    {
      child->undo ();
    }
}

void
MacroCommand::redo ()
{
  for (const auto &child : children_)
    {
      child->redo ();
    }
}

Command::EngineRequirement
MacroCommand::engine_requirement () const noexcept
{
  auto result = EngineRequirement::None;
  for (const auto &child : children_)
    {
      result = (std::max) (result, child->engine_requirement ());
    }
  return result;
}

void
MacroCommand::add_child (std::unique_ptr<Command> child)
{
  children_.push_back (std::move (child));
}

Command *
MacroCommand::child_at (std::size_t index) noexcept
{
  return index < children_.size () ? children_[index].get () : nullptr;
}

const Command *
MacroCommand::child_at (std::size_t index) const noexcept
{
  return index < children_.size () ? children_[index].get () : nullptr;
}

Command *
MacroCommand::last_child () noexcept
{
  return children_.empty () ? nullptr : children_.back ().get ();
}

const Command *
MacroCommand::last_child () const noexcept
{
  return children_.empty () ? nullptr : children_.back ().get ();
}

} // namespace zrythm::commands
