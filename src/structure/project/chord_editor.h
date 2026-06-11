// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/project/editor_settings.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::project
{

/**
 * @brief View state for the chord editor lane.
 *
 * Holds scroll position and zoom level for the chord arrangement view.
 */
class ChordEditor : public EditorSettings
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ChordEditor (QObject * parent = nullptr);

  friend void init_from (
    ChordEditor                   &obj,
    const ChordEditor             &other,
    zrythm::utils::ObjectCloneType clone_type);
  friend void to_json (nlohmann::json &j, const ChordEditor &editor);
  friend void from_json (const nlohmann::json &j, ChordEditor &editor);
};

}
