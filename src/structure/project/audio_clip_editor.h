// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/project/editor_settings.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::project
{

class AudioClipEditor : public EditorSettings
{
  Q_OBJECT
  QML_ELEMENT

public:
  AudioClipEditor (QObject * parent = nullptr) : EditorSettings (parent) { }

private:
  friend void init_from (
    AudioClipEditor       &obj,
    const AudioClipEditor &other,
    utils::ObjectCloneType clone_type);
  friend void to_json (nlohmann::json &j, const AudioClipEditor &editor);
  friend void from_json (const nlohmann::json &j, AudioClipEditor &editor);
};

}
