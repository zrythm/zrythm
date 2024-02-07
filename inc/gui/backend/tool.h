/*
 * SPDX-FileCopyrightText: © 2018-2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 */

#ifndef __GUI_BACKEND_TOOL_H__
#define __GUI_BACKEND_TOOL_H__

#define P_TOOL PROJECT->tool

typedef enum Tool
{
  TOOL_SELECT_NORMAL,
  TOOL_SELECT_STRETCH,
  TOOL_EDIT,
  TOOL_CUT,
  TOOL_ERASER,
  TOOL_RAMP,
  TOOL_AUDITION,
} Tool;

#endif
