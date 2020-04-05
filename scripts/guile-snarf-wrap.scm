#!@GUILE@ -s
!#
;;; Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

(define-module (guile-snarf-wrap)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-98)
  #:use-module (ice-9 match)
  #:use-module (guile-utils))

(define (main . args)
  ;; get args
  (match args
    ((this-program guile-pkgconf-name
                   output-file input-file)
     (unless
       (zero?
         (system*
           "@GUILE_SNARF@"
           "-o" output-file input-file
           (get-cflags-from-pkgconf-name
             guile-pkgconf-name)))
       (exit -1)))))

(apply main (program-arguments))
