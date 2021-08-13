/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Vamp plugin utils.
 *
 * See https://code.soundsoftware.ac.uk/projects/vamp-plugin-sdk/repository/entry/vamp/vamp.h
 * and https://code.soundsoftware.ac.uk/projects/vamp-plugin-sdk/repository/entry/vamp-hostsdk/host-c.h
 * for API.
 */

#ifndef __UTILS_VAMP_H__
#define __UTILS_VAMP_H__

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Prints detected vamp plugins.
 */
void
vamp_print_all (void);

/**
 * @}
 */

#endif // __UTILS_VAMP_H__
