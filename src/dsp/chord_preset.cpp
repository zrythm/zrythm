// SPDX-FileCopyrightText: © 2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_preset.h"
#include "utils/serialization.h"

#include <nlohmann/json.hpp>

using namespace zrythm;

ChordPreset::ChordPreset (QObject * parent) : QObject (parent) { }

ChordPreset::ChordPreset (const QString &name, QObject * parent)
    : QObject (parent), name_ (name)
{
}

ChordPreset::ChordPreset (
  const QString &name,
  const QString &category,
  bool           is_builtin,
  QObject *      parent)
    : QObject (parent), name_ (name), category_ (category),
      is_builtin_ (is_builtin)
{
}

void
init_from (
  ChordPreset           &obj,
  const ChordPreset     &other,
  utils::ObjectCloneType clone_type)
{
  obj.name_ = other.name_;
  obj.category_ = other.category_;
  obj.is_builtin_ = other.is_builtin_;
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

void
ChordPreset::addDescriptor (
  utils::QObjectUniquePtr<dsp::ChordDescriptor> &&descr)
{
  descr_.push_back (std::move (descr));
}

QString
ChordPreset::infoText () const
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

  return (str + joined_str).to_qstring ();
}

bool
operator== (const ChordPreset &lhs, const ChordPreset &rhs)
{
  if (lhs.name_ != rhs.name_ || lhs.descr_.size () != rhs.descr_.size ())
    return false;
  for (size_t i = 0; i < lhs.descr_.size (); ++i)
    {
      if (!lhs.descr_[i]->isEquivalent (*rhs.descr_[i]))
        return false;
    }
  return true;
}

QString
ChordPreset::name () const
{
  return name_;
}

void
ChordPreset::setName (const QString &name)
{
  if (name_ == name)
    return;

  name_ = name;
  Q_EMIT nameChanged (name_);
}

QString
ChordPreset::category () const
{
  return category_;
}

void
ChordPreset::setCategory (const QString &category)
{
  if (category_ == category)
    return;

  category_ = category;
  Q_EMIT categoryChanged (category_);
}

void
to_json (nlohmann::json &j, const ChordPreset &preset)
{
  j[ChordPreset::kNameKey] = preset.name_;
  j[ChordPreset::kCategoryKey] = preset.category_;
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
  if (j.contains (ChordPreset::kCategoryKey))
    j.at (ChordPreset::kCategoryKey).get_to (preset.category_);
  preset.descr_.clear ();
  for (const auto &descr_json : j.at (ChordPreset::kDescriptorsKey))
    {
      auto ptr = utils::make_qobject_unique<dsp::ChordDescriptor> ();
      from_json (descr_json, *ptr);
      preset.descr_.push_back (std::move (ptr));
    }
}
