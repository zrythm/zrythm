;;; This file is in the public domain (CC0).
;;;
;;; An example script for generating projects or
;;; project templates.

(use-modules (project)
             (zrythm))

(define zrythm-script
  (lambda ()
    (display "script called")
    (newline)
    (display (zrythm-get-ver))
    (newline)
    (let* ((prj (zrythm-get-project))
           (prj-title (project-get-title prj)))
      (display prj-title)
      (newline))))
