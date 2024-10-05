// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * The control room backend.
 */
#ifndef __AUDIO_CONTROL_ROOM_H__
#define __AUDIO_CONTROL_ROOM_H__

#include "dsp/fader.h"
#include "utils/icloneable.h"

class Fader;
class AudioEngine;
class ExtPort;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define CONTROL_ROOM (AUDIO_ENGINE->control_room_)

/**
 * The control room allows to specify how Listen will work on each Channel and
 * to set overall volume after the Master Channel so you can change the volume
 * without touching the Master Fader.
 */
class ControlRoom final
    : public ICloneable<ControlRoom>,
      public ISerializable<ControlRoom>
{
public:
  ControlRoom () = default;
  ControlRoom (AudioEngine * engine);

  bool is_in_active_project () const;

  /**
   * Inits the control room from a project.
   */
  void init_loaded (AudioEngine * engine);

  /**
   * Sets dim_output to on/off and notifies interested parties.
   */
  void set_dim_output (bool dim_output) { dim_output_ = dim_output; }

  void init_after_cloning (const ControlRoom &other) override
  {
    monitor_fader_ = other.monitor_fader_->clone_unique ();
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_common ();

public:
  /**
   * The volume to set muted channels to when
   * soloing/muting.
   */
  std::unique_ptr<Fader> mute_fader_;

  /**
   * The volume to set listened channels to when
   * Listen is enabled on a Channel.
   */
  std::unique_ptr<Fader> listen_fader_;

  /**
   * The volume to set other channels to when Listen
   * is enabled on a Channel, or the monitor when
   * dim is enabled.
   */
  std::unique_ptr<Fader> dim_fader_;

  /** Dim the output volume. */
  bool dim_output_ = false;

  /**
   * Monitor fader.
   *
   * The Master stereo out should connect to this.
   *
   * @note This needs to be serialized because some ports connect to it.
   */
  std::unique_ptr<Fader> monitor_fader_;

  std::string hw_out_l_id_;
  std::string hw_out_r_id_;

  /* caches */
  ExtPort * hw_out_l_ = nullptr;
  ExtPort * hw_out_r_ = nullptr;

  /** Pointer to owner audio engine, if any. */
  AudioEngine * audio_engine_ = nullptr;
};

/**
 * @}
 */

#endif
