/*
 * audio/ports_manager.h - manages all ports created for/by plugins
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

#ifndef __AUDIO_PORT_MANAGER_H__
#define __AUDIO_PORT_MANAGER_H__

struct Port;

typedef struct _Port_Manager
{
  Port          * ports;             ///< ports array
  int        pcount;          ///< ports counter

} Port_Manager;

void
init_port_manager ();


#endif
