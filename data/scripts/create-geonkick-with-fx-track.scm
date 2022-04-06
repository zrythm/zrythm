;;; Create Geonkick with FX track
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
  (track-set-muted geonkick-track #t))
