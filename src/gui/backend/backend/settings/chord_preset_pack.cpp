// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/rt_thread_id.h"
#include "common/utils/string.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/chord_preset_pack.h"
#include "gui/backend/backend/zrythm.h"

ChordPresetPack::ChordPresetPack (QObject * parent) : QObject (parent) { }

ChordPresetPack::
  ChordPresetPack (const NameT &name, bool is_standard, QObject * parent)
    : ChordPresetPack (parent)
{
  name_ = name;
  is_standard_ = is_standard;
}

void
ChordPresetPack::add_preset (const ChordPreset &pset)
{
  auto * clone = pset.clone_raw_ptr ();
  clone->pack_ = this;
  clone->setParent (this);
  presets_.push_back (clone);
}

bool
ChordPresetPack::contains_name (const ChordPresetPack::NameT &name) const
{
  return std::any_of (
    presets_.begin (), presets_.end (),
    [&name] (const auto &pset) { return pset->name_ == name; });
}

bool
ChordPresetPack::contains_preset (const ChordPreset &pset) const
{
  return std::any_of (
    presets_.begin (), presets_.end (),
    [&pset] (const auto &p) { return *p == pset; });
}

void
ChordPresetPack::delete_preset (const ChordPreset &pset)
{
  presets_.erase (
    std::remove_if (
      presets_.begin (), presets_.end (),
      [&pset] (const auto &p) {
        if (*p == pset)
          {
            p->setParent (nullptr);
            p->deleteLater ();
            return true;
          }
        return false;
      }),
    presets_.end ());

  /* EVENTS_PUSH (EventType::ET_CHORD_PRESET_REMOVED, nullptr); */
}

void
ChordPresetPack::setName (const ChordPresetPack::NameT &name)
{
  if (name_ == name)
    return;

  name_ = name;
  Q_EMIT nameChanged (name_);
}

#if 0
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
  menuitem = z_gtk_create_menu_item (QObject::tr ("_Rename"), "edit-rename", action);
  g_menu_append_item (menu, menuitem);

  /* delete */
  sprintf (action, "app.delete-chord-preset-pack::%p", this);
  menuitem = z_gtk_create_menu_item (QObject::tr ("_Delete"), "edit-delete", action);
  g_menu_append_item (menu, menuitem);

  return G_MENU_MODEL (menu);
}
#endif