// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/chord_object.h"
#include "structure/arrangement/clip.h"

namespace zrythm::structure::arrangement
{
class ChordClip final : public Clip, public ArrangerObjectOwner<ChordObject>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordClip,
    chordObjects,
    ChordObject)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ChordClip (
    const dsp::TempoMapWrapper &tempo_map_wrapper,
    utils::IObjectRegistry     &registry,
    QObject *                   parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  // ========================================================================

  std::string
  get_field_name_for_serialization (const ChordObject *) const override
  {
    return "chordObjects";
  }

  std::vector<ArrangerObjectListModel *> get_child_list_models () const override
  {
    return { ArrangerObjectOwner<ChordObject>::get_model () };
  }

private:
  friend void init_from (
    ChordClip             &obj,
    const ChordClip       &other,
    utils::ObjectCloneType clone_type);

  friend void to_json (nlohmann::json &j, const ChordClip &clip)
  {
    to_json (j, static_cast<const Clip &> (clip));
    to_json (j, static_cast<const ArrangerObjectOwner &> (clip));
  }
  friend void from_json (const nlohmann::json &j, ChordClip &clip)
  {
    from_json (j, static_cast<Clip &> (clip));
    from_json (j, static_cast<ArrangerObjectOwner &> (clip));
  }

private:
  BOOST_DESCRIBE_CLASS (
    ChordClip,
    (Clip, ArrangerObjectOwner<ChordObject>),
    (),
    (),
    ())
};

}
