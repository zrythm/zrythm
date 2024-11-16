// SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/chord_preset.h"
#include "gui/backend/backend/settings/chord_preset_pack_manager.h"
#include "gui/backend/backend/zrythm.h"

ChordPreset::ChordPreset (QObject * parent) : QObject (parent) { }

ChordPreset::ChordPreset (const ChordPreset::NameT &name, QObject * parent)
    : ChordPreset (parent)
{
  name_ = name;
}

void
ChordPreset::init_after_cloning (const ChordPreset &other)
{
  name_ = other.name_;
  descr_ = other.descr_;
}

std::string
ChordPreset::get_info_text () const
{
  std::string str = QObject::tr ("Chords").toStdString ();
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

ChordPreset::NameT
ChordPreset::getName () const
{
  return name_;
}

void
ChordPreset::setName (const ChordPreset::NameT &name)
{
  if (name_ == name)
    return;

  name_ = name;
  Q_EMIT nameChanged (name_);
}

#if 0
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
    menu, z_gtk_create_menu_item (QObject::tr ("_Rename"), "edit-rename", action));

  /* delete */
  sprintf (action, "app.delete-chord-preset::%p", this);
  g_menu_append_item (
    menu, z_gtk_create_menu_item (QObject::tr ("_Delete"), "edit-delete", action));

  return G_MENU_MODEL (menu);
}
#endif