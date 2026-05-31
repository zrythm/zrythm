// SPDX-FileCopyrightText: © 2020, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/project/editor_settings.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::utils
{
class IObjectRegistry;
}

namespace zrythm::structure::project
{

class TimelineEditor : public EditorSettings
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  TimelineEditor (
    const utils::IObjectRegistry &registry,
    QObject *                     parent = nullptr);

private:
  friend void init_from (
    TimelineEditor        &obj,
    const TimelineEditor  &other,
    utils::ObjectCloneType clone_type);
  friend void to_json (nlohmann::json &j, const TimelineEditor &p);
  friend void from_json (const nlohmann::json &j, TimelineEditor &p);
};

}
