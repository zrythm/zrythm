========================================
(audio track)
========================================

``(midi-track-new idx name)``
   Returns a new track with name ``name`` to be placed at position
   ``idx`` in the tracklist.


``(track-get-name track)``
   Returns the name of ``track``.


``(track-get-processor track)``
   Returns the processor of ``track``.


``(track-get-channel track)``
   Returns the channel of ``track``.


``(track-set-soloed track solo)``
   Sets whether ``track`` is soloed or not. This creates an undoable
   action and performs it.


``(track-set-muted track muted)``
   Sets whether ``track`` is muted or not. This creates an undoable
   action and performs it.


``(track-add-lane-region track region lane_pos)``
   Adds ``region`` to track ``track``. To be used for regions with lanes
   (midi/audio)


