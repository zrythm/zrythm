// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/timeline.h"

Timeline::Timeline (QObject * parent) : QObject (parent) { }

void
Timeline::init_after_cloning (const Timeline &other, ObjectCloneType clone_type)
{
  tracks_width_ = other.tracks_width_;
  EditorSettings::copy_members_from (other, clone_type);
}