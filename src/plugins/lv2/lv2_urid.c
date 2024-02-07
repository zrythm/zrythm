/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
