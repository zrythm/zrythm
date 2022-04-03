/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
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
lv2_log_printf (
  LV2_Log_Handle handle,
  LV2_URID       type,
  const char *   fmt,
  ...);

/**
 * @}
 */
