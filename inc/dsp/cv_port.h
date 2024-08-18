// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CV_PORT_H__
#define __AUDIO_CV_PORT_H__

#include "dsp/port.h"
#include "utils/icloneable.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * @brief CV port specifics.
 */
class CVPort final
    : public Port,
      public ICloneable<CVPort>,
      public ISerializable<CVPort>
{
public:
  CVPort ()
  {
    minf_ = -1.f;
    maxf_ = 1.f;
    zerof_ = 0.f;
  };

  CVPort (std::string label, PortFlow flow)
      : Port (label, PortType::CV, flow, -1.f, 1.f, 0.f)
  {
  }

  bool has_sound () const override;

  void
  process (const EngineProcessTimeInfo time_nfo, const bool noroll) override;

  void allocate_bufs () override;

  void clear_buffer (AudioEngine &engine) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

  void init_after_cloning (const CVPort &other) override;
};

/**
 * @}
 */

#endif