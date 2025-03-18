// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_AUTOMATION_EDITOR_H__
#define __GUI_BACKEND_AUTOMATION_EDITOR_H__

#include "gui/backend/backend/editor_settings.h"

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
      public ICloneable<AutomationEditor>,
      public zrythm::utils::serialization::ISerializable<AutomationEditor>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_EDITOR_SETTINGS_QML_PROPERTIES
public:
  AutomationEditor (QObject * parent = nullptr) : QObject (parent) { }

public:
  DECLARE_DEFINE_FIELDS_METHOD ();

  void
  init_after_cloning (const AutomationEditor &other, ObjectCloneType clone_type)
    override
  {
    static_cast<EditorSettings &> (*this) =
      static_cast<const EditorSettings &> (other);
  }
};

/**
 * @}
 */

#endif
