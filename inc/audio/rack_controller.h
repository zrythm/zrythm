/*
 * audio/rack_controller.h - Rack controller
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __AUDIO_RACK_CONTROLLER_H__
#define __AUDIO_RACK_CONTROLLER_H__

typedef struct Plugin Plugin;

typedef struct RackController
{
  int id;          ///< position in the rack
  int plugin_id;   ///< the plugin doing the work
  Plugin * plugin; ///< cache
} RackController;

#endif
