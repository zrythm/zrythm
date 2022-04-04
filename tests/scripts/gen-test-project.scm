;;; SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
;;; SPDX-License-Identifier: CC0-1.0
;;;
;;; An example script for generating projects or
;;; project templates.

(use-modules
  (actions create-tracks-action)
  (actions port-connection-action)
  (actions undo-manager)
  (audio channel)
  (audio port)
  (audio track)
  (audio track-processor)
  (audio tracklist)
  (plugins plugin)
  (plugins plugin-manager)
  (project)
  (zrythm))

;; Creates a MIDI track
(define (create-midi-track prj)
  (let* ((tracklist (project-get-tracklist prj))
         (track-pos
           (tracklist-get-num-tracks tracklist))
         (action
           (create-tracks-action-new-midi
             track-pos 1))
         (undo-manager
           (project-get-undo-manager prj)))
    (undo-manager-perform undo-manager action)
    (tracklist-get-track-at-pos
      tracklist track-pos)))

;; Creates an audio FX track
(define (create-audio-fx-track prj)
  (let* ((tracklist (project-get-tracklist prj))
         (track-pos
           (tracklist-get-num-tracks tracklist))
         (action
           (create-tracks-action-new-audio-fx
             track-pos 1))
         (undo-manager
           (project-get-undo-manager prj)))
    (undo-manager-perform undo-manager action)
    (tracklist-get-track-at-pos
      tracklist track-pos)))

;; Creates a track from an audio effect plugin
(define (create-plugin-track prj track-type uri)
  (let* ((tracklist (project-get-tracklist prj))
         (a (zrythm-message "a"))
         (pm (zrythm-get-plugin-manager))
         (b (zrythm-message "b"))
         (track-pos
           (tracklist-get-num-tracks tracklist))
         (c (zrythm-message "c"))
         (descr
           (plugin-manager-find-plugin-from-uri
             pm uri))
         (d (zrythm-message "d"))
         (action
           (create-tracks-action-new-with-plugin
             track-type descr track-pos 1))
         (e (zrythm-message "e"))
         (undo-manager
           (project-get-undo-manager prj)))
    (undo-manager-perform undo-manager action)
    (tracklist-get-track-at-pos tracklist track-pos)))

(define zrythm-script
  (lambda ()
    (display (zrythm-get-ver))
    (newline)
    (let* ((prj (zrythm-get-project))
           (tracklist (project-get-tracklist prj))
           (pm (zrythm-get-plugin-manager))
           (undo_mgr (project-get-undo-manager prj)))
      (display (project-get-title prj))
      (newline)
      (plugin-manager-scan-plugins pm)
      (create-midi-track prj)
      (let*
        ((ins-track
           (create-plugin-track
             prj "Instrument"
             "http://geontime.com/geonkick"))
         (ins-channel
           (track-get-channel ins-track))
         (ins
           (channel-get-instrument ins-channel))
         (ins-mono-out
           (plugin-get-out-port ins 1))
         (fx-track
           (create-audio-fx-track prj))
         (fx-processor
           (track-get-processor fx-track))
         (fx-stereo-in
           (track-processor-get-stereo-in
             fx-processor))
         (fx-stereo-in-l
           (stereo-ports-get-port fx-stereo-in #t))
         (action
           (port-connection-action-new-connect
             ins-mono-out fx-stereo-in-l)))
        (undo-manager-perform undo_mgr action)
      )
    )
  )
)
