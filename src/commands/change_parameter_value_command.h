// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/parameter.h"

#include <QUndoCommand>

namespace zrythm::commands
{
class ChangeParameterValueCommand : public QUndoCommand
{

public:
  ChangeParameterValueCommand (dsp::ProcessorParameter &param, float value)
      : QUndoCommand (
          QObject::tr ("Change '%1' value to %2").arg (param.label ()).arg (value)),
        param_ (param), value_after_ (value)
  {
  }

  int  id () const override { return 89453187; }
  bool mergeWith (const QUndoCommand * other) override
  {
    if (other->id () != id ())
      return false;

    // only merge if other command was made in quick succession of this
    // command's redo()
    const auto * other_cmd =
      dynamic_cast<const ChangeParameterValueCommand *> (other);
    const auto cur_time = std::chrono::steady_clock::now ();
    const auto duration = cur_time - last_redo_timestamp_;
    if (
      std::chrono::duration_cast<std::chrono::milliseconds> (duration).count ()
      > 1'000)
      {
        return false;
      }

    last_redo_timestamp_ = cur_time;
    value_after_ = other_cmd->value_after_;
    return true;
  }

  void undo () override { param_.setBaseValue (value_before_); }
  void redo () override
  {
    value_before_ = param_.baseValue ();
    param_.setBaseValue (value_after_);
    last_redo_timestamp_ = std::chrono::steady_clock::now ();
  }

private:
  dsp::ProcessorParameter                           &param_;
  float                                              value_before_{};
  float                                              value_after_{};
  std::chrono::time_point<std::chrono::steady_clock> last_redo_timestamp_;
};

} // namespace zrythm::commands
