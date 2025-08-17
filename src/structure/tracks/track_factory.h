// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "gui/backend/backend/settings_manager.h"
#include "plugins/plugin_all.h"
#include "structure/tracks/track_all.h"

namespace zrythm::structure::tracks
{

/**
 * @brief Factory for tracks.
 */
class TrackFactory : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

  using ArrangerObjectRegistry = arrangement::ArrangerObjectRegistry;

public:
  TrackFactory () = delete;
  TrackFactory (
    FinalTrackDependencies track_deps,
    gui::SettingsManager  &settings_mgr,
    QObject *              parent = nullptr)
      : QObject (parent), track_deps_ (std::move (track_deps)),
        settings_manager_ (settings_mgr)
  {
  }

  static TrackFactory * get_instance ();

  template <typename TrackT> class Builder
  {
    friend class TrackFactory;

  private:
    explicit Builder (FinalTrackDependencies track_deps)
        : track_deps_ (std::move (track_deps))
    {
    }

    Builder &with_settings_manager (gui::SettingsManager &settings_manager)
    {
      settings_manager_ = settings_manager;
      return *this;
    }

  public:
    std::unique_ptr<TrackT> build_for_deserialization () const
    {
      return std::make_unique<TrackT> (track_deps_);
    }

    auto build ()
    {
      auto obj_ref = [&] () {
        return track_deps_.track_registry_.create_object<TrackT> (track_deps_);
      }();

      // auto * obj = std::get<PluginT *> (obj_ref.get_object ());

      return obj_ref;
    }

  private:
    FinalTrackDependencies            track_deps_;
    OptionalRef<gui::SettingsManager> settings_manager_;
  };

  template <typename TrackT> auto get_builder () const
  {
    auto builder =
      Builder<TrackT> (track_deps_).with_settings_manager (settings_manager_);
    return builder;
  }

private:
  template <FinalTrackSubclass TrackT>
  TrackUuidReference create_empty_track () const
  {
    auto obj_ref = get_builder<TrackT> ().build ();
    return obj_ref;
  }

  // temporary hack to access create_empty_track()
  friend class gui::actions::TracklistSelectionsAction;

  auto create_empty_track (Track::Type type) const
  {
    switch (type)
      {
      case Track::Type::Audio:
        return create_empty_track<AudioTrack> ();
      case Track::Type::Midi:
        return create_empty_track<MidiTrack> ();
      case Track::Type::MidiGroup:
        return create_empty_track<MidiGroupTrack> ();
      case Track::Type::Folder:
        return create_empty_track<FolderTrack> ();
      case Track::Type::Instrument:
        return create_empty_track<InstrumentTrack> ();
      case Track::Type::Master:
        return create_empty_track<MasterTrack> ();
      case Track::Type::Chord:
        return create_empty_track<ChordTrack> ();
      case Track::Type::Marker:
        return create_empty_track<MarkerTrack> ();
      case Track::Type::Modulator:
        return create_empty_track<ModulatorTrack> ();
      case Track::Type::AudioBus:
        return create_empty_track<AudioBusTrack> ();
      case Track::Type::MidiBus:
        return create_empty_track<MidiBusTrack> ();
      case Track::Type::AudioGroup:
        return create_empty_track<AudioGroupTrack> ();
      }
  }

public:
  template <FinalTrackSubclass TrackT>
  auto add_empty_track (Tracklist &tracklist)
  {
    auto track_ref = create_empty_track<TrackT> ();
// TODO
#if 0
    tracklist.append_track (
      track_ref,
      *audio_engine_, false, false);
#endif
    return track_ref;
  }

  Q_INVOKABLE QVariant addEmptyTrackFromType (Track::Type tt);

  template <typename TrackT>
  auto clone_new_object_identity (const TrackT &other) const
  {
    return track_deps_.plugin_registry_.clone_object (
      other, track_deps_.plugin_registry_);
  }

  template <typename TrackT>
  auto clone_object_snapshot (const TrackT &other, QObject &owner) const
  {
    TrackT * new_obj{};

    new_obj = other.clone_qobject (
      &owner, utils::ObjectCloneType::Snapshot, track_deps_.plugin_registry_);
    return new_obj;
  }

private:
  FinalTrackDependencies track_deps_;
  gui::SettingsManager  &settings_manager_;
};

} // namespace zrythm::structure::tracks
