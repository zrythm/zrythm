// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/uuid_identifiable_object.h"
#include "utils/variant_helpers.h"

namespace zrythm::dsp
{
class Port;

/**
 * Direction of the signal.
 */
enum class PortFlow : std::uint_fast8_t
{
  Unknown,
  Input,
  Output
};

/**
 * Type of signals the Port handles.
 */
enum class PortType : std::uint_fast8_t
{
  Unknown,
  Audio,
  Midi,
  CV
};

using PortUuid = utils::UuidIdentifiableObject<Port>::Uuid;

class MidiPort;
class AudioPort;
class CVPort;
using PortVariant = std::variant<MidiPort, AudioPort, CVPort>;
using PortPtrVariant = to_pointer_variant<PortVariant>;

template <typename T>
concept FinalPortSubclass =
  std::is_same_v<T, MidiPort> || std::is_same_v<T, AudioPort>
  || std::is_same_v<T, CVPort>;
};

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::dsp::PortUuid)
