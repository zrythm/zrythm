// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/control_port.h"
#include "dsp/cv_port.h"
#include "dsp/ext_port.h"
#include "dsp/midi_port.h"
#include "dsp/port.h"
#include "dsp/port_connection.h"
#include "dsp/port_connections_manager.h"

void
PortIdentifier::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("label", label_), make_field ("symbol", sym_),
    make_field ("uri", uri_), make_field ("comment", comment_),
    make_field ("ownerType", owner_type_), make_field ("type", type_),
    make_field ("flow", flow_), make_field ("unit", unit_),
    make_field ("flags", flags_), make_field ("flags2", flags2_),
    make_field ("trackNameHash", track_name_hash_),
    make_field ("pluginId", plugin_id_), make_field ("portGroup", port_group_),
    make_field ("externalPortId", ext_port_id_),
    make_field ("portIndex", port_index_));
}

void
Port::define_base_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("id", id_),
    make_field ("exposedToBackend", exposed_to_backend_));
}

void
ControlPort::define_fields (const Context &ctx)
{
  using T = ISerializable<ControlPort>;
  T::call_all_base_define_fields<Port> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("control", control_), T::make_field ("minf", minf_),
    T::make_field ("maxf", maxf_), T::make_field ("zerof", zerof_),
    T::make_field ("deff", deff_),
    T::make_field ("carlaParameterId", carla_param_id_),
    T::make_field ("baseValue", base_value_));

  if (ctx.is_deserializing ())
    {
      unsnapped_control_ = control_;
    }
}

void
MidiPort::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiPort>;
  T::call_all_base_define_fields<Port> (ctx);
}

void
CVPort::define_fields (const Context &ctx)
{
  using T = ISerializable<CVPort>;
  T::call_all_base_define_fields<Port> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("minf", minf_), T::make_field ("maxf", maxf_),
    T::make_field ("zerof", zerof_));
}

void
AudioPort::define_fields (const Context &ctx)
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
StereoPorts::define_fields (const Context &ctx)
{
  using T = ISerializable<StereoPorts>;
  T::serialize_fields (ctx, T::make_field ("l", l_), T::make_field ("r", r_));
}

void
ExtPort::define_fields (const Context &ctx)
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
PortConnection::define_fields (const Context &ctx)
{
  using T = ISerializable<PortConnection>;
  T::serialize_fields (
    ctx, T::make_field ("srcId", src_id_), T::make_field ("destId", dest_id_),
    T::make_field ("multiplier", multiplier_),
    T::make_field ("enabled", enabled_), T::make_field ("locked", locked_));
  T::serialize_fields (
    ctx,
    T::make_field (
      "baseValue", base_value_,
      !ctx.is_serializing () || src_id_.type_ != PortType::CV));
}

void
PortConnectionsManager::define_fields (const Context &ctx)
{
  using T = ISerializable<PortConnectionsManager>;
  T::serialize_fields (ctx, T::make_field ("connections", connections_));

  if (ctx.is_deserializing ())
    regenerate_hashtables ();
}