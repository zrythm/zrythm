// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_HARDWARE_PROCESSOR_H__
#define __AUDIO_HARDWARE_PROCESSOR_H__

#include "common/dsp/audio_port.h"
#include "common/dsp/ext_port.h"
#include "common/dsp/midi_port.h"
#include "common/utils/icloneable.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

class AudioEngine;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define HW_IN_PROCESSOR (AUDIO_ENGINE->hw_in_processor_)
#define HW_OUT_PROCESSOR (AUDIO_ENGINE->hw_out_processor_)

/**
 * Hardware processor.
 */
class HardwareProcessor final
    : public ICloneable<HardwareProcessor>,
      public ISerializable<HardwareProcessor>
{
public:
  // Rule of 0
  HardwareProcessor () = default;

  HardwareProcessor (bool input, AudioEngine * engine);

  void init_loaded (AudioEngine * engine);

  bool is_in_active_project () const;

  /**
   * Rescans the hardware ports and appends any missing ones.
   *
   * @note This is a GSource callback.
   */
  static bool rescan_ext_ports (HardwareProcessor * self);

  /**
   * Finds an ext port from its ID (type + full name).
   *
   * @see ExtPort.get_id()
   */
  ExtPort * find_ext_port (const std::string &id);

  /**
   * Finds a port from its ID (type + full name).
   *
   * @see ExtPort.get_id()
   */
  template <typename T = Port> T * find_port (const std::string &id);

  /**
   * Sets up the ports but does not start them.
   */
  void setup ();

  /**
   * Starts or stops the ports.
   *
   * @param activate True to activate, false to deactivate
   */
  void activate (bool activate);

  /**
   * Processes the data.
   */
  ATTR_REALTIME
  void process (nframes_t nframes);

  /**
   * To be used during serialization.
   */
  void init_after_cloning (const HardwareProcessor &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  template <typename T>
  std::unique_ptr<T>
  create_port_for_ext_port (const ExtPort &ext_port, PortFlow flow);

public:
  /**
   * Whether this is the processor at the start of the graph (input) or at the
   * end (output).
   */
  bool is_input_ = false;

  /**
   * Ports selected by the user in the preferences to enable.
   *
   * To be cached at startup (need restart for changes to take effect).
   *
   * This is only for inputs.
   */
  std::vector<std::string> selected_midi_ports_;
  std::vector<std::string> selected_audio_ports_;

  /**
   * All known external ports.
   */
  std::vector<std::unique_ptr<ExtPort>> ext_audio_ports_;
  std::vector<std::unique_ptr<ExtPort>> ext_midi_ports_;

  /**
   * Ports to be used by Zrythm, corresponding to the external ports.
   */
  std::vector<std::unique_ptr<AudioPort>> audio_ports_;
  std::vector<std::unique_ptr<MidiPort>>  midi_ports_;

  /** Whether set up already. */
  bool setup_ = false;

  /** Whether currently active. */
  bool activated_ = false;

  guint rescan_timeout_id_ = 0;

  /** Pointer to owner engine, if any. */
  AudioEngine * engine_ = nullptr;
};

extern template std::unique_ptr<MidiPort>
HardwareProcessor::create_port_for_ext_port (const ExtPort &, PortFlow);
extern template std::unique_ptr<AudioPort>
HardwareProcessor::create_port_for_ext_port (const ExtPort &, PortFlow);
extern template Port *
HardwareProcessor::find_port (const std::string &);

/**
 * @}
 */

#endif
