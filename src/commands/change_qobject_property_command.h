// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <typeinfo>
#include <utility>

#include "commands/command.h"

#include <QObject>
#include <QVariant>

namespace zrythm::commands
{

/**
 * @brief Changes a QObject property with undo support.
 *
 * @warning This command stores a raw reference (@ref object_) to the QObject.
 * The caller MUST ensure the QObject outlives this command on the undo stack.
 * If the object may be destroyed by other commands (e.g., it is owned by a
 * model that can remove/replace it), use a state-based command that captures
 * the data by value instead — otherwise undo will dereference a dangling
 * reference (use-after-free).
 */
class ChangeQObjectPropertyCommand : public Command
{
public:
  /**
   * @param engine_requirement Engine cooperation this change needs: pass
   * PauseProcessing when the property affects the tempo map, None
   * otherwise.
   */
  ChangeQObjectPropertyCommand (
    QObject          &object,
    QString           property_name,
    QVariant          value,
    EngineRequirement engine_requirement)
      : Command (QObject::tr ("Change %1").arg (property_name), engine_requirement),
        object_ (object), property_name_ (property_name),
        original_value_ (object.property (property_name.toLatin1 ().data ())),
        value_ (std::move (value))
  {
  }

  bool merge_with (const Command &other) override
  {
    // Only the exact same class merges: subclasses of this command (e.g.
    // the UUID-referencing variant) carry their own merge rules, and
    // merging one into this raw-reference variant would lose their extra
    // safety
    if (typeid (*this) != typeid (other))
      return false;
    const auto * other_cmd =
      static_cast<const ChangeQObjectPropertyCommand *> (&other);

    // Only merge with changes needing the same engine cooperation: a
    // tempo-map-affecting change merged into a plain one would lose its
    // engine pause
    if (other_cmd->engine_requirement () != engine_requirement ())
      return false;

    // only merge if other command was made in quick succession of this
    // command's redo() and operates on the same objects
    const auto cur_time = std::chrono::steady_clock::now ();
    const auto duration = cur_time - last_redo_timestamp_;
    if (
      std::chrono::duration_cast<std::chrono::milliseconds> (duration).count ()
      > 1'000)
      {
        return false;
      }

    // Check if we're operating on the same object and property
    if (
      &object_ != &other_cmd->object_
      || property_name_ != other_cmd->property_name_)
      return false;

    last_redo_timestamp_ = cur_time;
    value_ = other_cmd->value_;
    return true;
  }

  void undo () override
  {
    object_.setProperty (property_name_.toLatin1 ().data (), original_value_);
  }

  void redo () override
  {
    object_.setProperty (property_name_.toLatin1 ().data (), value_);
    last_redo_timestamp_ = std::chrono::steady_clock::now ();
  }

private:
  QObject                                           &object_;
  QString                                            property_name_;
  QVariant                                           original_value_;
  QVariant                                           value_;
  std::chrono::time_point<std::chrono::steady_clock> last_redo_timestamp_;
};

} // namespace zrythm::commands
