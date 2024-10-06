// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/gtk.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/chord_preset.h"
#include "gui/backend/backend/settings/chord_preset_pack_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

ChordPreset::ChordPreset (const std::string &name) : ChordPreset ()
{
  name_ = name;
}

std::string
ChordPreset::get_info_text () const
{
  std::string str = _ ("Chords");
  str += ":\n";
  bool have_any = false;
  for (const auto &descr : descr_)
    {
      if (descr.type_ == ChordType::None)
        break;

      str += descr.to_string ();
      str += ", ";
      have_any = true;
    }

  if (have_any)
    {
      str.erase (str.length () - 2);
    }

  return str;
}

std::string
ChordPreset::get_name () const
{
  return name_;
}

void
ChordPreset::set_name (const std::string &name)
{
  name_ = name;

  EVENTS_PUSH (EventType::ET_CHORD_PRESET_EDITED, nullptr);
}

GMenuModel *
ChordPreset::generate_context_menu () const
{
  ChordPresetPack * pack =
    CHORD_PRESET_PACK_MANAGER->get_pack_for_preset (*this);
  z_return_val_if_fail (pack, nullptr);
  if (pack->is_standard_)
    return nullptr;

  GMenu * menu = g_menu_new ();
  char    action[800];

  /* rename */
  sprintf (action, "app.rename-chord-preset::%p", this);
  g_menu_append_item (
    menu, z_gtk_create_menu_item (_ ("_Rename"), "edit-rename", action));

  /* delete */
  sprintf (action, "app.delete-chord-preset::%p", this);
  g_menu_append_item (
    menu, z_gtk_create_menu_item (_ ("_Delete"), "edit-delete", action));

  return G_MENU_MODEL (menu);
}