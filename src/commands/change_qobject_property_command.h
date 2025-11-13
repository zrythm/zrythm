// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include <QObject>
#include <QUndoCommand>
#include <QVariant>

namespace zrythm::commands
{
class ChangeQObjectPropertyCommand : public QUndoCommand
{
public:
  ChangeQObjectPropertyCommand (
    QObject &object,
    QString  property_name,
    QVariant value)
      : object_ (object), property_name_ (property_name),
        original_value_ (object.property (property_name.toLatin1 ().data ())),
        value_ (std::move (value))
  {
    setText (QObject::tr ("Change %1").arg (property_name_));
  }

  int  id () const override { return 1762957285; }
  bool mergeWith (const QUndoCommand * other) override
  {
    if (other->id () != id ())
      return false;

    // only merge if other command was made in quick succession of this
    // command's redo() and operates on the same objects
    const auto * other_cmd =
      dynamic_cast<const ChangeQObjectPropertyCommand *> (other);
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

class ChangeTempoMapAffectingQObjectPropertyCommand
    : public ChangeQObjectPropertyCommand
{
public:
  static constexpr int CommandId = 1762957664;
  using ChangeQObjectPropertyCommand::ChangeQObjectPropertyCommand;

  int id () const override { return CommandId; }
};

} // namespace zrythm::commands
