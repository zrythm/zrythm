;;;  Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
;;;
;;;  This file is part of Zrythm
;;;
;;;  Zrythm is free software: you can redistribute it and/or modify
;;;  it under the terms of the GNU Affero General Public License as published by
;;;  the Free Software Foundation, either version 3 of the License, or
;;;  (at your option) any later version.
;;;
;;;  Zrythm is distributed in the hope that it will be useful,
;;;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;  GNU Affero General Public License for more details.
;;;
;;;  You should have received a copy of the GNU Affero General Public License
;;;  along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
;;;
;;; Generate appdata.xml
;;;
;;; Usage: gen-appdata-xml [OUTPUT-FILE]

(add-to-load-path "@SCRIPTS_DIR@")

(define-module (gen-gtk-resources-xml)
  #:use-module (guile-utils)
  #:use-module (ice-9 format)
  #:use-module (ice-9 match)
  #:use-module (ice-9 ftw)
  #:use-module (sxml simple))

;; Returns a list of the last 4 releases
(define (get-releases)
  (let*
    ((changelog
       (file-to-string
         (join-path
           '("@MESON_SOURCE_ROOT@"
             "CHANGELOG.md"))))
     (changelog-list
       (cdr (string-split-substring changelog "## [")))
     (releases-list `()))
    (for-each
      (lambda (x)
        (let*
          ((ver (car (string-split-substring x "]")))
           (str-from-date
             (car
               (cdr
                 (string-split-substring x " - "))))
           (date (car (string-split-substring str-from-date "\n"))))
          (set!
            releases-list
            (append
              releases-list
              `((release
                  (@ (date ,date)
                     (version ,ver)
                     (type "development"))
                  (url
                    ,(string-append
                       "https://git.sr.ht/~alextee/zrythm/refs/v" ver))))))))
      changelog-list)
    (list-head releases-list 4)))

#!
Args:
1: output file
2: app ID
!#
(define (main . args)

  ;; verify number of args
  (unless (eq? (length args) 3)
    (display "Need 2 arguments")
    (newline)
    (exit -1))

  ;; get args
  (match args
    ((this-program output-file app-id)

     (with-output-to-file output-file
       (lambda ()

           ;; write XML
           (sxml->xml
            `(*TOP*
               (*PI*
                 xml
                 "version=\"1.0\" encoding=\"UTF-8\"")
               ;; TODO insert: Copyright 2022 Alexandros Theodotou
               (component
                 (id "org.zrythm.Zrythm")
                 ;; The tag 'metadata_license' means
                 ;; the licence of this file, not the
                 ;;whole product
                 (metadata_license "CC0-1.0")
                 (project_license  "AGPL-3.0-or-later")
                 (name "Zrythm")
                 (summary "Digital audio workstation")
                 (description
                   (p "Zrythm is a digital audio
  workstation designed to
  be featureful and easy to use. It allows
  limitless automation through curves, LFOs and
  envelopes, supports multiple plugin formats
  including LV2, VST2 and VST3, works with
  multiple backends including JACK, RtAudio/RtMidi
  and SDL2, assists with chord progressions via a
  special Chord Track and chord pads, and can be
  used in multiple languages including English,
  French, Portuguese, Japanese and German.")
                   (p "Zrythm can be extended using GNU Guile."))
                 (url
                   (@ (type "homepage"))
                   "@HOMEPAGE_URL@")
                 (url
                   (@ (type "bugtracker"))
                   "@BUG_REPORT_URL@")
                 ;; TODO faq
                 (url
                   (@ (type "help"))
                   "@USER_MANUAL_URL@")
                 (url
                   (@ (type "donation"))
                   "@DONATION_URL@")
                 (url
                   (@ (type "translate"))
                   "@TRANSLATE_URL@")
                 (url
                   (@ (type "contact"))
                   "@CONTACT_URL@")
                 (launchable
                   (@ (type "desktop-id"))
                   ,(string-append app-id
                                   ".desktop"))
                 (screenshots
                   (screenshot
                     (@ (type "default"))
                     (image
                       "@PIANO_ROLL_SCREENSHOT_URL@")
                     (caption
                       "Composing MIDI in Zrythm"))
                   (screenshot
                     (@ (type "default"))
                     (image
                       "@MIXER_SCREENSHOT_URL@")
                     (caption "Mixer view")))
                 (update_contact "alex_at_zrythm.org")
                 (keywords
                   (keyword "Zrythm")
                   (keyword "DAW"))
                 (kudos
                   (kudo "HiDpiIcon")
                   (kudo "ModernToolkit"))
                 (project_group "Zrythm")
                 (translation
                   (@ (type "gettext"))
                   "zrythm")
                 (provides
                   (binary "zrythm_launch"))

                 (releases
                   ,@(get-releases))

                 (content_rating
                   (@ (type "oars-1.1")))))))))))

(apply main (program-arguments))
