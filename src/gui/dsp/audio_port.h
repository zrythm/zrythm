// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_PORT_H__
#define __AUDIO_AUDIO_PORT_H__

#include "dsp/panning.h"
#include "gui/dsp/port.h"
#include "utils/icloneable.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

class PortConnectionsManager;

/**
 * @brief Audio port specifics.
 */
class AudioPort final
    : public QObject,
      public Port,
      public ICloneable<AudioPort>,
      public zrythm::utils::serialization::ISerializable<AudioPort>
{
  Q_OBJECT
  QML_ELEMENT

public:
  AudioPort ();

  AudioPort (std::string label, PortFlow flow);

  bool has_sound () const override;

  static constexpr size_t AUDIO_RING_SIZE = 65536;

  /**
   * @brief Applies the fader to the audio buffer.
   *
   * @param amp The fader amplitude (0.0 to 1.0).
   * @param start_frame The start frame offset from 0 in this cycle.
   * @param nframes The number of frames to process.
   */
  void apply_fader (float amp, nframes_t start_frame, nframes_t nframes);

  /**
   * @brief Applies the pan to the audio buffer.
   *
   * @param pan The pan value (-1.0 to 1.0).
   * @param pan_law The pan law to use.
   * @param pan_algo The pan algorithm to use.
   * @param start_frame The start frame offset from 0 in this cycle.
   * @param nframes The number of frames to process.
   */
  void apply_pan (
    float                     pan,
    zrythm::dsp::PanLaw       pan_law,
    zrythm::dsp::PanAlgorithm pan_algo,
    nframes_t                 start_frame,
    nframes_t                 nframes);

  /**
   * @brief Returns the peak amplitude of the audio buffer.
   *
   * @return The peak amplitude (0.0 to 1.0).
   */
  float get_peak () const { return peak_; }

  /**
   * @brief Resets the peak amplitude to 0.
   */
  void reset_peak () { peak_ = 0.f; }

  void process (EngineProcessTimeInfo time_nfo, bool noroll) override;

  void allocate_bufs () override;

  void clear_buffer (AudioEngine &engine) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

  bool is_stereo_port () const
  {
    return ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::StereoL)
           || ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::StereoR);
  }

  void init_after_cloning (const AudioPort &other, ObjectCloneType clone_type)
    override;

private:
  /**
   * Sums the inputs coming in from the dummy engine StereoPorts, before the
   * port is processed.
   */
  void sum_data_from_dummy (nframes_t start_frame, nframes_t nframes);

private:
  /** Max amplitude during processing (fabsf). */
  float peak_ = 0.f;

  /** Last time @ref peak_ was set. */
  qint64 peak_timestamp_ = 0;
};

/**
 * Convenience factory for L/R audio port pairs.
 */
class StereoPorts final
{
public:
  static constexpr std::pair<std::string, std::string>
  get_name_and_symbols (bool left, std::string name, std::string symbol)
  {
    return std::make_pair (
      fmt::format ("{} {}", name, left ? "L" : "R"),
      fmt::format ("{}_{}", symbol, left ? "l" : "r"));
  }

  static std::pair<AudioPort *, AudioPort *> create_stereo_ports (
    PortRegistry &port_registry,
    bool          input,
    std::string   name,
    std::string   symbol);
};

/**
 * @}
 */

#endif
