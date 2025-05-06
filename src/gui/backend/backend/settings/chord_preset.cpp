// SPDX-FileCopyrightText: © 2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/settings/chord_preset.h"

ChordPreset::ChordPreset (QObject * parent) : QObject (parent) { }

ChordPreset::ChordPreset (const ChordPreset::NameT &name, QObject * parent)
    : ChordPreset (parent)
{
  name_ = name;
}

void
ChordPreset::init_after_cloning (
  const ChordPreset &other,
  ObjectCloneType    clone_type)
{
  name_ = other.name_;
  descr_ = other.descr_;
}

utils::Utf8String
ChordPreset::get_info_text () const
{
  auto str = utils::Utf8String::from_qstring (QObject::tr ("Chords"));
  str += u8":\n";
  const auto joined_str = fmt::format (
    "{}",
    fmt::join (
      descr_ | std::views::filter ([] (const auto &descr) {
        return descr.type_ != dsp::ChordType::None;
      }) | std::views::transform ([] (const auto &descr) {
        return descr.to_string ().str ();
      }),
      ", "));

  return str + utils::Utf8String::from_utf8_encoded_string (joined_str);
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
