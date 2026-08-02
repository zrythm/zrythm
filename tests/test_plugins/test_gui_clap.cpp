// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <cstring>
#include <string_view>

#include <clap/all.h>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>

namespace zrythm_test_plugins
{

using ClapPluginBase = clap::helpers::Plugin<
  clap::helpers::MisbehaviourHandler::Terminate,
  clap::helpers::CheckingLevel::Maximal>;

/**
 * Gain plugin with a stub offscreen editor view (fixed size, supports all
 * platform window APIs, records parenting/visibility without creating any
 * real window).
 */
class TestGuiClap final : public ClapPluginBase
{
public:
  static constexpr uint32_t kWidth = 320;
  static constexpr uint32_t kHeight = 240;

  /** Width requested from on_main_thread() as an observable side effect. */
  static constexpr uint32_t kMainThreadCallbackWidth = 640;

  explicit TestGuiClap (const clap_host * host)
      : ClapPluginBase (descriptor (), host)
  {
  }

  static const clap_plugin_descriptor * descriptor ()
  {
    static constexpr const char * const features[] = {
      CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr
    };
    static const clap_plugin_descriptor desc = {
      .clap_version = CLAP_VERSION,
      .id = kPluginId.data (),
      .name = kPluginName.data (),
      .vendor = "Zrythm",
      .url = "https://zrythm.org",
      .manual_url = "https://manual.zrythm.org",
      .support_url = "https://gitlab.zrythm.org/zrythm/zrythm/-/issues",
      .version = "1.0.0",
      .description = "Minimal gain with a stub editor used as a test fixture",
      .features = features,
    };
    return &desc;
  }

  // string_view of a literal - .data() is null-terminated, as the CLAP ABI
  // expects
  static constexpr std::string_view kPluginId = "org.zrythm.TestGuiClap";
  static constexpr std::string_view kPluginName = "Test GUI CLAP";

  static const clap_plugin * createInstance (const clap_host * host) noexcept
  {
    auto * p = new TestGuiClap (host);
    return p->clapPlugin ();
  }

  // audio ports
  bool     implementsAudioPorts () const noexcept override { return true; }
  uint32_t audioPortsCount (bool isInput) const noexcept override { return 1; }
  bool
  audioPortsInfo (uint32_t index, bool isInput, clap_audio_port_info * info)
    const noexcept override
  {
    if (index != 0)
      return false;
    info->id = 0;
    std::snprintf (
      info->name, sizeof (info->name), "%s", isInput ? "Input" : "Output");
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = isInput ? 0 : CLAP_INVALID_ID;
    return true;
  }

  clap_process_status process (const clap_process * process) noexcept override
  {
    const auto frames = process->frames_count;
    for (uint32_t ch = 0; ch < 2; ++ch)
      {
        std::copy_n (
          process->audio_inputs[0].data32[ch], frames,
          process->audio_outputs[0].data32[ch]);
      }
    return CLAP_PROCESS_SLEEP;
  }

  // GUI
  bool implementsGui () const noexcept override { return true; }
  bool guiIsApiSupported (const char * api, bool isFloating) noexcept override
  {
    if (isFloating)
      return false;
    return std::strcmp (api, CLAP_WINDOW_API_X11) == 0
           || std::strcmp (api, CLAP_WINDOW_API_WIN32) == 0
           || std::strcmp (api, CLAP_WINDOW_API_COCOA) == 0;
  }
  bool guiCreate (const char * api, bool isFloating) noexcept override
  {
    return guiIsApiSupported (api, isFloating);
  }
  void guiDestroy () noexcept override { parent_set_ = false; }
  bool guiGetSize (uint32_t * width, uint32_t * height) noexcept override
  {
    *width = kWidth;
    *height = kHeight;
    return true;
  }
  bool guiSetParent (const clap_window * window) noexcept override
  {
    if (window == nullptr || window->ptr == nullptr)
      return false;
    parent_set_ = true;
    return true;
  }
  bool guiShow () noexcept override
  {
    visible_ = true;
    return parent_set_;
  }
  bool guiHide () noexcept override
  {
    visible_ = false;
    return true;
  }

  void onMainThread () noexcept override
  {
    // Observable side effect so hosts can verify main-thread delivery
    _host.guiRequestResize (kMainThreadCallbackWidth, kHeight);
  }

private:
  bool parent_set_ = false;
  bool visible_ = false;
};

static const clap_plugin_factory plugin_factory = {
  .get_plugin_count = [] (const clap_plugin_factory *) -> uint32_t { return 1; },
  .get_plugin_descriptor = [] (const clap_plugin_factory *, uint32_t index)
    -> const clap_plugin_descriptor * {
    return index == 0 ? TestGuiClap::descriptor () : nullptr;
  },
  .create_plugin =
    [] (const clap_plugin_factory *, const clap_host * host, const char * plugin_id)
    -> const clap_plugin * {
    if (host == nullptr || !clap_version_is_compatible (host->clap_version))
      return nullptr;
    if (TestGuiClap::kPluginId == plugin_id)
      return TestGuiClap::createInstance (host);
    return nullptr;
  },
};

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry = {
  .clap_version = CLAP_VERSION,
  .init = [] (const char *) -> bool { return true; },
  .deinit = [] () { },
  .get_factory = [] (const char * factory_id) -> const void * {
    return std::strcmp (factory_id, CLAP_PLUGIN_FACTORY_ID) == 0
             ? static_cast<const void *> (&zrythm_test_plugins::plugin_factory)
             : nullptr;
  },
};
}
