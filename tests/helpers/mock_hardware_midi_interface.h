// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/hardware_midi_interface.h"

namespace zrythm::test_helpers
{

class MockHardwareMidiInterface : public dsp::IHardwareMidiInterface
{
public:
  void
  set_device_change_callback (std::optional<DeviceChangeCallback> cb) override
  {
    callback_ = std::move (cb);
  }

  BufferMap device_buffers () const override { return current_buffers_; }

  void simulate_device_change (BufferMap buffers)
  {
    current_buffers_ = std::move (buffers);
    if (callback_)
      {
        (*callback_) ();
      }
  }

private:
  std::optional<DeviceChangeCallback> callback_;
  BufferMap                           current_buffers_;
};

}
