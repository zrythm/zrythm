// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/iplugin_host_window.h"

namespace zrythm::plugins
{

/**
 * @brief Implementation of IPluginHostWindow based on juce::DocumentWindow.
 *
 * @note This is more reliable than using Qt since JUCE always uses X11 windows
 * on GNU/Linux, whereas Qt will use Wayland windows if we are on Wayland, and
 * forcing QT_QPA_PLATFORM=xcb for the whole application just for plugin UIs is
 * excessive.
 */
class JuceDocumentPluginHostWindow final
    : private juce::DocumentWindow,
      public IPluginHostWindow
{
public:
  using CloseHandler = std::function<void ()>;

  JuceDocumentPluginHostWindow (
    const utils::Utf8String &title,
    CloseHandler             close_handler);

  /**
   * @brief Sets the JUCE component (normally a juce editor component) as the
   * child of this window.
   *
   * To be used by JUCE-hosted plugins.
   */
  void
  setJuceComponentContentNonOwned (juce::Component * content_non_owned) override;

  /**
   * @brief Sets the window size and centers it on the screen.
   *
   * @note This is not needed when using @ref setJuceComponentContentNonOwned.
   */
  void setSizeAndCenter (int width, int height) override;

  /**
   * @brief Set the window size without centering.
   *
   * Used for resizes.
   */
  void setSize (int width, int height) override
  {
    juce::DocumentWindow::setSize (width, height);
  }

  void setVisible (bool shouldBeVisible) override
  {
    juce::DocumentWindow::setVisible (shouldBeVisible);
  }

  /**
   * @brief Gets a native window handle to embed in.
   *
   * To be used in natively hosted plugins.
   */
  WId getEmbedWindowId () const override;

private:
  void closeButtonPressed () override;

private:
  CloseHandler close_handler_;
};
} // namespace zrythm::plugins
