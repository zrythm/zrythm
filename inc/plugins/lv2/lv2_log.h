/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * LV2 Log feature implementations.
 */

#include <lv2/log/log.h>

/**
 * @addtogroup lv2
 *
 * @{
 */

/**
 * Needed because we can't set them directly when
 * gi18n.h is included.
 */
void
lv2_log_set_printf_funcs (LV2_Log_Log * log);

int
lv2_log_vprintf (
  LV2_Log_Handle handle,
  LV2_URID       type,
  const char *   fmt,
  va_list        ap);

int
lv2_log_printf (LV2_Log_Handle handle, LV2_URID type, const char * fmt, ...);

/**
 * @}
 */
