// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/timeline.h"

Timeline::Timeline (
  const structure::arrangement::ArrangerObjectRegistry &registry,
  QObject *                                             parent)
    : QObject (parent),
      selection_manager_ (
        utils::make_qobject_unique<
          gui::backend::ArrangerObjectSelectionManager> (registry, this))
{
}

void
init_from (Timeline &obj, const Timeline &other, utils::ObjectCloneType clone_type)
{
  obj.editor_settings_ =
    utils::clone_unique_qobject (*other.editor_settings_, &obj);
  obj.tracks_width_ = other.tracks_width_;
}
