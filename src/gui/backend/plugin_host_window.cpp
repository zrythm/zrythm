// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "gui/backend/plugin_host_window.h"

namespace zrythm::plugins
{
JuceDocumentPluginHostWindow::JuceDocumentPluginHostWindow (
  const utils::Utf8String &title,
  CloseHandler             close_handler)
    : juce::DocumentWindow (
        title.to_juce_string (),
        juce::Colours::cadetblue,
        juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton),
      close_handler_ (std::move (close_handler))
{
  setAlwaysOnTop (true); // Optional: keep on top
  setUsingNativeTitleBar (true);
  setResizable (true, true);
  addToDesktop ();
  setVisible (true);
  toFront (true);
}

void
JuceDocumentPluginHostWindow::setJuceComponentContentNonOwned (
  juce::Component * content_non_owned)
{
  setContentNonOwned (content_non_owned, true);
  setSizeAndCenter (
    content_non_owned->getWidth (), content_non_owned->getHeight ());
}

void
JuceDocumentPluginHostWindow::setSizeAndCenter (int width, int height)
{
  centreWithSize (width, height);
}

void
JuceDocumentPluginHostWindow::closeButtonPressed ()
{
  close_handler_ ();
}

WId
JuceDocumentPluginHostWindow::getEmbedWindowId () const
{
  // Get the peer (native window wrapper)
  juce::ComponentPeer * peer = getPeer ();
  if (peer == nullptr)
    return 0;

  return reinterpret_cast<WId> (peer->getNativeHandle ());

#if 0
  {
    // Platform-specific native handle access

#  ifdef Q_OS_WIN
    // On Windows, get the HWND
    void * nativeHandle = peer->getNativeHandle ();
    HWND   hwnd = static_cast<HWND> (nativeHandle);
    return hwnd;

#  elifdef Q_OS_WIN
    // On macOS, get the NSView*
    void *   nativeHandle = peer->getNativeHandle ();
    NSView * nsView = static_cast<NSView *> (nativeHandle);
    return nsView;

#  elifdef Q_OS_LINUX
    // On Linux/X11, get the X11 Window
    void * nativeHandle = peer->getNativeHandle ();
    auto   x11Window =
      static_cast<::Window> (reinterpret_cast<uintptr_t> (nativeHandle));
    return x11Window;
#  endif
  }
#endif
}
}
