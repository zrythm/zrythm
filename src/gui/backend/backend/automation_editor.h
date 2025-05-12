// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/editor_settings.h"
#include "gui/dsp/arranger_object_all.h"
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
class AutomationEditor final
    : public QObject,
      public EditorSettings,
      public ICloneable<AutomationEditor>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_EDITOR_SETTINGS_QML_PROPERTIES
public:
  AutomationEditor (QObject * parent = nullptr) : QObject (parent) { }

public:
  void
  init_after_cloning (const AutomationEditor &other, ObjectCloneType clone_type)
    override
  {
    static_cast<EditorSettings &> (*this) =
      static_cast<const EditorSettings &> (other);
    selected_objects_ = other.selected_objects_;
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
  ArrangerObjectSelectionManager::UuidSet selected_objects_;
};

/**
 * @}
 */
