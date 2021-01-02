;;; This file is in the public domain (CC0).
;;;
;;; An example script for generating projects or
;;; project templates.

(use-modules
  (actions create-tracks-action)
  (actions undo-manager)
  (audio tracklist)
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
    (undo-manager-perform undo-manager action)))

(define zrythm-script
  (lambda ()
    (display (zrythm-get-ver))
    (newline)
    (let* ((prj (zrythm-get-project)))
      (display (project-get-title prj))
      (newline)
      (create-midi-track prj))))
