// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Automation editor backend.
 */

#ifndef __GUI_BACKEND_AUTOMATION_EDITOR_H__
#define __GUI_BACKEND_AUTOMATION_EDITOR_H__

#include "gui/backend/editor_settings.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUTOMATION_EDITOR_SCHEMA_VERSION 1

#define AUTOMATION_EDITOR (CLIP_EDITOR->automation_editor)

typedef struct Region Region;

/**
 * Backend for the automation editor.
 */
typedef struct AutomationEditor
{
  EditorSettings editor_settings;
} AutomationEditor;

/**
 * Inits the AutomationEditor after a Project has been
 * loaded.
 */
void
automation_editor_init_loaded (AutomationEditor * self);

/**
 * Initializes the AutomationEditor.
 */
void
automation_editor_init (AutomationEditor * self);

AutomationEditor *
automation_editor_clone (AutomationEditor * src);

AutomationEditor *
automation_editor_new (void);

void
automation_editor_free (AutomationEditor * self);

/**
 * @}
 */

#endif
