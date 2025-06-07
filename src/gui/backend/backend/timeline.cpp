// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/timeline.h"

Timeline::Timeline (QObject * parent) : QObject (parent) { }

void
init_from (Timeline &obj, const Timeline &other, utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<EditorSettings &> (obj),
    static_cast<const EditorSettings &> (other), clone_type);
  obj.tracks_width_ = other.tracks_width_;
  obj.selected_objects_ = other.selected_objects_;
}
