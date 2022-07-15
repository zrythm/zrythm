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

#include "plugins/lv2/lv2_urid.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_manager.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "zix/sem.h"

LV2_URID
lv2_urid_map_uri (LV2_URID_Map_Handle handle, const char * uri)
{
  zix_sem_wait (&PM_SYMAP_LOCK);
  const LV2_URID id = symap_map (PM_SYMAP, uri);
  zix_sem_post (&PM_SYMAP_LOCK);
  return id;
}

const char *
lv2_urid_unmap_uri (LV2_URID_Unmap_Handle handle, LV2_URID urid)
{
  zix_sem_wait (&PM_SYMAP_LOCK);
  const char * uri = symap_unmap (PM_SYMAP, urid);
  zix_sem_post (&PM_SYMAP_LOCK);
  return uri;
}
