// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QtGui/qwindowdefs.h>

#include <juce_wrapper.h>

namespace zrythm::plugins
{
/**
 * @brief Interface for top-level plugin hosting windows.
 */
class IPluginHostWindow
{
public:
  virtual ~IPluginHostWindow () = default;

  /**
   * @brief Sets the JUCE component (normally a juce editor component) as the
   * child of this window.
   *
   * To be used by JUCE-hosted plugins.
   */
  virtual void
  setJuceComponentContentNonOwned (juce::Component * content_non_owned) = 0;

  /**
   * @brief Sets the window size and centers it on the screen.
   *
   * @note This is not needed when using @ref setJuceComponentContentNonOwned.
   */
  virtual void setSizeAndCenter (int width, int height) = 0;

  /**
   * @brief Set the window size without centering.
   *
   * Used for resizes.
   */
  virtual void setSize (int width, int height) = 0;

  virtual void setVisible (bool shouldBeVisible) = 0;

  /**
   * @brief Gets a native window handle to embed in.
   *
   * To be used in natively hosted plugins.
   */
  virtual WId getEmbedWindowId () const = 0;
};
} // namespace zrythm::plugins
