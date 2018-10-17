/*
 * utils/io.h - IO utils
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 */
#ifndef __UTILS_IO_H__
#define __UTILS_IO_H__


/**
 * Gets system file separator. MUST be freed.
 */
char *
io_get_separator (); ///< string to write to

/**
 * Gets directory part of filename. MUST be freed.
 */
char *
io_get_dir (const char * filename); ///< filename containing directory

/**
 * Makes directory if doesn't exist.
 */
void
io_mkdir (const char * dir);

#endif

