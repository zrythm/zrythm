// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/timeline.h"

namespace zrythm::structure::arrangement
{
Timeline::Timeline (
  const structure::arrangement::ArrangerObjectRegistry &registry,
  QObject *                                             parent)
    : QObject (parent),
      editor_settings_ (utils::make_qobject_unique<EditorSettings> (this))
{
}

void
init_from (Timeline &obj, const Timeline &other, utils::ObjectCloneType clone_type)
{
  obj.editor_settings_ =
    utils::clone_unique_qobject (*other.editor_settings_, &obj);
  obj.tracks_width_ = other.tracks_width_;
}
}
