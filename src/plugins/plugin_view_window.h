// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QtGui/qwindowdefs.h>

#include <juce_wrapper.h>

namespace zrythm::plugins
{
class PluginViewWindow : public juce::DocumentWindow
{
public:
  using CloseHandler = std::function<void ()>;

  PluginViewWindow (juce::String title, CloseHandler close_handler);

  /**
   * @brief Sets the JUCE component (normally a juce editor component) as the
   * child of this window.
   *
   * To be used by JUCE-hosted plugins.
   */
  void setJuceComponentContentNonOwned (juce::Component * content_non_owned);

  /**
   * @brief Gets a native window handle to embed in.
   *
   * To be used in natively hosted plugins.
   */
  WId getEmbedWindowId () const;

public:
  static bool wantsLogicalSize () noexcept
  {
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
  }

  void closeButtonPressed () override;

private:
  CloseHandler close_handler_;
};
} // namespace zrythm::plugins
