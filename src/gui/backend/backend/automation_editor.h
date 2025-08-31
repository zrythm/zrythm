// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/arranger_object_selection_manager.h"
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
class AutomationEditor : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    gui::backend::EditorSettings * editorSettings READ getEditorSettings
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::backend::ArrangerObjectSelectionManager * selectionManager READ
      selectionManager CONSTANT)
  QML_UNCREATABLE ("")

public:
  AutomationEditor (
    const structure::arrangement::ArrangerObjectRegistry &registry,
    QObject *                                             parent = nullptr)
      : QObject (parent),
        selection_manager_ (
          utils::make_qobject_unique<
            gui::backend::ArrangerObjectSelectionManager> (registry, this))
  {
  }

  // =========================================================
  // QML interface
  // =========================================================

  gui::backend::EditorSettings * getEditorSettings () const
  {
    return editor_settings_.get ();
  }

  gui::backend::ArrangerObjectSelectionManager * selectionManager () const
  {
    return selection_manager_.get ();
  }

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
  friend void to_json (nlohmann::json &j, const AutomationEditor &editor)
  {
    j[kEditorSettingsKey] = editor.editor_settings_;
  }
  friend void from_json (const nlohmann::json &j, AutomationEditor &editor)
  {
    j.at (kEditorSettingsKey).get_to (editor.editor_settings_);
  }

private:
  utils::QObjectUniquePtr<gui::backend::EditorSettings> editor_settings_{
    new gui::backend::EditorSettings{ this }
  };

  utils::QObjectUniquePtr<gui::backend::ArrangerObjectSelectionManager>
    selection_manager_;
};

/**
 * @}
 */
