// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "structure/tracks/track.h"

#include <QUndoCommand>

namespace zrythm::commands
{
class ChangeTrackCommentCommand : public QUndoCommand
{

public:
  ChangeTrackCommentCommand (structure::tracks::Track &track, QString comment)
      : QUndoCommand (QObject::tr ("Change Track Comment")), track_ (track),
        comment_after_ (std::move (comment))
  {
  }

  void undo () override { track_.setComment (comment_before_); }
  void redo () override
  {
    comment_before_ = track_.comment ();
    track_.setComment (comment_after_);
  }

private:
  structure::tracks::Track &track_;
  QString                   comment_before_;
  QString                   comment_after_;
};

} // namespace zrythm::commands
