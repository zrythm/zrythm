// SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MODULATOR_TRACK_H__
#define __AUDIO_MODULATOR_TRACK_H__

#include "common/dsp/automatable_track.h"
#include "common/dsp/modulator_macro_processor.h"
#include "common/dsp/processable_track.h"
#include "common/plugins/carla_native_plugin.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define P_MODULATOR_TRACK (TRACKLIST->modulator_track_)

/**
 * @brief A track that can host modulator plugins.
 */
class ModulatorTrack final
    : public ProcessableTrack,
      public ICloneable<ModulatorTrack>,
      public ISerializable<ModulatorTrack>,
      public InitializableObjectFactory<ModulatorTrack>
{
  friend class InitializableObjectFactory<ModulatorTrack>;

public:
  using ModulatorPtr = std::shared_ptr<zrythm::plugins::Plugin>;

  /**
   * Inserts and connects a Modulator to the Track.
   *
   * @param replace_mode Whether to perform the operation in replace mode
   * (replace current modulator if true, not touching other modulators, or push
   * other modulators forward if false).
   */
  template <typename T = zrythm::plugins::Plugin>
  std::shared_ptr<T> insert_modulator (
    int                slot,
    std::shared_ptr<T> modulator,
    bool               replace_mode,
    bool               confirm,
    bool               gen_automatables,
    bool               recalc_graph,
    bool               pub_events);

  /**
   * Removes a plugin at pos from the track.
   *
   * @param deleting_modulator
   * @param deleting_track If true, the automation tracks associated with the
   * plugin are not deleted at this time.
   * @param recalc_graph Recalculate mixer graph.
   */
  ModulatorPtr remove_modulator (
    int  slot,
    bool deleting_modulator,
    bool deleting_track,
    bool recalc_graph);

  void init_loaded () override;

  void init_after_cloning (const ModulatorTrack &other) override;

  bool validate () const override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  ModulatorTrack (int track_pos = 0);

  bool initialize () override;

public:
  /** Modulators. */
  std::vector<ModulatorPtr> modulators_;

  /** Modulator macros. */
  std::vector<std::unique_ptr<ModulatorMacroProcessor>>
    modulator_macro_processors_;
};

extern template std::shared_ptr<zrythm::plugins::Plugin>
ModulatorTrack::insert_modulator (
  int                                      slot,
  std::shared_ptr<zrythm::plugins::Plugin> modulator,
  bool                                     replace_mode,
  bool                                     confirm,
  bool                                     gen_automatables,
  bool                                     recalc_graph,
  bool                                     pub_events);
extern template std::shared_ptr<zrythm::plugins::CarlaNativePlugin>
ModulatorTrack::insert_modulator (
  int                                                 slot,
  std::shared_ptr<zrythm::plugins::CarlaNativePlugin> modulator,
  bool                                                replace_mode,
  bool                                                confirm,
  bool                                                gen_automatables,
  bool                                                recalc_graph,
  bool                                                pub_events);

/**
 * @}
 */

#endif
