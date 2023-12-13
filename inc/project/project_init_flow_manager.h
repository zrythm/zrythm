// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Project helper.
 */

#ifndef __PROJECT_PROJECT_INIT_FLOW_MANAGER_H__
#define __PROJECT_PROJECT_INIT_FLOW_MANAGER_H__

#include "zrythm-config.h"

/**
 * @addtogroup project
 *
 * @{
 */

/**
 * Callback to call when project initialization (loading or creating new)
 * finishes.
 */
typedef void (
  *ProjectInitDoneCallback) (bool success, GError * error, void * user_data);

/**
 * If filename set, it loads that, otherwise it loads the default project.
 *
 * @note This function is async when running normally.
 *
 * @param filename The filename to open. This will be the template in the case
 * of template, or the actual project otherwise.
 * @param is_template Load the project as a template and create a new project
 * from it.
 * @param cb Callback to call when finished.
 * @param user_data User data to pass to @p cb.
 */
void
project_init_flow_manager_load_or_create_default_project (
  const char *            filename,
  const bool              is_template,
  ProjectInitDoneCallback cb,
  void *                  user_data);

/**
 * @}
 */

#endif
