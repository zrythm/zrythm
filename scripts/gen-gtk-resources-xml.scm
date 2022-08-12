;;; SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
;;; SPDX-License-Identifier: LicenseRef-ZrythmLicense
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

(define (remove-prefix str prefix)
  (if (string-prefix? prefix str)
    (string-drop str (string-length prefix))
    str))

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
  Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>

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
")
         (display
"  </gresource>
  <gresource prefix='/org/zrythm/Zrythm/app'>
")
         ;; print GL shaders
         (let ((gl-shaders-dir "gl/shaders"))
           (for-each
             (lambda (x)
               (display
                 (string-append
                   "    <file>"
                   gl-shaders-dir "/" x
                   "</file>"))
               (newline))
             (scandir
               (join-path
                 (list resources-dir
                       gl-shaders-dir))
             (lambda (f)
               (string-suffix? ".glsl" f)))))

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
                 (newline))
               (scandir
                 (join-path
                   (list resources-dir "icons" dir))
                 (lambda (f)
                   (or
                     (string-suffix? ".svg" f)
                     (string-suffix? ".png" f))))))
           '("arena" "gnome-builder" "gnome-icon-library" "ext"
             "fork-awesome" "font-awesome"
             "fluentui" "jam-icons"))

         ;; insert standard gtk menus
         ;; (see GtkApplication docs)
         (display
"  </gresource>
  <gresource prefix='/org/zrythm/Zrythm'>
    <file preprocess='xml-stripblanks'>gtk/menus.ui</file>
    <file preprocess='xml-stripblanks'>gtk/help-overlay.ui</file>
  </gresource>
</gresources>"))))))

(apply main (program-arguments))
