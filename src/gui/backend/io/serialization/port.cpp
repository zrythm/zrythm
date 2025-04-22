// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/audio_port.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/cv_port.h"
#include "gui/dsp/ext_port.h"
#include "gui/dsp/midi_port.h"
#include "gui/dsp/port.h"
#include "gui/dsp/port_connection.h"
#include "gui/dsp/port_connections_manager.h"

void
PortRange::define_fields (const utils::serialization::Context &ctx)
{
  serialize_fields (
    ctx, make_field ("minf", minf_), make_field ("maxf", maxf_),
    make_field ("zerof", zerof_));
}

void
Port::define_base_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<Port>;
  T::serialize_fields (
    ctx, T::make_field ("id", id_),
    T::make_field ("exposedToBackend", exposed_to_backend_));
}

void
ControlPort::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<ControlPort>;
  T::call_all_base_define_fields<Port> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("control", control_), T::make_field ("range", range_),
    T::make_field ("deff", deff_),
    T::make_field ("carlaParameterId", carla_param_id_),
    T::make_field ("baseValue", base_value_));

  if (ctx.is_deserializing ())
    {
      unsnapped_control_ = control_;
    }
}

void
MidiPort::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<MidiPort>;
  T::call_all_base_define_fields<Port> (ctx);
}

void
CVPort::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<CVPort>;
  T::call_all_base_define_fields<Port> (ctx);
  T::serialize_fields (ctx, T::make_field ("range", range_));
}

void
AudioPort::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<AudioPort>;
  T::call_all_base_define_fields<Port> (ctx);
}

#if 0
void
ControlPort::deserialize (Context ctx)
{
  Port::deserialize_common (ctx);
  yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);
  deserialize_field (it, "control", control_, ctx);
  deserialize_field (it, "minf", minf_, ctx);
  deserialize_field (it, "maxf", maxf_, ctx);

  /* this was incorrectly serialized in the past */
  std::string zerof_str = "zerof";
  if (ctx.format_major_version_ == 1 && ctx.format_minor_version_ < 11)
    zerof_str = "zerof_";
  deserialize_field (it, zerof_str.c_str (), zerof_, ctx);

  deserialize_field (it, "deff", deff_, ctx);
  deserialize_field (it, "carlaParameterId", carla_param_id_, ctx);

  /* added in 1.7 */
  deserialize_field (it, "baseValue", base_value_, ctx, true);

  unsnapped_control_ = control_;
}
#endif

#if 0
void
CVPort::deserialize (Context ctx)
{
  Port::deserialize_common (ctx);
  yyjson_obj_iter it = yyjson_obj_iter_with (ctx.obj_);
  deserialize_field (it, "minf", minf_, ctx);
  deserialize_field (it, "maxf", maxf_, ctx);

  /* this was incorrectly serialized in the past */
  std::string zerof_str = "zerof";
  if (ctx.format_major_version_ == 1 && ctx.format_minor_version_ < 11)
    zerof_str = "zerof_";
  deserialize_field (it, zerof_str.c_str (), zerof_, ctx);
}
#endif

void
ExtPort::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<ExtPort>;
  T::serialize_fields (
    ctx, T::make_field ("fullName", full_name_),
    T::make_field ("shortName", short_name_, true),
    T::make_field ("alias1", alias1_, true),
    T::make_field ("alias2", alias2_, true),
    T::make_field ("rtAudioDeviceName", rtaudio_dev_name_, true),
    T::make_field ("numAliases", num_aliases_),
    T::make_field ("isMidi", is_midi_), T::make_field ("type", type_),
    T::make_field ("rtAudioChannelIndex", rtaudio_channel_idx_));
}

void
PortConnection::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<PortConnection>;
  T::serialize_fields (
    ctx, T::make_field ("srcId", src_id_), T::make_field ("destId", dest_id_),
    T::make_field ("multiplier", multiplier_),
    T::make_field ("enabled", enabled_), T::make_field ("locked", locked_),
    // note: this is only needed for CV ports
    T::make_field ("baseValue", base_value_));
}

void
PortConnectionsManager::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<PortConnectionsManager>;
  T::serialize_fields (ctx, T::make_field ("connections", connections_));

  if (ctx.is_deserializing ())
    regenerate_hashtables ();
}
