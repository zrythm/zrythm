.. Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>

   This file is part of Zrythm

   Zrythm is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Zrythm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU General Affero Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

Project Info
============

In Zrythm, your work is saved inside a Project.
Projects consist of a folder with a YAML
file describing the Project and additional
files used such as MIDI and audio files.

Here is an example of a simple project file:

.. code-block:: yaml

  version: 0.5.120
  title: Untitled Project (4)
  tracklist:
    tracks:
    - name: Chords
      type: Chord
      lanes_visible: 0
      bot_paned_visible: 0
      visible: 1
      handle_pos: 1
      mute: 0
      solo: 0
      recording: 0
      pinned: 1
      color:
        red: 0.0117647
        green: 0.156863
        blue: 0.980392
        alpha: 1
      lanes:
      - pos: 0
        name: Lane 0
        handle_pos: 0
        mute: 0
        solo: 0
        regions: []
        track_pos: 0
      chords: []
      scales: []
      automation_tracklist:
        ats: []
    - name: Markers
      type: 4
      lanes_visible: 0
      bot_paned_visible: 0
      visible: 1
      handle_pos: 1
      mute: 0
      solo: 0
      recording: 0
      pinned: 1
      color:
        red: 0.639216
        green: 0.156863
        blue: 0.603922
        alpha: 1
      lanes:
      - pos: 0
        name: Lane 0
        handle_pos: 0
        mute: 0
        solo: 0
        regions: []
        track_pos: 1
      chords: []
      scales: []
      automation_tracklist:
        ats: []
    - name: Master
      type: Master
      lanes_visible: 0
      bot_paned_visible: 0
      visible: 1
      handle_pos: 1
      mute: 0
      solo: 0
      recording: 0
      pinned: 0
      color:
        red: 0.941176
        green: 0.0627451
        blue: 0.0627451
        alpha: 1
      lanes:
      - pos: 0
        name: Lane 0
        handle_pos: 0
        mute: 0
        solo: 0
        regions: []
        track_pos: 2
      chords: []
      scales: []
      channel:
        aggregated_plugins: []
        type: Master
        fader:
          volume: 0
          amp: 1
          phase: 0
          pan: 0.5
        stereo_in:
          l:
            identifier:
              label: Master stereo in L
              owner_type: Track
              type: Audio
              flow: Input
              flags:
                stereo_l: 0x1
              track_pos: 2
              plugin_slot: -1
              port_index: 0
            src_ids: []
            dest_ids:
            - label: Master Stereo out L
              owner_type: Track
              type: Audio
              flow: Output
              flags:
                stereo_l: 0x1
              track_pos: 2
              plugin_slot: -1
              port_index: 0
            internal_type: 0
          r:
            identifier:
              label: Master stereo in R
              owner_type: Track
              type: Audio
              flow: Input
              flags:
                stereo_r: 0x1
              track_pos: 2
              plugin_slot: -1
              port_index: 0
            src_ids: []
            dest_ids:
            - label: Master Stereo out R
              owner_type: Track
              type: Audio
              flow: Output
              flags:
                stereo_r: 0x1
              track_pos: 2
              plugin_slot: -1
              port_index: 0
            internal_type: 0
        midi_in:
          identifier:
            label: Master MIDI in
            owner_type: Track
            type: Event
            flow: Input
            flags: {}
            track_pos: 2
            plugin_slot: -1
            port_index: 0
          src_ids: []
          dest_ids: []
          internal_type: 0
        piano_roll:
          identifier:
            label: Master Piano Roll
            owner_type: Track
            type: Event
            flow: Input
            flags:
              piano_roll: 0x1
            track_pos: 2
            plugin_slot: -1
            port_index: 0
          src_ids: []
          dest_ids: []
          internal_type: 0
        stereo_out:
          l:
            identifier:
              label: Master Stereo out L
              owner_type: Track
              type: Audio
              flow: Output
              flags:
                stereo_l: 0x1
              track_pos: 2
              plugin_slot: -1
              port_index: 0
            src_ids:
            - label: Master stereo in L
              owner_type: Track
              type: Audio
              flow: Input
              flags:
                stereo_l: 0x1
              track_pos: 2
              plugin_slot: -1
              port_index: 0
            dest_ids:
            - label: JACK Stereo Out / L
              owner_type: Backend
              type: Audio
              flow: Output
              flags:
                stereo_l: 0x1
              track_pos: -1
              plugin_slot: -1
              port_index: 0
            internal_type: 0
          r:
            identifier:
              label: Master Stereo out R
              owner_type: Track
              type: Audio
              flow: Output
              flags:
                stereo_r: 0x1
              track_pos: 2
              plugin_slot: -1
              port_index: 0
            src_ids:
            - label: Master stereo in R
              owner_type: Track
              type: Audio
              flow: Input
              flags:
                stereo_r: 0x1
              track_pos: 2
              plugin_slot: -1
              port_index: 0
            dest_ids:
            - label: JACK Stereo Out / R
              owner_type: Backend
              type: Audio
              flow: Output
              flags:
                stereo_r: 0x1
              track_pos: -1
              plugin_slot: -1
              port_index: 0
            internal_type: 0
        output_pos: -1
      automation_tracklist:
        ats:
        - index: 0
          automatable:
            index: 0
            slot: -1
            label: Volume
            type: Channel Fader
          aps: []
          acs: []
          created: 1
          visible: 1
          handle_pos: 0
        - index: 0
          automatable:
            index: 0
            slot: -1
            label: Pan
            type: Channel Pan
          aps: []
          acs: []
          created: 0
          visible: 0
          handle_pos: 0
        - index: 0
          automatable:
            index: 0
            slot: -1
            label: Mute
            type: Channel Mute
          aps: []
          acs: []
          created: 0
          visible: 0
          handle_pos: 0
  clip_editor:
    piano_roll:
      notes_zoom: 3
      midi_modifier: Velocity
      drum_mode: 0
  snap_grid_timeline:
    grid_auto: 1
    note_length: 1/1
    note_type: normal
    snap_to_grid: 1
    snap_to_grid_keep_offset: 0
    snap_to_events: 0
  quantize_timeline:
    use_grid: 1
    note_length: 1/1
    note_type: normal
  audio_engine:
    sample_rate: 48000
    frames_per_tick: 21
    mixer:
      master_id: 0
    stereo_in:
      l:
        identifier:
          label: JACK Stereo In / L
          owner_type: Backend
          type: Audio
          flow: Input
          flags:
            stereo_l: 0x1
          track_pos: -1
          plugin_slot: -1
          port_index: 0
        src_ids: []
        dest_ids: []
        internal_type: JACK Port
      r:
        identifier:
          label: JACK Stereo In / R
          owner_type: Backend
          type: Audio
          flow: Input
          flags:
            stereo_r: 0x1
          track_pos: -1
          plugin_slot: -1
          port_index: 0
        src_ids: []
        dest_ids: []
        internal_type: JACK Port
    stereo_out:
      l:
        identifier:
          label: JACK Stereo Out / L
          owner_type: Backend
          type: Audio
          flow: Output
          flags:
            stereo_l: 0x1
          track_pos: -1
          plugin_slot: -1
          port_index: 0
        src_ids:
        - label: Master Stereo out L
          owner_type: Track
          type: Audio
          flow: Output
          flags:
            stereo_l: 0x1
          track_pos: 2
          plugin_slot: -1
          port_index: 0
        dest_ids: []
        internal_type: JACK Port
      r:
        identifier:
          label: JACK Stereo Out / R
          owner_type: Backend
          type: Audio
          flow: Output
          flags:
            stereo_r: 0x1
          track_pos: -1
          plugin_slot: -1
          port_index: 0
        src_ids:
        - label: Master Stereo out R
          owner_type: Track
          type: Audio
          flow: Output
          flags:
            stereo_r: 0x1
          track_pos: 2
          plugin_slot: -1
          port_index: 0
        dest_ids: []
        internal_type: JACK Port
    midi_in:
      identifier:
        label: JACK MIDI In
        owner_type: Backend
        type: Event
        flow: Input
        flags: {}
        track_pos: -1
        plugin_slot: -1
        port_index: 0
      src_ids: []
      dest_ids: []
      internal_type: JACK Port
    midi_editor_manual_press:
      identifier:
        label: MIDI Editor Manual Press
        owner_type: Backend
        type: Event
        flow: Input
        flags:
          manual_press: 0x1
        track_pos: -1
        plugin_slot: -1
        port_index: 0
      src_ids: []
      dest_ids: []
      internal_type: 0
    transport:
      total_bars: 128
      playhead_pos:
        bars: 1
        beats: 1
        sixteenths: 1
        ticks: 0
      cue_pos:
        bars: 1
        beats: 1
        sixteenths: 1
        ticks: 0
      loop_start_pos:
        bars: 1
        beats: 1
        sixteenths: 1
        ticks: 0
      loop_end_pos:
        bars: 8
        beats: 1
        sixteenths: 1
        ticks: 0
      start_marker_pos:
        bars: 1
        beats: 1
        sixteenths: 1
        ticks: 0
      end_marker_pos:
        bars: 128
        beats: 1
        sixteenths: 1
        ticks: 0
      beats_per_bar: 4
      beat_unit: 4
      position: 0
      bpm: 140
      loop: 1
      recording: 0
  snap_grid_midi:
    grid_auto: 1
    note_length: 1/8
    note_type: normal
    snap_to_grid: 1
    snap_to_grid_keep_offset: 0
    snap_to_events: 0
  quantize_midi:
    use_grid: 1
    note_length: 1/8
    note_type: normal
  mixer_selections:
    slots: []
    track_pos: 0
  timeline_selections:
    regions: []
    aps: []
    chords: []
  midi_arranger_selections:
    midi_notes: []
  tracklist_selections:
    tracks: []
  range_1:
    bars: 0
    beats: 0
    sixteenths: 0
    ticks: 0
  range_2:
    bars: 0
    beats: 0
    sixteenths: 0
    ticks: 0
  has_range: 0
