// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/timeline.h"

#include <json/json.h>

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

void
to_json (nlohmann::json &j, const Timeline &p)
{
  j[Timeline::kEditorSettingsKey] = p.editor_settings_;
  j[Timeline::kTracksWidthKey] = p.tracks_width_;
}
void
from_json (const nlohmann::json &j, Timeline &p)
{
  j.at (Timeline::kEditorSettingsKey).get_to (p.editor_settings_);
  j.at (Timeline::kTracksWidthKey).get_to (p.tracks_width_);
}
}
