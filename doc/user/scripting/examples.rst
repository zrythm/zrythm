.. SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
.. SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Examples
========

The following examples are in the public domain
(`CC 0 <https://creativecommons.org/publicdomain/zero/1.0/>`_).
Feel free to copy and modify them.

.. todo:: API changed - check that these work.

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

Create Geonkick track and route its mono output to an Audio FX track
--------------------------------------------------------------------

.. code-block:: scheme

  (use-modules
    (actions create-tracks-action)
    (actions port-connection-action)
    (actions undo-manager)
    (audio channel)
    (audio port)
    (audio track-processor)
    (audio tracklist)
    (audio track)
    (plugins plugin)
    (plugins plugin-manager))
  (define zrythm-script
    (lambda ()
      (let*
        ((action
           (create-tracks-action-new-with-plugin
             ;; track type (0 = instrument)
             0
             ;; plugin to add
             (plugin-manager-find-plugin-from-uri
               "http://geontime.com/geonkick")
             ;; track pos to insert at
             4
             ;; number of tracks to create
             1)))
        (undo-manager-perform action))
      (let*
        ((action
           (create-tracks-action-new-audio-fx
             ;; track pos to insert at
             5
             ;; number of tracks to create
             1)))
        (undo-manager-perform action))
      (let*
        ((geonkick-track (tracklist-get-track-at-pos 4))
         (geonkick-channel (track-get-channel geonkick-track))
         (geonkick (channel-get-instrument geonkick-channel))
         (geonkick-mono-out (plugin-get-out-port geonkick 1))
         (fx-track (tracklist-get-track-at-pos 5))
         (fx-processor (track-get-processor fx-track))
         (fx-stereo-in (track-processor-get-stereo-in fx-processor))
         (fx-stereo-in-l (stereo-ports-get-port fx-stereo-in #t))
         (fx-stereo-in-r (stereo-ports-get-port fx-stereo-in #f))
         (action-1
           (port-connection-action-new-connect
             geonkick-mono-out fx-stereo-in-l))
         (action-2
           (port-connection-action-new-connect
             geonkick-mono-out fx-stereo-in-r)))
        (undo-manager-perform action-1)
        (undo-manager-perform action-2)
        (track-set-muted geonkick-track #t))))

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
