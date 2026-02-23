// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/editor_settings.h"
#include "utils/icloneable.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::arrangement
{

/**
 * Backend for the automation editor.
 */
class AutomationEditor : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::arrangement::EditorSettings * editorSettings READ
      getEditorSettings CONSTANT FINAL)
  QML_UNCREATABLE ("")

public:
  AutomationEditor (QObject * parent = nullptr);

  // =========================================================
  // QML interface
  // =========================================================

  auto getEditorSettings () const { return editor_settings_.get (); }

  // =========================================================

public:
  friend void init_from (
    AutomationEditor       &obj,
    const AutomationEditor &other,
    utils::ObjectCloneType  clone_type)

  {
    obj.editor_settings_ =
      utils::clone_unique_qobject (*other.editor_settings_, &obj);
  }

private:
  static constexpr auto kEditorSettingsKey = "editorSettings"sv;
  friend void to_json (nlohmann::json &j, const AutomationEditor &editor);
  friend void from_json (const nlohmann::json &j, AutomationEditor &editor);

private:
  utils::QObjectUniquePtr<EditorSettings> editor_settings_;
};

}
