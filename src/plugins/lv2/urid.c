/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/lv2_plugin.h"
#include "plugins/lv2/urid.h"
#include "utils/sem.h"

LV2_URID
urid_map_uri (
  LV2_URID_Map_Handle handle,
  const char*         uri)
{
  Lv2Plugin * plugin = (Lv2Plugin *) handle;
  zix_sem_wait (&plugin->symap_lock);
  const LV2_URID id = symap_map (plugin->symap, uri);
  zix_sem_post (&plugin->symap_lock);
  return id;
}

const char *
urid_unmap_uri (
  LV2_URID_Unmap_Handle handle,
  LV2_URID              urid)
{
  Lv2Plugin * plugin = (Lv2Plugin *) handle;
  zix_sem_wait (&plugin->symap_lock);
  const char* uri = symap_unmap (plugin->symap, urid);
  zix_sem_post (&plugin->symap_lock);
  return uri;
}

