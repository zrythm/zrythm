// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/timeline.h"

Timeline::Timeline (QObject * parent) : QObject (parent) { }

void
init_from (Timeline &obj, const Timeline &other, utils::ObjectCloneType clone_type)
{
  obj.editor_settings_ =
    utils::clone_unique_qobject (*other.editor_settings_, &obj);
  obj.tracks_width_ = other.tracks_width_;
  obj.selected_objects_ = other.selected_objects_;
}
