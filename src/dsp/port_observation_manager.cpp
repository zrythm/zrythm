// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <concepts>
#include <ranges>

#include "dsp/port.h"
#include "dsp/port_observation_cache.h"
#include "dsp/port_observation_manager.h"
#include "dsp/port_observer.h"

#include <QTimer>

namespace zrythm::dsp
{

namespace
{
// Append @p incoming to @p buf, then drop the oldest entries from the front so
// the buffer never exceeds @p cap. Uses range APIs throughout, so it is safe
// for empty buffers (no raw-pointer/nonnull pitfalls as with std::memmove).
template <typename T, std::ranges::input_range R>
  requires std::convertible_to<std::ranges::range_value_t<R>, T>
void
append_capped (std::vector<T> &buf, R &&incoming, size_t cap)
{
  buf.append_range (incoming);
  if (buf.size () > cap)
    {
      const auto excess = buf.size () - cap;
      const auto new_end = std::shift_left (buf.begin (), buf.end (), excess);
      buf.erase (new_end, buf.end ());
    }
}
} // namespace

struct PortObservationManager::Impl
{
  Impl (utils::IObjectRegistry &registry) : registry_ (registry) { }

  utils::IObjectRegistry &registry_;

  int next_id_ = 0;

  struct Registration
  {
    PortUuid                              port_uuid;
    std::unique_ptr<PortObservationCache> cache;
  };

  std::unordered_map<RegistrationId, Registration>            registrations_;
  std::unordered_map<PortUuid, int>                           ref_counts_;
  std::unordered_map<PortUuid, std::unique_ptr<PortObserver>> observers_;
  std::vector<PortObserver *>                                 observer_ptrs_;

  utils::QObjectUniquePtr<QTimer> drain_timer_;
};

PortObservationManager::PortObservationManager (
  utils::IObjectRegistry &registry,
  QObject *               parent)
    : QObject (parent), impl_ (std::make_unique<Impl> (registry))
{
  impl_->drain_timer_ = utils::make_qobject_unique<QTimer> (this);
  impl_->drain_timer_->setInterval (1000 / 60);
  QObject::connect (
    impl_->drain_timer_.get (), &QTimer::timeout, this,
    [this] () { drain_all (); });
  impl_->drain_timer_->start ();
}

PortObservationManager::~PortObservationManager () = default;

PortObservationManager::RegistrationId
PortObservationManager::register_request (const Port &port)
{
  const auto port_uuid = port.get_uuid ();
  const int  id = impl_->next_id_++;

  impl_->registrations_.emplace (
    id,
    Impl::Registration{ port_uuid, std::make_unique<PortObservationCache> () });

  auto      &ref_count = impl_->ref_counts_[port_uuid];
  const bool was_empty = ref_count == 0;
  ++ref_count;

  if (was_empty)
    {
      auto observer = std::make_unique<PortObserver> (impl_->registry_, port);
      impl_->observer_ptrs_.push_back (observer.get ());
      impl_->observers_.emplace (port_uuid, std::move (observer));
      Q_EMIT observationChanged ();
    }

  return id;
}

void
PortObservationManager::unregister_request (RegistrationId id)
{
  auto reg_it = impl_->registrations_.find (id);
  if (reg_it == impl_->registrations_.end ())
    return;

  auto port_uuid = reg_it->second.port_uuid;
  impl_->registrations_.erase (reg_it);

  auto it = impl_->ref_counts_.find (port_uuid);
  if (it == impl_->ref_counts_.end ())
    return;

  --it->second;
  if (it->second == 0)
    {
      impl_->ref_counts_.erase (it);

      std::unique_ptr<PortObserver> doomed;
      auto                          obs_it = impl_->observers_.find (port_uuid);
      if (obs_it != impl_->observers_.end ())
        {
          doomed = std::move (obs_it->second);
          impl_->observers_.erase (obs_it);
          std::erase (impl_->observer_ptrs_, doomed.get ());
        }

      Q_EMIT observationChanged ();
    }
}

PortObservationCache &
PortObservationManager::cache (RegistrationId id)
{
  auto it = impl_->registrations_.find (id);
  if (it == impl_->registrations_.end ())
    throw std::out_of_range ("PortObservationManager: invalid registration ID");
  return *it->second.cache;
}

PortObserver *
PortObservationManager::find_observer_by_uuid (const PortUuid &port_uuid) const
{
  auto it = impl_->observers_.find (port_uuid);
  return it != impl_->observers_.end () ? it->second.get () : nullptr;
}

PortObserver *
PortObservationManager::get_observer (const Port &port) const
{
  return find_observer_by_uuid (port.get_uuid ());
}

std::span<PortObserver * const>
PortObservationManager::observers () const
{
  return impl_->observer_ptrs_;
}

void
PortObservationManager::drain_all ()
{
  // Read once per port (consuming read), then fan out to all caches
  struct TempData
  {
    std::vector<std::vector<float>> audio;
    std::vector<RealtimeMidiEvent>  midi;
  };

  std::unordered_map<PortUuid, TempData> port_data;

  // Pass 1: consuming read from ring buffers into temp storage
  for (auto &[id, reg] : impl_->registrations_)
    {
      if (port_data.contains (reg.port_uuid))
        continue;

      auto * observer = find_observer_by_uuid (reg.port_uuid);
      if (observer == nullptr)
        continue;

      auto &data = port_data[reg.port_uuid];

      if (observer->has_audio_rings ())
        {
          const auto num_channels = observer->num_channels ();
          data.audio.resize (num_channels);
          for (int ch = 0; ch < num_channels; ++ch)
            {
              auto &ring = observer->audio_ring (ch);
              auto  avail = ring.read_space ();
              data.audio[ch].resize (avail);
              if (!ring.read_multiple (data.audio[ch].data (), avail))
                data.audio[ch].clear ();
            }
        }

      if (observer->has_midi_ring ())
        {
          auto &ring = observer->midi_ring ();
          auto  avail = ring.read_space ();
          data.midi.resize (avail);
          if (!ring.read_multiple (data.midi.data (), avail))
            data.midi.clear ();
        }
    }

  // Pass 2: append temp data into all registration caches, trimming the
  // oldest entries when a cap is exceeded.
  for (auto &[id, reg] : impl_->registrations_)
    {
      auto it = port_data.find (reg.port_uuid);
      if (it == port_data.end ())
        continue;

      auto &data = it->second;
      auto &cache = *reg.cache;

      if (cache.audio.size () < data.audio.size ())
        cache.audio.resize (data.audio.size ());
      for (size_t ch = 0; ch < data.audio.size (); ++ch)
        append_capped (
          cache.audio[ch], data.audio[ch],
          PortObservationCache::kMaxAudioSamples);

      append_capped (
        cache.midi, data.midi, PortObservationCache::kMaxMidiEvents);
    }
}

}
