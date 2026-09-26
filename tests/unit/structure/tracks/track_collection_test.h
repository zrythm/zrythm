// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tempo_map.h"
#include "plugins/plugin.h"

#include <QObject>

namespace zrythm::structure::tracks
{

// Minimal concrete plugin with a single gain parameter, used to stage
// plugin-reported values and observe them being applied
class FlushPlugin : public plugins::Plugin
{
  Q_OBJECT

public:
  FlushPlugin (utils::IObjectRegistry &registry, QObject * parent = nullptr)
      : Plugin (registry, parent)
  {
    add_parameter (generate_default_gain_param ());
  }

  void process_impl (
    dsp::graph::ProcessBlockInfo,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept override
  {
  }
  std::string save_state_impl () const override { return {}; }
  bool load_state_impl (const std::string &state) override { return true; }

  /** Sizes the pending-value slots the way preparing for processing
   * would, so values can be staged without a running engine. */
  void prepare_param_sync ()
  {
    param_sync_.prepare (get_parameters ().size ());
  }

  using Plugin::set_param_pending_from_plugin;

  /** The gain parameter (this plugin's only parameter). */
  dsp::ProcessorParameter * gain_parameter () const
  {
    return get_parameters ()[0].get ();
  }
};

}
