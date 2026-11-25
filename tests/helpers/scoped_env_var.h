// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include <QByteArray>
#include <QtGlobal>

namespace zrythm::test_helpers
{

/** Puts an environment variable back to its previous value when destroyed. */
class ScopedEnvVar
{
public:
  explicit ScopedEnvVar (const char * name) : name_ (name)
  {
    if (qEnvironmentVariableIsSet (name))
      {
        previous_value_ = qgetenv (name);
      }
  }

  ScopedEnvVar (const ScopedEnvVar &) = delete;
  ScopedEnvVar &operator= (const ScopedEnvVar &) = delete;

  ~ScopedEnvVar ()
  {
    if (previous_value_)
      {
        qputenv (name_, *previous_value_);
      }
    else
      {
        qunsetenv (name_);
      }
  }

private:
  const char *              name_;
  std::optional<QByteArray> previous_value_{};
};

/** Restores the environment variables that name the caches of Qt Quick. */
struct QtCacheVariables
{
  ScopedEnvVar qml_disk_cache_path{ "QML_DISK_CACHE_PATH" };
  ScopedEnvVar disk_cache_disabled{ "QSG_RHI_DISABLE_DISK_CACHE" };
  ScopedEnvVar pipeline_cache_save{ "QSG_RHI_PIPELINE_CACHE_SAVE" };
  ScopedEnvVar pipeline_cache_load{ "QSG_RHI_PIPELINE_CACHE_LOAD" };
};

} // namespace zrythm::test_helpers
