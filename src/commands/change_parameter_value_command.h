// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "commands/command.h"
#include "dsp/parameter.h"
#include "utils/logger.h"

namespace zrythm::commands
{
class ChangeParameterValueCommand : public Command
{

public:
  /**
   * QUndoCommand id for merge participation: Qt only calls mergeWith()
   * between commands sharing a non-negative id.
   */
  static constexpr int kMergeId = 89453187;

  ChangeParameterValueCommand (
    dsp::ProcessorParameter &param,
    utils::IObjectRegistry  &registry,
    float                    value)
      : Command (
          QObject::tr ("Change '%1' value to %2").arg (param.label ()).arg (value),
          EngineRequirement::None),
        param_ref_ (param.get_uuid (), registry), value_after_ (value)
  {
  }

  int  id () const override { return kMergeId; }
  bool mergeWith (const QUndoCommand * other) override
  {
    if (other->id () != id ())
      return false;

    const auto * other_cmd =
      dynamic_cast<const ChangeParameterValueCommand *> (other);
    if (other_cmd == nullptr)
      return false;

    // only merge edits of the same parameter
    if (!(other_cmd->param_ref_.id () == param_ref_.id ()))
      return false;

    // only merge if other command was made in quick succession of this
    // command's redo()
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

  void undo () override
  {
    auto * param = param_ref_.get ();
    z_return_if_fail (param != nullptr);
    param->setBaseValueByUser (value_before_);
  }

  void redo () override
  {
    auto * param = param_ref_.get ();
    z_return_if_fail (param != nullptr);
    value_before_ = param->baseValue ();
    param->setBaseValueByUser (value_after_);
    last_redo_timestamp_ = std::chrono::steady_clock::now ();
  }

private:
  // Holds a registry reference count so the parameter stays alive while this
  // command is on the undo stack, even if its owning plugin is detached from
  // its rack by another command
  dsp::ProcessorParameterUuidReference               param_ref_;
  float                                              value_before_{};
  float                                              value_after_{};
  std::chrono::time_point<std::chrono::steady_clock> last_redo_timestamp_;
};

} // namespace zrythm::commands
