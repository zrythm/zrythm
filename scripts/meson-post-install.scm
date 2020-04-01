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

(add-to-load-path "@SCRIPTS_DIR@")

(define-module (meson-post-install)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-98)
  #:use-module (guile-utils))

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

(define (main . args)
  (unless (getenv "DESTDIR")
    (display "Compiling gsettings schemas...\n")
    (system* "glib-compile-schemas" schemadir)
    (when (program-found? "gtk-update-icon-cache")
      (display "Updating icon cache...\n")
      (system* "touch"
        (join-path (list datadir "icons/hicolor")))
      (system* "gtk-update-icon-cache"))
    (when (program-found? "update-mime-database")
      (display "Updating MIME database...\n")
      (system* "update-mime-database" mime-dir))
    (when (program-found? "update-desktop-database")
      (display "Updating desktop database...\n")
      (unless (file-exists? desktop-db-dir)
        (mkdir desktop-db-dir))
      (system* "update-desktop-database" "-q"
               desktop-db-dir))
    (when (program-found? "update-gdk-pixbuf-loaders")
      (system "update-gdk-pixbuf-loaders"))))

(apply main (program-arguments))
