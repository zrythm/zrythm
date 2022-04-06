;;; Print all tracks
;;;
;;; This is an example GNU Guile script using modules provided by Zrythm.
;;; See https://www.gnu.org/software/guile/ for more info about GNU Guile.
;;; See https://manual.zrythm.org/en/scripting/intro.html for more info
;;; about scripting in Zrythm.
(use-modules (audio track)
             (audio tracklist)
             (project)
             (zrythm))
(let* ((prj (zrythm-get-project))
       (tracklist (project-get-tracklist prj))
       (num-tracks (tracklist-get-num-tracks tracklist)))
  (let loop ((i 0))
    (when (< i num-tracks)
      (let ((track (tracklist-get-track-at-pos tracklist i)))
        (display (track-get-name track))
        (newline))
      (loop (+ i 1)))))
