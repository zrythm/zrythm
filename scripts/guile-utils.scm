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
  #:use-module (srfi srfi-1)
  #:use-module (srfi srfi-98)
  #:export (file-name-sep-char
            join-path
            getenv-or-default
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
