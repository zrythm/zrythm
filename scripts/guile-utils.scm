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

(define-module (guile-utils)
  #:use-module (ice-9 popen)
  #:use-module (ice-9 rdelim)
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-98)
  #:export (file-name-sep-char
            file-to-string
            join-path
            invoke-with-fail
            get-command-output
            getenv-or-default
            get-cflags-from-pkgconf-name
            program-found?))

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

(define (program-found? program)
  (zero? (system* "which" program)))

(define (get-command-output cmd trim?)
  (let* ((port (open-input-pipe cmd))
         (str  (read-line port)))
    (close-pipe port)
    (if (eq? trim? #t)
      (string-trim-both str)
      str)))

(define (get-cflags-from-pkgconf-name pkgconf-name)
  (get-command-output
    (string-append
      "pkg-config --cflags-only-I " pkgconf-name)
    #t))

(define (file-to-string path)
  (let ((str ""))
    (with-input-from-file
      path
      (lambda ()
        (let loop ((x (read-char)))
          (when (not (eof-object? x))
            (begin
              (set! str
                (string-append
                  str (string x)))
              (loop (read-char)))))))
    str))

(define (invoke-with-fail args)
  (unless
    (zero? (apply system* args))
    (exit -1)))
