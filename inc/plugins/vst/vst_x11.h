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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
    Copyright (C) 2012 Paul Davis
    Based on code by Paul Davis, Torben Hohn as part of FST

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * \file
 *
 * Engine for handling VST plugins on X11.
 */

#include "config.h"

#ifdef HAVE_X11
#ifndef __PLUGINS_VST_VST_X11_H__
#define __PLUGINS_VST_VST_X11_H__

typedef struct VstPlugin VstPlugin;

/**
 * @addtogroup vst
 *
 * @{
 */

/**
 * Inits the engine.
 */
int
vst_x11_init (void* ptr);

/**
 * Closes the engine.
 */
void
vst_x11_exit (void);

/**
 * Adds a new plugin instance to the linked list.
 */
int
vst_x11_run_editor (
  VstPlugin * vstfx);

/**
 * Destroy the editor window.
 */
void
vst_x11_destroy_editor (
  VstPlugin* vstfx);

/**
 * @}
 */

#endif
#endif // HAVE_X11
