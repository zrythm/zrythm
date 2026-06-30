// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include "dsp/timebase.h"

#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief Undoable command that sets or clears a TimebaseProvider's override.
 *
 * Works at any level of the provider chain (track, lane, clip). Pass a
 * concrete Timebase to set an override, or std::nullopt to clear it (inherit
 * from the parent provider).
 *
 * @warning Stores a raw reference to @p provider. The caller must ensure the
 * provider outlives this command on the undo stack (guaranteed by the
 * reference-counted UUID system — deleting a track keeps it alive in the
 * registry until the delete command itself is destroyed).
 */
class ChangeTimebaseOverrideCommand : public QUndoCommand
{
public:
  ChangeTimebaseOverrideCommand (
    dsp::TimebaseProvider       &provider,
    std::optional<dsp::Timebase> timebase)
      : QUndoCommand (QObject::tr ("Change Timebase")), provider_ (provider),
        new_override_ (timebase)
  {
  }

  void undo () override
  {
    if (old_override_.has_value ())
      provider_.setOverride (*old_override_);
    else
      provider_.clearOverride ();
  }

  void redo () override
  {
    old_override_ = provider_.overrideValue ();
    if (new_override_.has_value ())
      provider_.setOverride (*new_override_);
    else
      provider_.clearOverride ();
  }

private:
  dsp::TimebaseProvider       &provider_;
  std::optional<dsp::Timebase> old_override_;
  std::optional<dsp::Timebase> new_override_;
};

} // namespace zrythm::commands
