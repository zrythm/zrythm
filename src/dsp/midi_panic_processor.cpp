// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_panic_processor.h"
#include "dsp/midi_port.h"
#include "utils/registry_utils.h"

namespace zrythm::dsp
{

MidiPanicProcessor::MidiPanicProcessor (
  utils::IObjectRegistry &registry,
  QObject *               parent)
    : QObject (parent), dsp::ProcessorBase (registry, u8"MIDI Panic")
{
  add_output_port (
    utils::create_object<dsp::MidiPort> (
      registry, u8"Panic MIDI out", dsp::PortFlow::Output));
}

} // namespace zrythm::dsp
