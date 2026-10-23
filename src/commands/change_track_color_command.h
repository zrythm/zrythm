// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <utility>

#include "structure/tracks/track.h"

#include <QUndoCommand>

namespace zrythm::commands
{
/**
 * @brief Sets or clears a track's user-set color.
 *
 * A nullopt color resets the track to its type-derived default color.
 */
class ChangeTrackColorCommand : public QUndoCommand
{

public:
  ChangeTrackColorCommand (
    structure::tracks::Track                      &track,
    std::optional<structure::tracks::Track::Color> color_after)
      : QUndoCommand (
          color_after ? QObject::tr ("Change Track Color")
                      : QObject::tr ("Reset Track Color")),
        track_ (track), color_after_ (std::move (color_after))
  {
  }

  void undo () override { track_.set_stored_color (color_before_); }
  void redo () override
  {
    color_before_ = track_.stored_color ();
    track_.set_stored_color (color_after_);
  }

private:
  structure::tracks::Track                      &track_;
  std::optional<structure::tracks::Track::Color> color_before_;
  std::optional<structure::tracks::Track::Color> color_after_;
};

} // namespace zrythm::commands
