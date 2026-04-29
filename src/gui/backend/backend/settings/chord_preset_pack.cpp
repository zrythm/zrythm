// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "gui/backend/backend/settings/chord_preset_pack.h"
#include "utils/serialization.h"

#include <nlohmann/json.hpp>

ChordPresetPack::ChordPresetPack (QObject * parent) : QObject (parent) { }

ChordPresetPack::
  ChordPresetPack (const NameT &name, bool is_standard, QObject * parent)
    : ChordPresetPack (parent)
{
  name_ = name;
  is_standard_ = is_standard;
}

void
init_from (
  ChordPresetPack               &obj,
  const ChordPresetPack         &other,
  zrythm::utils::ObjectCloneType clone_type)
{
  obj.name_ = other.name_;
  obj.is_standard_ = other.is_standard_;
  obj.presets_ = other.presets_;
  for (auto &pset : obj.presets_)
    {
      pset->pack_ = &obj;
    }
}

void
ChordPresetPack::add_preset (const ChordPreset &pset)
{
  auto * clone = zrythm::utils::clone_qobject (pset, this);
  clone->pack_ = this;
  presets_.push_back (clone);
}

int
ChordPresetPack::get_preset_index (const ChordPreset &pset) const
{
  auto it = std::ranges::find (presets_, &pset);
  assert (it != presets_.end ());
  return static_cast<int> (std::distance (presets_.begin (), it));
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

void
to_json (nlohmann::json &j, const ChordPresetPack &pack)
{
  j[ChordPresetPack::kNameKey] = pack.name_;
  j[ChordPresetPack::kPresetsKey] = pack.presets_;
  j[ChordPresetPack::kIsStandardKey] = pack.is_standard_;
}
void
from_json (const nlohmann::json &j, ChordPresetPack &pack)
{
  j.at (ChordPresetPack::kNameKey).get_to (pack.name_);
  for (const auto &pset : j.at (ChordPresetPack::kPresetsKey))
    {
      // TODO: check if this is correct
      auto * pset_ptr = new ChordPreset (&pack);
      from_json (pset, *pset_ptr);
      pack.presets_.push_back (pset_ptr);
    }
  j.at (ChordPresetPack::kIsStandardKey).get_to (pack.is_standard_);
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
