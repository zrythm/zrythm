// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/logger.h"
#include "common/utils/lsp_dsp_context.h"

#if HAVE_LSP_DSP
LspDspContextRAII::LspDspContextRAII ()
{
  static std::once_flag init_flag;
  std::call_once (init_flag, [] () {
    z_info ("Initing LSP DSP...");
    lsp::dsp::init ();
  });

  lsp::dsp::info_t * info = lsp::dsp::info ();
  if (info != nullptr)
    {
      z_info (
        "Architecture:   {}\n"
        "Processor:      {}\n"
        "Model:          {}\n"
        "Features:       {}\n",
        info->arch, info->cpu, info->model, info->features);
      free (info);
    }
  else
    {
      z_warning ("Failed to get system info");
    }

  ctx_ = std::make_unique<lsp::dsp::context_t> ();
  z_info ("Starting LSP DSP...");
  lsp::dsp::start (ctx_.get ());
}

LspDspContextRAII::~LspDspContextRAII ()
{
  z_info ("Finishing LSP DSP...");
  if (ctx_)
    lsp::dsp::finish (ctx_.get ());
}
#endif