;;; Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
;;;
;;; This file is part of Zrythm
;;;
;;; Zrythm is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU Affero General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; Zrythm is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU Affero General Public License for more details.
;;;
;;; You should have received a copy of the GNU Affero General Public License
;;; along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

(define-module (zrythm post-install-script)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-98))

;; file name separator char
(define file-name-sep-char
  (string-ref file-name-separator-string 0))

;; join paths
;; TODO there's probably a built in function somewhere
(define (join-path parts)
  (fold
    (lambda (x accumulator)
      (string-append
        (string-trim-right
          accumulator file-name-sep-char)
        (if (eq? x (car parts))
          x
          (string-append
            file-name-separator-string
            (string-trim-both
              x file-name-sep-char)))))
    ""
    parts))

;; return the value of the given env variable or
;; return the given default value
(define (getenv-or-default name default)
  (or (getenv name) default))

;; define paths
(define prefix
  (let* ((suffix
           (getenv-or-default
             "MESON_INSTALL_PREFIX" "/usr/local"))
         (destdir (getenv "DESTDIR")))
    (if destdir
      (join-path (list destdir suffix))
      suffix)))
(define datadir
  (join-path (list prefix "share")))
(define schemadir
  (join-path (list datadir "glib-2.0" "schemas")))
(define fontsdir
  (join-path (list datadir "fonts" "zrythm")))
(define desktop-db-dir
  (join-path (list datadir "applications")))
(define mime-dir
  (join-path (list datadir "mime")))
(define doc-dir
  (join-path (list datadir "doc" "zrythm")))

(define (program-found? program)
  (zero? (system* "which" program)))

(define (main . args)
  (unless (getenv "DESTDIR")
    (display "Compiling gsettings schemas...")
    (newline)
    (system* "glib-compile-schemas" schemadir)
    (when (program-found? "gtk-update-icon-cache")
      (display "Updating icon cache...")
      (newline)
      (system*
        "touch"
        (join-path
          (list datadir "icons/hicolor")))
      (system* "gtk-update-icon-cache"))
    (when (program-found? "update-mime-database")
      (display "Updating MIME database...")
      (newline)
      (system* "update-mime-database" mime-dir))
    (when (program-found? "update-desktop-database")
      (display "Updating desktop database...")
      (newline)
      (unless (file-exists? desktop-db-dir)
        (mkdir desktop-db-dir))
      (system* "update-desktop-database" "-q"
               desktop-db-dir))
    (when (program-found? "update-gdk-pixbuf-loaders")
      (system "update-gdk-pixbuf-loaders"))))

(apply main (program-arguments))
