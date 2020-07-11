.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Examples
========

The following examples are in the public domain
(`CC 0 <https://creativecommons.org/publicdomain/zero/1.0/>`_).
Feel free to copy and modify them.

Hello World
-----------

.. code-block:: scheme

  (define zrythm-script
    (lambda ()
      (display "Hello, World!")))

Print track names
-----------------

.. code-block:: scheme

  (use-modules (audio track)
               (audio tracklist))
  (define zrythm-script
    (lambda ()
      (let ((num-tracks (tracklist-get-num-tracks)))
        (let loop ((i 0))
          (when (< i num-tracks)
            (let ((track (tracklist-get-track-at-pos i)))
              (display (track-get-name track))
              (newline))
            (loop (+ i 1)))))))

Create Geonkick track
---------------------

.. code-block:: scheme

  (use-modules
    (actions create-tracks-action)
    (actions undo-manager)
    (plugins plugin-manager))
  (define zrythm-script
    (lambda ()
      (let*
        ((action
           (create-tracks-action-new-with-plugin
             0
             (plugin-manager-find-plugin-from-uri
               "http://geontime.com/geonkick")
             4 1)))
        (undo-manager-perform action))))

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
