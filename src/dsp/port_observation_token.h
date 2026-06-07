// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port_observation_cache.h"
#include "dsp/port_observation_manager.h"

#include <QPointer>

namespace zrythm::dsp
{

class ObservationToken
{
public:
  ObservationToken (PortObservationManager &manager, const Port &port)
      : manager_ (&manager), port_uuid_ (port.get_uuid ()),
        id_ (manager.register_request (port))
  {
  }

  ~ObservationToken ()
  {
    if (manager_ != nullptr && id_ >= 0)
      manager_->unregister_request (id_);
  }

  ObservationToken (const ObservationToken &) = delete;
  ObservationToken &operator= (const ObservationToken &) = delete;

  ObservationToken (ObservationToken &&other) noexcept
      : manager_ (other.manager_), port_uuid_ (other.port_uuid_), id_ (other.id_)
  {
    other.manager_ = nullptr;
    other.id_ = -1;
  }

  ObservationToken &operator= (ObservationToken &&other) noexcept
  {
    if (this != &other)
      {
        if (manager_ != nullptr && id_ >= 0)
          manager_->unregister_request (id_);
        manager_ = other.manager_;
        port_uuid_ = other.port_uuid_;
        id_ = other.id_;
        other.manager_ = nullptr;
        other.id_ = -1;
      }
    return *this;
  }

  PortUuid port_uuid () const { return port_uuid_; }

  PortObservationCache &cache ()
  {
    if (manager_ == nullptr)
      throw std::runtime_error ("ObservationToken: manager destroyed");
    return manager_->cache (id_);
  }

private:
  QPointer<PortObservationManager>       manager_;
  PortUuid                               port_uuid_;
  PortObservationManager::RegistrationId id_;
};

}
