// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/chord_preset_pack.h"
#include "utils/gtk.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
ChordPresetPack::add_preset (const ChordPreset &pset)
{
  ChordPreset clone = pset;
  clone.pack_ = this;
  presets_.push_back (clone);
}

bool
ChordPresetPack::contains_name (const std::string &name) const
{
  for (const auto &pset : presets_)
    {
      if (string_is_equal_ignore_case (pset.name_, name))
        return true;
    }

  return false;
}

bool
ChordPresetPack::contains_preset (const ChordPreset &pset) const
{
  for (const auto &cur_pset : presets_)
    {
      if (pset == cur_pset)
        return true;
    }

  return false;
}

void
ChordPresetPack::delete_preset (const ChordPreset &pset)
{
  presets_.erase (
    std::remove_if (
      presets_.begin (), presets_.end (),
      [&pset] (const ChordPreset &p) { return p == pset; }),
    presets_.end ());

  EVENTS_PUSH (EventType::ET_CHORD_PRESET_REMOVED, nullptr);
}

void
ChordPresetPack::set_name (const std::string &name)
{
  name_ = name;

  EVENTS_PUSH (EventType::ET_CHORD_PRESET_PACK_EDITED, nullptr);
}

GMenuModel *
ChordPresetPack::generate_context_menu () const
{
  if (is_standard_)
    return nullptr;

  auto        menu = g_menu_new ();
  GMenuItem * menuitem;
  char        action[800];

  /* rename */
  sprintf (action, "app.rename-chord-preset-pack::%p", this);
  menuitem = z_gtk_create_menu_item (_ ("_Rename"), "edit-rename", action);
  g_menu_append_item (menu, menuitem);

  /* delete */
  sprintf (action, "app.delete-chord-preset-pack::%p", this);
  menuitem = z_gtk_create_menu_item (_ ("_Delete"), "edit-delete", action);
  g_menu_append_item (menu, menuitem);

  return G_MENU_MODEL (menu);
}