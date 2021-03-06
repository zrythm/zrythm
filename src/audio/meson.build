# Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
# Copyright (C) 2020 Ryan Gonzalez <rymg19 at gmail dot com>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

audio_srcs = [
  'audio_function.c',
  'audio_region.c',
  'audio_track.c',
  'automation_function.c',
  'automation_point.c',
  'automation_region.c',
  'automation_track.c',
  'automation_tracklist.c',
  'audio_bus_track.c',
  'audio_group_track.c',
  'balance_control.c',
  'channel.c',
  'channel_send.c',
  'channel_track.c',
  'chord_descriptor.c',
  'chord_object.c',
  'chord_region.c',
  'chord_track.c',
  'clip.c',
  'control_port.c',
  'control_room.c',
  'curve.c',
  'encoder.c',
  'engine.c',
  'engine_alsa.c',
  'engine_dummy.c',
  'engine_jack.c',
  'engine_pa.c',
  'engine_pulse.c',
  'engine_rtaudio.c',
  'engine_rtmidi.c',
  'engine_sdl.c',
  'engine_windows_mme.c',
  'exporter.c',
  'ext_port.c',
  'fade.c',
  'fader.c',
  'foldable_track.c',
  'graph.c',
  'graph_node.c',
  'graph_thread.c',
  'graph_export.c',
  'group_target_track.c',
  'hardware_processor.c',
  'instrument_track.c',
  'marker.c',
  'marker_track.c',
  'master_track.c',
  'meter.c',
  'metronome.c',
  'midi.c',
  'midi_bus_track.c',
  'midi_event.c',
  'midi_file.c',
  'midi_function.c',
  'midi_group_track.c',
  'midi_mapping.c',
  'midi_note.c',
  'midi_region.c',
  'midi_track.c',
  'modulator_macro_processor.c',
  'modulator_track.c',
  'pool.c',
  'port.c',
  'port_identifier.c',
  'position.c',
  'quantize_options.c',
  'pan.c',
  'recording_event.c',
  'recording_manager.c',
  'region.c',
  'region_identifier.c',
  'region_link_group.c',
  'region_link_group_manager.c',
  'router.c',
  'rtaudio_device.c',
  'rtmidi_device.c',
  'sample_playback.c',
  'sample_processor.c',
  'scale.c',
  'scale_object.c',
  'snap_grid.c',
  'stretcher.c',
  'supported_file.c',
  'tempo_track.c',
  'track.c',
  'track_lane.c',
  'track_processor.c',
  'tracklist.c',
  'transport.c',
  'true_peak_dsp.c',
  'velocity.c',
  'windows_mmcss.c',
  'windows_mme_device.c',
  ]

# optimized audio
optimized_audio_lib = static_library (
  'zrythm-optimized-audio-lib',
  sources: [
    'kmeter_dsp.c',
    'peak_dsp.c',
    ],
  dependencies: zrythm_deps,
  include_directories: all_inc,
  c_args: [
    common_cflags, strict_cflags, '-O3',
    extra_optimizations_cflags,
    extra_extra_optimizations_cflags,
    ],
  )

zrythm_srcs += files (audio_srcs)
