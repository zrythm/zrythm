// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_INSTRUMENT_TRACK_H__
#define __AUDIO_INSTRUMENT_TRACK_H__

#include "common/dsp/group_target_track.h"
#include "common/dsp/piano_roll_track.h"
#include "common/utils/object_factory.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

class InstrumentTrack final
    : public QObject,
      public GroupTargetTrack,
      public PianoRollTrack,
      public ICloneable<InstrumentTrack>,
      public ISerializable<InstrumentTrack>,
      public InitializableObjectFactory<InstrumentTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (InstrumentTrack)
  DEFINE_LANED_TRACK_QML_PROPERTIES (InstrumentTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (InstrumentTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (InstrumentTrack)

  friend class InitializableObjectFactory<InstrumentTrack>;

public:
  void init_loaded () override;

  void init_after_cloning (const InstrumentTrack &other) override;

  zrythm::plugins::Plugin * get_instrument ();

  const zrythm::plugins::Plugin * get_instrument () const;

  /**
   * Returns if the first plugin's UI in the instrument track is visible.
   */
  bool is_plugin_visible () const;

  /**
   * Toggles whether the first plugin's UI in the instrument Track is visible.
   */
  void toggle_plugin_visible ();

  bool validate () const override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  /**
   * @brief Main constructor.
   *
   * @param name Track name.
   * @param pos Track position.
   */
  InstrumentTrack (const std::string &name = "", int pos = 0);

  bool initialize () override;

public:
};

/**
 * @}
 */

#endif // __AUDIO_INSTRUMENT_TRACK_H__
