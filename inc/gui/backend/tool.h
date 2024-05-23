// clang-format off
// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 */

#ifndef __GUI_BACKEND_TOOL_H__
#define __GUI_BACKEND_TOOL_H__

#define P_TOOL PROJECT->tool

enum class Tool
{
  TOOL_SELECT,
  TOOL_EDIT,
  TOOL_CUT,
  TOOL_ERASER,
  TOOL_RAMP,
  TOOL_AUDITION,
};

#endif
