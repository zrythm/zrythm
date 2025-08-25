// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "structure/tracks/track.h"

#include <QUndoCommand>

namespace zrythm::commands
{
class ChangeTrackColorCommand : public QUndoCommand
{

public:
  ChangeTrackColorCommand (structure::tracks::Track &track, const QColor &color)
      : QUndoCommand (QObject::tr ("Change Track Color")), track_ (track),
        color_after_ (color)
  {
  }

  void undo () override { track_.setColor (color_before_); }
  void redo () override
  {
    color_before_ = track_.color ();
    track_.setColor (color_after_);
  }

private:
  structure::tracks::Track &track_;
  QColor                    color_before_;
  QColor                    color_after_;
};

} // namespace zrythm::commands
