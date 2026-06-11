// SPDX-FileCopyrightText: © 2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/settings/chord_preset.h"
#include "utils/serialization.h"

#include <nlohmann/json.hpp>

using namespace zrythm;

ChordPreset::ChordPreset (QObject * parent) : QObject (parent) { }

ChordPreset::ChordPreset (const ChordPreset::NameT &name, QObject * parent)
    : ChordPreset (parent)
{
  name_ = name;
}

void
init_from (
  ChordPreset           &obj,
  const ChordPreset     &other,
  utils::ObjectCloneType clone_type)
{
  obj.name_ = other.name_;
  obj.descr_.clear ();
  for (const auto &ptr : other.descr_)
    {
      auto new_descr = utils::make_qobject_unique<dsp::ChordDescriptor> (
        ptr->rootNote (), ptr->chordType (), ptr->chordAccent (),
        ptr->inversion (),
        ptr->hasBass ()
          ? std::optional<dsp::MusicalNote> (ptr->bassNote ())
          : std::nullopt);
      obj.descr_.push_back (std::move (new_descr));
    }
}

utils::Utf8String
ChordPreset::get_info_text () const
{
  auto str = utils::Utf8String::from_qstring (QObject::tr ("Chords"));
  str += u8":\n";
  const auto joined_str = utils::Utf8String::join (
    descr_ | std::views::filter ([] (const auto &descr_ptr) {
      return descr_ptr->chordType () != dsp::ChordType::None;
    }) | std::views::transform ([] (const auto &descr_ptr) {
      return descr_ptr->to_string ();
    }),
    u8", ");

  return str + joined_str;
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

void
to_json (nlohmann::json &j, const ChordPreset &preset)
{
  j[ChordPreset::kNameKey] = preset.name_;
  nlohmann::json descr_arr = nlohmann::json::array ();
  for (const auto &ptr : preset.descr_)
    {
      nlohmann::json descr_json;
      to_json (descr_json, *ptr);
      descr_arr.push_back (descr_json);
    }
  j[ChordPreset::kDescriptorsKey] = descr_arr;
}
void
from_json (const nlohmann::json &j, ChordPreset &preset)
{
  j.at (ChordPreset::kNameKey).get_to (preset.name_);
  preset.descr_.clear ();
  for (const auto &descr_json : j.at (ChordPreset::kDescriptorsKey))
    {
      auto ptr = utils::make_qobject_unique<dsp::ChordDescriptor> ();
      from_json (descr_json, *ptr);
      preset.descr_.push_back (std::move (ptr));
    }
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
