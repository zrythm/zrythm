// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "common/plugins/plugin_descriptor.h"

#if 0
namespace zrythm::gui::wrappers
{

class PluginDescriptor : public QObject
{
  Q_OBJECT

  Q_PROPERTY (QString name READ getName CONSTANT FINAL)

public:
  PluginDescriptor (plugins::PluginDescriptor descr, QObject * parent = nullptr)
      : QObject (parent), wrapped_ (std::move (descr))
  {
  }

  [[nodiscard]] QString getName () const
  {
    return QString::fromStdString (wrapped_.name_);
  }

private:
  plugins::PluginDescriptor wrapped_;
};
}
#endif