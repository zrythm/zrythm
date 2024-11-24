// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_FOLDER_TRACK_H__
#define __AUDIO_FOLDER_TRACK_H__

#include "gui/dsp/channel_track.h"
#include "gui/dsp/foldable_track.h"

/**
 * @brief A track that can contain other tracks.
 */
class FolderTrack final
    : public QObject,
      public FoldableTrack,
      // public ChannelTrack,
      public ICloneable<FolderTrack>,
      public zrythm::utils::serialization::ISerializable<FolderTrack>,
      public InitializableObjectFactory<FolderTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (FolderTrack)

  friend class InitializableObjectFactory<FolderTrack>;

public:
  bool validate () const override;

  bool get_listened () const override
  {
    return is_status (MixerStatus::Listened);
  }

  bool get_muted () const override { return is_status (MixerStatus::Muted); }

  bool get_implied_soloed () const override
  {
    return is_status (MixerStatus::ImpliedSoloed);
  }

  bool get_soloed () const override { return is_status (MixerStatus::Soloed); }
  void init_loaded () override;

  void init_after_cloning (const FolderTrack &other) override;
  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  FolderTrack (const std::string &name = "", int pos = 0);

  bool initialize () override;
};

#endif /* __AUDIO_FOLDER_TRACK_H__ */
