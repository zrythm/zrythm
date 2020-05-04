;;;  Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
;;; Generate gtk.gresources.xml
;;;
;;; Usage: gen-gtk-gresources-xml SRCDIR_GTK [OUTPUT-FILE]

(add-to-load-path "@SCRIPTS_DIR@")

(define-module (gen-gtk-resources-xml)
  #:use-module (guile-utils)
  #:use-module (ice-9 format)
  #:use-module (ice-9 match)
  #:use-module (ice-9 ftw))

(define breeze-icon-list
  '("amarok_scripts"
    "application-ogg"
    "application-msword"
    "application-x-m4"
    "application-x-zerosize"
    "audio-midi"
    "audio-x-mpeg"
    "audio-x-flac"
    "audio-volume-high"
    "audio-headphones"
    "audio-x-wav"
    "audio-card"
    "configure"
    "document-edit"
    "document-decrypt"
    "document-properties"
    "draw-line"
    "dialog-messages"
    "document-new"
    "document-open"
    "document-save"
    "document-save-as"
    "document-send"
    "draw-eraser"
    "edit-find"
    "edit-select"
    "emblem-symbolic-link"
    "edit-undo"
    "edit-redo"
    "edit-cut"
    "edit-copy"
    "edit-none"
    "edit-paste"
    "edit-delete"
    "edit-select-all"
    "edit-clear"
    "distribute-graph-directed"
    "filename-dot-amarok"
    "folder"
    "folder-favorites"
    "format-justify-fill"
    "list-add"
    "list-remove"
    "help-about"
    "input-keyboard"
    "kdenlive-show-markers"
    "transform-move"
    "media-seek-backward"
    "media-seek-forward"
    "media-playlist-repeat"
    "media-record"
    "media-playback-start"
    "media-playback-stop"
    "media-optical-audio"
    "media-album-track"
    "minuet-chords"
    "media-repeat-album-amarok"
    "insert-math-expression"
    "music-note-16th"
    "news-subscribe"
    "node-type-cusp"
    "network-connect"
    "plugins"
    "selection-end-symbolic"
    "select-rectangular"
    "show-menu"
    "snap-extension"
    "system-help"
    "taxes-finances"
    "text-x-csrc"
    "tools-report-bug"
    "view-fullscreen"
    "view-list-text"
    "view-media-visualization"
    "view-visible"
    "view-hidden"
    "window-close"
    "window-minimize"
    "window-pin"
    "zoom-in"
    "zoom-out"
    "zoom-fit-best"
    "zoom-original"))

(define (remove-prefix str prefix)
  (if (string-prefix? prefix str)
    (string-drop str (string-length prefix))
    str))

(define (handle-breeze-icon
          resources-dir full-src-dir
          full-alias-dir src-dir file)
  (let
    ((icon-path
       (join-path (list full-src-dir file))))
    (for-each
      (lambda (prefix)
        (let ((new-alias
                (join-path
                  (list
                    full-alias-dir
                    (string-append prefix file)))))
          (display
            (string-append
              "    <file alias=\"" new-alias
              "\">"
              (remove-prefix
                icon-path
                (string-append
                  src-dir
                  file-name-separator-string))
              "</file>"))
          (newline)))
      '("zbreeze-" ""))))

#!
Args:
1: resources dir
2: output file
!#
(define (main . args)

  ;; verify number of args
  (unless (eq? (length args) 3)
    (display "Need 2 arguments")
    (newline)
    (exit -1))

  ;; get args
  (match args
    ((this-program resources-dir output-file)

     ;; open file
     (with-output-to-file output-file
       (lambda ()

         (display
"<!--
  Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>

  This file is part of Zrythm

  Zrythm is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Zrythm is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
-->
<?xml version='1.0' encoding='UTF-8'?>
<gresources>
  <gresource prefix='/org/gtk/libgtk'>
    <file>theme/Matcha-dark-sea/gtk.css</file>
    <file>theme/Matcha-dark-sea/gtk-dark.css</file>
")

         ;; print matcha image assets
         (let ((dark-sea-assets-dir
                 "theme/Matcha-dark-sea/assets"))
           (for-each
             (lambda (x)
               (display
                 (string-append
                   "    <file>"
                   dark-sea-assets-dir "/" x
                   "</file>"))
               (newline))
             (scandir
               (join-path
                 (list resources-dir
                       dark-sea-assets-dir))
               (lambda (f)
                 (or
                   (string-suffix? ".png" f)
                   (string-suffix? ".svg" f))))))

         (display
"  </gresource>
  <gresource prefix='/org/zrythm/Zrythm/app'>
")

         ;; Print UI files
         (for-each
           (lambda (x)
             (display
               (string-append
                 "    <file preprocess="
                 "'xml-stripblanks'>ui/" x
                 "</file>"))
             (newline))
           (scandir
             (join-path
               (list resources-dir "ui"))
             (lambda (f)
               (string-suffix? ".ui" f))))

         ;; add icons except breeze
         (for-each
           (lambda (dir)
             (for-each
               (lambda (icon-file)
                 (display
                   (string-append
                     "    <file alias=\"icons/"
                     dir "/" dir "-" icon-file
                     "\">icons/"
                     dir "/" icon-file
                     "</file>"))
                 (newline)
                 (display
                   (string-append
                     "    <file>icons/"
                     dir "/" icon-file
                     "</file>"))
                 (newline))
               (scandir
                 (join-path
                   (list resources-dir "icons" dir))
                 (lambda (f)
                   (or
                     (string-suffix? ".svg" f)
                     (string-suffix? ".png" f))))))
           '("gnome-builder" "ext"
             "fork-awesome" "font-awesome"))

         ;; add breeze icons
         (for-each
           (lambda (cat)
             (for-each
               (lambda (size)
                 (let*
                   ((src-dir
                      (string-append cat "/" size))
                    (alias-dir
                      (if (equal? size "symbolic")
                        (string-append
                          "16x16/" cat)
                        (string-append
                          size "x" size "/" cat)))
                    (full-src-dir
                      (join-path
                        (list "icons" "breeze-icons"
                               src-dir)))
                    (full-alias-dir
                      (join-path
                        (list "icons" "breeze-icons"
                              alias-dir))))
                   (when
                     (file-exists?
                       (join-path
                         (list resources-dir
                               full-src-dir)))
                     (for-each
                       (lambda (icon-name)
                         (when
                           (file-exists?
                             (string-append
                               (join-path
                                 (list
                                   resources-dir
                                   full-src-dir
                                   icon-name))
                               ".svg"))
                           (handle-breeze-icon
                             resources-dir
                             full-src-dir
                             full-alias-dir
                             src-dir
                             (string-append
                               icon-name ".svg"))))
                       breeze-icon-list))))
               '("12" "16" "22" "24" "32" "64"
                 "symbolic")))
           '("actions" "animations" "applets" "apps"
             "categories" "devices" "emblems" "emotes"
             "mimetypes" "places" "preferences"
             "status"))

         ;; add theme and close
         (display
"  </gresource>
</gresources>"))))))

(apply main (program-arguments))
