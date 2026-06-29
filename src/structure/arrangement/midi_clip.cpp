// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/midi_clip.h"

namespace zrythm::structure::arrangement
{
MidiClip::MidiClip (
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  utils::IObjectRegistry     &object_registry,
  QObject *                   parent)
    : Clip (Type::MidiClip, tempo_map_wrapper, parent),
      ArrangerObjectOwner<MidiNote> (object_registry, *this),
      ArrangerObjectOwner<MidiControlEvent> (object_registry, *this)
{
  QObject::connect (
    midiNotes (), &ArrangerObjectListModel::contentChanged, this,
    &MidiClip::contentChanged);
  QObject::connect (
    midiControlEvents (), &ArrangerObjectListModel::contentChanged, this,
    &MidiClip::contentChanged);

  // Reconfigure warp whenever the effective timebase changes.
  QObject::connect (
    timebaseProvider (), &dsp::TimebaseProvider::effectiveTimebaseChanged, this,
    [this] () { update_warp_configuration (); });
}

void
MidiClip::set_source_bpm (units::bpm_t bpm)
{
  if (source_bpm_ != bpm)
    {
      source_bpm_ = bpm;
      update_warp_configuration ();
    }
}

void
MidiClip::update_warp_configuration ()
{
  auto * warp = contentWarp ();
  if (warp == nullptr)
    return;
  // Musical timebase OR no source BPM: identity warp (follows project tempo).
  // Absolute timebase with known BPM: source-anchored warp.
  if (
    timebaseProvider ()->effectiveTimebase () == dsp::Timebase::Musical
    || source_bpm_ <= units::bpm (0.))
    warp->configure_as_project (source_bpm_);
  else
    warp->configure_as_source (source_bpm_);
}

void
init_from (MidiClip &obj, const MidiClip &other, utils::ObjectCloneType clone_type)
{
  obj.source_bpm_ = other.source_bpm_;
  init_from (
    static_cast<Clip &> (obj), static_cast<const Clip &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<MidiNote> &> (obj),
    static_cast<const ArrangerObjectOwner<MidiNote> &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<MidiControlEvent> &> (obj),
    static_cast<const ArrangerObjectOwner<MidiControlEvent> &> (other),
    clone_type);
  // Reconfigure the warp to reflect the cloned source BPM and (possibly
  // inherited) timebase. The constructor's effectiveTimebaseChanged
  // connection won't fire on a freshly-built clone, so do it explicitly.
  obj.update_warp_configuration ();
}
}
