.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Examples
========

The following examples are in the public domain
(CC 0). Feel free to copy and modify them.

Create MIDI track with notes
----------------------------

.. code-block:: scheme

  (use-modules (audio track)
               (audio midi-note)
               (audio midi-region)
               (audio position)
               (audio tracklist))
  (define zrythm-script
    (lambda ()
      (let* ((track-slot 3)
            (track (midi-track-new track-slot "my midi track"))
            (r-start-pos (position-new 1 1 1 0 0))
            (r-end-pos (position-new 4 1 1 0 0))
            (region (midi-region-new r-start-pos r-end-pos track-slot 0 0))
            (mn-start-pos (position-new 2 1 1 0 0))
            (mn-end-pos (position-new 3 1 1 0 0))
            (note (midi-note-new region mn-start-pos mn-end-pos 80 90)))
        (for-each
          (lambda (pitch)
            (let ((note (midi-note-new region mn-start-pos mn-end-pos pitch 90)))
              (midi-region-add-midi-note region note)))
          '(56 60 63))
        (tracklist-insert-track track track-slot)
        (track-add-lane-region track region 0))))
