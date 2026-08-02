// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/parameter.h"
#include "plugins/plugin.h"
#include "plugins/plugin_host_window.h"
#include "utils/logger.h"

namespace zrythm::plugins
{

PluginHostWindow::PluginHostWindow (Plugin &plugin, QObject * parent)
    : QObject (parent), plugin_ (plugin)
{
  connect (this, &PluginHostWindow::closeRequested, this, [this] () {
    z_debug (
      "close button pressed on '{}' plugin window", plugin_.get_node_name ());
    plugin_.setUiVisible (false);
  });

  auto * bypass = plugin_.bypassParameter ();
  connect (this, &PluginHostWindow::bypassToggleRequested, this, [bypass] () {
    bypass->setBaseValue (
      bypass->range ().isToggled (bypass->baseValue ()) ? 0.f : 1.f);
  });
  connect (
    bypass, &dsp::ProcessorParameter::baseValueChanged, this,
    [this, bypass] (float value) {
      Q_EMIT bypassedChanged (bypass->range ().isToggled (value));
    });

  connect (this, &PluginHostWindow::presetSelectorRequested, this, [this] {
    z_debug (
      "preset selector requested on '{}' plugin window",
      plugin_.get_node_name ());
  });
  connect (this, &PluginHostWindow::abSwitchRequested, this, [this] {
    const auto current = plugin_.save_state ();
    if (current.empty ())
      {
        z_warning (
          "A/B: plugin '{}' has no state to save", plugin_.get_node_name ());
        return;
      }

    // Save the current state into the active slot, then switch to the other
    // slot (initialized as a copy of the current state on first use)
    auto &active_slot = ab_b_active_ ? ab_state_b_ : ab_state_a_;
    auto &other_slot = ab_b_active_ ? ab_state_a_ : ab_state_b_;
    active_slot = current;
    if (other_slot.empty ())
      other_slot = current;
    else
      plugin_.load_state (other_slot);
    ab_b_active_ = !ab_b_active_;
    Q_EMIT abStateChanged (ab_b_active_);
  });
}

bool
PluginHostWindow::bypassed () const
{
  const auto * bypass = plugin_.bypassParameter ();
  return bypass->range ().isToggled (bypass->baseValue ());
}

QString
PluginHostWindow::pluginName () const
{
  return plugin_.get_node_name ().to_qstring ();
}

} // namespace zrythm::plugins
