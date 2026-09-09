// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/logger.h"

#include "lv2_urid_map.h"
#include <lv2/atom/atom.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/midi/midi.h>
#include <lv2/parameters/parameters.h>
#include <lv2/time/time.h>

namespace zrythm::plugins
{

struct Lv2UridMap::Impl
{
  std::mutex mutex;

  /** Forward map; node-based, so key addresses are stable. */
  std::unordered_map<std::string, uint32_t> by_uri;

  /** Reverse lookup; URID is the index plus one. */
  std::vector<const std::string *> by_urid;
};

Lv2UridMap::Lv2UridMap () : impl_ (std::make_unique<Impl> ()) { }
Lv2UridMap::~Lv2UridMap () = default;

uint32_t
Lv2UridMap::map (const char * uri)
{
  if (uri == nullptr || uri[0] == '\0')
    {
      z_warning ("cannot map an empty URI");
      return 0;
    }

  const std::lock_guard lock (impl_->mutex);
  auto [it, inserted] = impl_->by_uri.try_emplace (uri, 0);
  if (inserted)
    {
      it->second = static_cast<uint32_t> (impl_->by_urid.size ()) + 1;
      impl_->by_urid.push_back (&it->first);
    }
  return it->second;
}

const char *
Lv2UridMap::unmap (uint32_t urid) const
{
  if (urid == 0)
    {
      return nullptr;
    }

  const std::lock_guard lock (impl_->mutex);
  if (urid > impl_->by_urid.size ())
    {
      return nullptr;
    }
  return impl_->by_urid[urid - 1]->c_str ();
}

Lv2HostUrids
Lv2UridMap::host_urids ()
{
  Lv2HostUrids u;
  u.atom_Sequence = map (LV2_ATOM__Sequence);
  u.atom_Chunk = map (LV2_ATOM__Chunk);
  u.atom_Bool = map (LV2_ATOM__Bool);
  u.atom_Float = map (LV2_ATOM__Float);
  u.atom_Int = map (LV2_ATOM__Int);
  u.atom_Long = map (LV2_ATOM__Long);
  u.atom_Double = map (LV2_ATOM__Double);
  u.midi_MidiEvent = map (LV2_MIDI__MidiEvent);
  u.time_Position = map (LV2_TIME__Position);
  u.time_speed = map (LV2_TIME__speed);
  u.time_frame = map (LV2_TIME__frame);
  u.time_framesPerSecond = map (LV2_TIME__framesPerSecond);
  u.time_bar = map (LV2_TIME__bar);
  u.time_barBeat = map (LV2_TIME__barBeat);
  u.time_beatUnit = map (LV2_TIME__beatUnit);
  u.time_beatsPerBar = map (LV2_TIME__beatsPerBar);
  u.time_beatsPerMinute = map (LV2_TIME__beatsPerMinute);
  u.param_sampleRate = map (LV2_PARAMETERS__sampleRate);
  u.bufsz_minBlockLength = map (LV2_BUF_SIZE__minBlockLength);
  u.bufsz_maxBlockLength = map (LV2_BUF_SIZE__maxBlockLength);
  u.bufsz_nominalBlockLength = map (LV2_BUF_SIZE__nominalBlockLength);
  u.bufsz_sequenceSize = map (LV2_BUF_SIZE__sequenceSize);
  return u;
}

} // namespace zrythm::plugins
