// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 */

#ifndef __GUI_BACKEND_TOOL_H__
#define __GUI_BACKEND_TOOL_H__

#define P_TOOL (PROJECT->tool_)

enum class Tool
{
  Select,
  Edit,
  Cut,
  Eraser,
  Ramp,
  Audition,
};

#endif
