/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Error codes.
 */

#ifndef __UTILS_ERROR_CODES_H__
#define __UTILS_ERROR_CODES_H__

typedef enum ErrorCode
{
  ERR_PLUGIN_INSTANTIATION_FAILED = 1,
  ERR_OBJECT_IS_NULL,
  ERR_PORT_MAGIC_FAILED,
  ERR_FAILED_TO_LOAD_STATE_FROM_FILE,
} ErrorCode;

/**
 * Return the message corresponding to the given
 * error code.
 */
const char *
error_code_get_message (
  ErrorCode err_code);

#endif
