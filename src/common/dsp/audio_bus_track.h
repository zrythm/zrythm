// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_BUS_TRACK_H__
#define __AUDIO_AUDIO_BUS_TRACK_H__

#include "common/dsp/channel_track.h"

/**
 * @brief An audio bus track that can be processed and has channels.
 */
class AudioBusTrack final
    : public QObject,
      public ChannelTrack,
      public ICloneable<AudioBusTrack>,
      public ISerializable<AudioBusTrack>,
      public InitializableObjectFactory<AudioBusTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (AudioBusTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (AudioBusTrack)

  friend class InitializableObjectFactory<AudioBusTrack>;

public:
  void init_after_cloning (const AudioBusTrack &other) override;

  void init_loaded () override;

  bool validate () const override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  AudioBusTrack (const std::string &name = "", int pos = 0);

  bool initialize () override;
};

#endif // __AUDIO_AUDIO_BUS_TRACK_H__
