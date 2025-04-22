// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/timeline.h"

Timeline::Timeline (QObject * parent) : QObject (parent) { }

void
Timeline::init_after_cloning (const Timeline &other, ObjectCloneType clone_type)
{
  EditorSettings::copy_members_from (other, clone_type);
  tracks_width_ = other.tracks_width_;
  selected_objects_ = other.selected_objects_;
}

void
Timeline::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<Timeline>;
  T::call_all_base_define_fields<EditorSettings> (ctx);
  T::serialize_fields (
    ctx,
    // available after 1.8
    T::make_field ("tracksWidth", tracks_width_, true));
}