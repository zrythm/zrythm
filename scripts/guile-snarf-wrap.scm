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
  #:use-module (ice-9 popen)
  #:use-module (ice-9 rdelim)
  #:use-module (guile-utils))

;; returns the dependency cflags as a string
(define (get-dep-cflags deps-str)
  (fold
    (lambda (x accumulator)
      (let* ((port (open-input-pipe
                     (string-append
                       "pkg-config --cflags-only-I "
                       x)))
             (str  (read-line port)))
        (close-pipe port)
        (when (string? str)
          (string-append
            accumulator " " str))))
    ""
    (string-split deps-str #\ )))

(define (get-local-include root dir)
  (string-append
    "-I" (join-path (list root dir))))

(define (main . args)
  ;; get args
  (match args
    ((this-program meson-source-root
                   deps-str have-lilv
                   output-file input-file)
     (display (get-dep-cflags deps-str))
     (unless
       (zero?
         (system*
           "@GUILE_SNARF@"
           (get-local-include
             meson-source-root "inc")
           (get-local-include
             meson-source-root
             (join-path (list "ext" "zix")))
           (when
             (string=? have-lilv "false")
             (get-local-include
               meson-source-root
               (join-path
                 (list "subprojects" "lilv"
                       "lilv-0.24.6"))))
           (get-dep-cflags deps-str)
           "-o" output-file input-file))
       (exit -1)))))

(apply main (program-arguments))
