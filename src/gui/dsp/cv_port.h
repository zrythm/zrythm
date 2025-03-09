// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CV_PORT_H__
#define __AUDIO_CV_PORT_H__

#include "gui/dsp/port.h"

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
    : public QObject,
      public Port,
      public ICloneable<CVPort>,
      public zrythm::utils::serialization::ISerializable<CVPort>
{
  Q_OBJECT
  QML_ELEMENT
public:
  CVPort ();
  CVPort (std::string label, PortFlow flow);

  bool has_sound () const override;

  void process (EngineProcessTimeInfo time_nfo, bool noroll) override;

  void allocate_bufs () override;

  void clear_buffer (AudioEngine &engine) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

  void
  init_after_cloning (const CVPort &other, ObjectCloneType clone_type) override;
};

/**
 * @}
 */

#endif
