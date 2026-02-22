// SPDX-FileCopyrightText: © 2019-2021, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/clip_editor.h"
#include "structure/tracks/track_all.h"

namespace zrythm::structure::project
{

ClipEditor::ClipEditor (
  ArrangerObjectRegistry &reg,
  TrackResolver           track_resolver,
  QObject *               parent)
    : QObject (parent), object_registry_ (reg),
      track_resolver_ (std::move (track_resolver)),
      piano_roll_ (
        utils::make_qobject_unique<structure::arrangement::PianoRoll> (this)),
      audio_clip_editor_ (
        utils::make_qobject_unique<structure::arrangement::AudioClipEditor> (
          this)),
      automation_editor_ (
        utils::make_qobject_unique<structure::arrangement::AutomationEditor> (
          this)),
      chord_editor_ (
        utils::make_qobject_unique<structure::arrangement::ChordEditor> (this))
{
}

void
ClipEditor::init_loaded ()
{
}

void
ClipEditor::setRegion (QVariant region, QVariant track)
{
  auto * r = dynamic_cast<ArrangerObject *> (region.value<QObject *> ());
  auto * t =
    dynamic_cast<structure::tracks::Track *> (track.value<QObject *> ());
  set_region (r->get_uuid (), t->get_uuid ());
}

void
ClipEditor::unsetRegion ()
{
  if (!region_id_)
    return;

  region_id_.reset ();
  Q_EMIT regionChanged ({});
}

auto
ClipEditor::get_region_and_track () const
  -> std::optional<std::pair<ArrangerObjectPtrVariant, TrackPtrVariant>>
{
  auto region_var = std::visit (
    [&] (auto &&obj) -> ArrangerObjectPtrVariant {
      if constexpr (
        structure::arrangement::RegionObject<base_type<decltype (obj)>>)
        {
          return obj;
        }
      throw std::bad_variant_access ();
    },
    object_registry_.find_by_id_or_throw (region_id_->first));
  auto track_var = track_resolver_ (region_id_->second);
  return std::make_pair (region_var, track_var);
}

void
ClipEditor::init ()
{
  piano_roll_->init ();
  chord_editor_->init ();
  // the rest of the editors are initialized in their respective classes
}

} // namespace zrythm::structure::project
