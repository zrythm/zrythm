// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/editor_settings.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/icloneable.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUTOMATION_EDITOR (CLIP_EDITOR->automation_editor_)

/**
 * Backend for the automation editor.
 */
class AutomationEditor final : public QObject, public EditorSettings
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_EDITOR_SETTINGS_QML_PROPERTIES
public:
  AutomationEditor (QObject * parent = nullptr) : QObject (parent) { }

public:
  friend void init_from (
    AutomationEditor       &obj,
    const AutomationEditor &other,
    utils::ObjectCloneType  clone_type)

  {
    static_cast<EditorSettings &> (obj) =
      static_cast<const EditorSettings &> (other);
    obj.selected_objects_ = other.selected_objects_;
  }

  auto &get_selected_object_ids () { return selected_objects_; }

private:
  friend void to_json (nlohmann::json &j, const AutomationEditor &editor)
  {
    to_json (j, static_cast<const EditorSettings &> (editor));
  }
  friend void from_json (const nlohmann::json &j, AutomationEditor &editor)
  {
    from_json (j, static_cast<EditorSettings &> (editor));
  }

private:
  structure::arrangement::ArrangerObjectSelectionManager::UuidSet
    selected_objects_;
};

/**
 * @}
 */
