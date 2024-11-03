// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CV_PORT_H__
#define __AUDIO_CV_PORT_H__

#include "common/dsp/port.h"
#include "common/utils/icloneable.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * @brief CV port specifics.
 */
class CVPort final
    : public QObject,
      public Port,
      public ICloneable<CVPort>,
      public ISerializable<CVPort>
{
  Q_OBJECT
  QML_ELEMENT
public:
  CVPort ();
  CVPort (std::string label, PortFlow flow);

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