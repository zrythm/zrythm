// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/project/editor_settings.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::project
{

class AutomationEditor : public EditorSettings
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  AutomationEditor (QObject * parent = nullptr) : EditorSettings (parent) { }

private:
  friend void init_from (
    AutomationEditor       &obj,
    const AutomationEditor &other,
    utils::ObjectCloneType  clone_type);
  friend void to_json (nlohmann::json &j, const AutomationEditor &editor);
  friend void from_json (const nlohmann::json &j, AutomationEditor &editor);
};

}
