// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Project serialization.
 */

#ifndef __IO_PROJECT_H__
#define __IO_PROJECT_H__

#include "utils/types.h"

#include <glib.h>

TYPEDEF_STRUCT (Project);

char *
project_serialize_to_json_str (const Project * project, GError ** error);

Project *
project_deserialize_from_json_str (const char * json, GError ** error);

#endif // __IO_PROJECT_H__
